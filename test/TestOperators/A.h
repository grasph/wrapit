#include <string>

struct A {
  int i;

  int* p(){ return &i; }

  A(): i(0){}
  
  std::string last_call;
  A operator+(A const & rhs){
    last_call = std::string("member infix +");
    return *this;
  }
  A operator+() {
    last_call = std::string("member prefix +");
    return *this;
  }
  A operator-(A const & rhs){
    last_call = std::string("member infix -");
    return *this;
  }
  A operator-() {
    last_call = std::string("member prefix -");
    return *this;
  }
  A operator/(A const & rhs){
    last_call = std::string("member infix /");
    return *this;
  }
  A operator*(A const & rhs){
    last_call = std::string("member infix *");
    return *this;
  }
  A operator*() {
    last_call = std::string("member prefix *");
    return *this;
  }
  A operator%(A const & rhs){
    last_call = std::string("member infix %");
    return *this;
  }
  A operator!() {
    last_call = std::string("member prefix !");
    return *this;
  }
  A operator&(A const & rhs){
    last_call = std::string("member infix &");
    return *this;
  }
  A operator|(A const & rhs){
    last_call = std::string("member infix |");
    return *this;
  }
  A operator^(A const & rhs){
    last_call = std::string("member infix ^");
    return *this;
  }
  A operator&&(A const & rhs){
    last_call = std::string("member infix &&");
    return *this;
  }
  A operator||(A const & rhs){
    last_call = std::string("member infix ||");
    return *this;
  }
  A operator<(A const & rhs){
    last_call = std::string("member infix <");
    return *this;
  }
  A operator>(A const & rhs){
    last_call = std::string("member infix >");
    return *this;
  }
  A operator<=(A const & rhs){
    last_call = std::string("member infix <=");
    return *this;
  }
  A operator>=(A const & rhs){
    last_call = std::string("member infix >=");
    return *this;
  }
  A operator==(A const & rhs){
    last_call = std::string("member infix ==");
    return *this;
  }
  A operator!=(A const & rhs){
    last_call = std::string("member infix !=");
    return *this;
  }
  A operator<=>(A const & rhs){
    last_call = std::string("member infix <=>");
    return *this;
  }
  A operator++() {
    last_call = std::string("member prefix ++");
    return *this;
  }
  A operator++(int) {
    last_call = std::string("member postfix ++");
    return *this;
  }
  A operator--() {
    last_call = std::string("member prefix --");
    return *this;
  }
  A operator--(int) {
    last_call = std::string("member postfix --");
    return *this;
  }
  A* operator->(){
    last_call = std::string("member ->");
    return this;
  }
  A* operator->*(int* p){
    last_call = std::string("member ->*");
    return this;
  }
  A operator+=(A const & rhs){
    last_call = std::string("member infix +=");
    return *this;
  }
  A operator-=(A const & rhs){
    last_call = std::string("member infix -=");
    return *this;
  }
  A operator/=(A const & rhs){
    last_call = std::string("member infix /=");
    return *this;
  }
  A operator*=(A const & rhs){
    last_call = std::string("member infix *=");
    return *this;
  }
  A operator%=(A const & rhs){
    last_call = std::string("member infix %=");
    return *this;
  }
  A operator^=(A const & rhs){
    last_call = std::string("member infix ^=");
    return *this;
  }
  A operator&=(A const & rhs){
    last_call = std::string("member infix &=");
    return *this;
  }
  A operator|=(A const & rhs){
    last_call = std::string("member infix |=");
    return *this;
  }
};

struct C {
  std::string last_call;
};

C operator+(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix +");
  return lhs;
}

C operator+(C& lhs) {
  lhs.last_call = std::string("non-member prefix +");
  return lhs;
}

C operator-(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix -");
  return lhs;
}
C operator-(C& lhs) {
  lhs.last_call = std::string("non-member prefix -");
  return lhs;
}
C operator/(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix /");
  return lhs;
}
C operator*(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix *");
  return lhs;
}
C operator*(C& lhs) {
  lhs.last_call = std::string("non-member prefix *");
  return lhs;
}
C operator%(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix %");
  return lhs;
}
C operator!(C& lhs) {
  lhs.last_call = std::string("non-member prefix !");
  return lhs;
}
C operator&(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix &");
  return lhs;
}
C operator|(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix |");
  return lhs;
}
C operator^(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix ^");
  return lhs;
}
C operator&&(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix &&");
  return lhs;
}
C operator||(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix ||");
  return lhs;
}
C operator<(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix <");
  return lhs;
}
C operator>(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix >");
  return lhs;
}
C operator<=(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix <=");
  return lhs;
}
C operator>=(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix >=");
  return lhs;
}
C operator==(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix ==");
  return lhs;
}
C operator!=(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix !=");
  return lhs;
}
C operator<=>(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix <=>");
  return lhs;
}
C operator++(C& lhs) {
  lhs.last_call = std::string("non-member prefix ++");
  return lhs;
}
C operator++(C& lhs, int) {
  lhs.last_call = std::string("non-member postfix ++");
  return lhs;
}
C operator--(C& lhs) {
  lhs.last_call = std::string("non-member prefix --");
  return lhs;
}
C operator--(C& lhs, int) {
  lhs.last_call = std::string("non-member postfix --");
  return lhs;
}
C operator+=(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix +=");
  return lhs;
}
C operator-=(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix -=");
  return lhs;
}
C operator/=(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix /=");
  return lhs;
}
C operator*=(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix *=");
  return lhs;
}
C operator%=(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix %=");
  return lhs;
}
C operator^=(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix ^=");
  return lhs;
}
C operator&=(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix &=");
  return lhs;
}
C operator|=(C& lhs, C const & rhs){
  lhs.last_call = std::string("non-member infix |=");
  return lhs;
}
