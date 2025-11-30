#ifndef TYPERCD_H
#define TYPERCD_H

#include <clang-c/Index.h>
#include "MethodRcd.h"

#include <string>
#include <vector>

class TypeMapper;

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
  bool stl;
  bool stl_const;
  bool stl_ptr;
  bool stl_const_ptr;
  int id;
  static int nRecords;
  bool default_ctor;
  bool copy_op_deleted;
  bool finalize;
  TypeRcd(const CXCursor& cursor, const std::string& type_name = std::string());
  TypeRcd(): cursor(clang_getNullCursor()){}

  std::string name(const std::vector<std::string>& params) const;
  std::string name(int combi) const;
  std::vector<std::string> names() const;
  std::ostream& specialization_list(std::ostream& o) const;
  
  void setStrictNumberTypeFlags(const TypeMapper& typeMap);

};

#endif //TYPERCD_H not defined
