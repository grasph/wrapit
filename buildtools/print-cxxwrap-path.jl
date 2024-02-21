#!/usr/bin/env julia
#
using Pkg

# Bootstrap environment for the first run
redirect_stdio(stdout=stderr) do
    Pkg.activate(@__DIR__)
    Pkg.instantiate()
end

# Output the path to CxxWrap to be used by CMake
import CxxWrap
println(CxxWrap.prefix_path())
