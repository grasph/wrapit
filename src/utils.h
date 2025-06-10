//-*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// vim: noai:ts=2:sw=2:expandtab
//
// Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>
//
#ifndef UTILS_H
#define UTILS_H

#include <clang-c/Index.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static const char* one_indent = "  ";
extern int verbose;
extern long version_int_base;


std::string nth(int i);

std::ostream& indent(std::ostream& o, int n);

//Converts version string x.y.z (optionnally prefixed by the letter v)
//to an int which is larger for newer version
//and is defined as 10^6*x + 10^3*y + z
//return -1 if the text format was not recognised or one of the
//version subnumber includes than 3 digits.
//(long return type ensures the type has a minimum of 32 bits.
long version_string_to_int(const std::string& version_string);

std::string version_int_to_string(long version_int, int depth = 3);

int version_major(long version_int);

template<typename T, typename U>
bool has(const T& collection, const U& element){
  return std::find(collection.begin(), collection.end(), element) != collection.end();
}

template<typename T, typename U, typename F>
bool has(const T& collection, const U& element){
  F equals;
  for(const auto& e: collection){
    if(equals(e, element)) return true;
  }
  return false;
}

void replace(std::string& s, const std::string& to_replace,
             const std::string& replacement);

std::ostream& operator<<(std::ostream& stream, const CXString& str);
std::ostream& operator<<(std::ostream& stream, const CXCursor& x);
std::ostream& operator<<(std::ostream& stream, const CXType& x);
std::ostream& operator<<(std::ostream& stream, const CXTypeKind& x);
std::ostream& operator<<(std::ostream& stream, const CXCursorKind& x);
std::ostream& operator<<(std::ostream& stream, CX_CXXAccessSpecifier x);
std::ostream& operator<<(std::ostream& stream, CXSourceLocation location);
std::ostream& operator<<(std::ostream& stream, const CXFile& file);
std::string str(const CXString& x);


std::string jl_type_name(const std::string& s);
std::string jl_type_name(const CXType& t);

std::string fully_qualified_name(CXCursor c);

std::string fully_qualified_name(CXType type);

int hasDefaultConstructor(CXCursor cursor);

CXType remove_non_builtin_qualifiers(CXType& type);

CXType base_type(CXType type);


/** Concatenate a list of strings inserting an optional separator between
 * the strings. When used with a vector of elements of a different
 * type, the elements will be converted to string.
 * example: join(", ", {"cat", "dog", "rabbit"} -> "cat, dog, rabbit"
 */
template<typename T>
std::string join(const std::vector<T>& elts, const std::string& sep = "");


template<typename Coll>
Coll myapply(const Coll&v, typename Coll::value_type (*f)(const typename Coll::value_type& x)){
  Coll new_v;
  for(const auto& e: v){
    new_v.emplace_back(f(e));
  }
  return new_v;
}

template<typename T>
std::string join(const std::vector<T>& elts, const std::string& sep){
  const char* sep_ = "";
  std::stringstream buf;
  for(const auto& s: elts){
    buf << sep_;
    buf << s;
    sep_ = sep.c_str();
  }
  return buf.str();
}

std::vector<std::string> get_template_parameters(CXCursor cursor);

std::string join_paths(const std::string& p1, const std::string& p2);

#endif //UTILS_H not defined
