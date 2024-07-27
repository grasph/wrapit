using Test
using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path="$(@__DIR__)/build/TestStdVector")
using TestStdVector

using CxxWrap: StdVector

function runtest()
    @testset "StdVector test" begin
        v1 = StdVector(TestStdVector.EltType1.(1:10))
        @test mysum(v1) == 55

        v2 = StdVector(TestStdVector.EltType2_.(1:10))
        @test mysum(v2) == 55

        v3 = StdVector(TestStdVector.EltType3_.(1:10))
        @test mysum(v3) == 55

        #following does not work due to a CxxWrap limitation
        #v4_ = StdVector(CxxPtr.(EltType4_.(1:10)))
        #@test mysum(v4) == 55

        #following does not work due to a CxxWrap limitation
        #v5_ = StdVector(CxxPtr.(EltType5_.(1:10)))
        #@test mysum(v5) == 55

        v6 = f6()
        @test all(v6 .== [1, 2])

        v7 = f7()
        @test all(v7 .== [1, 2])

        a = TestStdVector.A()
        @test all(f8(a) .== ["a", "b"])
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
