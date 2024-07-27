using Test
using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path="$(@__DIR__)/build/TestStdString")
using TestStdString

function runtest()
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
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end

