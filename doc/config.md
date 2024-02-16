# Documentation of `cxxwrapgen` configuration file

The configuration file uses the [toml](https://toml.io) syntax.

The list of parameters with their defaut value is provided below.


### Main parameters

```
# Name of the julia module to generate
module_name         = "CxxLib"

# UUID to assign to the generated Julia project. It will
# be copied into the Project.toml file. If empty a new
# uuid is generated.
uuid                 = ""

# version to assign to the generated Julia project. If
# not empty, it will be copied into the Project.toml file
# of the julia project.
version               = ""


# List of input header files containing the classes to wrap
# See also include_dirs
input               = [ ]

# List of directories, where header files must be look for
include_dirs        = [ "." ]

# C++ standard to be used to interpret the header files
# See clang -std= option
# Possible values: "c++11", "c++14", "c++17", and "c++20"
cxx-std             = "c++17"
```

### Extra options to control the generated code organisation

```
# Destination directory for the generated c++ code
# If not set, default to: lib<module_name>/src
# out_cxx_dir = 

# Destination directory for the generated julia package
# If not set, default to: <module_name>. The julia module code
# is written in a src subdirectory and Project.toml file
# at the root of the directory.
# out_jl_dir =

# Number of class wrapper to group in the same source file
# -1: one steering file, one file for class wrappers, one file 
#     for global functions and variables
# 0: all code in a single file
# 1: one file per class (default)
# n, n > 1: n classed per file.
n_classes_per_file = 1

```

### Extra options to control the wrapper generation
   
```
# List of extra header files to look for definition of types,
# whose binding generation was implicitly triggered
# by their use in a function argument list or return value.
# See also include_dirs
extra_headers       = [ ]


# File containing a list of type and functions to exclude from the
# wrapping generation. The exact entity signature that can be found
# in the comment of the generated C++ source must be used. Alternatively,
# one can use a regex surrounded by '/' to exclude signatures based on
# pattern matching.
veto_list           = ""

# List of classes with instances owned by the C++ library
# for which the destructor should not be called by the Julia
# garbage collector. It is necessary to list classed, whose
# destructor is not public or is deleted. These are handled
# automatically.
vetoed_finalizer_classes = []

# Mode to generate binding not requested by required
# to define a requested function binding. Possible values:
#   "types": generate binding only for the type (recommended)
#   "methods": generate binding for the type (a struct/class)
#              and its member functions 
propagation_mode    = "types"

# Switch for the generation of field and variable accessors
fields_and_variables = true

# List of class inheritance mapping specification, to be used for classes
# with multinheritance to specify the inheritance to be mapped to Julia,
# as only single inheritance is supported.
#
# When not specified, the first class of the parent list given in
# the header file defining the class is used.
#
# Format: "Child_Class:Parent_Class"
#         "Child_Class:"
#
# The second form disable the inheritance mapping for the 
# given class.
#
inheritances        = [ "" ]
```

### Extra options for the julia module code generation

```
# Name to be used for the file to generate for the Julia module code.
# If empty, use the module name with .jl extension added
module_jl_fname     = ""

# Name of the seperate file for the module export statement
# If empty, the statement is written directly in the module file
export_jl_fname     = ""

# Base name (i.e without the file extension) of the shared library
# the wrapper code is built into and to load
# If not set, $(@__DIR__)/../deps/lib<module_name>, assuming the
# shared library will be place in a deps subdirectory of the 
# julia project directory specified with the out_jl_dir parameter.
#lib_basename

# Mode for the generation of the export statement in
# the julia module code.
# "none": no symbol is exported
# "member_functions": non-static class methods are exported
# "all_functions": all functions (global, static and non-static methods)
# "all": all symbols including types
export              = "member_functions"

# List of symbols to exclude from the Julia module export list to be used
# to prevent clashes with existing symbol.
# Note: a list of the symbols exported by the Julia Main, Base and Core
# modules which are by default imported in the root namepace can be produced
# with the command,
# julia -e 'println(string.(vcat(names(Main),names(Base),names(Core))))'
export_blacklist    = []
```

### Extra options for the header file code interpretation ([clang](https://clang.llvm.org/docs/UsersManual.html) options)

```
# List of macros to define when intrepreting the header files
macro_definitions   = [ ]

# List of clang features to enable. See the -x option
# of clang
clang_features      = [ ]
```

# List of options to pass to clang when use to parse the header files.
# See the clang command line documentation
clang_opts      = [ ]
```

# Discard methods with no wrapper for one type of its argument type
# or return value
auto_veto = true

### Debugging mode options

```
# Switch to enable code compilation test during the code generation.
# This feature is to be used for debugging purposes, when we cannot identify
# which line of the generated code is causing an error. A command to build the code
# and possibly test it is run regularly during the code generation process,
# and the generation is stopped if the command exit with a code different than 0.
test_build   	    = false

# Command to run when test_build is enabled
build_cmd    	    = "echo No build command defined"

# Prescale of the build frequency for the test_build mode.
# 1 will run a build after almost each code statement
# generation, 2 twice less, etc.
build_every         = 1

# Number of builds to skip when build_cmd is enabled
build_nskips 	    = 0

# Maximum number of built to perform when build_cmd is enabled
build_nmax   	    = -1
```
