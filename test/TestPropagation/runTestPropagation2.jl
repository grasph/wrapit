using Test
using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path="$(@__DIR__)/build/TestPropagation2")
import TestPropagation2

function runtest()
    @testset "Test 2 of method_propagation modes" begin
        @test :f_B in names(TestPropagation2, all=true)
        @test !(:f_A in names(TestPropagation2, all=true))
        @test begin; try; TestPropagation2.f_B(TestPropagation2.B(), TestPropagation2.A()); true; catch; false; end; end
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
