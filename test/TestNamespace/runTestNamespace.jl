using Test

using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path="$(@__DIR__)/build/TestNamespace")
using TestNamespace

using CxxWrap

function runtest()
    @testset "Namespaces" begin
        @test begin; c = TestNamespace.ns1!ns2!C(); true; end
        @test TestNamespace.ns1!ns2!global_var() == 2
        a = TestNamespace.ns1!ns2!A{Cint}()
        @test getval(a) == 0
        a_value = 3
        setval(a, a_value)
        @test getval(a) == a_value
        @test begin ns1!ns2!g(a); true; end
    
        b = TestNamespace.ns1!ns2!B()
        b_data1_value = 0
        b_data1 = getval ∘ data1
        @test b_data1(b) == b_data1_value
        f(b, a) #f sets a to b.data1
        @test getval(a) == b_data1_value
        @test isa(data2(b)[], TestNamespace.ns1!ns2!C)

        @test b |> data2 |> i  == 0  #b.data2.i is equal to 0
        c = CxxRef(TestNamespace.ns1!ns2!C())
        set(c, Int32(12))
        @test c |> i == 12

        #Set c to b.data2 (whose i field value is 0)
        g_ref(b, c)
        @test c |> i == 0

        set(c, 12)
        
        #this will set c to b.data2 (whose i field value is 0)
        g_ptr(b, CxxPtr(c))
        @test c |> i == 0

        set(c, 12)

        #this will set c to b.data2 (whose i field value is 0)
        g_ptrofptr(b, CxxPtr(c))
        @test c |> i == 0

        set(c, 12)

        #this will set c to b.data2 (whose i field value is 0)
        g_arrofptr(b, CxxPtr(c))
        @test c |> i == 0

    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
