push!(LOAD_PATH, "build")
using Test

@testset "TestTemplate3" begin
    using TestTemplate3
    #If no exception was thrown by the previous statement, the test has passed
    @test true
end
