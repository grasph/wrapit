template<typename T>
class A {
public: 
  A(): a(0){}

  T getval(){ return a;}
  
  void setval(const T& val){ a = val; }

  T a;
};

template class A<int>;
