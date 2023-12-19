struct B1{
  B1(): i_(0){}
  int i_;
};

struct B2{
  B2(const B1& b1): b1_(b1){}
  //  void nottowrap(){}
  B1 b1_;
};

struct B3{
  B3(): i_(0){}
  int i_;
};

struct B4{
  B4(const B3& b3): b3_(b3){}
  B3 b3_;
};
