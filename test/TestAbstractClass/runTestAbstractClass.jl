using Test
using Serialization

import Pkg
Pkg.activate("$(@__DIR__)/build")
Pkg.develop(path="$(@__DIR__)/build/TestAbstractClass")
using TestAbstractClass

function runtest()
    @testset "Abstract Class" begin
        box = G4Box("box", 10., 10., 10.)
        vec1 = G4ThreeVector(5., 5., 5.)
        vec2 = G4ThreeVector(0., 0., 20.)
        @test Inside(box, vec1)
        @test !Inside(box, vec2)
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
