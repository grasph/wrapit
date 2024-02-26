module ROOT

import Base.getindex
import Base.setindex!

using CxxWrap
@wrapmodule(()->"$(@__DIR__)/../deps/libjlROOT")

function __init__()
    @initcxx
    global gROOT = ROOT!GetROOT()
end

export gROOT, gSystem
include("ROOT-export.jl")
export SetAddress

end #module
