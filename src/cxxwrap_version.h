#include <tuple>

//Default value for the cxxwrap_version config parameter
static const char* default_cxxwrap_version = "0.15.0";

static const char* supported_cxxwrap_versions = "0.14.x, 0.15.x";

static int cxxwrap_v0_15 = 15 * 1000; //v0.15

std::tuple<long, long> version_libcxxwrap_bounds(long cxxwrap_version);

