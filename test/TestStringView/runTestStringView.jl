using Test
using TestStringView

@testset "std::string_view test" begin
    @test f("123") == 3
    @test f() == "Hello"
end

