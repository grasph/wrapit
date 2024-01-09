# Drive the build of the wrapit hello example
#
using CxxWrap
cxxwrap_prefix = CxxWrap.prefix_path()

original_wd = pwd()
build_location = "build"

if !isdir(build_location)
    mkdir(build_location)
end
cd(build_location)

try
    run(`cmake -S .. -B . -DCMAKE_PREFIX_PATH=$cxxwrap_prefix`)
    run(`cmake --build .`)
    cd(original_wd)
    true
catch
    cd(original_wd)
    false
end
