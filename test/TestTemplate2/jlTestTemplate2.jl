module jlTestTemplate2

import Base.copy

using CxxWrap
@wrapmodule("jlTestTemplate2")

function __init__()
    @initcxx
end

end #module
