namespace ns1{
  namespace ns2 {
  struct C {
    C(): i(0){}
    int i;
    void set(int i_){ i = i_;}
  };

  static int global_var = 2;

    template<typename T>
    struct A {

      A(): a(0){}

      T getval(){ return a;}

      void setval(const T& val){ a = val; }
      T a;
    };

    template class A<int>;
    
    void g(A<int>& a) { }
    
    struct B {
      void f(A<int>& a) { a = data1; }
      void g_ref(C& c) { c = data2; }
      void g_ptr(C* c) { *c = data2; }
      void g_ptrofptr(C** c) { **c = data2; }
      void g_arrofptr(C* c[]){ *c[0] = data2; }
      A<int> data1;
      C data2;
    };
  }
}
