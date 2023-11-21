#include <string>

struct A: public std::string{
  A(const char* c): std::string(c){}
  std::string f(){ return *this + "_"; }
};

