#Import the module.
# The iROOT module is required for interactive graphic display
# Loading the package trigger a loop that process ROOT graphic events.
using ROOT
using iROOT

# Create a ROOT histogram, fill it with random events, and fit it.
h = ROOT.TH1D("h", "Normal distribution", 100, -5., 5.)
ROOT.FillRandom(h, "gaus")

#Draw the histogram on screen
c = ROOT.TCanvas()
ROOT.Draw(h)

#Fit the histogram wih a normal distribution
ROOT.Fit(h, "gaus")

#Save the Canvas in an image file
ROOT.SaveAs(c, "demo_ROOT.png")

#Save the histogram and the graphic canvas in the demo_ROOT_out.root file.
f = ROOT.TFile!Open("demo_ROOT_out.root", "RECREATE")
ROOT.Write(h)
ROOT.Write(c)
Close(f)
