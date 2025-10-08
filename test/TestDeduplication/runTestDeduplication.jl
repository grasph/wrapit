using Test
using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path="$(@__DIR__)/build/TestDeduplication")
using TestDeduplication

function runtest()
    @testset "Deduplication" begin
        a = A()
        @test f(a) == "const char* f()"
        @test g(a,1) == "const char* g(int64_t)"
        @test A!h(1) == "static const char* h(int64_t)"
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
