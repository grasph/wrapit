using Test
using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path="$(@__DIR__)/build/TestPropagation3")
import TestPropagation3

function runtest()
    @testset "Test 3 of method_propagation modes" begin
        @test :f_B in names(TestPropagation3, all=true)
        @test :f_A in names(TestPropagation3, all=true)
        @test begin; try; TestPropagation3.f_B(TestPropagation3.B(), TestPropagation3.A()); true; catch; false; end; end
        @test begin; try; TestPropagation3.f_A(TestPropagation3.A()); true; catch; false; end; end
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
