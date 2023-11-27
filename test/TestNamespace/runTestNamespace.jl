using Test
using TestNamespace
using CxxWrap

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
    @test isa(data2(b), TestNamespace.ns1!ns2!C)
end
