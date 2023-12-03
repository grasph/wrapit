#ifndef A_H
#define A_H

#include <string_view>

size_t f(const std::string_view& sv) { return sv.size(); }

std::string_view f(){
  return std::string_view("Hello");
}

#endif //A_H not defined
