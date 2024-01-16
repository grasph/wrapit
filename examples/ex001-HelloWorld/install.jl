#!/usr/bin/env julia

import Pkg

# Drive the build of the wrapit hello example
#
build_location = "build"
wrapper_module = "Hello" 

#Generate the wrapper code and compile:
run(`cmake -S . -B $build_location`)
run(`cmake --build $build_location`)


#Temporary solution, in the final one, wrapit will generate
projecttoml="$build_location/$wrapper_module/Project.toml"
isfile(projecttoml) || open(projecttoml, "w") do f
    println(f, """
name = "Hello"
uuid = "bcc47610-b56b-11ee-863b-7bdb89c8e0c8"
[deps]
CxxWrap = "1f15a43c-97ca-5a2a-ae31-89f07a497df4"
""")
end

#Install the wrapport module:
Pkg.develop(path="$build_location/$wrapper_module")

