// Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>
#include "TypeRcd.h"
#include "utils.h"
#include "assert.h"

int TypeRcd::nRecords = 0;

TypeRcd::TypeRcd(const CXCursor& cursor, const std::string& type_name)
  : cursor(cursor), to_wrap(false), id(nRecords++),
    explicit_ctor(false), vetoed_default_ctor(false){
  if(type_name.size() == 0){
    this->type_name = fully_qualified_name(cursor);
  }  else{
    this->type_name = type_name;
  }
  assert(this->type_name.size() > 0);
}
