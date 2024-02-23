using ROOT

println("Reading back the file created with write_tree1.jl using TTree::GetEntry.\n")

f = ROOT.TFile!Open("test1.root")
f != C_NULL || error("File not found.")

t = ROOT.GetTTree(f[], "tree")
t != C_NULL ||  error("Tree not found!")

a = fill(0)
SetBranchAddress(t[], "a", a)
nevts = GetEntries(t)
for i in 1:nevts
    GetEntry(t, i-1)
    println("Read back value: ", a[])
end
Close(f)

