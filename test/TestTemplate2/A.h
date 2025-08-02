#include <string>

template<typename S>
class A {
public:
  A(): a(0){}

  S getval(){ return a;}

  void setval(const S& val){ a = val; }

  S a;
};

template class A<int>;

namespace ns{
  template<typename S>
  class B {
  public:
    B(): a(0){}

    S getval(){ return a;}

    void setval(const S& val){ a = val; }

    typedef typename std::string String;
    int slen(const String& s) { return s.size(); }


    S a;
  };

  template class B<int>;
}
