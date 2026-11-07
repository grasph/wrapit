using Test
using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path="$(@__DIR__)/build/TestMacroGenFunc")
using TestMacroGenFunc

function runtest()
    @testset "macro-generated function" begin
        a = A()
        @test GetValue(a) == 0
        SetValue(a, 10)
        @test GetValue(a) == 10
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
