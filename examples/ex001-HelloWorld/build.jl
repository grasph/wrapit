# Drive the build of the wrapit hello example
#
using CxxWrap
cxxwrap_prefix = CxxWrap.prefix_path()

build_location = "build"

try
    run(`cmake -S . -B $build_location -DCMAKE_PREFIX_PATH=$cxxwrap_prefix`)
    run(`cmake --build $build_location`)
    true
catch
    false
end
