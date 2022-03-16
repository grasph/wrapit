# Simple example of WrapIt! usage

This directory contains a very simple example to show how WrapIt! works. You will find a Jupyter notebook version of these instructions in the [demo_Hello.ipynb](demo_Hello.ipynb) file.

## Code generation and compilation of the shared library

```
make all
```

💡 The above command runs WrapIt! to generate the c++ code with the command:
```
wrapit hello.wit
```
and then it builds the shared library from the generated `jlHello.cxx` file.

## Use of the wrapper.

As the package is not installed in the system and is only in the current directory, you need to add this directory to the LD_LIBRARY_PATH (used for the shared library) and JULIA_LOAD_PATH (used for the Julia module) environment variable. You can source the `setup.sh` files (`setup.csh` if your shell is tcsh or csh) to perform this.

```
source setup.sh
```

The file [demo_Hello.jl](demo_Hello.jl) reproduced below shows how to use the c++ library from Julia.

```julia
#Import the library
using Hello

#Create an instance of the class A
a = Hello.A("World")

#Call the class A member function
say_hello(a)
```
