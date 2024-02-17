using Test
using Serialization

push!(LOAD_PATH, "$(@__DIR__)/build")
using TestUsingType

function runtest()
    @testset "using type test" begin
        data = G4VMPLData()
        @test physicsVector(data) == C_NULL
        initialize(data)
        @test physicsVector(data) != C_NULL
        pv = physicsVector(data)
        @test physicsVector(data) |> length == 0
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
