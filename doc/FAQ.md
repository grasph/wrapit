# WarpIt FAQ

## Installation

### Can I use Julia package manager to install WrapIt.

Yes! Just type `add WrapIt` on the Julia package manager prompt.


### Missing libclangBasic.a file`cmake` error.

_`Cmake` reports the error:_

```
The   imported target "clangBasic" references the file
"/usr/lib/llvm-13/lib/libclangBasic.a"
but this file does not exist.
```

</br>

This error has been seen on Debian based distributions when the libclang-XX-dev package was missing. Install the package, where XX is your LLVM version, 13 in the example above.

## Questions on wrapper generation.

### How to include wrappers for template class?

To generate wrappers for a template class, you need to instantiate each specialization of the class to be wrapped. See test/TestTemplate1 for an example.

### How to include wrappers for template functions?

As for template classes, the specializations needs to be instantiated. Current version of wrapit understands explicit specialization for classes only. The explicit specialization can be replaced by a specialization declaration (same syntax but with the `template<>` prefix keryword instead of solely `template`). It is typically not desirable to have this declaration when the code is compiled. Put the declaration block between two `#ifdef WRAPIT` and `#endif` to make it visible to wrapit only or used the approach taken in [exampes/ex002-ROOT/Templates.h](../exampes/ex002-ROOT/Templates.h). In Wrapit release 1.1.x and below, the macro must be included in the `macro_definitions` list specified in the `.wit` file.

### <a name="custom_method"></a>How to add a wrapper for a class method that does not exist in the C++ version?

Adding a method `m` to a Class `A` in the Julia interface, that does not exist in the C++ version of the code, or that has been vetoed in order to provide a different implementation than the C++ one, can be done by defining two global functions m on the following pattern:

```
return_type m(A& a, args....){
//your custom function implementation
}

return_type m(A* a, args...){
  return m(*a, args...);
}
```

To define a `const` method, add the `const` qualifier to the first argument, `return_type m(const A&a, args...)`, etc. The first version will acts on instance and `CxxRef` and the second on `CxxPtr`: see [values_refs_and_ptrs.md](values_refs_and_ptrs.md) for the details.

The methods must be defined in a header file that will be included in the `input` list of the wrapit configuration file.


### How to customize a class method wrapper?

To customize a class method wrapper, add the method into your veto file in order to disable the generation of the default wrapper, and follow instructions from _[How to add a wrapper for a class method that does not exist in the C++ version?](#custom_method)_ to add the custom wrapper.



## Issues when generating code with wrapit.

### Several types , like std::string, were replaced by `int` in the generated wrapper code, which consequently fails to compile. How can I fix this?

This problem can happen if the C or C++ standard libraray are not found. You should:

* run `wrapit` with `-v 1` option
* Check the reported "Clang options" and note the path that follows `-resource-dir`. The report will look like,
```
Clang options:
	-resource-dir
	/usr/lib/llvm-13/lib/clang/13.0.1
	-x
	c++-header
	-std=c++17
	-I
	.
	-D
	WRAPIT
```

## Issue when using code generated by wrapit.

### Exception "No appropriate factory for type <mangled_name>".

First, use `c++filt -t <mangled_name>` to get the name of the problematic type `T`. Beware of not being misled by mangled_name that looks like a plain name, always demangle it. If the exception occurs when importing the Julia module, see the question "I get an exception when loading the Julia module. How can I find the guilty code?" to find the wrapper declaration that is causing the exception.

The exception indicates that the wrapper for `T` must be declared (with the `add_type()` function) before the declaration triggering the exception. There two options, the wrapper is not declared or is declared too late. You can check order of type declaration in the generation file name jl<module_name>.cxx: the wrapport of type `<type_name>` is declared with a call `newJl<type_name>(jlModule)`.

The order of wrapper declaration lines can be tuned using the `class_order_constraints` configuration parameter. If the wrapper is not declared at all, check the `wrapit` logs to find the reason. If the file, where the type is declared is not included in the configuration, you can try to add it in the `input` or `extra_headers` list.

### I get an exception when loading the Julia module. How can I find the guilty code?

For wrapit release 1.1.x or less, compile the code with the VERBOSE_IMPORT macro defined (`-DVERBOSE_IMPORT` compiler flag). For newer release, it is sufficient to define the `WRAPIT_VERBOSE_IMPORT` environment variable.

### What the `Warning: specialization found at `X` for class/struct `Y` before its definition.` message mean?

It means that wrapit parsed a template specialization before the class template was declared. Check that the header file that declares the class is in your `input` header file list and placed before the header file that declare the specialization.

### Why is the code is randomly crashing?

Crashes can be done by the Julia garbage collector releasing memory allocated for a variable that was already released by the C++ code. If the C++ code is responsible for freeing instances of a wrapped type, you need to include the type in the `veto_finalizer_classes` list specified in the `.wit` configuration file.

