module ROOT

import Base.getindex
import Base.setindex!

using CxxWrap
@wrapmodule(()->"$(@__DIR__)/../deps/libjlROOT")

function __init__()
    @initcxx
    global gROOT = ROOT!GetROOT()
    push!(LOAD_PATH, @__DIR__)
end

export gROOT, gSystem
include("ROOT-export.jl")

end #module
