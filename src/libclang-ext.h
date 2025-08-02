//-*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// vim: noai:ts=2:sw=2:expandtab
//
// Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>
//

#ifndef LIBCLANG_EXT_H
#define LIBCLANG_EXT_H

#include <clang-c/Index.h>
#include "clang/AST/Type.h"
#include <string>

std::string fully_qualified_name(CXCursor c);

std::string fully_qualified_name(CXType type);

int hasDefaultConstructor(CXCursor cursor);

CXType remove_non_builtin_qualifiers(CXType& type);

CXType base_type_(CXType type);

CXType base_type(CXType type);

std::string remove_cv(const std::string& type_name);

const clang::TemplateArgument & get_IntegralTemplateArgument(CXCursor cursor, int i);

std::string get_resource_dir();

bool get_namespace_and_type_from_decl(CXCursor cursor,
                                      std::string& ns,
                                      std::string& clazz);

bool same_type(CXType t1, CXType t2);

bool is_method_deleted(CXTranslationUnit unit, CXCursor cursor);

#endif //LIBCLANG_EXT_H not defined
