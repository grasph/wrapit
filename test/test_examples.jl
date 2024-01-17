#!/usr/bin/env julia
using Test

import Pkg

Pkg.activate(; temp=true)

ex_basedir = normpath(joinpath(pwd(), "../examples"))

nprocs=Sys.CPU_THREADS

@testset "ex001-HelloWorld (cmake)" begin
    ex_dir = joinpath(ex_basedir, "ex001-HelloWorld")
    cd(ex_dir)
    run(`./install.jl`)
    @test readchomp(`julia demo_Hello.jl`) == "Hello World!"
end

@testset "ex001-HelloWorld (make)" begin
    ex_dir = joinpath(ex_basedir, "ex001-HelloWorld")
    cd(ex_dir)
    run(`make clean all`)
    @test readchomp(`julia demo_Hello.jl`) == "Hello World!"
end

run_exroot = false
try
    run(`root-config --bindir`)
    global run_exroot = true
catch
    println(stderr, "ROOT software not found. The ROOT example won't be tested.")
end

#FIXME.
#Temporary trick to get the 'make test' command run in an environment
#with CxxWrap installed until. Also a nice way to get them run in
#a clean environment.
ENV["JULIA_PROJECT"]=Pkg.project().path

if run_exroot
    ex = "ex002-ROOT"
    @testset "$ex" begin
        cd(joinpath(ex_basedir, ex))
        run_ex002_ROOT = try
            run(`make clean`);
            run(`make -j $nprocs all`);
            run(`make test`)
            true;
        catch;
            false;
        end
        @test run_ex002_ROOT
    end
end
