#include <string>
#include <iostream>

/** Class A that says hello.
 */
class A {
public:
  A(const char* person = "World"): person_(person){}
  void say_hello() const{
    std::cout << "Hello " << person_ << "!\n";
  }
private:
  std::string person_;
};

