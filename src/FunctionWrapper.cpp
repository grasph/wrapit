// Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch> 
#include "FunctionWrapper.h"
#include "utils.h"
#include <sstream>
#include <regex>

std::ostream&
FunctionWrapper::gen_ctor(std::ostream& o) const{

  indent(o << "\n", nindents)
    << "DEBUG_MSG(\"Adding wrapper for "
    << signature()
    << " (\" __HERE__ \")\");""\n";
  indent(o, nindents) << "// defined in "     << clang_getCursorLocation(method.cursor) << "\n";

  indent(o, nindents) << varname << "." << (templated_ ? "template " : "")
		      <<    "constructor<"
		      << short_arg_list_cxx << ">();";
  return o;
}
  
std::ostream&
FunctionWrapper::gen_setindex(std::ostream& o) const{
  indent(o << "\n", nindents) << "DEBUG_MSG(\"Adding setindex! method "
	   << " to wrap " << signature()
	   << " (\" __HERE__ \")\");\n"
	   << "// defined in "     << clang_getCursorLocation(method.cursor) << "\n";

  indent(o, 1) <<  varname << ".method(\"setindex!\",\n";
  indent(o, 2) << "[](" << classname
	       << "& a, " << short_arg_list_cxx << " i, "
	       << std::regex_replace(fully_qualified_name(return_type_), std::regex("\\s*&\\s*$"), " const &")
	       << " val" << "){\n";
  indent(o, 2) << "return a[i] = val;\n";
  indent(o, 1) << "});\n";
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
			  << "& a, " << short_arg_list_cxx << " i){\n";
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
    //void (*) -> void(*argN)
    //    std::regex e("(.*\\(\\*)(\\).*)");
    //    std::cmatch cm;
    //    arg_list_buf_2 << sep <<  arg_decl(i);
    arg_list_buf_3 << sep <<  "::" << jl_type_name(argtype_fqn);
    sep = ", ";
  }
  short_arg_list_cxx = arg_list_buf_1.str();
  short_arg_list_jl = arg_list_buf_3.str();
}


// Generates a wrapper for a c++ global or class member function.
// Uses a static cast to select the proper c++ functions
// if the function is overloaded.
// Do not generates wrapper to call the function with
// default parameter values. use gen_func_with_lambdas
// for this.
std::ostream&
FunctionWrapper::gen_func_with_cast(std::ostream& o){
  std::string msg1;
  std::string msg2;

  if(is_static){
    msg1 = "global function ";
    msg2 = "";
  } else{
    msg1 = "method ";
    msg2 = std::string("of class ") + classname;
  }
  
  indent(o << "\n", nindents)
    << "DEBUG_MSG(\"Adding wrapper for "
    << signature()
    << " (\" __HERE__ \")\");\n";
  indent(o, nindents) << "// defined in "     << clang_getCursorLocation(method.cursor) << "\n";
 

  
  indent(o, nindents) << varname << ".method(";
  
  if(name_jl_.size() > 0){
    //note: operator() is a special case
    //      and requires to omit the method
    //      name parameter
    o <<  "\"" << name_jl_
      << "\", ";
  }
  
  o << "static_cast<"
    << fully_qualified_name(return_type_) << " (" << (is_static ? "" : class_prefix) << "*)"
    << "(" << short_arg_list_cxx << (is_variadic ? ",..." : "" ) << ") " << cv
    << ">(&" << class_prefix << name_cxx << "));\n";

  wrapper_generated_ = true;
  
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

  int ntypes = (is_static || classname.size() == 0) ? 1 : 2;

  for(int itype = 0; itype < ntypes; ++itype){
    for(int nargs = nargsmin; nargs <= nargsmax; ++nargs){
      indent(o, nindents) <<  varname << ".method(\"" << name_jl_<<  "\", [](";
      std::string sep;
      if(!is_static && classname.size() > 0){
	o << classname << cv << ref_types[itype] << " a";
      sep = ", ";
      }
      for(decltype(nargs) iarg = 0; iarg < nargs; ++iarg){
	//      const auto& argtype = clang_getArgType(method_type, iarg);
	//o << sep << argtype << " arg" << iarg;
	o << sep << arg_decl(iarg);
	sep = ", ";
      }
      o << ")->" << fully_qualified_name(return_type_ ) << "{ ";
      if(clang_getCursorResultType(method.cursor).kind != CXType_Void){
	o << "return ";
      }
      
      if(is_static) o << classname << "::";
      if(!is_static && classname.size() > 0) o << "a" << accessors[itype];
      o << name_cxx << "(";
      sep = "";
      for(decltype(nargs) iarg = 0; iarg < nargs; ++iarg){
	o << sep << "arg" << iarg;
	sep = ", ";
      }
      o << "); });\n";
    }
  }

  wrapper_generated_ = true;
  return o;
}

std::string
FunctionWrapper::arg_decl(int iarg) const{
  const auto& argtype = clang_getArgType(method_type, iarg);
  const auto argtypename = fully_qualified_name(argtype); //str(clang_getTypeSpelling(argtype));

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
  all_lambda(true),
  rvalueref_arg(false),
  templated_(templated),
  wrapper_generated_(false){
  cursor = method.cursor;
  method_type = clang_getCursorType(cursor);
  is_variadic = clang_isFunctionTypeVariadic(method_type);

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
  
  is_static = clang_Cursor_getStorageClass(cursor) == CX_SC_Static;
  return_type_ = clang_getResultType(method_type);
  inaccessible_type = !isAccessible(return_type_);

  if(pTypeRcd){
    name_cxx = str(clang_getCursorSpelling(cursor));
  } else{
    //cursor name of global functions are not fully qualified.
    name_cxx = fully_qualified_name(cursor);
  }

  std::string name_jl_suffix = jl_type_name(name_cxx);    

  static std::regex opregex("^operator(.{1,2})$");
  std::cmatch m;
  override_base_ = false;
  if(std::regex_match(name_cxx.c_str(), m, opregex)){
    name_jl_suffix = m[1];
    override_base_ = true;
  }

  //FIXME: check that julia = operator is not already mapped to c++ operator=
  //by CxxWrep. In that case we won't need to define an assign function
  if(name_jl_suffix == "="){
    name_jl_suffix = "assign";
    override_base_ = false;
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
     ) is_static = true;
  

  if(is_static){
    name_jl_ = jl_type_name(class_prefix) + name_jl_suffix;
  } else{
    //FIXME: Add prefix. The namespace?
    name_jl_ = name_jl_suffix;
  }

  build_arg_lists();

  if(clang_CXXMethod_isConst(method.cursor)){
    cv = " const";
  }

  is_ctor = clang_getCursorKind(method.cursor) == CXCursor_Constructor;

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

  if(is_ctor){
    if(!is_abstract_){
      gen_ctor(o);
    }
    return o;
  }

  
  indent(o,1) << "// " << signature() << "\n";
  indent(o << "\n", nindents)
    << "DEBUG_MSG(\"Adding wrapper for "
    << signature()
    << " (\" __HERE__ \")\");""\n";
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

