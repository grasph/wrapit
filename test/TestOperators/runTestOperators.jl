using Test
using TestOperators

# Test an operator executing exp.
# Test a mapped C++ operator
# `object` is the instance of the test class
# to use. Use an instance of A (resp. C) to test
# member (resp. non-member) operators.
# `expectedname` is the expected operator name returned
# by lastcall(`object`). See `A.h` file for more details.
macro optest(object, expectedname, exp)
    quote
        $(esc(exp))
        @test last_call($(esc(object)))[] == $(esc(expectedname))
    end
end


@testset "Operators" begin
    a, b = TestOperators.A(), TestOperators.A()
    c, d = TestOperators.C(), TestOperators.C()

    #Test of member prefix operators
    @optest a "member prefix +" +a
    @optest a "member prefix -" -a
    @optest a "member prefix *" a[]
    @optest a "member prefix !" !a
    @optest a "member prefix ++" inc!(a)
    @optest a "member prefix --" dec!(a)
    @optest a "member ->" arrow(a)

    #Test of non-member prefix operators
    @optest c "non-member prefix +" +c
    @optest c "non-member prefix -" -c
    @optest c "non-member prefix *" c[]
    @optest c "non-member prefix !" !c
    @optest c "non-member prefix ++" inc!(c)
    @optest c "non-member prefix --" dec!(c)
    
    #Test of member postfix operators
    dummy = 0
    @optest a "member postfix ++" inc!(a, dummy)
    @optest a "member postfix --" dec!(a, dummy)

    #Test of non-member postfix operators
    dummy = 0
    @optest c "non-member postfix ++" inc!(c, dummy)

    #Test of member infix operators
    @optest a "member infix +" (a + b)
    @optest a "member infix -" (a - b)
    @optest a "member infix /" (a / b)
    @optest a "member infix *" (a * b)
    @optest a "member infix &" (a & b)
    @optest a "member infix |" (a | b)
    @optest a "member infix ^" (a ⊻ b)
    @optest a "member infix ^" xor(a,b)
    @optest a "member infix &&" logicaland(a, b)
    @optest a "member infix ||" logicalor(a,b)
    @optest a "member infix >" (a > b)
    @optest a "member infix <" (a < b)
    @optest a "member infix >=" (a ≥ b)
    @optest a "member infix <=" (a ≤ b)
    @optest a "member infix ==" (a == b)
    @optest a "member infix !=" (a != b)
    @optest a "member infix <=>" cmp(a,b)
    @optest a "member infix +=" add!(a, b)
    @optest a "member infix -=" sub!(a, b)
    @optest a "member infix /=" fdiv!(a, b)
    @optest a "member infix *=" mult!(a, b)
    @optest a "member infix %=" rem!(a, b)
    @optest a "member infix ^=" xor!(a, b)
    @optest a "member infix &=" and!(a, b)
    @optest a "member infix |=" or!(a, b)
    memberptr = p(a)
    @optest a "member ->*" arrowstar(a, memberptr)

    #Test of non-member infix operators
    @optest c "non-member infix +" (c + d)
    @optest c "non-member infix -" (c - d)
    @optest c "non-member infix /" (c / d)
    @optest c "non-member infix *" (c * d)
    @optest c "non-member infix &" (c & d)
    @optest c "non-member infix |" (c | d)
    @optest c "non-member infix ^" (c ⊻ d)
    @optest c "non-member infix ^" xor(c, d)
    @optest c "non-member infix &&" logicaland(c, d)
    @optest c "non-member infix ||" logicalor(c, d)
    @optest c "non-member infix >" (c > d)
    @optest c "non-member infix <" (c < d)
    @optest c "non-member infix >=" (c ≥ d)
    @optest c "non-member infix <=" (c ≤ d)
    @optest c "non-member infix ==" (c == d)
    @optest c "non-member infix !=" (c != d)
    @optest c "non-member infix <=>" cmp(c,d)
    @optest c "non-member infix +=" add!(c, d)
    @optest c "non-member infix -=" sub!(c, d)
    @optest c "non-member infix /=" fdiv!(c, d)
    @optest c "non-member infix *=" mult!(c, d)
    @optest c "non-member infix %=" rem!(c, d)
    @optest c "non-member infix ^=" xor!(c, d)
    @optest c "non-member infix &=" and!(c, d)
    @optest c "non-member infix |=" or!(c, d)    
end

