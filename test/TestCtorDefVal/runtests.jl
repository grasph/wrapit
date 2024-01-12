module RunTestCtorDefVal

module TestCtorDefVal

export i_, i_!

using CxxWrap
using Libdl

wrapped_lib_path = joinpath(@__DIR__, "build", "lib")
@wrapmodule(()->joinpath(wrapped_lib_path, "libjlTestCtorDefVal.$(Libdl.dlext)"))

function __init__()
    @initcxx
end

end #module

using Test
using .TestCtorDefVal
using CxxWrap

@testset "Constructor argument default value" begin
    a = TestCtorDefVal.CtorDefVal()
    @test i_(a) == 0
    a = TestCtorDefVal.CtorDefVal(11)
    @test i_(a) == 11
    i_!(a, 10)
    @test i_(a) == 10
end

end
