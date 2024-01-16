#!/usr/bin/env julia
#
# Bootstrap environment for the first run
using Pkg
Pkg.activate(@__DIR__)
Pkg.instantiate()

using Test
using Serialization
tests = [ "TestSizet", "TestCtorDefVal", "TestAccessAndDelete", "TestNoFinalizer", "TestInheritance",
          "TestPropagation",  "TestTemplate1",  "TestTemplate2", "TestVarField", "TestStdString", "TestStringView",
	  "TestStdVector", "TestOperators", "TestEnum", "TestPointers", "TestEmptyClass", "TestUsingType", "TestNamespace",
          "TestOrder"
        ]
using CxxWrap

cxxwrap_prefix = CxxWrap.prefix_path()

# Temporary override for converted tests
tests = [ "TestAccessAndDelete" , "TestCtorDefVal", "TestStdString", "TestStdVector" ]
#tests = [ "TestStdVector" ]

#Julia module of each test are stored in the build directory:
ENV["JULIA_LOAD_PATH"] = ":build"

@testset verbose=true "Tests" begin
    for t in tests
        @testset "$t" failfast=true begin
            source_dir = t
            build_dir = joinpath(source_dir, "build")
            test_script = lowercasefirst(t) * ".jl"
            @test begin
                try
                    # Configure and build with CMake
	            run(`cmake -S $source_dir -B $build_dir`)
                    run(`cmake --build $build_dir`)
                    true
                catch
                    false
                end
            end
            Test.record(Test.get_testset(), deserialize(open(Cmd(`julia --project=.. $test_script -s`, dir=source_dir))))
        end
    end
    include("test_examples.jl")
end
