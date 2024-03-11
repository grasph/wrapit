# WrapIt development TODO list

In this file we keep a list of tasks to be done to improve WrapIt.

   - [ ] Fix issues shown with test/TestTemplate3 and test/TestTemplate4: see .ref files: fix order of templated type declarations, set ismirrored types for all the involved types, remove erroneous default constructor definition. 
   
      > **Note**: for template order, prefer to separate the 'apply' from the add_type and call the apply methods in a second block coming after the add_type method block: that way we decorrelate order constraints from template parameters and from class inheritance. 
   - [ ] More generally complete support for templates. The current issue is the interpretation of templated types in function argument list which are "unexposed" by libclang (is it still true?).
   - [ ] Allow specifying by configuration the preferred Julia type name in case a C++ type has several names defined with typedef or using statements
   - [ ] Add an option to add in the generated code, Julia aliases to map the C/C++ typedefs/using. Some thought on how to deal with types involving pointers or references needed.
   - [ ] Add a feature to generate automatically the veto list exploiting the test_build feature.
   - [ ] Enhance multiple inheritance support: add Julia binding to methods of the extra parents, that are not mapped to Julia supertypes.
   - [ ] Accessors: generate julia code to map getproperty and setproperty to the accessors
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
   - [ ] Simplify the code that handles generation of indentations by defining a custom output streamer that handles identation. It needs to provide methods to increment and decrement indentations, keep track of the current indentation size, keep track of ew lines.
   - [ ] Integration: put in place automatic run of unit tests by the repository, using tests from the test directory
   - [ ] Add support to customize some wrappers. Signature to custom wrapper name mapping could be provided in the file as the veto list, no wrapper being a special case. This feature can be useful when we need to further wrap a method in the module Julia code for the purpose of "juliafication". 
   - [ ] In the autoveto mode, if an argument with a missing type has a default value, produce the wrapper using this value.
   - [ ] Review the return type of the generated accessors, currently return by value for POD and by reference for objects. Should we return by value in both cases? Should it be configurable?
   - [ ] Generate and maintains a project database, in the form of an editable toml file, with the list of entity to wrap. The database should allow enabling/disabling the wrapping for each type, function and variables (accessors). It will replace and extend the veto file. This should ease both customization and upgrade to a new wrapit release, that support more cases and generate wrappers for type or functions previously auto vetoed.
   - [ ] Add support to generate Julia docstring from the doxygen documentation found in the code. Support of documentation written in the source files instead of parsed headers (ROOT is in this case).
