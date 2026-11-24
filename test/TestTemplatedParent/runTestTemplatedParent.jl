using Test
using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path="$(@__DIR__)/build/TestTemplatedParent")
using TestTemplatedParent

function runtest()
    @testset "TemplatedParent test" begin
        c0 = ns2!Child_0param()
        @test (f1(c0, 1.0f0) == 2.0f0
               && typeof(f1(c0, 1.0f0)) == Float32
               && f2(c0) == 2.0
               && typeof(f2(c0)) == Float32
               )
        
        c1 = ns2!Child_1param{Int32}()
        @test (f1(c1, 1.1f0) == 2
               && typeof(f1(c1, 1.0f0)) == Int32
               && f2(c1) == 2.0
               && typeof(f2(c1)) == Float32
               )
        
        c2 = ns2!Child_2params{Int32, Int64}()
        @test (f1(c2, 1) == 2
               && typeof(f1(c2, 1)) == Int32
               && f2(c2) == 2
               && typeof(f2(c2)) == Int64
               )
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
