using Test
using Serialization

push!(LOAD_PATH, "build")
using TestSizet

function runtest()
    @testset "std::size_t test" begin
        @test TestSizet.f() == 2
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
