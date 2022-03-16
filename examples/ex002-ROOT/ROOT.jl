module ROOT

import Base.getindex
import Base.setindex!

using CxxWrap
@wrapmodule("jlROOT")

function __init__()
    @initcxx
    global gROOT = ROOT!GetROOT()
    global gSystem = GetSystem()
end

export gROOT, gSystem
include("ROOT-export.jl")

end #module
