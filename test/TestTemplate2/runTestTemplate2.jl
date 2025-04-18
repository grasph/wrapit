using Test
using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path="$(@__DIR__)/build/TestTemplate2")
using TestTemplate2


function runtest()
    @testset "Template 2" begin
        a = TestTemplate2.A{Int32}();
        @test getval(a) == 0
        val = 3
        setval(a, val)
        @test getval(a) == val
        
        b = TestTemplate2.ns!B{Int32}();
        @test getval(b) == 0
        val = 5
        setval(b, val)
        @test getval(b) == val

        @test slen(b, "123") == 3                            
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
