#include <string>

//template<typename T> class B{ };

//typedef B<int> C;

struct B1 {
  bool isB1() { return true; }
};

typedef B1 C1;

class A_typedef: public C1 {
public:
  int f() { return 0; }
};

template<typename T>
struct B2{
  T x;
  bool isB2(){ return true; }
};

typedef B2<int> C2;

template class B2<int>;

class A_template: public B2<int> {
public:
  int f() { return 1; }
};

class A_typedef_template: public C2 {
public:
  int f() { return 2; }
};
