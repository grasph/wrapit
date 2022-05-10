using Test
using TestTemplate2
using CxxWrap

@testset "Template 2" begin
    a = TestTemplate2.A{Int32}();
    @test getval(a) == 0
    val = 3
    setval(a, val)
    @test getval(a) == val
end
