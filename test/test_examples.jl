#!/usr/bin/env julia
using Test

ex_basedir = normpath(joinpath(pwd(), "../examples"))

@testset "ex001-HelloWorld" begin
ex_dir = joinpath(ex_basedir, "ex001-HelloWorld")
cd(ex_dir)
run(`make clean all`)
    @test readchomp(`/bin/sh -c '. ./setup.sh && julia demo_Hello.jl'`) == "Hello World!"
end

run_exroot = false
try
    run(`root-config --bindir`)
    global run_exroot = true
catch
    println(stderr, "ROOT software not found. The ROOT example won't be tested.")
end

if run_exroot
    ex = "ex002-ROOT"
    @testset "$ex" begin
        cd(joinpath(ex_basedir, ex))
        run_ex002_ROOT = try
            run(`make clean test`);
            true;
        catch;
            false;
        end
        @test run_ex002_ROOT
    end
end
