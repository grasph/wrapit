#!/usr/bin/env julia
import TestPropagation1

using Test

@testset "Test 1 of method_propagation modes" begin
    @test :f_B in names(TestPropagation1, all=true)
    @test !(:f_A in names(TestPropagation1, all=true))
    @test begin; try; TestPropagation1.f_B(TestPropagation1.B(), TestPropagation1.A()); true; catch; false; end; end
end

