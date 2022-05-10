using CxxWrap
using ROOT
using iROOT


# Defines a TGraph with the points to fit
x = collect(1.:10.); y = map(x -> x^2 * (1 + 0.1*randn()), x)
g = ROOT.TGraph(convert(Int32, length(x)), x, y)

f_jl(x,y) = x[1]^y[1]
f_x = @safe_cfunction(Float64, (ConstCxxPtr{Float64}, ConstCxxPtr{Float64})) do x, p
    f_jl(unsafe_wrap(Vector{Float64}, x.cpp_object, 1), unsafe_wrap(Vector{Float64}, p.cpp_object, 1))
end
tf1 = ROOT.TF1("func", f_x, 0., 10., 1, 1, 0)

#Fit(g, "func")
Fit(g, CxxPtr(tf1))

c = ROOT.TCanvas()
Draw(g, "A*")
