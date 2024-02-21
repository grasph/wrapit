using Test
using Serialization

push!(LOAD_PATH, "$(@__DIR__)/build")
import TestPropagation1

function runtest()
    @testset "Test 1 of method_propagation modes" begin
        @test :f_B in names(TestPropagation1, all=true)
        @test !(:f_A in names(TestPropagation1, all=true))
        @test begin; try; TestPropagation1.f_B(TestPropagation1.B(), TestPropagation1.A()); true; catch; false; end; end
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
