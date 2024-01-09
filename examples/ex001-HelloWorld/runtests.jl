using Test

using Pkg
Pkg.activate(".")
Pkg.instantiate()

@testset "ex001-HelloWorld" begin
    @test begin
        include("build.jl")
    end
    @test begin
        include("test.jl") == "Hello World!"
    end
end
