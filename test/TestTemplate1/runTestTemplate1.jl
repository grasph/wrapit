using Test
using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path=joinpath(@__DIR__, "build", "TestTemplate1"))
using TestTemplate1

function runtest()
    @testset "Template 1" begin
        a = TestTemplate1.TemplateType{TestTemplate1.P1, TestTemplate1.P2}()
        @test get_first(a) == 2
        @test get_second(a) == 2.5
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
