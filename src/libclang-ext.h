#ifndef LIBCLANG_EXT_H
#define LIBCLANG_EXT_H

#include <clang-c/Index.h>
#include <string>

std::string fully_qualified_name(CXCursor c);

std::string fully_qualified_name(CXType type);

int hasDefaultConstructor(CXCursor cursor);

CXType remove_non_builtin_qualifiers(CXType& type);

CXType base_type(CXType type);

std::string remove_cv(const std::string& type_name);

#endif //LIBCLANG_EXT_H not defined
