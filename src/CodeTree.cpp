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

#include "utils.h"
extern const char* version;

#include "clang/AST/Type.h" //FOR DEBUG
#include "clang/AST/DeclTemplate.h"

#include <dlfcn.h>

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

CXCursor
CodeTree::getParentClassForWrapper(CXCursor cursor) const{
  if(verbose > 5) std::cerr << "Calling getParentClassForWrapper("
                            << cursor << ")\n";

  auto null_cursor = clang_getNullCursor();
  auto clazz = str(clang_getTypeSpelling(clang_getCursorType(cursor)));

  struct data_t{
    const CodeTree* tree;
    std::string clazz;
    CXCursor c;
    std::string preferred_parent;
    CXCursor first_parent;
  } data = { this, clazz, null_cursor, std::string(), null_cursor};

  //FIXME: handling of namespace?
  bool inheritance_preference =  false;
  auto it = inheritance_.find(clazz);
  if(it != inheritance_.end()){
    data.preferred_parent = it->second;
    inheritance_preference = true;
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
        if(!clang_Cursor_isNull(data.c)){
          if(verbose > 0){
            std::cerr << "Warning C++ class " << data.clazz
                      << " inherits from several classes. "
                      << "Only the inheritance from "
                      << clang_getCursorType(data.c)
                      << " will be mapped in the Julia binding.\n";
          }
          return CXChildVisit_Break;
        }

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
          if(clang_Cursor_isNull(data.first_parent)){
            data.first_parent = cursor;
          }
          //FIXME: handling of parent namespace?
          if(data.preferred_parent.size() == 0
             || str(clang_getTypeSpelling(clang_getCursorType(data.c)))
             == data.preferred_parent){
            data.c = cursor;
          }
        }
      }
    }
    return CXChildVisit_Continue;
  };

  if(!inheritance_preference || data.preferred_parent.size() != 0){
    if(verbose > 1){
      std::cerr << "Calling clang_visitChildren(" << cursor
                << ", visitor, &data) from " << __FUNCTION__ << "\n";
    }

    clang_visitChildren(cursor, visitor, &data);

    if(clang_Cursor_isNull(data.c) && !clang_Cursor_isNull(data.first_parent)){
      std::cerr << "Inheritance preference" << clazz << ":" << data.preferred_parent
                << " cannot be fulfilled as there is no such inheritance. "
                << "Inheritance from class "
                << clang_getCursorType(data.first_parent) << " is mapped into "
                << " Julia.\n";
      data.c = data.first_parent;
    }
  }

  data.c = clang_getCursorDefinition(data.c);

  //FIXME
  return data.c;
}

std::string CodeTree::wrapper_classsname(const std::string& classname) const{
  std::regex r("::");
  std::string s = std::string("Jl") + (classname.size() == 0 ? "Global" : std::regex_replace(classname, r, "_"));
  return s;
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
  indent(o, 1) << "// defined in "   << clang_getCursorLocation(cursor) << "\n";


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

    //generate inheritance mapping
    //FIXME: retrieve also parent classes which are not mapped to a super type
    //       and generate wrapper for the inherited public mehod.
    //       The whole parent tree must be parsed.
    CXCursor base = getParentClassForWrapper(t.cursor);
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
          std::cerr << "Warning: inheritance " << t.type_name << " <: "
                    << base << " will not be mapped because it is not yet "
            " supported for template classes.";
        }
      } else{
        o << "template<> struct SuperType<"
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

  //add_methods method
  indent(o, 1) << "void add_methods() const{\n";
  if(notype){
    indent(o, 2) << "auto& t = module_;\n";
  } else{
    indent(o, 2) << "auto& t = *type_;\n";
  }

  if(!notype){
    //extract ctor method name from the class name after
    //removing the possible namespace specification:
    std::regex re(".*::([^:]*)|[^:]*");
    std::cmatch cm;
    std::regex_match(t.type_name.c_str(), cm, re);
    std::string ctor(cm[1]);

    //Generate a wrapper for the implicit default ctor if needed
    if(t.default_ctor){
      FunctionWrapper::gen_ctor(o, 2, "t", t.template_parameters.empty(),
                                t.finalize, std::string());
    }
  }

  //Generate wrappers of the other methods
  //FIXME: add generation of accessor for templated classes
  //
  if(notype || clang_getCursorKind(t.cursor)!= CXCursor_ClassTemplate){

    //Generate wrappers for the class methods
    for(const auto& m: t.methods){
      generate_method_cxx(o, m);
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
          generate_accessor_cxx(o, &t, f, accessor_gen == accessor_mode_t::getter, 2);
        }
      }
    }
  } else if(t.template_parameter_combinations.size() > 0){
    generate_methods_of_templated_type_cxx(o, t);
    //FIXME: add class field accessors.
  }

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


  //  auto it = std::find(types_missing_def_.begin(), types_missing_def_.end(), typename_cxx);
  //  if(it!=types_missing_def_.end()){
  //    types_missing_def_.erase(it);
  //  }

  const auto& base = getParentClassForWrapper(cursor);

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
  std::string p(join_paths(out_cxx_dir_, std::string("jl") + module_name_ + ".cxx"));
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

  o << "#include \"jl" << module_name_ << ".h\"\n";

  //  for(const auto& include: forced_headers_){
  //    o << "#include \"" << include << "\"\n";
  //  }


  //FIXME
  //  for(const auto& t: types_missing_def_){
  //    o << "static_assert(is_type_complete_v<" << t
  //      << ">, \"Please include the header that defines the type "
  //      << t << "\");\n";
  //  }

  o << "#include \"dbg_msg.h\"\n";
  o << "#include \"Wrapper.h\"\n";

  std::vector<std::string> wrappers;

  std::string type_out_fname(std::string("jl") + module_name_ + ".cxx");

  //File stream to write type wrapper.
  //Fall back to current file, not expected to happen apart from a bug
  std::ofstream* pout = &o;
  std::ofstream type_out;

  int i_towrap_type = -1;
  for(const auto& c: types_){
    if(!c.to_wrap) continue;
    ++i_towrap_type;

    if(towrap_type_filenames_.size() > 0
       && towrap_type_filenames_[i_towrap_type] != type_out_fname){
      type_out_fname = towrap_type_filenames_.at(i_towrap_type);
      std::string type_out_fpath = join_paths(out_cxx_dir_, towrap_type_filenames_[i_towrap_type]);
      type_out = checked_open(type_out_fpath);
      pout = &type_out;
      generate_type_wrapper_header(type_out);
    }

    const auto& t = clang_getCursorType(c.cursor);

    if(is_type_vetoed(c.type_name)) continue;

    if(c.type_name.size() == 0 //holder of global functions and variables
       || t.kind == CXType_Record || c.template_parameter_combinations.size() > 0){
      wrappers.emplace_back(wrapper_classsname(c.type_name));
      generate_cxx_for_type((*pout), c);
    }
  }

  o << "\n";
  for(const auto& w: wrappers){
    o << "class " << w << ";\n";
  }

  o << "\n";

  for(const auto& w: wrappers){
    o << "std::shared_ptr<Wrapper> new" << w << "(jlcxx::Module&);\n";
  }


  o << "\n\nJLCXX_MODULE define_julia_module(jlcxx::Module& jlModule){\n";

  indent(o, 1) << "std::vector<std::shared_ptr<Wrapper>> wrappers = {\n";
  std::string sep;
  for(const auto& w: wrappers){
    indent(o << sep, 2) << "std::shared_ptr<Wrapper>(new" << w << "(jlModule))";
    sep = ",\n";
  }
  o << "\n";
  indent(o, 1) << "};\n";

  for(const auto& e: enums_){
    generate_enum_cxx(o, e.cursor);
  }

  indent(o, 1) << "for(const auto& w: wrappers) w->add_methods();\n";

  //FIXME: generate a wrapper class for the globals as for the class
  //  bool global_class = false;
  //  if(functions_.size() > 0 || vars_.size() > 0){
  //    global_class = true;
  //    TypeRcd fake_type;
  //    fake_type.methods = functions_;
  //    fake_type.fields = vars_;
  //    //FIXME: handle name clash in case a class to wrap has name "globals"
  //    std::string type_out_fpath = join_paths(out_cxx_dir_, "JlGlobals.cxx");
  //    type_out = checked_open(type_out_fpath);
  //
  //    generate_type_wrapper_header(type_out);
  //    generate_cxx_for_type(type_out, fake_type);
  //  }


  //  if(functions_.size() > 0 || vars_.size() > 0){
  //    indent(o, 1) << "/**********************************************************************\n";
  //    indent(o, 1) << " * Wrappers for global functions and variables including\n";
  //    indent(o, 1) << " * class static members\n";
  //    indent(o, 1) << " */\n";
  //
  //    //    indent(o, 1) << "auto& t = jlModule;\n";
  //    indent(o, 1) << "auto& module_ = jlModule;\n";
  //  }
  //
  //  for(const auto& f: functions_){
  //    generate_method_cxx(o, f);
  //  }
  //
  //  for(const auto& v: vars_){
  //    auto accessor_gen = check_veto_list_for_var_or_field(v, true);
  //    if(accessor_gen != accessor_mode_t::none){
  //      generate_accessor_cxx(o, nullptr, v, accessor_gen == accessor_mode_t::getter, 1);
  //    }
  //  }
  //
  //  if(functions_.size() > 0 || vars_.size() > 0){
  //    indent(o << "\n", 1) << "/* End of global function wrappers\n";
  //    indent(o, 1) << " **********************************************************************/\n\n";
  //  }
  //
  //
  //  indent(o, 1) << "DEBUG_MSG(\"End of wrapper definitions\");\n";
  o << "\n}\n";


  std::ofstream o2 = checked_open(join_paths(out_cxx_dir_, "dbg_msg.h"));
  o2 << "#ifdef VERBOSE_IMPORT\n"
    "#  define DEBUG_MSG(a) std::cerr << a << \"\\n\"\n"
    "#else\n"
    "#  define DEBUG_MSG(a)\n"
    "#endif\n"
    "#define __HERE__  __FILE__ \":\" QUOTE2(__LINE__)\n"
    "#define QUOTE(arg) #arg\n"
    "#define QUOTE2(arg) QUOTE(arg)\n";
  o2.close();

  o2 = checked_open(join_paths(out_cxx_dir_, "Wrapper.h"));
  o2 << "#include \"jlcxx/jlcxx.hpp\"\n\n"
    "struct Wrapper{\n";
  indent(o2, 1) << "Wrapper(jlcxx::Module& module): module_(module) {};\n";
  indent(o2, 1) << "virtual ~Wrapper() {};\n";
  indent(o2, 1) << "virtual void add_methods() const = 0;\n";
  o2 << "\nprotected:\n";
  indent(o2, 1) << "jlcxx::Module& module_;\n";
  o2 << "};\n";
  o2.close();

  o2 = std::ofstream(join_paths(out_cxx_dir_, "generated_cxx"));
  o2 << "jl" << module_name_ << ".cxx";
  for(const auto& fname: towrap_type_filenames_set_){
    o2 << " " << fname;
  }
  o2.close();

  show_stats(std::cout);
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
    abort();
    //exit(1);
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

  FunctionWrapper helper(MethodRcd(cursor), type_rcd, type_map_, "", "", nindents);

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
CodeTree::generate_method_cxx(std::ostream& o, const MethodRcd& method){
  return method_cxx_decl(o, method, "", "", 2);
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
CodeTree::method_cxx_decl(std::ostream& o, const MethodRcd& method,
                          std::string varname, std::string classname, int nindents,
                          bool templated){


  TypeRcd* pTypeRcd = find_class_of_method(method.cursor);

  FunctionWrapper wrapper(method, pTypeRcd, type_map_, varname, classname, nindents,
                          templated);

  //FIXME: check that code below is needed. Should now be vetoed upstream
  if(std::find(veto_list_.begin(), veto_list_.end(), wrapper.signature()) != veto_list_.end()){
    if(verbose > 0){
      std::cerr << "Info: " << "func " << wrapper.signature() << " vetoed\n";
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
CodeTree::generate_templated_type_cxx(std::ostream& o, const TypeRcd& type_rcd){

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
  indent(o, 1) << "// defined in "   << clang_getCursorLocation(cursor) << "\n";

  //types.add_type<Parametric<
  indent(o,1) << "t"
              << " = types.add_type<jlcxx::Parametric<";

  //TypeVar<1>, TypeVar<2>
  const unsigned nparams = type_rcd.template_parameter_combinations[0].size();
  const char* sep = "";
  for(unsigned i = 1; i <= nparams; ++i){
    o << sep << "jlcxx::TypeVar<" << i << ">";
    sep = ", ";
  }
  //>>("TemplateType")
  o << ">>(\""    << typename_jl << "\");\n";


  if(export_mode_ == export_mode_t::all) to_export_.insert(typename_jl);

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
  indent(o, 3) << "auto module_ = this->module_;";
  //        typedef A<T1, T2> T;
  if(t.methods.size() > 0){
    indent(o, 3) << "typedef " <<  t.type_name << "<" << param_list2 << "> WrappedType;\n";
  }

  //    wrapped.constructor<>();
  if(t.default_ctor){
    FunctionWrapper::gen_ctor(o, 3, "wrapped", /*templated=*/true,
                              t.finalize, std::string());
  }

  //        wrapped.method("get_first", [](const T& a) -> T1 { return a.get_first(); });
  //        wrapped.method("get_second", [](T& a, const T2& b) { a.set_second(b); });
  for(const auto& m: t.methods){
    method_cxx_decl(o, m, "wrapped", "WrappedType", 3, /*templated=*/true);
  }

  if(override_base_){
    indent(o << "\n", 3) << "module_.unset_override_module();\n";
    override_base_ = false;
  }

  //  };
  indent(o,2) << "};\n";


  //t1.apply<
  indent(o, 2) << "t.apply<";
  const char* sep1 = ""        ;
  //In order to declare the type a type depend on through its template parameters,
  //we must travel the parameter type combination list in reverse order:
  for(int i = t.template_parameter_combinations.size() - 1; i >= 0; --i){
    const auto& combi = t.template_parameter_combinations[i];
    //TemplateType<
    o << sep1 << t.type_name << "<";
    sep1 = ", ";

    //P1, P2
    const char* sep2 ="";
    for(const auto& arg_typename: combi){
      o << sep2 << arg_typename;
      sep2 = ", ";
    }
    //>
    o << ">";
  }
  //>(t1_decl_methods);"
  o << ">(" << decl_methods << ");\n";

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
    "@wrapmodule(()->\"" << shared_lib_basename<< "\")\n"
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


  if(str(clang_getCursorSpelling(cursor)).size() == 0){
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


  FunctionWrapper wrapper(MethodRcd(cursor), nullptr, type_map_);
  if(in_veto_list(wrapper.signature())){
    //  if(std::find(veto_list_.begin(), veto_list_.end(), wrapper.signature()) != veto_list_.end()){
    if(verbose > 0){
      std::cerr << "Info: " << "func " << wrapper.signature() << " vetoed\n";
    }
    return ;
  }


  std::vector<CXType> missing_types;
  int min_args;
  std::tie(missing_types, min_args) = visit_function_arg_and_return_types(cursor);

  if(!auto_veto_ || !inform_missing_types(missing_types, MethodRcd(cursor))){
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
  static std::regex re("std::(vector|valarray)[[:space:]]*<");

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
  auto eltype_name = fully_qualified_name(eltype);

  auto eltype_rcd = std::find_if(types_.begin(), types_.end(),
                                 [&](auto a){ return eltype_name == a.type_name; });

  if(eltype_rcd == types_.end()){
    if(verbose > 0){
      std::cerr << "WARNING: type " << eltype_name << " was not found in "
        " the type registry. The type " << type << " might miss a julia mappring.\n";
    }
    return;
  }

  eltype_rcd->to_wrap = true;

  bool isconst = clang_isConstQualifiedType(type);
  bool isptr   = (type.kind == CXType_Pointer);
  if(isptr){
    if(isconst) eltype_rcd->stl_const_ptr = true;
    else        eltype_rcd->stl_ptr = true;
  } else{//!isptr
    if(isconst) eltype_rcd->stl_const = true;
    else        eltype_rcd->stl = true;
  }
}

bool
CodeTree::register_type(const CXType& type){

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

  for(const auto& c: defs){
    if(clang_Cursor_isNull(c)){
      //a POD type, no definition to include.
      continue;
    }

    const auto& type0 = clang_getCursorType(c);

    //by retrieving the type from the definition cursor, we discard
    //the qualifiers.
    std::string type0_name = fully_qualified_name(type0); ////str(clang_getTypeSpelling(type0));

    if(type0_name.size() == 0){
      std::cerr << "Empty spelling for cursor of kind "
                << clang_getCursorKind(c)
                << " found in "
                << clang_getCursorLocation(c)
                << ". type: " << type0
                << " of kind " << type0.kind
                << "\n";
    }

    std::string type0_name_base = type0_name;
    auto cc = clang_getSpecializedCursorTemplate(c);

    if(!clang_Cursor_isNull(cc) && !clang_equalCursors(cc, c)){//a template
      type0_name_base = fully_qualified_name(cc);
      if(is_natively_supported(type0_name_base)){
        continue;
      }
    }

    if(type0.kind == CXType_Record){

      bool found = false;

      int i = -1;

      if(!clang_Cursor_isNull(cc) && !clang_equalCursors(cc, c)){//a template
        i = add_type(cc);
        if(i == types_.size() - 1){//new type
          types_[i].template_parameters = get_template_parameters(cc);
        }
      } else{
        i = add_type(c);
      }

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
  }

  return true;
}



std::tuple<std::vector<CXType>, int>
CodeTree::visit_function_arg_and_return_types(CXCursor cursor){

  const auto& method_type = clang_getCursorType(cursor);
  const auto return_type = clang_getResultType(method_type);

  //TODO: find a more robust way than comparing identifier  names.
  auto is_class_param = [cursor, this](CXType type){
    TypeRcd* pTypeRcd = find_class_of_method(cursor);
    if(pTypeRcd==nullptr) return true;
    auto type_name = remove_cv(str(clang_getTypeSpelling(base_type_(type))));
    auto params = pTypeRcd->template_parameters;
    return std::find(params.begin(), params.end(), type_name) != params.end();
  };

  std::vector<CXType> missing_types;

  if(return_type.kind != CXType_Void){
    if(verbose > 3) std::cerr << cursor << ", return type: " << return_type << "\n";
    if(is_class_param(return_type)){
      if(verbose > 3) std::cerr << return_type 
                                << " identified as a parameter of the holding class\n";
    } else if(type_map_.is_mapped(return_type, /*as_return=*/ true)){
      if(verbose > 3) std::cerr << return_type << " is mapped to an alternative type.\n";
    } else{
      bool rc = register_type(return_type);
      if(!rc) missing_types.push_back(return_type);
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
        bool rc = register_type(argtype);
        if(!rc) missing_types.push_back(argtype);
      }
    }
  }

  for(auto const& x: missing_types) types_missing_def_.insert(fully_qualified_name(base_type(x)));

  int min_args = get_min_required_args(cursor);

  if(verbose > 3) std::cerr << __FUNCTION__ << "(" << cursor << ") -> min_args = "
                            << min_args << "\n";

  return std::make_tuple(missing_types, min_args);
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

  //FIXME: handle the case where the method is defined in the parent class
  // => would need to define a Julia method that will throw an exception
  //This is not handled either when the access is restricted.
  if(is_method_deleted(cursor)){
    if(verbose > 1) std::cerr << "Method " << cursor << " is deleted.\n";
    return;
  }

  TypeRcd* pTypeRcd = find_class_of_method(cursor);

  FunctionWrapper wrapper(MethodRcd(cursor), pTypeRcd, type_map_);

  if(in_veto_list(wrapper.signature())){
    //  if(std::find(veto_list_.begin(), veto_list_.end(), wrapper.signature()) != veto_list_.end()){
    if(verbose > 0){
      std::cerr << "Info: " << "func " << wrapper.signature() << " vetoed\n";
    }
    return ;
  }

  std::vector<CXType> missing_types;
  int min_args;
  std::tie(missing_types, min_args) = visit_function_arg_and_return_types(cursor);

  auto p = find_class_of_method(cursor);

  if(!p){
    std::cerr << "Warning: method " << cursor << " found at "
              << clang_getCursorLocation(cursor)
              << " skipped because the definition of its class "
      "was not found.\n";
  } else if(!auto_veto_ || !inform_missing_types(missing_types, MethodRcd(cursor), p)){
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
    std::string funcname =  FunctionWrapper(methodRcd, classRcd, type_map_).signature();
    std::cerr << "Info: missing definition of type"
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
  if(verbose > 3)  std::cerr << __FUNCTION__ << "(" << cursor << "), cursor location: "
                             << clang_getCursorLocation(cursor) << "\n";

  //     declaration or definition
  auto range = clang_getCursorExtent(cursor);

  CXToken* toks = nullptr;
  unsigned nToks = 0;
  bool deleted = false;
  bool expecting_operator = false;
  int in_paren = 0;
  if(!clang_Range_isNull(range)){
    clang_tokenize(unit_, range, &toks, &nToks);
    bool equal = false;
    for(unsigned i =0; i < nToks; ++i){
      const auto& s = str(clang_getTokenSpelling(unit_, toks[i]));

      if(s == "("){ ++in_paren; continue; }
      if(s == ")"){ --in_paren; continue; }

      if(equal){
        if(s == "delete") deleted = true;
        break;
      }
      if(s == "=" && !expecting_operator && !in_paren) equal = true;

      if(s== "{"){ //body start
        break;
      }

      if(s == "operator") expecting_operator = true;
      else expecting_operator = false;
    }
    clang_disposeTokens(unit_, toks, nToks);
  } else{
    if(verbose > 1) std::cerr << __FUNCTION__ << " cursor extend not found!\n";
  }

  if(verbose > 3) std::cerr << "-> is " << (deleted?"":"not ") << "deleted\n";
  return deleted;
}

void
CodeTree::visit_class_constructor(CXCursor cursor){

  if(verbose > 3) std::cerr << __FUNCTION__ << "(" << cursor << ")\n";

  TypeRcd* pTypeRcd = find_class_of_method(cursor);

  FunctionWrapper wrapper(MethodRcd(cursor), pTypeRcd, type_map_);

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
  std::tie(missing_types, min_args) = visit_function_arg_and_return_types(cursor);

  auto* p = find_class_of_method(cursor);

  auto access = clang_getCXXAccessSpecifier(cursor);

  bool is_def_ctor = (0 == clang_Cursor_getNumArguments(cursor));

  if(p && !is_def_ctor){
    if(access == CX_CXXPublic
       && (!auto_veto_ || !inform_missing_types(missing_types, MethodRcd(cursor), p))){

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

  auto rc =  (kind == CXCursor_ClassDecl || kind == CXCursor_StructDecl)
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

CXChildVisitResult CodeTree::visit(CXCursor cursor, CXCursor parent, CXClientData client_data){
  CodeTree& tree = *static_cast<CodeTree*>(client_data);

  if(!pp) pp = clang_getCursorPrintingPolicy(cursor);

  if(verbose > 5) std::cerr << "visiting " << clang_getCursorLocation(cursor) << "\n";

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
  } else if(kind == CXCursor_EnumDecl){
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

bool
CodeTree::parse(){

  if(!fs::is_directory(out_cxx_dir_) && !fs::create_directories(out_cxx_dir_)){
    std::cerr << "Failed to create directory " << out_cxx_dir_ << ".\n";
    return false;
  }

  header_file_path_ = join_paths(out_cxx_dir_,  std::string("jl") + module_name_ + ".h");

  std::ofstream header_file(header_file_path_, out_open_mode_);
  if(header_file.tellp() != 0){
    std::cerr << "File " << header_file_path_
              << " is in the way. Remove it or use the --force actions "
      "to disable the check.\n";
    return false;
  }

  for(const auto& fname: forced_headers_){
    header_file << "#include \"" << fname << "\"\n";
  }

  files_to_wrap_fullpaths_.clear();
  for(const auto& fname: files_to_wrap_){
    files_to_wrap_fullpaths_.push_back(resolve_include_path(fname));
    header_file << "#include \"" << fname << "\"\n";
  }

  header_file.close();

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
  return o;
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

  //Move the class parents to be declared before their children
  for(unsigned i = 0; i < types_.size(); ++i){
    CXCursor parent = getParentClassForWrapper(types_[i].cursor);

    if(clang_Cursor_isNull(parent))  continue;

    for(unsigned j = i + 1 ; j < types_.size(); ++j){
      if(clang_equalCursors(parent, types_[j].cursor)){
        std::swap(types_[i], types_[j]);
        j = i + 1;
        --i;
        break;
      }
    }
  }

  ///  //Remove from types_missing_def_ definition found in a different
  ///  //Translation unit
  ///  decltype(types_missing_def_)::const_iterator next_it;
  ///  decltype(types_missing_def_) to_remove;
  ///  for(decltype(types_missing_def_)::const_iterator it = types_missing_def_.begin();
  ///      it != types_missing_def_.end(); ++it){ // = ++next_it){
  ///    const auto& tn = *it;
  ///
  ///    for(const auto& t: types_){
  ///       if(str(clang_getTypeSpelling(clang_getCursorType(t.cursor))) == tn){
  ///         to_remove.insert(tn);
  ///         break;
  ///       }
  ///    }
  ///  }
  ///  for(const auto& t: to_remove){
  ///     types_missing_def_.erase(t);
  ///  }

  if(verbose > 2){
    std::cerr << "\nType list at end of preprocess:\n";
    for(const auto& t: types_){
      std::cerr << t.type_name << "\n";
    }
    std::cerr << "\n";
  }

  //add a fake_type to hold global functions and variables
  if(functions_.size() > 0 || vars_.size() > 0){
    types_.emplace_back();
    types_.back().methods = functions_;
    types_.back().fields = vars_;
    types_.back().to_wrap = true;
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
    //exit(1);
    abort();
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

//std::vector<unsigned> get_order_of_template_applies(){
//  std::tuple<std::string, unsigned, unsigned> tpl_types;
//
//  for(unsigned itype = 0; itype < types_.size(); ++itype){
//    const auto& type = types_[itype];
//    for(usinged icombi = 0; icombi < type.template_parameter_combinations.size(); ++icombi){
//      const auto& combi = type.template_parameter_combinations[icombi];
//      std::stringstream buf;
//      buf << types_[itype].type_name  << "<";
//      const char* sep = "";
//      for(unsigned iparam = 0; iparam < combi.size(); ++iparam){
//        buf << sep << combi[iparam];
//        sep = ", ";
//      }
//      buf << ">";
//    }
//    tpl_type.push_back(std::tuple<std::string, unsigned, unsigned>(buf.str, itype, icombi));
//  }
//  return indices;
//}

void CodeTree::update_wrapper_filenames(){
  int ifile = 0;
  int itype = 0;
  int igroupedtype = 0;
  towrap_type_filenames_.clear();
  towrap_type_filenames_set_.clear();
  for(const auto& c: types_){
    if(!c.to_wrap) continue;
    ++itype;
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

bool CodeTree::is_natively_supported(const CXType& type) const{
  return is_natively_supported(fully_qualified_name(type));
}

bool CodeTree::is_natively_supported(const std::string& type_fqn) const{
  static std::vector<std::string> natively_supported = {
    "std::string",
    "std::wstring",
    "std::vector",
    "std::valarray",
    "jlcxx::SafeCFunction",
    "std::shared_ptr",
    "std::unique_ptr",
    "std::weak_ptr"
  };

  if(verbose > 4) std::cerr << __FUNCTION__ << "(" << type_fqn << ")\n";

  return std::find(natively_supported.begin(), natively_supported.end(), type_fqn)
    != natively_supported.end();
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

