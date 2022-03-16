#ifndef TYPERCD_H
#define TYPERCD_H

#include <clang-c/Index.h>
#include "MethodRcd.h"

#include <string>
#include <vector>

struct TypeRcd {
  CXCursor cursor;
  std::string type_name;
  std::vector<MethodRcd> methods;
  std::vector<std::string> template_parameters;
  std::vector<std::vector<std::string> > template_parameter_combinations;
  bool to_wrap;
  int id;
  static int nRecords;
  bool explicit_ctor;
  bool vetoed_default_ctor;
  TypeRcd(const CXCursor& cursor, const std::string& type_name = std::string());
  TypeRcd(): cursor(clang_getNullCursor()){}
};

#endif //TYPERCD_H not defined
