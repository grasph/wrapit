#include <string>
#include <iostream>
#include <sstream>

/** Class A that says hello.
 */
class A {
public:

  A(const char* person = "World"): person_(person){}

  void say_hello() const{
    std::cout << "Hello " << person_ << "!\n";
  }

  std::string whisper_hello() const {
    std::stringstream ss;
    ss << "Hello " << person_ << "!";
    return ss.str();
  }

private:
  std::string person_;
};

