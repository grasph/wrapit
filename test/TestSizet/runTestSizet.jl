using Test
using TestSizet

@testset "std::size_t test" begin
    @test TestSizet.f() == 2
end

