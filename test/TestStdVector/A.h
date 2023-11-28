#include <vector>
#include <cstdint>

struct EltType{
  EltType(int64_t i = 0): i_(i){}
  int64_t i_;
};

int mysum(std::vector<EltType> v){
  int64_t acc = 0;
  for(const auto& e: v) acc += e.i_;
  return acc;
}
