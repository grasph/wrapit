include("setup.jl")

using ROOT, iROOT
x = collect(1.:10.)
y = x.^2
g = ROOT.TGraph(convert(Int32, length(x)), x, y)
c = ROOT.TCanvas()
Draw(g, "A*")
Fit(g, "pol2")
Draw(g)
#Save the Canvas in an image file
ROOT.SaveAs(c, "demo_TGraph.png")
