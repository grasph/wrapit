#include "cxxwrap_version.h"

//according to https://github.com/JuliaInterop/libcxxwrap-julia/releases
//we have following CxxWrap/libcxxwrap version mapping
//v0.10.0 (not used by any CxxWrap release),
//v0.11.0 (CxxWrap v0.14.0)
//v0.12.0 (CxxWrap v0.15.0) change in generated code
//v0.13.0 (CxxWrap v0.16.0)
//v0.14.0 (CxxWrap v0.17.0) change in generated code


std::tuple<long, long> version_libcxxwrap_static_bounds(long cxxwrap_version){
  if(cxxwrap_version < cxxwrap_v0_15){
    return std::tuple<long, long>(11*1000, 12*1000);
  } else if(cxxwrap_version < cxxwrap_v0_17){
    return std::tuple<long, long>(12*1000, 14*1000);
  } else{
    return std::tuple<long, long>(14*1000, 15*1000);
  }
}

std::tuple<long, long> version_libcxxwrap_runtime_bounds(long cxxwrap_version){
  if(cxxwrap_version < cxxwrap_v0_15){    //CxxWrap v0.14
    return std::tuple<long, long>(11*1000, 12*1000);
  } else if(cxxwrap_version < 16 * 1000){ //CxxWrap v0.15
    return std::tuple<long, long>(12*1000, 13*1000);
  } else if(cxxwrap_version < 17 * 1000){ //CxxWrap v0.16
    return std::tuple<long, long>(13*1000, 14*1000);
  } else{ //CxxWrap v0.17+
    return std::tuple<long, long>(14*1000, 15*1000);
  }
}
