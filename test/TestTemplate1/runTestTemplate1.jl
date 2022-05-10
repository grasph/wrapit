using Test
using TestTemplate1
using CxxWrap

@testset "Template 1" begin
    a = TestTemplate1.TemplateType{TestTemplate1.P1, TestTemplate1.P2}()
    @test get_first(a) == 2
    @test get_second(a) == 2.5
end
