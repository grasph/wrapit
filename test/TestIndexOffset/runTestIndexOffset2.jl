# index_offset on

using Test
using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path="$(@__DIR__)/build/TestIndexOffset2")
using TestIndexOffset2

function runtest()
    @testset "Test 2 of method_propagation modes" begin
        a = A(10)
        #set a[i] to i with 1-based index
        @test begin setindex!.(Ref(a), 1:10, 1:10); true; end
        #operator[] const returns twice the set value.
        #miss of the factor 2 indicate mistaken call to non-const
        #version
        @test all(getindex.(Ref(a), 1:10) .== 1:10)
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
