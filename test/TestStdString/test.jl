# Test the wrapped library for the wrapit hello example
#

using Test

# Inline definition of the module
module TestStdString

using CxxWrap
using Libdl

export String, getenv, getenvString, getenvstd, getenvstd_1, getenvstd_2, getenvstd_3, getenvstd_std, setenvstd

# Make this work in the source or the build area
wrapped_lib_path = joinpath(@__DIR__, "build", "lib")

@wrapmodule(()->joinpath(wrapped_lib_path, "libjlTestStdString.$(Libdl.dlext)"))

function __init__()
    @initcxx
end

end

# Import the library
using .TestStdString

@testset "StdString test" begin
    varname = "TEST"
    val = "Hello"
    ENV[varname] = val
    v = TestStdString.getenvstd(varname)
    to_string(x) = x == C_NULL ? "" : unsafe_string(x)
    s = to_string(v)
    @test s == val
    newval = val * val
    r = TestStdString.setenvstd(varname, newval)
    @test r == 0
    @test to_string(TestStdString.getenvstd(varname)) == newval

    @test to_string(TestStdString.getenvstd_1(varname)) == newval
    @test to_string(TestStdString.getenvstd_2(varname)) == newval
    @test to_string(TestStdString.getenvstd_3(varname)) == newval
    @test TestStdString.getenvstd_std(varname) == newval
    
    varname_ = TestStdString.String(varname)
    @test TestStdString.getenvString(varname_) == newval
    
end
