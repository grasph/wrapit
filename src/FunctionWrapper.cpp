// Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>
#include "FunctionWrapper.h"
#include "utils.h"
#include "libclang-ext.h"
#include <sstream>
#include <regex>

std::ostream&
FunctionWrapper::gen_ctor(std::ostream& o){
  int nargsmin =  method.min_args;
  int nargsmax = clang_getNumArgTypes(method_type);

  indent(o << "\n", nindents)
    << "DEBUG_MSG(\"Adding wrapper for "
    << signature()
    << " (\" __HERE__ \")\");""\n";
  indent(o, nindents) << "// defined in "     << clang_getCursorLocation(method.cursor) << "\n";

  for(int nargs = nargsmin; nargs <= nargsmax; ++nargs){
    indent(o, nindents) << varname << "." << (templated_ ? "template " : "")
			<< "constructor<";
    gen_arg_list(o, nargs, "", /*argtypes_only = */ true) << ">();\n";
  }
  generated_jl_functions_.insert(name_jl_);
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
  auto target_name_jl = jl_type_name(target_name);
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
      std::string r(str(clang_getTypeSpelling(type)));
      if(not_a_pointer){
	return remove_cv(r);
      } else{
	return r;
      }
    } else{
      std::regex r("^(const)?\\s*(.*[^&])\\s*&?");
      return std::regex_replace(str(clang_getTypeSpelling(type)), r, "const $2&");
    }
  };

  auto non_const_type_or_pod = [](CXType type){
    if(clang_isPODType(type)){
      bool not_a_pointer = clang_getPointeeType(type).kind == CXType_Invalid;
      std::string r(str(clang_getTypeSpelling(type)));
      if(not_a_pointer) return remove_cv(r);
      else return r;
    } else{
      std::regex r("(.*[^&])\\s*&?");
      return std::regex_replace(str(clang_getTypeSpelling(type)), r, "$1&");
      }
  };

  auto gen_setters = !is_const && !getter_only;

  bool wrapper_generated = false;
    
  if(kind == CXCursor_FieldDecl){

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
      indent(o, nindents) << varname << ".method(\"" << target_name_jl << "\", []("
			  << "const " << classname << ref << " a) -> " << const_type(target_type)
			  << " { return a" << op << target_name << "; });\n";

      if(non_const_getter){
	if(verbose > 0){
	  std::cout << "Info: Generating non-const getter for " << classname << "::" << target_name << "\n";
	}
	indent(o, nindents) << varname << ".method(\"" << target_name_jl << "\", []("
			    <<  classname << ref << " a) -> " << non_const_type_or_pod(target_type)
			    << " { return a" << op << target_name << "; });\n";
      }
    }


    if(gen_setters){
      indent(o, nindents)
	<< "// defined in "     << clang_getCursorLocation(cursor) << "\n";
      indent(o, nindents) << "// signature to use in the veto list: " << fully_qualified_name(cursor) << "\n";
      indent(o, nindents)
	<< "// with ! suffix to veto the setter only\n";

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
    auto fqn_jl = jl_type_name(fqn);

    indent(o << "\n", nindents) << "DEBUG_MSG(\"Adding " << fqn_jl << " methods "
	     << "to provide access to the global variable " << fqn
	     << " (\" __HERE__ \")\");\n";
    indent(o, nindents) << "// defined in "     << clang_getCursorLocation(cursor) << "\n";

    auto rtype = gen_setters ? non_const_type_or_pod(target_type) : const_type(target_type);
    //types.method("ns!A!x", []() -> T& { return ns::A::x; });
    indent(o, nindents) << "types.method(\"" << fqn_jl << "\", []()"
		 << "-> " << rtype
		 << " { return " << fqn << "; });\n";

    if(gen_setters){
      //types.method("ns!A!x!", [](int val) -> T& { return ns::A::x = val; });
      indent(o, nindents) << "types.method(\"" << fqn_jl << "!\", []("
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
			<< "& a, " << fix_template_type(short_arg_list_cxx) << " i, "
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
    auto argtype_fqn = fully_qualified_name(argtype);

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
    << "(" << fix_template_type(short_arg_list_cxx) << (is_variadic ? ",..." : "" ) << ") " << cv
    << ">(&" << class_prefix << name_cxx << "));\n";

  generated_jl_functions_.insert(name_jl_);

  return o;
}

std::ostream&
FunctionWrapper::gen_arg_list(std::ostream& o, int nargs, std::string sep, bool argtype_only) const{
  for(decltype(nargs) iarg = 0; iarg < nargs; ++iarg){
    o << sep << fix_template_type(arg_decl(iarg, argtype_only));
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
  const auto argtypename = fully_qualified_name(argtype); //str(clang_getTypeSpelling(argtype));

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
//	    << "\n"
//	    << "\tmethod.cursor: " <<  method.cursor << "\n"
//	    << "\tpTypeRcd->cursor: " << pTypeRcd->cursor << "\n"
//	    << "\tpTypeRcd->type_name: " << pTypeRcd->type_name << "\n"
//	    << "\tfully_qualified_name(pTypeRcd->cursor): " << fully_qualified_name(pTypeRcd->cursor) << "\n"
//	    << "\tclassname " << classname << "\n";

  if(pTypeRcd && classname.size() == 0){
    this->classname = pTypeRcd->type_name;
  }

  if(this->classname.size() > 0){
    class_prefix = this->classname + "::";
  }

  if(varname.size() == 0){
    if(pTypeRcd!=0){
      std::stringstream buf;
      buf << "t" << pTypeRcd->id;
      this->varname = buf.str();
    } else{
      this->varname = "types";
    }
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

  static std::regex opregex("(^|.*::)operator(.{1,2})$");
  std::cmatch m;
  override_base_ = false;
  if(std::regex_match(name_cxx.c_str(), m, opregex)){
    name_jl_suffix = m[2];
    override_base_ = true;
  }

  //FIXME: check that julia = operator is not already mapped to c++ operator=
  //by CxxWrap. In that case we won't need to define an assign function
  if(name_jl_suffix == "="){
    name_jl_suffix = "assign";
    override_base_ = false;
  }

  std::vector<std::pair<std::string, std::string>> op_map = {
    {"()", "paren"},
    {"+=", "add!"},
    {"-=", "sub!"},
    {"*=", "mult!"},
    {"/=", "fdiv!"},
    {"%=", "mod!"},
    {">>=", "rshift!"},
    {"<<=", "lshift!"},
    {"^=", "bwxor!"},
    {"|=", "bwor!"},
    {"&=", "bwand!"},
  };

  for(const auto& m: op_map){
    if(name_jl_suffix == m.first){
      name_jl_suffix = m.second;
      override_base_ = false;
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
  buf << fully_qualified_name(return_type_) << " " << class_prefix << name_cxx
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

  std::string genuine_classname = pTypeRcd->type_name;

  std::regex r(std::string("(?=^|::)") + genuine_classname + "(?=$|::)");

  std::string fixed = std::regex_replace(type_name, r, "WrappedType");
  if(fixed != type_name){
    fixed = "typename " + fixed;
  }

  return fixed;
}
