#!/usr/bin/env julia
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
tests = [ "TestAccessAndDelete", "TestStdString" ]

@testset verbose=true "Tests" begin
    for t in tests
        @testset "$t" begin
            source_dir = t
            build_dir = joinpath(source_dir, "build")
            test_script = joinpath(source_dir, "test.jl")
            @test begin
                try
                    # Configure and build with CMake
	                run(`cmake -S $source_dir -B $build_dir -DCMAKE_PREFIX_PATH=$cxxwrap_prefix`)
                    run(`cmake --build $build_dir`)
                    # Tests run in Julia
                    include(test_script)
                    true
                catch
                    false
                end
            end
        end
    end
    include("test_examples.jl")
end
