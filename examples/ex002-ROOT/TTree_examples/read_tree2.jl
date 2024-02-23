using UnROOT

println("Reading back the file created with write_tree2.jl using LazyTree of UnROOT.\n")

t = LazyTree("test2.root", "tree")
display(t)
