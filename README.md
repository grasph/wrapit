# Automatic generation of C++ -- Julia bindings

The goal of this project is the automatization of the generation of [Julia](https://julialang.org) bindings for C++ libraries.

The WrapIt! tool complements the [CxxWrap.jl](https://github.com/JuliaInterop/CxxWrap.jl) package. It generates automatically the c++ wrapper code needed by CxxWrap.jl from the c++ header files of the library to wrap.

Support of generation of code in multiple files (now the default) to reduce time and memory needs for compilation in case of large project. See `n_classes_per_file` in [config.md](../doc/config.md).

🆕 Installation using the Julia package manager and execution of the tool from the Julia REPL: [WrapIt.jl](https://github.com/grasph/WrapIt.jl/).

## Usage

```
wrapit [--force] [-v N] config.toml
```

The file config.toml contains the parameters for the generation, in particular the input header files specified by the `input` (list of file names) and `input_dirs` (list of directories where to look the files for) parameters. The different parameters are described in [config](doc/config.md).

## Features

  * Generation of bindings for struct, class and enums;
  * Generation of bindinges for global and class functions (limited by design to public methods);
  * Map c++ class inheritance to Julia for type conversion, with support of Abstract type (*);
  * Map c++ [] operator to Julia setindex! and getindex functions.

(*) In case of a class inheriting from several classes, only one inheritance is mapped. Choice of the parent class is configurable, with a default that takes the first specified in the c++ class definition.

## Installation

### Pre-built package

WrapIt! is available from the Julia binary repository. Follow the instructions from this [README](https://github.com/grasph/WrapIt.jl/blob/main/README.md) to install the pre-built package using Julia. Note that it does not install the examples and tests, which must be downloaded from this repository.

### Build from source

An alternative to the installation of the pre-built executable is to build it from the source. This project uses [CMake](https://cmake.org) as build tool.

#### Dependencies:

  * Software to be present on the system before running `cmake`:
    * libclang: packages `clang-13` and `libclang-13-dev` on Debian
    * libclang: packages `llvm`, `llvm-devel`, `clang`, `clang-devel` on Alma/Rocky/RHEL
    * On MacOS, you can install `llvm/clang` with brew running the command `brew install llvm` from a terminal
  * Software will be downloaded  by the `cmake` command:
    * [tomlplusplus](https://github.com/marzer/tomlplusplus.git)

#### Build

Build using cmake in the "usual" way. e.g.,

```sh
mkdir build && cd build
cmake -S .. -B . -DCMAKE_INSTALL_PREFIX=/opt
cmake --build .
```

On OS X it is necessary to add the prefix of the clang/llvm installation, `-DCMAKE_PREFIX_PATH=/opt/homebrew/Cellar/llvm/LLVM_VERSION`

The build process will produce the `wrapit` executable. Install it with `cmake --build . --target install`.

## A simple example

A simple example can be found in the example/ex001-Hello directory.

## A more complex example

This project is being conducted in the contex of High energy physics (HEP) research, that used a large software ecosystem, with a lot of code written in C++. We will find in the directory examples/ex002-ROOT an example generating bindings for more than 2000 functions and 80 types from the [ROOT Data analysis framework](http://root.cern.ch).

## Mapping conventions

### Mapping of non-static c++ class methods

Non-static c++ class methods are mapped to Julia methods with taking the class instance `this` pointer as first argument

### Mapping of C++ `::`

The `::` C++ namespace separator is mapped to the `!` character in julia. We have chosen a character allowed in Julia identifers but not in C++ ones. For instance, std::pair is mapped to std!pair. The `!` is also used for class static methods. A `static` method `f()` of a class `A`, `A::f()` in C++ is mapped to A!f().

### Export policy

When importing a Julia module with the `using` directive, the exported symbols land in the current namespace. Other symbols are accessed by prefixing them with the module name. See [Julia documentation](https://docs.julialang.org/en/v1/manual/modules/#namespace-management) for more explanation.

The tool support different options for the list of exported symbol controlled by the `export` parameter  are available: no export, only non-static class methods (the default), export of all functions, export of all functions and all types.

This list is defined in the generated Julia module and can be easily be edited for customization. It can be generated in a dedicated file (specified with the `export_jl_fname` parameter) that can be included with the `include` function in the module code, when not using the one produced by the tool.

## Specification of the type and function to wrap

The tool selects the bindings to produce from the contents of c++ header files provided by the user. Although, the user can write dedicated header files, the tool is designed to allow usage of the existing header files of the library. For this purpose the selection is done based on a set of rules that follows.

   1. Binding are produced for all classes defined directly (\*) in the header files of the list provided by the `input` parameter;
   2. The selection is recursive and binding are produced for all classes with public access of classes  selected for binding;
   3. Binding are produced for all global functions defined directly (*) in these header files;
   4. Binding are produced for all class member functions with a public access including inherited one of the structs and classes defined above;
   5. **Binding are produced for all return and argument types of the bound functions if they are classes**;
   6. Structs are treated the same way as classes;
   7. Entities listed in the veto file (specified by the `veto` parameter) are excluded from the selection (\*\*).

The tool needs the definitions of the types selected by Rule 5 to produce the wrapping code, while the header file may contain only a forward declaration. The tool look for the type definition in all the files of the `input` list and files provided for this purpose in the `extra_header` parameter after resolving the #include chains.

If the `propagation_mode` parameter is set to "methods", then the types selected in Rule 5 are treated as if they were present in one file of the `input` independently if it is the case or not. Bindings will be produced for the public members of these types. As this is recursive it can lead to an inflation of the entity selected for bindings.

For a complex header file tree, the rules may not be sufficient to define the entity to bind using only existing header files. A challenging example is the set of header files of g++ for the c++ standard library.


(\*) Directly means here that the content of the #include files are ignored.
(\*\*) Note that an exact match of the entity signature is required for the veto to work. The signature can be found in comments above the wrap declaration in the generated c++ code before the entity is vetoed.

## Memory management

Information on how releasing of allocated memory is handled can be found in the [CxxWrap](https://github.com/JuliaInterop/CxxWrap.jl) documentation. CxxWrap creates a finalizer that take care of deleted the C++ object when an object is released by the garbage collector. The code genrated by WrapIt disable the finalizer for classes, whose destructor is not public or is deleted. The finalizer can also be disabled on request with the veto_finalizer_classes [configuration](../doc/config.md) parameter.

## C++ pointer and references

CxxWrap maps C++ (const) pointer and reference to Julia types (`Const`)`CxxPtr{T}` and (`Const`)`CxxRef{T}`. More on how to deal with values, pointers, and references can be found in dedicated [page](../doc/values_refs_and_ptrs.md).

## Debugging Tips

   1. In case of wrapping failure, error message when importing the module in Julia, can refer to types using mangled c++ identifier like St6vectorIdSaIdEE. To demangle, the name, use the Unix command `c++filt` with the `-t` option, in this example, `c++-filt -t St6vectorIdSaIdEE` which will reveal the actual type, `std::vector<double, std::allocator<double> >`.
   2. When a type if used by a function, its wrapping should appear before the function in the wrapping c++ code. The tool takes into account this constraint when generating the code. An error message when importing the module about missing type can be due to misordering, for instance after manual edition of the file.
   3. The wrapper can be compiled with the macro 'VERBOSE_IMPORT' to display the import progress. That should help debugging an error occuring when import the Julia module.

## Status and limitations

The code is experimental, but can already produce ready-to-use code. It has several limitations:

  * Support of wrapping of template class is incomplete and typically require manual correction to the code;
  * Wrapping of variadic functions is not supported;

## Technology

The code uses [Clang](https://clang.llvm.org/) from the [LLVM](https://llvm.org/) compiler infrastructure to interpret the C++ code. It uses mainly its high level C API [libclang](https://clang.llvm.org/docs/Tooling.html#libclang). The C++ clang AST library is accessed directly for the required features not exposed by libclang (see `libclang-ex.h` and `libclang-ex.cpp` files). These direct calls aim to be dropped whenever the corresponding features will be added to the libclang API.

