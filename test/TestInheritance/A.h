#include "C.h"
#include <string>

class E;

#include "D.h"

struct A1{
  int f1() { return 1; }
  void hidden_method(){}
  virtual  std::string virtual_func(){ return std::string("A1::virtual_func()"); }
  std::string real_func(){ return std::string("A1::real_func()"); }
};

struct A2{
  int f2() { return 2; }
  virtual ~A2(){}
};

struct A2bis{
  void f2(int) {}
};

struct B: public A1, A2, C {
  int f3(){ return 3; }
  void hidden_method(int i){}; //hide A1::hidden_method
  virtual std::string virtual_func(){ return std::string("B::virtual_func()"); }
  std::string real_func(){ return std::string("B::real_func()"); }
};

//note: .wit file configuration asks for B <: A2 in Julia
struct B2: public A1, A2, C {
  void hidden_method(int i){}; //hide A1::hidden_method
  int f7(){ return 7; }
};

struct E: public D {
  int f6(){ return 6; }
};

A1* new_B(){
  return new B();
};

//note: supertype in Julia forced to Any by .wit configuration
struct F: public A1{
  int f7(){ return 7;}
  void hidden_method(float){};
};

//to test that f2(int) is ignored as it should
//because of overriding ambiguity in C++
struct G1: public A2, A2bis{
};

//to test that both f2() and f2(int) is ignored as it should
//because of overriding ambiguity in C++
struct G2: public A1, A2, A2bis{
};
