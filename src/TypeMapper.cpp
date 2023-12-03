#include "TypeMapper.h"
#include "libclang-ext.h"

bool TypeMapper::is_mapped(CXType t, bool as_return) const {
  return is_mapped(fully_qualified_name(t), as_return);
}

std::string TypeMapper::mapped_typename(CXType from, bool as_return,
                                        bool* ismapped) const{
  return mapped_type(fully_qualified_name(from), as_return, ismapped);
}

