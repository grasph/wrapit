#include <iostream>

enum class A {
   C = 1,
   D = 2
};

enum class B {
   C = 3,
   D = 4
};

enum E {
  E1 = 5,
  E2 = 6
};

// Only the definition should create a binding, otherwise a duplicate will be
// created for the forward declaration which will cause an error during
// precompilation.
enum ForwardDecl : int;
enum ForwardDecl : int {
    X = 1,
    Y = 2
};

//void f(int) { std::cout << __FUNCTION__ << "\n"; }
A f(A a) { return a; }
