#include <string>
#include <iostream>

class A {
public:
  A(const char* person): person_(person){}
  void say_hello() const{
    std::cout << "Hello " << person_ << "!\n";
  }
private:
  std::string person_;
};

