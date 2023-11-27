using Test
using TestUsingType

@testset "using type test" begin
    data = G4VMPLData()
    @test physicsVector(data) == C_NULL
    initialize(data)
    @test physicsVector(data) != C_NULL
    pv = physicsVector(data)
    @test physicsVector(data) |> length == 0
end

