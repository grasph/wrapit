using Test
using Serialization

import Pkg
Pkg.activate(;temp=true)
Pkg.develop(path="$(@__DIR__)/build/TestCtorDefVal")
using TestCtorDefVal

function runtest()
    @testset "Constructor argument default value" begin
        a = TestCtorDefVal.A()
        @test i_(a) == 0
        a = TestCtorDefVal.A(11)
        @test i_(a) == 11
        i_!(a, 10)
        @test i_(a) == 10
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
