//-*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// vim: noai:ts=2:sw=2:expandtab
//
// Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>
//
#include "TypeRcd.h"
#include "utils.h"
#include "assert.h"

int TypeRcd::nRecords = 0;

TypeRcd::TypeRcd(const CXCursor& cursor, const std::string& type_name)
  : cursor(cursor), to_wrap(false), stl(false), stl_const(false),
    stl_ptr(false), stl_const_ptr(false), id(nRecords++), default_ctor(true),
    finalize(true){
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
