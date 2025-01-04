## Aliasing

Aliasing is an analogue to c preprocessor define, except it is not preprocessed as text.
When using an alias the referenced expression or typename is expanded inline.
Arguments of inline expressions may be cached, and evaluated only once.

**Example**: type aliasing

```
inline double = float64;
inline float = float32;
inline byte = uint8;
```

**Example**: constant

```
inline pi = 3.14159265359;
inline true = 0 == 0;
inline false = 0 != 0;
```

**Example**: macro

```
inline half(int32 value) = value / 2;
inline min(int32 a, int32 b) = a < b ? a : b;
inline max(float32 a, float32 b) = a > b ? a : b;
```

- the right-hand side can also use local variables if static is not used
- static should force the right side not to use local variables


## Inline the content of a file

There are three ways to inline the content of a file:
absolute paths can be used to specify the exact file in the filesystem (not recommended);
if the path starts with `./` or `../` the path of the inlined file will be relative to the current files path;
otherwise the file will be included from the compiler home folder (like a library).

The inline keyword can be used to inline the content of a file:
* `inline "lib.cmpl";`: inline the content of the file named `lib.cmpl`, from the compilers home folder
* `inline "lib.cmpl";`: the inlined file is optional, no error will be raised if the file does not exist
* `inline "./part.cmpl";`: inline the content of the file named `part.cmpl`, from the same directory of the including file
* `inline "../part.cmpl";`: inline the content of the file named `part.cmpl`, from the parent directory of the including file

## Inline low level operations
When the inline keyword is used as a function, it's arguments will be emitted as instructions / values.

The instructions that can be emitted are exposed only for this method, some examples:
* `inline.sti8` [opcode to store indirect an 8-bit value](#instruction-storei8).
* `inline.i32.mul` [opcode to multiply two 32-bit integers](#instruction-muli32)
* `inline.i64.f32` [opcode to convert int64 to float32](#instruction-i642f32)
* use `-api` [global option flag](#global-options) to list all possible instructions.

The values should be typed wit a cast when emitting them, otherwise a warning will be raised.
In case there is no cat for the required type, 'inline' can be used again, as used in the next example.
The result is not always typed, it can be assigned to a variable, but it requires that
the stack size after the execution of the operation to match the size of the variable.

**Example:**
```
struct Complex {
	float64 re;
	float64 im;
}

inline Complex(Complex copy) = inline(copy);
inline Complex(float64 re, float64 im) = inline(inline(im), inline(re));
inline +(Complex a, Complex b) = Complex(inline(Complex(a), Complex(b), p128.add2d));
inline -(Complex a, Complex b) = Complex(inline(Complex(a), Complex(b), p128.sub2d));

Complex a = Complex(1., 2.);
Complex b = {re: 3, im: 4};
Complex c = a + b;
Complex d = inline(Complex(a), Complex(b), p128.add2d);
```

The last statement (`Complex d = inline(Complex(a), Complex(b), p128.add2d)`) expands to the following instructions:
```
push a          ; copy `a` by value to the stack.
push b          ; copy `b` by value to the stack.
p128.add2d      ; Packed Double-Precision Floating-Point Add(see: sse2 ADDPD instruction)
```

**Example:**
```
float32 sin_pi_2 = inline(float32(3.14/2), float32.sin);
```

Pushes 3.14/2 as floating point and executes the `float32.sin` native function.

**Example:**
```
int32 b = inline(float32(500));
assert(b == 1140457472);
```

Pushes the value as floating point and interprets it as an integer.

**Example:**
```
char a[] = inline(int(3), pointer("string"));
// a: char[]([3] {'s', 't', 'r'})
```

Creates a slice from the string with length 3.

<!-- todo:
* [ ] offset of result can overlap the first parameter, in case it is of value type without destructor


## inline type = typename;
* [x] `inline int = int32;`

## inline constant = expression;
* [x] `inline pi = 3.1415...;`

## inline macro(args) = expression;
* [x] `inline half(int32 value) = value / 2;`

## inline operator(args) = expression;
* [x] `inline +(int32 a, int32 b) = inline(int32(a), int32(b), add.i32);`
* [ ] `inline *(static Arithmetic a,static Arithmetic b) = a.add(b);`

## inline array or object literal
* [ ] `double max = Math.max(inline int[]{1, 2, 3});`

## inline anonymous or closure function
* [ ] `eval(inline int(int x, int y) { return x ^ y; });`

## include file
* [x] `inline "lib.cmpl";`
* [x] `inline "lib.cmpl"?;`
* [x] `inline "./part.cmpl";`
* [ ] `inline "lib.cmpl";`: should inline only once the content of the file, (similar to pragma once in headers)
* [ ] `inline "lib.cmpl"!;`: should force inline the file again, even if it was already included
-->
