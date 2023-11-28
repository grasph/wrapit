using Test
using TestStdVector
using CxxWrap

@testset "StdVector test" begin
    v = StdVector(map.(TestStdVector.EltType, 1:10))
    @test mysum(v) == 55
end

