struct AccessDeleteA{
  //deleting default ctor and providing one
  //through default argument value is awkward
  //but the code compile till no call to
  //the ctor with no argument is performed:
  AccessDeleteA() = delete;
  AccessDeleteA(int i = 0): i_(i) {}

  int i_;
  int f() { return 10; }
  int g() { return 11; }
  int h() { return 12; }
private:
  int e() { return 13; }
};

class AccessDeleteB: public AccessDeleteA{
  AccessDeleteB(int i): AccessDeleteA(i) {}
  int g() { return -1; }
public:
  AccessDeleteB(int i, int j): AccessDeleteA(i+j) {}
  int f() = delete;
};

struct AccessDeleteC{
  AccessDeleteC(int i = 0) = delete;
};

struct AccessDeleteD{
private:
  AccessDeleteD(){}
};
