using Test
push!(LOAD_PATH, "build")
@testset "Template 4" begin
    import CxxWrap
    import TestTemplate4
    @test begin
    s = TestTemplate4.std!set{CxxWrap.StdLib.StdString,TestTemplate4.std!less{CxxWrap.StdLib.StdString}, TestTemplate4.std!allocator{CxxWrap.StdLib.StdString}}()
    TestTemplate4.f(s) == 0
    end
end
