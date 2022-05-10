using Test
import TestPropagation3

@testset "Test 3 of method_propagation modes" begin
    @test :f_B in names(TestPropagation3, all=true)
    @test :f_A in names(TestPropagation3, all=true)
    @test begin; try; TestPropagation3.f_B(TestPropagation3.B(), TestPropagation3.A()); true; catch; false; end; end
    @test begin; try; TestPropagation3.f_A(TestPropagation3.A()); true; catch; false; end; end
end

