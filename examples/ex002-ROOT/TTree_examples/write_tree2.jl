using ROOT

println("Creating a ROOT file with a TTree filled with  arrays.\n")

f = ROOT.TFile!Open("test2.root", "RECREATE")
tree = ROOT.TTree("tree", "tree")

nMuon::Array{Int32,0} = fill(0)
Muon_pt::Vector{Float32} = Float32[]
Muon_eta::Vector{Float32} = Float32[]
Muon_phi::Vector{Float32} = Float32[]

brCnt = Branch(tree, "nMuon", nMuon, 32000, 99)
brMuon_pt  = Branch(tree, "Muon_pt[nMuon]", Muon_pt, 32000, 99)
brMuon_eta = Branch(tree, "Muon_eta[nMuon]", Muon_eta, 32000, 99)
brMuon_phi = Branch(tree, "Muon_phi[nMuon]", Muon_phi, 32000, 99)

nevts = 10

for i in 1:nevts
    nMuon[] = rand(1:10)

    #resizing the vectors instead of instantiating new ones
    #is expected to reduce number of allocation and runs faster
    resize!.([Muon_pt, Muon_eta, Muon_phi], nMuon[])
    
    Muon_pt .= 100 .* rand(Float32, nMuon[])
    Muon_eta .= 5 .- 10 .* rand(Float32, nMuon[])
    Muon_phi .= 2π * rand(Float32, nMuon[])
    
    #Vectors resizing can invalidate the addresses, need to set them again.
    SetAddress(brMuon_pt, Muon_pt)
    SetAddress(brMuon_eta, Muon_eta)
    SetAddress(brMuon_phi, Muon_phi)

    println("Fill tree with an event containing ", nMuon[], " muons.")
    
    #Note: we expect the use of global variables to hold the references to
    #nMuon, Muon_pt, etc.  ensures that the pointer are kept valid.    
    #For local variables, use GC.@preserve:
    #  GC.@preserve nMuon Muon_pt Muon_eta Muon_phi Muon_mass Fill(tree)
    Fill(tree)
end

println()

Write(tree)
Close(f)
