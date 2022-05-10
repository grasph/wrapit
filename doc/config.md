# Documentation of `cxxwrapgen` configuration file

The configuration file use the [toml](https://toml.io) syntax.

The list of parameter with their defaut value is provided below.


### Main parameters

```
# Name of the julia module to generate
module_name         = "CxxLib"

# List of input header files containing the classes to wrap
# See also include_dirs
input               = [ ]

# List of directory, where header files must be look for
include_dirs        = [ "." ]

# C++ standard to be used to interpret the header files
# See clang -std= option
# Possible values: "c++11", "c++14", "c++17", and "c++20"
cxx-std             = "c++17"
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
# in the comment of the generated C++ source must be used.
veto_list           = ""

# Mode to generate binding not requested by required
# to define a requested function binding. Possible values:
#   "types": generate binding only for the type (recommended)
#   "methods": generate binding for the type (a struct/class)
#              and its member functions 
propagation_mode    = "types"

# Switch for the genration of field and variable accessors
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
# Name to be used for the file to genarate for the Julia module code.
# If empty, use the module name with .jl extension added
module_jl_fname     = ""

# Name of the seperate file for the module export statement
# If empty, the statement is written directly in the module file
export_jl_fname     = ""

# Mode for the generation of the export statement in
# the julia module code.
# "none": no symbol is exported
# "member_functions": non-static class methods are exported
# "all_functions": all functions (global, static and non-static methods)
export              = "methods"

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
# This feature is to be used for debugging purpose, when we cannot identify
# which line of the generated code is causing an error. A command to build the code
# and possibly test it is run regularly during the code generation process,
# and the generation is stopped it the command exit with a code different than 0.
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
