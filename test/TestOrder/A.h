#ifndef A_H
#define A_H

//This test check that wrapit reshuffle the
//wrapper declaration in order suitable for CxxWrap.
//Types are declared in an order that would lead to a Julia mofule loading
//with a "missing factory" message, if their wrapper are declared in
//the same order.

//use forward declaration in order to use pointer or reference
//of these types before they are declared.
struct B4;
struct B3;
struct B2;
struct B1;

struct C1;
struct C2;

extern B4 b4;
extern B3 b3;

struct Achild{
};

struct Aparent: Achild{
  bool f1(B1*){ return true; }
  bool f2(B2&){ return true; }
  B3* f3(){ return &b3; }
  B4& f4(){ return b4; }
};

template<typename T, typename S> struct Atemplate{
  bool f(T&, S&){ return true;}
};

template class Atemplate<C1, C2>;

struct B4 {};
struct B3 {};
struct B2 {};
struct B1 {};
struct C1 {};
struct C2 {};

#endif //A_H not defined
