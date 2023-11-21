#ifndef A_H
#define A_H

struct A1{
  A1(): i(1){}
  int f(){return i;}
private:
  int i;
};


class Vetoed1{
  Vetoed1(): i(0){}
  void g(){}
  int i;
};


struct A2{
  A2(): i(2){}
  int f(){ return i;}
private:
  int i;
};

struct Vetoed2{
  Vetoed2(){}
  int f(){ return 2;}
  float x;
};


struct A3{
  A3(): i(3){}
  int f(){ return i;}
private:
  int i;
};

struct A4{
  A4(): i(4){}
  int f(){ return i;}
private:
  int i;
};

struct A5{
  A5(): i(5){}
  int f(){ return i;}
private:
  int i;
};

struct A6{
  A6(): i(6){}
  int f(){ return i;}
private:
  int i;
};


#endif //A_H not defined
