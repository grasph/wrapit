#include <iostream>

struct A{
  static int dtor_ncalls;
  A(){}
  ~A(){
    ++dtor_ncalls;
    std::cout << "A::~A called\n";
  }
};


struct B{
  static int dtor_ncalls;
  ~B(){
    ++dtor_ncalls;
    std::cout << "B::~B called\n";
  }
};

struct C{
  static int dtor_ncalls;
  C(){}
protected:
  ~C(){
    ++dtor_ncalls;
    std::cout << "C::~C called\n";
  }
};

struct D{
  static int dtor_ncalls;
private:
  ~D(){
    ++dtor_ncalls;
    std::cout << "C::~C called\n";
  }
};

struct E{
  static int dtor_ncalls;
  ~E() = delete;
};
