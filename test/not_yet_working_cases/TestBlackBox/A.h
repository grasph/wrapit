#ifndef A_H
#define A_H

struct BlackBox
{
  BlackBox(int i): i_(i) {}
  int i_;
};

typedef int NonStructBlackBox;

BlackBox get1(int i){ return BlackBox(i); }

NonStructBlackBox get2(int i){ return NonStructBlackBox(i); }

int val(const BlackBox& b){ return b.i_; }

int val(const NonStructBlackBox& b){ return b; }

template<typename T>
class A{
  T i_;
public:
  A(const T& i):i_(i){}
  T val() const{ return i_;}
};

template class A<int>;
template class A<BlackBox>;

static BlackBox bb2(10);

class BlackBox2;

const BlackBox2* get3() { return reinterpret_cast<BlackBox2*>(&bb2); }

int val(const BlackBox2* x){ return reinterpret_cast<const BlackBox*>(x)->i_; }


#endif //A_H not defined
