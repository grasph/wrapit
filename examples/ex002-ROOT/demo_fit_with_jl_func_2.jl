include("setup.jl")

#Import the module.
# The iROOT module is required for interactive graphic display
# Loading the package trigger a loop that process ROOT graphic events.
using ROOT
using iROOT

# Help module that provides more transparent support of fit of TH1
# and TGraph with a julia function.
using ROOTex

# Defines a TGraph with the points to fit
x = collect(1.:10.); y = map(x -> x^2 * (1 + 0.1*randn()), x)
g = TGraphW(convert(Int32, length(x)), x, y)

# Function for the fit
f_jl(x,y) = x[1]^y[1]

println("\nFit a TGraph with a function defined in Julia.")
ROOT.Fit(f_jl, g, "", "", 0., 10., 1, 1, 0)

#   embedded in ROOT TF1
println("Define a TF1 from a Julia function")
tf1 = ROOT.TF1(f_jl, "func", 0., 10., 1, 1, 0)

println("\nRedo the fit using the TF1 function refered by its name.")
Fit(g, "func")

println("\nRedo the fit using the TF1 function refered by its instance.")
Fit(g, tf1)


c = ROOT.TCanvas()
Draw(g, "A*")
