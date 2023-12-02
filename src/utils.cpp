//-*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// vim: noai:ts=2:sw=2:expandtab
//
// Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>
//
#include "utils.h"
#include <regex>
#include <string>
#include <sstream>
#include <fstream>

#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "llvm/Support/raw_ostream.h"


int verbose = 0;
CXPrintingPolicy pp = nullptr;

std::ostream& indent(std::ostream& o, int n){
  for(int i = 0; i < n; ++i){
    o << one_indent;
  }
  return o;
}
std::ostream& operator<<(std::ostream& stream, const CXString& str){
  stream << clang_getCString(str);
  clang_disposeString(str);
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const CXCursor& x){
  stream << clang_getCursorSpelling(x);
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const CXType& x){
  return stream << clang_getTypeSpelling(x);
}

std::ostream& operator<<(std::ostream& stream, const CXTypeKind& x){
  return stream << clang_getTypeKindSpelling(x);
}

std::ostream& operator<<(std::ostream& stream, const CXCursorKind& x){
  return stream << clang_getCursorKindSpelling(x);
}


std::ostream& operator<<(std::ostream& stream, CX_CXXAccessSpecifier x){
  if(x == CX_CXXPublic) return stream << "public";
  if(x == CX_CXXPrivate) return stream << "private";
  if(x == CX_CXXProtected) return stream << "protected";
  return stream << "invalid";
}

std::ostream& operator<<(std::ostream& stream, CXSourceLocation location){
  unsigned line;
  unsigned column;
  CXFile file;
  clang_getSpellingLocation(location, &file, &line, &column, NULL);
  CXString filepath =  clang_getFileName(file);
  return stream << filepath << ":" << line << ":" << column;
}

std::ostream& operator<<(std::ostream& stream, const CXFile& file){
  return stream << clang_getFileName(file);
}

std::string str(const CXString& x){
  std::string s(clang_getCString(x));
  clang_disposeString(x);
  return s;
}

std::string jl_type_name(const std::string& s){
  return std::regex_replace(s, std::regex("::"), "!");
}

std::string jl_type_name(const CXType& t){
  std::string s = str(clang_getTypeSpelling(t));
  return jl_type_name(s);
}

void replace(std::string& s, const std::string& to_replace,
             const std::string& replacement){
  auto pos = s.find(to_replace);
  if(pos!=std::string::npos){
    s.replace(pos, to_replace.size(), replacement.c_str());
  }
}

std::string join_paths(const std::string& p1, const std::string& p2){
  std::string p = p1.size() > 0 ? p1 : ".";
  return p + "/" + p2;
}
