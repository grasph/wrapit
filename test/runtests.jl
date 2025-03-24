#!/usr/bin/env julia
#
using Test
using Serialization
tests = [ "TestSizet", "TestCtorDefVal", "TestAccessAndDelete", "TestNoFinalizer", "TestInheritance",
          "TestPropagation",  "TestTemplate1",  "TestTemplate2", "TestVarField", "TestStdString", "TestStringView",
	  "TestStdVector", "TestOperators", "TestEnum", "TestPointers", "TestEmptyClass", "TestUsingType", "TestNamespace",
          "TestOrder", "TestAutoAdd", "TestAbstractClass", "TestAnonymousStruct"
          ]

# Switch to test examples
testExamples = true

# List of test can also be specified on the command line
if length(ARGS) > 0
    tests = replace.(filter(x->x ≠ "examples", ARGS), r"/$" => "")
    if length(ARGS) == length(tests)
        testExamples = false
    end
end

# Number of CPU cores to use to compile the code
ncores=Sys.CPU_THREADS

# Clean the module load path for spwaneed julia process to reduce test dependency
# on the installation the test is run on (remove "@#.#")
ENV["LOAD_PATH"] = "@:@stdlib"

@testset verbose=true "Tests" begin
    for t in tests
        @testset "$t" begin
            source_dir = t
            build_dir = joinpath(source_dir, "build")
            test_script = "run" * t * ".jl"
            @test begin
                try
                    if isfile(joinpath(source_dir, "CMakeLists.txt"))
                        println("Try to run cmake...")
                        #cmake based build
                        # Configure and build with CMake
                        run(`rm -rf "$build_dir"`)
	                run(`cmake -S "$source_dir" -B "$build_dir"`)
                        run(`cmake --build "$build_dir" -t clean`)
                        run(`cmake --build "$build_dir" -j $ncores`)
                    else
                        println("source_dir:", source_dir)
                        #assumes plain make build
                        run(`make -C "$source_dir" clean`)
                        run(`make -C "$source_dir" -j $ncores`)
                    end
                    true
                catch
                    false
                end
            end
            Test.record(Test.get_testset(), deserialize(open(Cmd(`julia --project=.. $test_script -s`, dir=source_dir))))
        end
    end
    testExamples && include("test_examples.jl")
end
