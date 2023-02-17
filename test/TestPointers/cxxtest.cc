#include "A.h"

int main(){
  std::cout << __FUNCTION__ << "\n";
  std::cout << "\n";
  std::cout<< "f_val().i: " << f_val().i << "\n";
  std::cout << "f_valconst().i: " << f_valconst().i << "\n";
  std::cout << "f_ref().i: " << f_ref().i << "\n";
  std::cout << "f_constref().i: " << f_constref().i << "\n";
  std::cout << "f_ptr()->i: "  << f_ptr()->i << "\n";
  std::cout << "f_constptr()->i: " << f_constptr()->i << "\n";
  std::cout << "f_sharedptr()->i: " << f_sharedptr()->i << "\n";
  std::cout << "f_uniqueptr()->i: " << f_uniqueptr()->i << "\n";
  std::cout << "f_weakptr(f_shared_ptr).lock()->i: " << f_weakptr(f_sharedptr()).lock()->i << "\n";
  std::cout << "\n";
}
