# Simple example of WrapIt! usage

This directory contains a very simple example to show how WrapIt! works.

## Code generation and compilation of the shared library

The `CMakeLists.txt` file configures the build process, which in essence is

1. Run the `wrapit` executable over the simple source header `A.h`
    1. The output can be examined in `libHello/src/jlHello.cxx`
    2. The Julia CxxWrap loader module is written to `Hello/src/Hello.jl`
2. Compile the shared library into `lib`

As CMake needs to know the location of CxxWrap, this is best driven from
`build.jl` (use the `Project.toml` file, or any other project where the
`CxxWrap` module is available). This will perform the build in `./build`.

e.g.,

```sh
julia --project=. build.jl
```

## Use of the wrapper

### Shell environment

As the package is not installed in the system and is only in the current
directory, you can add this directory to `LD_LIBRARY_PATH` or
`DYLD_LIBRARY_PATH` (used for the shared library) and `JULIA_LOAD_PATH` (used
for the Julia module) environment variables. You can source the `setup.sh` file
in the build area to perform this.

```sh
cd build
source setup.sh
```

The file [demo_Hello.jl](demo_Hello.jl) reproduced below shows how to use the C++ library from Julia.

```julia
# Import the library
using Hello

# Create an instance of the class A
a = Hello.A("World")

# Call the class A member function
say_hello(a)
```

### Julia script

Instead of the environment modifications above, the `test.jl` script will define
the CxxWrap module internally and run the basic tests, in a self-contained way.

### Build and test

The `runtests.jl` script will build the wrapper and test it.
