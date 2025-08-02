using Test
using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path="$(@__DIR__)/build/TestFuncPtr")
using TestFuncPtr
import CxxWrap: @safe_cfunction

function runtest()
    @testset "FuncPtr" begin
        #Test autoveto of function returing a function pointer
        @test :getg ∉ names(TestFuncPtr)
        @test :getf ∉ names(TestFuncPtr)
        h(x) = 2*x
        h_c = @safe_cfunction(f, Int32, (Int32,))
        i = Int32(2)
        @test TestFuncPtr.apply(h_c, i) == h(i)
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
