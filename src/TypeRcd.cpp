//-*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// vim: noai:ts=2:sw=2:expandtab
//
// Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>
//
#include "TypeRcd.h"
#include "utils.h"
#include "assert.h"
#include <set>
#include "libclang-ext.h"
#include "FunctionWrapper.h"
#include "TypeMapper.h"

int TypeRcd::nRecords = 0;

TypeRcd::TypeRcd(const CXCursor& cursor, const std::string& type_name)
  : cursor(cursor), to_wrap(false), stl(false), stl_const(false),
    stl_ptr(false), stl_const_ptr(false), id(nRecords++), default_ctor(true),
    copy_op_deleted(false), finalize(true){
  if(type_name.size() == 0){
    this->type_name = fully_qualified_name(cursor);
  }  else{
    this->type_name = type_name;
  }
  assert(this->type_name.size() > 0);
}

std::string
TypeRcd::name(int icombi) const{
  std::stringstream buf;
  buf << type_name;
  if(icombi >= 0){
    buf << "<";
    const char* sep = "";
    for(const auto& ptype: template_parameter_combinations[icombi]){
      buf << sep << ptype;
      sep = ", ";
    }
    buf << ">";
  }
  return buf.str();
}

std::vector<std::string> TypeRcd::names() const{
  if(template_parameters.size() == 0){
    return std::vector<std::string>(1, type_name);
  } else{
    std::vector<std::string> r;
    for(unsigned i = 0; i < template_parameter_combinations.size(); ++i){
      r.emplace_back(name(i));
    }
    return r;
  }
}

std::ostream& TypeRcd::specialization_list(std::ostream& o) const{
  const char* sep1 = "";
  for(int i = template_parameter_combinations.size() - 1; i >= 0; --i){
    const auto& combi = template_parameter_combinations[i];
    //TemplateType<
    o << sep1 << type_name << "<";
    sep1 = ", ";

    //P1, P2
    const char* sep2 ="";
    for(const auto& arg_typename: combi){
      o << sep2 << arg_typename;
      sep2 = ", ";
    }
    o << ">";
  }
  return o;
}

//FIXME: extend the algorithm to handle arguments with default values
//FIXME: type comparison must be done after type map is applied
void TypeRcd::setStrictNumberTypeFlags(const TypeMapper& typeMapper){
  auto nmethods = methods.size();
  for(decltype(nmethods) im1 = 0; im1 < nmethods; ++im1){
    MethodRcd& m1 = methods[im1];

    //StrictlyTypedNumber<> does not work for constructors,
    //skip them.
    if(clang_getCursorKind(m1.cursor) == CXCursor_Constructor){
      continue;
    }
    
    auto m1type = clang_getCursorType(m1.cursor);
    std::set<int> strict_type_args;
    for(decltype(nmethods) im2 = im1 + 1; im2 < nmethods; ++im2){
      MethodRcd& m2 = methods[im2];

      std::set<int> relevant_args;

      //compare function names:
      if(str(clang_getCursorSpelling(m1.cursor))
         != str(clang_getCursorSpelling(m2.cursor))) continue;

      //compare number of arguments
      auto m2type = clang_getCursorType(m2.cursor);
      if(clang_getNumArgTypes(m1type)
         != clang_getNumArgTypes(m2type)) continue;

      //if this point is reached, the two methods have same name and same number
      //of arguments.

      //compare argument types
      bool disentangled = false;
      for(int iarg = 0; iarg < clang_getNumArgTypes(m1type); ++iarg){
        const auto& t1 = clang_getCanonicalType(clang_getCursorType(clang_getTypeDeclaration(clang_getArgType(m1type, iarg))));
        const auto& t2 = clang_getCanonicalType(clang_getCursorType(clang_getTypeDeclaration(clang_getArgType(m2type, iarg))));

        auto isnumtype = [](CXType t){
          return 3 <= t.kind && t.kind <= 23;
        };

        //if(!same_type(t1, t2)){
        if(typeMapper.mapped_typename(t1) != typeMapper.mapped_typename(t2)){
          if(isnumtype(t1) && isnumtype(t2)){
            relevant_args.insert(iarg);
          } else{
            disentangled = true;
          }
          continue;
        }
      } //next iarg
      if(!disentangled){
        for(const auto& iarg: relevant_args){
          strict_type_args.insert(iarg);
        }
      }
    } //next m2
    
    m1.strict_number_type = std::vector(clang_getNumArgTypes(m1type), false);

    bool atleastone=false;
    for(const int iarg: strict_type_args){
      m1.strict_number_type[iarg] = true;
      atleastone = true;
    }

    if(atleastone && verbose > 1){
      std::cerr << "Info: " << "i-th argument(s) of "
                << FunctionWrapper::signature(cursor, m1.cursor, false, false,
                                              typeMapper)
                << ", with i = ";
      for(auto i: strict_type_args){
        std::cerr << i << ", ";
      }
      std::cerr << "is marked as of strict number type because of the presence "
        "the method is also implemented for a different number type.\n";
    }
  }//next m1
}
