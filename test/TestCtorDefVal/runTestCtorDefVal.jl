module RunTestCtorDefVal

using Test
using TestCtorDefVal
using CxxWrap

@testset "Constructor argument default value" begin
    a = TestCtorDefVal.A()
    @test i_(a) == 0
    a = TestCtorDefVal.A(11)
    @test i_(a) == 11
    i_!(a, 10)
    @test i_(a) == 10
end

end
