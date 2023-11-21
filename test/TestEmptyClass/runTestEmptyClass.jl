using Test
using TestEmptyClass

@testset "Empty Class" begin
    generatedcodedir = "libTestEmptyClass/src/"
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

