using Test
using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path="$(@__DIR__)/build/TestInheritance")
using TestInheritance

function runtest()
    @testset "Inheritance test" begin
        b = TestInheritance.B()
        @test f3(b) == 3
        @test f1(b) == 1
        e = TestInheritance.E()
        @test f6(e) == 6
        @test f5(e) == 5
        #Following call returns a B instance casted to A1*
        b = TestInheritance.new_B()
        #A virtual function, the method of the actual instance must be called
        @test virtual_func(b) == "B::virtual_func()"
        #A non-virtual function: the method of the cast type must be called
        @test real_func(b) == "A1::real_func()"
    
        s = TestInheritance.String("Hello")
        s_with_underscore = f(s)
        @test s_with_underscore == s * "_"

        @test supertype(TestInheritance.F) == Any
        @test f7(TestInheritance.F()) == 7

        @test supertype(TestInheritance.B2) == TestInheritance.A2
        @test f2(TestInheritance.B2()) == 2
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
