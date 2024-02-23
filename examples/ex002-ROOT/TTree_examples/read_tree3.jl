using ROOT

println("Reading back the file created with write_tree3.jl using TTreeReader.\n")

f = ROOT.TFile!Open("test3.root")
f != C_NULL || error("File not found.")

reader = ROOT.TTreeReader("tree", f)
Muon_pt_ = ROOT.TTreeReaderArray{Float32}(reader, "Muon_pt")
Muon_eta_ = ROOT.TTreeReaderArray{Float32}(reader, "Muon_eta")
Muon_phi_ = ROOT.TTreeReaderArray{Float32}(reader, "Muon_phi")

ievt = 0
while Next(reader)
    global ievt += 1
    println("Event ", ievt)

    println("Muon multiplicity: ", ROOT.length(Muon_pt_))
    
    Muon_pt::Vector{Float32} = [ Muon_pt_[i-1] for i in 1:ROOT.length(Muon_pt_)]
    println("Muon pt: ", Muon_pt)

    Muon_eta::Vector{Float32} = [ Muon_eta_[i-1] for i in 1:ROOT.length(Muon_eta_)]
    println("Muon eta: ", Muon_eta)

    Muon_phi::Vector{Float32} = [ Muon_phi_[i-1] for i in 1:ROOT.length(Muon_phi_)]
    println("Muon phi: ", Muon_phi)

    println()
end
