//-*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// vim: noai:ts=2:sw=2:expandtab
//
// Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>
//
#include "CodeTree.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <set>
#include <regex>
#include <memory>
#include <functional>
#include <filesystem>

#include "assert.h"
#include "stdio.h"

#include "FunctionWrapper.h"
#include "libclang-ext.h"
#include "FileTimeRestorer.h"
#include "utils.h"
#include "cxxwrap_version.h"

extern const char* version;

#include "md5sum.h"

#include "clang/AST/Type.h" //FOR DEBUG
#include "clang/AST/DeclTemplate.h"

#include <dlfcn.h>

//The general strategy for class wrapper declaration is to first declare all
//classes (add_type() calls) and in a second step declare the wrapper for
//the methods of every class (add_method() calls).
//For template class, declaration of type and method cannot be fully
//decoupled: see https://github.com/JuliaInterop/libcxxwrap-julia/issues/138
//
//With the macro DEFINE_TEMPLATE_METHODS_IN_CTOR defined, wrappers for
//methods of template classes are defined in the type wrapper declaration
//step and type wrapper declaration are sorted such that the types required
//by these methods are defined first. This ordering is not always possible
//because C++ allows cyclic dependencies for type pointers handled byforward
//declaration.
//
//When it is not defined, the same two-step scheme as for non-template class
//is used with no type wrapper declaration reordering. Nevertheless, it misses
//declaration to CxxWrap of the class specializations (call to the apply()
//function), which cannnot be decoupled from method declaration with current
//CxxWrap version. The consequence is that a "missing factory" error message
//on the Julia wrapper module load is likely to happen.
//
//We keep the two code versions with this switch in order to ease code update if
//support support of type and method declaration decouploing is added to
//CxxWrap.
#define DEFINE_TEMPLATE_METHODS_IN_CTOR

extern CXPrintingPolicy pp;

namespace{
  void diagnose(const fs::path& p, const CXTranslationUnit& tu){
    unsigned diagnosticCount = clang_getNumDiagnostics(tu);

    for(unsigned i = 0; i < diagnosticCount; i++) {
      CXDiagnostic diagnostic = clang_getDiagnostic(tu, i);
      CXSourceLocation location = clang_getDiagnosticLocation(diagnostic);
      CXString text = clang_getDiagnosticSpelling(diagnostic);
      std::cerr << location <<  ": " << text << "\n";
    }
  }
}

using namespace codetree;
namespace codetree{
  static std::string to_julia_name(const std::string& cpp_name){
    //Replace all '::' occurences by '!':
    return std::regex_replace(cpp_name, std::regex("::"), "!");
  }
}

std::tuple<CXCursor, std::vector<CXCursor>>
CodeTree::getParentClassesForWrapper(CXCursor cursor) const{
  if(verbose > 5) std::cerr << "Calling getParentClassForWrapper("
                            << cursor << ")\n";

  auto null_cursor = clang_getNullCursor();
  auto clazz = str(clang_getTypeSpelling(clang_getCursorType(cursor)));

  struct data_t{
    const CodeTree* tree;
    std::string clazz;
    CXCursor main_parent;
    std::vector<CXCursor> extra_parents;
    bool inheritance_preference;
    std::string preferred_parent;
  } data = { this, clazz, null_cursor, std::vector<CXCursor>(), false, "" };

  //FIXME: handling of namespace?
  auto it = inheritance_.find(clazz);
  if(it != inheritance_.end()){
    data.preferred_parent = it->second;
    data.inheritance_preference = true;
  }

  const auto & visitor = [](CXCursor cursor, CXCursor parent, CXClientData p_data){
    data_t& data = *static_cast<data_t*>(p_data);
    const CodeTree& tree = *data.tree;

    const auto& kind = clang_getCursorKind(cursor);
    if(kind == CXCursor_CXXBaseSpecifier){
      std::string parent_name = str(clang_getTypeSpelling(clang_getCursorType(cursor)));
      const auto& inheritance_access = clang_getCXXAccessSpecifier(cursor);
      const auto& t1 = clang_getCursorType(cursor);
      if(inheritance_access == CX_CXXPublic){
        bool isBaseWrapped = false;
        for(const auto& c: tree.types_){
          //FIXME: support for templates
          const auto& t2 = clang_getCursorType(c.cursor);
          if(same_type(t1, t2)){
            isBaseWrapped = true;
            break;
          }
        }

        if(str(clang_getTypeSpelling(t1)) == "std::string"){
          isBaseWrapped = true;
        }

        if(isBaseWrapped){
          //FIXME: handling of parent namespace?
          if(clang_Cursor_isNull(data.main_parent)
             && (!data.inheritance_preference
                 || (str(clang_getTypeSpelling(clang_getCursorType(cursor)))
                     == data.preferred_parent))){
            data.main_parent = cursor; // clang_getCursorDefinition(cursor);
          } else{
            data.extra_parents.push_back(cursor); //clang_getCursorDefinition(cursor));
          }
        }
      }
    }
    return CXChildVisit_Continue;
  };

  clang_visitChildren(cursor, visitor, &data);

  if(clang_Cursor_isNull(data.main_parent) && data.inheritance_preference
     && data.preferred_parent.size() > 0){
    std::cerr << "Inheritance preference " << clazz << ":" << data.preferred_parent
              << " cannot be fulfilled because no such inheritance has been found.\n";
  }

  data.main_parent = clang_getCursorDefinition(data.main_parent);

  if(!clang_Cursor_isNull(data.main_parent) && get_template_parameters(data.main_parent).size() > 0){
    if(verbose > 2){
      std::cerr << "Warning: inheritance " << clazz << " <: "
                << data.main_parent << " will not be mapped as super type because it is not "
        "yet supported for template classes. Method inherited in C++ will be "
        "explicitly defined in the Julia wrapper.\n";
    }

    data.extra_parents.insert(data.extra_parents.begin(), data.main_parent);
    data.main_parent = clang_getNullCursor();
  }

    return std::make_tuple(data.main_parent, data.extra_parents);
}

std::string CodeTree::wrapper_classsname(const std::string& classname) const{
  std::regex r("::");
  std::string s = std::string("Jl") + (classname.size() == 0 ? "Global" : std::regex_replace(classname, r, "_"));
  return s;
}

std::ostream&
CodeTree::generate_version_check_cxx(std::ostream& o) const{
  auto [sjllmin, sjllmax] = version_libcxxwrap_static_bounds(cxxwrap_version_);

  indent(o,0) << "//method from libcxxwrap returning its version\n";
  indent(o,0) << "extern \"C\" JLCXX_API const char* cxxwrap_version_string();\n\n";
  indent(o, 0) << "//Check the code is compiled with a compatible version of libcxxwrap:\n";
  indent(o,0) << "static_assert(1000*1000*JLCXX_VERSION_MAJOR "
    " + 1000 * JLCXX_VERSION_MINOR + JLCXX_VERSION_PATCH >= " << sjllmin << "\n";
  indent(o, 1) << "&& 1000 * 1000 * JLCXX_VERSION_MAJOR "
    " + 1000 * JLCXX_VERSION_MINOR + JLCXX_VERSION_PATCH < " << sjllmax << ",\n";
  indent(o,1) << "\"The code was generated with WrapIt! for \"\n";
  indent(o,1) << "\"a different CxxWrap version (controlled with the cxxwrap_version parameter).\");\n\n";
  indent(o, 0) << "//Check the version of loaded libcxxrap library:\n";
  indent(o,0) << "void throw_if_version_incompatibility(){\n";
  indent(o,1) <<   "std::string version_str = cxxwrap_version_string();\n";
  indent(o,1) <<   "static std::regex r(\"([[:digit:]]{1,3})\\\\.([[:digit:]]{1,3})\\\\.([[:digit:]]{1,3})\");\n";
  indent(o,1) <<   "std::smatch matches;\n";
  indent(o,1) <<   "if(!std::regex_match(version_str, matches, r)){\n";
  indent(o,2) <<     "std::cerr << \"Warning: Failed to check libcxxwrap version.\";\n";
  indent(o,1) <<   "} else{";
  indent(o,2) <<     "long version_int =   1000*1000*strtol(matches[1].str().c_str(), 0, 10)\n";
  indent(o,2) <<     "                   +      1000*strtol(matches[2].str().c_str(), 0, 10)\n";
  indent(o,2) <<     "                   +           strtol(matches[3].str().c_str(), 0, 10);\n";
  indent(o,2) <<     "long jllmin = 1000*1000*JLCXX_VERSION_MAJOR + 1000 * JLCXX_VERSION_MINOR;\n";
  indent(o,2) <<     "long jllmax = 1000*1000*JLCXX_VERSION_MAJOR + 1000 * (JLCXX_VERSION_MINOR + 1);\n";
  indent(o,2) <<     "if(version_int < jllmin || version_int >= jllmax){\n";
  indent(o,3) <<     "throw std::runtime_error(std::string(\"Found libcxxwrap_jll version \")\n";
  indent(o,4) <<       "+ version_str + \", while module " << module_name_ << " requires version \"\n";
  indent(o,4) <<       "+ std::to_string(JLCXX_VERSION_MAJOR) + \".\" + std::to_string(JLCXX_VERSION_MINOR) + \".x.\"\n";
  indent(o,4) <<       "\" Note: if the module was installed with the package manager, the Project.toml file \"\n";
  indent(o,4) <<       "\"of the package is probably missing a compat specification that would have prevented \"\n";
  indent(o,4) <<       "\"the inconsistency.\");\n";
  indent(o,2) <<     "}\n";
  indent(o,1) <<   "}\n";
  indent(o,0) << "}\n";

//  indent(o, 1) << "long libcxxwrap_vers = 1000*100* JLCXX_VERSION_MAJOR + 1000 * JLCXX_VERSION_MINOR + JLCXX_VERSION_PATCH;\n";
//  indent(o, 1) << "if(libcxxwrap_vers < " << jllmin << " || libcxxwrap_vers >= " << jllmax << "){\n";
//  indent(o, 2) << "throw std::runtime_error(\"Found libcxxwrap_jll version \" "
//               << " JLCXX_VERSION_STRING \", module " << module_name_
//               << " needs a vesion in ["
//               << version_int_to_string(jllmin) << ", "
//               << version_int_to_string(jllmax) << ")"
//    ". Note: if the module was installed with the package manager, the Project.toml file of the package is probably missing a compat specification that would have prevented the inconsistency.\");\n";
//  indent(o, 1) << "}\n\n";
  return o;
}

std::ostream&
CodeTree::generate_template_add_type_cxx(std::ostream& o,
                                         const TypeRcd& type_rcd,
                                         std::string& add_type_param){

  if(verbose > 3) std::cerr << __FUNCTION__ << "("
                            << "..." << ", "
                            << "{cursor = " << type_rcd.cursor << ",..}" << ")\n";

  //FIXME: support for operators

  //Example
  //
  //Class to wrap:
  //  template<typename T1, typename T2> class A;
  //  template class A<P1, P2>;
  //  template class A<P3, P4>;
  //
  //Code to generate:
  //  t1 = types.add_type<Parametric<TypeVar<1>, TypeVar<2>>>("A")
  //  .apply<A<P1, P2>, A<P3, P4>([](auto){});
  //
  const auto& cursor = type_rcd.cursor;
  const auto& type = clang_getCursorType(cursor);
  const auto& typename_cxx = type_rcd.type_name;
  const auto& typename_jl = jl_type_name(typename_cxx);
  int id = type_rcd.id;


  //  if(is_type_vetoed(typename_cxx)){
  //    std::cerr << "Info: class/struct " << typename_cxx << " vetoed\n";
  //    return o;
  //  }

  if(type_rcd.template_parameter_combinations.empty()){
    std::cerr << "Warning: no specialization found for template class "
              << typename_cxx
              << " and no wrapper will be generated for this class.\n";
    return o;
  }

  ++nwraps_.types;
  ++nwraps_.type_templates;

  indent(o, 2) << "DEBUG_MSG(\"Adding wrapper for type " << typename_cxx
               << " (\" __HERE__ \")\");\n";
  indent(o, 2) << "// defined in "   << clang_getCursorLocation(cursor) << "\n";


  std::stringstream buf;
  buf << "jlcxx::Parametric<";
  //TypeVar<1>, TypeVar<2>
  const unsigned nparams = type_rcd.template_parameter_combinations[0].size();
  const char* sep = "";
  for(unsigned i = 1; i <= nparams; ++i){
    buf << sep << "jlcxx::TypeVar<" << i << ">";
    sep = ", ";
  }
  buf << ">";
  add_type_param = buf.str();

  //types.add_type<Parametric<...>("TemplateType")
  indent(o, 2) << "jlcxx::TypeWrapper<" << add_type_param << ">  t = "
               << " jlModule.add_type<" << add_type_param
               << ">(\""    << typename_jl << "\");\n";

  gen_apply_stl(o, 2, type_rcd, add_type_param);

  indent(o, 2) <<  "type_ = std::unique_ptr<jlcxx::TypeWrapper<"
               << add_type_param << ">>(new jlcxx::TypeWrapper<"
               << add_type_param << ">(jlModule, t));\n";

#ifdef DEFINE_TEMPLATE_METHODS_IN_CTOR
  if(type_rcd.default_ctor){
    FunctionWrapper::gen_ctor(o, 2, "t", type_rcd.template_parameters.empty(),
                              type_rcd.finalize, std::string(), std::string(),
                              cxxwrap_version_);
  }
#endif

  generate_methods_of_templated_type_cxx(o, type_rcd);

  if(export_mode_ == export_mode_t::all) to_export_.insert(typename_jl);

  return o;
}

std::ostream&
CodeTree::generate_cxx_for_type(std::ostream& o,
                                const TypeRcd& t){

  //if true, fake type used to hold global functions
  bool notype = t.type_name.size() == 0;

  bool no_copy_ctor = find(copy_ctor_to_veto_.begin(),
                           copy_ctor_to_veto_.end(), t.type_name) != copy_ctor_to_veto_.end();

  if(!notype){
    o << "\nnamespace jlcxx {\n";
    //generate code that disables mirrored type
    if(verbose > 2) std::cerr << "Disable mirrored type for type " << t.type_name << "\n";
    if(t.template_parameter_combinations.size() > 0){
      auto nparams = t.template_parameters.size();
      std::vector<std::string> param_list;
      for(decltype(nparams) i = 0; i < nparams; ++i){
        param_list.emplace_back(t.template_parameter_types[i] + " " + t.template_parameters[i]);
      }
      auto param_list1 = join(param_list, ", ");
      auto param_list2 = join(t.template_parameters, ", ");
      o << "\n";
      indent(o, 1) << "template<" << param_list1 << ">\n";
      indent(o, 1) << "struct BuildParameterList<" << t.type_name << "<" << param_list2 << ">>\n";
      indent(o, 1) << "{\n";
      indent(o, 2) << "typedef ParameterList<";
      const char* sep = "";
      for(decltype(nparams) i = 0; i < nparams; ++i){
        if (t.template_parameter_types[i] != "typename") {
          o << sep << "std::integral_constant<" << t.template_parameter_types[i] << ", " << t.template_parameters[i] << ">";
          sep = ", ";
        } else {
          o << sep << t.template_parameters[i];
          sep = ", ";
        }
      }
      o << "> type;\n";
      indent(o,1) << "};\n\n";
      indent(o, 1) << "template<" << param_list1 << "> struct IsMirroredType<" << t.type_name << "<" << param_list2 << ">> : std::false_type { };\n";
      indent(o, 1) << "template<" << param_list1 << "> struct DefaultConstructible<" << t.type_name << "<" << param_list2 << ">> : std::false_type { };\n";
      if(no_copy_ctor){
        indent(o, 1) << "template<" << param_list1 << "> struct CopyConstructible<" << t.type_name << "<" << param_list2 << ">> : std::false_type { };\n";
      }
    } else{
      indent(o, 1) << "template<> struct IsMirroredType<" << t.type_name << "> : std::false_type { };\n";
      indent(o, 1) << "template<> struct DefaultConstructible<" << t.type_name << "> : std::false_type { };\n";
      if(no_copy_ctor){
        indent(o, 1) << "template<> struct CopyConstructible<" << t.type_name << "> : std::false_type { };\n";
      }
    }

    auto [base, extra_parents] = getParentClassesForWrapper(t.cursor);
    if(verbose > 4){
      std::cerr << "Debug: parent of " << t.cursor << ": "
                << (clang_Cursor_isNull(base) ?
                      "none"
                    : str(clang_getCursorSpelling(base)))
                << "\n";
    }
    if(!clang_Cursor_isNull(base)){
      if(t.template_parameters.size() > 0){
        if(verbose > 0){
          //getParentClassesForWrapper is expected to put template class inheritances
          //exclusivelt in extra_parents
          std::cerr << "Bug found. Methods inherited from " << base << " by " << t.type_name
                    << "won't be wrapped due to a bug in " << __FUNCTION__
                    << ", " << __FILE__ << ":" << __LINE__ << "].\n";
        }
      } else{
        indent(o, 1) << "template<> struct SuperType<"
                     << t.type_name
                     << "> { typedef " << fully_qualified_name(base) << " type; };\n";
      }
    }
      o << "}\n\n";
  }

  std::string wrapper = wrapper_classsname(t.type_name);

  o << "// Class generating the wrapper for type " << t.type_name << "\n"
    << "// signature to use in the veto file: "<< t.type_name << "\n";
  o << "struct " << wrapper << ": public Wrapper {\n\n";

  std::string add_type_param;
  //wrapper ctor
  indent(o, 1) << wrapper << "(jlcxx::Module& jlModule): Wrapper(jlModule){\n";
  if(!notype){
    if(t.template_parameter_combinations.size() > 0){
      generate_template_add_type_cxx(o, t, add_type_param);
    } else if(clang_getCursorKind(t.cursor)!= CXCursor_ClassTemplate){
      generate_non_template_add_type_cxx(o, t, add_type_param);
    }

    ////  indent(o, 2) << "jlcxx::TypeWrapper t = module.add_type<"
    ////               << t.type_name << ">(" << jl_type_name(t.type_name) <<");\n";
    ////  indent(o, 2) << "type_ = std::unique_ptr<jlcxx::TypeWrapper<" << t.type_name << ">>"
    ////               << "(new jlcxx::TypeWrapper<" << t.type_name << ">(module, t));\n";
  }
  indent(o, 1) << "}\n\n";

#ifdef DEFINE_TEMPLATE_METHODS_IN_CTOR
  bool add_method_body_moved_to_ctor = (!notype) && t.template_parameter_combinations.size() > 0;
#else
  bool add_method_body_moved_to_ctor = false;
#endif

  //add_methods method
  indent(o, 1) << "void add_methods() const{\n";
  if(!add_method_body_moved_to_ctor){
    if(notype){
      indent(o, 2) << "auto& t = module_;\n";
    } else{
      indent(o, 2) << "auto& t = *type_;\n";
    }

    if(!notype && clang_getCursorKind(t.cursor)!= CXCursor_ClassTemplate){
      //extract ctor method name from the class name after
      //removing the possible namespace specification:
      std::regex re(".*::([^:]*)|[^:]*");
      std::cmatch cm;
      std::regex_match(t.type_name.c_str(), cm, re);
      std::string ctor(cm[1]);

      //Generate a wrapper for the implicit default ctor if needed
      if(t.default_ctor){
        FunctionWrapper::gen_ctor(o, 2, "t", t.template_parameters.empty(),
                                  t.finalize, std::string(), std::string(),
                                  cxxwrap_version_);
      }
    }

    //Generate wrappers of the other methods
    //FIXME: add generation of accessor for templated classes
    //
    if(notype || clang_getCursorKind(t.cursor)!= CXCursor_ClassTemplate){

      //Generate wrappers for the class methods
      for(const auto& m: get_methods_to_wrap(t)){
        generate_method_cxx(o, t, m);
      }

      if(override_base_){
        indent(o << "\n", 2) << "module_.unset_override_module();\n";
        override_base_ = false;
      }

      //Generate class field accessors
      if(accessor_generation_enabled()){
        for(const auto& f: t.fields){
          auto accessor_gen = check_veto_list_for_var_or_field(f, false);
          if(accessor_gen != accessor_mode_t::none){
            //skip field with anomymous struct types:
            bool anonymous = false;
            CXType field_type = clang_getCursorType(f);
            if(field_type.kind != CXType_Invalid){
              auto def = clang_getTypeDeclaration(field_type);
              if(!clang_Cursor_isNull(def) && clang_Cursor_isAnonymous(def)){
                anonymous = true;
              }
            }
            if(anonymous){
              if(verbose > 1){
                std::cerr << "No accessor generated for field " << f
                          << " because its type is anonymous.\n";
              }
            } else{
              generate_accessor_cxx(o, &t, f, accessor_gen == accessor_mode_t::getter, 2);
            }
          }
        }
      }
    } else if(t.template_parameter_combinations.size() > 0){
      generate_methods_of_templated_type_cxx(o, t);
      indent(o, 2) << "//for parametric types methods are added in the ctor\n";
      //FIXME: add class field accessors.
    }
  }  //!add_method_body_moved_to_ctor
  indent(o, 1) << "}\n";

  if(!notype){
    o << "\nprivate:\n";
    indent(o, 1) <<  "std::unique_ptr<jlcxx::TypeWrapper<" << add_type_param << ">> type_;\n";
  }

  o << "};\n";

  o << "std::shared_ptr<Wrapper> new" << wrapper << "(jlcxx::Module& module){\n";
  indent(o, 1) << "return std::shared_ptr<Wrapper>(new " << wrapper << "(module));\n";
  o << "}\n";

  return o;
}

std::ostream&
CodeTree::generate_non_template_add_type_cxx(std::ostream& o,
                                             const TypeRcd& type_rcd,
                                             std::string&  add_type_param){

  const auto& cursor = type_rcd.cursor;
  const auto& type = clang_getCursorType(cursor);
  add_type_param= type_rcd.type_name;

  if(add_type_param.size() == 0){
    std::cerr << "Bug found. Unexpected empty string for TypeRcd::type_name of cursor "
              << "'" << type_rcd.cursor << "' defined in "
              << clang_getCursorLocation(type_rcd.cursor)
              << "\n";
  }

  const auto& typename_jl = jl_type_name(type_rcd.type_name);
  if(export_mode_ == export_mode_t::all) to_export_.insert(typename_jl);

  ++nwraps_.types;
  indent(o, 2) << "DEBUG_MSG(\"Adding wrapper for type " << type_rcd.type_name
               << " (\" __HERE__ \")\");\n";
  indent(o, 2) << "// defined in "   << clang_getCursorLocation(cursor) << "\n";


  indent(o, 2) << "jlcxx::TypeWrapper<" << add_type_param << ">  t = "
               << "jlModule.add_type<" << add_type_param
               << ">(\"" << typename_jl << "\"";

  auto [ base, extra_parents]  = getParentClassesForWrapper(cursor);

  if(!clang_Cursor_isNull(base)){
    const auto& base_type = clang_getCursorType(base);
    const auto& base_name_cxx = str(clang_getTypeSpelling(base_type));

    indent(o << ",\n", 3) << "jlcxx::julia_base_type<"
             << base_name_cxx << ">()";
  }

  o << ");\n";

  gen_apply_stl(o, 2, type_rcd, add_type_param);

  indent(o, 2) <<  "type_ = std::unique_ptr<jlcxx::TypeWrapper<"
               << add_type_param << ">>(new jlcxx::TypeWrapper<"
               << add_type_param << ">(jlModule, t));\n";

  return o;
}


void
CodeTree::generate_cxx(){

  reset_wrapped_methods();

  //default filename for type wrapper code:
  std::string type_out_fname = std::string("jl") + module_name_ + ".cxx";

  std::string p(join_paths(out_cxx_dir_, type_out_fname));
  FileTimeRestorer otimerstore(p);
  std::ofstream o(p, out_open_mode_);
  if(o.tellp() != 0){
    std::cerr << "File " << p
              << " in the way. Remove it or use the --force option.\n";
    //Cleanup:
    if(header_file_path_.size() > 0 ) fs::remove(header_file_path_.c_str());
    exit(1);
  }

  o << "// this file was auto-generated by wrapit " << version << "\n";

  o << //"#include <type_traits>\n"
    "#include \"jlcxx/jlcxx.hpp\"\n"
    "#include \"jlcxx/functions.hpp\"\n"
    "#include \"jlcxx/stl.hpp\"\n\n";

  o << "#include \"jl" << module_name_ << ".h\"\n\n"
    "#include <regex>\n\n";

  //FIXME
  //  for(const auto& t: types_missing_def_){
  //    o << "static_assert(is_type_complete_v<" << t
  //      << ">, \"Please include the header that defines the type "
  //      << t << "\");\n";
  //  }

  o << "#include \"dbg_msg.h\"\n";
  o << "#include \"Wrapper.h\"\n";

  std::vector<std::string> wrappers;

  //File stream to write type wrapper,
  //current file by default
  std::ofstream type_out;
  std::ofstream* pout = &o;

  int nignoredlines = 1;
  FileTimeRestorer timerestore;
  int i_towrap_type = -1;
  //  for(const auto& c: types_){
  for(auto i: types_sorted_indices_){
    auto& c = types_[i];
    if(!c.to_wrap) continue;
    ++i_towrap_type;

    c.setStrictNumberTypeFlags(type_map_);

    if(towrap_type_filenames_.size() > 0
       && towrap_type_filenames_[i_towrap_type] != type_out_fname){

      type_out_fname = towrap_type_filenames_.at(i_towrap_type);
      std::string type_out_fpath =
        join_paths(out_cxx_dir_, towrap_type_filenames_[i_towrap_type]);

      if(pout != &o){
        pout->close();
        if(update_mode_){
          timerestore.settimestamp();
        }
      }
      if(update_mode_) timerestore = FileTimeRestorer(type_out_fpath, nignoredlines);
      type_out = checked_open(type_out_fpath);
      pout = &type_out;
      generate_type_wrapper_header(*pout);
    }

    const auto& t = clang_getCursorType(c.cursor);

    if(is_type_vetoed(c.type_name)) continue;

    if(c.type_name.size() == 0 //holder of global functions and variables
       || t.kind == CXType_Record || c.template_parameter_combinations.size() > 0){
      wrappers.emplace_back(wrapper_classsname(c.type_name));
      generate_cxx_for_type((*pout), c);
    }
  }

  if(pout != (&o)){
    pout->close();
    timerestore.settimestamp();
  }

  o << "\n";
  for(const auto& w: wrappers){
    o << "class " << w << ";\n";
  }

  o << "\n";

  for(const auto& w: wrappers){
    o << "std::shared_ptr<Wrapper> new" << w << "(jlcxx::Module&);\n";
  }


  generate_version_check_cxx(o);

  o << "\n\nJLCXX_MODULE define_julia_module(jlcxx::Module& jlModule){\n";


  indent(o << "\n", 1) << "throw_if_version_incompatibility();\n\n";

  if(type_straight_mapping_.size() > 0){
    indent(o, 1) << "// mapping of types that are bit-to-bit matched\n";
    indent(o, 1) << "// WrapIt note: map is generated from the mapped_types list provided in\n";
    indent(o, 1) << "// the configuration file without further check..\n";
  }

  for(const auto& type_pair: type_straight_mapping_){
    indent(o, 1) << "jlModule.map_type<" << type_pair.first
                 << ">(\"" << type_pair.second << "\");\n";
  }

  for(const auto& e: enums_){
    generate_enum_cxx(o, e.cursor);
  }

  indent(o, 1) << "std::vector<std::shared_ptr<Wrapper>> wrappers = {\n";
  std::string sep;
  for(const auto& w: wrappers){
    indent(o << sep, 2) << "std::shared_ptr<Wrapper>(new" << w << "(jlModule))";
    sep = ",\n";
  }
  o << "\n";
  indent(o, 1) << "};\n";

  indent(o, 1) << "for(const auto& w: wrappers) w->add_methods();\n";

  o << "\n}\n";
  o.close();
  otimerstore.settimestamp();

  auto fname = join_paths(out_cxx_dir_, "dbg_msg.h");
  timerestore = FileTimeRestorer(fname);
  std::ofstream o2 = checked_open(fname);
  o2 << "#ifdef VERBOSE_IMPORT\n"
    "#  define DEBUG_MSG(a) std::cerr << a << \"\\n\"\n"
    "#else\n"
    "#  define DEBUG_MSG(a) if(getenv(\"WRAPIT_VERBOSE_IMPORT\")) std::cerr << a << \"\\n\"\n"
    "#endif\n"
    "#define __HERE__  __FILE__ \":\" QUOTE2(__LINE__)\n"
    "#define QUOTE(arg) #arg\n"
    "#define QUOTE2(arg) QUOTE(arg)\n";
  o2.close();
  timerestore.settimestamp();

  fname = join_paths(out_cxx_dir_, "Wrapper.h");
  timerestore = FileTimeRestorer(fname);
  o2 = checked_open(fname);
  o2 << "#include \"jlcxx/jlcxx.hpp\"\n\n"
    "struct Wrapper{\n";
  indent(o2, 1) << "Wrapper(jlcxx::Module& module): module_(module) {};\n";
  indent(o2, 1) << "virtual ~Wrapper() {};\n";
  indent(o2, 1) << "virtual void add_methods() const = 0;\n";
  o2 << "\nprotected:\n";
  indent(o2, 1) << "jlcxx::Module& module_;\n";
  o2 << "};\n";
  o2.close();
  timerestore.settimestamp();

  o2 = std::ofstream(join_paths(out_cxx_dir_, "generated_cxx"));
  o2 << "jl" << module_name_ << ".cxx";
  for(const auto& fname: towrap_type_filenames_set_){
    o2 << " " << fname;
  }
  o2.close();

  if(cmake_.size() > 0){
    //At this stage a cmake_ file has been produced by the clang -M option with
    //the format:
    //
    //target: dependencies
    //
    //The content must be tranformed to:
    //
    //(set WRAPIT_DEPENDS dependencies)
    //
    //and
    //
    //(set WRAPIT_PRODUCTS ...)
    //
    //must be added.
    std::ifstream ifs(cmake_);
    std::string content( (std::istreambuf_iterator<char>(ifs) ),
                         (std::istreambuf_iterator<char>()    ) );
    ifs.close();

    //remove target part from "target: dependencies..."
    static std::regex re1("^[^:]*:[[:space:]]*(\\\\\n[[:space:]]*)?");
    content = std::regex_replace(content, re1, "");

    //drop dependency which is the generated header_file_path_ file:
    auto pos = content.find_first_of(" \t\r\n");
    content.erase(0, pos);

    //trim ending space:
    pos = content.find_last_not_of(" \t\r\n");
    if(pos < content.size()){
      content.erase(pos + 1);
    }

    //replace escaped new line, not supported by cmake, by plain
    //new line and an identation.
    static std::regex re2("\\" "\\" "\n");
    content = std::regex_replace(content, re2, "\n  ");

    content.insert(0, "set(WRAPIT_DEPENDS ");
    content.append(")\n");

    o2 = std::ofstream(cmake_);

    o2 << "# File generated by wrapit version " << version << "\n";
    auto t = time(0);
    o2 << "# Generation time: " << ctime(&t)
      << "\n# List of files produced by wrapit:\n"
       << "set(WRAPIT_PRODUCTS"
       << "\n  " << out_cxx_dir_ << "/jl" << module_name_ << ".cxx";

    for(const auto& fname: towrap_type_filenames_set_){
      o2 << "\n  " << join_paths(out_cxx_dir_, fname);
    }
    o2 << ")\n";

    o2 << "\n# List of files the produced file contents depend on:\n"
       << content;
    o2.close();
  }

  show_stats(std::cout);

  if(type_dependencies_.isCyclic()){
    std::cerr << "Warning: cyclic class dependency found. "
      "This can lead to a \"No factory for type X\" error when loading "
      "the wrapper Julia module.\n";
  }
}

std::ostream& CodeTree::generate_type_wrapper_header(std::ostream& o) const{
  o << "// this file was auto-generated by wrapit " << version << "\n"
    "#include \"Wrapper.h\"\n\n"
    "#include \"jl" << module_name_ << ".h\"\n"
    "#include \"dbg_msg.h\"\n"
    "#include \"jlcxx/functions.hpp\"\n"
    "#include \"jlcxx/stl.hpp\"\n";

  return o;
}

std::ofstream CodeTree::checked_open(const std::string& path) const{
  std::ofstream o(path.c_str(), out_open_mode_);
  if(o.tellp() > 0){
    std::cerr << "File " << path << " in the way. Remove it or use the --force option.\n";
    exit(1);
  }
  return o;
}


std::ostream& CodeTree::show_stats(std::ostream& o) const{
  o << "\nGenerated wrapper statictics\n"
    << std::setw(20) << std::left << "  enums: "
    << nwraps_.enums << "\n"
    << std::setw(20) << std::left << "  classes/structs: "
    << nwraps_.types << "\n"
    << std::setw(20) << std::left << "    templates: "
    << nwraps_.type_templates << "\n"
    << std::setw(20) << std::left << "    others: "
    << (nwraps_.types-nwraps_.type_templates) << "\n"
    << std::setw(20) << std::left << "  class methods: "
    << nwraps_.methods << "\n"
    << std::setw(20) << std::left << "  field accessors:"
    << nwraps_.field_getters << " getters and "
    << nwraps_.field_setters << " setters\n"
    << std::setw(20) << std::left << "  global variable accessors: "
    << nwraps_.global_var_getters << " getters and "
    << nwraps_.global_var_setters << " setters\n"
    << std::setw(20) << std::left << "  global functions: "
    << nwraps_.global_funcs << "\n"
    << "\n";
  return o;
}

std::ostream&
CodeTree::generate_accessor_cxx(std::ostream& o, const TypeRcd* type_rcd,
                                const CXCursor& cursor, bool getter_only,
                                int nindents){


  FunctionWrapper helper(cxx_to_julia_, MethodRcd(cursor), type_rcd, type_map_,
                         cxxwrap_version_,  "", "", nindents);

  int ngens = 0;
  helper.gen_accessors(o, getter_only, &ngens);
  if((type_rcd && export_mode_ >= export_mode_t::member_functions && type_rcd)
     || export_mode_ >= export_mode_t::all_functions){
    for(const auto& n: helper.generated_jl_functions()) to_export_.insert(n);
  }

  const int ngetters = ngens > 0 ? 1 : 0;
  const int nsetters = ngens > 1 ? 1 : 0;

  if(type_rcd){
    nwraps_.field_getters += ngetters;
    nwraps_.field_setters += nsetters;
  } else {
    nwraps_.global_var_getters += ngetters;
    nwraps_.global_var_setters += nsetters;
  }
  return o;
}


//CXType
//CodeTree::resolve_private_typedef(CXType type) const{
//  const auto& decl = clang_getTypeDeclaration(type);
//  auto access = clang_getCXXAccessSpecifier(decl);
//  //If return type is a non-public typedef, while underlying type
//  //is public, resolve it:
//  if(type.kind == CXType_Typedef &&  (access == CX_CXXProtected || access ==CX_CXXPrivate)){
//    const auto& type_ = clang_getCanonicalType(type);
//    //access = clang_getCXXAccessSpecifier(clang_getTypeDeclaration(type_));
//    if(!(access == CX_CXXProtected || access ==CX_CXXPrivate)){
//      std::cerr << "Replacing " << type << " by " << type_ << "\n";
//      type = type_;
//    }
//  }
//  return type;
//}


std::ostream&
CodeTree::generate_method_cxx(std::ostream& o, const TypeRcd& typeRcd, const MethodRcd& method){
  return method_cxx_decl(o, typeRcd, method, "", "", 2);
}

void CodeTree::set_type_rcd_ctor_info(TypeRcd& rcd){
  if(verbose > 3) std::cerr << __FUNCTION__ << "(" << rcd.cursor << ")\n";

  bool with_ctor = false;
  struct data_t {
    bool explicit_def_ctor;
    bool implicit_def_ctor;
    bool public_dtor;
    int n_def_ctor_decls;
    const CodeTree* tree;
  } data = {false, true, true, 0, this};

  clang_visitChildren(rcd.cursor, [](CXCursor cursor, CXCursor, CXClientData data_){
    auto& data = *static_cast<data_t*>(data_);
    const auto& kind = clang_getCursorKind(cursor);
    const auto& access = clang_getCXXAccessSpecifier(cursor);
    if(kind == CXCursor_Constructor){
      if(0 == data.tree->get_min_required_args(cursor)){//clang_Cursor_getNumArguments(cursor)){ //default ctor or equivalent
        ++data.n_def_ctor_decls;
        if(access != CX_CXXPublic
           || data.tree->is_method_deleted(cursor)){
          data.implicit_def_ctor = false;
        } else {
          data.explicit_def_ctor = true;
        }
      } else{ ////a non-default ctor
        data.implicit_def_ctor = false;
      }
    } else if(kind == CXCursor_Destructor){
      if(access != CX_CXXPublic || data.tree->is_method_deleted(cursor)){
        data.public_dtor = false;
      }
    }
    return CXChildVisit_Continue;
  }, &data);

  bool finalize_vetoed = false;
  auto it = find(finalizers_to_veto_.begin(), finalizers_to_veto_.end(), rcd.type_name);
  if(it != finalizers_to_veto_.end()){
    finalize_vetoed = true;
    vetoed_finalizers_.insert(rcd.type_name);
  }

  rcd.default_ctor = !clang_CXXRecord_isAbstract(rcd.cursor)
    && (data.implicit_def_ctor || data.explicit_def_ctor)
    && data.n_def_ctor_decls < 2; //multiple decls can happen with the ctor using default argument values
  //                               this will lead to an error when calling the ctor ⇒ ignore it.

  rcd.finalize = data.public_dtor && !finalize_vetoed;
}

void
CodeTree::test_build(std::ostream& o){
  ++ibuild_;
  int i = ibuild_ - build_nskips_;
  if(build_nskips_ < 0 || ibuild_ <= build_nskips_ || i % build_every_ != 0){
    std::cerr << ibuild_ << " built skipped.\n";
  } else if (build_nmax_ <= 0 || i <= build_nmax_){
    auto p = o.tellp();
    o << "}\n";
    o.flush();

    std::cerr << "build # " << ibuild_ << "\n";
    int rc = system(build_cmd_.c_str());
    o.seekp(p);

    if(rc!=0){
      std::cerr << "Build failed. Abort the code generation.\n";
      exit(rc);
    }
  }
}

std::ostream&
CodeTree::method_cxx_decl(std::ostream& o, const TypeRcd& typeRcd,
                          const MethodRcd& method,
                          std::string varname, std::string classname,
                          int nindents,
                          bool templated){


  TypeRcd* pTypeRcd = find_class_of_method(method.cursor);

  FunctionWrapper wrapper(cxx_to_julia_, method, &typeRcd, type_map_,
                          cxxwrap_version_, varname,
                          classname, nindents, templated);

  //FIXME: check that code below is needed. Should now be vetoed upstream
  if(std::find(veto_list_.begin(), veto_list_.end(),
               wrapper.signature()) != veto_list_.end()){
    if(verbose > 0){
      std::cerr << "Info: " << "func " << wrapper.signature() << " vetoed\n";
    }
    return o;
  }

  auto cxxsignature = wrapper.signature(true);
  auto exposed_cxxsignature = wrapper.signature(true, true);
  std::string already_existing_signature;
  bool new_decl = add_wrapped_method(cxxsignature,
                                     exposed_cxxsignature,
                                     &already_existing_signature);

  if(!new_decl){
    overlap_skipped_methods_.push_back(std::make_pair(cxxsignature,
                                                      already_existing_signature));
    if(verbose > 0){
      std::cerr << "Warning: "
                << "Wrapping of method " << cxxsignature
                << " skipped because it would overwrite the already wrapped method "
                << already_existing_signature
                << ". Both methods needs to be exposed as "
                << exposed_cxxsignature << "\n";
    }
    return o;
  }

  bool new_override_base = wrapper.override_base();
  if(override_base_ && !new_override_base){
    indent(o << "\n", nindents) << "module_.unset_override_module();\n";
    override_base_ = new_override_base;
  } else if(!override_base_ && new_override_base){
    indent(o, nindents) << "module_.set_override_module(jl_base_module);\n";
    override_base_ = new_override_base;
  }
  o << "\n";

  wrapper.generate(o,  get_index_generated_);

  import_getindex_ |= wrapper.defines_getindex();
  import_setindex_ |= wrapper.defines_setindex();

  if(wrapper.generated_jl_functions().size() > 0){
    if(pTypeRcd){
      ++nwraps_.methods;
    }  else{
      ++nwraps_.global_funcs;
    }
  }

  if(!wrapper.is_ctor()
     && (export_mode_ >= export_mode_t::all_functions
         || (export_mode_ >= export_mode_t::member_functions && !wrapper.is_global()))){
    for(const auto& n: wrapper.generated_jl_functions()){
      if(!new_override_base){
        to_export_.insert(n);
      }
    }
  }

  return o;
}

std::ostream&
CodeTree::generate_methods_of_templated_type_cxx(std::ostream& o,
                                                 const TypeRcd& t){
  //Example
  //
  //Class to wrap:
  //  template<typename T1, typename T2> class A {
  //      T1 get_first() const;
  //      void set_second(const T2&);
  //  };
  //  template class A<P1, P2>;
  //  template class A<P3, P4>;
  //
  //Code to generate:
  //  auto t1_decl_methods = []<typename T1, typename T2>(jlcxx::TypeWrapper<A<T1, T2>> wrapped){
  //        typedef A<T1, T2> T;
  //    wrapped.constructor<>();
  //        wrapped.method("get_first", [](const T& a) -> T1 { return a.get_first(); });
  //        wrapped.method("get_second", [](T& a, const T2& b) { a.set_second(b); });
  //  }
  //  t1.apply<A<P1,P2>, A<P3,P4>>(t1_decl_methods);
  //});


  const auto& typename_jl = jl_type_name(t.type_name);
  std::stringstream buf;
  buf << "t" << t.id << "_decl_methods";
  std::string decl_methods = buf.str();

  auto methods = get_methods_to_wrap(t);

  auto nparams = t.template_parameters.size();
  std::vector<std::string> param_list;
  for(decltype(nparams) i = 0; i < nparams; ++i){
    param_list.emplace_back(t.template_parameter_types[i] + " " + t.template_parameters[i]);
  }
  auto param_list1 = join(param_list, ", ");
  auto param_list2 = join(t.template_parameters, ", ");

  //  auto t1_decl_methods = []<typename T1, typename T2>(jlcxx::TypeWrapper<T1, T2> wrapped){
  indent(o,2) << "auto " << decl_methods << " = [this]<" << param_list1
              << "> (jlcxx::TypeWrapper<" << t.type_name << "<" << param_list2
              << ">> wrapped){\n";
  // auto module_ = this->modules_;
  indent(o, 3) << "auto module_ = this->module_;\n";
  //        typedef A<T1, T2> T;
  if(methods.size() > 0){
    indent(o, 3) << "typedef " <<  t.type_name << "<" << param_list2 << "> WrappedType;\n";
  }

  //    wrapped.constructor<>();
  if(t.default_ctor){
    FunctionWrapper::gen_ctor(o, 3, "wrapped", /*templated=*/true,
                              t.finalize, std::string(), std::string(),
                              cxxwrap_version_);
  }

  //        wrapped.method("get_first", [](const T& a) -> T1 { return a.get_first(); });
  //        wrapped.method("get_second", [](T& a, const T2& b) { a.set_second(b); });
  for(const auto& m: methods){
    method_cxx_decl(o, t, m, "wrapped", "WrappedType", 3, /*templated=*/true);
  }

  if(override_base_){
    indent(o << "\n", 3) << "module_.unset_override_module();\n";
    override_base_ = false;
  }

  //  };
  indent(o,2) << "};\n";


  //t1.apply<
  indent(o, 2) << "t.apply<";
  t.specialization_list(o) << ">(" << decl_methods << ");\n";

  return o;
}

std::ostream& CodeTree::generate_jl(std::ostream& o,
                                    std::ostream& export_o,
                                    const std::string& module_name,
                                    const std::string& shared_lib_basename) const{
  o << "module " << module_name << "\n";

  bool first = true;
  unsigned linewidth = 0;
  unsigned max_linewidth = 120;

  for(const auto& n: to_export_){
    if(export_blacklist_.count(n)){
      if(verbose > 0){
        std::cerr << "Info: identifier '" << n << "' found in the veto list and not exported.\n";
      }
      continue;
    }


    std::string s;
    if(first) s = "\nexport ";
    else s =  ", ";
    first = false;
    s += n;
    linewidth += s.size();
    if(linewidth > max_linewidth){
      linewidth = 7;
      export_o << "\nexport ";
      s = n;
    }
    export_o << s;
  }
  if(!first) o << "\n";

  if(import_setindex_ || import_getindex_) o << "\n";
  if(import_getindex_) o << "import Base.getindex\n";
  if(import_setindex_) o << "import Base.setindex!\n";

  o <<  "\n"
    "using CxxWrap\n"
    "import Libdl\n"
    "@wrapmodule(()->\"" << shared_lib_basename<< ".\" * Libdl.dlext)\n"
    "\n"
    "function __init__()\n"
    "    @initcxx\n"
    "end\n";

  //FIXME add code documentation generation
  //  for(const auto& t: types){
  //    if(t->wrapper()!=Entity::kNoWrapper && t->docstring().size() > 0){
  //      o << "\n@doc \"\"\"" << std::regex_replace(t->docstring(), std::regex("\""), "\\\"")
  //        << "\"\"\" " << t->name_jl();
  //    }
  //  }
  o << "\nend #module\n";

  if(verbose > 2){
    std::cerr << "\nVariables.\n";
    for(const auto&v : vars_){
      std::cerr << fully_qualified_name(v)
        /*<< ": " << fully_qualified_name(clang_getCursorSemanticParent(v))*/
                << ", " << clang_getCursorKind(v)
                << "\n";
    }
  }

  return o;
}

void
CodeTree::visit_class(CXCursor cursor){

  if(verbose > 3) std::cerr << __FUNCTION__ << "("
                            << cursor << ")\n";


  //FIXME: is the first check needed?
  if(str(clang_getCursorSpelling(cursor)).size() == 0 || clang_Cursor_isAnonymous(cursor)){
    if(verbose > 0){
      std::cerr << "Skipping anonymous struct found in "
                << clang_getCursorLocation(cursor)
                << "\n";
      return;
    }
  }

  if(has_cursor(visited_classes_, cursor)) return;

  visited_classes_.push_back(cursor);

  const auto& kind = clang_getCursorKind(cursor);
  const auto& type = clang_getCursorType(cursor);

  //get_template_parameters(cursor);
  const auto& access= clang_getCXXAccessSpecifier(cursor);

  if(verbose > 4) std::cerr << "Add type " << type << "\n";
  int index = add_type(cursor);

  types_[index].template_parameters = get_template_parameters(cursor);

  if(is_to_visit(cursor) && !is_type_vetoed(types_[index].type_name)){
    types_[index].to_wrap = true;
  }

  if(verbose > 3) std::cerr << "Calling clang_visitChildren(" << cursor
                            << ", visitor, &data) from "
                            << __FUNCTION__ << "\n";

  bool save = visiting_a_templated_class_;
  if(kind == CXCursor_ClassTemplate) visiting_a_templated_class_ = true;
  clang_visitChildren(cursor, visit, this);
  visiting_a_templated_class_ = save;
}


bool
CodeTree::in_veto_list(const std::string signature) const{
  bool r = false;
  for(const auto& v: veto_list_){
    if(v.size() == 0) continue;
    if(v[0] == '/'){ //regex
      if(v[v.size()-1] != '/'){
        std::cerr << "ERROR: syntax error in the veto file: missing an ending"
          "slash on a line starting with a slash (/).\n";
        exit(1);
      }
      auto re_ = v.substr(1, v.size() - 2);
      if(verbose > 5) std::cerr << "Debug: Comparing " << signature
                                << " with grep-like regular expression "
                                << re_ << "\n";
      std::regex re(re_, std::regex_constants::basic);
      if(std::regex_match(signature, re)){
        r = true;
        break;
      }
    } else{
      if(v == signature){
        r = true;
        break;
      }
    }
  }

  if(verbose > 1) std::cerr << __FUNCTION__ << "("  << signature << ") -> " << r << "\n";

  return r;

}

bool
CodeTree::is_type_vetoed(const std::string& type_name) const{

  static auto def_vetoes = {
    std::regex("std::(.*::)?vector(<.*>)?"),
    std::regex("(std::allocator<)?std::(.*::)?(basic_)?string(<(char|wchar_t)>)?>?")
  };

  bool r = false;

  for(const auto& re: def_vetoes){
    if(std::regex_match(type_name, re)) r = true;
  }

  if(!r){
    r = in_veto_list(type_name);
  }

  return r;
}


void
CodeTree::visit_global_function(CXCursor cursor){

  if(verbose > 3) std::cerr << __FUNCTION__ << "(" << cursor << ")\n";

  if(is_method_deleted(cursor)){
    if(verbose > 1) std::cerr << "Method " << cursor << " is deleted.\n";
    return;
  }


  FunctionWrapper wrapper(cxx_to_julia_, MethodRcd(cursor), nullptr, type_map_, cxxwrap_version_);
  if(in_veto_list(wrapper.signature())){
    //  if(std::find(veto_list_.begin(), veto_list_.end(), wrapper.signature()) != veto_list_.end()){
    if(verbose > 0){
      std::cerr << "Info: " << "func " << wrapper.signature() << " vetoed\n";
    }
    return ;
  }


  std::vector<CXType> missing_types;
  int min_args;
  bool funcptr_returntype;
  std::tie(missing_types, min_args, funcptr_returntype) = visit_function_arg_and_return_types(cursor);

  auto missing_some_type = inform_missing_types(missing_types, MethodRcd(cursor));

  bool dowrap;
  if(auto_veto_){
    //wrap funcion only if no type is missing
    dowrap = !missing_some_type && !funcptr_returntype;
    if(funcptr_returntype && verbose > 0){
      std::cerr << "Wrapper of function " << wrapper.signature()
                << " skipped because it returns a function pointer.\n";
    }

    if(!dowrap) auto_vetoed_methods_.insert(cursor);
  } else{
    //always wrap the function
    dowrap = true;
  }

  if(dowrap){
    functions_.emplace_back(cursor, min_args);
  }

  for(auto const& x: missing_types) types_missing_def_.insert(fully_qualified_name(base_type(x)));
}


std::string CodeTree::getUnQualifiedTypeSpelling(const CXType& type) const{
  auto s = str(clang_getTypeSpelling(type));


  std::regex re;
  if(type.kind == CXType_FunctionProto){
    re = "[[:space:]]*\\b(const|volatile|restict)[[:space:]]*$";
  } else{
    re = "[[:space:]]*\\b(const|volatile|restict)\\b[[:space:]]*";
  }

  s = std::regex_replace(s, re, "");

  return s;
}

std::tuple<bool, std::vector<CXCursor>>
CodeTree::find_type_definition(const CXType& type) const{
  const auto& type0 = base_type(type);
  bool usable = true;
  std::vector<CXCursor> definitions;
  bool u = true;
  CXCursor d;
  std::tie(u, d) = find_base_type_definition_(type0);
  usable = usable && u;

  if(clang_Cursor_isNull(d)){
    return std::make_tuple(usable, definitions);
  }

  definitions.push_back(d);

  int nparams = clang_Type_getNumTemplateArguments(type);
  //overwrite nparams in case of natively supported type
  if(is_natively_supported(type0, &nparams)){
    if(verbose > 4){
      std::cerr << "Debug: restricted number of template parameters "
                << "to " << nparams << " for natively supported type "
                << type << "\n";
    }
  }

  for(int i = 0; i < nparams; ++i){
    bool u;
    std::vector<CXCursor> vd;
    auto param_type = base_type(clang_Type_getTemplateArgumentAsType(type, i));
    std::tie(u, vd) = find_type_definition(param_type);
    if(!u){
      std::cerr << "No definition found for " << param_type << "\n";
    }
    usable = usable && u;

    for(const auto& d: vd){
      auto access = clang_getCXXAccessSpecifier(d);
      if(access == CX_CXXPrivate || access == CX_CXXProtected){
        usable = false;
      }

      auto it = std::find_if(definitions.begin(), definitions.end(),[&d](const auto& c){
        return clang_equalCursors(c, d);
      });
      if(it == definitions.end()){
        definitions.push_back(d);
      }
    }
  }
  return std::make_tuple(usable, definitions);
}

std::tuple<bool, CXCursor>
CodeTree::find_base_type_definition_(const CXType& type0) const{
  bool usable = true;

  if(type0.kind == CXType_Invalid || type0.kind == CXType_Unexposed){
    if(verbose > 4) std::cerr << __FUNCTION__ << "(" << type0
                              << "): type0.kind = "
                              << type0.kind << " => failed\n";
    return std::make_tuple(false,
                           clang_getNullCursor());
  }

  //FIXME: do we need to check if the wrapper of the type
  //is requested, that is definition header in the list and class not vetoed,
  //and set usable to false if it is not?
  if(/*type0.kind != CXType_Record
       && type0.kind != CXType_Enum
       &&*/ type0.kind <= CXType_LastBuiltin
       || type0.kind == CXType_FunctionProto){
    if(verbose > 4) std::cerr << __FUNCTION__ << "(" << type0
                              << "): type0.kind = "
                              << type0.kind << " built-in or function prototype"
                      " => succeedded\n";
    return std::make_tuple(true,
                           clang_getNullCursor());
  }

  auto decl = clang_getTypeDeclaration(type0);

  if(clang_isInvalid(clang_getCursorKind(decl))){
    if(verbose>1){
      std::cerr << "Warning: failed to find declaration of "
                << "type " << type0
                << " of kind "<< type0.kind << "\n";
    }
  }

  CXCursor def = clang_getCursorDefinition(decl);

  if(clang_isInvalid(clang_getCursorKind(def))){
    if(verbose > 1){
      std::cerr << "Warning: failed to find definition of "
                << fully_qualified_name(type0)
                << "\n";
    }
    if(!is_natively_supported(fully_qualified_name(decl))) usable = false;
    def = decl;
  }

  auto access = clang_getCXXAccessSpecifier(def);

  if(access == CX_CXXPrivate || access == CX_CXXProtected){
    usable = false;
  }

  return std::make_tuple(usable, def);
}

void CodeTree::check_for_stl(const CXType& type_){
  auto type = clang_getCanonicalType(type_);
  auto btype = base_type_(type);
  auto btype_name = fully_qualified_name(btype);
  static std::regex re("^std::(vector|valarray)[[:space:]]*<");

  //if not a vector or valrray, our work stops here.
  if(!std::regex_search(btype_name, re)) return;

  int nparams = clang_Type_getNumTemplateArguments(btype);
  if(nparams < 1){
    if(verbose>1){
      std::cerr << "WARNING: registering container " << type
                << " without element type specification."
                << " Julia mapping could be missing.\n";
    }
    return;
  }
  auto eltype = clang_Type_getTemplateArgumentAsType(btype, 0);
  auto beltype = base_type_(clang_Type_getTemplateArgumentAsType(btype, 0));
  auto beltype_name = fully_qualified_name(beltype);

  int itype = -1;
  auto rc = register_type(eltype, &itype);
  ////FIXME: we should register the type as in might be just foward declared.
  //auto beltype_rcd = std::find_if(types_.begin(), types_.end(),
  //                                [&](auto a){ return beltype_name == a.type_name; });

  //  if(beltype_rcd == types_.end()){
  if(!rc){
    if(verbose > 0){
      //      std::cerr << "WARNING: type " << beltype_name << " was not found in "
      //" the type registry. The type " << type << " might miss a julia mapping.\n";
      std::cerr << "WARNING: definition of type " << beltype_name
                << " was not found. Julia mapping of  type " << type
                << " might be missing.\n";
    }
    return;
  }

  if(itype>=0){

    auto& beltype_rcd = types_.at(itype);
    beltype_rcd.to_wrap = true;

    if(verbose > 0){
      auto [_, def ] = find_base_type_definition_(btype);
      std::cerr << "INFO: STL activated for type "
                << beltype_rcd.type_name
                << " to support " << fully_qualified_name(type_)
                << ". ";
      if(!clang_Cursor_isNull(visited_cursor_)){
        std::cerr << "Hint: the latest is likely needed at "
                  << clang_getCursorLocation(visited_cursor_)
                  << ". ";
      }
      std::cerr << "\n";
    }

    bool isconst = clang_isConstQualifiedType(eltype);
    bool isptr   = (eltype.kind == CXType_Pointer);
    if(isptr){
      if(isconst) beltype_rcd.stl_const_ptr = true;
      else        beltype_rcd.stl_ptr = true;
    } else{//!isptr
      if(isconst) beltype_rcd.stl_const = true;
      else        beltype_rcd.stl = true;
    }
  }
}

bool
CodeTree::register_type(const CXType& type, int* pItype, int* pIenum){

  if(pItype) *pItype = -1;
  if(pIenum) *pIenum = -1;

  if(verbose > 3) std::cerr << __FUNCTION__ << "(" << type << "), type of kind "
                            << type.kind
                            << ".\n";


  check_for_stl(type);

  std::string type_name = fully_qualified_name(type);

  bool usable;
  std::vector<CXCursor> defs;
  std::tie(usable, defs) = find_type_definition(type);

  if(!usable){
    return false;
  }


  if(defs.size() > 0 && clang_Cursor_isAnonymous(defs[0])){
    if(verbose > 2){
      std::cerr << "Type " << type << " and its possible instance not mapped because the type is anonymous.\n";
    };
    return false;
  }


  bool maintype = true;
  for(const auto& c: defs){
    if(clang_Cursor_isNull(c)){
      //a POD type, no definition to include.
      continue;
    }

    const auto& type0 = clang_getCursorType(c);


    if(maintype && is_natively_supported(type0)) return true;


    //by retrieving the type from the definition cursor, we discard
    //the qualifiers.
    std::string type0_name = fully_qualified_name(type0);

    if(type0_name.size() == 0){
      std::cerr << "Empty spelling for cursor of kind "
                << clang_getCursorKind(c)
                << " found in "
                << clang_getCursorLocation(c)
                << ". type: " << type0
                << " of kind " << type0.kind
                << "\n";
    }

    if(in_veto_list(type0_name)){
      if(verbose > 2) std::cerr << "Type " << type0_name << " is vetoed. ("
                                << __FUNCTION__ << "() "
                                << __FILE__ << ":" << __LINE__ << ")\n";
      return false;
    }

    std::string type0_name_base = type0_name;
    auto cc = clang_getSpecializedCursorTemplate(c);

    bool is_template = !clang_Cursor_isNull(cc) && !clang_equalCursors(cc, c);
    if(is_template){//a template
      type0_name_base = fully_qualified_name(cc);
      if(is_natively_supported(type0_name_base)){
        continue;
      }
      if(in_veto_list(type0_name_base)){
        if(verbose > 2) std::cerr << "Type " << type0_name_base << " is vetoed. ("
                                  << __FUNCTION__ << "() "
                                  << __FILE__ << ":" << __LINE__ << ")\n";
        return false;
      }
    }

    if(type0.kind == CXType_Record){

      bool found = false;

      int i;

      if(!clang_Cursor_isNull(cc) && !clang_equalCursors(cc, c)){//a template
        i = add_type(cc);
        if(i == types_.size() - 1){//new type
          types_[i].template_parameters = get_template_parameters(cc);
        }
      } else{
        i = add_type(c);
      }

      if(maintype && pItype) *pItype = i;

      types_[i].to_wrap = true;

      if(verbose > 3) std::cerr << "Adding " << c
                                << ", type " << clang_getCursorType(c)
                                << " to the type list ("
                                << __FUNCTION__ << ")"
                                << ". Number of template parameters: "
                                << clang_Type_getNumTemplateArguments(clang_getCursorType(c))
                                << ".\n";

      add_type_specialization(&types_[i], clang_getCursorType(c));
      incomplete_types_.push_back(types_.size() - 1);
    } else if(type0.kind == CXType_Enum){
      auto it = std::find_if(enums_.begin(), enums_.end(),
                             [c](const TypeRcd& t){
                               return clang_equalCursors(t.cursor, c);
                             });

      if(it == enums_.end()){
        enums_.emplace_back(c);
        it = enums_.end() - 1;
      }
      if(maintype && pIenum) *pIenum = (int)(it - enums_.begin());
      it->to_wrap = true;
    } else if(type0.kind == CXType_Typedef){
      auto underlying_base_type = base_type(clang_getTypedefDeclUnderlyingType(c));
      return register_type(underlying_base_type);
    } else if(type0.kind == CXType_Elaborated){
      auto elab_type =  static_cast<const clang::ElaboratedType*>(type0.data[0]);
      std::cerr << "\t named type: " << elab_type->getNamedType().getAsString() << "\n";
    } else{
      std::cerr << "Warning: type '" << type0 << "' is of the unsupported kind '"
                << clang_getTypeKindSpelling(type0.kind)
                << "'.\n";
    }
    maintype = false;
  } //next c

  return true;
}



std::tuple<std::vector<CXType>, int, bool>
CodeTree::visit_function_arg_and_return_types(CXCursor cursor){

  const auto& method_type = clang_getCursorType(cursor);
  const auto return_type = clang_getResultType(method_type);

  //TODO: find a more robust way than comparing identifier  names.
  auto is_class_param = [cursor, this](CXType type){
    TypeRcd* pTypeRcd = find_class_of_method(cursor);
    if(pTypeRcd==nullptr) return false;
    auto type_name = remove_cv(str(clang_getTypeSpelling(base_type_(type))));
    auto params = pTypeRcd->template_parameters;
    return std::find(params.begin(), params.end(), type_name) != params.end();
  };

  std::vector<CXType> missing_types;

  //FIXME: if the registration of the function failed, then we can end
  //up with types which were registered for that function but are finally
  //not needed. We should avoid the registrations that add unecessary
  //wraps.

  if(return_type.kind != CXType_Void){
    if(verbose > 3) std::cerr << cursor << ", return type: " << return_type << "\n";
    if(is_class_param(return_type)){
      if(verbose > 3) std::cerr << return_type
                                << " identified as a parameter of the holding class\n";
    } else if(type_map_.is_mapped(return_type, /*as_return=*/ true)){
      if(verbose > 3) std::cerr << return_type << " is mapped to an alternative type.\n";
    } else{
      //we call in_veto_list instead of is_type_vetoed to not exclude std::vector
      bool rc = !in_veto_list(fully_qualified_name(return_type)) && register_type(return_type);
      if(!rc) missing_types.push_back(base_type(return_type));
    }
  }

  for(int i = 0; i < clang_getNumArgTypes(method_type); ++i){
    auto argtype = clang_getArgType(method_type, i);
    if(verbose > 3) std::cerr << cursor << ", arg " << (i+1) << " type: " << argtype << "\n";
    if(argtype.kind == CXType_IncompleteArray){
      auto eltype = clang_getElementType(argtype);
      bool rc = register_type(eltype);
      if(!rc) missing_types.push_back(argtype);
    } else if(argtype.kind != CXType_Void){
      if(is_class_param(argtype)){
        if(verbose > 3) std::cerr << cursor << " identified as a parameter of the holding class\n";
      } else if(type_map_.is_mapped(argtype)){
        if(verbose > 3) std::cerr << argtype << " is mapped to an alternative type.\n";
      } else{
        //we call in_veto_list instead of is_type_vetoed to not exclude std::vector
        bool rc = !in_veto_list(fully_qualified_name(base_type(argtype))) && register_type(argtype);
        if(!rc) missing_types.push_back(argtype);
      }
    }
  }

  for(auto const& x: missing_types) types_missing_def_.insert(fully_qualified_name(base_type(x)));

  int min_args = get_min_required_args(cursor);

  if(verbose > 3) std::cerr << __FUNCTION__ << "(" << cursor << ") -> min_args = "
                            << min_args << "\n";

  bool return_a_func_ptr = false;
  if((return_type.kind == CXType_Pointer
      && clang_getPointeeType(return_type).kind == CXType_FunctionProto)
     || (clang_getPointeeType(clang_getTypedefDeclUnderlyingType(clang_getTypeDeclaration(return_type))).kind
         == CXType_FunctionProto)){
    return_a_func_ptr = true;
  }

  return std::make_tuple(missing_types, min_args, return_a_func_ptr);
}

int CodeTree::get_min_required_args(CXCursor cursor) const{
  struct data_t{
    const CodeTree* tree;
    unsigned min_args;
    unsigned max_args;
    bool default_values;
  } data = { this, 0, 0, false};

  if(verbose > 1){
    std::cerr << "Calling " << __FUNCTION__  << "(" << cursor
              << ") from " << __FUNCTION__
              << ", cusor location: " << clang_getCursorLocation(cursor) << "\n";
  }
  clang_visitChildren(cursor, [](CXCursor cursor, CXCursor, CXClientData data){
    //skip the return type of non-void function:
    if(clang_getCursorKind(cursor) != CXCursor_ParmDecl) return CXChildVisit_Continue;

    auto& rcd = *static_cast<data_t*>(data);
    rcd.default_values = rcd.default_values || rcd.tree->arg_with_default_value(cursor);

    //count  min_args until we find a default value:
    if(!rcd.default_values) rcd.min_args += 1;

    rcd.max_args += 1;
    return CXChildVisit_Continue;
  }, &data);

  auto method_type = clang_getCursorType(cursor);
  auto max_args = clang_getNumArgTypes(method_type);
  if(max_args != data.max_args){
    std::cerr << "Warning: failed to determine if some argument of function "
              << cursor
              << " has a default value. This can happen if the function is "
      "defined  with the help of a macro. Support of default argument value "
      "disabled for this function.\n";
    data.min_args = data.max_args = max_args;
  }

  return data.min_args;
}

std::string CodeTree::fundamental_type(std::string type){
  static std::regex re("[[:space:]]*(const|volatile|\\*|&)[[:space:]]*");
  return std::regex_replace(type, re, "");
}

void
CodeTree::visit_member_function(CXCursor cursor){

  if(verbose > 3) std::cerr << __FUNCTION__ << "(" << cursor << ")\n";

  TypeRcd* pTypeRcd = find_class_of_method(cursor);

  FunctionWrapper wrapper(cxx_to_julia_, MethodRcd(cursor), pTypeRcd, type_map_, cxxwrap_version_);

  //FIXME: handle the case where the method is defined in the parent class
  // => would need to define a Julia method that will throw an exception
  //This is not handled either when the access is restricted.
  if(is_method_deleted(cursor)){
    if(verbose > 1) std::cerr << wrapper.signature() << " is deleted.\n"; //"Method " << cursor << " is deleted.\n";
    std::stringstream buf;
    buf << pTypeRcd->type_name << " & " << pTypeRcd->type_name << "::operator=(const " << pTypeRcd->type_name << " &)";
    if(verbose>1) std::cerr << "\tcompared with " << buf.str() << " to check if copy operator must be marked as deleted.\n";
    if(pTypeRcd && wrapper.signature() == buf.str()/*str(clang_getCursorSpelling(cursor)) == "operator="*/){
      if(verbose > 3){
        std::cerr << "Mark copy contructor of class " << pTypeRcd->type_name << " as deleted.\n";
      }
      pTypeRcd->copy_op_deleted = true;
    }

    return;
  }

  if(in_veto_list(wrapper.signature())){
    //  if(std::find(veto_list_.begin(), veto_list_.end(), wrapper.signature()) != veto_list_.end()){
    if(verbose > 0){
      std::cerr << "Info: " << "func " << wrapper.signature() << " vetoed\n";
    }
    return ;
  }

  std::vector<CXType> missing_types;
  int min_args;
  bool funcptr_returntype;
  std::tie(missing_types, min_args, funcptr_returntype) = visit_function_arg_and_return_types(cursor);

  auto p = find_class_of_method(cursor);

  if(!p){
    std::cerr << "Warning: method " << cursor << " found at "
              << clang_getCursorLocation(cursor)
              << " skipped because the definition of its class "
      "was not found.\n";
  } else{
    bool missing_some_type = inform_missing_types(missing_types, MethodRcd(cursor), p);
    bool dowrap;
    if(auto_veto_){
      //method wrapped only if all argument and return types can be wrapped
      dowrap = !missing_some_type && !funcptr_returntype;
      if(funcptr_returntype && verbose > 0){
        std::cerr << "Wrapper of function " << wrapper.signature()
                  << " skipped because it returns a function pointer.\n";
      }

      if(!dowrap) auto_vetoed_methods_.insert(cursor);
    } else{
      //always wrap the function
      dowrap = true;
    }

    if(dowrap){
      auto it = std::find_if(p->methods.begin(), p->methods.end(),
                             [cursor](const MethodRcd& m){
                             return clang_equalCursors(clang_getCanonicalCursor(m.cursor),
                                                       clang_getCanonicalCursor(cursor));
                           });
      if(it == p->methods.end()){
        p->methods.emplace_back(cursor, min_args);
      } else if(it->min_args > min_args){
        it->min_args = min_args;
        it->cursor = cursor;
      }
    }
  }
}

void
CodeTree::disable_owner_mirror(CXCursor cursor){
  CXCursor prev = cursor;
  CXCursor base = clang_getCursorSemanticParent(cursor);
  while(!clang_Cursor_isNull(base) && clang_getCursorKind(base) != CXCursor_TranslationUnit
        && !clang_equalCursors(prev, base)){
    if(clang_getCursorKind(base) ==  CXCursor_ClassDecl
       || clang_getCursorKind(base) ==  CXCursor_StructDecl){
      no_mirror_types_.insert(type_name(base));
    }
    prev = base;
    base = clang_getCursorSemanticParent(cursor);
  }

}

TypeRcd*
CodeTree::find_class_of_method(const CXCursor& method){
  const auto& myClass = clang_getCursorSemanticParent(method);

  for(auto& t: types_){
    if(clang_equalCursors(t.cursor, myClass)){
      return &t;
    }
  }
  return nullptr;
}

bool CodeTree::has_type(CXCursor cursor) const{
  for(const auto& x: types_){
    return clang_equalCursors(cursor, x.cursor);
  }
  return false;
}

bool CodeTree::has_type(const std::string& t) const{
  for(const auto& x: types_){
    //    const auto& t2 = clang_getCursorType(x.cursor);
    //return str(clang_getTypeSpelling(t2)) == t;
    if(x.type_name == t) return true;
  }
  return false;
}

bool
CodeTree::inform_missing_types(std::vector<CXType> missing_types,
                               const MethodRcd& methodRcd,
                               const TypeRcd* classRcd) const{

  if(missing_types.size() == 0) return false;

  auto join = [this](const std::vector<CXType> v){
    std::stringstream buf;
    std::string sep;
    for(const auto& e: v){
      buf << sep << e;
      sep = ", ";
    }
    return buf.str();
  };

  if(verbose > 0){
    std::string funcname =  FunctionWrapper(cxx_to_julia_, methodRcd, classRcd,
                                            type_map_, cxxwrap_version_).signature();
    std::cerr << "Warning: missing definition of type "
              << (missing_types.size() > 1 ? "s" : "")
              << " "
              << join(missing_types)
              << " to define wrapper for "
              << funcname
              << "\n";
  }

  return true;
}

CXToken* CodeTree::next_token(CXSourceLocation loc) const{
  CXToken* tok = nullptr;
  CXFile file;
  unsigned offset;
  clang_getSpellingLocation(loc, &file, nullptr, nullptr, nullptr);
  while(!tok){
    clang_getSpellingLocation(loc, nullptr, nullptr, nullptr, &offset);
    if(offset == 0) break; //loc of file reached => roll over
    loc = clang_getLocationForOffset(unit_, file, offset + 1);
    tok = clang_getToken(unit_, loc);
  }
  return tok;
}

CXSourceRange CodeTree::function_decl_range(const CXCursor& cursor) const{
  auto range = clang_getCursorExtent(cursor);

  CXSourceLocation start = clang_getRangeStart (range);
  CXSourceLocation end = clang_getRangeEnd (range);
  CXFile file;
  clang_getSpellingLocation (end, &file, nullptr, nullptr, nullptr);
  std::string s;
  while(s != ";" && s != "{"){
    CXToken* tok = next_token(end);
    if(tok==nullptr) break;

    s = str(clang_getTokenSpelling (unit_, *tok));
    range = clang_getTokenExtent (unit_, *tok);
    end = clang_getRangeEnd(range);
    clang_disposeTokens(unit_, tok, 1);
  }

  return clang_getRange(start, end);
}


bool CodeTree::is_method_deleted(CXCursor cursor) const{
  return ::is_method_deleted(unit_, cursor);
}

void
CodeTree::visit_class_constructor(CXCursor cursor){

  if(verbose > 3) std::cerr << __FUNCTION__ << "(" << cursor << ")\n";

  TypeRcd* pTypeRcd = find_class_of_method(cursor);

  FunctionWrapper wrapper(cxx_to_julia_, MethodRcd(cursor), pTypeRcd, type_map_,
                          cxxwrap_version_);

  if(in_veto_list(wrapper.signature())){
    //if(std::find(veto_list_.begin(), veto_list_.end(), wrapper.signature()) != veto_list_.end()){
    if(verbose > 0){
      std::cerr << "Info: " << "func " << wrapper.signature() << " vetoed\n";
    }
    return ;
  }

  if(is_method_deleted(cursor)){
    if(verbose > 1) std::cerr << "Constructor " << wrapper.signature() << " is deleted.\n";
    return;
  }

  std::vector<CXType> missing_types;
  int min_args;
  bool funcptr_returntype;
  std::tie(missing_types, min_args, funcptr_returntype) = visit_function_arg_and_return_types(cursor);

  auto* p = find_class_of_method(cursor);

  auto access = clang_getCXXAccessSpecifier(cursor);

  bool is_def_ctor = (0 == clang_Cursor_getNumArguments(cursor));

  if(p && !is_def_ctor){
    if(access == CX_CXXPublic){
      auto missing_some_type = inform_missing_types(missing_types, MethodRcd(cursor), p);
      bool dowrap;

      if(auto_veto_){
        dowrap = !missing_some_type && !funcptr_returntype;
        if(funcptr_returntype && verbose > 0){
          std::cerr << "Wrapper of function " << wrapper.signature()
                    << " skipped because it returns a function pointer.\n";
        }

        if(!dowrap) auto_vetoed_methods_.insert(cursor);
      } else{
        dowrap = true;
      }

      if(dowrap){
        auto it = std::find_if(p->methods.begin(), p->methods.end(),
                               [cursor](const MethodRcd& m){
                                 return clang_equalCursors(clang_getCanonicalCursor(m.cursor),
                                                           clang_getCanonicalCursor(cursor));
                               });
        if(it == p->methods.end()){
          p->methods.emplace_back(cursor, min_args);
        } else if(it->min_args > min_args){
          it->min_args = min_args;
          it->cursor = cursor;
        }
      }
    }
  }
}

void
CodeTree::visit_enum(CXCursor cursor){
  if(has_cursor(enums_, cursor)) return;
  if(verbose > 2) std::cerr << __FUNCTION__ << "(" << cursor
                            << " defined in "
                            << clang_getCursorLocation(cursor)
                            << ")\n";
  disable_owner_mirror(cursor);
  const auto& type = clang_getCursorType(cursor);
  enums_.emplace_back(cursor, str(clang_getTypeSpelling(type)));
}


void
CodeTree::visit_typedef(CXCursor cursor){
  if(verbose > 3) std::cerr << __FUNCTION__ << "(" << cursor << ")\n";

  if(has_cursor(typedefs_, cursor)) return;

  TypeRcd* pTypeRcd = find_class_of_method(cursor);
  if(pTypeRcd){
    pTypeRcd->typedefs.push_back(cursor);
  } else{
    disable_owner_mirror(cursor);
    typedefs_.push_back(cursor);
  }
}

void
CodeTree::visit_field_or_global_variable(CXCursor cursor){
  if(!accessor_generation_enabled()) return;

  if(verbose > 3) std::cerr << __FUNCTION__ << "(" << cursor << ")\n";

  disable_owner_mirror(cursor);

  auto kind = clang_getCursorKind(cursor);
  if(kind == CXCursor_FieldDecl){
    auto clazz = clang_getCursorSemanticParent(cursor);
    if(clang_Cursor_isNull(clazz)){
      if(verbose > 0){
        std::cerr << "Warning: idendifier " << cursor << " found at "
                  << clang_getCursorLocation(cursor)
                  << " ignored. It looks likes a class field, but we failed "
                  << "to determine to owner class.";
      }
      return;
    }
    auto rcd = std::find_if(types_.begin(), types_.end(),
                            [&](auto a){ return clang_equalCursors(a.cursor, clazz); });
    if(rcd == types_.end()){
      std::cerr << "Warning: field " << cursor << " of class "
                << clazz << " found at "
                << clang_getCursorLocation(cursor)
                << " ignored. The class record was not found."
                << "This is not expected please report the problem to the developers.\n";
      return;
    }
    auto type = clang_getCursorType(cursor);
    bool rc  = register_type(type);
    if(!rc) types_missing_def_.insert(fully_qualified_name(base_type(type)));
    //call to register_type can modify types_ and invalidate rcd,
    rcd = std::find_if(types_.begin(), types_.end(),
                       [&](auto a){ return clang_equalCursors(a.cursor, clazz); });
    rcd->fields.push_back(cursor);
  } else if(kind == CXCursor_VarDecl){
    auto type = clang_getCursorType(cursor);
    bool rc  = register_type(type);
    if(!rc) types_missing_def_.insert(fully_qualified_name(base_type(type)));
    //to prevent duplicates in case of static fields, which can
    //appear both within the class block and outside, the
    //following check is performed:
    if(clang_equalCursors(clang_getCursorLexicalParent(cursor),
                          clang_getCursorSemanticParent(cursor))){
      vars_.push_back(cursor);
    }
  }
}

void
CodeTree::visit_class_template_specialization(CXCursor cursor){
  if(verbose > 3) std::cerr << __FUNCTION__ << "(" << cursor << ")\n";

  const auto& specialized_cursor = clang_getSpecializedCursorTemplate(cursor);

  const auto& type = clang_getCursorType(cursor);

  TypeRcd* pTypeRcd = nullptr;
  for(auto& r: types_){
    if(clang_equalCursors(r.cursor, specialized_cursor)){
      pTypeRcd =  &r;
      break;
    }
  }

  if(!pTypeRcd){
    std::cerr << "Warning: specialization found at " << clang_getCursorLocation(cursor)
              << " for class/struct " << cursor << " before its definition.\n";
    add_type(specialized_cursor, /*check = */false);
    pTypeRcd = & types_.back();
  }
  add_type_specialization(pTypeRcd, type);
  pTypeRcd->to_wrap = true;
  pTypeRcd->template_parameters = get_template_parameters(specialized_cursor);
}

bool CodeTree::add_type_specialization(TypeRcd* pTypeRcd, const CXType& type){

  auto cursor = clang_getTypeDeclaration(type);
  auto nparams = clang_Type_getNumTemplateArguments(type);
  if(nparams <= 0) return false;

  std::vector<std::string> combi;
  std::vector<std::string> param_types;
  for(decltype(nparams) i = 0; i < nparams; ++i){
    const auto& param = clang_Type_getTemplateArgumentAsType(type, i);
    if (param.kind != CXType_Invalid) {
      combi.push_back(str(clang_getTypeSpelling(param)));
      param_types.push_back("typename");
    } else {
      auto TA = get_IntegralTemplateArgument(cursor, i);
      using clang::TemplateArgument;
      if (TA.getKind() == TemplateArgument::ArgKind::Integral){
        combi.push_back(std::to_string(TA.getAsIntegral().getSExtValue()));
        param_types.push_back(TA.getIntegralType().getAsString());
      } else {
        // The "FIXME" is a message for the user; highlighting that the template argument
        // is neither a type or an integral parameter
        combi.push_back("/* FIXME */");
      }
    }
  }

  auto& combi_list = pTypeRcd->template_parameter_combinations;
  if(combi_list.end() == std::find(combi_list.begin(), combi_list. end(),
                                   combi)){
    combi_list.push_back(combi);
  }
  pTypeRcd->template_parameter_types = param_types;

  return true;
}

bool CodeTree::isAForwardDeclaration(CXCursor cursor) const{
  const auto& kind = clang_getCursorKind(cursor);

  auto rc =  (kind == CXCursor_ClassDecl || kind == CXCursor_StructDecl || kind == CXCursor_EnumDecl)
    && !clang_equalCursors(clang_getCursorDefinition(cursor), cursor);

  const auto& special = clang_getSpecializedCursorTemplate(cursor);
  if(!clang_Cursor_isNull(special)) rc = false;
  return rc;
}

bool CodeTree::is_to_visit(CXCursor cursor) const{
  const auto& type = clang_getCursorType(cursor);
  const auto& access = clang_getCXXAccessSpecifier(cursor);
  const auto& kind = clang_getCursorKind(cursor);
  if( kind != CXCursor_Constructor //ctors are visited independtly of their access
      //                             because we need to identify when the default
      //                             ctor access was resticted
      && (access == CX_CXXPrivate || access == CX_CXXProtected)){
    if(verbose > 1){
      std::cerr << "Skipping cursor " << cursor << " with access "
                <<  access
                <<  ", " << type
                << ", " << clang_getCursorLocation(cursor)
                << ".\n";
      std::cerr << "\n";
    }
    return false;
  }

  if(isAForwardDeclaration(cursor)){
    if(verbose > 3){
      std::cerr << "Skipping forward declaration of class/struct " << cursor
                << ".\n";
      return false;
    }
  }

  if(mainFileOnly_ && !fromMainFiles(cursor)){
    if(verbose > 3){
      std::cerr << "Skipping cursor " << cursor << " defined at " << clang_getCursorLocation(cursor)
                << " in an included file, " << cursor << ".\n";
    }
    return false;
  }

  return true;
}

bool CodeTree::fromMainFiles(const CXCursor& cursor) const{
  const auto& loc = clang_getCursorLocation(cursor);
  CXFile file;
  clang_getFileLocation(loc, &file, nullptr, nullptr, nullptr);
  //FIXME: stores files as CXFile to prevent conversions and allocations
  auto fname = fs::canonical(fs::path(str(clang_getFileName(file)))).string();
  bool result = std::find(files_to_wrap_fullpaths_.begin(), files_to_wrap_fullpaths_.end(), fname)
    != files_to_wrap_fullpaths_.end();


  if(verbose > 3) std::cerr << __FUNCTION__ << "(" << cursor << ") -> "
                            << result
                            << " (file defined in " << fname << ")"
                            << "\n";

  return result;
}

CXChildVisitResult CodeTree::visit(CXCursor cursor, CXCursor parent,
                                   CXClientData client_data){
  CodeTree& tree = *static_cast<CodeTree*>(client_data);

  if(!pp) pp = clang_getCursorPrintingPolicy(cursor);

  tree.visited_cursor_ = cursor;

  if(verbose > 5) std::cerr << "visiting " << clang_getCursorLocation(cursor)
                            << "\n";

  const auto& kind = clang_getCursorKind(cursor);
  const auto& access= clang_getCXXAccessSpecifier(cursor);

  const auto& type_access = clang_getCXXAccessSpecifier(clang_getTypeDeclaration(clang_getCursorType(cursor)));

  //if(kind != CXCursor_Constructor && !tree.is_to_visit(cursor)) return CXChildVisit_Continue;
  if(!tree.is_to_visit(cursor)) return CXChildVisit_Continue;

  if(verbose > 1) std::cerr << "visiting " << clang_getCursorLocation(cursor)
                            << "\t cursor " << cursor
                            << " of kind " << kind
                            << ", type " << clang_getCursorType(cursor)
                            << ", and access " << access
                            << ", and type access " << type_access
                            << "\n";

  if(tree.visiting_a_templated_class_
     && kind!= CXCursor_CXXMethod
     && kind!= CXCursor_Constructor){
    return CXChildVisit_Continue;
  }

  if(kind == CXCursor_Namespace){
    return CXChildVisit_Recurse;
  } else if((kind == CXCursor_ClassDecl || kind == CXCursor_StructDecl)
            && clang_getCursorType(cursor).kind != CXType_Invalid
            && !tree.isAForwardDeclaration(cursor)
            ){
    const auto& special = clang_getSpecializedCursorTemplate(cursor);
    if(clang_Cursor_isNull(special)){
      tree.visit_class(cursor);
    } else{
      //static std::regex re("<[^>]*>");
      //auto type_name = std::regex_replace(str(clang_getTypeSpelling(clang_getCursorType(cursor))), re, "");
      tree.visit_class_template_specialization(cursor);
    }
  } else if(kind == CXCursor_ClassTemplate){
    tree.visit_class(cursor);
  } else if(kind == CXCursor_FunctionDecl){
    tree.visit_global_function(cursor);
  } else if(kind == CXCursor_EnumDecl && !tree.isAForwardDeclaration(cursor)){
    tree.visit_enum(cursor);
  } else if(kind == CXCursor_TypedefDecl){
    //  tree.visit_typedef(cursor);
  } else if((kind == CXCursor_VarDecl || kind == CXCursor_FieldDecl)){
    tree.visit_field_or_global_variable(cursor);
  } else if(kind == CXCursor_CXXMethod){
    tree.visit_member_function(cursor);
  } else if(kind == CXCursor_Constructor){
    //note: we need to visit constructors independenlty of their accessibilities.
    tree.visit_class_constructor(cursor);
  } else if(verbose > 1){
    std::cerr << "Cursor " <<  clang_getCursorSpelling(cursor)
              << " at " << clang_getCursorLocation(cursor)
              << " of kind " << clang_getCursorKindSpelling(kind) << " skipped.\n";
  }
  return CXChildVisit_Continue;
}


std::string CodeTree::resolve_include_path(const std::string& fname){
  fs::path p(fname);
  fs::path pp;
  bool found = false;
  for(const auto& d: include_dirs_){
    try{
      pp =  fs::canonical(d / p);
      found = true;
      break;
    } catch(fs::filesystem_error){
      //path does not exist
      /* NO-OP */
    }
  }
  return found ? pp.string() : fname;
}

bool CodeTree::isASTvalid(CXTranslationUnit translationUnit){
    int nbDiag = clang_getNumDiagnostics(translationUnit);
    bool foundError = false;
    for (unsigned int currentDiag = 0; currentDiag < nbDiag; ++currentDiag) {
        CXDiagnostic diagnotic = clang_getDiagnostic(translationUnit, currentDiag);
        CXString errorString = clang_formatDiagnostic(diagnotic,clang_defaultDiagnosticDisplayOptions()); //include filename and lineo
        std::string tmp{clang_getCString(errorString)};
        clang_disposeString(errorString);
        if (tmp.find("error:") != std::string::npos) {
            foundError = true;
        }
        std::cerr << tmp << std::endl;
    }
    return !foundError;
}

bool
CodeTree::parse(){

  if(!fs::is_directory(out_cxx_dir_) && !fs::create_directories(out_cxx_dir_)){
    std::cerr << "Failed to create directory " << out_cxx_dir_ << ".\n";
    return false;
  }

  header_file_path_ = join_paths(out_cxx_dir_,  std::string("jl") + module_name_ + ".h");
  FileTimeRestorer timerestore(header_file_path_);

  std::ofstream header_file(header_file_path_, out_open_mode_);
  if(header_file.tellp() != 0){
    std::cerr << "File " << header_file_path_
              << " is in the way. Remove it or use the --force actions "
      "to disable the check.\n";
    return false;
  }

  for(const auto& fname: extra_headerss_){
    header_file << "#include \"" << fname << "\"\n";
  }

  files_to_wrap_fullpaths_.clear();
  for(const auto& fname: files_to_wrap_){
    files_to_wrap_fullpaths_.push_back(resolve_include_path(fname));
    //DEBUG>>
    std::cerr << "File to wrap: " << files_to_wrap_fullpaths_.back() << "\n";
//<<DEBUG
    header_file << "#include \"" << fname << "\"\n";
  }

  header_file.close();
  timerestore.settimestamp();

  index_ = clang_createIndex(0, 0);

  std::vector<const char*> opts(opts_.size() + 2);

  if(!check_resource_dir(true)){
    exit(1);
  }

  opts[0] = "-resource-dir";
  opts[1] = clang_resource_dir_.c_str();

  for(unsigned i = 0; i < opts_.size(); ++i){
    if(opts_[i] == "-resource-dir"){
      std::cerr << "\n----------------------------------------------------------------------\n"
        "Warning: DEPRECATED -resource-dir option found in the "
        "clang_opt parameter of the configuration file. This will be ignore or "
        "results in a fatal error in future releases. Please use the default "
        "resource directory path or set an alternative one using the command "
        "line option. See wrapit --help.\n"
        "----------------------------------------------------------------------\n";
    }
    opts[2+i] = opts_[i].c_str();
  }

  std::string mfopt;
  if(cmake_.size() > 0){
    //exit program if file is in the way and force mode is disabled
    checked_open(cmake_).close();
    mfopt = std::string("-MF").append(cmake_);

    opts.push_back("-MMD");
    opts.push_back(mfopt.c_str());
  }

  if(verbose > 1){
    //Enable clang verbose option
    opts.push_back("-v");
  }

  if(verbose > 0){
    std::cerr << "Clang options:\n";
    for(const auto& x: opts) std::cerr << "\t" << x << "\n";
    std::cerr << "\n";
  }


  CXTranslationUnit unit = clang_parseTranslationUnit(index_, header_file_path_.c_str(),
                                                      opts.data(), opts.size(),
                                                      nullptr, 0,
                                                      CXTranslationUnit_SkipFunctionBodies);
  unit_ = unit;

  diagnose(header_file_path_.c_str(), unit);

  if (unit == nullptr) {
    std::cerr << "Unable to parse " << header_file_path_ << ". Quitting.\n";
    exit(-1);
  }

  if(!isASTvalid(unit)){
    if(ignore_parsing_errors_){
      std::cerr << "Warning: code generation forced by the --ignore-parsing-errors "
        "option. The generated code will likely be invalid. Common issue is "
        "some original data types changed into int.\n";
    } else{
      //note: error messages already displayed by isASTvalid().
      return false;
    }
  }

  const auto&  cursor = clang_getTranslationUnitCursor(unit);

  if(verbose > 1) std::cerr << "Calling clang_visitChildren(" << cursor
                            << ", CodeTree::visit, &data) from "
                            << __FUNCTION__ << "\n";

  clang_visitChildren(cursor, CodeTree::visit, this);
  return true;
}


CodeTree::~CodeTree(){
  if(unit_) clang_disposeTranslationUnit(unit_);
  if(index_) clang_disposeIndex(index_);

}

std::ostream& CodeTree::report(std::ostream& o){
  o << "Dependency cycles:  ";
  if(type_dependencies_.isCyclic()){
    o << "bad. Type dependencies have at least one cycle."
      " THIS CAN LEAD TO \"No factory\" ERROR WHEN LOADING THE WRAPPER MODULE.\n";
  } else{
    o << "ok. No dependency cycle.\n";
  }

  std::string s = "Undefined types";
  o << s << "\n";
  for(unsigned i = 0; i < s.size(); ++i) o << "-";

  o << "\n\n";
  o << "The definition of the following types where not found. Use the "
    "extra_headers configuration parameters to specify the header files that defines.\n\n";
  if(auto_veto_){
    o << "The auto_veto parameter is set to true, generation of the wrappers that "
      "would have needed theses types were skipped.\n";
  } else{
    o << "The auto_veto parameter is set to false. The generated julia module "
      "is likely to fail to be imported because of the missing wrappers for these types.\n";
  }

  o << "\n";

  for(const auto& t: types_missing_def_){
    o << t << "\n";
  }

  if(types_missing_def_.size() == 0){
    o << "No missing definitions\n";
  }


  s = "Auto-vetoed functions";
  o << "\n" << s << "\n";
  for(unsigned i = 0; i < s.size(); ++i) o << "-";
  o << "\n";

  for(const auto& cursor: auto_vetoed_methods_){
    auto sig = FunctionWrapper(cxx_to_julia_, MethodRcd(cursor),
                               find_class_of_method(cursor),
                               type_map_,
                               cxxwrap_version_).signature();
    o << sig << "\n";
  }

  //
  //   std::set<int> used_headers;
  //   for(const auto& t: types_missing_def_){
  //     Type* tt = find_type(t);
  //     if(tt && tt->ifilename() >= 0){
  //       used_headers.insert(tt->ifilename());
  //     }
  //   }
  //
  //   s = "\n\nUnused auxiliary file.";
  //   o << s << "\n";
  //   for(unsigned i = 0; i < s.size(); ++i) o << "-";
  //   o << "\n\n";
  //   o << "File from the auxiliary_file or inputs configurable list which were not used.\n\n";
  //
  // //  for(unsigned i = 0; i < filenames_.size(); ++i){
  // //    if(used_headers.find(i) == used_headers.end()) o << filenames_[i] << "\n";
  // //  }
  //
  //   s = "\n\nUsed file.";
  //   o << s << "\n";
  //   for(unsigned i = 0; i < s.size(); ++i) o << "-";
  //   o << "\n\n";
  //   o << "File from the auxiliary_files or inputs configurable list which were used.\n\n";
  //
  // //  for(unsigned i = 0; i < filenames_.size(); ++i){
  // //    if(used_headers.find(i) != used_headers.end()) o << filenames_[i] << "\n";
  // //  }

  s = "\nClass(es) from vetoed_finalizer_classes configuration parameter that were not found:";
  o << s << "\n";
  for(unsigned i = 0; i < s.size(); ++i) o << "-";
  o << "\n\n";

  std::vector<std::string> missed_vetoes;
  for(const auto& v: finalizers_to_veto_){
    if(vetoed_finalizers_.find(v) == vetoed_finalizers_.end()){
      missed_vetoes.push_back(v);
    }
  }

  if(missed_vetoes.size() > 0){
    const char* sep ="";
    for(const auto& e: missed_vetoes){
      o << sep << e;
      sep = ", ";
    }
    o << "\n";
  } else{
    o << "None";
  }
  o << "\n";

  o << "\nList of wrapped classes:\n\n";
  for(const auto& t: types_){
    if(t.to_wrap){
      o << t.type_name << "\n";
    }
  }

  o << "\n\nList of wrapped methods:\n\n:";
  for(const auto& m: wrapped_methods_){
    o << m << "\n";
  }

  o << "\n\nList of methods not wrapped to prevent overwriting. "
    "Method that would have been overwritten in parenthesis.\n";
  for(const auto& p: overlap_skipped_methods_){
    o << p.first << "\n\t" << p.second << "\n";
  }

  return o;
}


void CodeTree::reset_wrapped_methods(){
  wrapped_methods_.clear();
  wrapped_methods_after_map_.clear();
}

bool CodeTree::add_wrapped_method(const std::string signature_before_type_map,
                                  const std::string signature_after_type_map,
                                  std::string* found_signature){
  auto it = std::find(wrapped_methods_after_map_.begin(),
                       wrapped_methods_after_map_.end(),
                      signature_after_type_map);
  if(wrapped_methods_after_map_.end() == it){
    wrapped_methods_.push_back(signature_before_type_map);
    wrapped_methods_after_map_.push_back(signature_after_type_map);
     if(found_signature) *found_signature = "";
     return true;
  } else{
    if(found_signature) *found_signature = *it;
    return false;
  }
}


void CodeTree::preprocess(){
  if(propagation_mode_ == propagation_mode_t::methods){
    //Add methods of all used classes
    //Disable limitation by file depth
    bool savedMainFileOnly = mainFileOnly_;
    mainFileOnly_ = false;
    auto incomplete_types = incomplete_types_;
    for(const auto& i: incomplete_types){
      if(verbose > 1 && !clang_Cursor_isNull(types_[i].cursor)){
        std::cerr << "Calling clang_visitChildren("
                  << types_[i].cursor << ", CodeTree::visit, &data) from "
                  << __FUNCTION__ << " for types_[" << i << "]\n";
      }

      auto t = clang_getCursorType(types_[i].cursor);
      auto c_decl = clang_getTypeDeclaration(t);
      auto c_def = clang_getCursorDefinition(c_decl);
      if(!clang_equalCursors(c_decl, types_[i].cursor)){
        std::cerr << types_[i].cursor << " is not a definition!\n";
        abort();
      }

      clang_visitChildren(types_[i].cursor, visit, this);
    }
    mainFileOnly_ = savedMainFileOnly;
  }

  //Add child -> parent class dependencies
  for(unsigned iChild = 0; iChild < types_.size(); ++iChild){
    auto [ parent, extra_parents ] = getParentClassesForWrapper(types_[iChild].cursor);
    //dependency is relevant for the main parent only, ignore the extra parents.
    if(!clang_Cursor_isNull(parent)){
      for(unsigned iParent = 0;  iParent < types_.size(); ++iParent){
        if(clang_equalCursors(parent, types_[iParent].cursor)){
          //Parent must be declared before its child:
          if(verbose > 4){
            std::cerr << "Debug: "
                    << types_[iParent].type_name
                      << " must be declared before its child "
                      << types_[iChild].type_name << "\n";
          }
          type_dependencies_.preceeds(iParent, iChild);
        }
      }
    }

    //Add dependencies of classes on their template parameteres
    auto nParams = types_[iChild].template_parameter_types.size();
    for(unsigned iCombi = 0;
        iCombi < types_[iChild].template_parameter_combinations.size();
        ++iCombi){
      for(unsigned iParam = 0; iParam < nParams; ++iParam){
        if(types_[iChild].template_parameter_types[iParam] != "typename") continue;

        const auto& paramType = types_[iChild].template_parameter_combinations[iCombi].at(iParam);

        for(unsigned jType = 0; jType < types_.size(); ++jType){
          if(types_[jType].type_name == paramType){
            if(verbose > 4){
              std::cerr << "Debug: "
                        << paramType
                        << " must be declared before the class "
                        << types_[iChild].type_name
                        << " that uses it as a template parameter.\n";
            }
            type_dependencies_.preceeds(jType, iChild);
            break;
          }
        }//next iParamType
      }//next paramType
    }//next iCombi


    //Add user defined dependencies
    for(const auto& dep: class_order_constraints_){
      auto it1 = std::find_if(types_.begin(), types_.end(),
                              [dep](const auto& x){ return x.type_name == dep.first;});
      auto it2 = std::find_if(types_.begin(), types_.end(),
                              [dep](const auto& x){ return x.type_name == dep.second;});

      if(it1 != types_.end() && it2 != types_.end()){
        type_dependencies_.preceeds(it1 - types_.begin(), it2 - types_.begin());
        if(verbose > 1) std::cerr << "Dependency \"" << dep.second << " requires " << dep.first
                                << "\" added.\n";
      } else{
        if(verbose>0){
          std::cerr << "Warning: class dependency "
                    << dep.first << " < " << dep.second
                    << " not used.";
          if(it1 == types_.end()) std::cerr << " No " << dep.first << " class.";
          if(it2 == types_.end()) std::cerr << " No " << dep.second << " class.";
          std::cerr << "\n";
        }
      }
    }

#ifdef DEFINE_TEMPLATE_METHODS_IN_CTOR
    //Add dependencies of templated classes to the type of their methods
    //argument and return value.
    if(types_[iChild].template_parameter_combinations.size() > 0){
      for(const auto& m: types_[iChild].methods){
        auto method_type = clang_getCursorType(m.cursor);
        for(int iarg = 0; iarg < clang_getNumArgTypes(method_type); ++iarg){
          auto argtype = clang_getArgType(method_type, iarg);
          argtype = base_type_(argtype);
          if(argtype.kind == CXType_IncompleteArray){
            argtype = clang_getElementType(argtype);
          }
          argtype = base_type_(argtype);
          auto argtypename = fully_qualified_name(argtype);
          if(argtype.kind == CXType_Record){
            for(unsigned iArgType = 0; iArgType < types_.size(); ++iArgType){
              if(types_[iArgType].type_name == argtypename){
                if(verbose > 4){
                  std::cerr << "Debug: "
                            << argtypename
                            << " must be declared before "
                            << types_[iChild].type_name
                            << " because it is used in a method of this "
                            << " templated class."
                            << "\n";
                }
                type_dependencies_.preceeds(iArgType, iChild);
              }//types_[iArgType].type_name == argtypename
            }//next iArgType
          }//(argtype.kind == CXType_Record
        }//next iarg
      }//next m
    } //types_[iChild].template_parameter_combinations.size() > 0
  } //next iChild
#endif

  if(verbose > 2){
    std::cerr << "\nType list at end of preprocess:\n";
    for(const auto& t: types_){
      std::cerr << t.type_name << "\n";
    }
    std::cerr << "\n";
  }

  type_dependencies_.extend(types_.size());

  types_sorted_indices_ = type_dependencies_.sortedIndices();

  if(verbose > 2){
    std::cerr << "\nType list at end of preprocess after reordering:\n";
    for(const auto i: types_sorted_indices_){
      std::cerr << types_[i].type_name << "\n";
    }
    std::cerr << "\n";
  }

  //add a fake_type to hold global functions and variables
  if(functions_.size() > 0 || vars_.size() > 0){
    types_.emplace_back();
    types_.back().methods = deduplicate_methods(functions_);
    types_.back().fields = vars_;
    types_.back().to_wrap = true;
    types_sorted_indices_.push_back(types_.size() - 1);
  }

  update_wrapper_filenames();
  if(out_open_mode_ & std::ios_base::app){
    //not overwriting mode (--force option disabled)
    exit_if_wrapper_files_in_the_way();
  }
}

void CodeTree::exit_if_wrapper_files_in_the_way(){
  std::stringstream buf;
  std::string sep;
  int ifile = 0;
  for(const auto& fname: towrap_type_filenames_){
    std::string fpath = join_paths(out_cxx_dir_, fname);
    if(fs::exists(fpath)){
      buf << sep << fpath;
      sep = ", ";
      ++ifile;
    }
  }
  if(ifile > 0){
    if(ifile == 1){
      std::cerr << "File " << buf.str() << " is in the way. Remove it or use the --force option.\n";
    } else{
      std::cerr << "Files " << buf.str() << " are in the way. Remove them or use the --force option.\n";
    }
    exit(1);
  }
}


//extract prefix from a string formatted as 'prefix::type_name':
std::string
CodeTree::get_prefix(const std::string& type_name) const{
  return std::regex_replace(type_name, std::regex("(.*::).*$"), "$1", std::regex_constants::format_no_copy);
}


std::ostream&
CodeTree::generate_enum_cxx(std::ostream& o, CXCursor cursor){

  const bool anonymous_enum = clang_Cursor_isAnonymous(cursor);
  const bool scoped_enum = clang_EnumDecl_isScoped(cursor);

  std::string type_name;
  const auto& type = clang_getCursorType(cursor);

  type_name = str(clang_getTypeSpelling(type));


  if(std::find(veto_list_.begin(), veto_list_.end(), type_name)!= veto_list_.end()){
    std::cerr << "Info: enum " << type_name << " vetoed\n";
    return o;
  }

  ++nwraps_.enums;

  //extract prefix from a string formatted as 'prefix::type_name':
  auto prefix_cxx = get_prefix(type_name);
  auto prefix_jl = scoped_enum ? jl_type_name(type_name) + "!" : jl_type_name(prefix_cxx);

  if(prefix_cxx.size() > 2){
    const auto& clazz = prefix_cxx.substr(0, prefix_cxx.size() - 2);
    if(std::find(veto_list_.begin(), veto_list_.end(), clazz) != veto_list_.end()){
      if(verbose > 0){
        std::cerr << "Info: enum " << type_name << " vetoed\n";
      }
      return o;
    }
  }

  if(!anonymous_enum){
    auto typename_jl = jl_type_name(type);
    indent(o << "\n",1) << "DEBUG_MSG(\"Adding wrapper for enum " << type
             << " (\" __HERE__ \")\");\n";
    indent(o, 1) << "// defined in "   << clang_getCursorLocation(cursor) << "\n";

    indent(o,1) << "jlModule.add_bits<" << type_name << ">(\""
                << typename_jl << "\", jlcxx::julia_type(\"CppEnum\"));\n";

    if(export_mode_ == export_mode_t::all) to_export_.insert(typename_jl);
  } else{
    indent(o << "\n", 1) << "DEBUG_MSG(\"Adding anonymous enum defined in "
             << clang_getCursorLocation(cursor)
             << " (\" __HERE__ \")\");\n";
    indent(o, 1) << "// defined in "   << clang_getCursorLocation(cursor) << "\n";
  }

  auto values = get_enum_constants(cursor);

  for(const auto& value: values){
    std::string value_cpp;
    if(scoped_enum){
      value_cpp = type_name + "::" + value;
    } else{
      value_cpp = prefix_cxx + value;
    }
    std::string value_jl = prefix_jl + value;
    if(anonymous_enum){
      //anonymous enum values must be cast to int
      value_cpp = std::string("static_cast<int>(" + value_cpp + ")");
    }

    if(export_mode_ == export_mode_t::all) to_export_.insert(value_jl);

    indent(o,1) << "jlModule.set_const(\"" << value_jl << "\", "
                << value_cpp << ");\n";
  }
  return o;
}

std::vector<std::string> CodeTree::get_enum_constants(CXCursor cursor) const{
  std::vector<std::string> v;

  auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data){
    auto& v = *static_cast<std::vector<std::string>*>(data);
    const auto& kind = clang_getCursorKind(cursor);

    if(kind == CXCursor_EnumConstantDecl){
      auto c = clang_getCursorSpelling(cursor);
      v.emplace_back(clang_getCString(c));
      clang_disposeString(c);
    }
    return CXChildVisit_Continue;
  };

  if(verbose > 1) std::cerr << "Calling clang_visitChildren(" << cursor
                            << ", visitor, &data) from "
                            << __FUNCTION__ << "\n";
  clang_visitChildren(cursor, visitor, &v);

  return v;
}

void
CodeTree::parse_vetoes(const fs::path& fname){
  std::ifstream f(fname.c_str());
  if(f.fail()){
    std::cerr << "Warning: veto file " << fname << " was not found.\n";
  }

  while(!f.fail()){
    std::string line;
    std::getline(f, line);
    //trim spaces
    std::string whitespaces (" \t\f\v\n\r");
    line.erase(line.find_last_not_of(whitespaces + ";") + 1);
    line.erase(0, line.find_first_not_of(whitespaces));
    if(line.size() > 0
       && line[0]!='#'
       && !(line[0]=='/' && line[1] =='/')){
      veto_list_.push_back(line);
    }
  }
}


bool
CodeTree::arg_with_default_value(const CXCursor& c) const{
  CXToken* toks = nullptr;
  unsigned nToks = 0;

  const auto& range = clang_getCursorExtent(c);
  if(clang_Range_isNull(range)) return false;

  clang_tokenize(unit_, range, &toks, &nToks);

  bool res = false;
  for(unsigned i =0; i < nToks; ++i){
    const auto& s = str(clang_getTokenSpelling(unit_, toks[i]));
    if(s == "=") res = true;
  }
  clang_disposeTokens(unit_, toks, nToks);


  return res;
}

void CodeTree::inheritances(const std::vector<std::string>& val){
  inheritance_.clear();
  std::regex re("([^:]*):(.*)");
  for(const auto&v :val){
    std::cmatch cm;
    if(std::regex_match(v.c_str(), cm, re)){
      inheritance_[std::string(cm[1])] = std::string(cm[2]);
    } else{
      std::cerr << "Invalid syntax for inheritance configuration list element: "
                << v << ". The syntax is Child_class:Parent_class\n";
    }
  }
}


CodeTree::accessor_mode_t
CodeTree::check_veto_list_for_var_or_field(const CXCursor& cursor, bool global_var) const{
  std::string entity = global_var ? "global variable" : "field";

  auto field_name  = fully_qualified_name(cursor);
  //  if(std::find(veto_list_.begin(), veto_list_.end(), field_name) != veto_list_.end()){
  if(in_veto_list(field_name)){
    if(verbose > 0){
      std::cerr << "Info: " << entity << " " << field_name << " accessors vetoed\n";
    }
    return accessor_mode_t::none;
    //  } else if(std::find(veto_list_.begin(), veto_list_.end(), field_name + "!") != veto_list_.end()){
  } else if(in_veto_list(field_name + "!")){
    if(verbose > 0){
      std::cerr << "Info: " << entity << " " << field_name << " getter vetoed\n";
    }
    return accessor_mode_t::getter;
  } else{
    return accessor_mode_t::both;
  }
}

int
CodeTree::add_type(const CXCursor& cursor, bool check){
  const int not_found =  -1;
  int index = not_found;
  if(check){
    int i = 0;
    for(const auto& t: types_){
      if(clang_equalCursors(t.cursor, cursor)){
        index = i;
        break;
      }
      ++i;
    }
  }
  if(index == not_found){
    index = types_.size();
    types_.emplace_back(cursor);
    set_type_rcd_ctor_info(types_.back());
  }

  //  auto name = str(clang_getTypeSpelling(clang_getCursorType(cursor)));
  //  auto it = finalizers_to_veto_.find(name);
  //  if(it != finalizers_to_veto_.end()){
  //    if(verbose>1){
  //      std::cerr << "Info: Disable contructor call from Julia for the class " << name
  //                << " listed in the vetoed_finalize_class configuration parameter\n";
  //    }
  //    types_.back().finalize = false;
  //    finalizers_to_veto_.erase(it);
  //  }

  return index;
}


void CodeTree::update_wrapper_filenames(){
  int ifile = 0;
  int igroupedtype = 0;
  towrap_type_filenames_.clear();
  towrap_type_filenames_set_.clear();

  for(auto i: types_sorted_indices_){
    auto& c = types_[i];
    if(!c.to_wrap) continue;
    if(n_classes_per_file_ > 1
       && (igroupedtype % n_classes_per_file_) == 0){
      ++ifile;
    }

    if(c.type_name.size() == 0 && n_classes_per_file_ != 0){
      //holder of global functions and variables
      towrap_type_filenames_set_.insert("JlGlobals.cxx");
      towrap_type_filenames_.emplace_back("JlGlobals.cxx");
      continue;
    }

    ++igroupedtype;
    std::stringstream buf;
    if(n_classes_per_file_ < 0){
      //FIXME: handle possible name clashes
      buf << "Jl" << std::regex_replace(c.type_name, std::regex("::"), "_") << ".cxx";
    } else if(n_classes_per_file_ == 0){
      buf << "jl" << module_name_ << ".cxx";
    } else if (n_classes_per_file_ == 1){
      buf << "JlClasses.cxx";
    } else {
      buf << "JlClasses_" << std::setfill('0') << std::setw(3) << ifile << ".cxx";
    }
    towrap_type_filenames_set_.insert(buf.str());
    towrap_type_filenames_.emplace_back(buf.str());
  }
}

bool CodeTree::check_resource_dir(bool verbose) const{
  bool rc = std::filesystem::is_directory(std::filesystem::path(clang_resource_dir_));
  if(!rc && verbose){
    std::cerr << "ERROR: '" << clang_resource_dir_
              << "' is not a directory. Use -resource-dir wrapit command line "
      "option to specify an alternative path for the clang resouce directory: "
      "see 'clang --help' and 'clang -print-resource-dir'.\n";
  }

  if(rc){
    auto stddef_path = join_paths(join_paths(clang_resource_dir_, "include"),
                                  "stddef.h");
    rc &=  std::filesystem::exists(std::filesystem::path(stddef_path));
    if(verbose && !rc){
      std::cerr << "ERROR: file 'include/stddef.h' not found in the resource "
        "directory '" << clang_resource_dir_ << "'.  Use --resource-dir "
        "wrapit command line option to specify an alternative path. "
        "See 'clang --help' and 'clang -print-resource-dir'.\n";
    }
  }
  return rc;
}

std::ostream&
CodeTree::gen_apply_stl(std::ostream& o, int indent_depth,
                        const TypeRcd& type_rcd,
                        const std::string& add_type_param) const{

  if(cxxwrap_version_ >= cxxwrap_v0_17){
    //apply_stl not relevant for CxxWrap 0.17+
    return o;
  }

  if(type_rcd.stl){
    indent(o, indent_depth) << "jlcxx::stl::apply_stl<" << add_type_param
                            << ">(jlModule);\n";
  }
  if(type_rcd.stl_const){
    indent(o, indent_depth) << "jlcxx::stl::apply_stl<const " << add_type_param
                            << ">(jlModule);\n";
  }
  if(type_rcd.stl_ptr){
    indent(o, indent_depth) << "jlcxx::stl::apply_stl<" << add_type_param << "*"
                            << ">(jlModule);\n";
  }
  if(type_rcd.stl_const_ptr){
    indent(o, indent_depth) << "jlcxx::stl::apply_stl<const " << add_type_param
                            << "*>(jlModule);\n";
  }

  return o;
}

bool CodeTree::is_natively_supported(const CXType& type, int* nparams) const{
  return is_natively_supported(fully_qualified_name(type), nparams);
}

bool CodeTree::is_natively_supported(const std::string& type_fqn,
                                     int* nparams) const{
  struct rcd {
    std::string name;
    int ntplparams;
    rcd(const char* name, int ntplparams): name(name), ntplparams(ntplparams){}
  };

  static bool firstcall = true;
  static std::vector<rcd> natively_supported = {
    {"_jl_value_t", 0},
    {"jl_value_t", 0},
    {"std::string", 0},
    {"std::wstring", 0},
    {"std::vector", 1},
    {"std::deque", 1},
    {"std::valarray", 1},
    {"jlcxx::SafeCFunction", 0},
    {"jlcxx::Array", 1},
    {"jlcxx::ArrayRef", 2},
    {"std::shared_ptr", 1},
    {"std::unique_ptr", 1},
    {"std::weak_ptr", 1}
  };

  if(firstcall){
    firstcall = false;
    if(cxxwrap_version_ >= cxxwrap_v0_15){
      natively_supported.emplace_back("std::queue", 1);
    }
    if(cxxwrap_version_ >= cxxwrap_v0_16){
      natively_supported.emplace_back("std::set", 1);
      natively_supported.emplace_back("std::multiset", 1);
    }
  }


  if(verbose > 4) std::cerr << __FUNCTION__ << "(" << type_fqn << ")\n";

  auto it = std::find_if(natively_supported.begin(), natively_supported.end(),
                         [type_fqn](const auto& x){ return x.name == type_fqn;});

  if(nparams){
    if(it!= natively_supported.end()) *nparams = it->ntplparams;
    else *nparams = 0;
  }

  return it != natively_supported.end();
}

std::string CodeTree::libclangdir(){
  std::string r;
  Dl_info info;
  if (dladdr(reinterpret_cast<void*>(clang_getCursorType), &info)){
    r = fs::path(info.dli_fname).remove_filename().string();
  }
  return r;
}

std::string CodeTree::resolve_clang_resource_dir_path(std::string path){
  auto p = fs::path(path);
  if(p.is_relative()){
    p = fs::path(libclangdir()) / p;
    path = p.string();
  }
  return path;
}

void CodeTree::generate_project_file(std::ostream& o,
                                     const std::string& uuid,
                                     const std::string& version){
  o << "name = \"" << module_name_ << "\"\n"
    "uuid = \"" << uuid << "\"\n";

  if(version.size() > 0){
    o << "version = \"" << version << "\"\n";
  }
  o << "\n[deps]\n"
    "CxxWrap = \"1f15a43c-97ca-5a2a-ae31-89f07a497df4\"\n"
    "Libdl = \"8f399da3-3557-5675-b5ff-fb832c97cbdb\"\n\n";


  int version_depth = version_major(cxxwrap_version_) == 0 ? 2 : 1;
  o << "\n[compat]\n"
    "CxxWrap = \"" << version_int_to_string(cxxwrap_version_, version_depth)
    << "\"\n";
}

//FIXME: factorize codes of set_julia_names and set_mapped_types

void CodeTree::set_julia_names(const std::vector<std::string>& name_map){
  cxx_to_julia_.clear();
  std::regex re("\\s*->\\s*");
  int i = 0;
  for(const std::string& m: name_map){
    ++i;
    std::sregex_token_iterator it{m.begin(), m.end(), re, -1};
    std::vector<std::string> tokens{it, {}};
    if(tokens.size() > 1){
      cxx_to_julia_[tokens[0]] = tokens[1];
    } else{
      std::cerr << "ERROR. Syntax error for the " << nth(i) << " element "
                << m
                << " of julia_names parameter.\n";
    }
  }
}

void CodeTree::set_class_order_constraints(const std::vector<std::string>& class_order_constraints){
  class_order_constraints_.clear();
  std::regex re("\\s*<\\s*");
  int i = 0;
  for(const std::string& m: class_order_constraints){
    ++i;
    std::sregex_token_iterator it{m.begin(), m.end(), re, -1};
    std::vector<std::string> tokens{it, {}};
    if(tokens.size() > 1){
      class_order_constraints_.push_back(std::make_pair(tokens[0], tokens[1]));
    } else{
      std::cerr << "ERROR. Syntax error for the " << nth(i) << " element "
                << m
                << " of class_dependencies configuration parameter.\n";
    }
  }
}

void CodeTree::set_mapped_types(const std::vector<std::string>& name_map){
  type_straight_mapping_.clear();
  std::regex re("\\s*->\\s*");
  int i = 0;
  for(const std::string& m: name_map){
    ++i;
    std::sregex_token_iterator it{m.begin(), m.end(), re, -1};
    std::vector<std::string> tokens{it, {}};
    if(tokens.size() == 2){
      type_straight_mapping_[tokens[0]] = tokens[1];
    } else{
      std::cerr << "ERROR. Syntax error for the " << nth(i) << " element "
                << m
                << " of mapped_types configuration parameter.\n";
    }
  }
}

void CodeTree::set_cxx2cxx_typemap(const std::vector<std::string>& name_map){
  reset_type_map();

  std::regex re1("\\s*->\\s*");

  //to extract a, b from "(a, b)" or a from "a"
  std::regex re2("\\s*(\\(\\s*([^,]*)\\s*,\\s*([^,]*)\\s*\\)|[^,]*)\\s*");

  int i = 0;
  for(const std::string& m: name_map){
    ++i;
    std::sregex_token_iterator it{m.begin(), m.end(), re1, -1};
    std::vector<std::string> tokens{it, {}};
    bool failed = false;
    if(tokens.size() == 2){
      auto & from = tokens[0];
      auto & to = tokens[1];
      //decode to which can be (argtype, returntype) or universaltype
      std::smatch m2;
      if(std::regex_match(to, m2, re2)){
        std::string argtype, returntype;
        if(m2[2].str().size() > 0){ //(argtype, returntype)
          argtype = m2[2].str();
          returntype = m2[3].str();
        } else{
          argtype = returntype = m2[1].str();
        }
        if(verbose > 2){
          std::cerr << "Configured to change from type " << from
                    << " to " << argtype << " and " << returntype
                    << " in function prototypes declared to CxxWrap, "
                    << "respectively for argument and return types.\n";
        }
        type_map_.add(from, argtype, returntype);
      } else{
        failed = true;
      }
    } else{
      failed = true;
    }
    if(failed){
      std::cerr << "ERROR. Syntax error for the " << nth(i) << " element "
                << m
                << " of mapped_types configuration parameter.\n";
    }
  }
}

std::vector<MethodRcd>
CodeTree::deduplicate_methods(const std::vector<MethodRcd>& methods, bool quiet) const{
  struct rcd{
    bool iscont;
    std::string nomapsig;
    int ircd;
  };
  std::map<std::string, rcd> sigs;
  std::vector<MethodRcd> res;
  for(const auto& m: methods){
    FunctionWrapper wrapper(cxx_to_julia_, m, nullptr, type_map_, cxxwrap_version_);
    std::string pref = clang_CXXMethod_isStatic(m.cursor) ? "static " : "";
    std::string mapped_sig    = pref + wrapper.signature(/*cv=*/false, /*map=*/true);
    std::string notmapped_sig = pref + wrapper.signature(/*cv=*/false, /*map=*/false);
    bool isconst = clang_CXXMethod_isConst(m.cursor);

    auto it = sigs.find(mapped_sig);
    if(it != sigs.end()){ //already in
      std::string hidden_nomapsig = notmapped_sig;
      std::string exposed_nomapsig = it->second.nomapsig;

      //give precedence to the form that the type map not changed by type mapping
      //TODO: choose the option with less differences when for both type mapping
      //       introduces a change (e.g., counting the numbers of changed tokens).
      if(mapped_sig == notmapped_sig
         && it->second.nomapsig != it->first){
        auto ires = it->second.ircd;
        res[ires] = m;
        sigs.erase(it);
        sigs[mapped_sig] = {isconst, notmapped_sig, ires};
        std::swap(hidden_nomapsig, exposed_nomapsig);
      }

      if(hidden_nomapsig != exposed_nomapsig){
        if(!quiet && verbose > 1){
          std::cerr << "Warning: method " << hidden_nomapsig
                    << " was not wrapped because it conflicts with "
                    << exposed_nomapsig << ". Both turn into "
                    << mapped_sig << " after the cxx2cxx type mapping.\n";
        }
      }
    } else{
      sigs[mapped_sig] = {isconst, notmapped_sig, (int)res.size()};
      res.push_back(m);
    }
  }
  return res;
}

std::vector<MethodRcd>
CodeTree::get_methods_to_wrap(const TypeRcd& type_rcd, bool quiet) const{

  //method signature with the child class as prefix
  auto methodsig = [&](const MethodRcd& methodrcd){
    FunctionWrapper wrapper(cxx_to_julia_, methodrcd, &type_rcd, type_map_, cxxwrap_version_);
    std::string pref = clang_CXXMethod_isStatic(methodrcd.cursor) ? "static " : "";
    return pref + wrapper.signature(/*cv=*/false, /*map=*/true);
  };

  std::vector<MethodRcd> towrap;
  std::set<std::string> sigs;
  const std::vector<MethodRcd>& own_methods = deduplicate_methods(type_rcd.methods, quiet);

  for(const auto& m: own_methods){
    towrap.push_back(m);
  }

  if(!multipleInheritance_) return towrap;

  auto [mapped_base, extra_direct_parents ] = getParentClassesForWrapper(type_rcd.cursor);
  auto it_base_rcd = std::find_if(types_.begin(), types_.end(),
                                  [&](auto a){ return clang_equalCursors(a.cursor, mapped_base);});

  std::vector<MethodRcd> base_methods;
  if(it_base_rcd != types_.end() && !clang_Cursor_isNull(it_base_rcd->cursor)){
    base_methods = get_methods_to_wrap(*it_base_rcd, /*quiet=*/true);
  }

  //check if the method x has the same function name of one of the methods
  //listed in v:
  const auto& definedby = [&](const MethodRcd& x, const std::vector<MethodRcd>& v){
    const auto& s1 = str(clang_getCursorSpelling(x.cursor));
    return v.end() != std::find_if(v.begin(), v.end(),
                                   [&](const auto& x2){
                                     return s1 == str(clang_getCursorSpelling(x2.cursor));
                                   });
  };

  //We will walk through the extra-inheritance tree of type_rcd.cursor

  //methods hidden by a definition with same function name in the child:
  std::vector<std::set<std::string>> hidden_method_stack;
  //parents to process organised by generation:
  std::vector<std::vector<CXCursor>> parent_stack;

  //add parents of c to the stacks
  auto add_parents_of = [this](std::vector<std::vector<CXCursor>>& parent_stack,
                               std::vector<std::set<std::string>>& hidden_method_stack,
                               const CXCursor& c,
                               const std::set<std::string>& hidden_methods){
    auto [base, extra_parents] = getParentClassesForWrapper(c);
    parent_stack.push_back(extra_parents);
    hidden_method_stack.push_back(hidden_methods);
  };

  auto ishidden = [](const std::vector<std::set<std::string>>& hidden_method_stack,
                     std::string& funcname){
    for(auto fnames: hidden_method_stack){
      if(fnames.end() != fnames.find(funcname)){
        return true;
      }
    }
    return false;
  };

  auto funcnamesofclass = [](const TypeRcd& typercd){
    std::set<std::string> r;
    for(const auto& m: typercd.methods){
      r.insert(str(clang_getCursorSpelling(m.cursor)));
    }
    return r;
  };
  add_parents_of(parent_stack, hidden_method_stack, type_rcd.cursor, funcnamesofclass(type_rcd));

  //list of candidates for method declarations:
  //use the function name as key.
  //if several children provide methods with the same function name, then these
  //methods should be ignore as it raises an ambiguity in C++, that used solely
  //the function name to resolve the class that provides the methods with the
  //same function name.
  std::map<std::string, std::vector<std::pair<TypeRcd, MethodRcd>>> extra_defs;

  //storage of the names of the functions of the class declared
  //by current cursor within the next for-loop
  std::set<std::string> funcnames;
  for(;;){
    //Draw a cursor from the pile and replace it by its parents:
    while(parent_stack.back().empty()){
      parent_stack.pop_back();
      hidden_method_stack.pop_back();
      if(parent_stack.empty()) break;
    }
    if(parent_stack.empty()) break;

    auto c = parent_stack.back().back();
    parent_stack.back().pop_back();

    auto itTypeRcd = types_.end();

    if(!clang_Cursor_isNull(c)){
      const auto& t = clang_getCursorType(c);
      itTypeRcd = std::find_if(types_.begin(), types_.end(),
                               [&t](const auto& x){
                                 return same_type(t, clang_getCursorType(x.cursor));
                               });
    }

    funcnames.clear();
    //if the class was found, pass through its methods
    if(itTypeRcd != types_.end()){
      for(const auto& m: deduplicate_methods(itTypeRcd->methods, /*quiet=*/true)){
        auto kind = clang_getCursorKind(m.cursor);
        auto is_static = clang_CXXMethod_isStatic(m.cursor);
        if(kind != CXCursor_Constructor
           && kind != CXCursor_Destructor
           && !is_static
           && !clang_CXXConstructor_isCopyConstructor(m.cursor)
           && !clang_CXXConstructor_isConvertingConstructor(m.cursor)
           && !clang_CXXConstructor_isMoveConstructor(m.cursor)
           && !clang_CXXMethod_isCopyAssignmentOperator(m.cursor)
           && !clang_CXXMethod_isDeleted(m.cursor)
           && !clang_CXXMethod_isMoveAssignmentOperator(m.cursor)
           && !clang_CXXMethod_isPureVirtual(m.cursor)
           ){

          auto funcname = str(clang_getCursorSpelling(m.cursor));
          funcnames.insert(funcname);

          if(ishidden(hidden_method_stack, funcname)){
            //skip hidden methods
            //} else if(definedby(m, own_methods)){ //should never happend
            //if(verbose > 2){
            //  std::cerr << "Method " << methodsig(m)
            //            << " takes precedence over the same method of ancestor class "
            //            << c << "\n";
            //}
          } else if(definedby(m, base_methods)){
            //FIXME we miss to exlude functions defined by an ancestor of base.
            //A strategy to do so could be, to include the base class branch in our
            //walk and mark the definition coming from this branch as such
            //(in extra_defs) and exclude the functions in the loop on extra_defs.
            if(verbose > 0){
              std::cerr << "Warning: method " << methodsig(m)
                        << " is inherited in Julia by " << type_rcd.type_name
                        << " from the " << mapped_base
                        << ", while it is not in C++ because of an overriding "
                "ambiguity with the class " << itTypeRcd->type_name << ".\n";
            }
          } else{
            auto fname = str(clang_getCursorSpelling(m.cursor));
            extra_defs[fname].push_back(std::make_pair(*itTypeRcd, m));
          }
        }
      } //next m
    } else if(verbose > 0){
      std::cerr << "No record found for the class " << c
                << ", ancestor of class " << type_rcd.type_name
                << ". The methods will not be inherited.\n";
    }

    add_parents_of(parent_stack, hidden_method_stack, c, funcnames);
  } //for(;;)

  for(const auto& m: extra_defs){
    if(m.second.size() == 1){
      bool vetoed = false;
      //check veto both with the name of this class and of the ancestor
      //that declared the method
      for(const auto& t: {type_rcd, m.second[0].first}){
        FunctionWrapper wrapper(cxx_to_julia_, m.second[0].second,
                                &t, type_map_,
                                cxxwrap_version_);
        vetoed |= (std::find(veto_list_.begin(), veto_list_.end(),
                             wrapper.signature()) != veto_list_.end());
      }
      if(!vetoed) towrap.push_back(m.second[0].second);
    } else{
      if(verbose > 0){
        std::cerr << "Warning: function "
                  <<  m.first
                  << " not wrapped for type " << type_rcd.type_name
                  << ", because of an overriding ambiguity in C++. Function "
          "provided by the ancestor classes";
        for(const auto& cl: m.second) std::cerr << ", " << cl.first.type_name << "\n";
        std::cerr << ".\n";
      }
    }
  }

//   std::cerr << "Method to wrap for " << type_rcd.type_name << ": ";
//   std::string sep;
//   for(const auto& m: towrap){
//     std::cerr << sep << m.cursor;
//     sep = ", ";
//   }
//   std::cerr << "\n";
  return towrap;
}
