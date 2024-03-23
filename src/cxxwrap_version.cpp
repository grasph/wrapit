#include "cxxwrap_version.h"

//according to https://github.com/JuliaInterop/libcxxwrap-julia/releases
//we have breaking changes with libcxxwrap_jll versions,
//v0.10.0 (not used by any CxxWrap release),
//v0.11.0 (CxxWrap v0.14.0),
//v0.12.0 (CxxWrap v0.15.0)
std::tuple<long, long> version_libcxxwrap_bounds(long cxxwrap_version){
  if(cxxwrap_version < cxxwrap_v0_15){
    return std::tuple<long, long>(11*1000, 12*1000);
  } else{
    return std::tuple<long, long>(12*1000, 13*1000);
  }
}
