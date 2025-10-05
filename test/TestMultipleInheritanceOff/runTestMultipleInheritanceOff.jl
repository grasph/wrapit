using Test
using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path="$(@__DIR__)/build/TestMultipleInheritanceOff")
using TestMultipleInheritanceOff

const TestInheritance = TestMultipleInheritanceOff

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

        # Check method inheritance
        # D: defined in the class
        # S: inheritance mapped to supertype
        # I: inheritance mapped with method duplication
        # |      | A1 | A2 |  B | B2 |  D |  E |  F |
        # |------|----|----|----|----|----|----|----|
        # |  f1  |  D |    |  S |  S |    |    |  I |
        # |  f2  |    | D  |  I |  I |    |    |    |
        # |  f3  |    |    |  D |    |    |    |    |
        # |  f5  |    |    |    |    |  D |  S |    |
        # |  f6  |    |    |    |    |    |  D |    |
        # |  f7  |    |    |    |  D |    |    |  D |

        @test let
            funcs = [f1, f2, f3, f5, f6, f7]
            types = [TestInheritance.A1, TestInheritance.A2, TestInheritance.B, TestInheritance.B2,
                     TestInheritance.D, TestInheritance.E, TestInheritance.F]
            #                            A1 A2 B B2  D  E  F
            expected_nmethods = [ #=f1=# 1  0  1  0  0  0  0
                                  #=f2=# 0  1  0  1  0  0  0
                                  #=f3=# 0  0  1  0  0  0  0
                                  #=f5=# 0  0  0  0  1  1  0
                                  #=f6=# 0  0  0  0  0  1  0
                                  #=f7=# 0  0  0  1  0  0  1 ]
            
            #note: nmethods(f, t) = length(methods(f, [t])) to get number of methods
            #      of `f` taking an argument of type `t` does not work. E.g.,
            #      methods(f1, [A2]) returns a method although f1(::A2) is not
            #      defined and f1(A2()) throws an exception.
            #      Therefore we test the existence of the method with `which`.
            function nmethods(f, t)
                try
                    which(f, (t,))
                    1
                catch
                    0
                end
            end
            
            actual_nmethods = nmethods.(funcs, permutedims(types)) # (funcs[i], types[j]) in row i, column j

            # the test:
            expected_nmethods == actual_nmethods
        end
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
