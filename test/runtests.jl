#!/usr/bin/env julia
#
# Bootstrap environment for the first run
using Pkg
Pkg.activate(@__DIR__)
Pkg.instantiate()

using Test
tests = [ "TestSizet", "TestCtorDefVal", "TestAccessAndDelete", "TestNoFinalizer", "TestInheritance",
          "TestPropagation",  "TestTemplate1",  "TestTemplate2", "TestVarField", "TestStdString", "TestStringView",
	  "TestStdVector", "TestOperators", "TestEnum", "TestPointers", "TestEmptyClass", "TestUsingType", "TestNamespace",
          "TestOrder"
        ]
using CxxWrap

cxxwrap_prefix = CxxWrap.prefix_path()

# tests = [ "TestSizet", "TestCtorDefVal", "TestAccessAndDelete", "TestNoFinalizer", "TestInheritance",
#           "TestPropagation",  "TestTemplate1",  "TestTemplate2", "TestVarField", "TestStdString", "TestStringView",
# 	  "TestStdVector", "TestOperators", "TestEnum", "TestPointers", "TestEmptyClass", "TestUsingType", "TestNamespace"
#         ]
tests = [ "TestAccessAndDelete", "TestCtorDefVal", "TestStdString" ]

@testset verbose=true "Tests" begin
    for t in tests
        @testset "$t" begin
            source_dir = t
            build_dir = joinpath(source_dir, "build")
            test_script = joinpath(source_dir, "runtests.jl")
            @test begin
                try
                    # Configure and build with CMake
	                run(`cmake -S $source_dir -B $build_dir -DCMAKE_PREFIX_PATH=$cxxwrap_prefix`)
                    run(`cmake --build $build_dir`)
                    # Tests run in this Julia instance directly
                    include(test_script)
                    # Alternative test run, using a subprocess
                    # cd(source_dir)
                    # run(`julia --project=.. test.jl`)
                    # cd("..")
                    true
                catch
                    false
                end
            end
        end
    end
    include("test_examples.jl")
end
