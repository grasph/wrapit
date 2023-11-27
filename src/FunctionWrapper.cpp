//-*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// vim: noai:ts=2:sw=2:expandtab
//
// Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>
//
#include "FunctionWrapper.h"
#include "utils.h"
#include "libclang-ext.h"
#include <sstream>
#include <regex>
#include <sstream>

std::ostream&
FunctionWrapper::gen_ctor(std::ostream& o){
  int nargsmin =  std::max(1, method.min_args); //no-arg ctor, aka "default ctor" generate elsewhere with direct call to gen_ctor(std::ostream& o, int nindents,
  //                                              const std::string& varname, bool templated, bool finalize, const std::string& arg_list)
  int nargsmax = clang_getNumArgTypes(method_type);

  indent(o << "\n", nindents)
    << "DEBUG_MSG(\"Adding wrapper for "
    << signature()
    << " (\" __HERE__ \")\");""\n";
  indent(o, nindents) << "// defined in "     << clang_getCursorLocation(method.cursor) << "\n";
  auto finalize = pTypeRcd && pTypeRcd->finalize;

  for(int nargs = nargsmin; nargs <= nargsmax; ++nargs){
    std::stringstream  buf;
    gen_arg_list(buf, nargs, "", /*argtypes_only = */ true);
    gen_ctor(o, nindents, varname, templated_, finalize,  buf.str());
  }
  generated_jl_functions_.insert(name_jl_);
  return o;
}

std::ostream&
FunctionWrapper::gen_ctor(std::ostream& o, int nindents,
                          const std::string& varname, bool templated,
                          bool finalize,
                          const std::string& arg_list){
  indent(o, nindents) << varname << "." << (templated ? "template " : "")
                      << "constructor<"
                      << arg_list
                      << ">(/*finalize=*/" << (finalize?"true":"false") << ");\n";
  return o;
}



std::ostream&
FunctionWrapper::gen_accessors(std::ostream& o, bool getter_only, int* ngens) {
  //Code to generate for non-static field ns::A::x of type T
  // T& and const T& replaced by T if T is a POD.
  //
  //tA.method("x", [](const ns::A&  a) -> const T& { return a.x; });
  //tA.method("x", [](ns::A&  a) -> T& { return a.x; }); //for a non-POD only
  //tA.method("x!", [](ns::A&  a, int val) -> T&  { return a.x = val; });
  // and their equivalents for a passed as a pointer instead of a reference
  //
  //For a static field or a global variable (ns::A::x becomes then ns::x)
  //
  //types.method("ns!A!x", []() -> T& { return ns::A::x; });
  //types.method("ns!A!x!", [](int val) -> T& { return ns::A::x = val; });

  auto kind = clang_getCursorKind(cursor);

  //target: variable or field accessors must be generated for
  auto target_name = str(clang_getCursorSpelling(cursor));
  std::string target_name_jl;
  auto target_type = clang_getCursorType(cursor);
  auto target_type_name = str(clang_getTypeSpelling(target_type));
  auto target_type_jl = jl_type_name(target_type_name);
  auto pod_target =  clang_isPODType(target_type);
  auto target_pointee = clang_getPointeeType(target_type);
  auto pointer_target =  target_pointee.kind != CXType_Invalid;

  bool is_const;
  if(pointer_target) is_const = clang_isConstQualifiedType(target_pointee);
  else is_const = clang_isConstQualifiedType(target_type);

  if(clang_getArraySize(target_type) >=0){
    //a c-array
    if(verbose > 0){
      std::cerr << "Warning: no accessor created for the c-array type variable "
                << class_prefix << target_name << "\n";
    }
    return o;
  }

  auto const_type = [](CXType type){
    if(clang_isPODType(type)){
      bool not_a_pointer = clang_getPointeeType(type).kind == CXType_Invalid;
      std::string r(fully_qualified_name(type));
      if(not_a_pointer){
        return remove_cv(r);
      } else{
        return r;
      }
    } else{
      std::regex r("^(const)?\\s*(.*[^&])\\s*&?");
      return std::regex_replace(fully_qualified_name(type), r, "const $2&");
    }
  };

  auto non_const_type_or_pod = [](CXType type){
    if(clang_isPODType(type)){
      bool not_a_pointer = clang_getPointeeType(type).kind == CXType_Invalid;
      std::string r(fully_qualified_name(type));
      if(not_a_pointer) return remove_cv(r);
      else return r;
    } else{
      std::regex r("(.*[^&])\\s*&?");
      return std::regex_replace(fully_qualified_name(type), r, "$1&");
    }
  };

  auto copy_return_type = [](CXType type){
    if(clang_isPODType(type)){
      bool not_a_pointer = clang_getPointeeType(type).kind == CXType_Invalid;
      std::string r(fully_qualified_name(type));
      if(not_a_pointer){
        return remove_cv(r);
      } else{
        return r;
      }
    } else{
      std::regex r("^(const)?\\s*(.*[^&])\\s*&?");
      return std::regex_replace(fully_qualified_name(type), r, "$2");
    }
  };

  bool return_by_copy = false;
  
  auto gen_setters = !is_const && !getter_only;

  bool wrapper_generated = false;

  if(kind == CXCursor_FieldDecl){

    target_name_jl = jl_type_name(target_name);

    const char* ref_types[] = { "&", "*" };
    const char* ops[] = { ".", "->"};

    bool non_const_getter = !is_const;

    indent(o << "\n", nindents) << "DEBUG_MSG(\"Adding " << target_name_jl << " methods "
             << " to provide read access to the field " << target_name
             << " (\" __HERE__ \")\");\n";
    indent(o, nindents) << "// defined in " << clang_getCursorLocation(cursor) << "\n";
    indent(o, nindents) << "// signature to use in the veto list: " << fully_qualified_name(cursor) << "\n";


    for(unsigned iref = 0; iref < 2; ++iref){
      auto ref = ref_types[iref];
      auto op = ops[iref];

      //tA.method("x", [](const ns::A&  a) -> const T& { return a.x; });
      // or
      //tA.method("x", [](const ns::A&  a) -> T { return a.x; });
      indent(o, nindents) << varname << ".method(\"" << target_name_jl << "\", []("
                          << "const " << classname << ref << " a) -> "
                          << (return_by_copy ?
                              copy_return_type(target_type) :
                              const_type(target_type))
                          << " { return a" << op << target_name << "; });\n";
      
      if(non_const_getter){
        if(verbose > 0){
          std::cout << "Info: Generating non-const getter for " << classname << "::" << target_name << "\n";
        }
        //tA.method("x", [](ns::A&  a) -> T& { return a.x; });
        // or
        //tA.method("x", [](ns::A&  a) -> T { return a.x; });
        indent(o, nindents) << varname << ".method(\"" << target_name_jl << "\", []("
                            <<  classname << ref << " a) -> "
                            << (return_by_copy ?
                                copy_return_type(target_type)
                                : non_const_type_or_pod(target_type))
                            << " { return a" << op << target_name << "; });\n";
      }
    }


    if(gen_setters){
      indent(o, nindents)
        << "// defined in "     << clang_getCursorLocation(cursor) << "\n";
      indent(o, nindents) << "// signature to use in the veto list: " << fully_qualified_name(cursor) << "\n";
      indent(o, nindents)
        << "// with ! suffix to veto the setter only.";

      for(unsigned iref = 0; iref < 2; ++iref){
        auto ref = ref_types[iref];
        auto op = ops[iref];
        indent(o << "\n", nindents) << "DEBUG_MSG(\"Adding " << target_name_jl << "! methods "
                 << "to provide write access to the field " << target_name
                 << " (\" __HERE__ \")\");\n";
        //tA.method("x!", [](ns::A&  a, int val) -> T&  { return a.x = val; });
        indent(o, nindents) << varname << ".method(\"" << target_name_jl << "!\", []("
                            << classname << ref << " a, " << const_type(target_type) << " val) -> "
                            << non_const_type_or_pod(target_type)
                            << " { return a" << op << target_name << " = val; });\n";
      }
    }
    wrapper_generated = true;
  } else if(kind == CXCursor_VarDecl){
    auto fqn = fully_qualified_name(cursor);
    target_name_jl = jl_type_name(fqn);

    indent(o << "\n", nindents) << "DEBUG_MSG(\"Adding " << target_name_jl << " methods "
             << "to provide access to the global variable " << fqn
             << " (\" __HERE__ \")\");\n";

    indent(o, nindents) << "// defined in "     << clang_getCursorLocation(cursor) << "\n";

    auto rtype = gen_setters ? non_const_type_or_pod(target_type) : const_type(target_type);
    //types.method("ns!A!x", []() -> T& { return ns::A::x; });
    indent(o, nindents) << varname << ".method(\"" << target_name_jl << "\", []()"
                        << "-> " << rtype
                        << " { return " << fqn << "; });\n";

    if(gen_setters){
      //types.method("ns!A!x!", [](int val) -> T& { return ns::A::x = val; });
      indent(o, nindents) << varname << ".method(\"" << target_name_jl << "!\", []("
                          << const_type(target_type) << " val)"
                          << "-> " << rtype
                          << " { return " << fqn << " = val; });\n";
    }
    wrapper_generated = true;
  } else{
    std::cerr << "Bug found at " << __FILE__ << ":" << __LINE__ << "\n";
  }

  if(wrapper_generated){
    if(ngens) *ngens += 1;
    generated_jl_functions_.insert(target_name_jl);
  }

  if(wrapper_generated && gen_setters){
    if(ngens) *ngens += 1;
    generated_jl_functions_.insert(target_name_jl + "!");
  }

  return o;
}

std::ostream&
FunctionWrapper::gen_setindex(std::ostream& o) const{
  indent(o << "\n", nindents) << "DEBUG_MSG(\"Adding setindex! method "
           << " to wrap " << signature()
           << " (\" __HERE__ \")\");\n"
           << "// defined in "     << clang_getCursorLocation(method.cursor) << "\n";

  indent(o, nindents) <<  varname << ".method(\"setindex!\",\n";
  indent(o, nindents+1) << "[](" << classname
                        << "& a, " << short_arg_list_cxx << " i, "
                        << std::regex_replace(fix_template_type(fully_qualified_name(return_type_)), std::regex("\\s*&\\s*$"), " const &")
                        << " val" << "){\n";
  indent(o, nindents+1) << "return a[i] = val;\n";
  indent(o, nindents) << "});\n";
  return o;
}


//Returns false if the method wrapper had already
//been generated, otherwise generates it and returns true.
std::ostream&
FunctionWrapper::gen_getindex(std::ostream& o,
                              std::vector<std::string>& get_index_register) const{

  std::stringstream buf;
  buf << "getindex(::" << classname << ", ::" << clang_getArgType(method_type, 0) << ")";
  std::string getindex_signature = buf.str();
  if(has(get_index_register, getindex_signature)) return o;

  indent(o << "\n", nindents) << "DEBUG_MSG(\"Adding getindex method to wrap "
           << signature()
           << " (\" __HERE__ \")\");\n";
  indent(o, nindents) << "// defined in "     << clang_getCursorLocation(method.cursor) << "\n";

  indent(o, nindents) <<  varname << ".method(\"getindex\",\n";
  indent(o, nindents + 1) << "[](" << classname
                          << "& a, " << fix_template_type(short_arg_list_cxx) << " i){\n";
  indent(o, nindents + 1) << "return a[i];\n";
  indent(o, nindents) << "});\n";

  get_index_register.push_back(getindex_signature);

  return o;
}

void FunctionWrapper::build_arg_lists(){

  std::stringstream arg_list_buf_1;
  //  std::stringstream arg_list_buf_2;
  std::stringstream arg_list_buf_3;
  std::string sep;

  rvalueref_arg = false;
  for(int i = 0; i < clang_getNumArgTypes(method_type); ++i){
    auto argtype = clang_getArgType(method_type, i);
    auto argtype_fqn = fix_template_type(fully_qualified_name(argtype));

    inaccessible_type = inaccessible_type || !isAccessible(argtype);
    if(argtype.kind == CXType_RValueReference) rvalueref_arg = true;
    arg_list_buf_1 << sep <<  argtype_fqn;
    arg_list_buf_3 << sep <<  "::" << jl_type_name(argtype_fqn);
    sep = ", ";
  }
  short_arg_list_cxx = arg_list_buf_1.str();
  short_arg_list_jl = arg_list_buf_3.str();
}


// Generates a wrapper for a c++ global or class member function.
// Uses a static cast to select the proper c++ functions
// if the function is overloaded.
// This method does not generate wrappers to call functions
// with default parameter values. Use gen_func_with_lambdas
// to generate their wrapper.
std::ostream&
FunctionWrapper::gen_func_with_cast(std::ostream& o){
  std::string msg1;
  std::string msg2;

  if(is_static_){
    msg1 = "global function ";
    msg2 = "";
  } else{
    msg1 = "method ";
    msg2 = std::string("of class ") + classname;
  }

  indent(o, nindents) << varname << ".method(";

  if(name_jl_.size() > 0){
    //note: operator() is a special case
    //      and requires to omit the method
    //      name parameter
    o <<  "\"" << name_jl_
      << "\", ";
  }

  o << "static_cast<"
    << fix_template_type(fully_qualified_name(return_type_)) << " (" << (is_static_ ? "" : class_prefix) << "*)"
    << "(" << short_arg_list_cxx << (is_variadic ? ",..." : "" ) << ") " << cv
    << ">(&" << class_prefix << name_cxx << "));\n";

  generated_jl_functions_.insert(name_jl_);

  return o;
}

std::ostream&
FunctionWrapper::gen_arg_list(std::ostream& o, int nargs, std::string sep, bool argtype_only) const{
  for(decltype(nargs) iarg = 0; iarg < nargs; ++iarg){
    o << sep << arg_decl(iarg, argtype_only);
    sep = ", ";
  }
  return o;
}


// Generates wrappers for a c++ global or class member function.
// Uses a lambda function indirection to handle
// overloaded functions and default parameters.
//
// If this->all_lambda is false, only wrappers for call
// with one or more default parameter values are generated.
// The standard call is assumed to be generated with
// gen_func_with_cast
std::ostream&
FunctionWrapper::gen_func_with_lambdas(std::ostream& o){

  int nargsmin =  method.min_args;
  int nargsmax = clang_getNumArgTypes(method_type);
  if(!all_lambda) nargsmax -= 1;

  //  auto const& method_type = clang_getCursorType(method.cursor);

  //For class methods, we need to produce
  //two wrappers, one taking a reference to the class instance
  //and one taking a pointer to it.
  const char* ref_types[] = { "&", "*" };
  const char* accessors[] = { ".", "->"};

  int ntypes = (is_static_ || classname.size() == 0) ? 1 : 2;

  for(int itype = 0; itype < ntypes; ++itype){
    for(int nargs = nargsmin; nargs <= nargsmax; ++nargs){
      indent(o, nindents) <<  varname << ".method(\"" << name_jl_<<  "\", [](";
      std::string sep;
      if(!is_static_ && classname.size() > 0){
        o << classname << cv << ref_types[itype] << " a";
        sep = ", ";
      }
      gen_arg_list(o, nargs, sep);
      o << ")->" << fix_template_type(fully_qualified_name(return_type_ )) << "{ ";
      if(clang_getCursorResultType(method.cursor).kind != CXType_Void){
        o << "return ";
      }

      if(is_static_) o << classname << "::";
      if(!is_static_ && classname.size() > 0) o << "a" << accessors[itype];
      o << name_cxx << "(";
      sep = "";
      for(decltype(nargs) iarg = 0; iarg < nargs; ++iarg){
        o << sep << "arg" << iarg;
        sep = ", ";
      }
      o << "); });\n";
    }
  }
  generated_jl_functions_.insert(name_jl_);
  return o;
}

std::string
FunctionWrapper::arg_decl(int iarg, bool argtypes_only) const{
  const auto& argtype = clang_getArgType(method_type, iarg);
  const auto argtypename = fix_template_type(fully_qualified_name(argtype)); //str(clang_getTypeSpelling(argtype));

  if(argtypes_only) return argtypename;

  //type pattern of function pointer or function pointer array
  //to split parts before and after the *
  std::regex re_func("(.*\\(\\s*\\*\\s*)(\\).*)");

  //pattern of c-array type
  //to seperate dimension specification
  std::regex re_carray("([^[]*)((?:\\[[^\\[\\]]*\\]\\s*))");
  std::cmatch cm;

  std::stringstream buf;
  if(std::regex_match(argtypename.c_str(), cm, re_func)){
    buf << cm[1] << "arg" << iarg << cm[2];
  } else if(std::regex_match(argtypename.c_str(), cm, re_carray)){
    buf << cm[1] << " arg" << iarg << cm[2];
  } else{
    buf << argtypename << " arg" << iarg;
  }

  return buf.str();
}


FunctionWrapper::FunctionWrapper(const MethodRcd& method, const TypeRcd* pTypeRcd,
                                 std::string varname, std::string classname,
                                 int nindents, bool templated):
  method(method),
  varname(varname),
  classname(classname),
  nindents(nindents),
  all_lambda(false),
  pTypeRcd(pTypeRcd),
  rvalueref_arg(false),
  templated_(templated){

  cursor = method.cursor;
  method_type = clang_getCursorType(cursor);
  is_variadic = clang_isFunctionTypeVariadic(method_type);


  //  std::cerr << __FUNCTION__
  //        << "\n"
  //        << "\tmethod.cursor: " <<  method.cursor << "\n"
  //        << "\tpTypeRcd->cursor: " << pTypeRcd->cursor << "\n"
  //        << "\tpTypeRcd->type_name: " << pTypeRcd->type_name << "\n"
  //        << "\tfully_qualified_name(pTypeRcd->cursor): " << fully_qualified_name(pTypeRcd->cursor) << "\n"
  //        << "\tclassname " << classname << "\n";

  if(pTypeRcd && classname.size() == 0){
    this->classname = pTypeRcd->type_name;
  }

  if(this->classname.size() > 0){
    class_prefix = this->classname + "::";
  }

  if(varname.size() == 0){
    this->varname = "t";
  }

  is_static_ = clang_Cursor_getStorageClass(cursor) == CX_SC_Static;
  return_type_ = clang_getResultType(method_type);
  inaccessible_type = !isAccessible(return_type_);

  if(pTypeRcd){
    name_cxx = str(clang_getCursorSpelling(cursor));
  } else{
    //cursor name of global functions are not fully qualified.
    name_cxx = fully_qualified_name(cursor);
  }

  std::string name_jl_suffix = jl_type_name(name_cxx);

  static std::regex opregex("(^|.*::)operator[[:space:]]*(.*)$");
  std::cmatch m;
  override_base_ = false;

  //number of args including the implicit "this"
  //of non-statis method
  int noperands = clang_getNumArgTypes(method_type);
  if((this->classname.size()) != 0 && !is_static_) noperands += 1;
  
  if(std::regex_match(name_cxx.c_str(), m, opregex)){
    name_jl_suffix = m[2];
  }
  
  //FIXME: check that julia = operator is not already mapped to c++ operator=
  //by CxxWrap. In that case we won't need to define an assign function
  if(name_jl_suffix == "="){
    name_jl_suffix = "assign";
  }

  if(name_jl_suffix == "*" && noperands == 1){
    name_jl_suffix = "getindex"; //Deferencing operator, *x -> x[]
    override_base_ = true;
  }
  
  if(name_jl_suffix == "[]"){
    name_jl_suffix = "getindex";
    override_base_ = true;
  }

  if(name_jl_suffix == "+"){
    //Base.+ can take an arbitrary number of arguments
    //including zero, case that corrsponds to the prefix operator
    override_base_ = true;
  }

  //prefix operators
  static std::vector<std::string> prefix_base_ops = { "~", "!", "-", "+" };
  if(noperands == 1
     && std::find(prefix_base_ops.begin(), prefix_base_ops.end(),
                  name_jl_suffix) != prefix_base_ops.end()){
    override_base_ = true;
  }
  
  static std::vector<std::string> infix_base_ops = { "-", "*", "/",
    "%", "[]", "&", "|", "^", ">>", ">>>", "<<", ">", "<", "<=", ">=",
    "==", "!=", "<=>"};
  
  if(noperands == 2
     && std::find(infix_base_ops.begin(), infix_base_ops.end(),
                  name_jl_suffix) != infix_base_ops.end()){
    override_base_ = true;
  }
  
  //TODO map cast operator to convert
  //T A::operator R();
  
  //C++ -> Julia operation name map for operators65;6203;1c
  //When not in the list, operatorOP() is mapped to Base.OP()
  std::vector<std::pair<std::string, std::string>> op_map = {
    {"()", "paren"},
    {"+=", "add!"},
    {"-=", "sub!"},
    {"*=", "mult!"},
    {"/=", "fdiv!"},
    {"%=", "rem!"},
    {"^=", "xor!"},
    {"|=", "or!"},
    {"&=", "and!"},
    {"<<=", "lshit!"},
    {">>=", "rshit!"},
    {"^", "xor"},
    {"->", "arrow"}, 
    {"->*", "arrowstar"},
    {",", "comma"},
    {"<=>", "cmp"},
    {"--", "dec!"},
    {"++", "inc!"},
    {"&&", "logicaland"},//&& and ||, to which short-circuit evalution is applied
    {"||", "logicalor"},//cannot be overloaded in Julia.
  };

  for(const auto& m: op_map){
    if(name_jl_suffix == m.first){
      name_jl_suffix = m.second;
    }
  }

  setindex_ = getindex_ = false;

  if(name_cxx == "operator[]"){
    getindex_ = true;

    if(return_type_.kind == CXType_LValueReference
       && !(clang_isConstQualifiedType(clang_getPointeeType(return_type_)))){
      setindex_ = true;
    }
  }

  if(name_cxx == "operator new" || name_cxx == "operator delete"
     || name_cxx == "operator new[]" || name_cxx == "operator delete[]"
     ) is_static_ = true;


  if(is_static_){
    if(templated_)
      name_jl_ = jl_type_name(pTypeRcd->type_name + "::") + name_jl_suffix;
    else
    name_jl_ = jl_type_name(class_prefix) + name_jl_suffix;
  } else{
    //FIXME: Add prefix. The namespace?
    name_jl_ = name_jl_suffix;
  }

  build_arg_lists();

  if(clang_CXXMethod_isConst(method.cursor)){
    cv = " const";
  }

  is_ctor_ = clang_getCursorKind(method.cursor) == CXCursor_Constructor;

  is_abstract_ = pTypeRcd ? clang_CXXRecord_isAbstract(pTypeRcd->cursor) : false;  
}

bool
FunctionWrapper::validate(){
  if(name_cxx == "operator[]" && clang_getNumArgTypes(method_type) !=1){
    std::cerr << "Warning: " << method_type << " is skipped because of its "
              << "unexpected number of parameters " << clang_getNumArgTypes(method_type)
              << ". One paramter was expected.\n";
    return false;
  }

  if(inaccessible_type){
    std::cerr << "Warning: no wrapper generated for function '"
              << return_type_ << " " << "(" << short_arg_list_cxx << ")" << cv
              << "' because it requires a type that is private or protected.\n";
    return false;
  }

  if(is_variadic){
    //The code generated for varidadic functions does not compile.
    //Until, it is fixed, skipped these functions
    std::cerr << "Warning: no wrapper will be produced for function '"
              << name_cxx << "(" << short_arg_list_cxx << ", ...) " << cv
              << "' because of lack of support for variadic functions.\n";
    return false;
  }

  if(rvalueref_arg){
    //The code generated for a function with an argument passed as a r-value
    //reference does not compile.
    //Until, it is fixed, skipped these functions
    std::cerr << "Warning: no wrapper will be produced for function '"
              << name_cxx << "(" << short_arg_list_cxx << ") " << cv
              << "' because it contains an argument passed by r-value "
              << "which is not supported yet.\n";
    return false;
  }

  return true;
}

std::string FunctionWrapper::signature() const{
  std::stringstream buf;
  std::string genuine_classname_prefix = pTypeRcd ? (pTypeRcd->type_name + "::") : "";
  buf << fully_qualified_name(return_type_) << " " << genuine_classname_prefix << name_cxx
      << "(" <<  short_arg_list_cxx << ")";
  return buf.str();
}

std::ostream&
FunctionWrapper::generate(std::ostream& o,
                          std::vector<std::string>& get_index_register){

  if(!validate()) return o;

  if(setindex_ || getindex_){
    if(setindex_) gen_setindex(o);
    if(getindex_) gen_getindex(o, get_index_register);
    return o;
  }

  if(is_ctor_){
    if(!is_abstract_){
      gen_ctor(o);
    }
    return o;
  }

  indent(o, nindents)
    << "DEBUG_MSG(\"Adding wrapper for "
    << signature()
    << " (\" __HERE__ \")\");""\n";
  indent(o,nindents) << "// signature to use in the veto list: " << signature() << "\n";

  indent(o, nindents) << "// defined in "     << clang_getCursorLocation(method.cursor) << "\n";

  if(!all_lambda) gen_func_with_cast(o);

  gen_func_with_lambdas(o);

  return o;
}

bool FunctionWrapper::isAccessible(CXType type) const{
  CXType type1 = {CXType_Invalid, nullptr};
  while((type.kind == CXType_Pointer || type.kind == CXType_LValueReference)
        && !clang_equalTypes(type, type1)){
    type1 = type;
    type = clang_getPointeeType(type);
  }
  auto access = clang_getCXXAccessSpecifier(clang_getTypeDeclaration(type));
  return !(access == CX_CXXPrivate || access == CX_CXXProtected);
}

//FIXME: this method handle the case the method of a templated class
//use the class as an argument or return type.
//Need to handle more generally cases of dependency on the class template parameters
std::string FunctionWrapper::fix_template_type(std::string type_name) const{

  if(!templated_ || !pTypeRcd) return type_name;

  std::string class_namespace;
  std::string class_genuine_name;
  bool rc = get_namespace_and_type_from_decl(pTypeRcd->cursor,
                                             class_namespace,
                                             class_genuine_name);
  if(!rc){
    std::cerr << "WARNING: failed to get the namespace and name of the class "
      "where the type " << type_name << " is defined. Generated code may be "
      "uncorrect.";
    class_namespace = "";
    class_genuine_name = pTypeRcd->type_name;
  }

  auto param_list = join(pTypeRcd->template_parameters, ", ");
  class_genuine_name = "(?:" + class_namespace + ")?" + class_genuine_name;

  std::regex r(std::string("(?=^|::)(const )?(?:") + class_genuine_name + ")(?=$|::| [*&])");
  std::string fixed = std::regex_replace(type_name, r, "$1typename WrappedType");

  std::regex r2(std::string("(?=^|::)(const )?(?:") + class_genuine_name + "<" + param_list + ">)(?=$| [*&])");
  fixed = std::regex_replace(fixed, r2, "$1WrappedType");

  return fixed;
}
