# Drive the build of the wrapit hello example
#
using CxxWrap
cxxwrap_prefix = CxxWrap.prefix_path()

build_location = "build"

if !isdir(build_location)
    mkdir(build_location)
end
cd(build_location)

try
    run(`cmake -S .. -B . -DCMAKE_PREFIX_PATH=$cxxwrap_prefix`)
    run(`cmake --build .`)
    true
catch
    false
end
