# Test the wrapped library for the wrapit hello example
#

# Inline definition of the module
module Hello

using CxxWrap
using Libdl

# Make this work in the source or the build area
wrapped_lib_path = @__DIR__
if isdir(joinpath("build", "lib"))
    wrapped_lib_path = joinpath(@__DIR__, "build", "lib")
elseif isdir("lib")
    wrapped_lib_path = joinpath(@__DIR__, "lib")
end

export say_hello
export whisper_hello

@wrapmodule(()->joinpath(wrapped_lib_path, "libjlHello.$(Libdl.dlext)"))

function __init__()
    @initcxx
end

end

# Import the library
using .Hello

# Create an instance of the class A
a = Hello.A("World")

# Call the class A member function
# Print to cout
say_hello(a)

# Return a string
msg = whisper_hello(a)
