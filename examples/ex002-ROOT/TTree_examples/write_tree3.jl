using ROOT
using CxxWrap

println("Creating a ROOT file with a TTree filled with std vectors.\n")

f = ROOT.TFile!Open("test3.root", "RECREATE")
tree = ROOT.TTree("tree", "tree")

Muon_pt = StdVector{Float32}()
Muon_eta = StdVector{Float32}()
Muon_phi = StdVector{Float32}()

brMuon_pt = Branch(tree, "Muon_pt", CxxPtr(Muon_pt), 32000, 99)
brMuon_eta = Branch(tree, "Muon_eta", CxxPtr(Muon_eta), 32000, 99)
brMuon_phi = Branch(tree, "Muon_phi", CxxPtr(Muon_phi), 32000, 99)

nevts = 10

for i in 1:nevts
    nMuon = rand(1:10)
    resize!.([Muon_pt, Muon_eta, Muon_phi], nMuon)
    Muon_pt .= 100 * rand(Float32, nMuon)
    Muon_eta .= 5 .- 10 .* rand(Float32, nMuon)
    Muon_phi .= 2π .* rand(Float32, nMuon)

    println("Fill tree with an event containing ", nMuon[], " muons.")
    
    #Note: we expect the use of global variables to hold the references to
    #Muon_pt, Muon_eta, etc.  ensures that the pointer are kept valid.
    #For local variables, use GC.@preserve:
    #  GC.@preserve Muon_pt Muon_eta Muon_phi Fill(tree)
    Fill(tree)
end
Write(tree)
Close(f)
