using Test
using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path="$(@__DIR__)/build/TestUsingType")
using TestUsingType

function runtest()
    @testset "using type test" begin
        data = G4VMPLData()
        @test physicsVector(data) == C_NULL
        initialize(data)
        @test physicsVector(data) != C_NULL
        pv = physicsVector(data)
        @test physicsVector(data) |> length == 0
        s = MyString("Hello")
        @test str(s) == "Hello"
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
