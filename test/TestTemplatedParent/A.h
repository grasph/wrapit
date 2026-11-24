#include <cstdint>

namespace ns1{
  template<typename U1, typename U2>
  struct Parent{
    U1 f1(U2 u){ return 2*u; }
    U2 f2(){ return 2; }
  };
};

using namespace ns1;

namespace ns2{

  struct Child_0param: public Parent<float, float>{
  };

  template<typename T1>
  struct Child_1param: public Parent<T1, float>{
  };

  template<typename T1, typename T2>
  struct Child_2params: public Parent<T1, T2>{
  };

  template struct Child_1param<int32_t>;
  template struct Child_2params<int32_t, int64_t>;
};
