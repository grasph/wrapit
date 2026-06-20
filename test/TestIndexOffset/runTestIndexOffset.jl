#!/usr/bin/env julia
using Test
using Serialization

function runtest()
    @testset verbose=true "TestIndexOffset" begin
        for test_script in ["runTestIndexOffset1.jl" "runTestIndexOffset2.jl"]
            Test.record(Test.get_testset(), deserialize(open(Cmd(`julia --project=.. $test_script -s`, dir=@__DIR__))))
        end
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
