using Test
using Serialization

push!(LOAD_PATH, "build")
using TestAccessAndDelete

function runtest()
    @testset "Access and deleted method test" begin
        #Deleted contructors:
        @test_throws MethodError a = AccessDeleteA()
        @test_throws MethodError b = AccessDeleteB()

        #Private constructor:
        @test_throws MethodError b = AccessDeleteB(1)

        a = AccessDeleteA(1)
        b = AccessDeleteB(1, 2)

        #public field
        @test i_(a) == 1

        #following methods are public for the mother class A:
        @test f(a) == 10
        @test g(a) == 11
        @test h(a) == 12

        #this one is private:
        @test_throws UndefVarError e(a)

        #this method is private in the child class B
        #current wrapit version does not support yet access restriction of an inherited method.
        #following test can be use when the support will be added
        #@test_throws UndefVarError g(b)

        #this method is deleted in the child class B
        #current wrapit version does not support yet access restriction of an inherited method.
        #following test can be use when the support will be added
        #@test_thows UndefVarError f(b)

        #this public method is inherited fron the mother class A
        @test h(b) == 12

        #Both C and C(int) are deleted
        @test_throws MethodError c = AccessDeleteC()
        @test_throws MethodError c = AccessDeleteC(1)

        #D is private
        @test_throws MethodError d = AccessDeleteD()
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
