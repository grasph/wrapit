struct A{
  //deleting default ctor and providing one
  //through default argumet value is awkward
  //but the code compile till no call to
  //the ctor with no argument is performed:
  A() = delete;
  A(int i = 0): i_(i) {}

  int i_;
  int f() { return 10; }
  int g() { return 11; }
  int h() { return 12; }
private:
  int e() { return 13; }
};

class B: public A{
  B(int i): A(i) {}
  int g() { return -1; }
public:
  B(int i, int j): A(i+j) {}
  int f() = delete;
};

struct C{
  C(int i = 0) = delete;
};

struct D{
private:
  D(){}
};
