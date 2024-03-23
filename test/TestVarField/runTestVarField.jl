#!/usr/bin/env julia
using Test
using Serialization
import Pkg

function runtest()
    args = collect(Base.julia_cmd())
    push!(args, "")
    iscript = length(args)
    push!(args, "-s")
    ENV["JULIA_PROJECT"] = Pkg.project().path
    @testset verbose=true "TestVarField" begin
        for testname in ["TestVarFieldOn" "TestVarFieldOff"]
            args[iscript] = "run" * testname * ".jl"
            Test.record(Test.get_testset(), deserialize(open(Cmd(Cmd(args), dir=@__DIR__))))
        end
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
