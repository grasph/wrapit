# Simple example of WrapIt! usage

This directory contains a very simple example to show how WrapIt! works.

## Code generation and compilation of the shared library

To compile the code and install the julia package, run `julia install.jl`. Use the `--project` julia command line option to install the package in a different environment than your default one.

For illustration, both configuration file for `cmake` (`CMakeLists.txt`) and plain `make` (`Makefile`) are provided. The `install.jl` script uses the first option.

The build process will perform two steps:

1. Running the `wrapit` executable over the simple source header `A.h`
    1. The output can be examined in `libHello/src/jlHello.cxx`
    2. The Julia CxxWrap loader module is written to `Hello/src/Hello.jl`
2. Compilation of the shared library into `lib`

💡 If you modify the code, you don't need to install the package again, because the `install.jl` script install the package in the `dev` mode. Just rebuild the library runing `make` in the build directory. Make sure to restart julia after this, unless you use `Revise.jl`.

## Use of the wrapper

The file [demo_Hello.jl](demo_Hello.jl) reproduced below shows how to use the C++ library from Julia.

```julia
# Import the library
using Hello

# Create an instance of the class A
a = Hello.A("World")

# Call the class A member function
say_hello(a)
```

⚠ If you specified a project directory when running `install.jl`, you need to specify the same directory when running `demo_Hello.jl` in order for the script to find the installed `Hello` package.

