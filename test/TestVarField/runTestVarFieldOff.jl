using Test
using TestVarFieldOff

@testset "Variable and field accessors switch" begin
    @test :B ∈ names(TestVarFieldOff, all=true)
    @test :C ∉ names(TestVarFieldOff, all=true)
    @test :global_C ∉ names(TestVarFieldOff, all=true)
    @test :global_C! ∉ names(TestVarFieldOff, all=true)
end
