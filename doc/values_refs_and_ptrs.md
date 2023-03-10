# CxxWrap C++/Julia mapping of plain, pointer, and reference types

In this page, is summarised the mapping of C++ function argument and return types to Julia types performed by CxxWrap for struct/class. The provided information is based on CxxWrap version 0.13.3. For older versions refer to [this document version](https://github.com/grasph/wrapit/blob/e20fa248e8513442f696a16d1af64ed060da1481/doc/values_refs_and_ptrs.md).

We will find at the end of the page a [cheat sheet](#Cheat_sheet) for calling a C++ function with arguments which are not in the expected type, between original type and a pointer or reference to it.

## Mapping of function argument types

| C++         | Julia wrapper                                          |
|-------------|--------------------------------------------------------|
| f(A)        | f(::Union{ConstCxxRef{A}, CxxRef{A}, A})               |
| f(const A&) | f(::Union{A, ConstCxxRef{<:A}}, CxxRef{<:A}})          |
| f(A&)       | f(::Union{A, CxxRef{<:A}})                             |
| f(const A*) | f(::Union{Ptr{Nothing}, ConstCxxPtr{<:A}, CxxPtr{<:A}) |
| f(A*)       | f(::Union{Ptr{Nothing}, CxxPtr{<:A}})                  |
| f(shared_ptr<A>) | f(::SmartPointer{A})                              |

## Mapping of function return type

| C++ function return type | Julia wrapper return type and its parent tree     |
|-------------------|----------------------------------------------------------|
| A                 | AAllocated <: A                                          |
| const A*          | ConstCxxPtr{A} <: CxxBaseRef{A} <: Ref{A}                |
| A*                | CxxPtr{A} <: CxxBaseRef{A} <: Ref{A}                     |
| const A&          | ConstCxxRef{A} <: CxxBaseRef{A} <: Ref{A}                |
| A&                | CxxRef{A} <: CxxBaseRef{A} <: Ref{A}                     |
| std::unique_ptr<A>| UniquePtrAllocated{A} <: UniquePtr{A} <: SmartPointer{A} |
| std::shared_ptr<A>| SharedPtrAllocated{A} <: SharedPtr{A} <: SmartPointer{A} |
| std::weak_ptr<A>  | WeakPtrAllocated{A} <: WeakPtr{A} <: SmartPointer{A}     |

## Conversions

A `CxxBaseRef{A}` instance can be converted to an `ADereferenced <: A` with the dereference operator:

`a::CxxBaseRef{A}` ⇒ `a[]::ADereferenced`

An `A` or `CxxBaseRef{A}` instance can be converted to a `CxxPtr{A}` (resp. `ConstCxxPtr{A}`) with the function `CxxPtr` (resp. `ConstCxxPtr`):

`a::A` ⇒ `CxxPtr(a)::CxxPtr{A}`

A `SmartPointer{A}` can be converted to a `CxxRef{A}` with the dereference operator:
`a::SmartPointer{A}` ⇒ a[]::CxxRef{A}

## Cheat sheet <a name="Cheat_sheet"></a>

The following table shows how to call a C++ function wrapped with CxxWrap, when the argument to pass is not of the expected "flavour" (plain type, c++ reference, or c++ pointer).

| Type of variable a | f(A)  | f(const A&) | f(A&)    | f(const A\*)         | f(A\*)          |
|-----------------|----------|-------------|----------|----------------------|-----------------|
| A⁽¹⁾            | f(a)     | f(a)        | f(a)     | f(ConstCxxPtr(a))⁽²⁾ | f(CxxPtr(a))    |
| ConstCxxRef{A}  | f(a)     | f(a)        | f(a[])⁽³⁾| f(ConstCxxPtr(a))⁽²⁾ | f(CxxPtr(a))⁽³⁾ |
| CxxRef{A}       | f(a)     | f(a)        | f(a)     | f(ConstCxxPtr(a))⁽²⁾ | f(CxxPtr(a))    |
| ConstCxxPtr{A}  | f(a[])   | f(a[])      | f(a[])⁽³⁾| f(a)                 | f(CxxPtr(a))⁽³⁾ |
| CxxPtr{A}       | f(a[])   | f(a[])      | f(a[])   | f(a)                 | f(a)            |
| SmartPointer{A} | f(a[])   | f(a[])      | f(a[])   | f(ConstCxxPtr(a[])⁽²⁾| f(CxxPtr(a[])   |

(1) Includes AAllocated and ADerefenced, the concrete types of A.  
(2) Also works with CxxPtr in place of ConstCxxPtr  
(3) Discards the constantness of a.
