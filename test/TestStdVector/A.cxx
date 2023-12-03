#include "A.h"

std::vector<EltType6> f6(){
  return std::vector<EltType6>({EltType6(1), EltType6(2)});
}

std::vector<EltType7*> f7(){
  static EltType7 v7[] = {EltType7(1), EltType7(2)};
  return std::vector<EltType7*>({v7, v7 + 1});
}

int mysum(std::vector<EltType1> v){
  int64_t acc = 0;
  for(const auto& e: v) acc += e.i_;
  return acc;
}

int mysum(std::vector<EltType2> v){
  int64_t acc = 0;
  for(const auto& e: v) acc += e.i_;
  return acc;
}

int mysum(std::vector<EltType3> v){
  int64_t acc = 0;
  for(const auto& e: v) acc += e.i_;
  return acc;
}

int mysum4(std::vector<EltType4Ptr> v){
  int64_t acc = 0;
  for(const auto& e: v) acc += e->i_;
  return acc;
}

int mysum5(std::vector<EltType5Ptr> v){
  int64_t acc = 0;
  for(const auto& e: v) acc += e->i_;
  return acc;
}
