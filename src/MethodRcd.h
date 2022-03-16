#ifndef METHODRCD_H
#define METHODRCD_H

#include <clang-c/Index.h>

struct MethodRcd{
  CXCursor cursor;
  int min_args;
  MethodRcd(CXCursor cursor, int min_args = -1): cursor(cursor), min_args(min_args){}
};
    

#endif //METHODRCD_H not defined
