// Wrappers added manually with the hooks

#include "jlcxx/jlcxx.hpp"
#include <iostream>
struct B{
  int f(){ return 2; }
};

namespace jlcxx {
  template<> struct IsMirroredType<B> : std::false_type { };
}

void end_add_type_hook(jlcxx::Module& jlModule){
  std::cerr << "User end_add_type hook called.\n";
  jlModule.add_type<B>("B");
}

void end_hook(jlcxx::Module& jlModule){
  std::cerr << "User end hook called.\n";
  jlModule.method("f", [](B& b){ return b.f(); });
}
