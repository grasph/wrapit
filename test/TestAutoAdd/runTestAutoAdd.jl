using Test

@testset "Automatic type wrapping" begin
    using TestAutoAdd

    b1 = TestAutoAdd.B1()
    @test isa(TestAutoAdd.f(b1), TestAutoAdd.B2)

    a = TestAutoAdd.A()
    b3 = TestAutoAdd.B3()
    @test isa(f(a, b3), TestAutoAdd.B4)
end

