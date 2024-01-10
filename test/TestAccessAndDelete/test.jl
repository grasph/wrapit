
module TestAccessAndDelete

export A, B, C, D, f, g, h, i_, i_!

using CxxWrap
using Libdl

wrapped_lib_path = joinpath(@__DIR__, "build", "lib")
@wrapmodule(()->joinpath(wrapped_lib_path, "libjlTestAccessAndDelete.$(Libdl.dlext)"))

function __init__()
    @initcxx
end

end #module

using .TestAccessAndDelete
using Test

@testset "Access and deleted method test" begin
    #Deleted contructors:
    @test_throws MethodError a = A()
    @test_throws MethodError b = B()

    #Private constructor:
    @test_throws MethodError b = B(1)

    a = A(1)
    b = B(1, 2)

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
    @test_throws MethodError c = C()
    @test_throws MethodError c = C(1)

    #D is private
    @test_throws MethodError d = D()
end
