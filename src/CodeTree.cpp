// Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>
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

#include "utils.h"

#include "clang/AST/Type.h" //FOR DEBUG


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

    //FIXME: do apply after all class wrappers have been defined
    //       as done for non-template class and calls to method
    const auto& kind = clang_getCursorKind(cursor);
    if(kind == CXCursor_CXXBaseSpecifier){
      std::string parent_name = str(clang_getTypeSpelling(clang_getCursorType(cursor)));
      const auto& inheritance_access = clang_getCXXAccessSpecifier(cursor);
      const auto& t1 = clang_getCursorType(cursor);
      if(inheritance_access == CX_CXXPublic){
        //         || inheritance_access == CX_CXXProtected){
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
          if(clang_equalTypes(t1, t2)){
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

std::ostream&
CodeTree::generate_cxx(std::ostream& o){
  o << "#include <type_traits>\n"
    "#include \"jlcxx/jlcxx.hpp\"\n"
    "#include \"jlcxx/functions.hpp\"\n"
    "#include \"jlcxx/stl.hpp\"\n\n";

  o << "#include " << filename_ << "\n";

  //  for(const auto& include: forced_headers_){
  //    o << "#include \"" << include << "\"\n";
  //  }



  o << "\n#ifdef VERBOSE_IMPORT\n";
  indent(o << "#",1) << "define DEBUG_MSG(a) std::cerr << a << \"\\n\"\n";
  o << "#else\n";
  indent(o << "#",1) << "define DEBUG_MSG(a)\n";
  o << "#endif\n";

  o <<"#define __HERE__  __FILE__ \":\" QUOTE2(__LINE__)\n"
    "#define QUOTE(arg) #arg\n"
    "#define QUOTE2(arg) QUOTE(arg)\n";

  //FIXME
  //  for(const auto& t: types_missing_def_){
  //    o << "static_assert(is_type_complete_v<" << t
  //      << ">, \"Please include the header that defines the type "
  //      << t << "\");\n";
  //  }


  std::vector<CXType> no_mirrored_types;

  o << "\nnamespace jlcxx {\n";
  for(const auto& c: types_){
    if(!c.to_wrap) continue;

    //    std::cerr << "==> Comments of " << c.type_name << ":\n"
    //              << clang_Cursor_getRawCommentText(c.cursor)
    //              << "\n";

    const auto& t = clang_getCursorType(c.cursor);
    auto type_name_cxx = str(clang_getTypeSpelling(t));

    if(is_type_vetoed(c.type_name)) continue;

    // Generate the IsMirrorType<> statements
    if(t.kind == CXType_Record || c.template_parameter_combinations.size() > 0){
      auto canonical_type = clang_getCanonicalType(t);
      bool done = false;
      if(canonical_type.kind != CXType_Invalid){
        auto it = std::find_if(no_mirrored_types.begin(), no_mirrored_types.end(),
			       [canonical_type](const CXType& t){  return clang_equalTypes(t, canonical_type); });
        if(it == no_mirrored_types.end()){
          no_mirrored_types.push_back(canonical_type);
        } else{
          done = true;
        }
      }
      if(!done){ //no-mirrored statement not yet generated
        if(verbose > 2) std::cerr << "Disable mirrored type for type " << type_name_cxx << "\n";
        if(c.template_parameter_combinations.size() > 0){
          for(unsigned i = 0; i < c.template_parameter_combinations.size(); ++i){
            indent(o, 1) << "template<> struct IsMirroredType<" << c.name(i)<< "> : std::false_type { };\n";
          }
        } else{
          indent(o, 1) << "template<> struct IsMirroredType<" << type_name_cxx << "> : std::false_type { };\n";
        }
      }
    }
  }

  //Specify the class inheritances:
  for(const auto& t:types_){
    if(!t.to_wrap) continue;
    CXCursor base = getParentClassForWrapper(t.cursor);
    if(!clang_Cursor_isNull(base)){
      indent(o, 1) << "template<> struct SuperType<"
                   << t.type_name
                   << "> { typedef " << fully_qualified_name(base) << " type; };\n";
    }
  }

  o << "}\n"
    "\n"
    "JLCXX_MODULE define_julia_module(jlcxx::Module& types){\n";

  for(const auto& e: enums_){
    generate_enum_cxx(o, e.cursor);
  }

  for(const auto& t: types_){
    if(!t.to_wrap) continue;
    if(is_type_vetoed(t.type_name)){
      continue;
    }
    if(t.template_parameter_combinations.size() > 0){
      o << "\n";
      generate_templated_type_cxx(o, t);
    } else if(clang_getCursorKind(t.cursor)!= CXCursor_ClassTemplate){
      o << "\n";
      generate_type_cxx(o, t);
    }
  }

  bool comment_header_generated = false;

  comment_header_generated = false;
  auto gen_comment_header = [&](const TypeRcd& t){
    if(!comment_header_generated){
      indent(o << "\n", 1)
        << "/**********************************************************************/\n";
      indent(o, 1) << "/* Wrappers for the methods of class " << t.type_name << "\n";
      indent(o, 1) << " */\n";
      comment_header_generated = true;
    }
  };

  for(const auto& t: types_){
    if(!t.to_wrap) continue;

    if(is_type_vetoed(t.type_name)){
      continue;
    }

    comment_header_generated = false;

    std::regex re(".*::([^:]*)|[^:]*");
    std::cmatch cm;
    std::regex_match(t.type_name.c_str(), cm, re);
    std::string ctor(cm[1]);

    //FIXME: add generation of accessor for templated classes
    if(/*t.template_parameter_combinations.size() == 0
         &&*/ clang_getCursorKind(t.cursor)!= CXCursor_ClassTemplate){
      for(const auto& m: t.methods){
        gen_comment_header(t);
        generate_method_cxx(o, m);
        if(test_build_) test_build(o);
      }
      if(accessor_generation_enabled()){
	for(const auto& f: t.fields){
	  auto accessor_gen = check_veto_list_for_var_or_field(f, false);
	  if(accessor_gen != accessor_mode_t::none){
            gen_comment_header(t);
            generate_accessor_cxx(o, &t, f, accessor_gen == accessor_mode_t::getter, 1);
          }
        }
      }

      if(comment_header_generated){
        indent(o << "\n", 1) << "/* End of " << t.type_name << " class method wrappers\n";
        indent(o, 1) << " **********************************************************************/\n\n";
      }
    }
  }


  // Generate declaration of template class method wrappers
  // In order to declare the type another type depends on through its template parameters,
  // we must travel the types_list in reverse order:
  for(int i = types_.size() - 1; i >= 0; --i){
    const auto& t = types_[i];
    if(!t.to_wrap) continue;
    if(is_type_vetoed(t.type_name)) continue;

    comment_header_generated = false;
    if(t.template_parameter_combinations.size() > 0){
      gen_comment_header(t);
      generate_methods_of_templated_type_cxx(o, t);
      indent(o << "\n", 1) << "/* End of " << t.type_name << " class method wrappers\n";
      indent(o, 1) << " **********************************************************************/\n\n";
      if(test_build_) test_build(o);
    }
  }

  if(functions_.size() > 0 || vars_.size() > 0){
    indent(o, 1) << "/**********************************************************************\n";
    indent(o, 1) << " * Wrappers for global functions and variables including\n";
    indent(o, 1) << " * class static members\n";
    indent(o, 1) << " */\n";
  }

  for(const auto& f: functions_){
    generate_method_cxx(o, f);
    if(test_build_) test_build(o);
  }

  if(override_base_){
    indent(o, 1) << "types.unset_override_module();\n";
    override_base_ = false;
  }
  
  for(const auto& v: vars_){
    auto accessor_gen = check_veto_list_for_var_or_field(v, true);
    if(accessor_gen != accessor_mode_t::none){
      generate_accessor_cxx(o, nullptr, v, accessor_gen == accessor_mode_t::getter, 1);
    }
  }

  if(functions_.size() > 0 || vars_.size() > 0){
    indent(o, 1) << "/* End of global function wrappers\n";
    indent(o, 1) << " **********************************************************************/\n\n";
  }


  indent(o, 1) << "DEBUG_MSG(\"End of wrapper definitions\");\n";
  o << "\n}\n";

  show_stats(std::cout);

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
  FunctionWrapper helper(MethodRcd(cursor), type_rcd, "", "", nindents);

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

std::ostream&
CodeTree::generate_type_cxx(std::ostream& o, const TypeRcd& type_rcd){

  const auto& cursor = type_rcd.cursor;
  const auto& type = clang_getCursorType(cursor);
  const auto& typename_cxx = type_rcd.type_name;
  const auto& typename_jl = jl_type_name(typename_cxx);

  indent(o,1) << "DEBUG_MSG(\"Adding wrapper for type " << typename_cxx
              << " (\" __HERE__ \")\");\n";
  indent(o, 1) << "// defined in "   << clang_getCursorLocation(cursor) << "\n";

  indent(o, 1);
  if(type_rcd.methods.size() > 0 || type_rcd.explicit_ctor || type_rcd.vetoed_default_ctor){
    o << "auto t" << type_rcd.id << " = ";
  }

  if(typename_cxx.size() == 0){
    std::cerr << "Bug found. Unexpected empty string for TypeRcd::type_name of cursor "
              << "'" << type_rcd.cursor << "' defined in "
              << clang_getCursorLocation(type_rcd.cursor)
              << "\n";
  }

  ++nwraps_.types;
  o << "types.add_type<" << typename_cxx
    << ">(\"" << typename_jl << "\"";

  if(export_mode_ == export_mode_t::all) to_export_.insert(typename_jl);

  //  auto it = std::find(types_missing_def_.begin(), types_missing_def_.end(), typename_cxx);
  //  if(it!=types_missing_def_.end()){
  //    types_missing_def_.erase(it);
  //  }

  const auto& base = getParentClassForWrapper(cursor);

  if(!clang_Cursor_isNull(base)){
    const auto& base_type = clang_getCursorType(base);
    const auto& base_name_cxx = str(clang_getTypeSpelling(base_type));

    //FIXME: should it be base_name_jl ?
    o << ", jlcxx::julia_base_type<" << base_name_cxx << ">()";
  }

  o << ")";
  o << ";\n";
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
  return method_cxx_decl(o, method);
}


bool CodeTree::has_an_implicit_default_ctor(const CXCursor& def) const{
  if(verbose > 3) std::cerr << __FUNCTION__ << "(" << def << ")\n";
  if(clang_isInvalid(clang_getCursorKind(def))){
    std::cerr << __FUNCTION__ << "(" << def << ") -> "
              << "(" << false << ", null) [definition not found]" << "\n";
    return false;
  }

  bool with_ctor = false;
  struct data_t {
    bool explicit_def_ctor;
    bool implicit_def_ctor;
    const CodeTree* tree;
  } data = {false, true, this};

  clang_visitChildren(def, [](CXCursor cursor, CXCursor, CXClientData data_){
    auto& data = *static_cast<data_t*>(data_);
    const auto& kind = clang_getCursorKind(cursor);
    const auto& access = clang_getCXXAccessSpecifier(cursor);
    if(kind == CXCursor_Constructor){
      if(0 == clang_Cursor_getNumArguments(cursor)){ //default ctor
        if(access != CX_CXXPublic
           || data.tree->is_method_deleted(cursor)){
          data.implicit_def_ctor = false;
        } else {
          data.explicit_def_ctor = true;
        }
      } else{ ////a non-default ctor
        data.implicit_def_ctor = false;
      }
    }

    return CXChildVisit_Continue;
  }, &data);

  return data.implicit_def_ctor && !data.explicit_def_ctor;
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

  FunctionWrapper wrapper(method, pTypeRcd, varname, classname, nindents,
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
    indent(o << "\n", 1) << "types.unset_override_module();\n";
    override_base_ = new_override_base;
  } else if(!override_base_ && new_override_base){
    indent(o, 1) << "types.set_override_module(jl_base_module);\n";
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
    for(const auto& n: wrapper.generated_jl_functions()) to_export_.insert(n);
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

  //types.add_type<Parametric<
  indent(o,1) << "auto t" << type_rcd.id
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

  auto param_list1 = join(myapply(t.template_parameters,
                                  [](const std::string& x){
                                    return std::string("typename ") + x;}),
                          ", ");
  auto param_list2 = join(t.template_parameters, ", ");

  //  auto t1_decl_methods = []<typename T1, typename T2>(jlcxx::TypeWrapper<T1, T2> wrapped){
  indent(o,1) << "auto " << decl_methods << " = []<" << param_list1
              << "> (jlcxx::TypeWrapper<" << t.type_name << "<" << param_list2
              << ">> wrapped){\n";
  //        typedef A<T1, T2> T;
  if(t.methods.size() > 0){
    indent(o, 2) << "typedef " <<  t.type_name << "<" << param_list2 << "> WrappedType;\n";
  }

  //    wrapped.constructor<>();
  //        wrapped.method("get_first", [](const T& a) -> T1 { return a.get_first(); });
  //        wrapped.method("get_second", [](T& a, const T2& b) { a.set_second(b); });
  for(const auto& m: t.methods){
    method_cxx_decl(o, m, "wrapped", "WrappedType", 2, /*templated=*/true);
  }
  //  };
  indent(o,1) << "};\n";


  //t1.apply<
  indent(o, 1) << "t" << t.id  << ".apply<";
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
    "@wrapmodule(\"" << shared_lib_basename<< "\")\n"
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

  if(is_to_visit(cursor)) types_[index].to_wrap = true;

  if(verbose > 3) std::cerr << "Calling clang_visitChildren(" << cursor << ", visitor, &data) from " << __FUNCTION__ << "\n";
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
      //TODO: check that last character is / and throw an error if not.
      std::regex re(v.substr(1, v.size() - 2));
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


  FunctionWrapper wrapper(MethodRcd(cursor), nullptr);
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

  //FIXME: definitions_ is empty
  //-> simplify the code to definitions.push_back(d)
  ///  auto it = std::find_if(definitions.begin(), definitions.end(),[&d](const auto& c){
  ///    return clang_equalCursors(c, d);
  ///  });
  ///
  ///  if(it == definitions.end()){
  ///    definitions.push_back(d);
  ///  } else{
  ///    return std::make_tuple(usable, definitions);
  ///  }
  definitions.push_back(d);

  int nparams = clang_Type_getNumTemplateArguments(type0);

  for(int i = 0; i < nparams; ++i){
    bool u;
    std::vector<CXCursor> vd;
    auto param_type = base_type(clang_Type_getTemplateArgumentAsType(type0, i));
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
    return std::make_tuple(true,
                           clang_getNullCursor());
  }

  auto decl = clang_getTypeDeclaration(type0);

  //  auto tmp = clang_getSpecializedCursorTemplate(decl);
  //  if(!clang_Cursor_isNull(tmp)){
  //    decl = tmp;
  //  }

  CXCursor def = clang_getCursorDefinition(decl);

  if(clang_isInvalid(clang_getCursorKind(def))){
    if(verbose > 1){
      std::cerr << "Warning: failed to find definition of " << decl << " declared at "
                << clang_getCursorLocation(decl) << "\n";
    }
    usable = false;
    def = decl;
  }

  auto access = clang_getCXXAccessSpecifier(def);

  if(access == CX_CXXPrivate || access == CX_CXXProtected){
    usable = false;
  }

  return std::make_tuple(usable, def);
}

bool
CodeTree::register_type(const CXType& type){

  if(verbose > 3) std::cerr << __FUNCTION__ << "(" << type << "), type of kind "
                            << type.kind
                            << ".\n";
  
  std::string type_name = fully_qualified_name(type);

  std::vector<std::string> natively_supported = {
    "std::string",
    "std::wstring",
    "std::vector<std::string>",
    "std::vector<std::wstring>",
    "std::vector<bool>",
    "std::vector<char>",
    "std::vector<int>",
    "std::vector<long>",
    "std::vector<float>",
    "std::vector<double>",
    "std::vector<void*>",
  };

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
    std::string type0_name = str(clang_getTypeSpelling(type0));

    if(type0_name.size() == 0){
      std::cerr << "Empty spelling for cursor of kind "
                << clang_getCursorKind(c)
                << " found in "
                << clang_getCursorLocation(c)
                << ". type: " << type0
                << " of kind " << type0.kind
                << "\n";
      // abort();
    }

    if(std::find(natively_supported.begin(), natively_supported.end(), type0_name)
       != natively_supported.end()){
      continue;
    }

    if(type0.kind == CXType_Record){

      bool found = false;

      auto cc = clang_getSpecializedCursorTemplate(c);

      int i = -1;

      if(!clang_Cursor_isNull(cc) && !clang_equalCursors(cc, c)){//a template
	i = add_type(cc);
	if(i == types_.size() -1 ){//new type
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
      //abort();
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

  std::vector<CXType> missing_types;

  if(return_type.kind != CXType_Void){
    bool rc = register_type(return_type);
    if(!rc) missing_types.push_back(return_type);
  }

  for(int i = 0; i < clang_getNumArgTypes(method_type); ++i){
    auto argtype = clang_getArgType(method_type, i);
    if(verbose > 3) std::cerr << cursor << ", arg " << (i+1) << " type: " << argtype << "\n";
    if(argtype.kind != CXType_Void){
      bool rc = register_type(argtype);
      if(!rc) missing_types.push_back(argtype);
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
    std::cerr << "Calling clang_visitChildren(" << cursor
              << ", visitor, &data) from " << __FUNCTION__
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

  if(is_method_deleted(cursor)){
    if(verbose > 1) std::cerr << "Method " << cursor << " is deleted.\n";
    return;
  }

  TypeRcd* pTypeRcd = find_class_of_method(cursor);

  FunctionWrapper wrapper(MethodRcd(cursor), pTypeRcd);

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
    std::string funcname =  FunctionWrapper(methodRcd, classRcd).signature();
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
  auto range = function_decl_range(cursor);

  CXToken* toks = nullptr;
  unsigned nToks = 0;
  bool deleted = false;
  bool expecting_operator = false;
  if(!clang_Range_isNull(range)){
    clang_tokenize(unit_, range, &toks, &nToks);
    bool equal = false;
    for(unsigned i =0; i < nToks; ++i){
      const auto& s = str(clang_getTokenSpelling(unit_, toks[i]));
      if(equal){
        if(s == "delete") deleted = true;
        break;
      }
      if(s == "=" && !expecting_operator) equal = true;

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
  return deleted;
}

void
CodeTree::visit_class_constructor(CXCursor cursor){

  if(verbose > 3) std::cerr << __FUNCTION__ << "(" << cursor << ")\n";


  TypeRcd* pTypeRcd = find_class_of_method(cursor);

  FunctionWrapper wrapper(MethodRcd(cursor), pTypeRcd);

  if(std::find(veto_list_.begin(), veto_list_.end(), wrapper.signature()) != veto_list_.end()){
    if(verbose > 0){
      std::cerr << "Info: " << "func " << wrapper.signature() << " vetoed\n";
    }
    return ;
  }

  std::vector<CXType> missing_types;
  int min_args;
  std::tie(missing_types, min_args) = visit_function_arg_and_return_types(cursor);

  auto* p = find_class_of_method(cursor);

  auto access = clang_getCXXAccessSpecifier(cursor);

  if((0 == clang_Cursor_getNumArguments(cursor) //default ctor
      && access != CX_CXXPublic)
     || is_method_deleted(cursor)
     ){
    p->vetoed_default_ctor = true;
  } else if(p){
    p->explicit_ctor = true;
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
    rcd->fields.push_back(cursor);
  } else if(kind == CXCursor_VarDecl){
    auto type = clang_getCursorType(cursor);
    bool rc  = register_type(type);
    if(!rc) types_missing_def_.insert(fully_qualified_name(base_type(type)));
    vars_.push_back(cursor);
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

  auto nparams = clang_Type_getNumTemplateArguments(type);
  if(nparams <= 0) return false;

  std::vector<std::string> combi;
  for(decltype(nparams) i = 0; i < nparams; ++i){
    const auto& param_type = clang_Type_getTemplateArgumentAsType(type, i);
    //FIXME: add support for value template argument
    combi.push_back(str(clang_getTypeSpelling(param_type)));
  }

  auto& combi_list = pTypeRcd->template_parameter_combinations;
  if(combi_list.end() == std::find(combi_list.begin(), combi_list. end(),
                                   combi)){
    combi_list.push_back(combi);
  }

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

  if(kind != CXCursor_Constructor && !tree.is_to_visit(cursor)) return CXChildVisit_Continue;

  if(verbose > 1) std::cerr << "visiting " << clang_getCursorLocation(cursor)
                            << "\t cursor " << cursor
                            << " of kind " << kind
                            << ", type " << clang_getCursorType(cursor)
                            << ", and access " << access
                            << ", and type access " << type_access
                            << "\n";

  bool accessible = (access != CX_CXXProtected && access != CX_CXXPrivate);

  if(tree.visiting_a_templated_class_
     && kind!= CXCursor_CXXMethod
     && kind!= CXCursor_Constructor){
    return CXChildVisit_Continue;
  }

  if(kind == CXCursor_Namespace && accessible){
    return CXChildVisit_Recurse;
  } else if((kind == CXCursor_ClassDecl || kind == CXCursor_StructDecl)
            && clang_getCursorType(cursor).kind != CXType_Invalid
            && !tree.isAForwardDeclaration(cursor)
            && accessible){
    const auto& special = clang_getSpecializedCursorTemplate(cursor);
    if(clang_Cursor_isNull(special)){
      tree.visit_class(cursor);
    } else{
      //static std::regex re("<[^>]*>");
      //auto type_name = std::regex_replace(str(clang_getTypeSpelling(clang_getCursorType(cursor))), re, "");
      tree.visit_class_template_specialization(cursor);
    }
  } else if(kind == CXCursor_ClassTemplate && accessible){
    tree.visit_class(cursor);
  } else if(kind == CXCursor_FunctionDecl && accessible){
    tree.visit_global_function(cursor);
  } else if(kind == CXCursor_EnumDecl && accessible){
    tree.visit_enum(cursor);
  } else if(kind == CXCursor_TypedefDecl && accessible){
    //  tree.visit_typedef(cursor);
  } else if((kind == CXCursor_VarDecl || kind == CXCursor_FieldDecl) && accessible){
    tree.visit_field_or_global_variable(cursor);
  } else if(kind == CXCursor_CXXMethod && accessible){
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

void
CodeTree::parse(std::ofstream& header_file, const std::filesystem::path& header_file_path){
  for(const auto& fname: forced_headers_){
    header_file << "#include \"" << fname << "\"\n";
  }

  files_to_wrap_fullpaths_.clear();
  for(const auto& fname: files_to_wrap_){
    files_to_wrap_fullpaths_.push_back(resolve_include_path(fname));
    header_file << "#include \"" << fname << "\"\n";
  }

  header_file.close();

  filename_ = header_file_path.string();

  index_ = clang_createIndex(0, 0);

  std::vector<const char*> opts(opts_.size());
  for(unsigned i = 0; i < opts_.size(); ++i) opts[i] = opts_[i].c_str();

  if(verbose > 1){
    //Enable clang verbose option
    opts.push_back("-v");
  }

  if(verbose > 0){
    std::cerr << "Clang options:\n";
    for(const auto& x: opts) std::cerr << "\t" << x << "\n";
    std::cerr << "\n";
  }


  CXTranslationUnit unit = clang_parseTranslationUnit(index_, filename_.string().c_str(),
                                                      opts.data(), opts.size(),
                                                      nullptr, 0,
                                                      CXTranslationUnit_SkipFunctionBodies);
  unit_ = unit;


  diagnose(filename_.c_str(), unit);

  if (unit == nullptr) {
    std::cerr << "Unable to parse " << filename_ << ". Quitting.\n";
    exit(-1);
  }

  const auto&  cursor = clang_getTranslationUnitCursor(unit);

  if(verbose > 1) std::cerr << "Calling clang_visitChildren(" << cursor << ", CodeTree::visit, &data) from " << __FUNCTION__ << "\n";

  clang_visitChildren(cursor, CodeTree::visit, this);
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
}


//extract prefix from a string formatted as 'prefix::type_name':
std::string
CodeTree::get_prefix(const std::string& type_name) const{
  return std::regex_replace(type_name, std::regex("(.*::).*$"), "$1", std::regex_constants::format_no_copy);
}


std::ostream&
CodeTree::generate_enum_cxx(std::ostream& o, CXCursor cursor){

  bool anonymous_enum = clang_Cursor_isAnonymous(cursor);

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
  auto prefix_jl = jl_type_name(prefix_cxx);

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

    indent(o,1) << "types.add_bits<" << type_name << ">(\""
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
    if(clang_EnumDecl_isScoped(cursor)){
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

    indent(o,1) << "types.set_const(\"" << value_jl << "\", "
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

  if(verbose > 1) std::cerr << "Calling clang_visitChildren(" << cursor << ", visitor, &data) from " << __FUNCTION__ << "\n";
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
  }
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
