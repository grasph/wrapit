using Test

@testset "Wrapper declaration order test" begin
    #It is sufficiant to test that we can load the TestOrder module
    @test begin
        using TestOrder
        true
    end
end

