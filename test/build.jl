# Drive the build of a test example
#
# Use this script from within the test directory, e.g.,
#
# cd TestStdString
# julia --project=.. ../build.jl

using CxxWrap
cxxwrap_prefix = CxxWrap.prefix_path()

build_location = "build"

run(`cmake -S . -B $build_location -DCMAKE_PREFIX_PATH=$cxxwrap_prefix`)
run(`cmake --build $build_location`)
