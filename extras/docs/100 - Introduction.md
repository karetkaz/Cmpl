# Introduction

Designed to be simple and minimalist.

Syntax is similar to c / js influenced by:
* [C/C++](https://en.wikipedia.org/wiki/C_programming_language)
* [JavaScript](https://en.wikipedia.org/wiki/JavaScript)
* [D programming language](https://en.wikipedia.org/wiki/D_programming_language)
* [C# programming language](https://en.wikipedia.org/wiki/C_Sharp_programming_language)
* [Lua programming language](https://en.wikipedia.org/wiki/Lua_programming_language)

## Features

### Types are themselves variables of type `typename`.
* [x] Can be parameters of functions `int sizeof(typename type) { return type.size; }`.
* [ ] Reflection is part of the language (just some internal compiler functions exposed).


### Arrays are more than just typed pointers.
* [x] [Fixed-size arrays](#fixed-size-arrays): `int fixed[30];` (length known by the compiler)
* [x] [Dynamic-size arrays](#dynamic-size-arrays): `int dynamic[] = fixed;` (length known at run-time)
* [x] [Unknown-size arrays](#unknown-size-arrays): `int memcmp(byte a[*], byte b[*], int size);` (length known by developer)
* [ ] [Associative arrays](#associative-arrays): `double constants[string] = {"pi": 3.1415, ...};`


### Enumerations are enumerations:
* [x] Enumeration values are named constants of a given base-type (number, object, function, ...).
* [x] Enumeration type variables can be assigned only with values from the enumeration.
* [ ] Enumeration type is an object: `int field = EnumName.field;`.
* [ ] Enumeration type is an array: `int array[] = EnumName;` or `int field = EnumName[0];`.
* [ ] Enumeration type is a dictionary: `int array[string] = EnumName;` or `int field = EnumName["field"];`.


### Expressions can be aliased with the `inline` keyword.
* [x] aliasing a type: `inline double = float64;`
* [x] aliasing a constant: `inline pi = 3.14159265359;`
* [x] aliasing an expression: `inline min(int a, int b) = a < b ? a : b;`
* [ ] overloading an operator: `inline *(vec4f a, vec4f b) = emit(vec4f(a), vec4f(b), mul.p4f);`
* [x] inline the content of a file: `inline "file.ci";`


### Conditional compile-time code generation and declarations with `static if` statement.
* [x] `static if (int == int32) { /* 32 bit platform code */ }`
* [x] conditional inlining `static if (int == int32) { inline "file"; }`


### Functions
* [x] Extend any class with custom methods using [Uniform Function Call Syntax](https://en.wikipedia.org/wiki/Uniform_Function_Call_Syntax)
* [x] If a function name is a type name, it is treated as a constructor, and should return an instance of that type.


### `new` and `this` are not part of the language.
* [x] Create and initialize objects like in JavaScript `complex a = {re: 42, im: 2};`
* [x] `this` is not needed using [Uniform Function Call Syntax](https://en.wikipedia.org/wiki/Uniform_Function_Call_Syntax).


### `public`, `protected`, `private` access level modifiers are not part of the language.
* [x] everything is accessible inside one file with no restriction.
* [x] accessing a non-document commented variable, function field or method will raise a high-level warning that can be treated as an error.
* [x] deprecation and any other annotations should be specified in the documentation comment.
