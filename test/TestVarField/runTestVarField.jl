using Test
using TestVarField

@testset "Variable and field accessors" begin

    a = TestVarField.A()

    @test TestVarField.field_int(a) == 1
    TestVarField.field_int!(a, Int32(3))
    @test TestVarField.field_int(a) == 3

    @test try; TestVarField.field_int_const!(a, Int32(12)); false; catch; true; end
    @test TestVarField.field_int_const(a) == 3
    
    b = field_B(a)
    @test field(b) == 2
    field!(b, 12)
    @test field(field_B(a)) == 12
    
    @test TestVarField.A!static_field() == 1
    TestVarField.A!static_field!(3)
    @test TestVarField.A!static_field() == 3
    
    @test TestVarField.global_int() == 1
    TestVarField.global_int!(3)
    @test TestVarField.global_int() == 3

    a = TestVarField.global_A()
    b = field_B(a)
    @test field(b) == 20
    field!(b, 12)
    @test field(field_B(a)) == 12

    a = TestVarField.global_A_const()
    b = field_B(a)
    @test field(b) == 22
    @test try; TestVarField.field!(b, 12); false; catch; true; end
    @test field(field_B(a)) == 22

    nsa = TestVarField.ns!A()    
    @test TestVarField.field(nsa) == 2
    TestVarField.field!(nsa, Int32(3))
    @test TestVarField.field(nsa) == 3

    @test TestVarField.ns!A!static_field() == 2
    TestVarField.ns!A!static_field!(3)
    @test TestVarField.ns!A!static_field() == 3
    
    @test TestVarField.ns!global_var() == 2
    TestVarField.ns!global_var!(3)
    @test TestVarField.ns!global_var() == 3

end
