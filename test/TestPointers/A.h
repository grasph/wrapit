#ifndef A_H
#define A_H

#include <iostream>
#include <memory>

struct A{
  A():i(++n) {}
  A(const A& a): i(++n){}
  ~A() {}
  static int n;
  int i;
};

int A::n = 0;

A a;

A f_val(){ return a; }
const A f_valconst(){ return a; }
A& f_ref(){ return a; }
const A& f_constref(){ return a; }
A* f_ptr(){ return &a; }
const A* f_constptr(){return &a; }
std::shared_ptr<A> f_sharedptr(){ return std::shared_ptr<A>(new A()); }
std::unique_ptr<A> f_uniqueptr(){ return std::unique_ptr<A>(new A()); }
std::weak_ptr<A> f_weakptr(std::shared_ptr<A> a){ return std::weak_ptr<A>(a); }

int getiofacopy_val(A a){ return a.i; }
int geti_constref(const A& a){ return a.i; }
int inci_ref(A& a){ return ++a.i; }
int geti_constptr(const A* a){ return a->i; }
int inci_ptr(A* a){ return ++(a->i); }
int inci_sharedptr(std::shared_ptr<A>& a){ return ++(a->i); }
int inci_uniqueptr(std::unique_ptr<A>& a){ return ++(a->i); }
int inci_weakptr(std::weak_ptr<A> a){ return ++(a.lock()->i); }

#endif //A_H not defined
