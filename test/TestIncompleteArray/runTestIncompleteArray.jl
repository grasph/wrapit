using Test
using TestIncompleteArray

@testset "IncompleteArray test" begin
    @test TestIncompleteArray.f1(1, ["__"]) === nothing
    @test TestIncompleteArray.f2(1, ["__"]) === nothing
end

