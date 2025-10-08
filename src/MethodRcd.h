//-*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// vim: noai:ts=2:sw=2:expandtab
//
// Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>
//
#ifndef METHODRCD_H
#define METHODRCD_H

#include <clang-c/Index.h>
#include <vector>

struct MethodRcd{
  CXCursor cursor;
  int min_args;
  std::vector<bool> strict_number_type;
  MethodRcd(CXCursor cursor, int min_args = -1): cursor(cursor), min_args(min_args){}
};


#endif //METHODRCD_H not defined
