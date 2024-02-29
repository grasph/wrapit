using Test
using TestBlackBox
using CxxWrap

@testset "BlackBox" begin
    a = TestBlackBox.get1(4)
    @test TestBlackBox.val(a) == 4

    b = TestBlackBox.get2(5)
    @test TestBlackBox.val(b) == 5

    a = TestBlackBox.A{Int32}(6)
    @test val(a) == 6

    a = TestBlackBox.A{TestBlackBox.BlackBox}(TestBlackBox.get1(7))
    @test TestBlackBox.val(val(a)) == 7

end
