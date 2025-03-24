#include "C.h"

struct B{
  B(int x = 2): field(x){}
  int field;
};

struct NotAssignale {
  NotAssignale& operator=(const NotAssignale& a) = delete;
};

struct NotAssignale2 {
  NotAssignale2& operator=(const NotAssignale2&& a) = delete;
};

struct NoCopyCtor {
  NoCopyCtor() {}
  NoCopyCtor(const NoCopyCtor&)  = delete;
};

struct NoMoveAssignment {
  NoMoveAssignment& operator=(NoMoveAssignment&&) = delete;
  NoMoveAssignment& operator=(const NoMoveAssignment&) { return *this; };
};

class PrivateCopyOp {
  PrivateCopyOp& operator=(const PrivateCopyOp& a){
    return *this;
  }
};


struct A {
  A(int x = 1): field_int(x), field_int_const(3), field_B(2*x){}
  A& operator=(const A& x){
    field_int = x.field_int;
    field_B = x.field_B;
    return *this;
  }
  int field_int;
  const int field_int_const;
  B field_B;
  C field_C;
  static int static_field;
};

extern int global_int;
extern A global_A;
extern const A global_A_const;
extern C global_C;
extern NotAssignale global_NotAssignable;
extern NotAssignale2 global_NotAssignable2;
extern NoMoveAssignment global_NoMoveAssignment;
extern NoCopyCtor global_NoCopyCtor;
extern PrivateCopyOp global_PrivateCopyOp;


namespace ns{
  struct A {
    A(): field(2){} ;
    int field;
    static int static_field;
  };
  extern int global_var;
}
