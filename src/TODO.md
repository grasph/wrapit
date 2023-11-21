   - [X] Remove use of std::filesystem not supported on MacOS X 10.4. → use realpath instead of std::filesystem::canonize
   - [X] Pass -resource-dir DIR to the compiler and check that DIR/include contains stddef.h. Emit an error if it doesn't. We need to find out how to determine DIR: from CMake? At runtime from the clang executable using clang -print-resource-dir? Using the same method as described here: https://weliveindetail.github.io/blog/post/2017/07/25/compile-with-clang-at-runtime-simple.html .
   - [] Fix issues shown with test/TestTemplate3 and test/TestTemplate4: see .ref files: fix order of templated type declarations, set ismirrored types for all the involved types, remove erroneous default constructor definition. Note: for template order, prefer to separate the 'apply' from the add_type and call the apply methods in a second block coming after the add_type method block: that way we decorrelate order constraints from template parameters and from class inheritance. 
   - [X] Allow splitting of generation code in multiple files to reduce compilation time and memory footprint. For each class we should generate a wrapper class with a method to declare the type and a method to declare the methods. These classes do not need to be compiled in the same translation unit allowing that way splitting the code into many files. The steering code will instantiate the classes and call the methods in the right order. We can also call the type (resp. method) declaration method of each class in its constructor (resp. destructor).
   - [] Allow specifying by configuration the preferred Julia type name in case a C++ type has several names defined with typedef or using statements
   - [] Add an option to add in the generated code, Julia aliases to map the C/C++ typedefs/using. Need to think how to deal with types involving pointers or references.
   - [] Add a feature to generate automatically the veto list exploiting the test_build feature.
   - [] Disable generation of setter if the assignment operator is deleted or not public.
   - [] Complete support for templates. The current issue is the interpretation of templated type in function argument list which are "unexposed" by libclang.
   - [] Enhance multiple inheritance support: add Julia binding to methods of the extra parents, that are not mapped to Julia supertypes.
   - [] Accessors: generate julia code to map getproperty and setproperty to the accessors
```julia
  function getproperty(a ::x!y, symbol ::Symbol)
  if symbol == :z
    return z(a)
  elseif symbol == :t
    return t(a)
  else
   return getfield(a, symbol)
  end
  ```
   - [] Add support for template functions.
   - [] Simplify in the code the generation of indentations by defining a custom output streamer that handles identation. It needs to provide methods to increment and decrement indentations, keep track of the current indentation size, keep track of ew lines.
   - [] Integration: put in place automatic run of unit tests by the repository, using tests from the test directory
   - [] Integration: put in place packaging and distribution
   - [] Add support to customize some wrappers. Signature to custom wrapper name mapping could be provided in the file as the veto list, no wrapper being a special case. This feature can be useful when we need to further wrap a method in the module Julia code for the purpose of juliafication. 
   - [] With auto veto, if an argument with a missing type has a default value, produce the wrapper using the default value.  
   - Review the return type of the generated accesors, currently return by value for POD and by reference for objects. Should we return by value in both cases? Should it be configurable?
