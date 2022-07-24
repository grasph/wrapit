#!/usr/bin/env julia
using Test
tests = ["TestCtorDefVal"  "TestInheritance"  "TestPropagation"  "TestTemplate1"  "TestTemplate2"  "TestVarField"]

@testset verbose=true "Tests" begin
    for t in tests
        @testset "$t" begin
            @test begin
                try
	            run(`make -C $t clean test`)
                    true
                catch
                    false
                end
            end
        end
    end
    include("test_examples.jl")
end
