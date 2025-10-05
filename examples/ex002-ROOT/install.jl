#!/usr/bin/env julia

import Pkg

#Number of CPU cores to use for code compilation
ncores=Sys.CPU_THREADS

# Drive the build of the wrapit hello example
#
build_location = "build"
wrapper_module = "ROOT" 

#Generate the wrapper code and compile:
run(`make -j $ncores`)

#Install the wrapper module:
Pkg.develop(path="$build_location/$wrapper_module")

