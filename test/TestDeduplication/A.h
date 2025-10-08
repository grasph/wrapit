#include <iostream>

struct A{
  const char* f() { return "const char* f()"; }
  const char* f() const { return "const char* f() const"; }

  const char* g(int){ return "const char* g(int)"; }
  const char* g(int64_t){ return "const char* g(int64_t)"; }


  static const char* h(int){ return "static const char* h(int)"; }
  static const char* h(int64_t){ return "static const char* h(int64_t)"; }
};
