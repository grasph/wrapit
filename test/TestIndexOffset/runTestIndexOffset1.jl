# index_offset off

using Test
using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path="$(@__DIR__)/build/TestIndexOffset1")
using TestIndexOffset1

function runtest()
    @testset "Test 1 of method_propagation modes" begin
        a = A(10)
        #set a[i] to i with 0-based indexing
        @test begin setindex!.(Ref(a), 0:9, 1:10); true; end
        #operator[] const returns twice the set value.
        #miss of the factor 2 indicate mistaken call to non-const
        #version
        @test all(getindex.(Ref(a), 0:9) .== 1:10)
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
