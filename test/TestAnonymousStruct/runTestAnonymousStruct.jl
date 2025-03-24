using Test
using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path="$(@__DIR__)/build/TestAnonymousStruct")
using TestAnonymousStruct

function runtest()
    @testset "AnonymousStructs" begin
        @test_throws UndefVarError TestAnonymousStruct.s_global
        @test_throws UndefVarError TestAnonymousStruct.s_private
        @test_throws UndefVarError TestAnonymousStruct.s_public
        @test_throws UndefVarError TestAnonymousStruct.i
        @test_throws UndefVarError TestAnonymousStruct.j
        s = TestAnonymousStruct.A()
        @test s |> s_public_named |> k == 5;
        @test names(TestAnonymousStruct) ==  [:A, :A!S_public_named_type, :TestAnonymousStruct, :k, :k!, :s_public_named, :s_public_named!]
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
