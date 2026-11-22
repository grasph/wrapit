module TestTemplatedParent

export f, f1, f2, f3, f5, f6, real_func, virtual_func

using CxxWrap
@wrapmodule("jlTestTemplatedParent")

function __init__()
    @initcxx
end

end #module
