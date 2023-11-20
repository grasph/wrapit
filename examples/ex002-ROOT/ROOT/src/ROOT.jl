module ROOT

import Base.getindex
import Base.setindex!

using CxxWrap
@wrapmodule(()->"libjlROOT")

function __init__()
    @initcxx
    global gROOT = ROOT!GetROOT()
end

export gROOT, gSystem
include("ROOT-export.jl")

end #module
