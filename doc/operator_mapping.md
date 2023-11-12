# WraptIt! C++ operator mapping

WrapIt! maps the overloaded C++ operators are mapped to similar Julia `Base` module operators when available. When not possible the C++ operator is mapped to a function according to the table below. Mapping of both member and non-member operator is supported. We can refer to the operator test script [`test/TestOperators/runTestOperators`](../TestOperators/runTestOperators) for an usage example.

Note 2: the mapping is not final and could change in the future for some of the operators.  

| C++ operator                            | Julia operator or function | From Base | C++ ⇒ Julia Example               |
|-----------------------------------------|----------------------------|-----------|-----------------------------------|
| operator +()                            | +                          | ✓         | +a ⇒ +a                           |
| operator -()                            | -                          | ✓         | -a ⇒ -a                           |
| operator *()                            | []                         | ✓         | *a ⇒ a[]                          |
| operatr !()                             | !                          | ✓         | !a ⇒ !a                           |
| operator +(const T& b)                  | +                          | ✓         | a + b ⇒ a + b                     |
| operator -(const T& b)                  | -                          | ✓         | a - b ⇒ a - b                     |
| operator /(const T& b)                  | /                          | ✓         | a / b ⇒ a / b                     |
| operator *(const T& b)                  | *                          | ✓         | a * b ⇒ a * b                     |
| operator &(const T& b)                  | &                          | ✓         | a & b⇒ a & b                      |
| operator \|(const T& b)                 | \|                         | ✓         | a \| b ⇒ a \| b                   |
| operator ^(const T& b)                  | ^                          | ✓         | a ^ b ⇒ a ⊻ b, alt. a xor b       |
| operator <(const T& b)                  | <                          | ✓         | a < b ⇒ a < b)                    |
| operator >=(const T& b)                 | >=                         | ✓         | a >=  b ⇒ a >= b, alt. a ≥ b)     |
| operator <=(const T& b)                 | <=                         | ✓         | a <= b ⇒ a <=, alt. a ≤ b)        |
| operator ==(const T& b)                 | ==                         | ✓         | a == b ⇒ a == b                   |
| operator !=(const T& b)                 | !=                         | ✓         | a != b ⇒ a != b                   |
| operator <=>(const T& b)[¹](#footnote1) | cmp!                       |           | a <=> b ⇒ cmp(a,b)                |
| operator ++()                           | inc!                       |           | ++a ⇒ inc!(a)                     |
| operator --()                           | dec!                       |           | --a ⇒ dec!(a)                     |
| operator ++(int)[²](#footnote2)         | inc!                       |           | a++ ⇒ inc!(a, 0)                  |
| operator --(int)                        | dec!                       |           | --a ⇒ dec!(a, 0)                  |
| operator ->()                           | arrow                      |           | a->c ⇒ c(arrow(a))[³](#footnote3) |
| operator &&(const T& b)                 | logicaland                 |           | a && b ⇒ logicaland(a, b)         |
| operator \|\|(const T& b)               | logicalor                  |           | a \|\| b ⇒ logicalor(a,b)         |
| operator +=(const T& b)                 | add!                       |           | a += b ⇒ add!(a, b)               |
| operator -=(const T& b)                 | sub!                       |           | a -= b ⇒ sub!(a, b)               |
| operator /=(const T& b)                 | fdiv!                      |           | a /= b ⇒ fdiv!(a, b)              |
| operator *=(const T& b)                 | mult!                      |           | a *= b ⇒ mult!(a, b)              |
| operator %=(const T& b)                 | rem!                       |           | a %= b ⇒ rem!(a, b)               |
| operator ^=(const T& b)                 | xor!                       |           | a ^= b ⇒ xor!(a, b)               |
| operator &=(const T& b)                 | and!                       |           | a &= b ⇒ and!(a, b)               |
| operator \|=(const T& b)                | or!                        |           | a \|= b ⇒ or!(a, b)               |
| operator ->*(const T& b)                | arrowstar                  |           | a ->*b ⇒ arrowstar(a,b)           |

---
<a name="footnote1">¹</a> C++20 and above.

<a name="footnote2">²</a> According to the C++ specifications, the second argument is
a dummy argument used for the function signature only. Define a julia function
e.g., `getandinc!(a) = inc!(a,0)`, to hide the dummy argument. Note that in
general it is not needed to map this operator into Julia, as is just a C++-style
shortcut for operations that can be done using the `+` or `+=` operator.</a>

<a name="footnote3">³</a> In C++ `a->c` is expanded to `a.operator->()->c`. The
mapped function calls the `operator->()` function only. The C++ behaviour can be
emulated by adding in the Julia module the function definition, `→(object, member) = member(arrow(object))`.
