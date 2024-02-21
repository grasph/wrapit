using Test
using Serialization

push!(LOAD_PATH, "$(@__DIR__)/build")
using TestEmptyClass

function runtest()
    @testset "Empty Class" begin
        generatedcodedir = "$(@__DIR__)/build/libTestEmptyClass/src/"
        generatedcodefiles = readdir(generatedcodedir)
        #Checks that no file was created for vetoed classes:
        @test !any(occursin.("Vetoed", generatedcodefiles))
        #Test that we have at least one file per classes
        nclasses = 6
        @test length(generatedcodefiles) ≥ nclasses
        @test f(TestEmptyClass.A1()) == 1
        @test f(TestEmptyClass.A2()) == 2
        @test f(TestEmptyClass.A3()) == 3
        @test f(TestEmptyClass.A4()) == 4
        @test f(TestEmptyClass.A1()) == 1
        @test f(TestEmptyClass.A6()) == 6
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
