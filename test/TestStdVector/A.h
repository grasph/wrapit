#ifndef A_H
#define A_H

#include <vector>
#include <cstdint>
#include <string>
#include <iostream>

struct EltType1{
  EltType1(int64_t i = 0): i_(i){}
  int64_t i_;
};

struct EltType2_{
  EltType2_(int64_t i = 0): i_(i){}
  int64_t i_;
};


struct EltType3_{
  EltType3_(int64_t i = 0): i_(i){}
  int64_t i_;
};

struct EltType4_{
  EltType4_(int64_t i = 0): i_(i){}
  int64_t i_;
};

struct EltType5_{
  EltType5_(int64_t i = 0): i_(i){}
  int64_t i_;
};


using EltType2 = EltType2_;
typedef EltType3_ EltType3;

using EltType4Ptr = EltType4_*;
typedef EltType5_ *EltType5Ptr;

struct EltType6{
  EltType6(int64_t i = 0): i_(i){}
  bool operator==(int64_t i) const{
    return i_ == i;
  }
  int64_t i_;
};

std::vector<EltType6> f6();

struct EltType7{
  EltType7(int64_t i = 0): i_(i){}
  bool operator==(int64_t i) const{
    return i_ == i;
  }
  int64_t i_;
};

std::vector<EltType7*> f7();

class MyString: public std::string{
};

typedef MyString *MyStringPtr;

int mysum(std::vector<EltType1> v);

int mysum(std::vector<EltType2> v);

int mysum(std::vector<EltType3> v);

int mysum4(std::vector<EltType4Ptr> v);

int mysum5(std::vector<EltType5Ptr> v);

struct A{
  std::vector<MyString>& f8(){
    static std::vector<MyString> v({MyString("a"), MyString("b")});
    return v;
  }
};


#endif //A_H not defined
