using Test
using Serialization

push!(LOAD_PATH, "build")
using TestNoFinalizer

function runtest()
    @testset "NoFinalizer test" begin
        a = TestNoFinalizer.A()
        b = TestNoFinalizer.B()
        c = TestNoFinalizer.C()
   #    d = TestNoFinalizer.D()
   #    e = TestNoFinalizer.E()
        a = b = c = d = e = nothing
        GC.gc()
        @test TestNoFinalizer.A!dtor_ncalls() == 1
        @test TestNoFinalizer.B!dtor_ncalls() == 0
        @test TestNoFinalizer.C!dtor_ncalls() == 0
        @test TestNoFinalizer.D!dtor_ncalls() == 0
        @test TestNoFinalizer.E!dtor_ncalls() == 0
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end

