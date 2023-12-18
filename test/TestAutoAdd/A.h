#ifndef A_H
#define A_H

#include "B.h"

B2 f(const B1& b1){ return B2(b1); }

struct A{
  B4 f(const B3& b3){ return B4(b3); }
};


#endif //A_H not defined
