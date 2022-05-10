#include "C.h"
#include <string>

class E;

#include "D.h"

struct A1{
  int f1() { return 1; }
  virtual  std::string virtual_func(){ return std::string("A1::virtual_func()"); }
  std::string real_func(){ return std::string("A1::real_func()"); }
};

struct A2{
  int f2() { return 2; }
};

struct B: public A1, A2, C {
  int f3(){ return 3; }
  virtual std::string virtual_func(){ return std::string("B::virtual_func()"); }
  std::string real_func(){ return std::string("B::real_func()"); }
};

struct E: public D {
  int f6(){ return 6; }
};

A1* new_B(){
  return new B();
}

