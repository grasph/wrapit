using Test
using Serialization

push!(LOAD_PATH, "$(@__DIR__)/build")
using TestVarFieldOff

function runtest()
    @testset "Variable and field accessors switch" begin
        @test :B ∈ names(TestVarFieldOff, all=true)
        @test :C ∉ names(TestVarFieldOff, all=true)
        @test :global_C ∉ names(TestVarFieldOff, all=true)
        @test :global_C! ∉ names(TestVarFieldOff, all=true)
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end

