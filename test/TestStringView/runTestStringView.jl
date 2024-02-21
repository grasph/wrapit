using Test
using Serialization

push!(LOAD_PATH, "$(@__DIR__)/build")
using TestStringView

function runtest()
    @testset "std::string_view test" begin
        @test f("123") == 3
        @test f() == "Hello"
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
