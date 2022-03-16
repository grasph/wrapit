module iROOT

using ROOT

function _event_loop()
system = ROOT.GetSystem()
while true
    ProcessEvents(system)
    yield()
end
end

function __init__()
    ROOT.ROOT!EnableThreadSafety()
    ROOT.Delete(ROOT.TCanvas())
    ROOT.SetBatch(ROOT.ROOT!GetROOT(), false)
    schedule(Task(_event_loop))
    nothing
end


end #module
