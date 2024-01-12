#!/usr/bin/env julia
#
using Pkg

# Bootstrap environment for the first run
redirect_stdio(stdout="bootstrap.log", stderr="bootstrap.log") do
    Pkg.activate(@__DIR__)
    Pkg.instantiate()
end

# Output the path to CxxWrap to be used by CMake
using CxxWrap
println(CxxWrap.prefix_path())
