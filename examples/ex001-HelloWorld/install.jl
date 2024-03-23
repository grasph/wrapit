#!/usr/bin/env julia

import Pkg

# Drive the build of the wrapit hello example
#
build_location = "build"
wrapper_module = "Hello" 

#Generate the wrapper code and compile:
run(`cmake --fresh -B $build_location`)
run(`cmake --build $build_location`)

#Install the wrapport module:
Pkg.develop(path="$build_location/$wrapper_module")

