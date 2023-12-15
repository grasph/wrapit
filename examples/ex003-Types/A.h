#include <string>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

/** Class A that says hello.
 */
class A {
public:

  A(const char* person = "World"): person_(person), c_(0){}

  void say_hello() const{
    std::cout << "Hello " << person_ << "!\n";
  }

  std::string whisper_hello() const {
    std::stringstream ss;
    ss << "Hello " << person_ << "!";
    return ss.str();
  }

  const int citr() {
    c_++;
    return c_;
  }

  const std::pair<int, int> dbl() const {
    return std::pair<int, int>(c_, c_);
  }

  std::vector<int> vtr() const {
    std::vector<int> v{1,2,3,4,5,6,7,8,9};
    return v;
  }

private:
  std::string person_;
  int c_;
};

