using Test
using TestVarFieldOn

@testset "Variable and field accessors" begin

    @test :B ∈ names(TestVarFieldOn, all=true)
    @test :C ∈ names(TestVarFieldOn, all=true)
    @test :global_C ∈ names(TestVarFieldOn, all=true)
    @test :global_C! ∈ names(TestVarFieldOn, all=true)
    
    a = TestVarFieldOn.A()

    @test TestVarFieldOn.field_int(a) == 1
    TestVarFieldOn.field_int!(a, Int32(3))
    @test TestVarFieldOn.field_int(a) == 3

    @test try; TestVarFieldOn.field_int_const!(a, Int32(12)); false; catch; true; end
    @test TestVarFieldOn.field_int_const(a) == 3
    
    b = field_B(a)
    @test field(b) == 2
    field!(b, 12)
    @test field(field_B(a)) == 12
    
    @test TestVarFieldOn.A!static_field() == 1
    TestVarFieldOn.A!static_field!(3)
    @test TestVarFieldOn.A!static_field() == 3
    
    @test TestVarFieldOn.global_int() == 1
    TestVarFieldOn.global_int!(3)
    @test TestVarFieldOn.global_int() == 3

    a = TestVarFieldOn.global_A()
    b = field_B(a)
    @test field(b) == 20
    field!(b, 12)
    @test field(field_B(a)) == 12

    a = TestVarFieldOn.global_A_const()
    b = field_B(a)
    @test field(b) == 22
    @test try; TestVarFieldOn.field!(b, 12); false; catch; true; end
    @test field(field_B(a)) == 22

    nsa = TestVarFieldOn.ns!A()    
    @test TestVarFieldOn.field(nsa) == 2
    TestVarFieldOn.field!(nsa, Int32(3))
    @test TestVarFieldOn.field(nsa) == 3

    @test TestVarFieldOn.ns!A!static_field() == 2
    TestVarFieldOn.ns!A!static_field!(3)
    @test TestVarFieldOn.ns!A!static_field() == 3
    
    @test TestVarFieldOn.ns!global_var() == 2
    TestVarFieldOn.ns!global_var!(3)
    @test TestVarFieldOn.ns!global_var() == 3

end
