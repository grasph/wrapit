//-*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// vim: noai:ts=2:sw=2:expandtab
//
// Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>
//
#ifndef FUNCTIONWRAPPER_H
#define FUNCTIONWRAPPER_H

#include <clang-c/Index.h>
#include "MethodRcd.h"
#include "TypeRcd.h"
#include <iostream>
#include <set>
#include <map>

class TypeMapper;

// Helper class to generate CxxWrap.jl wrapper
// code for a global or class function.
//
// The C++ AST is explored by the constructor
// that prepare the required information for
// the code generation.
//
// Call generate() to generate the code.
//
class FunctionWrapper{
public:
  FunctionWrapper(const std::map<std::string, std::string>& name_map,
                  const MethodRcd& method,
                  const TypeRcd* pTypeRcd,
                  const TypeMapper& type_mapper,
                  long cxxwrap_version,
                  std::string varname = std::string(),
                  std::string classname = std::string(),
                  int nindents = 0,
                  bool templated = false);


  /// Returns the method signature.
  /// Used by CodeTree to support method
  /// vetoing.
  std::string signature() const;

  /// Generates the c++ code for the wrapper.
  /// @param [in,out] get_index_register register used to keep tracks
  //                  of all generated getindex method and prevent
  //                  duplicates. The same register must be used for all
  //                  the methods of the same class and can be shared
  //                  by all classes but it is not necessary.
  std::ostream&
  generate(std::ostream& o,
           std::vector<std::string>& get_index_register);

  /// Generate the default class contructor wrapper if the
  /// class requires it.
  static std::ostream&
  gen_ctor(std::ostream& o, int nindents,
           const std::string& varname, bool templated,
           bool finalize,
           const std::string& arg_list,
           const std::string& argname_list,
           long cxxwrap_version);

  /// Tells if the Base module must be overriden.
  /// This is the case for operators.
  bool override_base(){ return override_base_;}

  bool is_global() { return is_static_ || classname.size() == 0;}

  bool is_static() { return is_static_; }

  std::string name_jl() const { return name_jl_; }

  std::ostream&
  gen_accessors(std::ostream& o, bool getter_only = false, int* ngens = nullptr);

  std::set<std::string> generated_jl_functions(){ return generated_jl_functions_; }

  std::string fix_template_type(std::string type_name) const;

  bool defines_getindex() const { return getindex_; }

  bool defines_setindex() const { return setindex_; }

  bool is_ctor() const { return is_ctor_;}

protected:

  std::ostream& gen_arg_list(std::ostream& o, int nargs, std::string sep, bool argtypes_only = false) const;

  std::ostream& gen_argname_list(std::ostream& o, int nargs, std::string sep) const;
  
  std::ostream&
  gen_func_with_default_values(std::ostream& o);


  std::ostream&
  gen_ctor(std::ostream& o);

  std::ostream&
  gen_setindex(std::ostream& o) const;

  //Returns false if the method wrapper had already
  //been generated, otherwise generates it and returns true.
  std::ostream&
  gen_getindex(std::ostream& o,
               std::vector<std::string>& get_index_register) const;

  void build_arg_lists(bool& ismapped);


  // Generates a wrapper for a c++ global or class member function.
  // Uses a static cast to select the proper c++ functions
  // if the function is overloaded.
  // Do not generates wrapper to call the function with
  // default parameter values. use gen_func_with_lambdas
  // for this.
  std::ostream&
  gen_func_with_cast(std::ostream& o);


  // Generates wrappers for a c++ global or class member function.
  // Uses a lambda function indirection to handle
  // overloaded functions and default parameters.
  //
  // If this->all_lambda_ is false, only wrappers for call
  // with one or more default parameter values are generated.
  // The standard call is assumed to be generated with
  // gen_func_with_cast
  std::ostream&
  gen_func_with_lambdas(std::ostream& o);

  bool
  validate();

  bool isAccessible(CXType type) const;

  std::string arg_decl(int iarg, bool argtype_only) const;

  std::string get_name_jl_suffix(const std::string& cxx_name,
                                 int noperands) const;
  

private:
  long cxxwrap_version_;
  MethodRcd method;
  std::string varname_;
  std::string classname;
  std::string class_prefix;
  int nindents;

  bool all_lambda_;


  CXCursor cursor;
  CXType  method_type;
  //  std::map<std::string, std::string> name_map_;
  CXType  return_type_;
  bool is_variadic;
  const TypeRcd* pTypeRcd;

  std::string name_cxx;
  std::string name_jl_;
  std::set<std::string> generated_jl_functions_;

  bool is_static_;
  bool inaccessible_type;
  bool rvalueref_arg;
  bool allArgTypesAccessible;

  bool setindex_;
  bool getindex_;

  bool is_ctor_;
  bool is_abstract_;
  bool override_base_;

  std::string cv;

  std::string short_arg_list_signature;
  std::string short_arg_list_cxx;
  std::string long_arg_list_cxx;
  std::string short_arg_list_jl;

  bool templated_;

  const TypeMapper& type_map_;
};

#endif //FUNCTIONWRAPPER_H not defined
