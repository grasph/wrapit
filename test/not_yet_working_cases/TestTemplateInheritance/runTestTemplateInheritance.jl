push!(LOAD_PATH, "build")
using Test
using TestTemplateInheritance
using CxxWrap

@testset "Template Inheritance" begin
    a_typedef = TestTemplateInheritance.A_typedef()
    @test f(a_typedef) == 0
    @test isB1(a_typedef)
    a_template = TestTemplateInheritance.A_template()
    @test f(a_template) == 1
    @test isB2(a_template)
    a_typedef_template = TestTemplateInheritance.A_typedef_template()
    @test f(a_typedef_template) == 2
    @test isB2(a_typedef_template)
end
