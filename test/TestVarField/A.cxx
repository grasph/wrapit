#include "A.h"

int A::static_field = 1;
int global_int = 1;
A global_A(10);
const A global_A_const(11);
C global_C(1);


namespace ns{
  int A::static_field = 2;
  int global_var = 2;
};

const A& get_const_global_A(){ return global_A; }
A& get_global_A(){ return global_A; }

