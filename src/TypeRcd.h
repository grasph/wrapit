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
  std::vector<CXCursor> fields;
  std::vector<CXCursor> typedefs;
  std::vector<std::string> template_parameters;
  std::vector<std::string> template_parameter_types;
  std::vector<std::vector<std::string> > template_parameter_combinations;
  bool to_wrap;
  int id;
  static int nRecords;
  bool default_ctor;
  bool finalize;
  TypeRcd(const CXCursor& cursor, const std::string& type_name = std::string());
  TypeRcd(): cursor(clang_getNullCursor()){}

  std::string name(int combi) const;
  std::vector<std::string> names() const;
};

#endif //TYPERCD_H not defined
