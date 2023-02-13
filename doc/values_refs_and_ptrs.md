# CxxWrap C++/Julia mapping of plain, pointer, and reference types

In this page, is summarised the mapping of C++ function argument and return types to Julia types performed by CxxWrap for struct/class. The provided information is based on CxxWrap version 0.12.1.

We will find at the end of the page a [cheat sheet](#Cheat_sheet) for calling a C++ function with arguments which are not in the expected type, between original type and a pointer or reference to it.

## Mapping of function argument types

| C++         | Julia wrapper                                                                         |
|-------------|---------------------------------------------------------------------------------------|
| f(A)        | f(::Union{SmartPointer{<:AAllocated}, <:AAllocated}})                                 |
| f(A*)       | f(::Union{Ptr{Nothing}, CxxPtr{<:A}})                                                 |
| f(const A*) | f(::Union{Ptr{Nothing}, ConstCxxPtr{<:A}, CxxPtr{<:A})                                |
| f(A&)       | f(::Union{CxxRef{<:A}, SmartPointer{<:A}, <:A})                                       |
| f(const A&) | f(::Union{ConstCxxRef{<:A}, CxxRef{<:A}, CxxWrap.CxxWrapCore.SmartPointer{<:A}, <:A}) |

## Mapping of function return type

| C++ function return type | Julia wrapper return type and its parent tree              |
|-----------------|--------------------------------------------------|
| A               | AAllocated <: A <: Any                           |
| A*              | CxxPtr{A} <: CxxBaseRef{A} <: Ref{A} <: Any      |
| A&              | CxxRef{A} <: CxxBaseRef{A} <: Ref{A} <: Any      |
| const A*        | ConstCxxPtr{A} <: CxxBaseRef{A} <: Ref{A} <: Any |
| const A&        | ConstCxxRef{A} <: CxxBaseRef{A} <: Ref{A} <: Any |

## Conversions

A `CxxBaseRef{A}` instance can be converted to an `ADereferenced <: A` with the dereference operator:

`a::CxxBaseRef{A}` ⇒ `a[]::ADereferenced`

An `A` or `CxxBaseRef{A}` instance can be converted to a `CxxPtr{A}` (resp. `ConstCxxPtr{A}`) with the function `CxxPtr` (resp. `ConstCxxPtr`):

`a::A` ⇒ `CxxPtr(a)::CxxPtr{A}`

## Cheat sheet <a name="Cheat_sheet"></a>

The following table shows how to call a C++ function wrapped with CxxWrap, when the argument to pass is not of the expected "flavour" (plain type, c++ reference, or c++ pointer).

| Type of variable a      | f(A)      | f(const A&) | f(A&)                         | f(const A*)          | f(A*)           |
|----------------|-----------|-------------|-------------------------------|----------------------|-----------------|
| AAllocated     | f(a)      | f(a)        | f(a)                          | f(CxxPtr(a))         | f(CxxPtr(a))    |
| ADeferenced    | f(A(a))   | f(a)        | f(a)                          | f(CxxPtr(a))         | f(CxxPtr(a))    |
| ConstCxxRef{A} | f(A(a))   | f(a)        | f(A(a))⁽¹⁾ or f(CxxPtr(a))⁽³⁾ | f(ConstCxxPtr(a))⁽²⁾ | f(CxxPtr(a))⁽³⁾ |
| CxxRef{A}      | f(A(a))   | f(a)        | f(a)                          | f(ConstCxxPtr(a))⁽²⁾ | f(CxxPtr(a))    |
| ConstCxxPtr{A} | f(A(a[])) | f(a[])      | f(a[])⁽³⁾                     | f(a)                 | f(CxxPtr(a))⁽³⁾ |
| CxxPtr{A}      | f(A(a[])) | f(a[])      | f(a[])                        | f(a)                 | f(a)            |


(1) Won't be the expected behavior is f is meant to modify a as it will act on a copy and won't modify it.  
(2) Works also with f(CxxPtr(a)).  
(3) Works but discards the constantness of a.
