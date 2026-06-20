#include <vector>
struct A{

  std::vector<int> data_;

  A(int i): data_(i, 0){};

  int& operator[](int i) {
    // std::cerr << "Setting a[" << i << "].\n";
    return data_.at(i);
  }

};
