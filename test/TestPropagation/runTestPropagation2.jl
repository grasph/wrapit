using Test
import TestPropagation2

@testset "Test 2 of method_propagation modes" begin
    @test :f_B in names(TestPropagation2, all=true)
    @test !(:f_A in names(TestPropagation2, all=true))
    @test begin; try; TestPropagation2.f_B(TestPropagation2.B(), TestPropagation2.A()); true; catch; false; end; end
end


