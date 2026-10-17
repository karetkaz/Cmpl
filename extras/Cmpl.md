
# Introduction
Designed to be simple and minimalist.

Syntax is similar to c / js influenced by:
- [C/C++](https://en.wikipedia.org/wiki/C_programming_language)
- [JavaScript](https://en.wikipedia.org/wiki/JavaScript)
- [D programming language](https://en.wikipedia.org/wiki/D_programming_language)
- [C# programming language](https://en.wikipedia.org/wiki/C_Sharp_programming_language)
- [Lua programming language](https://en.wikipedia.org/wiki/Lua_programming_language)

## Features

### Types are themselves variables of type `typename`.
- [x] Can be parameters of functions `int sizeof(typename type) { return type.size; }`.
- [ ] Reflection is part of the language (just some internal compiler functions exposed).

### Arrays are more than just typed pointers.
- [x] [Fixed-size arrays](#fixed-size-arrays): `int fixed[30];` (length known by the compiler)
- [x] [Dynamic-size arrays](#dynamic-size-arrays): `int dynamic[] = fixed;` (length known at runtime)
- [x] [Unknown-size arrays](#unknown-size-arrays): `int memcmp(uint8 a[*], uint8 b[*], int size);` (length known by the developer)
- [ ] Associative arrays: `double constants[string] = {"pi": 3.1415, ...};`

### Enumerations are enumerations:
- [x] Enumeration values are named constants of a given base type (number, object, function, ...).
- [x] Enumeration type variables can be assigned only with values from the enumeration.
- [ ] Enumeration type is an object: `int field = EnumName.field;`.
- [ ] Enumeration type is an array: `int array[] = EnumName;` or `int field = EnumName[0];`.
- [ ] Enumeration type is a dictionary: `int array[string] = EnumName;` or `int field = EnumName["field"];`.

### Expressions can be aliased with the `inline` keyword.
- [x] aliasing a type: `inline double = float64;`
- [x] aliasing a constant: `inline pi = 3.14159265359;`
- [x] aliasing an expression: `inline min(int a, int b) = a < b ? a : b;`
- [x] overloading operators: `inline *(vec4f a, vec4f b) = inline(vec4f(a), vec4f(b), p128.mul4f);`
- [x] inline the content of a file: `inline "./file.cmpl";`

### Conditional compile-time code generation and declarations with `static if` statement.
- [x] platform specific code: `static if (int == int32) { ... }` is compiled only on 32-bit platforms
- [x] conditional inlining `static if (int == int32) { inline "file"; }`

### Functions
- [x] Extend any class with custom methods using [Uniform Function Call Syntax](https://en.wikipedia.org/wiki/Uniform_Function_Call_Syntax)
- [x] If a function name is a type name, it is treated as a constructor, and should return an instance of that type.

### `new` and `this` are not part of the language.
- [x] Create and initialize objects like in JavaScript `complex a = {re: 42, im: 2};`
- [x] `this` is not needed using [Uniform Function Call Syntax](https://en.wikipedia.org/wiki/Uniform_Function_Call_Syntax).

### No access level modifiers like: `public`, `protected`, `private`, ...
- [x] accessing an undocumented symbol from a different translation unit will raise a high-level warning that can be treated as an error.
- [x] deprecation and any other annotations should be specified in the documentation comment.

# Lexical structure

## Comments

Line comments start with `//` and run to the end of the line.

Block comments are delimited by `/* ... */`.

A doc-comment is a line comment starting with an extra `/` (`///`) or a block comment starting with an extra `*` that isn't immediately closed (`/** ... */`).

## Identifiers
Identifiers are named references of types, variables, and functions.

An identifier starts with a letter or underscore,
followed by any number of letters, digits, or underscores.

## Keywords
Keywords are reserved words, which cannot be used as identifiers:
- break
- continue
- else
- enum
- for
- if
- inline
- return
- static
- struct

## Literals
[Literals](https://en.wikipedia.org/wiki/Literal_(computer_programming)) are compile-time constant values.

### Integer and floating-point literals
Integers can be represented in decimal, binary, octal, or hexadecimal format,
while floating-point literals can be specified only in decimal format.

#### Number prefixes
The following prefixes may be used to specify the radix of an integer:
- binary: `0b` or `0B`
- octal: `0o` or `0O`
- hexadecimal: `0x` or `0X`

#### Number suffixes
The following suffixes are available to override the type of the value:
- `d`: results in an int32 constant, ex: `inline i32Value = 3d;`
- `D`: results in an int64 constant, ex: `inline i64Value = 3D;`
- `u`: results in a uint32 constant, ex: `inline u32Value = 3u;`
- `U`: results in a uint64 constant, ex: `inline u64Value = 3U;`
- `f`: results in a float32 constant, ex: `inline f32Value = 3f;`
- `F`: results in a float64 constant, ex: `inline f64Value = 3F;`

**Example**

```cmpl
int32 lexLiteralsFromBinary = 0b101;
assertEq(5, lexLiteralsFromBinary);

int32 lexLiteralsFromOctal = 0o17;
assertEq(15, lexLiteralsFromOctal);

int32 lexLiteralsFromHex = 0x1F;
assertEq(31, lexLiteralsFromHex);

inline lexLiteralsI32 = 3d;
inline lexLiteralsI64 = 3D;
inline lexLiteralsU32 = 3u;
inline lexLiteralsU64 = 3U;
inline lexLiteralsF32 = 3f;
inline lexLiteralsF64 = 3F;

assert(struct(lexLiteralsI32) == int32);
assert(struct(lexLiteralsI64) == int64);
assert(struct(lexLiteralsU32) == uint32);
assert(struct(lexLiteralsU64) == uint64);
assert(struct(lexLiteralsF32) == float32);
assert(struct(lexLiteralsF64) == float64);
```

### Character and string literals

#### Escape sequences
Escape sequences are used to define certain special characters within string literals.
The following escape sequences are available:
- `\'`: single quote
- `\"`: double quote
- `\?`: question mark
- `\\`: backslash
- `\a`: audible bell
- `\b`: backspace
- `\f`: form feed - new page
- `\n`: line feed - new line
- `\r`: carriage return
- `\t`: horizontal tab
- `\v`: vertical tab
- `\xnn`: arbitrary hexadecimal value, always 2 characters.
- `\<new line>`: continues the string on the next line.

#### Multi-line strings
A string literal is multi-line if it starts with a backslash followed by a newline.

**Example**

```cmpl
// character and string literal escape sequences

char lexStringsQuote = '\'';
assertEq(39, lexStringsQuote);

char lexStringsNewline = '\n';
assertEq(10, lexStringsNewline);

char lexStringsHex = '\x41';
assertEq('A', lexStringsHex);

// a string literal starting with `\` immediately followed by a newline is multi-line
char lexStringsMultiline[] = "\
<p>
	Hello
</p>
";
assert(lexStringsMultiline.length > 0);
```

## Operators
Operator symbols: `!`, `~`, `-`, `+`, `&`, `()`, `[]`, `.`, `*`, `/`, `%`, `|`, `^`, `<<`, `>>`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `||`, `&&`, `?:`, `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`, `...`, `,`.
The `?`, `!` and `&` symbols are also used as reference suffixes in declarations (`int32 x& = y;`),
and `^` marks a method that overrides a base method (`int32 read^(...)`).
The `:` separates members from their values in literals (`{x: 1}`) and the base type of a record or enumeration (`struct Point: object`).
See the Expressions chapter for their meaning and precedence.

# Expressions
Expressions are responsible for computing values.

## Unary expressions
Unary expressions are composed of a unary operator and one operand.
- `+`: unary plus. Returns the operand promoted to its default type.
- `-`: unary minus. Changes the sign of a number.
- `~`: complement. Valid for integer types.
- `!`: logical not. Results in `true` or `false`.
- `&`: explicitly passes the operand by reference, matching a parameter declared with a trailing `&`.

**Example**

```cmpl
// unary expressions

int32 exprUnaryA = 5;
assertEq(5, +exprUnaryA);   // unary plus: promotes to the operand's default type
assertEq(-5, -exprUnaryA);  // unary minus: changes the sign
assertEq(~5, ~exprUnaryA);  // complement: valid for integer types
assert(!false);             // logical not

// `&` in front of a call argument explicitly passes it by reference,
// matching a parameter declared with a trailing `&`
void exprUnarySet(int32 out&) {
	out = 42;
}
int32 exprUnaryTarget = 0;
exprUnarySet(&exprUnaryTarget);
assertEq(42, exprUnaryTarget);
```

## Binary expressions
Binary expressions are composed of a binary operator and two operands.

### Arithmetic expressions
Used to compute a new value from the two operands.
- `+`, `-`: additive operators.
- `*`, `/`, `%`: multiplicative operators.

### Bitwise expressions
May be used on integer types only.
- `&`, `|`, `^`: bitwise operators.
- `<<`, `>>`: bit shift operators.

### Relational expressions
Compare two operands, checking if one is less or greater than the other.
The result is a boolean value (`true` or `false`).

### Equality expressions
Check if the left and right operands are equal or not.
The result is a boolean value (`true` or `false`).

**Example**

```cmpl
// binary expressions: arithmetic, bitwise, relational and equality

int32 exprBinaryI = 7;
int32 exprBinaryJ = 2;

// arithmetic
assertEq(9, exprBinaryI + exprBinaryJ);
assertEq(5, exprBinaryI - exprBinaryJ);
assertEq(14, exprBinaryI * exprBinaryJ);
assertEq(3, exprBinaryI / exprBinaryJ);
assertEq(1, exprBinaryI % exprBinaryJ);

// bitwise: valid on integer types only
assertEq(2, exprBinaryI & exprBinaryJ);
assertEq(7, exprBinaryI | exprBinaryJ);
assertEq(5, exprBinaryI ^ exprBinaryJ);
assertEq(28, exprBinaryI << exprBinaryJ);
assertEq(1, exprBinaryI >> exprBinaryJ);

// relational
assert(exprBinaryJ < exprBinaryI);
assert(exprBinaryJ <= exprBinaryI);
assert(exprBinaryI > exprBinaryJ);
assert(exprBinaryI >= exprBinaryJ);

// equality
assert(exprBinaryI == 7);
assert(exprBinaryI != exprBinaryJ);
```

## Assignment expressions
This operator assigns the value on the right to the operand on the left side.
Compound operators are expanded; `a += 3 + b` is converted to `a = a + (3 + b)`.

**Example**

```cmpl
// assignment expressions: compound forms are expanded into a plain assignment

int32 exprAssignA = 0;
int32 exprAssignB = 3;

exprAssignA = 5;
assertEq(5, exprAssignA);

exprAssignA += 3 + exprAssignB;   // expanded to: exprAssignA = exprAssignA + (3 + exprAssignB)
assertEq(11, exprAssignA);
```

## Logical expressions
Logical operators evaluate their operands from left to right and return a boolean value.
The evaluation stops as soon as the result is known.
- `||`: logical or. Returns `true` if **any** of its operands is `true`; stops at the first `true`.
- `&&`: logical and. Returns `true` if **all** of its operands are `true`; stops at the first `false`.

## Ternary expression
The `?:` (conditional) operator is the only ternary operator, with 3 operands:
- test operand: based on its value, the second or the third operand is evaluated.
- true operand: evaluated and returned only if the test operand is `true`.
- false operand: evaluated and returned only if the test operand is `false`.

**Example**

```cmpl
// logical expressions: evaluated left to right, stopping as soon as the result is known

static int exprLogicalCalls = 0;
bool exprLogicalTrue() {
	exprLogicalCalls += 1;
	return true;
}
bool exprLogicalFalse() {
	exprLogicalCalls += 1;
	return false;
}

// `||` returns true if any operand is true; evaluation stops at the first `true`
exprLogicalCalls = 0;
bool exprLogicalOr = exprLogicalFalse() || exprLogicalTrue() || exprLogicalTrue();
assert(exprLogicalOr);
assertEq(2, exprLogicalCalls);   // the third operand is never evaluated

// `&&` returns true only if all operands are true; evaluation stops at the first `false`
exprLogicalCalls = 0;
bool exprLogicalAnd = exprLogicalTrue() && exprLogicalFalse() && exprLogicalTrue();
assert(!exprLogicalAnd);
assertEq(2, exprLogicalCalls);   // the third operand is never evaluated

// ternary (conditional) expression: `test ? whenTrue : whenFalse`
int32 exprTernaryResult = exprLogicalTrue() ? 1 : 2;
assertEq(1, exprTernaryResult);
```

## Primary expressions
- `child.name` — member access.
- `values[10]` — array access (subscript).
- `values[...end]` — prefix slice: 0-based, `end` exclusive.
- `values[start...end]` — slice: `start` inclusive, `end` exclusive.
- `...values` — spread: expands an array as the arguments passed to a variadic parameter.

**Example**

```cmpl
// member access, array access, array slicing, and spread

struct ExprSlicePoint {
	int32 x;
	int32 y;
}
ExprSlicePoint exprSliceP = {x: 3, y: 4};
assertEq(3, exprSliceP.x);   // member access

int32 exprSliceValues[5] = {10, 20, 30, 40, 50};
assertEq(30, exprSliceValues[2]);   // array access (subscript)

// `a[...end]`: prefix slice, 0-based, end exclusive
int32 exprSlicePrefix[] = exprSliceValues[...3];
assertEq(3, exprSlicePrefix.length);
assertEq(10, exprSlicePrefix[0]);

// `a[start...end]`: start inclusive, end exclusive
int32 exprSliceMiddle[] = exprSliceValues[1 ... 3];
assertEq(2, exprSliceMiddle.length);
assertEq(20, exprSliceMiddle[0]);

int32 exprSliceSum(int32 values...) {
	int32 total = 0;
	for (int32 i = 0; i < values.length; i += 1) {
		total += values[i];
	}
	return total;
}
// `...values`: spread an array as the arguments of a variadic parameter
assertEq(150, exprSliceSum(...exprSliceValues));
```

## Operator precedence

Operator precedence determines grouping, when parentheses are omitted: multiplication binds tighter than addition,
so `2 + 3 * 4` groups as `2 + (3 * 4)`, not `(2 + 3) * 4`.

| Operator                               | Title                    | Example                   | Associates    |
|----------------------------------------|--------------------------|---------------------------|---------------|
| 15: Primary                            |                          |                           | left to right |
| ( )                                    | Function call            | sqrt(x)                   |               |
| [ ]                                    | Array subscript          | values[10]                |               |
| .                                      | Member access            | child.name                |               |
| 14: Unary                              |                          |                           | right to left |
| +                                      | Unary plus               | +a                        |               |
| -                                      | Unary minus              | -a                        |               |
| ~                                      | Complement               | ~0o77                     |               |
| !                                      | Logical not              | !ready                    |               |
| &                                      | Pass to mutate           | update(&value)            |               |
| 13: Multiplicative                     |                          |                           | left to right |
| *                                      | Multiplication           | i * j                     |               |
| /                                      | Division                 | i / j                     |               |
| %                                      | Modulus                  | i % j                     |               |
| 12: Additive                           |                          |                           | left to right |
| +                                      | Addition                 | value + i                 |               |
| -                                      | Subtraction              | x - 10                    |               |
| 11: Bit shift                          |                          |                           | left to right |
| <<                                     | Left shift               | byte << 4                 |               |
| \>>                                    | Right shift              | i >> 2                    |               |
| 10: Relational                         |                          |                           | left to right |
| <                                      | Less than                | i < 10                    |               |
| <=                                     | Less than or equal to    | i <= j                    |               |
| \>                                     | Greater than             | i > 0                     |               |
| \>=                                    | Greater than or equal to | count >= 90               |               |
| 9: Equality                            |                          |                           | left to right |
| ==                                     | Equal to                 | result == 0               |               |
| !=                                     | Not equal to             | c != EOF                  |               |
| 8: Bitwise AND                         |                          |                           | left to right |
| &                                      | Bitwise AND              | word & 0o77               |               |
| 7: Bitwise XOR                         |                          |                           | left to right |
| ^                                      | Bitwise XOR              | word1 ^ word2             |               |
| 6: Bitwise OR                          |                          |                           | left to right |
| &#124;                                 | Bitwise OR               | word &#124; bits          |               |
| 5: Logical AND                         |                          |                           | left to right |
| &&                                     | Logical AND              | j > 0 && j < 10           |               |
| 4: Logical OR                          |                          |                           | left to right |
| &#124;&#124;                           | Logical OR               | i > 80 &#124;&#124; ready |               |
| 3: Conditional                         |                          |                           | right to left |
| ?:                                     | Conditional operator     | a > b ? a : b             |               |
| 2: Assignment                          |                          |                           | right to left |
| = *= /= %= += -= &= ^= &#124;= <<= >>= | Assignment operators     |                           |               |
| ...                                    | Slice range / spread     | a[1 ... 3], f(...args)    |               |
| 1: Collection                          |                          |                           | left to right |
| ,                                      | Comma operator           | i = 10, j = 0             |               |

**Example**

```cmpl
assert(2 + 3 * 4 == 14);
```

## Inline low level operations
When `inline` is used as a function, its arguments are emitted as raw instructions and values.
- `sti8` — opcode to store an 8-bit value indirectly.
- `i32.mul` — opcode to multiply two 32-bit integers.
- `i64.i32` — opcode to convert an int64 to an int32.

Use `-api` to list all available instructions.

Values should be typed with a cast when emitting them, otherwise a warning is raised.
When there is no cast for the required type, `inline` can be used again to convert it.
The result is not always typed; it can be assigned to a variable, but the stack size after the
operation must match the size of the destination variable.

**Example**

```cmpl
// inline low-level operations: emit raw VM instructions / values

struct DeclInlineComplex {
	float64 re;
	float64 im;
}

inline DeclInlineComplex(DeclInlineComplex copy) = inline(copy);
inline DeclInlineComplex(float64 re, float64 im) = inline(inline(im), inline(re));
inline +(DeclInlineComplex a, DeclInlineComplex b) = DeclInlineComplex(inline(DeclInlineComplex(a), DeclInlineComplex(b), p128.add2d));

DeclInlineComplex declInlineA = DeclInlineComplex(1., 2.);
DeclInlineComplex declInlineB = { re: 3, im: 4 };
DeclInlineComplex declInlineC = declInlineA + declInlineB;
assertEq(4.0, declInlineC.re);
assertEq(6.0, declInlineC.im);

// pushes 3.14/2 as a float and executes the `float32.sin` native function
float32 declInlineSin = inline(float32(3.14 / 2), float32.sin);
assert(declInlineSin > 0.999);

// pushes the value as a float and reinterprets its bits as an integer
int32 declInlineBits = inline(float32(500));
assertEq(1140457472, declInlineBits);

// creates a slice from the string with the given length
char declInlineSlice[] = inline(int(3), pointer("string"));
assertEq(3, declInlineSlice.length);
assertEq('s', declInlineSlice[0]);
```

# Statements
Statements are the basic blocks of a program.

## Block statement
Block statement groups zero or more statements as a single statement.

**Example**

```cmpl
// block statement: groups zero or more statements into a single statement

int stmtBlockValue = 0;
{
	int step = 3;
	stmtBlockValue += step;
	stmtBlockValue += step;
}
assertEq(6, stmtBlockValue);
```

## If statement
The if statement is a selection statement that can be used to execute a section of code only if a condition is met.
The main purpose of the if statement is to handle exceptional cases in the control flow.

An optional `else` branch is executed when the condition is not met.
Conditions can be chained with `else if`: the conditions are tested in order,
and only the branch of the first condition that is met is executed.

**Example**

```cmpl
if (Float64.sin(Float64.pi / 2) == 1) {
	debug("The sine of pi / 2 is one");
}

// the `else` branch is executed when the condition is not met
int32 stmtIfValue = 7;
bool stmtIfEven = false;
if (stmtIfValue % 2 == 0) {
	stmtIfEven = true;
} else {
	stmtIfEven = false;
}
assert(!stmtIfEven);

// `else if` chains conditions: only the first branch whose condition is met is executed
char stmtIfSign[*] = null;
if (stmtIfValue < 0) {
	stmtIfSign = "negative";
} else if (stmtIfValue == 0) {
	stmtIfSign = "zero";
} else {
	stmtIfSign = "positive";
}
assertEq('p', stmtIfSign[0]);
```

### Declaration in the condition
The condition may be preceded by an optional declaration, separated by a semicolon.
The declared variable is visible in the condition and in the `true` branch of the statement.
If only the declaration is given, its value converted to boolean is used as the condition:
`if (float64 x = someMethod())` is equivalent to `if (float64 x = someMethod(); bool(x))`.

**Example**

```cmpl
if (int32 one = Float64.sin(Float64.pi / 2); one == 1) {
	debug("The sine of pi / 2 is one", one);
}
```

### Static if statement
The static if construct can be used as a compile-time check.
- if the condition evaluates to `true`:
  - the declarations contained by the block will be exposed to the outer scope.
- if the condition evaluates to `false`:
  - declarations contained by the block will be exposed only to the inner scope.
  - statements inside the scope will be type-checked.
  - compilation errors will be reported as warnings.
  - inlined files will not be processed.
  - code generation will be skipped.

**Example**

```cmpl
static if (struct(double) == null) {
	debug("type `double` is not defined, define it as float64");
	inline double = float64;
} else {
	debug("type `double` was already defined");
}

{
	debug("double is ready for use as", double);
	double x = Float64.pi;
}
```

### Platform specific code
A `static if` can select code for the target platform at compile time:
the branches of the other platforms are type-checked, but no code is generated for them.

The `System.Platform` constants describe the build target of the compiler and virtual machine
(the platform `cmpl` was compiled for), not the machine it actually runs on:
- `System.Platform.WebAssembly`, `Windows`, `MacOS`, `Linux`, `Unix`: the pointer size in bits
  (32 or 64) of the `cmpl` process when it was built for the given platform, `0` otherwise.
- `System.Platform.X86`, `Arm`: the pointer size in bits (32 or 64) of the `cmpl` process
  when it was built for the given architecture, `0` otherwise (`X86 == 64` is x86_64, `Arm == 64` is aarch64).
- `System.Platform.pointerSize`: the size of a native pointer of the `cmpl` process in bytes (4 or 8),
  also the default packing of records, see [Memory layout](#memory-layout).
- `System.Platform.littleEndian`: `1` if the least significant byte is stored first in memory, `0` otherwise.

The `System.Compiler` constants describe the compiler itself:
- `System.Compiler.version`: the build date of the compiler as `yyyymmdd`.
- `System.Compiler.runsInTestMode`: `1` when compiling in test mode, `0` otherwise, enabled with the `-X+test` flag
  (ex: windows are closed right after they are shown).
- `System.Compiler.preferNativeCalls`: `1` if native functions should be used when available, `0` otherwise,
  instead of their implementation in Cmpl (disabled with the `-X-native` flag).

**Example**

```cmpl
static if (System.Platform.WebAssembly != 0) {
	debug("WebAssembly platform");
}
static if (System.Platform.Windows != 0) {
	debug("Windows platform");
}
static if (System.Platform.MacOS != 0) {
	debug("macOS platform");
}
static if (System.Platform.Linux != 0) {
	debug("Linux platform");
}
static if (System.Platform.X86 != 0) {
	debug("x86 architecture");
}
static if (System.Platform.Arm != 0) {
	debug("arm architecture");
}
```

## For statement
The for statement is a repetition statement that executes a section of code repeatedly, while a condition holds.

The init and step clauses are both optional.
- `for (init; test; step)` — the classic three-clause form.
- `for (init; test)` — no step clause.
- `for (test)` — only the condition, similar to a `while` loop in other languages.

**Example**

```cmpl
// for statement: classic forms (init; test; step)

int stmtForSum = 0;
for (int i = 0; i < 5; i += 1) {
	stmtForSum += i;
}
assertEq(10, stmtForSum);

// the step clause is optional
int stmtForTotal = 0;
for (int i = 0; i < 4; ) {
	stmtForTotal += i;
	i += 1;
}
assertEq(6, stmtForTotal);

// both the init and the step clauses are optional
int stmtForN = 5;
int stmtForHalved = 0;
for (stmtForN != 0) {
	stmtForN = stmtForN / 2;
	stmtForHalved += 1;
}
assertEq(3, stmtForHalved);
```

## Break statement
The break statement terminates the execution of the innermost enclosing loop.

**Example**

```cmpl
// break statement: terminates the innermost enclosing loop

int stmtBreakLastSeen = -1;
for (int i = 0; i < 7; i += 1) {
	if (i > 2) {
		break;
	}
	stmtBreakLastSeen = i;
}
assertEq(2, stmtBreakLastSeen);
```

## Continue statement
The continue statement terminates the current and begins the next iteration of the innermost enclosing loop.

**Example**

```cmpl
// continue statement: skips to the next iteration of the innermost enclosing loop

int stmtContinueSeen = 0;
for (int i = 0; i < 7; i += 1) {
	if (i < 2) {
		continue;
	}
	stmtContinueSeen += 1;
}
assertEq(5, stmtContinueSeen);
```

## Return statement
The return statement terminates the execution of the current function.

If an expression is given, it is assigned to the function's result parameter before returning.
It is allowed to `return` an expression of type `void` (e.g. invoking a void function) even from a void-returning function — the expression is evaluated, but nothing is returned.

**Example**

```cmpl
// return statement: assigns its expression into the function's result, then exits the function

int stmtReturnAddOne(int x) {
	return x + 1;
}
assertEq(6, stmtReturnAddOne(5));

void stmtReturnNoop() {
	return;
}
stmtReturnNoop();

// a `void`-returning function may `return` an expression of type `void`
// (e.g. invoking another void function): it is evaluated, but nothing is returned
static int stmtReturnSideEffects = 0;
void stmtReturnBump() {
	stmtReturnSideEffects += 1;
}
void stmtReturnBumpAndExit() {
	return stmtReturnBump();
}
stmtReturnBumpAndExit();
assertEq(1, stmtReturnSideEffects);
```

## Inline the content of a file
There are three ways to inline the content of a file:
- an absolute path specifies the exact file in the filesystem (not recommended).
- a path starting with `./` or `../` is resolved relative to the current file's directory.
- otherwise, the file is included from the compiler's home folder, like a library.

A trailing `?` makes a missing file a no-op instead of an error.

**Example**

```cmpl
// inline "./file.cmpl" pulls in the content of a file, resolved relative to the
// current file's directory since it starts with "./"
inline "./test.inline.file.included.cmpl";
assertEq(42, declInlineFileValue);

// the trailing "?" makes a missing file a no-op instead of an error
inline "./test.inline.file.doesNotExist.cmpl"?;
```

## Declaration statement
A declaration (variable, function, type, alias) is itself a valid statement.

A `static` local declaration is initialized only once, when the program starts,
and keeps its value across subsequent calls — this is a different concept from a `static if` or `static for`.

**Example**

```cmpl
// declaration statement: `static` on a local declaration initializes it once,
// when the program starts, and keeps its value across calls

int stmtDeclCallCount() {
	static int calls = 0;
	calls += 1;
	return calls;
}
assertEq(1, stmtDeclCallCount());
assertEq(2, stmtDeclCallCount());
assertEq(3, stmtDeclCallCount());
```

## Expression statement
An expression statement is an expression terminated with `;`.

Only assignment (`a = 2;`, including compound forms like `a += 3;`, which are expanded to a plain assignment)
and invocation (`foo();`) expressions may form a statement.
Any other expression statement, e.g. `a * 4;`, is reported as a warning, which can be treated as an error.

**Example**

```cmpl
// expression statement: only assignment and invocation expressions are valid statements

int stmtExprCounter = 0;
stmtExprCounter = 5;    // assignment
stmtExprCounter += 2;   // compound assignment is expanded to `stmtExprCounter = stmtExprCounter + 2`
assertEq(7, stmtExprCounter);

int stmtExprDoubled(int x) {
	return x * 2;
}
stmtExprDoubled(stmtExprCounter);   // invocation; its result may be discarded
assertEq(7, stmtExprCounter);

// stmtExprCounter * 2;   // compilation warning: not a valid statement (neither assignment nor invocation)
```

# Declarations
Declaration adds new variables, functions or types to the program.

## Variables
A [variable](https://en.wikipedia.org/wiki/Variable_(computer_science)) is a symbolic name
associated with a value, which may be changed at runtime.

### Static variables
Variables marked with the `static` attribute point to the same, shared global memory.
Initialization of all static variables is executed once, when `main` is executed.

### References
Variables marked with one of the following suffixes become references to variables:
- `?`: optional reference: the reference can be `null` or point to a valid value.
- `&`: mutable reference: the value is writable through the reference.
- `!`: const reference: the value is read-only through the reference.

A `static` immutable reference (`!`) is a common idiom for constants.

### Initialization
A variable can be initialized either by calling an initializer function, or with a literal
(optionally prefixed with a type name).

#### Default type initializer
Some builtin types have a default type initializer (`int`, `float`, ...); others must be
initialized explicitly when a new instance is created (`pointer`, `variant`, `typename`, `function`).
Enumerated types have no default initializer, so they must always be initialized explicitly.

#### Default member initializer
Every member of a record must be initialized when creating an instance, unless it has a default
member initializer, or it can use the default type initializer
(see [Record literal initializers](#record-literal-initializers)).

**Example**

```cmpl
// variables: static storage, references, and initialization

// a `static` local is initialized once and keeps its value across calls,
// pointing to the same shared (global) memory
int declVarCountCalls() {
	static int count = 0;
	return count += 1;
}
assertEq(1, declVarCountCalls());
assertEq(2, declVarCountCalls());

// a static member acts as shared state across all instances of the record
struct DeclVarInstance {
	static int count = 0;
	int instanceNr;
}
DeclVarInstance DeclVarInstance() {
	DeclVarInstance.count += 1;
	return { instanceNr: DeclVarInstance.count };
}
DeclVarInstance declVarFirst = DeclVarInstance();
DeclVarInstance declVarSecond = DeclVarInstance();
assertEq(1, declVarFirst.instanceNr);
assertEq(2, declVarSecond.instanceNr);

// a static immutable reference (`!`) is a common idiom for constants
static float64 declVarConstant! = 3.1415;
assert(declVarConstant == 3.1415);

// builtin types like `int` have a default type initializer (`0`)
int declVarDefaultInt;
assertEq(0, declVarDefaultInt);
```

## Arrays
The array type is a collection of zero or more elements of the same type.

### Fixed-size arrays
Allocates the space required for all the elements.
- Passed to functions by reference.
- The compiler knows the type of the elements and also the length.

### Dynamic-size arrays
A slice: a pair of a pointer to the first element and an integer containing the length.
- Initialized, assigned and passed to functions by reference, followed by the length.
- The compiler knows the type of the elements; the length is known at runtime.

### Unknown-size arrays
Pointers are arrays without a known length; useful to pass data to native functions.
- Initialized, assigned and passed to functions by reference.
- The compiler knows the type of the elements; the developer must track the length.
- No bounds checking is performed on these arrays, so they can be faster, but unsafe.

**Example**

```cmpl
// the three array kinds, and slicing

// fixed-size: the compiler knows the element type and the length; passed by reference
int32 declArrFixed[2] = {42, 97};
assertEq(2, declArrFixed.length);

// dynamic-size (a slice): a pointer + length pair, length known at runtime
int32 declArrSlice[] = {42, 97, 13};
assertEq(3, declArrSlice.length);

// if the initializer has no rest-form, the length is inferred and a fixed-size
// array is created first, then assigned to the slice — so this:
int32 declArrSliceEquiv[] = {42, 97, 13};
// is the same as:
int32 declArrFixedEquiv[3] = {42, 97, 13};
int32 declArrFixedAsSlice[] = declArrFixedEquiv;
assertEq(declArrSliceEquiv.length, declArrFixedAsSlice.length);

// unknown-size: no known length, useful for native/C interop; the developer must track the length
int32 declArrUnknown[*] = declArrFixed;
assertEq(42, declArrUnknown[0]);

// slicing: `a[...end]` (0-based prefix), `a[start ... end]` (start inclusive, end exclusive)
int32 declArrSource[10] = {...0};
int32 declArrPrefix[] = declArrSource[...4];       // the first 4 elements
assertEq(4, declArrPrefix.length);
int32 declArrMiddle[] = declArrSource[2 ... 5];    // elements 2, 3 and 4
assertEq(3, declArrMiddle.length);
int32 declArrEmpty[] = declArrSource[3 ... 3];     // empty slice
assertEq(0, declArrEmpty.length);
```

### Multidimensional arrays
Array dimensions are read in C order: the leftmost dimension is the outermost.
- `int32 a[2][3]` is an array of 2 elements, each of type `int32[3]`: `a[1][2]` is valid, `a[2][1]` is out of bounds.
- The elements are stored contiguously, row after row: `a[i][j]` is the element at position `i * 3 + j`.
- An element of the outer array (a row) is a fixed-size array: it can be indexed, sliced or referenced by a slice.
- The dimensions can be of different kinds; the leftmost dimension decides the kind of the variable:
  - `char names[4][*]` is a fixed-size array of 4 unknown-size arrays (strings).
  - `char lines[][80]` is a slice of fixed-size `char[80]` arrays.
- The same order is used by C, C++, Java, C# and Go.

Note: the type suffix notation of the D language (`int[3][2] a;`) reads the dimensions in reverse, this is not supported.

**Example**

```cmpl
// multidimensional arrays: dimensions are in C order, the leftmost is the outermost

// 2 rows, each row is an `int32[3]`
int32 declArrMatrix[2][3] = {
	{11, 12, 13},
	{21, 22, 23}
};
assertEq(2, declArrMatrix.length);
assertEq(3, declArrMatrix[0].length);
assertEq(23, declArrMatrix[1][2]);

// the elements are stored contiguously, row after row: `[i][j]` is the element `i * 3 + j`
int32 declArrMatrixFlat[*] = declArrMatrix[0];
assertEq(23, declArrMatrixFlat[1 * 3 + 2]);

// a row is a fixed-size array: it can be indexed, sliced or referenced by a slice
int32 declArrMatrixRow[] = declArrMatrix[1];
assertEq(3, declArrMatrixRow.length);
assertEq(21, declArrMatrixRow[0]);
int32 declArrMatrixRowPart[] = declArrMatrix[1][1 ... 3];
assertEq(2, declArrMatrixRowPart.length);
assertEq(22, declArrMatrixRowPart[0]);

// a slice of rows: the length of the rows is known by the compiler, the number of rows at runtime
int32 declArrMatrixRows[][3] = declArrMatrix;
assertEq(2, declArrMatrixRows.length);
assertEq(21, declArrMatrixRows[1][0]);

// mixing array kinds: the leftmost dimension decides the kind of the variable
char declArrNames[2][*] = {"Rocco", "Milo"};	// fixed-size array of 2 strings
assertEq('M', declArrNames[1][0]);
char declArrWords[][] = {"one", "two", "three"};	// slice of slices
assertEq(3, declArrWords.length);
assertEq(5, declArrWords[2].length);
```

### Array literal initializers
- `{}` — default initializer (iff the element type can be initialized by default).
- `{a, b}` — per-element initializer.
- `{...?}` — uninitialized memory.
- `{...0}` — zero-initialize memory (equivalent to `memset(var, 0, sizeof(var))`).
- `{...copy}` — copy elements from a source array.
- `{...copy, x: y}` — copy elements, then overwrite selected elements.
- `{...{}}` — fill every element with the default value.
- `{...{}, x: y}` — fill with the default value, then overwrite selected elements.
- `{...{fill}}` — fill every element with a given value.
- `{...{fill}, x: y}` — fill with a value, then overwrite selected elements.
- `{...(int idx){eval}}` — fill every element via a generator function.
- `{...(int idx){eval}, x: y}` — initialize the selected elements first (they must form a contiguous prefix starting at index 0), then fill the remaining elements via the generator.

**Example**

```cmpl
// array literal initializer forms

float64 declArrLitDefault[5] = {};                                                      // {} - default initializer (iff the element type has a default)
float64 declArrLitElements[5] = { 2, 5, 3, 8, 1 };                                       // {a, b} - per-element initializer
float64 declArrLitUndefined[5] = { ...? };                                              // {...?} - uninitialized memory
float64 declArrLitZero[5] = { ...0 };                                                    // {...0} - zero-initialize memory
float64 declArrLitCopy[5] = { ...declArrLitElements };                                  // {...copy} - copy elements from a source array
float64 declArrLitCopyOverwrite[5] = { ...declArrLitElements, 0: -9 };                  // {...copy, x: y} - copy, then overwrite selected elements
float64 declArrLitFillDefault[5] = { ...{} };                                           // {...{}} - fill every element with the default value
float64 declArrLitFillDefaultOverwrite[5] = { ...{}, 3: -9 };                           // {...{}, x: y} - fill with default, then overwrite selected elements
float64 declArrLitFillValue[5] = { ...{8} };                                            // {...{fill}} - fill every element with a given value
float64 declArrLitFillValueOverwrite[5] = { ...{8}, 3: -9 };                            // {...{fill}, x: y} - fill with a value, then overwrite selected elements
float64 declArrLitGenerated[5] = { ...(int i) {return i / 5.;} };                       // {...(int idx){eval}} - fill every element via a generator function
float64 declArrLitGeneratedOverwrite[5] = { ...(int i) {return i / 5.;}, 0: -1, 1: -2 }; // {...(int idx){eval}, x: y} - seed the first elements, then generate the rest

assertEq(0, declArrLitDefault[0]);
assertEq(2, declArrLitElements[0]);
assertEq(0, declArrLitZero[4]);
assertEq(2, declArrLitCopy[0]);
assertEq(-9, declArrLitCopyOverwrite[0]);
assertEq(5, declArrLitCopyOverwrite[1]);
assertEq(0, declArrLitFillDefault[3]);
assertEq(-9, declArrLitFillDefaultOverwrite[3]);
assertEq(8, declArrLitFillValue[2]);
assertEq(8, declArrLitFillValueOverwrite[0]);
assertEq(-9, declArrLitFillValueOverwrite[3]);
assertEq(-1, declArrLitGeneratedOverwrite[0]);
assertEq(-2, declArrLitGeneratedOverwrite[1]);
```

### Array slicing
A slice can be created from a fixed-size array, a slice, or an unknown-size array, using the slice operator.
- The start is optional and defaults to 0: `a[...end]` is the same as `a[0 ... end]`.
- The bounds are checked: `start <= end` must hold, and `end` must not exceed the length of the sliced array.
  Constant bounds on fixed-size arrays are verified at compile time, otherwise at runtime, aborting execution with `illegalArrayAccess`.
- Unknown-size arrays (`int a[*]`) have no length, so only the `start <= end` relation is validated.

**Example**

```cmpl
// a slice points to the same memory as the array it was created from, and saves the length
float64 declArrSlcFixed[3] = {2, 1, 3};
float64 declArrSlcAll[] = declArrSlcFixed;
assertEq(3, declArrSlcAll.length);
assertEq(declArrSlcFixed[2], declArrSlcAll[2]);

// the start is optional: `[... end]` is the same as `[0 ... end]`
float64 declArrSlcPrefix[] = declArrSlcFixed[... 2];
assertEq(2, declArrSlcPrefix.length);
assertEq(declArrSlcFixed[1], declArrSlcPrefix[1]);

// `start` is inclusive, `end` is exclusive
float64 declArrSlcMiddle[] = declArrSlcFixed[1 ... 3];
assertEq(2, declArrSlcMiddle.length);
assertEq(declArrSlcFixed[1], declArrSlcMiddle[0]);

// an unknown-size array has no length, it only points to the first element
float64 declArrSlcPtr[*] = declArrSlcFixed;
float64 declArrSlcFromPtr[] = declArrSlcPtr[1 ... 3];
assertEq(declArrSlcFixed[2], declArrSlcFromPtr[1]);
```

## Functions
- Functions that are not implemented will be converted to function references.
- Function references must be initialized, or later implemented (re-declared as an implemented function).
- Functions are assigned and passed to functions as pointers.

### Forward declared functions
Forward declared functions are functions that can be used before they are implemented.
Every forward declared function is converted to a function reference (which can decrease performance).

### Function references
Function references can be initialized to point to a function.
If a function's parameter is a function, it is converted to a function reference.

**Example**

```cmpl
// functions: variadic parameters, forward declaration, and references

float64 declFunSum(float64 data...) {
	float64 total = 0;
	for (int i = 0; i < data.length; i += 1) {
		total += data[i];
	}
	return total;
}
assertEq(6.0, declFunSum(1., 2., 3.));

// forward declaration: usable before it's implemented
float64 declFunForward();
float64 declFunForwardResult = declFunForward();
float64 declFunForward() {
	return Float64.pi;
}
assertEq(Float64.pi, declFunForwardResult);

// function reference: initialized to point to an existing function
float64 declFunReference(float64 value) = Float64.sin;
assertEq(0.0, declFunReference(0));

// a function-typed parameter converts to a function reference
int32 declFunApply(int32 x, int32 op(int32 value)) {
	return op(x);
}
int32 declFunDouble(int32 value) {
	return value * 2;
}
assertEq(10, declFunApply(5, declFunDouble));
```

### Functions with special names
If a function's name is a type name, by convention it is considered to be a constructor for that type,
so it should return an instance of the type its name represents.

**Example**

```cmpl
struct DeclFunPoint {
	int32 x;
	int32 y;
}

DeclFunPoint DeclFunPoint(int32 x, int32 y) {
	return { x: x, y: y };
}

DeclFunPoint declFunP = DeclFunPoint(3, 4);
assertEq(3, declFunP.x);
```

### Reflection functions
`pointer`, `variant` and `struct` can be used as functions for reflection:
- `pointer(x)` returns the address of the variable `x`.
- `variant(x)` constructs a variant carrying both the type and the address of `x`.
- `struct(x)` returns the type of `x`, or `null` if `x` is not a defined symbol — commonly used inside
  `static if` to conditionally define a missing alias.

**Example**

```cmpl
// pointer, struct (typename) and variant, used as functions for reflection

int32 declFunReflValue = 42;

pointer declFunReflPtr = pointer(declFunReflValue);   // the address of the variable
assert(declFunReflPtr != null);

variant declFunReflVar = variant(declFunReflValue);
assert(variant.type(declFunReflVar) == int32);
assert(variant.data(declFunReflVar) == declFunReflPtr);

typename declFunReflTyp = struct(declFunReflValue);   // the type of the variable
assert(declFunReflTyp == int32);

// `struct(x)` returns `null` when `x` is not a defined symbol —
// commonly used to conditionally define a missing alias
static if (struct(declFunReflNotDefined) == null) {
	inline declFunReflNotDefined = float64;
}
assert(struct(declFunReflNotDefined) == typename);
```

## Enumerations
An enumeration declares a list of named constants, all of a given type.
- It may also add a new type, the enumeration type, if the enumeration name is specified.
- The enumeration type is a reference type; its variables may be initialized, assigned or returned
  only with one of the values listed inside the enumeration, or with another variable of the same enumeration type.
- A value of the base type can be converted to the enumeration type only explicitly,
  using an [inline low level operation](#inline-low-level-operations): `inline(int32(value))`.
- The type of the enumerated values can be specified. If not specified, it defaults to `int`.
- Every enumeration variable declaration must be initialized.

### Enumeration values
- A value can be given explicitly with `name: value`; the value can be any constant expression
  of the enumeration's base type (`1 << 2`, `3.14`, ...).
- A value without an initializer is numbered automatically: the first value is 0,
  every other value is the previous integer value plus one.

### Anonymous enumeration
An anonymous enumeration does not create an enumeration type; it exposes its values directly
to the declaring scope.

**Example**

```cmpl
// enumerations: named constants, optionally forming their own reference type

enum DeclEnumColor: int32 {
	Red,
	Green,
	Blue,
}

DeclEnumColor declEnumC1 = DeclEnumColor.Red;
assert(declEnumC1 == DeclEnumColor.Red);   // ok: assigned from a value of the enumeration

DeclEnumColor declEnumC2 = declEnumC1;
assert(declEnumC2 == declEnumC1);          // ok: assigned from a variable of the same enum type

int32 declEnumRaw = DeclEnumColor.Green;
assertEq(1, declEnumRaw);                  // ok: enum variables can be assigned to variables of their base type

// DeclEnumColor declEnumBad = 2;           // error: enum variable can be assigned only from a value of the enumeration
// DeclEnumColor declEnumNull = null;       // error: enum variable can not be null
// DeclEnumColor declEnumUninit;            // error: enum variable must be initialized
// declEnumC1 = 2;                          // error: enum variable can be assigned only from a value of the enumeration

// explicit conversion from the base type, the value is not validated
int32 declEnumIndex = 2;
DeclEnumColor declEnumC3 = inline(int32(declEnumIndex));
assert(declEnumC3 == DeclEnumColor.Blue);

// explicit values: any constant expression of the base type
enum DeclEnumFlags: int32 {
	Read: 1 << 0,
	Write: 1 << 1,
	Execute: 1 << 2,
}
int32 declEnumFlagsRw = DeclEnumFlags.Read | DeclEnumFlags.Write;
assertEq(3, declEnumFlagsRw);

// automatic numbering continues from the previous explicit value
enum DeclEnumStatus: int32 {
	Ok,              // 0
	NotFound: 404,
	Gone,            // 405
}
int32 declEnumGone = DeclEnumStatus.Gone;
assertEq(405, declEnumGone);

// the base type is not limited to integers
enum DeclEnumConstant: float64 {
	Pi: 3.14159265358979,
	E: 2.71828182845905,
}
float64 declEnumPi = DeclEnumConstant.Pi;
assert(declEnumPi > 3.14 && declEnumPi < 3.15);

// an anonymous enumeration exposes its values to the declaring scope directly,
// without creating a new enumeration type
enum: int32 {
	DeclEnumAnonA,
	DeclEnumAnonB,
}
assertEq(0, DeclEnumAnonA);
assertEq(1, DeclEnumAnonB);
```

## Records
[Records](https://en.wikipedia.org/wiki/Record_(computer_science)) are user-defined compound types.
Records may contain only instance or static members and methods.

### Members
Members are the variables and functions declared inside a record.
Each instance of the record has its own, independent storage for its instance members.

If a member inside a record is declared `static`, it becomes a global variable or function,
accessible only through the declaring type.

A member may be marked with the same reference suffixes as a variable.
A member marked `&` is a mutable reference: it refers to the variable given at construction.

A `?`, `!` or `&` member must be bound to an existing, addressable variable — binding it to a literal
or other computed value is a compile-time error, since there is no storage for the member to refer to.
The only exception is an optional (`?`) member, which can also be initialized with `null`.

Every member of a record must be initialized when creating an instance,
unless it can be initialized by default (see [Record literal initializers](#record-literal-initializers)).

**Example**

```cmpl
struct DeclRecPoint {
	int32 x;
	int32 y;
}

DeclRecPoint declRecMemP1 = { x: 1, y: 2 };
DeclRecPoint declRecMemP2 = { x: 10, y: 20 };
assertEq(1, declRecMemP1.x);    // member access via `.`
assertEq(10, declRecMemP2.x);

declRecMemP1.x = 99;
assertEq(99, declRecMemP1.x);
assertEq(10, declRecMemP2.x);   // each instance keeps its own storage, unaffected by the change above

struct DeclRecRef {
	static int32 globalFunction(int32 x) { return x * x + x; }
	static int32 globalValue = 5;
	static int32 globalOptional? = globalValue;
	static int32 globalConstant! = 9;
	static int32 globalMutable& = globalValue;

	int32 functionRef(int32 x) = globalFunction;
	int32 optional? = null;
	int32 constant!;
	int32 mutable&;
	int32 value = 9;
}
DeclRecRef declRecMemR = {
	optional: DeclRecRef.globalValue,
	constant: DeclRecRef.globalValue,
	mutable: DeclRecRef.globalValue,
	value: DeclRecRef.globalValue,
};
assertEq(5, declRecMemR.optional);
assertEq(5, declRecMemR.constant);
assertEq(5, declRecMemR.mutable);
assertEq(5, declRecMemR.value);

declRecMemR.mutable = 7;
assertEq(7, declRecMemR.optional);
assertEq(7, declRecMemR.constant);
assertEq(7, declRecMemR.mutable);
assertEq(5, declRecMemR.value);

DeclRecRef.globalValue = 9;
assertEq(9, declRecMemR.optional);
assertEq(9, declRecMemR.constant);
assertEq(9, declRecMemR.mutable);
assertEq(5, declRecMemR.value);


// a `&`/`!` member must be bound to an existing variable: there is no storage for the member to
// refer to if it is bound to a literal or other computed value, so this is a compile-time error:
// DeclRecRef declRecMemBad = { optional: DeclRecRef.globalValue, constant: 5, mutable: DeclRecRef.globalValue };   // error: invalid cast(i64 -> ref)
```

### Record literal initializers
A record instance can be created from a literal, optionally prefixed with the type name (`Point{x: 1}`):
- `{}` — default initializer (iff every member can be initialized by default).
- `{x: y}` — member initializer; members not listed are initialized by default.
- `{...?}` — uninitialized memory.
- `{...0}` — zero-initialize memory (equivalent to `memset(var, 0, sizeof(var))`).
- `{...?, x: y}`, `{...0, x: y}` — uninitialized or zeroed memory, then initialize selected members.
- `{...copy}` — copy members from a source instance.
- `{...copy, x: y}` — copy members, then overwrite selected members.

The fill (`{...{fill}}`) and generator (`{...(int idx){eval}}`) forms are valid only for arrays.

A member that is not listed in the literal is initialized with:
- its default member initializer, if it has one;
- for a nested record whose members can be initialized by default, the same rules applied to its members;
- otherwise with the default type initializer of its type (`0` for numbers),
  if both the member and the initialized variable are mutable;
- otherwise it is a compile-time error: `pointer`, `typename`, `variant`, enumeration, object members,
  and `!`, `&` members must always be initialized explicitly.

**Example**

```cmpl
// record literal initializer forms

struct DeclRecLitPoint {
	int32 x;
	int32 y;
	int32 z = 7;
}

DeclRecLitPoint declRecLitMembers = { x: 1, y: 2 };                // {x: y} - `z` uses its default member initializer
assertEq(7, declRecLitMembers.z);

DeclRecLitPoint declRecLitTyped = DeclRecLitPoint{ x: 1 };         // type-prefixed literal, `y` uses the default type initializer
assertEq(0, declRecLitTyped.y);

DeclRecLitPoint declRecLitZero = { ...0 };                         // {...0} - zero everything, even `z`
assertEq(0, declRecLitZero.z);

DeclRecLitPoint declRecLitZeroOverwrite = { ...0, x: 5 };          // {...0, x: y} - zero, then initialize `x`
assertEq(5, declRecLitZeroOverwrite.x);
assertEq(0, declRecLitZeroOverwrite.z);

DeclRecLitPoint declRecLitUndefined = { ...?, x: 3 };              // {...?, x: y} - only `x` is initialized
assertEq(3, declRecLitUndefined.x);

DeclRecLitPoint declRecLitCopy = { ...declRecLitMembers };         // {...copy} - copy every member
assertEq(2, declRecLitCopy.y);

DeclRecLitPoint declRecLitCopyOverwrite = { ...declRecLitMembers, y: 9 };   // {...copy, x: y} - copy, then overwrite `y`
assertEq(1, declRecLitCopyOverwrite.x);
assertEq(9, declRecLitCopyOverwrite.y);

// nested records not listed in the literal are initialized member by member, using the same rules
struct DeclRecLitLine {
	DeclRecLitPoint start;
	int32 width = 1;
}
DeclRecLitLine declRecLitLine = { start: { x: 1, y: 2 } };          // `start.z` and `width` use their defaults
assertEq(7, declRecLitLine.start.z);
assertEq(1, declRecLitLine.width);

struct DeclRecLitStyle {
	int32 color = 0xff0000;
}
struct DeclRecLitShape {
	int32 corners;
	DeclRecLitStyle style;
}
DeclRecLitShape declRecLitShape = { corners: 3 };                  // `style` is not listed: `style.color` uses its default
assertEq(0xff0000, declRecLitShape.style.color);

// members without a default type initializer must be initialized explicitly
struct DeclRecLitNamed {
	char name![*];
	int32 value;
}
DeclRecLitNamed declRecLitNamed = { name: "answer", value: 42 };
// DeclRecLitNamed declRecLitNoName = { value: 42 };              // error: uninitialized member `name`
```

### Methods

Functions, just like variables, can be part of a record.

In case their first parameter is of the declaring type,
they can be invoked as `instance.method(...)`.

- If a `static` method is initialized, it becomes a function reference.
- If a `static` method is not implemented, it becomes a forward declared function.

- If a method is not implemented or initialized, it is **abstract**: it must be overridden in a
  record that extends this record, and is looked up at runtime.
- If a method is initialized (delegating to an existing implementation), it is **virtual**: it declares
  only the member function, can be overridden, and is looked up at runtime.
- If a method is implemented directly, it is **virtual**: the record declares both the member and the
  static function with the same name, it can be overridden, and is looked up at runtime.
- A `static` method is resolved at compile time.

A method that overrides an abstract or virtual method from a base record must mark its name with
a trailing `^`. Calling `TypeName.method(instance)` always invokes that exact method; calling
`instance.method()` may dispatch through an extension or an override instead.

**Example**

```cmpl
// methods: abstract, static, and overridden

// abstract method: declared but not implemented -- must be overridden by an extending
// record; resolved at runtime, not at compile time
struct DeclRecReader: object {
	int32 read(DeclRecReader this);

	// static method: implemented directly -- resolved at compile time
	static int32 magic() {
		return 7;
	}
}

struct DeclRecConstReader: DeclRecReader {
	int32 value;

	// overrides the abstract `read` method (the trailing `^` marks an override)
	int32 read^(DeclRecConstReader this) {
		return this.value;
	}
}

DeclRecConstReader declRecInstance = { value: 21 };

// `instance.read()` may dispatch through an extension or override
assertEq(21, declRecInstance.read());

// `TypeName.read(instance)` ensures this exact override is invoked
assertEq(21, DeclRecConstReader.read(declRecInstance));

assertEq(7, DeclRecReader.magic());
```

### Static records
A record declared `static` has all its members implicitly static (effectively a namespace).
Static records have no size and cannot be instantiated.

**Example**

```cmpl
// static records: all members are implicitly static, acting as a namespace

static struct DeclRecStaticNamespace {
	float64 linear(float64 t) {
		return t;
	}
	float64 square(float64 t) {
		return t * t;
	}
}

assertEq(0.5, DeclRecStaticNamespace.linear(0.5));
assertEq(0.25, DeclRecStaticNamespace.square(0.5));
```

### Memory layout
The members of a record are laid out in memory in declaration order, using the same rules as C:
- Every member is aligned to its natural alignment, so padding may be inserted before it:
  - numbers, `bool`, `char` and enumerations: their size (`int16` is aligned to 2, `float64` to 8).
  - references, pointers, slices, unknown-size arrays, variants and objects: the size of a reference.
  - fixed-size arrays: the alignment of their elements.
  - records: the largest alignment of their members.
- The size of the record is padded to the largest alignment of its members,
  so the members of every element of an array of records stay aligned.
- A record with a single member has the size of that member.
- Records are packed by default with the native pointer size of the `cmpl` process
  (`System.Platform.pointerSize`: 4 or 8 bytes, depending on how `cmpl` was built),
  and records extending `object` with the size of a reference.
  So every member is aligned to the smaller of its natural alignment and the packing:
  `struct { int8 a; int64 b; }` has 16 bytes in 64-bit builds, but 12 in 32-bit ones (like WebAssembly).

This makes it possible to declare records with the same memory layout as the structures
of native (C) code, without any annotation.

### Packed records
A record can be packed with a small power-of-2 integer (`0, 1, 2, 4, 8, 16, 32`):
the alignment of every member is limited to the given value instead of the default packing,
so the members are aligned to the smaller of their natural alignment and the given value
(same as `#pragma pack(n)` in C). The resulting layout is the same in every build.
- Packed with `0`: a C-like union — every member starts at the same memory location,
  the size is the size of the largest member, padded to the largest alignment of the members.
- Packed with `1`: tightly packed, no gap between members (may make member access expensive).
- Packed with a value not smaller than the natural alignment of the members: every member is aligned
  to its natural alignment, in 64-bit builds this is the same as not packed.

**Example**

```cmpl
// memory layout: members are aligned to their natural alignment, like in C
struct DeclRecLayout {
	int8 a;      // offset 0, followed by 3 bytes of padding
	int32 b;     // offset 4
	int16 c;     // offset 8, followed by 2 bytes of padding
}
assertEq(12, sizeof(DeclRecLayout));   // padded to the alignment of `b`

// a record with a single member has the size of that member
struct DeclRecWrapper {
	int32 value;
}
assertEq(4, sizeof(DeclRecWrapper));

// nested records are aligned to the largest alignment of their members
struct DeclRecNested {
	int8 tag;               // offset 0, followed by 3 bytes of padding
	DeclRecLayout layout;   // offset 4, the alignment of `DeclRecLayout` is 4
}
assertEq(16, sizeof(DeclRecNested));

// packed records: control the memory layout of the members

// packed with 0: a C-like union -- every member starts at the same offset
struct DeclRecUnion32: 0 {
	int32 i32;
	float32 f32;
}
DeclRecUnion32 declRecU = { i32: 0x3F800000 };
assertEq(1.0, declRecU.f32);   // both members share the same memory location
assertEq(4, sizeof(DeclRecUnion32));

// packed with 1: tightly packed, no gaps between members
struct DeclRecInstruction: 1 {
	uint8 opc;
	int32 arg;
}
DeclRecInstruction declRecI = { opc: 1, arg: 2 };
assertEq(1, declRecI.opc);
assertEq(2, declRecI.arg);
assertEq(5, sizeof(DeclRecInstruction));

// packed with 2: the alignment of the members is limited to 2
struct DeclRecPacked2: 2 {
	uint8 opc;   // offset 0, followed by 1 byte of padding
	int32 arg;   // offset 2
}
assertEq(6, sizeof(DeclRecPacked2));

// default packing: the alignment of `b` is limited to the native pointer size
struct DeclRecDefault {
	int8 a;      // offset 0, followed by 3 or 7 bytes of padding
	int64 b;     // offset 4 in 32-bit, 8 in 64-bit builds
}
assertEq(8 + System.Platform.pointerSize, sizeof(DeclRecDefault));

// packed with 8: `b` is aligned to its natural alignment in every build
struct DeclRecPacked8: 8 {
	int8 a;      // offset 0, followed by 7 bytes of padding
	int64 b;     // offset 8
}
assertEq(16, sizeof(DeclRecPacked8));
```

### Extended records
Extended records are usually allocated on the heap. The compiler should allocate on the heap only
variables that escape their scope. The base type of every extended record is the builtin type `object`.

**Example**

```cmpl
// extended records: their base type is the builtin type `object`

struct DeclRecComplex: object {
	float64 re;
	float64 im = 0;
}
DeclRecComplex declRecC1 = { re: 8 };
assertEq(8, declRecC1.re);
assertEq(0, declRecC1.im);
```

## Aliasing
Aliasing is analogous to a C preprocessor `#define`, except it is not preprocessed as text.
When an alias is used, the referenced expression or typename is expanded inline.
Arguments of an inline macro may be cached and evaluated only once.

**Example**

```cmpl
// aliasing: type, constant, and macro

inline declInlineByte = uint8;
assert(declInlineByte == uint8);

inline declInlinePi = 3.14159265359;
assert(declInlinePi > 3.14 && declInlinePi < 3.15);

inline declInlineHalf(int32 value) = value / 2;
assertEq(21, declInlineHalf(42));

inline declInlineMin(int32 a, int32 b) = a < b ? a : b;
assertEq(3, declInlineMin(3, 7));

// arguments of an inline macro may be cached and evaluated only once
static int declInlineCalls = 0;
int32 declInlineSideEffect() {
	declInlineCalls += 1;
	return 5;
}
inline declInlineSquare(int32 v) = v * v;
int32 declInlineResult = declInlineSquare(declInlineSideEffect());
assertEq(25, declInlineResult);
assertEq(1, declInlineCalls);
```

### Type construction/conversion operator
A function named like a type constructs an instance of that type; an `inline` alias from one
record type to another defines an explicit conversion between them.

**Example**

```cmpl
// type construction/conversion operators

struct DeclOpCelsius {
	float64 degrees;
}
struct DeclOpFahrenheit {
	float64 degrees;
}

DeclOpCelsius DeclOpCelsius(float64 value) {
	return { degrees: value };
}
DeclOpFahrenheit DeclOpFahrenheit(float64 value) {
	return { degrees: value };
}

// explicit conversion between two record types
inline DeclOpCelsius(DeclOpFahrenheit value) = DeclOpCelsius((value.degrees - 32) / 1.8);
inline DeclOpFahrenheit(DeclOpCelsius value) = DeclOpFahrenheit(value.degrees * 1.8 + 32);

DeclOpCelsius declOpBoilC = DeclOpCelsius(100.);
DeclOpFahrenheit declOpBoilF = DeclOpFahrenheit(declOpBoilC);   // explicit conversion
assert(declOpBoilF.degrees > 211.9 && declOpBoilF.degrees < 212.1);
```

### Property/Extension operators
An ordinary named `inline` function can be called with [Uniform Function Call Syntax](https://en.wikipedia.org/wiki/Uniform_Function_Call_Syntax)
as if it were a property or method of its first argument's type.

**Example**

```cmpl
// property/extension operators: an ordinary `inline` function used via UFCS as if it were a method

struct DeclOpExtComplex {
	float64 re;
	float64 im;
}
DeclOpExtComplex DeclOpExtComplex(float64 re, float64 im) {
	return { re: re, im: im };
}

DeclOpExtComplex declOpExtA = DeclOpExtComplex(3, 0);
DeclOpExtComplex declOpExtB = DeclOpExtComplex(5, 1);

inline declOpExtAbs(DeclOpExtComplex a) = Float64.sqrt(a.re * a.re + a.im * a.im);
float64 declOpExtX = declOpExtA.declOpExtAbs();
assertEq(3, declOpExtX);

inline declOpExtAdd(DeclOpExtComplex a, DeclOpExtComplex b) = DeclOpExtComplex(a.re + b.re, a.im + b.im);
DeclOpExtComplex declOpExtSum = declOpExtA.declOpExtAdd(declOpExtB);
assertEq(8, declOpExtSum.re);
```

## Operator overloading
Operators can be overloaded using the `inline` keyword.

**Example**

```cmpl
// unary and binary operator overloads

struct DeclOpComplex {
	float64 re;
	float64 im = 0;
}
DeclOpComplex DeclOpComplex(float64 re) {
	return { re: re };
}
DeclOpComplex DeclOpComplex(float64 re, float64 im) {
	return { re: re, im: im };
}

inline -(DeclOpComplex a) = DeclOpComplex(-a.re, -a.im);
inline +(DeclOpComplex a, DeclOpComplex b) = DeclOpComplex(a.re + b.re, a.im + b.im);

DeclOpComplex declOpA = DeclOpComplex(3);     //  3 + 0i
DeclOpComplex declOpB = -declOpA;             // -3 + 0i
assertEq(-3, declOpB.re);

DeclOpComplex declOpC = declOpA + declOpA;    //  6 + 0i
assertEq(6, declOpC.re);
```

# Type system
Every declared type is also a static variable referencing its metadata — the internal
representation of the symbol used by the compiler, which is exposed at runtime.

## Builtin types
The most fundamental types the virtual machine can work with are exposed to the compiler.

### void
The type void is an empty type with no values.
No variables of this type can be instantiated.
Mostly used when a function does not return a value.
It can also be used as a function to void (finalize) a variable.

### bool
May take on the values true and false only.
true and false are defined constants in the language.

### char
1 byte, ASCII character.

### int8
1 byte, signed (two's complement).
Covers values from -128 to 127.

### int16
2 bytes, signed (two's complement).
Covers values from -32,768 to 32,767.

### int32
4 bytes, signed (two's complement).
Covers values from -2,147,483,648 to 2,147,483,647.

### int64
8 bytes, signed (two's complement).
Covers values from -9,223,372,036,854,775,808 to +9,223,372,036,854,775,807.

### uint8
1 byte, unsigned.
Covers values from 0 to 255.

### uint16
2 bytes, unsigned.
Covers values from 0 to 65,535.

### uint32
4 bytes, unsigned.
Covers values from 0 to 4,294,967,295.

### uint64
8 bytes, unsigned.
Covers values from 0 to 18,446,744,073,709,551,615.

### float32
4 bytes, IEEE 754.
Covers a range from 1.40129846432481707e-45 to 3.40282346638528860e+38
(positive or negative).

### float64
8 bytes, IEEE 754.
Covers a range from 4.94065645841246544e-324 to 1.79769313486231570e+308
(positive or negative).

### pointer
`pointer` is a data type whose value refers directly to (or "points to") another value.
It contains utility functions for low level memory operations (memset, memcpy, ...).

Using it as a function with an identifier argument, it will return the address of the variable.
This might be useful to compare if variable references point to the same value or object.

### variant
`variant` is a dynamic type, which carries the type of the value, and a pointer to the value.

It may be defined as:
```
struct variant {
	typename type!;
	pointer data!;
}
```

### typename
`typename` is the compiler's internal symbol representation structure.
It contains static utility functions for reflection.

To get the type of a symbol, use `struct` as a function with an identifier argument:
- if the identifier is not defined, it will return `null`
- if the identifier is a type, it will return `typename`
- if the identifier is a variable, it will return its type (for a variant variable this is `variant`,
  use `variant.type(x)` to get the type of the carried value)


### function

`function` is the base type of all functions.

### object

`object` is the base type for all heap-allocated reference types.

Every type inherited from object will be reference counted,
and destroyed when there are no more references pointing to it.

## Builtin aliases and constants

### int
`int` is an alias for `int32` or `int64`, depending on the pointer size of the virtual machine.

### null
`null` is the constant reference value representing missing data.
It can be assigned to pointers, slices, unknown-size arrays, variants, functions,
objects and optional (`?`) references.

### true and false
`true` and `false` are the values of the `bool` type.
They are not keywords; the standard library defines them as an anonymous enumeration:
```
enum: bool {
	true: 0 == 0,
	false: 0 != 0,
}
```

**Example**

```cmpl
// `int` is an alias of one of the fixed-size integer types
assert(int == int32 || int == int64);

// `null` represents missing data
pointer typeBuiltinPtr = null;
assert(typeBuiltinPtr == null);
int32 typeBuiltinOptional? = null;

// `true` and `false` are the values of `bool`
bool typeBuiltinTrue = 0 == 0;
assert(typeBuiltinTrue == true);
assert(typeBuiltinTrue != false);
```

# Builtin functions

## The `raise` function
There is no `print` statement or exception handling built into the language.
The `raise` function can be used to log a message, with or without the current stacktrace,
and to abort the execution.

`void raise(int32 level, int32 trace, char message[*], variant details...);`

The compiler also passes the file name and line number where the function was invoked.

Arguments:
- `level`: the logging level of the message:
	- `raise.abort`: log the message, then abort the execution.
	- `raise.error`
	- `raise.print`
	- `raise.warn`
	- `raise.info`
	- `raise.debug`
	- `raise.verbose`
- `trace`: the maximum number of stack trace entries to print:
	- `raise.noTrace`: no stack trace.
	- `raise.defTrace`: the default stack trace length.
- `message`: the message to be printed.
- `details`: values to be inspected, printed after the message; useful in assertions.

The message is logged only if the level is enabled.
Except for `raise.abort`, the execution continues after the message is logged.

### Logging and assertion helpers
The standard library defines the following helpers on top of `raise`:

| Function                                     | Level     | Stack trace |
|----------------------------------------------|-----------|-------------|
| `verbose(message, details...)`               | `verbose` | no          |
| `debug(message, details...)`                 | `debug`   | no          |
| `trace(message, details...)`                 | `debug`   | yes         |
| `info(message, details...)`                  | `info`    | no          |
| `warn(message, details...)`                  | `warn`    | no          |
| `error(message, details...)`                 | `error`   | yes         |
| `abort(message, details...)`, `abort()`      | `abort`   | yes         |
| `assert(condition, message, details...)`     | `abort`   | yes         |
| `assertEq(expected, returned, message, ...)` | `abort`   | no          |

`assert` and `assertEq` abort the execution only if the assertion fails; the message is optional.
Prefer these helpers over writing to `File.out`: the messages go to the log file, and `File.out` may not be available.
Only variables should be passed as details after the message.

**Example**

```cmpl
// raise: log a message at a given level, with variables to inspect
int32 builtinRaiseValue = 42;
raise(raise.debug, raise.noTrace, "raise with details", builtinRaiseValue);

// the standard library helpers
debug("debug message", builtinRaiseValue);
assert(builtinRaiseValue == 42, "the value must be 42", builtinRaiseValue);
assertEq(42, builtinRaiseValue);
```

## The `recover` function
The `recover` function can be used to recover the execution if it was aborted by an error.

`int32 recover(pointer args, void action(pointer args));`

The function executes the given action passing it `args`, and returns the error that stopped it,
or `recover.noError` (`0`) if it was executed successfully.
The error codes are available as constants of the `recover` function:
- `recover.noError`: the action was executed successfully.
- `recover.illegalState`: invalid state, also returned if `action` is not a function.
- `recover.stackOverflow`: the stack size was exceeded.
- `recover.divisionByZero`: integer division by zero.
- `recover.illegalInstruction`: an invalid instruction was executed.
- `recover.illegalMemoryAccess`: invalid memory access, e.g. reading through a `null` reference.
- `recover.illegalArrayAccess`: array index out of bounds.
- `recover.nativeCallError`: a native function failed, also returned after `raise.abort`, e.g. a failed assertion.

**Example**

```cmpl
// recover: continue the execution after an error
void builtinRecoverSuccess(pointer args) {
}
void builtinRecoverDivideByZero(pointer args) {
	int32 zero = 0;
	int32 value = 1 / zero;
}
int32 builtinRecoverNoError = recover(null, builtinRecoverSuccess);
assert(builtinRecoverNoError == recover.noError);
int32 builtinRecoverError = recover(null, builtinRecoverDivideByZero);
assert(builtinRecoverError == recover.divisionByZero);
```

# Library and extensions
The documented public symbols of the standard library and of the loaded extensions, grouped by the type declaring them.

## cmplStd/lib/lang/Casts.cmpl
- `int32(value: int32): int32`: Explicit cast of base types
- `int64(value: int64): int64`: Explicit cast of base types
- `uint32(value: uint32): uint32`: Explicit cast of base types
- `uint64(value: uint64): uint64`: Explicit cast of base types
- `float32(value: float32): float32`: Explicit cast of base types
- `float64(value: float64): float64`: Explicit cast of base types
- `int32(value: int64): int32`: Explicit cast of base types
- `int32(value: uint32): int32`: Explicit cast of base types
- `int32(value: uint64): int32`: Explicit cast of base types
- `int32(value: float32): int32`: Explicit cast of base types
- `int32(value: float64): int32`: Explicit cast of base types
- `int64(value: int32): int64`: Explicit cast of base types
- `int64(value: uint64): int64`: Explicit cast of base types
- `int64(value: uint32): int64`: Explicit cast of base types
- `int64(value: float32): int64`: Explicit cast of base types
- `int64(value: float64): int64`: Explicit cast of base types
- `uint32(value: int32): uint32`: Explicit cast of base types
- `uint32(value: int64): uint32`: Explicit cast of base types
- `uint32(value: uint64): uint32`: Explicit cast of base types
- `uint32(value: float32): uint32`: Explicit cast of base types
- `uint32(value: float64): uint32`: Explicit cast of base types
- `uint64(value: int32): uint64`: Explicit cast of base types
- `uint64(value: int64): uint64`: Explicit cast of base types
- `uint64(value: uint32): uint64`: Explicit cast of base types
- `uint64(value: float32): uint64`: Explicit cast of base types
- `uint64(value: float64): uint64`: Explicit cast of base types
- `float32(value: int32): float32`: Explicit cast of base types
- `float32(value: int64): float32`: Explicit cast of base types
- `float32(value: uint32): float32`: Explicit cast of base types
- `float32(value: uint64): float32`: Explicit cast of base types
- `float32(value: float64): float32`: Explicit cast of base types
- `float64(value: int32): float64`: Explicit cast of base types
- `float64(value: int64): float64`: Explicit cast of base types
- `float64(value: uint32): float64`: Explicit cast of base types
- `float64(value: uint64): float64`: Explicit cast of base types
- `float64(value: float32): float64`: Explicit cast of base types
- `bool(value: bool): bool`: Explicit cast of base types
- `bool(value: int8): bool`: Explicit cast of base types
- `bool(value: int16): bool`: Explicit cast of base types
- `bool(value: int32): bool`: Explicit cast of base types
- `bool(value: int64): bool`: Explicit cast of base types
- `bool(value: uint8): bool`: Explicit cast of base types
- `bool(value: uint16): bool`: Explicit cast of base types
- `bool(value: uint32): bool`: Explicit cast of base types
- `bool(value: uint64): bool`: Explicit cast of base types
- `bool(value: float32): bool`: Explicit cast of base types
- `bool(value: float64): bool`: Explicit cast of base types
- `int8(value: bool): int8`: Explicit cast of base types
- `int8(value: int8): int8`: Explicit cast of base types
- `int8(value: int16): int8`: Explicit cast of base types
- `int8(value: int32): int8`: Explicit cast of base types
- `int8(value: int64): int8`: Explicit cast of base types
- `int8(value: uint8): int8`: Explicit cast of base types
- `int8(value: uint16): int8`: Explicit cast of base types
- `int8(value: uint32): int8`: Explicit cast of base types
- `int8(value: uint64): int8`: Explicit cast of base types
- `int8(value: float32): int8`: Explicit cast of base types
- `int8(value: float64): int8`: Explicit cast of base types
- `uint8(value: bool): uint8`: Explicit cast of base types
- `uint8(value: int8): uint8`: Explicit cast of base types
- `uint8(value: int16): uint8`: Explicit cast of base types
- `uint8(value: int32): uint8`: Explicit cast of base types
- `uint8(value: int64): uint8`: Explicit cast of base types
- `uint8(value: uint8): uint8`: Explicit cast of base types
- `uint8(value: uint16): uint8`: Explicit cast of base types
- `uint8(value: uint32): uint8`: Explicit cast of base types
- `uint8(value: uint64): uint8`: Explicit cast of base types
- `uint8(value: float32): uint8`: Explicit cast of base types
- `uint8(value: float64): uint8`: Explicit cast of base types
- `int16(value: bool): int16`: Explicit cast of base types
- `int16(value: int8): int16`: Explicit cast of base types
- `int16(value: int16): int16`: Explicit cast of base types
- `int16(value: int32): int16`: Explicit cast of base types
- `int16(value: int64): int16`: Explicit cast of base types
- `int16(value: uint8): int16`: Explicit cast of base types
- `int16(value: uint16): int16`: Explicit cast of base types
- `int16(value: uint32): int16`: Explicit cast of base types
- `int16(value: uint64): int16`: Explicit cast of base types
- `int16(value: float32): int16`: Explicit cast of base types
- `int16(value: float64): int16`: Explicit cast of base types
- `uint16(value: bool): uint16`: Explicit cast of base types
- `uint16(value: int8): uint16`: Explicit cast of base types
- `uint16(value: int16): uint16`: Explicit cast of base types
- `uint16(value: int32): uint16`: Explicit cast of base types
- `uint16(value: int64): uint16`: Explicit cast of base types
- `uint16(value: uint8): uint16`: Explicit cast of base types
- `uint16(value: uint16): uint16`: Explicit cast of base types
- `uint16(value: uint32): uint16`: Explicit cast of base types
- `uint16(value: uint64): uint16`: Explicit cast of base types
- `uint16(value: float32): uint16`: Explicit cast of base types
- `uint16(value: float64): uint16`: Explicit cast of base types
- `int32(value: bool): int32`: Explicit cast of base types
- `int32(value: int8): int32`: Explicit cast of base types
- `int32(value: uint8): int32`: Explicit cast of base types
- `int32(value: int16): int32`: Explicit cast of base types
- `int32(value: uint16): int32`: Explicit cast of base types
- `uint32(value: bool): uint32`: Explicit cast of base types
- `uint32(value: int8): uint32`: Explicit cast of base types
- `uint32(value: uint8): uint32`: Explicit cast of base types
- `uint32(value: int16): uint32`: Explicit cast of base types
- `uint32(value: uint16): uint32`: Explicit cast of base types
- `int64(value: bool): int64`: Explicit cast of base types
- `int64(value: int8): int64`: Explicit cast of base types
- `int64(value: uint8): int64`: Explicit cast of base types
- `int64(value: int16): int64`: Explicit cast of base types
- `int64(value: uint16): int64`: Explicit cast of base types
- `uint64(value: bool): uint64`: Explicit cast of base types
- `uint64(value: int8): uint64`: Explicit cast of base types
- `uint64(value: uint8): uint64`: Explicit cast of base types
- `uint64(value: int16): uint64`: Explicit cast of base types
- `uint64(value: uint16): uint64`: Explicit cast of base types
- `float32(value: bool): float32`: Explicit cast of base types
- `float32(value: int8): float32`: Explicit cast of base types
- `float32(value: uint8): float32`: Explicit cast of base types
- `float32(value: int16): float32`: Explicit cast of base types
- `float32(value: uint16): float32`: Explicit cast of base types
- `float64(value: bool): float64`: Explicit cast of base types
- `float64(value: int8): float64`: Explicit cast of base types
- `float64(value: uint8): float64`: Explicit cast of base types
- `float64(value: int16): float64`: Explicit cast of base types
- `float64(value: uint16): float64`: Explicit cast of base types

## cmplStd/lib/lang/Pointer.cmpl
- `int(value: bool): int32`: Explicit cast of base types
- `int(value: int8): int32`: Explicit cast of base types
- `int(value: int16): int32`: Explicit cast of base types
- `int(value: int32): int32`: Explicit cast of base types
- `int(value: int64): int32`: Explicit cast of base types
- `int(value: uint8): int32`: Explicit cast of base types
- `int(value: uint16): int32`: Explicit cast of base types
- `int(value: uint32): int32`: Explicit cast of base types
- `int(value: uint64): int32`: Explicit cast of base types
- `int(value: float32): int32`: Explicit cast of base types
- `int(value: float64): int32`: Explicit cast of base types
- `inc(ptr: pointer, val: int32): pointer`: Returns the pointer incremented with the given value
- `ldi(ptr: pointer): pointer`: Load pointer indirection
- `sti(ptr: pointer, value: pointer): void`: Store pointer indirect

## cmplStd/lib/lang/Debug.cmpl
- `verbose(message: char[*], details: variant[]): void`: report message at `verbose` logging level
- `debug(message: char[*], details: variant[]): void`: report message at `debug` logging level
- `trace(message: char[*], details: variant[]): void`: report message with stacktrace at `debug` logging level
- `info(message: char[*], details: variant[]): void`: report message at `info` logging level
- `warn(message: char[*], details: variant[]): void`: report message at `warn` logging level
- `error(message: char[*], details: variant[]): void`: report message with stacktrace at `error` logging level
- `abort(message: char[*], details: variant[]): void`: report message with stacktrace at `abort` logging level and abort execution
- `abort(): void`: report message with stacktrace at `abort` logging level and abort execution
- `assert(condition: bool, message: char[*], details: variant[]): void`: report message and abort the execution if the assertion condition fails (is false)
- `assert(condition: bool, details: variant[]): void`: report message and abort the execution if the assertion condition fails (is false)

## DebugTimer
small timer to measure elapsed time

- `start(): DebugTimer`: create a new timer and start it
- `end(timer: DebugTimer, message: char[*]): void`: log time spent since start
- `lap(timer: DebugTimer, message: char[*]): void`: log time spent since last lap

## NotEquals
Record used for debugging purposes, containing expected, returned and an array of extra values

- `expected: variant`: Value of the expected result
- `returned: variant`: Value of the actual result
- `details: variant[]`: Extra argument to identify what happened
- `debug(message: char[*], detail: NotEquals): void`: report the expected and returned values at `debug` logging level
- `error(message: char[*], detail: NotEquals): void`: report the expected and returned values at `error` logging level
- `abort(message: char[*], detail: NotEquals): void`: abort execution displaying the expected and returned values

## cmplStd/lib/lang/Debug.cmpl
- `NotEquals(expected: variant, returned: variant, details: variant[]): NotEquals`: Create a not equals record for inspection from expected and returned
- `assertEq(expected: int64, returned: int64, message: char[*], details: variant[]): void`: abort execution if the returned value is not equal to the expected value
- `assertEq(expected: int64, returned: int64): void`: abort execution if the returned value is not equal to the expected value
- `assertEq(expected: bool, returned: bool, message: char[*], details: variant[]): void`: abort execution if the returned value is not equal to the expected value

## cmplStd/lib.cmpl
- `sizeof(type: typename): int32`: Returns the size of the given type

## Uint8
8-bit unsigned integer

- `test(x: Uint8): int32`: Test if the value is zero or not, also the signum of the input (`0` if zero, `1` if positive)
- `neg(x: Uint8): Uint8`: Negates an 8-bit unsigned value
- `add(x: Uint8, y: Uint8): Uint8`: Adds two 8-bit unsigned integers
- `sub(x: Uint8, y: Uint8): Uint8`: Subtracts two 8-bit unsigned integers
- `mul(x: Uint8, y: Uint8): Uint8`: Multiplies two 8-bit unsigned integers
- `div(x: Uint8, y: Uint8): Uint8`: Divides two 8-bit unsigned integers
- `rem(x: Uint8, y: Uint8): Uint8`: Divide x by y, returning remainder
- `neg(x: Uint8, overflow: Uint8): Uint8`: Negates an 8-bit unsigned value
- `add(x: Uint8, y: Uint8, overflow: Uint8): Uint8`: Adds two 8-bit unsigned integers with carry out
- `add(x: Uint8, y: Uint8, c: Uint8, overflow: Uint8): Uint8`: Adds two 8-bit unsigned integers with carry in and out
- `sub(x: Uint8, y: Uint8, overflow: Uint8): Uint8`: Subtracts two 8-bit unsigned integers with borrow out
- `sub(x: Uint8, y: Uint8, c: Uint8, overflow: Uint8): Uint8`: Subtracts two 8-bit unsigned integers with borrow in and out
- `mul(x: Uint8, y: Uint8, overflow: Uint8): Uint8`: Multiplies two 8-bit unsigned integers with overflow out
- `cmt(x: Uint8): Uint8`: Computes the bitwise NOT of an 8-bit unsigned value
- `or(x: Uint8, y: Uint8): Uint8`: Computes the bitwise OR of two 8-bit unsigned integers
- `xor(x: Uint8, y: Uint8): Uint8`: Computes the bitwise XOR of two 8-bit unsigned integers
- `and(x: Uint8, y: Uint8): Uint8`: Computes the bitwise AND of two 8-bit unsigned integers
- `shl(x: Uint8, n: uint32): Uint8`: Performs a logical left shift of x by n bits for an 8-bit unsigned integer
- `shr(x: Uint8, n: uint32): Uint8`: Performs a logical right shift of x by n bits for an 8-bit unsigned integer
- `sar(x: Uint8, n: uint32): Uint8`: Performs an arithmetic right shift of x by n bits for an 8-bit unsigned integer
- `rotateLeft(x: Uint8, n: uint32): Uint8`: Returns the value of x rotated left by (n mod size) bits
- `rotateRight(x: Uint8, n: uint32): Uint8`: Returns the value of x rotated right by (n mod size) bits
- `length(x: Uint8): int32`: Returns the minimum number of bits required to represent the value of `x`
- `leadingZeros(x: Uint8): int32`: Returns the number of leading zero bits in x
- `trailingZeros(x: Uint8): int32`: Returns the number of trailing zero bits in x
- `scanReverse(x: Uint8): int32`: Finds the index of the most significant(highest) set bit.
- `scanForward(x: Uint8): int32`: Finds the index of the least significant(lowest) set bit
- `countOnes(y: Uint8): int32`: Returns the number of set bits ("population count") in x
- `reverseBits(y: Uint8): Uint8`: Returns the value of x with its bits in reversed order
- `reverseBytes(x: Uint8): Uint8`: Reverses the byte order of an 8-bit unsigned integer
- `abs(x: Uint8): Uint8`: Returns the absolute value of the input (identity for unsigned)
- `min(a: Uint8, b: Uint8): Uint8`: Returns the smaller of two values
- `max(a: Uint8, b: Uint8): Uint8`: Returns the greater of two values
- `clamp(value: Uint8, lo: Uint8, hi: Uint8): Uint8`: Returns the `value` clamped between the provided `lo` and `hi`
- `compare(x: Uint8, y: Uint8): int32`: Compare: returns 0 if equal, positive if x > y, negative if x < y
- `reverseBits(x: uint8): uint8`: todo: deprecated method, use the called one

## Int8
8-bit signed integer

- `neg(x: Int8): Int8`: Computes the arithmetic negation of an 8-bit signed integer
- `add(x: Int8, y: Int8): Int8`: Add two values
- `sub(x: Int8, y: Int8): Int8`: Subtract two values
- `mul(x: Int8, y: Int8): Int8`: Multiply two values
- `div(x: Int8, y: Int8): Int8`: Divide x by y, returning quotient
- `rem(x: Int8, y: Int8): Int8`: Divide x by y, returning remainder
- `neg(x: Int8, overflow: Int8): Int8`: Computes the arithmetic negation of an 8-bit signed integer
- `add(x: Int8, y: Int8, overflow: Int8): Int8`: Add two values with carry-out
- `sub(x: Int8, y: Int8, overflow: Int8): Int8`: Subtract two values with borrow-out
- `mul(x: Int8, y: Int8, overflow: Int8): Int8`: Multiply two values with overflow-out (overflow holds upper 8 bits of 16-bit product)
- `sign(x: Int8): int32`: Returns the sign of the value: `-1` if negative, `0` if zero, `1` if positive
- `abs(x: Int8): Int8`: Returns the absolute value
- `min(a: Int8, b: Int8): Int8`: Returns the smaller of two values
- `max(a: Int8, b: Int8): Int8`: Returns the greater of two values
- `clamp(value: Int8, lo: Int8, hi: Int8): Int8`: Returns the `value` clamped between the provided `lo` and `hi`
- `compare(x: Int8, y: Int8): int32`: Compare: returns 0 if equal, positive if x > y, negative if x < y

## cmplStd/lib/math/number/Int8.cmpl
- `Uint8(value: uint8): Uint8`: Construct a Uint8 from a packed byte value
- `uint32(value: Uint8): uint32`: Convert a Uint8 value to a uint32
- `Int8(value: int8): Int8`: Construct an Int8 from a signed byte value
- `int32(value: Int8): int32`: Convert an Int8 value to an int32

## Uint16
16-bit unsigned integer

- `test(x: Uint16): int32`: Test if the value is zero or not, also the signum of the input (`0` if zero, `1` if positive)
- `neg(x: Uint16): Uint16`: Negates a 16-bit unsigned value
- `add(x: Uint16, y: Uint16): Uint16`: Adds two 16-bit unsigned integers
- `sub(x: Uint16, y: Uint16): Uint16`: Subtracts two 16-bit unsigned integers
- `mul(x: Uint16, y: Uint16): Uint16`: Multiplies two 16-bit unsigned integers
- `div(x: Uint16, y: Uint16): Uint16`: Divides two 16-bit unsigned integers
- `rem(x: Uint16, y: Uint16): Uint16`: Divide x by y, returning remainder
- `neg(x: Uint16, overflow: Uint16): Uint16`: Negates a 16-bit unsigned value
- `add(x: Uint16, y: Uint16, overflow: Uint16): Uint16`: Adds two 16-bit unsigned integers with carry out
- `add(x: Uint16, y: Uint16, c: Uint16, overflow: Uint16): Uint16`: Adds two 16-bit unsigned integers with carry in and out
- `sub(x: Uint16, y: Uint16, overflow: Uint16): Uint16`: Subtracts two 16-bit unsigned integers with borrow out
- `sub(x: Uint16, y: Uint16, c: Uint16, overflow: Uint16): Uint16`: Subtracts two 16-bit unsigned integers with borrow in and out
- `mul(x: Uint16, y: Uint16, overflow: Uint16): Uint16`: Multiplies two 16-bit unsigned integers with overflow out
- `cmt(x: Uint16): Uint16`: Computes the bitwise NOT of a 16-bit unsigned value
- `or(x: Uint16, y: Uint16): Uint16`: Computes the bitwise OR of two 16-bit unsigned integers
- `xor(x: Uint16, y: Uint16): Uint16`: Computes the bitwise XOR of two 16-bit unsigned integers
- `and(x: Uint16, y: Uint16): Uint16`: Computes the bitwise AND of two 16-bit unsigned integers
- `shl(x: Uint16, n: uint32): Uint16`: Performs a logical left shift of x by n bits for a 16-bit unsigned integer
- `shr(x: Uint16, n: uint32): Uint16`: Performs a logical right shift of x by n bits for a 16-bit unsigned integer
- `sar(x: Uint16, n: uint32): Uint16`: Performs an arithmetic right shift of x by n bits for a 16-bit unsigned integer
- `rotateLeft(x: Uint16, n: uint32): Uint16`: Returns the value of x rotated left by (n mod size) bits
- `rotateRight(x: Uint16, n: uint32): Uint16`: Returns the value of x rotated right by (n mod size) bits
- `length(x: Uint16): int32`: Returns the minimum number of bits required to represent the value of `x`
- `leadingZeros(x: Uint16): int32`: Returns the number of leading zero bits in x
- `trailingZeros(x: Uint16): int32`: Returns the number of trailing zero bits in x
- `scanReverse(x: Uint16): int32`: Finds the index of the most significant(highest) set bit.
- `scanForward(x: Uint16): int32`: Finds the index of the least significant(lowest) set bit
- `countOnes(y: Uint16): int32`: Returns the number of set bits ("population count") in x
- `reverseBits(y: Uint16): Uint16`: Returns the value of x with its bits in reversed order
- `reverseBytes(y: Uint16): Uint16`: Reverses the byte order of a 16-bit unsigned integer
- `abs(x: Uint16): Uint16`: Returns the absolute value (identity for unsigned)
- `min(a: Uint16, b: Uint16): Uint16`: Returns the smaller of two values
- `max(a: Uint16, b: Uint16): Uint16`: Returns the greater of two values
- `clamp(value: Uint16, lo: Uint16, hi: Uint16): Uint16`: Returns the `value` clamped between the provided `lo` and `hi`
- `compare(x: Uint16, y: Uint16): int32`: Compare: returns 0 if equal, positive if x > y, negative if x < y

## Int16
16-bit signed integer

- `neg(x: Int16): Int16`: Computes the arithmetic negation of a 16-bit signed integer
- `add(x: Int16, y: Int16): Int16`: Add two values
- `sub(x: Int16, y: Int16): Int16`: Subtract two values
- `mul(x: Int16, y: Int16): Int16`: Multiply two values
- `div(x: Int16, y: Int16): Int16`: Divide x by y, returning quotient
- `rem(x: Int16, y: Int16): Int16`: Divide x by y, returning remainder
- `neg(x: Int16, overflow: Int16): Int16`: Computes the arithmetic negation of a 16-bit signed integer
- `add(x: Int16, y: Int16, overflow: Int16): Int16`: Add two values with carry-out
- `sub(x: Int16, y: Int16, overflow: Int16): Int16`: Subtract two values with borrow-out
- `mul(x: Int16, y: Int16, overflow: Int16): Int16`: Multiply two values with overflow-out (overflow holds upper 16 bits of the 32-bit product)
- `sign(x: Int16): int32`: Returns the sign of the value: `-1` if negative, `0` if zero, `1` if positive
- `abs(x: Int16): Int16`: Returns the absolute value
- `min(a: Int16, b: Int16): Int16`: Returns the smaller of two values
- `max(a: Int16, b: Int16): Int16`: Returns the greater of two values
- `clamp(value: Int16, lo: Int16, hi: Int16): Int16`: Returns the `value` clamped between the provided `lo` and `hi`
- `compare(x: Int16, y: Int16): int32`: Compare: returns 0 if equal, positive if x > y, negative if x < y

## cmplStd/lib/math/number/Int16.cmpl
- `Uint16(value: uint16): Uint16`: Construct a Uint16 from a 16-bit unsigned value
- `uint32(value: Uint16): uint32`: Convert a Uint16 value to a uint32
- `Int16(value: int16): Int16`: Construct an Int16 from a 16-bit signed value
- `int32(value: Int16): int32`: Convert an Int16 value to an int32

## Uint32
32-bit unsigned integer

- `test(x: Uint32): int32`: Test if the value is zero or not, also the signum of the input (`0` if zero, `1` if positive)
- `neg(x: Uint32): Uint32`: Negates a 32-bit unsigned value
- `add(x: Uint32, y: Uint32): Uint32`: Adds two 32-bit unsigned integers
- `sub(x: Uint32, y: Uint32): Uint32`: Subtracts two 32-bit unsigned integers
- `mul(x: Uint32, y: Uint32): Uint32`: Multiplies two 32-bit unsigned integers
- `div(x: Uint32, y: Uint32): Uint32`: Divides two 32-bit unsigned integers
- `rem(x: Uint32, y: Uint32): Uint32`: Divide x by y, returning remainder
- `neg(x: Uint32, overflow: Uint32): Uint32`: Negates a 32-bit unsigned value
- `add(x: Uint32, y: Uint32, overflow: Uint32): Uint32`: Adds two 32-bit unsigned integers with carry out
- `add(x: Uint32, y: Uint32, c: Uint32, overflow: Uint32): Uint32`: Adds two 32-bit unsigned integers with carry in and out
- `sub(x: Uint32, y: Uint32, overflow: Uint32): Uint32`: Subtracts two 32-bit unsigned integers with borrow out
- `sub(x: Uint32, y: Uint32, c: Uint32, overflow: Uint32): Uint32`: Subtracts two 32-bit unsigned integers with borrow in and out
- `mul(x: Uint32, y: Uint32, overflow: Uint32): Uint32`: Multiplies two 32-bit unsigned integers with overflow out
- `cmt(x: Uint32): Uint32`: Computes the bitwise NOT of a 32-bit unsigned value
- `or(x: Uint32, y: Uint32): Uint32`: Computes the bitwise OR of two 32-bit unsigned integers
- `xor(x: Uint32, y: Uint32): Uint32`: Computes the bitwise XOR of two 32-bit unsigned integers
- `and(x: Uint32, y: Uint32): Uint32`: Computes the bitwise AND of two 32-bit unsigned integers
- `shl(x: Uint32, n: uint32): Uint32`: Performs a logical left shift of x by n bits for a 32-bit unsigned integer
- `shr(x: Uint32, n: uint32): Uint32`: Performs a logical right shift of x by n bits for a 32-bit unsigned integer
- `sar(x: Uint32, n: uint32): Uint32`: Performs an arithmetic right shift of x by n bits for a 32-bit unsigned integer
- `rotateLeft(x: Uint32, n: uint32): Uint32`: Returns the value of x rotated left by (n mod size) bits
- `rotateRight(x: Uint32, n: uint32): Uint32`: Returns the value of x rotated right by (n mod size) bits
- `length(x: Uint32): int32`: Returns the minimum number of bits required to represent the value of `x`
- `leadingZeros(x: Uint32): int32`: Returns the number of leading zero bits in x
- `trailingZeros(x: Uint32): int32`: Returns the number of trailing zero bits in x
- `scanReverse(x: Uint32): int32`: Finds the index of the most significant(highest) set bit.
- `scanForward(x: Uint32): int32`: Finds the index of the least significant(lowest) set bit
- `countOnes(y: Uint32): int32`: Returns the number of set bits ("population count") in x
- `reverseBits(y: Uint32): Uint32`: Returns the value of x with its bits in reversed order
- `reverseBytes(y: Uint32): Uint32`: Reverses the byte order of a 32-bit unsigned integer
- `abs(x: Uint32): Uint32`: Returns the absolute value (identity for unsigned)
- `min(a: Uint32, b: Uint32): Uint32`: Returns the smaller of two values
- `max(a: Uint32, b: Uint32): Uint32`: Returns the greater of two values
- `clamp(value: Uint32, lo: Uint32, hi: Uint32): Uint32`: Returns the `value` clamped between the provided `lo` and `hi`
- `compare(x: Uint32, y: Uint32): int32`: Compare: returns 0 if equal, positive if x > y, negative if x < y
- `countOnes(x: uint32): int32`: todo: deprecated method, use the called one
- `reverseBits(x: uint32): uint32`: todo: deprecated method, use the called one
- `scanForward(x: uint32): int32`: todo: deprecated method, use the called one
- `scanReverse(x: uint32): int32`: todo: deprecated method, use the called one
- `sign(x: uint32): int32`: todo: deprecated method, use the called one
- `abs(x: uint32): uint32`: todo: deprecated method, use the called one
- `min(x: uint32, y: uint32): uint32`: todo: deprecated method, use the called one
- `max(x: uint32, y: uint32): uint32`: todo: deprecated method, use the called one
- `clamp(value: uint32, lo: uint32, hi: uint32): uint32`: todo: deprecated method, use the called one
- `compare(x: uint32, y: uint32): int32`: todo: deprecated method, use the called one
- `zeroExtend(value: uint32, offs: int32, size: int32): uint32`: Extracts and zero-extends a bit field from a value

## Int32
32-bit signed integer

- `neg(x: Int32): Int32`: Computes the arithmetic negation of a 32-bit signed integer
- `add(x: Int32, y: Int32): Int32`: Add two values
- `sub(x: Int32, y: Int32): Int32`: Subtract two values
- `mul(x: Int32, y: Int32): Int32`: Multiply two values
- `div(x: Int32, y: Int32): Int32`: Divide x by y, returning quotient
- `rem(x: Int32, y: Int32): Int32`: Divide x by y, returning remainder
- `neg(x: Int32, overflow: Int32): Int32`: Computes the arithmetic negation of a 32-bit signed integer
- `add(x: Int32, y: Int32, overflow: Int32): Int32`: Adds two 32-bit signed integers with carry out
- `sub(x: Int32, y: Int32, overflow: Int32): Int32`: Subtracts two 32-bit signed integers with borrow out
- `mul(x: Int32, y: Int32, overflow: Int32): Int32`: Multiplies two 32-bit signed integers with overflow out
- `sign(x: Int32): int32`: Returns the sign of the value: `-1` if negative, `0` if zero, `1` if positive
- `abs(x: Int32): Int32`: Returns the absolute value
- `min(a: Int32, b: Int32): Int32`: Returns the smaller of two values
- `max(a: Int32, b: Int32): Int32`: Returns the greater of two values
- `clamp(value: Int32, lo: Int32, hi: Int32): Int32`: Returns the `value` clamped between the provided `lo` and `hi`
- `compare(x: Int32, y: Int32): int32`: Compare: returns 0 if equal, positive if x > y, negative if x < y
- `sign(x: int32): int32`: todo: deprecated method, use the called one
- `abs(x: int32): int32`: todo: deprecated method, use the called one
- `min(x: int32, y: int32): int32`: todo: deprecated method, use the called one
- `max(x: int32, y: int32): int32`: todo: deprecated method, use the called one
- `clamp(value: int32, lo: int32, hi: int32): int32`: todo: deprecated method, use the called one
- `compare(x: int32, y: int32): int32`: todo: deprecated method, use the called one
- `signExtend(value: int32, offs: int32, size: int32): int32`: Extracts and sign-extends a bit field from a value

## cmplStd/lib/math/number/Int32.cmpl
- `Uint32(value: uint32): Uint32`: Construct a Uint32 from a 32-bit unsigned value
- `uint32(value: Uint32): uint32`: Convert a Uint32 value to a uint32
- `Int32(value: int32): Int32`: Construct an Int32 from a 32-bit signed value
- `int32(value: Int32): int32`: Convert an Int32 value to an int32

## Uint64
64-bit unsigned integer

- `test(x: Uint64): int32`: Test if the value is zero or not, also the signum of the input (`0` if zero, `1` if positive)
- `neg(x: Uint64): Uint64`: Negates a 64-bit unsigned value
- `add(x: Uint64, y: Uint64): Uint64`: Adds two 64-bit unsigned integers
- `sub(x: Uint64, y: Uint64): Uint64`: Subtracts two 64-bit unsigned integers
- `mul(x: Uint64, y: Uint64): Uint64`: Multiplies two 64-bit unsigned integers
- `div(x: Uint64, y: Uint64): Uint64`: Divides two 64-bit unsigned integers
- `rem(x: Uint64, y: Uint64): Uint64`: Divide x by y, returning remainder
- `neg(x: Uint64, overflow: Uint64): Uint64`: Negates a 64-bit unsigned value
- `add(x: Uint64, y: Uint64, overflow: Uint64): Uint64`: Adds two 64-bit unsigned integers with carry out
- `add(x: Uint64, y: Uint64, c: Uint64, overflow: Uint64): Uint64`: Adds two 64-bit unsigned integers with carry in and out
- `sub(x: Uint64, y: Uint64, overflow: Uint64): Uint64`: Subtracts two 64-bit unsigned integers with borrow out
- `sub(x: Uint64, y: Uint64, c: Uint64, overflow: Uint64): Uint64`: Subtracts two 64-bit unsigned integers with borrow in and out
- `mul(x: Uint64, y: Uint64, overflow: Uint64): Uint64`: Multiplies two 64-bit unsigned integers with overflow out
- `cmt(x: Uint64): Uint64`: Computes the bitwise NOT of a 64-bit unsigned value
- `or(x: Uint64, y: Uint64): Uint64`: Computes the bitwise OR of two 64-bit unsigned integers
- `xor(x: Uint64, y: Uint64): Uint64`: Computes the bitwise XOR of two 64-bit unsigned integers
- `and(x: Uint64, y: Uint64): Uint64`: Computes the bitwise AND of two 64-bit unsigned integers
- `shl(x: Uint64, n: uint32): Uint64`: Performs a logical left shift of x by n bits for a 64-bit unsigned integer
- `shr(x: Uint64, n: uint32): Uint64`: Performs a logical right shift of x by n bits for a 64-bit unsigned integer
- `sar(x: Uint64, n: uint32): Uint64`: Performs an arithmetic right shift of x by n bits for a 64-bit unsigned integer
- `rotateLeft(x: Uint64, n: uint32): Uint64`: Returns the value of x rotated left by (n mod size) bits
- `rotateRight(x: Uint64, n: uint32): Uint64`: Returns the value of x rotated right by (n mod size) bits
- `length(x: Uint64): int32`: Returns the minimum number of bits required to represent the value of `x`
- `leadingZeros(x: Uint64): int32`: Returns the number of leading zero bits in x
- `trailingZeros(x: Uint64): int32`: Returns the number of trailing zero bits in x
- `scanReverse(x: Uint64): int32`: Finds the index of the most significant(highest) set bit.
- `scanForward(x: Uint64): int32`: Finds the index of the least significant(lowest) set bit
- `countOnes(y: Uint64): int32`: Returns the number of set bits ("population count") in x
- `reverseBits(y: Uint64): Uint64`: Returns the value of x with its bits in reversed order
- `reverseBytes(y: Uint64): Uint64`: Reverses the byte order of a 64-bit unsigned integer
- `abs(x: Uint64): Uint64`: Returns the absolute value (identity for unsigned)
- `min(a: Uint64, b: Uint64): Uint64`: Returns the smaller of two values
- `max(a: Uint64, b: Uint64): Uint64`: Returns the greater of two values
- `clamp(value: Uint64, lo: Uint64, hi: Uint64): Uint64`: Returns the `value` clamped between the provided `lo` and `hi`
- `compare(x: Uint64, y: Uint64): int32`: Compare: returns 0 if equal, positive if x > y, negative if x < y
- `reverseBytes(x: uint64): uint64`: todo: deprecated method, use the called one
- `zeroExtend(value: uint64, offs: int32, size: int32): uint64`: Extracts and zero-extends a bit field from a value

## Int64
64-bit signed integer

- `neg(x: Int64): Int64`: Computes the arithmetic negation of a 64-bit signed integer
- `add(x: Int64, y: Int64): Int64`: Add two values
- `sub(x: Int64, y: Int64): Int64`: Subtract two values
- `mul(x: Int64, y: Int64): Int64`: Multiply two values
- `div(x: Int64, y: Int64): Int64`: Divide x by y, returning quotient
- `rem(x: Int64, y: Int64): Int64`: Divide x by y, returning remainder
- `neg(x: Int64, overflow: Int64): Int64`: Computes the arithmetic negation of a 64-bit signed integer
- `add(x: Int64, y: Int64, overflow: Int64): Int64`: Adds two 64-bit signed integers with carry out
- `sub(x: Int64, y: Int64, overflow: Int64): Int64`: Subtracts two 64-bit signed integers with borrow out
- `mul(x: Int64, y: Int64, overflow: Int64): Int64`: Multiplies two 64-bit signed integers with overflow out
- `sign(x: Int64): int32`: Returns the sign of the value: `-1` if negative, `0` if zero, `1` if positive
- `abs(x: Int64): Int64`: Returns the absolute value
- `min(a: Int64, b: Int64): Int64`: Returns the smaller of two values
- `max(a: Int64, b: Int64): Int64`: Returns the greater of two values
- `clamp(value: Int64, lo: Int64, hi: Int64): Int64`: Returns the `value` clamped between the provided `lo` and `hi`
- `compare(x: Int64, y: Int64): int32`: Compare: returns 0 if equal, positive if x > y, negative if x < y
- `sign(x: int64): int32`: todo: deprecated method, use the called one
- `abs(x: int64): int64`: todo: deprecated method, use the called one
- `signExtend(value: int64, offs: int32, size: int32): int64`: Extracts and sign-extends a bit field from a value

## cmplStd/lib/math/number/Int64.cmpl
- `Uint64(value: uint64): Uint64`: Construct a Uint64 from a 64-bit unsigned value
- `uint64(value: Uint64): uint64`: Convert a Uint64 value to a uint64
- `Int64(value: int64): Int64`: Construct an Int64 from a 64-bit signed value
- `int64(value: Int64): int64`: Convert an Int64 value to an int64

## Float64
64 bit floating point struct and mathematical methods

- `isUndefined(x: float64): bool`: Determines whether the given floating point value is undefined: not-a-number
- `isFinite(x: float64): bool`: Determines whether the given floating point value is finite: `!isUndefined(x) && !isInfinity(x)`
- `isInfinity(x: float64): bool`: Determines whether the given floating point value is ±infinity
- `toBits(value: float64): uint64`: return the 64 bit floating point value as an integer
- `fromBits(value: uint64): float64`: return the 64 bit integer as a floating point value
- `fromBits(hi: uint32, lo: uint32): float64`: return the two 32 bit integers merged as a floating point value
- `floor(x: float64): float64`: Computes the largest integer value not greater than the input
- `ceil(x: float64): float64`: Computes the smallest integer not less than the input
- `round(x: float64): float64`: Computes the nearest integer to the input
- `absMod(val: float64, mod: float64): float64`: Returns the positive modulus of the input
- `sign(x: float64): int32`: Returns the signum of the input (`0` if zero, `-1` if negative, `1` if positive)
- `abs(x: float64): float64`: Returns the absolute value of the input
- `ulp(x: float64): float64`: Compute least significant bit (Unit in the Last Place) for a number.
- `min(a: float64, b: float64): float64`: Returns the smaller value from the input values
- `max(a: float64, b: float64): float64`: Returns the greater value from the input values
- `clamp(value: float64, min: float64, max: float64): float64`: Returns the `value` clamped between `min` and `max`
- `mix(a: float64, b: float64, t: float64): float64`: Returns the linear interpolation between `a` and `b` by `t`
- `smoothstep(min: float64, max: float64, t: float64): float64`: Returns the Hermite interpolation between two values
- `min(values: float64[]): float64`: Returns the smaller value from the input values
- `max(values: float64[]): float64`: Returns the greater value from the input values
- `sum(values: float64[]): float64`: Returns the sum of the input values
- `mean(values: float64[]): float64`: Returns the mean of the input values
- `eval(x: float64, a0: float64, a1: float64): float64`: Return the polynomial evaluated at x using Horner's algorithm
- `eval(x: float64, a0: float64, a1: float64, a2: float64): float64`: Return the polynomial evaluated at x using Horner's algorithm
- `eval(x: float64, a0: float64, a1: float64, a2: float64, a3: float64): float64`: Return the polynomial evaluated at x using Horner's algorithm
- `eval(x: float64, a0: float64, a1: float64, a2: float64, a3: float64, a4: float64): float64`: Return the polynomial evaluated at x using Horner's algorithm
- `eval(x: float64, a0: float64, a1: float64, a2: float64, a3: float64, a4: float64, a5: float64): float64`: Return the polynomial evaluated at x using Horner's algorithm
- `eval(x: float64, polynomial: float64[]): float64`: Return the polynomial evaluated at x using Horner's algorithm
- `modf(x: float64, intPart: float64): float64`: Decompose a value into integer and fractional parts
- `frexp(f: float64, exp: int32): float64`: Frexp breaks f into a normalized fraction and an integral power of two, satisfying $ f == frac \times 2 ^ {exp} $
- `ldexp(f: float64, exp: int32): float64`: Ldexp is the inverse of Frexp, it returns $ frac \times 2 ^ {exp} $.
- `ln(x: float64): float64`: Returns the natural logarithm of the argument `x`.
- `exp(x: float64): float64`: Returns the base-e exponential: $ e^x $.
- `log2(x: float64): float64`: Returns the base-2 logarithm of the argument `x`.
- `exp2(x: float64): float64`: Returns the base-2 exponential of the argument `x`.
- `log10(x: float64): float64`: Returns the base-10 logarithm of the argument `x`.
- `log(x: float64, base: float64): float64`: Returns the logarithm of the specified `base` of the argument `x`.
- `sqrt(x: float64): float64`: Returns the square root of the parameter `x`: $\sqrt{x}$.
- `pow(x: float64, y: float64): float64`: Returns `x` raised to the power of `y`: $ x^y $.
- `radians(degrees: float64): float64`: Converts the input to radians from degrees.
- `degrees(radians: float64): float64`: Converts the input to degrees from radians.
- `atan2(y: float64, x: float64): float64`: Returns the inverse tangent of `y / x`.
- `sin(x: float64): float64`: Returns the sine of the radian argument `x`.
- `asin(x: float64): float64`: Returns the inverse sine of the radian argument `x`.
- `sinh(x: float64): float64`: Returns the hyperbolic sine of the radian argument `x`.
- `asinh(x: float64): float64`: Returns the inverse hyperbolic sine of the radian argument `x`.
- `cos(x: float64): float64`: Returns the cosine of the radian argument `x`.
- `acos(x: float64): float64`: Returns the inverse cosine of the radian argument `x`.
- `cosh(x: float64): float64`: Returns the hyperbolic cosine of the radian argument `x`.
- `acosh(x: float64): float64`: Returns the inverse hyperbolic cosine of the radian argument `x`.
- `tan(x: float64): float64`: Returns the tangent of the radian argument `x`.
- `atan(x: float64): float64`: Returns the inverse tangent of the radian argument `x`.
- `tanh(x: float64): float64`: Returns the hyperbolic tangent of the radian argument `x`.
- `atanh(x: float64): float64`: Returns the inverse hyperbolic tangent of the radian argument `x`.
- `sec(x: float64): float64`: Returns the secant of the radian argument `x`.
- `asec(x: float64): float64`: Returns the inverse secant of the radian argument `x`.
- `sech(x: float64): float64`: Returns the hyperbolic secant of the radian argument `x`.
- `asech(x: float64): float64`: Returns the inverse hyperbolic secant of the radian argument `x`.
- `csc(x: float64): float64`: Returns the cosecant of the radian argument `x`.
- `acsc(x: float64): float64`: Returns the inverse cosecant of the radian argument `x`.
- `csch(x: float64): float64`: Returns the hyperbolic cosecant of the radian argument `x`.
- `acsch(x: float64): float64`: Returns the inverse hyperbolic cosecant of the radian argument `x`.
- `cot(x: float64): float64`: Returns the cotangent of the radian argument `x`.
- `acot(x: float64): float64`: Returns the inverse cotangent of the radian argument `x`.
- `coth(x: float64): float64`: Returns the hyperbolic cotangent of the radian argument `x`.
- `acoth(x: float64): float64`: Returns the inverse hyperbolic cotangent of the radian argument `x`.

## Float32
32 bit floating point struct and mathematical methods

- `toBits(value: float32): uint32`: return the 32 bit floating point value as an integer
- `fromBits(value: uint32): float32`: return the 32 bit integer as a floating point value
- `floor(x: float32): float32`: Computes the largest integer value not greater than the input
- `ceil(x: float32): float32`: Computes the smallest integer not less than the input
- `round(x: float32): float32`: Computes the nearest integer to the input
- `sign(x: float32): int32`: Returns the signum of the input (`0` if zero, `-1` if negative, `1` if positive)
- `absMod(val: float32, mod: float32): float32`: Returns the positive modulus of the input
- `abs(x: float32): float32`: Returns the absolute value of the input
- `min(a: float32, b: float32): float32`: Returns the smaller value from the input values
- `max(a: float32, b: float32): float32`: Returns the greater value from the input values
- `clamp(value: float32, min: float32, max: float32): float32`: Returns the `value` clamped between `min` and `max`
- `mix(a: float32, b: float32, t: float32): float32`: Returns the linear interpolation between `a` and `b` by `t`
- `smoothstep(min: float32, max: float32, t: float32): float32`: Returns the Hermite interpolation between two values
- `ln(x: float32): float32`: Returns the natural logarithm of the argument `x`.
- `exp(x: float32): float32`: Returns the base-e exponential: $ e^x $.
- `log2(x: float32): float32`: Returns the base 2 logarithm of the argument `x`.
- `sqrt(x: float32): float32`: Returns the square root of the parameter `x`: $\sqrt{x}$.
- `pow(x: float32, y: float32): float32`: Returns `x` raised to the power of `y`: $ x^y $.
- `radians(degrees: float32): float32`: Converts the input to radians from degrees.
- `degrees(radians: float32): float32`: Converts the input to degrees from radians.
- `sin(x: float32): float32`: Returns the sine of the radian argument `x`.
- `cos(x: float32): float32`: Returns the cosine of the radian argument `x`.
- `tan(x: float32): float32`: Returns the tangent of the radian argument `x`.
- `atan2(y: float32, x: float32): float32`: Returns the inverse tangent of y/x.
- `asin(x: float32): float32`: Returns the inverse sine of the radian argument `x`.
- `acos(x: float32): float32`: Returns the inverse cosine of the radian argument `x`.
- `atan(x: float32): float32`: Returns the inverse tangent of the radian argument `x`.

## Fixed32
A 32 bit fixed-point type, with `precision` bits used for fractional part, the rest for integer part

- `bits(bits: int32): Fixed32`: return the fixed point value of the given raw bits
- `bits(value: Fixed32): int32`: return the internal raw value of the fixed point value
- `isUndefined(x: Fixed32): bool`: Determines whether the given fixed point value is undefined
- `neg(x: Fixed32): Fixed32`: Computes the negated value of the given number
- `add(x: Fixed32, y: Fixed32): Fixed32`: Adds the two fixed point numbers
- `sub(x: Fixed32, y: Fixed32): Fixed32`: Subtract one from another fixed point number
- `mul(x: Fixed32, y: Fixed32): Fixed32`: Multiply two fixed-point numbers
- `div(x: Fixed32, y: Fixed32): Fixed32`: Divide one fixed-point number with another one
- `rem(x: Fixed32, y: Fixed32): Fixed32`: Returns the remainder of the division of `x` by `y`
- `inv(x: Fixed32): Fixed32`: Compute the reciprocal of x: (1 / x)
- `cmp(x: Fixed32, y: Fixed32): int32`: Compares two fixed point numbers: returns `-1` if `x` is less than `y`, `1` if greater, `0` if equal
- `fract(x: Fixed32): Fixed32`: Returns the fractional part of the fixed point number
- `floor(x: Fixed32): Fixed32`: Computes the largest integer that is less than or equal to `x`
- `ceil(x: Fixed32): Fixed32`: Computes the smallest integer that is greater than or equal to `x`
- `round(x: Fixed32): Fixed32`: Computes the nearest integer to `x`
- `log2(x: Fixed32): Fixed32`: Returns the binary logarithm of the argument `x`
- `exp2(x: Fixed32): Fixed32`: Returns the base-2 exponential of the argument `x`
- `ln(x: Fixed32): Fixed32`: Returns the natural logarithm of the argument `x`
- `log(x: Fixed32, y: Fixed32): Fixed32`: Returns the logarithm of the argument `x` to the base `y`
- `exp(x: Fixed32): Fixed32`: Returns the base-e exponential: $ e^x $
- `sqrt(x: Fixed32): Fixed32`: Returns the square root of the parameter `x`: $\sqrt{x}$
- `pow(x: Fixed32, y: Fixed32): Fixed32`: Returns `x` raised to the power of `y`: $ x^y $
- `degrees(radians: Fixed32): Fixed32`: Converts the input to degrees from radians: $radians * 180 / pi$
- `radians(degrees: Fixed32): Fixed32`: Converts the input to radians from degrees: $degrees * pi / 180$
- `sin(x: Fixed32): Fixed32`: Returns the sine of the radian argument `x`
- `asin(x: Fixed32): Fixed32`: Returns the inverse sine of the radian argument `x`
- `sinh(x: Fixed32): Fixed32`: Returns the hyperbolic sine of the radian argument `x`
- `asinh(x: Fixed32): Fixed32`: Returns the inverse hyperbolic sine of the radian argument `x`
- `cos(x: Fixed32): Fixed32`: Returns the cosine of the radian argument `x`
- `acos(x: Fixed32): Fixed32`: Returns the inverse cosine of the radian argument `x`
- `cosh(x: Fixed32): Fixed32`: Returns the hyperbolic cosine of the radian argument `x`
- `acosh(x: Fixed32): Fixed32`: Returns the inverse hyperbolic cosine of the radian argument `x`
- `tan(x: Fixed32): Fixed32`: Returns the tangent of the radian argument `x`
- `atan(x: Fixed32): Fixed32`: Returns the inverse tangent of the radian argument `x`
- `tanh(x: Fixed32): Fixed32`: Returns the hyperbolic tangent of the radian argument `x`
- `atanh(x: Fixed32): Fixed32`: Returns the inverse hyperbolic tangent of the radian argument `x`
- `sec(x: Fixed32): Fixed32`: Returns the secant of the radian argument `x`
- `asec(x: Fixed32): Fixed32`: Returns the inverse secant of the radian argument `x`
- `sech(x: Fixed32): Fixed32`: Returns the hyperbolic secant of the radian argument `x`
- `asech(x: Fixed32): Fixed32`: Returns the inverse hyperbolic secant of the radian argument `x`
- `csc(x: Fixed32): Fixed32`: Returns the cosecant of the radian argument `x`
- `acsc(x: Fixed32): Fixed32`: Returns the inverse cosecant of the radian argument `x`
- `csch(x: Fixed32): Fixed32`: Returns the hyperbolic cosecant of the radian argument `x`
- `acsch(x: Fixed32): Fixed32`: Returns the inverse hyperbolic cosecant of the radian argument `x`
- `cot(x: Fixed32): Fixed32`: Returns the cotangent of the radian argument `x`
- `acot(x: Fixed32): Fixed32`: Returns the inverse cotangent of the radian argument `x`
- `coth(x: Fixed32): Fixed32`: Returns the hyperbolic cotangent of the radian argument `x`
- `acoth(x: Fixed32): Fixed32`: Returns the inverse hyperbolic cotangent of the radian argument `x`

## cmplStd/lib/math/number/Fixed32.cmpl
- `Fixed32(value: int32): Fixed32`: Convert the integer `value` to fixed-point value
- `int32(value: Fixed32): int32`: Convert the fixed point `value` to integer value
- `Fixed32(value: float64): Fixed32`: Convert the floating point `value` to fixed-point value
- `float64(value: Fixed32): float64`: Convert the fixed point `value` to floating point value

## Complex
Complex number

- `re: float64`: The real component of a complex number.
- `im: float64`: The imaginary component of a complex number.
- `neg(a: Complex): Complex`: Unary negation of a complex number: -a := Complex(-a.re, -a.im);
- `add(a: Complex, b: Complex): Complex`: Add two complex numbers: a + b := Complex(a.re + b.re, a.im + b.im);
- `add(a: Complex, b: float64): Complex`: Add a complex number `a` to a real number `b`
- `sub(a: Complex, b: Complex): Complex`: Subtract two complex numbers: a - b := Complex(a.re - b.re, a.im - b.im);
- `sub(a: Complex, b: float64): Complex`: Subtract the real number `b` from the complex number `a`
- `mul(a: Complex, b: Complex): Complex`: Multiply two complex numbers: a * b := Complex(a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re);
- `mul(a: Complex, b: float64): Complex`: Multiply a complex number `a` with a real number `b`
- `div(a: Complex, b: Complex): Complex`: Divide two complex numbers
- `div(a: Complex, b: float64): Complex`: Divide the complex number `a` by the real number `b`
- `abs(a: Complex): float64`: Compute the absolute value of the complex number `a`
- `arg(a: Complex): float64`: Compute the argument of the complex number `a`
- `inv(a: Complex): Complex`: Compute the reciprocal of a complex number (1 / a)
- `conj(a: Complex): Complex`: Compute the conjugate of the complex number `a`
- `exp(a: Complex): Complex`: Compute the exponential of a complex number: $ e^a $.
- `ln(a: Complex): Complex`: Compute the natural logarithm of the complex number `a`.
- `pow(a: Complex, b: Complex): Complex`: Compute the complex number `a` raised to the power of `b`: $ a^b $.
- `pow(a: Complex, b: float64): Complex`: Compute the complex number `a` raised to the power of `b`: $ a^b $.
- `sin(a: Complex): Complex`: Compute the sine of the complex number `a`.
- `cos(a: Complex): Complex`: Compute the cosine of the complex number `a`.
- `tan(a: Complex): Complex`: Compute the tangent of the complex number `a`.
- `cot(a: Complex): Complex`: Compute the cotangent of the complex number `a`.
- `sec(a: Complex): Complex`: Compute the secant of the complex number `a`.
- `csc(a: Complex): Complex`: Compute the cosecant of the complex number `a`.
- `sinh(a: Complex): Complex`: Compute the hyperbolic sine of the complex number `a`.
- `cosh(a: Complex): Complex`: Compute the hyperbolic cosine of the complex number `a`.
- `tanh(a: Complex): Complex`: Compute the hyperbolic tangent of the complex number `a`.
- `coth(a: Complex): Complex`: Compute the hyperbolic cotangent of the complex number `a`.
- `sech(a: Complex): Complex`: Compute the hyperbolic secant of the complex number `a`.
- `csch(a: Complex): Complex`: Compute the hyperbolic cosecant of the complex number `a`.

## cmplStd/lib/math/number/Complex.cmpl
- `Complex(value: float64): Complex`: Create a Complex number from the given `value`
- `Complex(real: float64, imaginary: float64): Complex`: Create a Complex number from the given `real` and `imaginary` values
- `toCartesian(x: Complex): Complex`: Convert from polar(r,θ) to cartesian(x,y) coordinates
- `toPolar(x: Complex): Complex`: Convert from cartesian(x,y) to polar(r,θ) coordinates

## vec2d
A 2d vector (2x float64)

- `x: float64`: X component of the vector
- `y: float64`: Y component of the vector
- `data: float64[2]`: Access the components as an array
- `add(a: vec2d, b: vec2d): vec2d`: Component wise add the two vectors.
- `sub(a: vec2d, b: vec2d): vec2d`: Component wise subtract the two vectors.
- `mul(a: vec2d, b: vec2d): vec2d`: Component wise multiply the two vectors.
- `div(a: vec2d, b: vec2d): vec2d`: Component wise divide the two vectors.
- `min(a: vec2d, b: vec2d): vec2d`: Component wise select the minimum from the two vectors.
- `max(a: vec2d, b: vec2d): vec2d`: Component wise select the maximum from the two vectors.
- `ceq(a: vec2d, b: vec2d): bool`: Compare the two vectors for equality.
- `dot(a: vec2d, b: vec2d): float64`: Dot product of the two vectors

## cmplStd/lib/math/vector/Vector2d.cmpl
- `vec2d(x: float64, y: float64): vec2d`: Initialize with given x, y components

## vec4f
A 4d vector (4x float32)

- `x: float32`: X component of the vector
- `y: float32`: Y component of the vector
- `z: float32`: Z component of the vector
- `w: float32`: W component of the vector
- `data: float32[4]`: Access the components as an array
- `neg(rhs: vec4f): vec4f`: Returns a negated copy of the vector.
- `add(lhs: vec4f, rhs: vec4f): vec4f`: Component wise add the two vectors.
- `sub(lhs: vec4f, rhs: vec4f): vec4f`: Component wise subtract the two vectors.
- `mul(lhs: vec4f, rhs: vec4f): vec4f`: Component wise multiply the two vectors.
- `div(lhs: vec4f, rhs: vec4f): vec4f`: Component wise divide the two vectors.
- `eval(v: vec4f, x: float32): float32`: Evaluate as a polynomial in point x

## cmplStd/lib/math/vector/Vector4f.cmpl
- `vec4f(x: float32, y: float32, z: float32, w: float32): vec4f`: Initialize with given x, y, z, w components
- `vec4f(x: float32, y: float32, z: float32): vec4f`: Initialize with (x: x, y: y, z: z, w: 1)
- `vec4f(x: float32, y: float32): vec4f`: Initialize with (x: x, y: y, z: 0, w: 1)
- `vec4f(xyz: vec4f, w: float32): vec4f`: Initialize x, y, z components using the components from the given vector, and w with the given value
- `vec4f(val: vec4f): vec4f`: Initialize all components using the components from the given vector
- `vec4f(val: float32): vec4f`: Initialize x, y, z, w components with the given scalar value
- `operator-(rhs: vec4f): vec4f`: Returns a negated copy of the vector.
- `operator+(lhs: vec4f, rhs: vec4f): vec4f`: Component wise add the two vectors.
- `operator+(lhs: vec4f, rhs: float32): vec4f`: Component wise add vector and scalar.
- `operator+(lhs: float32, rhs: vec4f): vec4f`: Component wise add vector and scalar.
- `operator-(lhs: vec4f, rhs: vec4f): vec4f`: Component wise subtract the two vectors.
- `operator-(lhs: vec4f, rhs: float32): vec4f`: Component wise subtract vector and scalar.
- `operator-(lhs: float32, rhs: vec4f): vec4f`: Component wise subtract vector and scalar.
- `operator*(lhs: vec4f, rhs: vec4f): vec4f`: Component wise multiply the two vectors.
- `operator*(lhs: vec4f, rhs: float32): vec4f`: Component wise multiply vector and scalar.
- `operator*(lhs: float32, rhs: vec4f): vec4f`: Component wise multiply vector and scalar.
- `operator/(lhs: vec4f, rhs: vec4f): vec4f`: Component wise divide the two vectors.
- `operator/(lhs: vec4f, rhs: float32): vec4f`: Component wise divide vector and scalar.
- `operator/(lhs: float32, rhs: vec4f): vec4f`: Component wise divide vector and scalar.
- `dp3(lhs: vec4f, rhs: vec4f): float32`: Dot product of the first 3 elements
- `dph(lhs: vec4f, rhs: vec4f): float32`: Homogeneous dot product
- `dp4(lhs: vec4f, rhs: vec4f): float32`: Dot product
- `cross(a: vec4f, b: vec4f): vec4f`: Cross product of the first 3 elements
- `length(v: vec4f): float32`: Length of the vector(xyz)
- `normalize(v: vec4f): vec4f`: Normalize the vector(xyz)
- `mix(a: vec4f, b: vec4f, t: float32): vec4f`: linear interpolate

## mat4f
A 4x4 matrix

- `m: float32[4][4]`: Access elements as an array: m[1][0]
- `data: float32[16]`: Access elements as a flattened array: data[4] => m[1][0]
- `v: vec4f[4]`: Access elements as an array of vectors: v[1].x => m[1][0]
- `x: vec4f`: The first row as a vector
- `y: vec4f`: The second row as a vector
- `z: vec4f`: The third row as a vector
- `w: vec4f`: The fourth row as a vector
- `xx: float32`: The first element of the first row
- `xy: float32`: The second element of the first row
- `xz: float32`: The third element of the first row
- `xw: float32`: The fourth element of the first row
- `yx: float32`: The first element of the second row
- `yy: float32`: The second element of the second row
- `yz: float32`: The third element of the second row
- `yw: float32`: The fourth element of the second row
- `zx: float32`: The first element of the third row
- `zy: float32`: The second element of the third row
- `zz: float32`: The third element of the third row
- `zw: float32`: The fourth element of the third row
- `wx: float32`: The first element of the fourth row
- `wy: float32`: The second element of the fourth row
- `wz: float32`: The third element of the fourth row
- `ww: float32`: The fourth element of the fourth row
- `dp3(mat: mat4f, vec: vec4f): vec4f`: Multiply with vector: dot product of the first 3 elements
- `dph(mat: mat4f, vec: vec4f): vec4f`: Multiply with vector: homogeneous dot product
- `dp4(mat: mat4f, vec: vec4f): vec4f`: Multiply with vector: full dot product
- `transform(mat: mat4f, x: float32, y: float32, z: float32): vec4f`: Transform a 3d point by the given matrix: homogeneous dot product
- `transform(mat: mat4f, x: float32, y: float32): vec4f`: Transform a 2d point by the given matrix: homogeneous dot product

## cmplStd/lib/math/vector/Matrix4f.cmpl
- `mat4f(xx: float32, xy: float32, xz: float32, xw: float32, yx: float32, yy: float32, yz: float32, yw: float32, zx: float32, zy: float32, zz: float32, zw: float32, wx: float32, wy: float32, wz: float32, ww: float32): mat4f`: Initialize all elements with the given values
- `mat4f(x: vec4f, y: vec4f, z: vec4f, w: vec4f): mat4f`: Initialize matrix with row vectors
- `transpose(mat: mat4f): mat4f`: Transpose the matrix
- `inverse(mat: mat4f): mat4f`: Build the inverse of the matrix
- `mul(lhs: mat4f, rhs: mat4f): mat4f`: Multiply two matrices (rows * cols)
- `rotation(center: vec4f, direction: vec4f, angle: float32): mat4f`: Build a rotation matrix
- `rotation(direction: vec4f, angle: float32): mat4f`: Build a rotation matrix
- `translation(direction: vec4f, amount: float32): mat4f`: Build a translation matrix
- `scale(direction: vec4f, amount: float32): mat4f`: Build a scaling matrix

## Random
A very simple random number generator

- `nextInt32(random: Random): int32`: Calculate the next uniformly distributed random number
- `nextInt64(random: Random): int64`: Calculate the next uniformly distributed random number
- `nextFloat32(random: Random): float32`: Calculate the next uniformly distributed random number
- `nextFloat64(random: Random): float64`: Calculate the next uniformly distributed random number
- `nextInt32(random: Random, min: int32, max: int32): int32`: Calculate the next uniformly distributed random number
- `nextInt64(random: Random, min: int64, max: int64): int64`: Calculate the next uniformly distributed random number
- `nextFloat32(random: Random, min: float32, max: float32): float32`: Calculate the next uniformly distributed random number
- `nextFloat64(random: Random, min: float64, max: float64): float64`: Calculate the next uniformly distributed random number
- `nextNormal(random: Random): float64`: Get normal (Gaussian) random sample with mean 0 and standard deviation 1
- `nextNormal(random: Random, mean: float64, standardDeviation: float64): float64`: Get normal (Gaussian) random sample with specified mean and standard deviation

## cmplStd/lib/math/random/Random.cmpl
- `Random(seed: int64): Random`: Create a random number generator based on the given seed
- `Random(): Random`: Create a random number generator based on the current time

## RunningStat
Statistics class to compute the most frequent statistical functions

- `clear(this: RunningStat): void`: reset the counters
- `push(this: RunningStat, value: float64): void`: add a new value to the stats
- `push(this: RunningStat, values: float64[]): void`: add new values to the stats
- `count(this: RunningStat): float64`: Returns the number of values added to the stats
- `min(this: RunningStat): float64`: Returns the smallest value added to the stats
- `max(this: RunningStat): float64`: Returns the largest value added to the stats
- `mean(this: RunningStat): float64`: Returns the mean of all values added to the stats
- `variance(this: RunningStat): float64`: Returns the variance of all values added to the stats
- `stddev(this: RunningStat): float64`: Returns the standard deviation of all values added to the stats
- `sum(this: RunningStat): float64`: Returns the sum of all values added to the stats

## Timeunit
enumeration of some known time units, used for conversions and to specify the precision


## cmplStd/lib/time/Timeunit.cmpl
- `convert(value: int64, from: Timeunit, to: Timeunit): int64`: Convert the given integer `value` from the `from` to `to` unit
- `convert(value: float64, from: Timeunit, to: Timeunit): float64`: Convert the given floating point `value` from the `from` to `to` unit

## Duration
Timespan is just a duration
 * for example: 33 seconds

- `value(duration: Duration, precision: Timeunit): int64`: Returns the value of the `duration` at the given `precision` (Seconds and Millis are mostly used as unix epoch)
- `millis(duration: int64): Duration`: Return duration from the passed milliseconds
- `millis(duration: Duration): int64`: Return milliseconds from the passed duration
- `seconds(duration: int64): Duration`: Return duration from the passed seconds
- `seconds(duration: Duration): int64`: Return seconds from the passed duration
- `minutes(duration: int64): Duration`: Return duration from the passed minutes
- `minutes(duration: Duration): int64`: Return minutes from the passed duration
- `hours(duration: int64): Duration`: Return duration from the passed hours
- `hours(duration: Duration): int64`: Return hours from the passed duration
- `days(duration: int64): Duration`: Return duration from the passed days
- `days(duration: Duration): int64`: Return days from the passed duration
- `add(lhs: Duration, rhs: Duration): Duration`: Add two durations

## cmplStd/lib/time/Duration.cmpl
- `Duration(value: int64, precision: Timeunit): Duration`: Construct a duration from the given `value` at the given `precision`

## Timestamp
Timestamp is a position in time, without timezone information
in case locale is needed use Datetime.

- `precision: Timeunit`: The time unit specifying the precision of the timestamp
- `value(timestamp: Timestamp, precision: Timeunit): int64`: Returns the value of `timestamp` at the given `precision` (Seconds and Millis are mostly used as unix epoch)
- `diff(a: Timestamp, b: Timestamp, precision: Timeunit): int64`: Returns the difference of `a` and `b` at the given `precision`
- `diff(a: Timestamp, b: Timestamp): Duration`: Returns the difference of `a` and `b`
- `compare(a: Timestamp, b: Timestamp): int32`: Returns whether the first timestamp is less(<0), equal(=0), or greater(>0) than the second timestamp
- `equal(a: Timestamp, b: Timestamp, precision: Timeunit): bool`: Check whether the two timestamps `a` and `b` are equal at the given `precision`
- `equal(a: Timestamp, b: Timestamp): bool`: Check whether the two timestamps `a` and `b` are equal
- `add(lhs: Timestamp, value: int64, precision: Timeunit): Timestamp`: Increment the current timestamp with the given `value` at the given `precision`
- `floor(value: Timestamp, precision: Timeunit): Timestamp`: Floor the current timestamp, leaving the field specified as the most significant field.
	 * For example, if you have the timestamp: 2002-03-28 13:45:01.231,
	 * using resolution Timeunit.Days, it should return 2002-03-28 00:00:00.000.
	 * using resolution Timeunit.Hours, it should return 2002-03-28 13:00:00.000.
	 * using resolution Timeunit.Minutes, it should return 2002-03-28 13:45:00.000.
	 *
	 * @param precision Resolution of the operation.
- `ceil(value: Timestamp, precision: Timeunit): Timestamp`: Ceil the current timestamp, leaving the field specified as the most significant field.
	 * For example, if you have the timestamp: 2002-03-28 13:45:01.231,
	 * using resolution Timeunit.Days, it should return 2002-03-29 00:00:00.000.
	 * using resolution Timeunit.Hours, it should return 2002-03-28 14:00:00.000.
	 * using resolution Timeunit.Minutes, it should return 2002-03-28 13:46:00.000.
	 *
	 * @param precision Resolution of the operation.
- `distribute(this: Timestamp, unit: Timeunit, duration: uint64): Timestamp`: Distribute randomly between current value and the duration at the given time-unit resolution.
	 * For example, if you had the timestamp: 2002-03-28 13:45:01.231,
	 * using distribute(Timeunit.Hours, 2), it will return a date between 2002-03-28 13:45:01.231 and 2002-03-28 15:45:01.231.
	 *
	 * @param unit  Resolution of the duration.
	 * @param duration The duration of the distribution time frame, negative values are allowed.
- `distribute(this: Timestamp, unit: Timeunit, start: int32, end: int32): Timestamp`: Distribute randomly between start and end at the given time-unit resolution.
	 * For example, if you have the timestamp of 2002-03-28 13:46:01.231,
	 * using distribute(Timeunit.Hours, 15, 17) as start and end, it will return a date between 2002-03-28 15:46:01.231 and 2002-03-28 17:46:01.231.
	 * in case you need a timestamp between 15:00:00.000 and 17:00:00.000 you will need to use the floor method before or after this operation.
	 *
	 * @param unit  Resolution of the operation.
	 * @param start The beginning of the distribution time frame.
	 * @param end   The end of the distribution time frame.
- `now(): Timestamp`: get the current utc time as timestamp from the system

## cmplStd/lib/time/Timestamp.cmpl
- `Timestamp(value: int64, precision: Timeunit): Timestamp`: Create a timestamp from the given `value` at the given `precision`

## Timezone
Computer / named timezone helpers.

- `name: char[]`: Zone abbreviation or id: "UTC", "EET", "America/New_York", ...
- `offset(zone: Timezone, timestamp: Timestamp, isDaylightSaving: bool): Duration`: Offset east of UTC at `timestamp`; sets `isDaylightSaving` when DST applies.
- `offset(zone: Timezone, timestamp: Timestamp): Duration`: Offset east of UTC at `timestamp` (ignores DST flag).
- `utc(): Timezone`: UTC timezone (offset always zero)
- `local(): Timezone`: Host computer local timezone (offset follows system DST rules)
- `lookup(name: char[]): Timezone`: Lookup timezone by IANA / Windows name (not implemented yet)

## Month
Month of year, January is 1, February is 2, ...


## cmplStd/lib/time/Datetime.cmpl
- `Month(index: int32): Month`: FIXME: implement enum indexing
- `indexOf(value: Month): int32`: FIXME: implement enum indexing

## Weekday
Day of week, Monday is 1, Tuesday is 2, ...


## cmplStd/lib/time/Datetime.cmpl
- `Weekday(index: int32): Weekday`: FIXME: implement enum indexing
- `indexOf(value: Weekday): int32`: FIXME: implement enum indexing

## Calendar
Type of the calendar


## Datetime
DateTime represents the parts of an instant in time.

- `year: int32`: Year: 1970
- `month: Month`: Month of year: 1 ... 12 / [Jan, Feb, Mar, ...]
- `day: uint8`: Day of month: 1 ... 31
- `hour: uint8`: Hour of day: 0 ... 23
- `minute: uint8`: Minute of hour: 0 ... 59
- `second: uint8`: Second of minute: 0 ... 59
- `millis: uint16`: Milliseconds of second: 0 ... 999
- `isoWeek: uint8`: ISO-8601 week number: 1 ... 53
- `isoYear: int32`: ISO-8601 week-numbering year (may differ near year boundaries)
- `dayOfYear: uint16`: Nth day of the year: 1 ... 366
- `dayOfWeek: Weekday`: Day of week: 1 ... 7 / [Mon, Tue, ...]
- `leapYear: bool`: this is a leap year
- `isLeapYear(year: int32): bool`: Checks whether the given year is a leap year or not
- `add(cal: Datetime, years: int32, months: int32, days: int32): Datetime`: Add the amount of `years`, `months` and `days` to the datetime

## cmplStd/lib/time/Datetime.cmpl
- `Timestamp(value: Datetime): Timestamp`: Convert the given Datetime `value` to a timestamp (milliseconds since 1970 UTC)
- `Datetime(timestamp: Timestamp, calendar: Calendar, timezone: Timezone): Datetime`: Convert the given `timestamp` to a datetime(year, month, day, ...) using the given `calendar` and `timezone`
- `Datetime(timestamp: Timestamp): Datetime`: Convert the given timestamp to a datetime in UTC
- `Datetime(timestamp: Timestamp, timezone: Timezone): Datetime`: Convert the given timestamp to a datetime in the given timezone
- `Datetime(year: int32, month: int32, day: int32, hour: int32, minute: int32, second: int32, millis: int32, zone: Timezone): Datetime`: Construct the Datetime from the given parts
- `Datetime(year: int32, month: int32, day: int32, hour: int32, minute: int32, second: int32, millis: int32): Datetime`: Construct the Datetime from the given parts
- `Datetime(year: int32, month: int32, day: int32, hour: int32, minute: int32, second: int32, zone: Timezone): Datetime`: Construct the Datetime from the given parts
- `Datetime(year: int32, month: int32, day: int32, zone: Timezone): Datetime`: Construct the Datetime from the given parts
- `Datetime(year: int32, month: int32, day: int32): Datetime`: Construct the Datetime from the given parts

## cmplStd/lib/text/cstr.cmpl
- `length(str: char[]): int32`: Computes the length of the string, bounded by the slice (stops at a NUL terminator)
- `length(str: char[*], max: int32): int32`: Computes the length of the string, bounded by the `max` argument (stops at a NUL terminator)
- `indexOf(str: char[], chr: char, start: int32): int32`: Returns the index of the first occurrence of a character in string
- `indexOf(str: char[], chr: char): int32`: Returns the position of a character inside an array of characters, returns a negative value if not found.
- `contains(str: char[], chr: char): bool`: Check if a string contains the given character
- `ignCaseCmp(a: char, b: char): int32`: ignore case character comparator
- `caseCmp(chr: char, with: char): int32`: default character comparator
- `startsWith(str: char[], with: char[], cmp(chr: char, with: char): int32): bool`: Check if a string begins with a specified string, using a custom comparator
- `endsWith(str: char[], with: char[], cmp(chr: char, with: char): int32): bool`: Check if a string ends in a specified string, using a custom comparator
- `compare(str: char[], with: char[], cmp(chr: char, with: char): int32): int32`: Check if the two strings are equal, less or greater, using a custom comparator
- `strcmp(str: char[], with: char[]): int32`: Check if the two strings are equal, less or greater (stops at a NUL terminator)
- `toHex(value: int32): char`: convert lower part (4 bits) of the input value to the corresponding lower case hexadecimal character
- `startsWith(str: char[], with: char[]): bool`: Check if a string begins with a specified string, using case-sensitive comparison
- `endsWith(str: char[], with: char[]): bool`: Check if a string ends in a specified string, using case-sensitive comparison
- `compare(str: char[], with: char[]): int32`: Check if the two strings are equal, less or greater, using case-sensitive comparison
- `equals(str: char[], with: char[]): bool`: Check if the two strings are equal, using case-sensitive comparison
- `reverse(str: char[], start: int32, end: int32): void`: reverse the characters in the string between start and end
- `remove(str: char[], start: int32, end: int32): void`: remove the characters in the string between start and end
- `float64(value: char[]): float64`: Convert a string to a 64 bit floating point value

## cmplStd/lib/text/Format.cmpl
- `append(output: char[], pos: int32, str: char[]): int32`: Append a string to the output at the given position
- `append(output: char[], pos: int32, chr: char): int32`: Append a character to the output at the given position

## NumberFormat
Number formatter

- `digits: char[]`: number digits to be used for formatting
- `digitSymbols: char[]`: specify the digit symbols and the radix to be used for formatting:
	 * * binary: "01",
	 * * decimal: "0123456789",
	 * * hexadecimal: "0123456789abcdef"
- `positiveSymbol: char`: the prefix symbol to be used for positive values
- `negativeSymbol: char`: the prefix symbol to be used for negative values
- `infinitySymbol: char[]`: the symbol to be used for floating-point infinity values
- `undefinedSymbol: char[]`: the symbol to be used for floating-point `not a number` values
- `groupingSymbol: char`: the symbol to be used for grouping values
- `digitGrouping: uint32`: the number of digits to group together
- `firstGrouping: uint32`: the number of digits making the first group
- `fractGrouping: uint32`: the number of digits to be grouped in the fractional part
- `paddingSymbol: char`: symbol used for padding the integer and fractional parts
- `width: int32`: minimum integer digits: 0: ".7", 1: "0.7", 2: "00.7", ...
- `fractionSymbol: char`: the symbol to be used between floating-point integer and fractional part as separator
- `trailingSeparator: bool`: keep trailing separator: false: "6", true: "6."
- `precision: int32`: floating-point precision to be used for the fractional part
- `trailingZeros: bool`: keep trailing zeroes: false: "0.7", true: ".700000" (for precision: 6)
- `format(format: NumberFormat, output: char[], pos: int32, value: int64): int32`: formatted print an integer value to the given `output` buffer
- `format(format: NumberFormat, output: char[], pos: int32, value: uint64): int32`: formatted print an integer value to the given `output` buffer
- `format(format: NumberFormat, output: char[], pos: int32, value: float64): int32`: formatted print a floating-point value to the given `output` buffer
- `format(format: NumberFormat, output: char[], pos: int32, value: Fixed32): int32`: formatted print a fixed-point value to the given `output` buffer

## DatetimeFormat
datetime regional formatter, currently 'en' if instantiated with: `DatetimeFormat fmt = {};`

- `format(formatter: DatetimeFormat, output: char[], end: int32, value: Datetime, format: char[]): int32`: Format date to text
	 * @param formatter the formatter to be used
	 * @param output write output to this buffer
	 * @param end position in buffer where to print the datetime
	 * @param value date to format
	 * @param format format string
	 *  	@null defaults to: "%Y.%m.%d"

## cmplStd/lib/text/Format.cmpl
- `format(value: uint64, output: char[], out: int32, format: char[]): int32`: Format integer to text
@param value value to format
@param output write output to this buffer
@param out starting position for output
@param format format string
	%b format as binary
	%o format as octal
	%d format as decimal
	%x format as hexadecimal
	%i format with custom radix passed as precision
- `format(value: int64, output: char[], out: int32, format: char[]): int32`: Format integer to text
@param value value to format
@param output write output to this buffer
@param out starting position for output
@param format format string
	%b format as binary
	%o format as octal
	%d format as decimal
	%x format as hexadecimal
	%i format with custom radix passed as precision
- `format(value: int32, output: char[], out: int32, format: char[]): int32`: Format integer to text
@param value value to format
@param output write output to this buffer
@param out starting position for output
@param format format string
	%b format as binary
	%o format as octal
	%d format as decimal
	%x format as hexadecimal
	%i format with custom radix passed as precision
- `format(value: float64, output: char[], out: int32, format: char[]): int32`: Format a 64-bit floating-point value
@param value value to format
@param output write output to this buffer
@param out starting position for output
@param format format string
	%d format as decimal(%lg): => 3.141593
	%f format as fixed precision(%lf) (always pad with `0` at the end)
	%b, %o, %x can be used to display the binary representation of the number
- `format(value: float32, output: char[], out: int32, format: char[]): int32`: Format a 32-bit floating-point value
@param value value to format
@param output write output to this buffer
@param out starting position for output
@param format format string
	%d format as decimal(%lg): => 3.141593
	%f format as fixed precision(%lf) (always pad with `0` at the end)
	%b, %o, %x can be used to display the binary representation of the number
- `format(value: Fixed32, output: char[], out: int32, format: char[]): int32`: Format a 32-bit fixed-point value
@param value value to format
@param output write output to this buffer
@param out starting position for output
@param format format string
	%d format as decimal(%lg): => 3.141593
	%f format as fixed precision(%lf) (always pad with `0` at the end)
	%b, %o, %x can be used to display the binary representation of the number
- `format(value: Datetime, output: char[], out: int32, format: char[]): int32`: extension method for backward compatibility
- `format(value: Datetime, output: char[], format: char[]): int32`: extension method for backward compatibility
- `format(value: float64, output: char[], format: char[]): int32`: extension method for debugging and testing
- `format(value: float32, output: char[], format: char[]): int32`: extension method for debugging and testing
- `format(value: Fixed32, output: char[], format: char[]): int32`: extension method for debugging and testing
- `format(value: uint32, output: char[], pos: int32, format: char[]): int32`: extension method for debugging and testing
- `format(value: uint64, output: char[], format: char[]): int32`: extension method for debugging and testing
- `format(value: int64, output: char[], format: char[]): int32`: extension method for debugging and testing
- `format(value: uint32, output: char[], format: char[]): int32`: extension method for debugging and testing
- `format(value: int32, output: char[], format: char[]): int32`: extension method for debugging and testing

## Closeable
Represents the base class of closeable objects like files, streams, etc

- `close(this: Closeable): void`: Release the resources held by this object.

## ByteReader
ByteReader can be used to read bytes from a stream (~= java:InputStream)

- `read(this: ByteReader, bytes: uint8[]): int32`: This abstract method must be implemented in subclasses containing the proper implementation
	 * the method must return the number of bytes read from the stream into the `bytes` array
	 * in case there is an error -1 should be returned
- `read(this: ByteReader): int32`: Read a single byte from the stream, a negative number is returned in case of error

## ByteWriter
ByteWriter can be used to write bytes to a stream (~= java:OutputStream)

- `write(this: ByteWriter, bytes: uint8[]): void`: This abstract method must be implemented in subclasses containing the proper implementation
	 * the method must write all the bytes from the `bytes` array
- `flush(this: ByteWriter): void`: This abstract method must be implemented in subclasses containing the proper implementation
	 * if a buffer is used directly or indirectly this method must flush all buffered data to the stream.
- `write(this: ByteWriter, oneByte: uint8): void`: Write a single byte to the stream
- `copy(this: ByteWriter, reader: ByteReader): int32`: Copy all bytes from the reader to the writer

## TextReader
TextReader can be used to read decoded text from a stream, including basic parsing

- `reader: ByteReader`: The underlying byte stream the text is read and decoded from
- `decode(this: TextReader, chars: uint32[]): int32`: This abstract method must be implemented in subclasses containing the proper implementation
	 * the method must return the number of characters decoded from the stream into the `chars` array
	 * in case there is an error -1 should be returned
- `close(this: TextReader): void`: Release the resources held by this object.
- `decode(this: TextReader): int32`: Read a single character from the stream, a negative number is returned in case of error

## TextWriter
TextWriter can be used to write encoded text to a stream, including basic printing


## TextWriter.Writeable
Interface of objects that can write themselves to a `TextWriter`


## Base64Encoder
Base64 encoder

- `writer: ByteWriter`: The underlying byte stream the encoded text is written to
- `wrap: int32`: wrap at the given position with a newline
- `write(this: Base64Encoder, data: uint8[]): void`: This abstract method must be implemented in subclasses containing the proper implementation
	 * the method must write all the bytes from the `bytes` array
- `flush(this: Base64Encoder): void`: This abstract method must be implemented in subclasses containing the proper implementation
	 * if a buffer is used directly or indirectly this method must flush all buffered data to the stream.
- `close(this: Base64Encoder): void`: Release the resources held by this object.

## Base64Decoder
Base64 decoder

- `reader: ByteReader`: The underlying byte stream the encoded text is read from
- `read(this: Base64Decoder, data: uint8[]): int32`: This abstract method must be implemented in subclasses containing the proper implementation
	 * the method must return the number of bytes read from the stream into the `bytes` array
	 * in case there is an error -1 should be returned
- `close(this: Base64Decoder): void`: Release the resources held by this object.

## AsciiReader
Ascii reader converts the given input by mapping every byte to a character

- `decode(this: AsciiReader, chars: uint32[]): int32`: This abstract method must be implemented in subclasses containing the proper implementation
	 * the method must return the number of characters decoded from the stream into the `chars` array
	 * in case there is an error -1 should be returned

## AsciiWriter
Ascii Writer converts the given input by chopping each character to a byte

- `encode(this: TextWriter, chars: uint32[]): void`: This abstract method must be implemented in subclasses containing the proper implementation
	 * the method must encode and write all the characters from the `chars` array

## Utf8Reader
Utf-8 decoder

- `decode(this: Utf8Reader, chars: uint32[]): int32`: This abstract method must be implemented in subclasses containing the proper implementation
	 * the method must return the number of characters decoded from the stream into the `chars` array
	 * in case there is an error -1 should be returned

## Utf8Writer
Utf-8 encoder

- `encode(bytes: uint8[4], cp: uint32): int32`: encode a codepoint to utf-8 bytes and return the number of bytes written
- `encode(this: Utf8Writer, chars: uint32[]): void`: This abstract method must be implemented in subclasses containing the proper implementation
	 * the method must encode and write all the characters from the `chars` array

## Easing
Easing functions map an input of [0, 1) usually to [0, 1) with an acceleration

- `linear(t: float64): float64`: Linear easing: returns `t` unchanged
- `quad(t: float64): float64`: Quadratic easing: `t^2`
- `cubic(t: float64): float64`: Cubic easing: `t^3`
- `quart(t: float64): float64`: Quartic easing: `t^4`
- `quint(t: float64): float64`: Quintic easing: `t^5`
- `expo(t: float64): float64`: Exponential easing: `2^(10 * (t - 1))`
- `sine(t: float64): float64`: Sinusoidal easing: `1 - cos(t * pi / 2)`
- `circle(t: float64): float64`: Circular easing: `1 - sqrt(1 - t^2)`
- `elastic(t: float64): float64`: Elastic easing: an exponentially growing sine wave, overshooting like a spring
- `back(t: float64): float64`: Back easing: pulls back slightly before moving forward
- `bounce(t: float64): float64`: Bounce easing: bounces like a ball dropped to the ground

## cmplStd/lib/math/Easing.cmpl
- `in(ease(ratio: float64): float64, ratio: float64): float64`: ease in, ex: Easing.elastic.in(t)
- `out(ease(ratio: float64): float64, ratio: float64): float64`: ease out, ex: Easing.elastic.out(t)
- `inOut(ease(ratio: float64): float64, ratio: float64): float64`: ease in and out, ex: Easing.elastic.inOut(t)
- `outIn(ease(ratio: float64): float64, ratio: float64): float64`: ease out and in, ex: Easing.elastic.outIn(t)

## Polynomial
Polynomial with `float64` coefficients, the coefficient at index `i` belongs to the term of degree `i`

- `destroy(this: Polynomial): void`: Dispose the polynomial, free up used memory for coefficients
- `destroy(this: Polynomial): void`: Dispose the polynomial, free up used memory for coefficients
- `degree(this: Polynomial): int32`: Compute the degree of the polynomial
- `get(this: Polynomial, index: int32): float64`: Get the nth coefficient of the polynomial
- `set(this: Polynomial, index: int32, value: float64): Polynomial`: Set the nth coefficient of the polynomial
- `neg(a: Polynomial): Polynomial`: Negate the polynomial
- `add(a: Polynomial, b: Polynomial): Polynomial`: Add two polynomials
- `sub(a: Polynomial, b: Polynomial): Polynomial`: Subtract two polynomials
- `mul(a: Polynomial, b: Polynomial): Polynomial`: Multiply two polynomials
- `evaluate(this: Polynomial, value: float64): float64`: Evaluate the polynomial using Horner's method
- `derivative(this: Polynomial): Polynomial`: Compute the derivative of the polynomial
- `integral(this: Polynomial, c: float64): Polynomial`: Compute the integral of the polynomial

## cmplStd/lib/math/vector/Polynomial.cmpl
- `Polynomial(copy: Polynomial): Polynomial`: Create a new polynomial by copying the given one.
- `Polynomial(values: float64[]): Polynomial`: Create a new polynomial with the given values, coefficients are reversed(lowest index === highest degree)

## cmplFile/lib.cmpl
- `read(file: File): int32`: extension method to read a single byte from a file
- `readLineRaw(file: File, buff: char[]): int32`: extension method to read a line from a file, or fill the buffer, no zero termination
- `readLine(file: File, buff: char[]): bool`: extension method to read a line from a file, or fill the buffer
- `write(file: File, value: int32): int32`: extension method to write a single byte into a file

## FileReader
Byte reader reading from a file

- `read(this: FileReader, bytes: uint8[]): int32`: This abstract method must be implemented in subclasses containing the proper implementation
	 * the method must return the number of bytes read from the stream into the `bytes` array
	 * in case there is an error -1 should be returned
- `close(this: FileReader): void`: Release the resources held by this object.

## FileWriter
Byte writer writing to a file

- `file: File`: The file the bytes are written to
- `write(this: FileWriter, bytes: uint8[]): void`: This abstract method must be implemented in subclasses containing the proper implementation
	 * the method must write all the bytes from the `bytes` array
- `flush(this: FileWriter): void`: This abstract method must be implemented in subclasses containing the proper implementation
	 * if a buffer is used directly or indirectly this method must flush all buffered data to the stream.
- `close(this: FileWriter): void`: Release the resources held by this object.

## BufferedFileWriter
Byte writer writing to a file through a 1 KiB buffer

- `write(this: BufferedFileWriter, bytes: uint8[]): void`: This abstract method must be implemented in subclasses containing the proper implementation
	 * the method must write all the bytes from the `bytes` array
- `flush(this: BufferedFileWriter): void`: This abstract method must be implemented in subclasses containing the proper implementation
	 * if a buffer is used directly or indirectly this method must flush all buffered data to the stream.
- `close(this: BufferedFileWriter): void`: Release the resources held by this object.

## cmplFile/lib/FileStream.cmpl
- `FileReader(path: char[]): FileReader`: Open the file at the given `path` for reading
- `FileWriter(path: char[], append: bool): FileWriter`: Open the file at the given `path` for writing, `append` to the end of an existing file or create a new one
- `BufferedFileWriter(path: char[], append: bool): BufferedFileWriter`: Open the file at the given `path` for buffered writing, `append` to the end of an existing file or create a new one

## cmplGfx/lib/rect.cmpl
- `empty(rectangle: Rect): bool`: Checks whether the given `rectangle` is empty or not.
- `contains(rect: Rect, x: int32, y: int32): bool`: Checks whether the given `rect` contains the point (`x`, `y`).
- `width(rectangle: Rect): int32`: Computes the width of the given `rectangle`.
- `width(rectangle: Rect, width: int32): void`: Sets the width of the given `rectangle`.
- `height(rectangle: Rect): int32`: Computes the height of the given `rectangle`.
- `height(rectangle: Rect, height: int32): void`: Sets the height of the given `rectangle`.
- `setSize(rectangle: Rect, width: int32, height: int32): void`: Sets the width and height of the given `rectangle`.
- `positionX(rectangle: Rect, x: int32): void`: Sets the x position of the given `rectangle`.
- `positionY(rectangle: Rect, y: int32): void`: Sets the y position of the given `rectangle`.
- `position(rectangle: Rect, x: int32, y: int32): void`: Sets the x and y position of the given `rectangle`.
- `adjust(rect: Rect, dx0: int32, dy0: int32, dx1: int32, dy1: int32): void`: Moves each edge of the given `rect` by the given deltas: `x0` by `dx0`, `y0` by `dy0`, `x1` by `dx1` and `y1` by `dy1`.
- `padded(rect: Rect, padding: int32): void`: Grows the given `rect` by `padding` on each side, a negative `padding` shrinks it.
- `translate(rectangle: Rect, dx: int32, dy: int32): void`: Moves the given `rectangle` by `dx` and `dy`.
- `translateX(rectangle: Rect, dx: int32): void`: Moves the given `rectangle` horizontally by `dx`.
- `translateY(rectangle: Rect, dy: int32): void`: Moves the given `rectangle` vertically by `dy`.
- `Rect(x0: int32, y0: int32, x1: int32, y1: int32): Rect`: Create a rectangle using left, top, right and bottom values

## argb
a 32 bit argb color

- `v: uint32`: value of color
- `b: uint8`: blue component
- `g: uint8`: green component
- `r: uint8`: red component
- `a: uint8`: alpha component

## cmplGfx/lib/color.cmpl
- `argb(argb: uint32): argb`: cast unsigned integer to argb value
- `uint32(value: argb): uint32`: cast argb value to unsigned integer
- `sat_s8(c: int32): uint32`: clamp signed gray or color component to fit in 8 bits (0 ... 255)
- `sat_u8(c: uint32): uint32`: clamp unsigned gray or color component to fit in 8 bits (0 ... 255)
- `pack(c: uint32): argb`: pack an argb color from the given component
- `pack(r: uint32, g: uint32, b: uint32): argb`: pack an argb color from the given components
- `pack(a: uint32, r: uint32, g: uint32, b: uint32): argb`: pack an argb color from the given components
- `gray(c: int32): argb`: construct a rgb8 color from the given gray value, clamp the input
- `gray(c: uint32): argb`: construct a rgb8 color from the given gray value, clamp the input
- `gray(c: float32): argb`: construct a rgb8 color from the given gray value, clamp the input
- `gray(c: float64): argb`: construct a rgb8 color from the given gray value, clamp the input
- `argb(r: int32, g: int32, b: int32): argb`: construct a rgb8 color from the given components, clamp the input
- `argb(r: uint32, g: uint32, b: uint32): argb`: construct a rgb8 color from the given components, clamp the input
- `argb(r: float32, g: float32, b: float32): argb`: construct a rgb8 color from the given components, clamp the input
- `argb(r: float64, g: float64, b: float64): argb`: construct a rgb8 color from the given components, clamp the input
- `argb(a: int32, r: int32, g: int32, b: int32): argb`: construct a rgb8 color from the given components, clamp the input
- `argb(a: uint32, r: uint32, g: uint32, b: uint32): argb`: construct a rgb8 color from the given components, clamp the input
- `argb(a: float32, r: float32, g: float32, b: float32): argb`: construct a rgb8 color from the given components, clamp the input
- `argb(a: float64, r: float64, g: float64, b: float64): argb`: construct a rgb8 color from the given components, clamp the input
- `ach(color: argb): int32`: get the Alpha channel of the color
- `rch(color: argb): int32`: get the Red channel of the color
- `gch(color: argb): int32`: get the Green channel of the color
- `bch(color: argb): int32`: get the Blue channel of the color
- `lum(color: argb): int32`: get the luminosity of the color (0.299 * color.r + 0.587 * color.g + 0.114 * color.b)
- `hue(rgb: argb): uint32`: get the hue of the color
- `mix_s8(t: int32, a: int32, b: int32): int32`: compute the linear interpolation of the values `a` and `b` by `t` in range 0...255
- `mix_s16(t: int32, a: int32, b: int32): int32`: compute the linear interpolation of the values `a` and `b` by `t` in range 0...65535
- `mix_rgb8(t: int32, a: argb, b: argb): argb`: compute the linear interpolation of the colors `a` and `b` by `t` in range 0...255
- `lerpLut(lut: argb[], colors: argb[]): void`: linear interpolate a lookup-table with the given colors
- `alphaLut(lut: int32[], hardness: int32, alpha: int32, reflect: bool): void`: fill an alpha lookup-table using the given `hardness` and `alpha`, mirrored if `reflect` is set
- `vec4f(rgb: argb): vec4f`: convert an argb color to a floating point vector
- `argb(vec: vec4f): argb`: convert a floating point vector to an argb color
- `lum(vec: vec4f): float32`: calculate the luminosity of a color

## cmplGfx/lib/color/color.hsl.cmpl
- `rgb2hsl(rgb: vec4f): vec4f`: Convert color from Rgb color space to Hsl color space
- `hsl2rgb(hsl: vec4f): vec4f`: Convert color from Hsl color space to Rgb color space

## cmplGfx/lib/image.cmpl
- `Image(width: int32, height: int32, depth: int32): Image`: Create an image with the given width, height and depth
- `Image(fileName: char[], depth: int32): Image`: Create an image by opening an image file
- `Image(fileName: char[]): Image`: Create an image by opening an image file
- `clip(image: Image, roi: Rect): bool`: Clip the rectangle against the given image boundaries
- `set(image: Image, x: int32, y: int32, color: argb): void`: Set the color of a pixel at the given position

## cmplGfx/lib/draw/draw.rect.cmpl
- `fillRect(image: Image, x0: int32, y0: int32, x1: int32, y1: int32, incl: int32, color: uint32): void`: Fill a rectangle on the `image` specified by the coordinates with the given `color`
- `fillRect(image: Image, x0: int32, y0: int32, x1: int32, y1: int32, color: uint32): void`: Fill a rectangle on the `image` specified by the coordinates with the given `color`
- `drawRect(image: Image, x0: int32, y0: int32, x1: int32, y1: int32, width: int32, color: uint32): void`: Draw a rectangle on the `image` specified by the coordinates with the given `width` and `color`
- `drawRect(image: Image, x0: int32, y0: int32, x1: int32, y1: int32, color: uint32): void`: Draw a rectangle on the `image` specified by the coordinates with the given `color`
- `fillRect(image: Image, roi: Rect, color: uint32): void`: Fill a rectangle on the `image` specified by `roi` with the given `color`
- `drawRect(image: Image, roi: Rect, color: uint32): void`: Draw a rectangle on the `image` specified by `roi` with the given `color`

## cmplGfx/lib/draw/draw.rrect.cmpl
- `fillRRect(image: Image, x0: int32, y0: int32, x1: int32, y1: int32, rx: int32, ry: int32, color: uint32): void`: Fill a rounded rectangle on `image` specified by the coordinates and rounding, with the given `color`
- `drawRRect(image: Image, x0: int32, y0: int32, x1: int32, y1: int32, rx: int32, ry: int32, color: uint32): void`: Draw a rounded rectangle on `image` specified by the coordinates and rounding, with the given `color`
- `fillRRect(image: Image, x1: int32, y1: int32, x2: int32, y2: int32, r: int32, color: uint32): void`: Fill the given rectangle rounding the corners using the given radius
- `drawRRect(image: Image, x1: int32, y1: int32, x2: int32, y2: int32, r: int32, color: uint32): void`: Draw the given rectangle rounding the corners using the given radius
- `fillRRect(image: Image, roi: Rect, rx: int32, ry: int32, color: uint32): void`: Fill the given rectangle rounding the corners using the given radii
- `fillRRect(image: Image, roi: Rect, r: int32, color: uint32): void`: Fill the given rectangle rounding the corners using the given radius

## cmplGfx/lib/draw/draw.line.cmpl
- `drawLineIncl(image: Image, x0: int32, y0: int32, x1: int32, y1: int32, color: uint32): void`: Draw a line, including ending pixel
- `drawLine(image: Image, x0: int32, y0: int32, x1: int32, y1: int32, color: uint32): void`: Draw a line between the given coordinates(excluding the rightmost bottom pixel)
- `drawBezier(image: Image, x0: int32, y0: int32, x1: int32, y1: int32, x2: int32, y2: int32, color: uint32): void`: Draw a quadratic bezier curve
- `drawBezier(image: Image, x0: int32, y0: int32, x1: int32, y1: int32, x2: int32, y2: int32, x3: int32, y3: int32, color: uint32): void`: Draw a cubic bezier curve

## cmplGfx/lib/draw.cmpl
- `fill(image: Image, color: uint32): Image`: Fill the entire image with the given color
- `fillOval(image: Image, x0: int32, y0: int32, x1: int32, y1: int32, color: uint32): void`: Fill an ellipse on the `image` specified by the given coordinates and the given `color`
- `drawOval(image: Image, x0: int32, y0: int32, x1: int32, y1: int32, color: uint32): void`: Draw an ellipse on the `image` specified by the given coordinates and the given `color`
- `fillOval(image: Image, roi: Rect, color: uint32): void`: Fill an ellipse on the `image` specified by `roi` with the given `color`
- `drawOval(image: Image, roi: Rect, color: uint32): void`: Draw an ellipse on the `image` specified by `roi` with the given `color`
- `drawText(image: Image, x: int32, y: int32, font: Image, text: char[], color: int32): void`: Draw the given ASCII `text` to the `image` at the given `x`, `y` coordinate, with the given `font` and `color`

## cmplGfx/lib/fx/image.copy.cmpl
- `copy(image: Image, x0: int32, y0: int32, src: Image, roi: Rect, extra: pointer, blend(extra: pointer, base: argb, with: argb): argb): void`: copy the `src` image to the destination with an optional blend method
- `copy(image: Image, x0: int32, y0: int32, src: Image, roi: Rect, blend(base: vec4f, with: vec4f): vec4f): void`: copy the `src` image to the destination with an optional blend method
- `copy(image: Image, x: int32, y: int32, src: Image, roi: Rect): void`: copy or convert `src` image to the destination `image`
- `copy(image: Image, src: Image): void`: copy or convert `src` image to the destination `image`
- `copy(image: Image, roi: Rect, col: argb): void`: fill the destination with the given `col` (alpha channel is used)
- `copy(image: Image, col: argb): void`: fill the destination with the given `col` (alpha channel is used)
- `blendAlpha(image: Image, x: int32, y: int32, src: Image, roi: Rect, alpha: int32): void`: copy the `src` image to the destination with the given transparency
- `blendDstAlpha(image: Image, x: int32, y: int32, src: Image, roi: Rect): void`: copy the `src` image to the destination masked with destination alpha
- `Image(copy: Image): Image`: Create an image by copying another one
- `tile(image: Image, x: int32, y: int32, src: Image, roi: Rect): void`: copy the `src` image multiple times so it fills the whole `image`
- `eval(image: Image, roi: Rect, aspect: float32, time: float32, eval(in: vec4f): vec4f): void`: Fill the surface by evaluating the given function for each pixel

## cmplGfx/lib/fx/image.transform.cmpl
- `transform(image: Image, rect: Rect, src: Image, mat: mat4f, interpolate: int32): void`: Copy one image to another using a transformation matrix [scale, rotate, translate, shear, perspective, etc]
- `resize(image: Image, rect: Rect, src: Image, roi: Rect, interpolate: int32): void`: Resize the `roi` region of the `src` image into the `rect` region of the destination `image` using the given `interpolate` mode.
- `Image(image: Image, width: int32, height: int32, align: int32, maxScale: float64): Image`: Create an image by copying another one using a custom width and height
- `Image(fileName: char[], width: int32, height: int32, depth: int32, align: int32, maxScale: float64): Image`: Create an image by opening an image file with custom width and height
- `Image(fileName: char[], width: int32, height: int32, depth: int32, align: int32): Image`: Create an image by opening an image file with custom width and height
- `Image(fileName: char[], width: int32, height: int32, depth: int32): Image`: Create an image by opening an image file with custom width and height

## cmplGfx/lib/fx/image.blur.cmpl
- `blur(image: Image, radius: int32, sigma: float64): void`: emulated implementation of blur function

## cmplGfx/lib/fx/color.lookup.cmpl
- `colorMap(image: Image, x: int32, y: int32, src: Image, roi: Rect, lut: argb[256]): void`: apply a color lookup table to each pixel component (alpha channel as luminosity), defined by the `roi` of the `src` image
- `colorMapRgb(image: Image, x: int32, y: int32, src: Image, roi: Rect, lut: argb[256]): void`: apply a color lookup table to the red, green and blue pixel components, defined by the `roi` of the `src` image
- `colorMap3d(image: Image, x: int32, y: int32, src: Image, roi: Rect, lut: argb[16777216]): void`: apply a 3d color lookup table, indexed by the red, green and blue pixel components, defined by the `roi` of the `src` image

## cmplGfx/lib/fx/color.matrix.cmpl
- `colorMat(image: Image, x: int32, y: int32, src: Image, roi: Rect, mat: mat4f): void`: apply a color matrix to each pixel defined by the `roi` of the `src` image

## cmplGfx/lib/3d/mesh.cmpl
- `Mesh(fileName: char[]): Mesh`: Create a mesh by opening a file

## Camera
Simple camera for 3d transformations and projection.

- `position: vec4f`: Position of the camera
- `forward: vec4f`: Direction the camera is looking at
- `right: vec4f`: Direction to the right of the camera
- `up: vec4f`: Direction above the camera
- `projection(fov: float32, asp: float32, n: float32, f: float32): mat4f`: Create an orthographic(`fov` == 0) or perspective projection matrix
- `projection(fov: float32, width: float32, height: float32): mat4f`: Create an orthographic(`fov` == 0) or perspective projection matrix in pixel space,
	 * mapping camera rays to pixels: (x, y, z) -> (cx + focal * x / z, cy + focal * y / z).
	 * `fov` is the full horizontal field of view in degrees; the w row holds the z divisor,
	 * so homogeneous consumers (Image.transform) apply the perspective divide.
- `lookAt(position: vec4f, target: vec4f, up: vec4f): Camera`: Create a camera at the given `position` looking at the given `target`
- `matrix(camera: Camera): mat4f`: Get the matrix of the camera, which can be multiplied with the projection matrix
- `matrix(camera: Camera, fov: float32, asp: float32, n: float32, f: float32): mat4f`: Get the matrix of the camera multiplied with the projection matrix
- `move(camera: Camera, direction: vec4f, amount: float32): void`: Move the camera in the given `direction` with the given `amount`
- `rotate(camera: Camera, orbit: vec4f, direction: vec4f, amount: float32): void`: Rotate the camera around `orbit` in the given `direction` with the given `amount`

## Window
Extends the native `Window` with timeout helpers

- `timeoutFps(fps: float64): int64`: calculate the timeout based on the given fps
- `show(image: Image, onEvent(action: int32, button: int32, ex: int32, ey: int32): int32): void`: Show the given `image` in a window and delegate the events to the `onEvent` callback
- `show(width: int32, height: int32, image0: Image, image1: Image, image2: Image): void`: Show the images in a window, clicking on the window will select one of them
- `show(image0: Image, image1: Image, image2: Image): void`: Show the images in a window, clicking on the window will select one of them
- `show(image0: Image, image1: Image): void`: Show the images in a window, clicking on the window will select one of them
- `show(image: Image): void`: Show the image in a window
- `show(width: int32, height: int32, animationDownScale: int32, eval(in: vec4f): vec4f): void`: Show or animate the procedural image generated by the `eval` function
- `show(width: int32, height: int32, eval(in: vec4f): vec4f): void`: Show the procedural image generated by the `eval` function
- `show(w: int32, h: int32, fovy: float32, mesh: Mesh): void`: Show the mesh in a window

## Style
Style used to layout and draw text and views.

- `dark: argb`: Default dark theme color for background
- `light: argb`: Default light theme color for background
- `blue: argb`: Default blue accent color for controls
- `green: argb`: Default green accent color for controls
- `gray: argb`: Default gray accent color for controls
- `orange: argb`: Default orange accent color for controls
- `red: argb`: Default red accent color for controls
- `magenta: argb`: Default magenta accent color for controls
- `cyan: argb`: Default cyan accent color for controls
- `align: int32`: Alignment of text in views
- `padding: int32`: Padding around text in views
- `font: Image`: The font to be used to draw text
- `defaultFormat: NumberFormat`: the default number format used for widgets
- `format: NumberFormat`: Formatting to be used printing numbers, dates, etc
- `backgroundColor: uint32`: Background color of views
- `hoveredColor: uint32`: Background color of hovered views
- `focusedColor: uint32`: Background color of focused views
- `valueColor: uint32`: Color used to draw values, and the border of pressed views
- `textColor: uint32`: Color used to draw text
- `align(parent: Rect, rect: Rect, align: int32): void`: Place the given `rect` inside the `parent` rectangle conforming to `align`
- `measure(this: Style, rect: Rect, text: char[]): void`: Measure the given `text` with `this` style.
- `align(this: Style, parent: Rect, rect: Rect, value: char[], align: int32): void`: Measure and align the given `value` with `this` style.
- `align(this: Style, parent: Rect, rect: Rect, value: char[]): void`: Measure and align the given `value` with `this` style.
- `setColor(style: Style, background: argb, accent: argb): void`: Setup a custom color for the style

## KeyEvent
Represents an event generated by a keyboard.

- `release: bool`: The key was released.
- `isShift: bool`: Shift key is pressed when this event is generated.
- `isCtrl: bool`: Control key is pressed when this event is generated.
- `isAlt: bool`: Alt key is pressed when this event is generated.
- `repeat: int32`: Number of times the key was repeated while holding it down.
- `key: int32`: Virtual key code.
- `chr: int32`: Unicode character.

## TouchEvent
Represents an event generated by a mouse or touch device (TouchScreen, TouchPad, etc).

- `release: bool`: The mouse-button was released or the finger was lifted from the screen.
- `isShift: bool`: Shift key is pressed when this event is generated.
- `isCtrl: bool`: Control key is pressed when this event is generated.
- `isAlt: bool`: Alt key is pressed when this event is generated.
- `clicks: int32`: Number of times the click or tap event was repeated.
- `button: int32`: The mouse-button that was pressed or the finger that was placed on the screen.
- `x: int32`: Horizontal coordinate of the event
- `y: int32`: Vertical coordinate of the event
- `dx: int32`: Horizontal change between the previous and current event.
- `dy: int32`: Vertical change between the previous and current event.
- `test(event: TouchEvent, rect: Rect): bool`: Checks whether the event is inside the given `rect`
- `isMotion(event: TouchEvent): bool`: Checks whether the event is a motion event
- `isPress(event: TouchEvent): bool`: Checks whether the event is a press event
- `isLongPress(event: TouchEvent): bool`: Checks whether the event is a long press event
- `isPress(event: TouchEvent, button: int32): bool`: Checks whether the event is a press event of the given `button`

## View
View is the base class of all user interface elements.
 * it has width and height, forming a rectangular shape on screen.
 * on its own a view has no position, it must be positioned in a layout.


## View.State
State of the view used for drawing, and returned from event handlers


## Layout
Layout is a view containing other views positioned by it.
 * the Layout class does not clip its children.
 * the size of the Layout class should contain all the views.

- `paddingTop: int32`: Space between layout and its content at top.
- `paddingLeft: int32`: Space between layout and its content at left.
- `paddingRight: int32`: Space between layout and its content at right.
- `paddingBottom: int32`: Space between layout and its content at bottom.
- `orientation: int32`: Vertical, horizontal or other orientation of the layout
	 * zero represents a stack(Qt) or frame(Android) layout (all child elements are placed on top of each other)
	 * negative value represents a horizontal layout, specifying the number of rows to be used
	 * positive value represents a vertical layout, specifying the number of columns to be used
- `getPressed(this: Layout): View`: Returns the pressed view in the current layout
- `getFocused(this: Layout): View`: Returns the focused view in the current layout
- `setFocused(this: Layout, focus: View): bool`: Sets the focused view in the current layout
- `addItems(this: Layout, alignment: int32, views: View[]): Layout`: Append the given `views` to `this` layout with the given `alignment`
- `size(this: Layout): int32`: Returns the number of views in this layout
- `getView(this: Layout, idx: int32): View`: Returns the view at the given index
- `setAlignment(this: Layout, idx: int32, alignment: int32): void`: Set the alignment of the view at the given index
- `padding(this: Layout, top: int32, left: int32, right: int32, bottom: int32): Layout`: Set the padding of `this` layout to the given values
- `onCreate(this: Layout, parent: View): void`: Callback function executed when the view is created
- `onDestroy(this: Layout): void`: Callback function executed when the view is destroyed
- `onKeyEvent(this: Layout, event: KeyEvent): State`: Callback function executed when the view receives a KeyEvent: press or release
- `onTouchEvent(this: Layout, rect: Rect, event: TouchEvent): State`: Callback function executed when the view receives a TouchEvent: press, motion or release
- `onMeasure(this: Layout, measured: Rect): bool`: Callback function executed when the view needs to be measured
- `onDraw(this: Layout, offs: Image, parent: Rect): void`: Callback function executed when the view needs to be drawn
- `Custom(rowsCols: int32, alignment: int32, items: View[]): Layout`: Create a custom Layout containing the given `items` all of them aligned as `alignment`
- `Row(items: View[]): Layout`: Create a Row-layout containing the given `items` arranged horizontally in a single row
- `Column(items: View[]): Layout`: Create a Column-layout containing the given `items` arranged vertically in a single column
- `Stack(items: View[]): Layout`: Create a Stack-layout containing the given `items` stacked on top of each other

## Widget
Widget is a special kind of layout, which can clip the content of its elements.
 * a widget may clip its children and apply scrolling to them, even show a scrollbar.

- `addItems(this: Widget, align: int32, views: View[]): Widget`: Append the given `views` to `this` widget with the given `alignment`
- `addItems(this: Widget, views: View[]): Widget`: Append the given `views` to `this` widget
- `focus(this: Widget, value: View): Widget`: Set the initial focused view in the widget
- `padding(this: Layout, top: int32, left: int32, right: int32, bottom: int32): Layout`: Set the padding of `this` widget to the given values
- `padding(this: Widget, vertical: int32, horizontal: int32): Layout`: Set the padding of `this` widget to the given values
- `padding(this: Widget, padding: int32): Widget`: Set the padding of `this` widget to the given value
- `onCreate(this: Widget, parent: View): void`: Callback function executed when the view is created
- `onKeyEvent(this: Widget, event: KeyEvent): State`: Callback function executed when the view receives a KeyEvent: press or release
- `onMeasure(this: Widget, measured: Rect): bool`: Callback function executed when the view needs to be measured
- `onDraw(this: Widget, offs: Image, rect: Rect): void`: Callback function executed when the view needs to be drawn

## cmplGfx/lib/ui/micro.ui.cmpl
- `getWidget(view: View): Widget`: Return the nearest widget containing the given `view`
- `focus(view: View): void`: Set the given `view` focused
- `isFocused(view: View): bool`: Returns whether the given `view` is in focused state or not.
- `hasFocused(this: Layout, items: View[]): bool`: Returns whether the focused view is one of the given `items`, searching in nested layouts; with no `items`, whether any view is focused
- `isPressed(view: View): bool`: Returns whether the given `view` is in pressed state or not.
- `hasPressed(this: Layout, items: View[]): bool`: Returns whether the pressed view is one of the given `items`, searching in nested layouts; with no `items`, whether any view is pressed
- `invalidate(view: View, layout: bool): void`: Invalidate the view, so the next frame it will be redrawn, and measured in case `layout` is set to true
- `invalidate(view: View): void`: Invalidate the view, so the next frame it will be measured and redrawn

## Button
Button is a view that can be clicked with the mouse, by touch, or with the space and enter keys

- `text: char[]`: The text displayed on the button
- `onClick(this: Button, clicks: int32): void`: Callback function executed when the view is clicked
- `onClick(this: Button, clicks: int32): void`: Callback function executed when the view is clicked
- `onKeyEvent(this: Button, event: KeyEvent): State`: Callback function executed when the view receives a KeyEvent: press or release
- `onTouchEvent(this: Button, rect: Rect, event: TouchEvent): State`: Callback function executed when the view receives a TouchEvent: press, motion or release
- `onMeasure(this: Button, rect: Rect): bool`: Callback function executed when the view needs to be measured
- `onDraw(this: Button, offs: Image, rect: Rect): void`: Callback function executed when the view needs to be drawn

## Slider
Slider is a button with a numeric value, adjusted by dragging, or with the `+`, `-` and digit keys

- `value: float64`: The current value
- `minimum: float64`: The lowest value
- `maximum: float64`: The highest value
- `stepSize: float64`: Values are rounded to multiples of this step, `0` disables rounding
- `shiftFactor: float64`: Multiplier applied to the change of the value while holding the shift key, for finer adjustments
- `clickToFocus: bool`: clicking on a non focused view will not update the value, it will just focus it.
- `onUpdate(this: Slider, value: float64): bool`: Callback function executed when the value changes, clamps and rounds the value, returns true if it changed
- `onUpdate(this: Slider, value: float64): bool`: Callback function executed when the value changes, clamps and rounds the value, returns true if it changed
- `onClick(this: Slider, clicks: int32): void`: Callback function executed when the view is clicked
- `onCreate(this: Slider, parent: View): void`: Callback function executed when the view is created
- `onKeyEvent(this: Slider, event: KeyEvent): State`: Callback function executed when the view receives a KeyEvent: press or release
- `onTouchEvent(this: Slider, rect: Rect, event: TouchEvent): State`: Callback function executed when the view receives a TouchEvent: press, motion or release
- `onMeasure(this: Slider, rect: Rect): bool`: Callback function executed when the view needs to be measured
- `onDrawSlider(this: Slider, offs: Image, rect: Rect): void`: Draw the value indicator bar of the slider
- `onDrawSlider(this: Slider, offs: Image, rect: Rect): void`: Draw the value indicator bar of the slider
- `onDraw(this: Slider, offs: Image, rect: Rect): void`: Callback function executed when the view needs to be drawn

## Switch
Switch is a button cycling through a list of states on each click

- `states: char[][]`: The text of each state, an empty list makes a graphical on/off switch
- `selected: int32`: The index of the selected state
- `clickToFocus: bool`: clicking on a non focused view will not change the value, it will just focus it.
- `onUpdate(this: Switch, selected: int32): bool`: update the selected value of this switch
- `onUpdate(this: Switch, selected: int32): bool`: update the selected value of this switch
- `onClick(this: Switch, clicks: int32): void`: Callback function executed when the view is clicked
- `onCreate(this: Switch, parent: View): void`: Callback function executed when the view is created
- `onKeyEvent(this: Switch, event: KeyEvent): State`: Callback function executed when the view receives a KeyEvent: press or release
- `onMeasure(this: Switch, rect: Rect): bool`: Callback function executed when the view needs to be measured
- `onDraw(this: Switch, offs: Image, rect: Rect): void`: Callback function executed when the view needs to be drawn

## Input
Input is a single line, editable text field.
 * the text is stored in a caller supplied, null terminated buffer,
 * the capacity of the field is the length of the `text` slice.

- `text: char[]`: Editable, null terminated text buffer; its `.length` is the capacity.
- `placeholder: char[]`: Text shown dimmed when the field is empty.
- `cursor: int32`: Position of the caret, an index into `text` between 0 and its length.
- `scroll: int32`: Horizontal pixel offset used to keep the caret visible.
- `clickToFocus: bool`: clicking on a non focused field will not move the caret, it will just focus it.
- `onChange(this: Input): void`: Callback function executed when the text changes
- `onChange(this: Input): void`: Callback function executed when the text changes
- `onSubmit(this: Input): void`: Callback function executed when the enter key is pressed
- `onSubmit(this: Input): void`: Callback function executed when the enter key is pressed
- `caretX(this: Input, cursor: int32): int32`: Pixel offset of the caret placed before the character at `cursor`.
- `caretAt(this: Input, rect: Rect, px: int32): int32`: Index of the caret nearest to the horizontal position `px`.
- `insert(this: Input, chr: char): bool`: Insert a character at the caret position, growing the text.
- `delete(this: Input, backspace: bool): bool`: Remove the character at the caret, or the one before it if `backspace` is set.
- `setText(this: Input, value: char[]): void`: Replace the content of the field with the given `value`, placing the caret at the end.
- `onCreate(this: Input, parent: View): void`: Callback function executed when the view is created
- `onKeyEvent(this: Input, event: KeyEvent): State`: Callback function executed when the view receives a KeyEvent: press or release
- `onTouchEvent(this: Input, rect: Rect, event: TouchEvent): State`: Callback function executed when the view receives a TouchEvent: press, motion or release
- `onMeasure(this: Input, rect: Rect): bool`: Callback function executed when the view needs to be measured
- `onDraw(this: Input, offs: Image, rect: Rect): void`: Callback function executed when the view needs to be drawn

## Window
Extends the native `Window` with methods to show and run widgets

- `EVENT_PROCESSED: typename`: events with action having this bit set were processed by the widget
- `show(offscreen: Image, onEvent(action: int32, button: int32, x: int32, y: int32): int32, widget: Widget): void`: Show the given `widget` in a window drawn to the `offscreen` image
- `show(widget: Widget): void`: Show the given `widget` in a window

## Panel
Panel is a layout that can be shown or hidden, hidden panels take no space

- `visible: bool`: Whether the panel is visible
- `visible(this: Panel, value: bool): bool`: Show or hide the panel, returns true if the visibility changed
- `onMeasure(this: Panel, rect: Rect): bool`: Callback function executed when the view needs to be measured

## Select
Select is a dropdown: a header showing the selected option, which expands a list of options when clicked

- `text: char[]`: The text displayed on the header
- `selectedData(this: Select): variant`: Returns the data of the selected option, or `null` if nothing is selected
- `setSelectedIndex(this: Select, index: int32): Select`: Select the option at the given `index`
- `addItems(this: Select, align: int32, views: View[]): Select`: Add the given views as options, aligned with `align`
- `addItem(this: Select, text: char[], data: variant): Select`: Add an option displaying `text`, having the given `data`
- `addItem(this: Select, text: char[]): Select`: Add an option displaying `text`, using the text also as its data

## DebugView
debug view to count draws

- `measures: int64`: Number of measures, initialize to `-1` to ignore
- `draws: int64`: Number of draw calls, initialize to `-1` to ignore
- `restartTimer(this: DebugView): void`: Restart the time measurement until this view is drawn on screen
- `startTimer(this: DebugView): void`: Start the time measurement, if not already started, until this view is drawn on screen
- `onTouchEvent(this: DebugView, rect: Rect, event: TouchEvent): State`: Callback function executed when the view receives a TouchEvent: press, motion or release
- `onCreate(this: DebugView, parent: View): void`: Callback function executed when the view is created
- `onMeasure(this: DebugView, rect: Rect): bool`: Callback function executed when the view needs to be measured
- `onDraw(this: DebugView, offs: Image, rect: Rect): void`: Callback function executed when the view needs to be drawn

## Histogram
Histogram is a view displaying the red, green, blue and luminosity distribution of an image

- `lines: bool`: display lines instead of bars
- `color: bool`: Draw only the luminosity with the colors of the entries, instead of the separate red, green, blue and luminosity channels
- `data: argb[256]`: The values of the histogram: red, green, blue, and luminosity in the alpha channel
- `update(this: Histogram, image: Image): void`: update the histogram computed from the given image
- `onUpdate(this: Histogram, idx: int32, value: argb): bool`: update the histogram value at the given lookup table index
- `onUpdate(this: Histogram, idx: int32, value: argb): bool`: update the histogram value at the given lookup table index
- `onCreate(this: Histogram, parent: View): void`: Callback function executed when the view is created
- `onTouchEvent(this: Histogram, rect: Rect, event: TouchEvent): State`: Callback function executed when the view receives a TouchEvent: press, motion or release
- `onKeyEvent(this: Histogram, event: KeyEvent): State`: Callback function executed when the view receives a KeyEvent: press or release
- `onMeasure(this: Histogram, rect: Rect): bool`: Callback function executed when the view needs to be measured
- `onDraw(this: Histogram, offs: Image, rect: Rect): void`: Callback function executed when the view needs to be drawn

## FxWidget
Widget used to display controls in image processing demos

- `showOriginal: bool`: Whether the original, unprocessed image should be shown, set while `\` is pressed, or `[` or `]` is held down
- `align: int32`: Alignment of the controls, `[` aligns them to the top left, `]` to the top right
- `margin: int32`: Distance of the controls from the edges
- `radius: int32`: Corner radius of the background
- `onTouchEvent(this: FxWidget, rect: Rect, event: TouchEvent): State`: Callback function executed when the view receives a TouchEvent: press, motion or release
- `onKeyEvent(this: FxWidget, event: KeyEvent): State`: Callback function executed when the view receives a KeyEvent: press or release
- `onMeasure(this: FxWidget, measured: Rect): bool`: Callback function executed when the view needs to be measured
- `onDraw(this: FxWidget, offs: Image, rect: Rect): void`: Callback function executed when the view needs to be drawn
- `showOriginal(anyWidget: Widget): bool`: returns true if the widget needs to show the original image

## cmplGfx/lib/ui/micro.ui.widget.fx.cmpl
- `Widget(width: int32, height: int32, style: Style): Widget`: Create a widget with the given `width`, `height` and `style`
- `Widget(width: int32, height: int32): Widget`: Create a widget with the given `width`, `height` and a default style
- `FxWidget(views: View[]): FxWidget`: Create a widget with the given `views` used in image processing demos

# Virtual machine
The virtual machine is a very simple stack based one, with a minimal set of registers.

## Memory layout
The memory is organized in 4 segments:

- meta\[r--]:
	- identifiers and string constants (are emitted in the compilation phase).
	- typename and variable metadata (are emitted in the compilation phase).
	- enumeration values and initializer literals.

- code\[r-x]:
	- functions implementations.

- data\[rw-]:
	- static variables.

- heap\[rw-]:
	- heap memory
	- stack(s).

In the compilation phase the lower part of the memory is filled with persistent data which remains available for code execution,
while the upper part is filled with token structures and other temporary data, that will be overwritten on code execution.

## Registers
The virtual machine registers:
- ip: Instruction pointer
- sp: Stack pointer
- tp: Trace pointer (used only when debugging/profiling)
- bp: Base pointer constant (to detect stack overflow)
- ss: Stack size constant

## Instruction set

The instructions of the vm are grouped by type:
* `0x00 ... 0x0f` system instructions
* `0x10 ... 0x1f` stack manipulation
* `0x20 ... 0x2f` memory manipulation
* `0x30 ... 0x3f` unsigned 32-bit
* `0x40 ... 0x4f` unsigned 64-bit
* `0x50 ... 0x5f` signed 32-bit
* `0x60 ... 0x6f` signed 64-bit
* `0x70 ... 0x7f` float 32-bit
* `0x80 ... 0x8f` float 64-bit
* `0x90 ... 0x9f` packed128: float32x4
* `0xa0 ... 0xaf` packed128: float64x2, float32x4, int64x2, int32x4, int16x8, int8x16
* `0xb0 ... 0xbf` packed256: float64x4, float32x8, int64x4, int32x8, int16x16, int8x32
* `0xc0 ... 0xcf` u32u64ext: todo
* `0xd0 ... 0xdf` i32i64ext: todo
* `0xe0 ... 0xef` f32f64ext: todo
* `0xf0 ... 0xff` reserved

### Instruction matrix

|        | 0        | 1        | 2         | 3         | 4         | 5        | 6         | 7        | 8         | 9         | a         | b          | c         | d         | e         | f          |
|:------:|:---------|:---------|:----------|:----------|:----------|:---------|:----------|:---------|:----------|:----------|:----------|:-----------|:----------|:----------|:----------|:-----------|
| 0:sys  | nop      | nfc      | call      | ret       | jmp       | jnz      | jz        | -        | -         | not.u32   | inc.i32   | mad.u32    | chk.idx   | chk.len   | inc.sp    | copy.mem   |
| 1:stk  | load.c32 | load.c64 | load.z32  | load.z64  | load.z128 | dup.x32  | dup.x64   | dup.x128 | set.x32   | set.x64   | set.x128  | mov.x32    | mov.x64   | mov.x128  | load.sp   | load.ref   |
| 2:mem  | load.is8 | load.iu8 | load.is16 | load.iu16 | load.i32  | load.i64 | load.i128 | store.i8 | store.i16 | store.i32 | store.i64 | store.i128 | load.m32  | load.m64  | store.m32 | store.m64  |
| 3:u32  | cmt.u32  | and.u32  | or.u32    | mul.u32   | div.u32   | rem.u32  | -         | -        | clt.u32   | cgt.u32   | shl.u32   | shr.u32    | sar.u32   | xor.u32   | u32.2i64  | u32.       |
| 4:u64  | cmt.u64  | and.u64  | or.u64    | mul.u64   | div.u64   | rem.u64  | -         | -        | clt.u64   | cgt.u64   | shl.u64   | shr.u64    | sar.u64   | xor.u64   | -         | -          |
| 5:i32  | neg.i32  | add.i32  | sub.i32   | mul.i32   | div.i32   | rem.i32  | -         | ceq.i32  | clt.i32   | cgt.i32   | i32.2bool | i32.2i64   | i32.2f32  | i32.2f64  | -         | -          |
| 6:i64  | neg.i64  | add.i64  | sub.i64   | mul.i64   | div.i64   | rem.i64  | -         | ceq.i64  | clt.i64   | cgt.i64   | i64.2i32  | i64.2bool  | i64.2f32  | i64.2f64  | -         | -          |
| 7:f32  | neg.f32  | add.f32  | sub.f32   | mul.f32   | div.f32   | rem.f32  | -         | ceq.f32  | clt.f32   | cgt.f32   | f32.2i32  | f32.2i64   | f32.2bool | f32.2f64  | -         | load.f32   |
| 8:f64  | neg.f64  | add.f64  | sub.f64   | mul.f64   | div.f64   | rem.f64  | -         | ceq.f64  | clt.f64   | cgt.f64   | f64.2i32  | f64.2i64   | f64.2f32  | f64.2bool | -         | load.f64   |
| 9:p128 | neg.v4f  | add.v4f  | sub.v4f   | mul.v4f   | div.v4f   | -        | -         | ceq.v4f  | min.v4f   | max.v4f   | dp3.v4f   | dp4.v4f    | dph.v4f   | swz.p4x   | load.m128 | store.m128 |
| a:p128 | neg.v2d  | add.v2d  | sub.v2d   | mul.v2d   | div.v2d   | -        | -         | ceq.v2d  | min.v2d   | max.v2d   | -         | -          | -         | -         | -         | -          |
| b:p256 | -        | -        | -         | -         | -         | -        | -         | -        | -         | -         | -         | -          | -         | -         | -         | -          |
| c:p512 | -        | -        | -         | -         | -         | -        | -         | -        | -         | -         | -         | -          | -         | -         | -         | -          |
| d:extU | -        | -        | -         | -         | -         | -        | -         | -        | -         | -         | -         | -          | -         | -         | -         | -          |
| e:extI | -        | -        | -         | -         | -         | -        | -         | -        | -         | -         | -         | -          | -         | -         | -         | -          |
| f:extF | -        | -        | -         | -         | -         | -        | -         | -        | -         | -         | -         | -          | -         | -         | -         | -          |

### Notation

Each instruction is described by a short summary, its operation in pseudocode, its details, the traps it can raise, and additional notes.

* a stack slot is 32 bits wide; 64-bit values occupy 2 slots, 128-bit values occupy 4 slots.
* the stack grows downwards: the slot pushed last is on top and has the lowest address.
* `pop()` and `push(x)` remove and add as many slots as the type of the value requires, e.g. `int64 a = pop()` removes 2 slots.
* `sp(n)` is the slot at position `n` from the top of the stack without removing it, `sp(0)` being the top; `drop(n)` removes `n` slots.
* `mem[addr].t` is the value of type `t` stored at memory address `addr`; `x32` denotes a raw 32-bit value with no particular type.
* `imm.t` is the immediate operand encoded in the instruction after the opcode: `imm.u8`, `imm.i24`, `imm.u24`, `imm.x32`, `imm.x64`.
* `IP` is the instruction pointer, `SP` is the stack pointer.
* `trap(error)` aborts the execution with the given error, `halt()` stops the execution of the virtual machine.
* Stack effect: `[…, a, b] → […, c]` shows the top of the stack before and after the instruction, the rightmost element being the top.
  A typed value is named once, regardless of how many slots it occupies; the slot counts are given by the stack inputs and outputs.
* vector lane `x` (lane `0`) is stored at the lowest address, so it is the top slot when the vector is on the stack.
* Traps lists the errors the instruction can raise; instructions without a Traps section never abort the execution.

### Instruction `nop`

Does nothing.

```
// no operation
```

Details:
* Instruction code: `0x00`
* Instruction length: `1` byte
* Stack inputs: `0` slots
* Stack outputs: `0` slots
* Immediate operand: none
* Stack effect: `[…] → […]`

### Instruction `nfc`

Calls a native ("C") function registered by the compiler or an extension.

```
libc nfc = nativeCalls[imm.u24]
nfc.call(sp(0) ... sp(nfc.in - 1))
SP -= nfc.out - nfc.in
if (imm.u24 == 0) {
    halt()
}
```

Details:
* Instruction code: `0x01`
* Instruction length: `4` bytes
* Stack inputs: `0` slots
* Stack outputs: `0` slots
* Immediate operand: `imm.u24`: index of the native function in the native call table
* Stack effect: `[…, result, params] → […, result]`

Traps:
* `nativeCallError`: the native function returns an error.

Notes:
* the stack inputs and outputs depend on the called function: its parameters and its result.
* the native function receives the top `nfc.in` slots as its arguments, the result space is reserved by the caller below the arguments.
* after the call the stack pointer is adjusted by `nfc.in - nfc.out` slots, which normally removes the parameters.
* the native function with index `0` is always `halt()`, calling it stops the execution of the virtual machine.

### Instruction `call`

Calls the function whose address is on top of the stack, replacing it with the return address.

```
fn = pop()
push(IP)
IP = fn
```

Details:
* Instruction code: `0x02`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, result, params, fn] → […, result, params, IP]`

Traps:
* `illegalInstruction`: the function address is `0`, or past the end of the memory.

Notes:
* the pushed return address `IP` is the address of the instruction following the `call`.

### Instruction `ret`

Returns from a function call, by jumping to the address on top of the stack (indirect jump).

```
IP = pop()
```

Details:
* Instruction code: `0x03`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `0` slots
* Immediate operand: none
* Stack effect: `[…, IP] → […]`

Traps:
* `illegalInstruction`: the return address is `0`, or past the end of the memory.

### Instruction `jmp`

Jumps unconditionally to an address relative to the current instruction.

```
IP += imm.i24
```

Details:
* Instruction code: `0x04`
* Instruction length: `4` bytes
* Stack inputs: `0` slots
* Stack outputs: `0` slots
* Immediate operand: `imm.i24`: signed 24-bit offset relative to the start of this instruction
* Stack effect: `[…] → […]`

### Instruction `jnz`

Pops the top of the stack, and jumps if it is non-zero.

```
i32 c = pop()
if (c != 0) {
    IP += imm.i24
}
```

Details:
* Instruction code: `0x05`
* Instruction length: `4` bytes
* Stack inputs: `1` slot
* Stack outputs: `0` slots
* Immediate operand: `imm.i24`: signed 24-bit offset relative to the start of this instruction
* Stack effect: `[…, c] → […]`

### Instruction `jz`

Pops the top of the stack, and jumps if it is zero.

```
i32 c = pop()
if (c == 0) {
    IP += imm.i24
}
```

Details:
* Instruction code: `0x06`
* Instruction length: `4` bytes
* Stack inputs: `1` slot
* Stack outputs: `0` slots
* Immediate operand: `imm.i24`: signed 24-bit offset relative to the start of this instruction
* Stack effect: `[…, c] → […]`

### Instruction `not.u32`

Computes the logical negation of the top of the stack.

```
u32 a = pop()
push(a == 0 ? 1 : 0)
```

Details:
* Instruction code: `0x09`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a] → […, !a]`

### Instruction `inc.i32`

Adds a signed immediate value to the top of the stack.

```
i32 a = pop()
push(a + imm.i24)
```

Details:
* Instruction code: `0x0a`
* Instruction length: `4` bytes
* Stack inputs: `1` slot
* Stack outputs: `1` slot
* Immediate operand: `imm.i24`: signed 24-bit increment
* Stack effect: `[…, a] → […, a + imm]`

Notes:
* the addition wraps around on overflow.

### Instruction `mad.u32`

Multiplies the top of the stack by an immediate value, and adds the result to the value below it.

```
u32 b = pop()
u32 a = pop()
push(a + b * imm.i24)
```

Details:
* Instruction code: `0x0b`
* Instruction length: `4` bytes
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: `imm.i24`: signed 24-bit multiplier
* Stack effect: `[…, a, b] → […, a + b * imm]`

Notes:
* the computation is done in unsigned 32-bit arithmetic, and wraps around on overflow.
* used to compute the address of an element: `reference + index * elementSize`.

### Instruction `chk.idx`

Checks the bounds of an array index, then computes the address of the element.

```
u32 index = pop()
u32 reference = pop()
u32 length = pop()
if (index >= length) {
    trap(illegalArrayAccess)
}
push(reference + index * imm.i24)
```

Details:
* Instruction code: `0x0c`
* Instruction length: `4` bytes
* Stack inputs: `3` slots
* Stack outputs: `1` slot
* Immediate operand: `imm.i24`: size of an element in bytes
* Stack effect: `[…, length, reference, index] → […, reference + index * imm]`

Traps:
* `illegalArrayAccess`: `index` is greater than or equal to `length` (compared as unsigned, so negative indices also trap).

### Instruction `chk.len`

Checks the bounds of a slice (`a[...n]`), keeping the requested count as the length of the resulting slice.

```
u32 reference = pop()
u32 length = pop()
u32 count = sp(0)
if (count > length) {
    trap(illegalArrayAccess)
}
push(reference)
```

Details:
* Instruction code: `0x0d`
* Instruction length: `1` byte
* Stack inputs: `3` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, count, length, reference] → […, count, reference]`

Traps:
* `illegalArrayAccess`: `count` is greater than the `length` of the sliced array (compared as unsigned).

Notes:
* `count` is not popped, it remains on the stack as the length of the resulting slice.

### Instruction `inc.sp`

Allocates or frees memory on the stack, by moving the stack pointer.

```
if (imm.i24 > 0) {
    reserve(imm.i24 / 4)    // push imm / 4 uninitialized slots
}
else {
    drop(-imm.i24 / 4)      // pop -imm / 4 slots
}
```

Details:
* Instruction code: `0x0e`
* Instruction length: `4` bytes
* Stack inputs: `0` slots
* Stack outputs: `0` slots
* Immediate operand: `imm.i24`: signed number of bytes to allocate (positive) or free (negative), must be a multiple of `4`
* Stack effect: `[…] → […, a1, …, an]` when allocating, `[…, a1, …, an] → […]` when freeing

Traps:
* `illegalInstruction`: `imm` is not a multiple of the stack slot size (`4` bytes).
* `stackOverflow`: there is no room on the stack for the allocated slots.

Notes:
* the stack inputs and outputs depend on the immediate operand: `imm / 4` slots are allocated, or `-imm / 4` slots are freed.
* the allocated slots are uninitialized, except in builds with `DEBUGGING` enabled, where they are zero-filled.

### Instruction `copy.mem`

Copies a block of memory between the two addresses on top of the stack.

```
u32 a = pop()
u32 b = pop()
if (imm.i24 < 0) {
    memmove(dst: a, src: b, -imm.i24)
}
else {
    memmove(dst: b, src: a, imm.i24)
}
```

Details:
* Instruction code: `0x0f`
* Instruction length: `4` bytes
* Stack inputs: `2` slots
* Stack outputs: `0` slots
* Immediate operand: `imm.i24`: number of bytes to copy, the sign selects the direction
* Stack effect: `[…, b, a] → […]`

Traps:
* `illegalMemoryAccess`: the source address is `null`, or the source block extends past the end of the memory.
* `illegalMemoryAccess`: the destination address points into the read-only region (constants and code), or the destination block extends past the end of the memory.

Notes:
* with a positive count the top of the stack is the source, with a negative count it is the destination.
* overlapping blocks are copied correctly (`memmove` semantics).
* temporary instruction, intended to be replaced by the native function call `pointer.move(dst, src, count)`.

### Instruction `load.c32`

Pushes a 32-bit constant, stored in the instruction, onto the stack.

```
push(imm.x32)
```

Details:
* Instruction code: `0x10`
* Instruction length: `5` bytes
* Stack inputs: `0` slots
* Stack outputs: `1` slot
* Immediate operand: `imm.x32`: 32-bit constant
* Stack effect: `[…] → […, imm]`

Traps:
* `stackOverflow`: there is no room on the stack for the new slot.

Notes:
* the constant is type-agnostic, it is used for any 32-bit value: `i32`, `u32`, `f32`, references, ...

### Instruction `load.c64`

Pushes a 64-bit constant, stored in the instruction, onto the stack.

```
push(imm.x64)
```

Details:
* Instruction code: `0x11`
* Instruction length: `9` bytes
* Stack inputs: `0` slots
* Stack outputs: `2` slots
* Immediate operand: `imm.x64`: 64-bit constant
* Stack effect: `[…] → […, imm]`

Traps:
* `stackOverflow`: there is no room on the stack for the new slots.

Notes:
* the 8 immediate bytes are copied verbatim: the top slot receives bytes `0‥3`, the slot below it receives bytes `4‥7`.
* the constant is type-agnostic, it is used for any 64-bit value: `i64`, `u64`, `f64`, ...

### Instruction `load.z32`

Pushes a 32-bit zero onto the stack.

```
push(0)
```

Details:
* Instruction code: `0x12`
* Instruction length: `1` byte
* Stack inputs: `0` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…] → […, 0]`

Traps:
* `stackOverflow`: there is no room on the stack for the new slot.

### Instruction `load.z64`

Pushes a 64-bit zero (two zero slots) onto the stack.

```
push(0)
push(0)
```

Details:
* Instruction code: `0x13`
* Instruction length: `1` byte
* Stack inputs: `0` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…] → […, 0, 0]`

Traps:
* `stackOverflow`: there is no room on the stack for the new slots.

### Instruction `load.z128`

Pushes a 128-bit zero (four zero slots) onto the stack.

```
push(0)
push(0)
push(0)
push(0)
```

Details:
* Instruction code: `0x14`
* Instruction length: `1` byte
* Stack inputs: `0` slots
* Stack outputs: `4` slots
* Immediate operand: none
* Stack effect: `[…] → […, 0, 0, 0, 0]`

Traps:
* `stackOverflow`: there is no room on the stack for the new slots.

### Instruction `dup.x32`

Pushes a copy of the 32-bit stack slot at the given position.

```
a = sp(imm.u8)
push(a)
```

Details:
* Instruction code: `0x15`
* Instruction length: `2` bytes
* Stack inputs: `0` slots
* Stack outputs: `1` slot
* Immediate operand: `imm.u8`: position of the slot to copy, `0` being the top of the stack
* Stack effect: `[…, a, …] → […, a, …, a]`

Traps:
* `stackOverflow`: there is no room on the stack for the new slot.

Notes:
* the position is measured before the push, `dup.x32 0` duplicates the top slot: `[…, a] → […, a, a]`.
* the position is not validated at runtime.

### Instruction `dup.x64`

Pushes a copy of the two 32-bit stack slots starting at the given position.

```
a2 = sp(imm.u8 + 0)
a1 = sp(imm.u8 + 1)
push(a1)
push(a2)
```

Details:
* Instruction code: `0x16`
* Instruction length: `2` bytes
* Stack inputs: `0` slots
* Stack outputs: `2` slots
* Immediate operand: `imm.u8`: position of the upper slot to copy, `0` being the top of the stack
* Stack effect: `[…, a1, a2, …] → […, a1, a2, …, a1, a2]`

Traps:
* `stackOverflow`: there is no room on the stack for the new slots.

Notes:
* the position is measured before the push, e.g. `dup.x64 1`: `[…, a1, a2, b] → […, a1, a2, b, a1, a2]`.
* the slots are copied verbatim, keeping their order.
* the position is not validated at runtime.

### Instruction `dup.x128`

Pushes a copy of the four 32-bit stack slots starting at the given position.

```
a4 = sp(imm.u8 + 0)
a3 = sp(imm.u8 + 1)
a2 = sp(imm.u8 + 2)
a1 = sp(imm.u8 + 3)
push(a1)
push(a2)
push(a3)
push(a4)
```

Details:
* Instruction code: `0x17`
* Instruction length: `2` bytes
* Stack inputs: `0` slots
* Stack outputs: `4` slots
* Immediate operand: `imm.u8`: position of the upper slot to copy, `0` being the top of the stack
* Stack effect: `[…, a1, a2, a3, a4, …] → […, a1, a2, a3, a4, …, a1, a2, a3, a4]`

Traps:
* `stackOverflow`: there is no room on the stack for the new slots.

Notes:
* the position is measured before the push, e.g. `dup.x128 0`: `[…, a1, a2, a3, a4] → […, a1, a2, a3, a4, a1, a2, a3, a4]`.
* the slots are copied verbatim, keeping their order.
* the position is not validated at runtime.

### Instruction `set.x32`

Overwrites the 32-bit stack slot at the given position with the top slot, then drops the top slot.

```
a = sp(0)
sp(imm.u8) = a
drop(min(imm.u8, 1))
```

Details:
* Instruction code: `0x18`
* Instruction length: `2` bytes
* Stack inputs: `1` slot
* Stack outputs: `0` slots
* Immediate operand: `imm.u8`: position of the slot to overwrite, `0` being the top of the stack
* Stack effect: `[…, b, …, a] → […, a, …]`

Notes:
* the position is measured before the drop, e.g. `set.x32 1`: `[…, b, a] → […, a]`.
* `set.x32 0` leaves the stack unchanged.
* the position is not validated at runtime.

### Instruction `set.x64`

Overwrites the two 32-bit stack slots at the given position with the two top slots, then drops the top slots.

```
a2 = sp(0)
a1 = sp(1)
sp(imm.u8 + 0) = a2
sp(imm.u8 + 1) = a1
drop(min(imm.u8, 2))
```

Details:
* Instruction code: `0x19`
* Instruction length: `2` bytes
* Stack inputs: `2` slots
* Stack outputs: `0` slots
* Immediate operand: `imm.u8`: position of the upper slot to overwrite, `0` being the top of the stack
* Stack effect: `[…, b1, b2, …, a1, a2] → […, a1, a2, …]`

Notes:
* the position is measured before the drop, e.g. `set.x64 3`: `[…, c, b, a, a1, a2] → […, a1, a2, a]`.
* when the position is less than `2` the destination overlaps the source: the value is moved `imm.u8` slots down and only `imm.u8` slots are dropped, `set.x64 0` leaves the stack unchanged.
* the position is not validated at runtime.

### Instruction `set.x128`

Overwrites the four 32-bit stack slots at the given position with the four top slots, then drops the top slots.

```
a4 = sp(0)
a3 = sp(1)
a2 = sp(2)
a1 = sp(3)
sp(imm.u8 + 0) = a4
sp(imm.u8 + 1) = a3
sp(imm.u8 + 2) = a2
sp(imm.u8 + 3) = a1
drop(min(imm.u8, 4))
```

Details:
* Instruction code: `0x1a`
* Instruction length: `2` bytes
* Stack inputs: `4` slots
* Stack outputs: `0` slots
* Immediate operand: `imm.u8`: position of the upper slot to overwrite, `0` being the top of the stack
* Stack effect: `[…, b1, b2, b3, b4, …, a1, a2, a3, a4] → […, a1, a2, a3, a4, …]`

Notes:
* the position is measured before the drop, e.g. `set.x128 4`: `[…, b1, b2, b3, b4, a1, a2, a3, a4] → […, a1, a2, a3, a4]`.
* when the position is less than `4` the destination overlaps the source: the value is moved `imm.u8` slots down and only `imm.u8` slots are dropped, `set.x128 0` leaves the stack unchanged.
* the position is not validated at runtime.

### Instruction `mov.x32`

Copies a 32-bit stack slot from the source position to the destination position.

```
a = sp(imm.src)
sp(imm.dst) = a
```

Details:
* Instruction code: `0x1b`
* Instruction length: `3` bytes
* Stack inputs: `0` slots
* Stack outputs: `0` slots
* Immediate operand: `imm.dst`, `imm.src`: positions of the destination and source slots, `0` being the top of the stack
* Stack effect: `[…] → […]`, the slot at `imm.dst` is overwritten

Notes:
* e.g. `mov.x32 3, 1`: `[…, a, b, c, d] → […, c, b, c, d]`.
* the positions are not validated at runtime.

### Instruction `mov.x64`

Copies two 32-bit stack slots from the source position to the destination position.

```
a2 = sp(imm.src + 0)
a1 = sp(imm.src + 1)
sp(imm.dst + 0) = a2
sp(imm.dst + 1) = a1
```

Details:
* Instruction code: `0x1c`
* Instruction length: `3` bytes
* Stack inputs: `0` slots
* Stack outputs: `0` slots
* Immediate operand: `imm.dst`, `imm.src`: positions of the upper destination and source slots, `0` being the top of the stack
* Stack effect: `[…] → […]`, the slots at `imm.dst` and `imm.dst + 1` are overwritten

Notes:
* e.g. `mov.x64 4, 0`: `[…, a1, a2, b1, b2, c1, c2] → […, c1, c2, b1, b2, c1, c2]`.
* the positions are not validated at runtime.

### Instruction `mov.x128`

Copies four 32-bit stack slots from the source position to the destination position.

```
a4 = sp(imm.src + 0)
a3 = sp(imm.src + 1)
a2 = sp(imm.src + 2)
a1 = sp(imm.src + 3)
sp(imm.dst + 0) = a4
sp(imm.dst + 1) = a3
sp(imm.dst + 2) = a2
sp(imm.dst + 3) = a1
```

Details:
* Instruction code: `0x1d`
* Instruction length: `3` bytes
* Stack inputs: `0` slots
* Stack outputs: `0` slots
* Immediate operand: `imm.dst`, `imm.src`: positions of the upper destination and source slots, `0` being the top of the stack
* Stack effect: `[…] → […]`, the slots from `imm.dst` to `imm.dst + 3` are overwritten

Notes:
* e.g. `mov.x128 0, 4`: `[…, a1, a2, a3, a4, b1, b2, b3, b4] → […, a1, a2, a3, a4, a1, a2, a3, a4]`.
* overlapping source and destination ranges are handled correctly.
* the positions are not validated at runtime.

### Instruction `load.sp`

Pushes the address of a stack location, used to take the address of local variables.

```
ref = SP + imm.i24
push(ref)
```

Details:
* Instruction code: `0x1e`
* Instruction length: `4` bytes
* Stack inputs: `0` slots
* Stack outputs: `1` slot
* Immediate operand: `imm.i24`: signed 24-bit byte offset relative to the stack pointer
* Stack effect: `[…] → […, ref]`

Traps:
* `stackOverflow`: there is no room on the stack for the new slot.

Notes:
* `SP` is the address of the top slot before the push.
* the result is a vm memory address, usable with the indirect memory instructions (`load.i32`, `store.i32`, ...).

### Instruction `load.ref`

Pushes a 32-bit reference constant, stored in the instruction, onto the stack.

```
push(imm.x32)
```

Details:
* Instruction code: `0x1f`
* Instruction length: `5` bytes
* Stack inputs: `0` slots
* Stack outputs: `1` slot
* Immediate operand: `imm.x32`: 32-bit reference (vm memory address)
* Stack effect: `[…] → […, ref]`

Traps:
* `stackOverflow`: there is no room on the stack for the new slot.

Notes:
* temporary instruction, executed exactly like `load.c32`; it is expected to be replaced by `load.c32` or `load.c64`.

### Instruction `load.is8`

Loads a signed 8-bit value from the memory address on top of the stack, sign-extended to 32 bits.

```
ref = pop()
i32 val = mem[ref].i8
push(val)
```

Details:
* Instruction code: `0x20`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, ref] → […, val]`

Traps:
* `illegalMemoryAccess`: `ref` is `null`, or `ref + 1` is past the end of the memory.

### Instruction `load.iu8`

Loads an unsigned 8-bit value from the memory address on top of the stack, zero-extended to 32 bits.

```
ref = pop()
u32 val = mem[ref].u8
push(val)
```

Details:
* Instruction code: `0x21`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, ref] → […, val]`

Traps:
* `illegalMemoryAccess`: `ref` is `null`, or `ref + 1` is past the end of the memory.

### Instruction `load.is16`

Loads a signed 16-bit value from the memory address on top of the stack, sign-extended to 32 bits.

```
ref = pop()
i32 val = mem[ref].i16
push(val)
```

Details:
* Instruction code: `0x22`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, ref] → […, val]`

Traps:
* `illegalMemoryAccess`: `ref` is `null`, or `ref + 2` is past the end of the memory.

Notes:
* the alignment of `ref` is not checked.

### Instruction `load.iu16`

Loads an unsigned 16-bit value from the memory address on top of the stack, zero-extended to 32 bits.

```
ref = pop()
u32 val = mem[ref].u16
push(val)
```

Details:
* Instruction code: `0x23`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, ref] → […, val]`

Traps:
* `illegalMemoryAccess`: `ref` is `null`, or `ref + 2` is past the end of the memory.

Notes:
* the alignment of `ref` is not checked.

### Instruction `load.i32`

Loads a 32-bit value from the memory address on top of the stack.

```
ref = pop()
a1 = mem[ref].x32
push(a1)
```

Details:
* Instruction code: `0x24`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, ref] → […, a1]`

Traps:
* `illegalMemoryAccess`: `ref` is `null`, or `ref + 4` is past the end of the memory.

Notes:
* the transfer is type-agnostic, it is used for any 32-bit value: `i32`, `u32`, `f32`, references, ...
* the alignment of `ref` is not checked.

### Instruction `load.i64`

Loads a 64-bit value from the memory address on top of the stack.

```
ref = pop()
a2 = mem[ref + 0].x32
a1 = mem[ref + 4].x32
push(a1)
push(a2)
```

Details:
* Instruction code: `0x25`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, ref] → […, a1, a2]`

Traps:
* `illegalMemoryAccess`: `ref` is `null`, or `ref + 8` is past the end of the memory.
* `stackOverflow`: there is no room on the stack for the extra slot.

Notes:
* the 8 bytes at `ref` are copied verbatim into two stack slots: the top slot `a2` receives bytes `0‥3`, the slot below it `a1` receives bytes `4‥7`.
* the transfer is type-agnostic, it is used for any 64-bit value: `i64`, `u64`, `f64`, ...
* the alignment of `ref` is not checked.

### Instruction `load.i128`

Loads a 128-bit value from the memory address on top of the stack.

```
ref = pop()
a4 = mem[ref +  0].x32
a3 = mem[ref +  4].x32
a2 = mem[ref +  8].x32
a1 = mem[ref + 12].x32
push(a1)
push(a2)
push(a3)
push(a4)
```

Details:
* Instruction code: `0x26`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `4` slots
* Immediate operand: none
* Stack effect: `[…, ref] → […, a1, a2, a3, a4]`

Traps:
* `illegalMemoryAccess`: `ref` is `null`, or `ref + 16` is past the end of the memory.
* `stackOverflow`: there is no room on the stack for the extra slots.

Notes:
* the 16 bytes at `ref` are copied verbatim into four stack slots: the top slot `a4` receives bytes `0‥3`, the deepest slot `a1` receives bytes `12‥15`.
* the transfer is type-agnostic, it is used for any 128-bit value: `v4f`, `v2d`, 16-byte records, ...
* the alignment of `ref` is not checked.

### Instruction `store.i8`

Stores the low 8 bits of a value from the stack to the memory address on top of the stack.

```
ref = pop()
val = pop()
mem[ref].i8 = val
```

Details:
* Instruction code: `0x27`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `0` slots
* Immediate operand: none
* Stack effect: `[…, val, ref] → […]`

Traps:
* `illegalMemoryAccess`: `ref` points into the read-only region (constants and code), or `ref + 1` is past the end of the memory.

Notes:
* the value is truncated to its lowest byte, the upper 24 bits are discarded.

### Instruction `store.i16`

Stores the low 16 bits of a value from the stack to the memory address on top of the stack.

```
ref = pop()
val = pop()
mem[ref].i16 = val
```

Details:
* Instruction code: `0x28`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `0` slots
* Immediate operand: none
* Stack effect: `[…, val, ref] → […]`

Traps:
* `illegalMemoryAccess`: `ref` points into the read-only region (constants and code), or `ref + 2` is past the end of the memory.

Notes:
* the value is truncated to its lowest 16 bits, the upper 16 bits are discarded.
* the alignment of `ref` is not checked.

### Instruction `store.i32`

Stores a 32-bit value from the stack to the memory address on top of the stack.

```
ref = pop()
a1 = pop()
mem[ref].x32 = a1
```

Details:
* Instruction code: `0x29`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `0` slots
* Immediate operand: none
* Stack effect: `[…, a1, ref] → […]`

Traps:
* `illegalMemoryAccess`: `ref` points into the read-only region (constants and code), or `ref + 4` is past the end of the memory.

Notes:
* the transfer is type-agnostic, it is used for any 32-bit value: `i32`, `u32`, `f32`, references, ...
* the alignment of `ref` is not checked.

### Instruction `store.i64`

Stores a 64-bit value from the stack to the memory address on top of the stack.

```
ref = pop()
a2 = pop()
a1 = pop()
mem[ref + 0].x32 = a2
mem[ref + 4].x32 = a1
```

Details:
* Instruction code: `0x2a`
* Instruction length: `1` byte
* Stack inputs: `3` slots
* Stack outputs: `0` slots
* Immediate operand: none
* Stack effect: `[…, a1, a2, ref] → […]`

Traps:
* `illegalMemoryAccess`: `ref` points into the read-only region (constants and code), or `ref + 8` is past the end of the memory.

Notes:
* the 2 stack slots below `ref` are copied verbatim into the 8 bytes at `ref`: the slot pushed last (`a2`) is written to bytes `0‥3`, the slot pushed first (`a1`) to bytes `4‥7`.
* the transfer is type-agnostic, it is used for any 64-bit value: `i64`, `u64`, `f64`, ...
* the alignment of `ref` is not checked.

### Instruction `store.i128`

Stores a 128-bit value from the stack to the memory address on top of the stack.

```
ref = pop()
a4 = pop()
a3 = pop()
a2 = pop()
a1 = pop()
mem[ref +  0].x32 = a4
mem[ref +  4].x32 = a3
mem[ref +  8].x32 = a2
mem[ref + 12].x32 = a1
```

Details:
* Instruction code: `0x2b`
* Instruction length: `1` byte
* Stack inputs: `5` slots
* Stack outputs: `0` slots
* Immediate operand: none
* Stack effect: `[…, a1, a2, a3, a4, ref] → […]`

Traps:
* `illegalMemoryAccess`: `ref` points into the read-only region (constants and code), or `ref + 16` is past the end of the memory.

Notes:
* the 4 stack slots below `ref` are copied verbatim into the 16 bytes at `ref`: the slot pushed last (`a4`) is written to bytes `0‥3`, the slot pushed first (`a1`) to bytes `12‥15`.
* the transfer is type-agnostic, it is used for any 128-bit value: `v4f`, `v2d`, 16-byte records, ...
* the alignment of `ref` is not checked.

### Instruction `load.m32`

Loads a 32-bit value from the memory address given by the immediate operand.

```
a1 = mem[imm.i24].x32
push(a1)
```

Details:
* Instruction code: `0x2c`
* Instruction length: `4` bytes
* Stack inputs: `0` slots
* Stack outputs: `1` slot
* Immediate operand: `imm.i24`: absolute memory address
* Stack effect: `[…] → […, a1]`

Traps:
* `illegalMemoryAccess`: the address is `null` or negative, or `address + 4` is past the end of the memory.
* `stackOverflow`: there is no room on the stack for the new slot.

Notes:
* compressed form of `load.ref address` followed by `load.i32`.
* only the first 8 MiB of the memory is addressable by the 24-bit immediate.
* the alignment of the address is not checked.

### Instruction `load.m64`

Loads a 64-bit value from the memory address given by the immediate operand.

```
a2 = mem[imm.i24 + 0].x32
a1 = mem[imm.i24 + 4].x32
push(a1)
push(a2)
```

Details:
* Instruction code: `0x2d`
* Instruction length: `4` bytes
* Stack inputs: `0` slots
* Stack outputs: `2` slots
* Immediate operand: `imm.i24`: absolute memory address
* Stack effect: `[…] → […, a1, a2]`

Traps:
* `illegalMemoryAccess`: the address is `null` or negative, or `address + 8` is past the end of the memory.
* `stackOverflow`: there is no room on the stack for the new slots.

Notes:
* compressed form of `load.ref address` followed by `load.i64`.
* the 8 bytes at the address are copied verbatim into two stack slots: the top slot `a2` receives bytes `0‥3`, the slot below it `a1` receives bytes `4‥7`.
* only the first 8 MiB of the memory is addressable by the 24-bit immediate.
* the alignment of the address is not checked.

### Instruction `store.m32`

Stores a 32-bit value from the stack to the memory address given by the immediate operand.

```
a1 = pop()
mem[imm.i24].x32 = a1
```

Details:
* Instruction code: `0x2e`
* Instruction length: `4` bytes
* Stack inputs: `1` slot
* Stack outputs: `0` slots
* Immediate operand: `imm.i24`: absolute memory address
* Stack effect: `[…, a1] → […]`

Traps:
* `illegalMemoryAccess`: the address points into the read-only region (constants and code), is negative, or `address + 4` is past the end of the memory.

Notes:
* compressed form of `load.ref address` followed by `store.i32`.
* only the first 8 MiB of the memory is addressable by the 24-bit immediate.
* the alignment of the address is not checked.

### Instruction `store.m64`

Stores a 64-bit value from the stack to the memory address given by the immediate operand.

```
a2 = pop()
a1 = pop()
mem[imm.i24 + 0].x32 = a2
mem[imm.i24 + 4].x32 = a1
```

Details:
* Instruction code: `0x2f`
* Instruction length: `4` bytes
* Stack inputs: `2` slots
* Stack outputs: `0` slots
* Immediate operand: `imm.i24`: absolute memory address
* Stack effect: `[…, a1, a2] → […]`

Traps:
* `illegalMemoryAccess`: the address points into the read-only region (constants and code), is negative, or `address + 8` is past the end of the memory.

Notes:
* compressed form of `load.ref address` followed by `store.i64`.
* the 2 top stack slots are copied verbatim into the 8 bytes at the address: the top slot `a2` is written to bytes `0‥3`, the slot below it `a1` to bytes `4‥7`.
* only the first 8 MiB of the memory is addressable by the 24-bit immediate.
* the alignment of the address is not checked.

### Instruction `cmt.u32`

Inverts each bit of the 32-bit integer on top of the stack (bitwise complement).

```
u32 a = pop()
push(~a)
```

Details:
* Instruction code: `0x30`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a] → […, ~a]`

### Instruction `and.u32`

Computes the bitwise and of the two 32-bit integers on top of the stack.

```
u32 b = pop()
u32 a = pop()
push(a & b)
```

Details:
* Instruction code: `0x31`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a & b]`

### Instruction `or.u32`

Computes the bitwise or of the two 32-bit integers on top of the stack.

```
u32 b = pop()
u32 a = pop()
push(a | b)
```

Details:
* Instruction code: `0x32`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a | b]`

### Instruction `mul.u32`

Multiplies the two unsigned 32-bit integers on top of the stack.

```
u32 b = pop()
u32 a = pop()
push(a * b)
```

Details:
* Instruction code: `0x33`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a * b]`

Notes:
* the result wraps around modulo 2^32.

### Instruction `div.u32`

Divides the two unsigned 32-bit integers on top of the stack.

```
u32 b = pop()
u32 a = pop()
push(a / b)
```

Details:
* Instruction code: `0x34`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a / b]`

Traps:
* `divisionByZero`: the divisor `b` is zero.

### Instruction `rem.u32`

Computes the remainder of the unsigned division of the two 32-bit integers on top of the stack.

```
u32 b = pop()
u32 a = pop()
push(a % b)
```

Details:
* Instruction code: `0x35`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a % b]`

Traps:
* `divisionByZero`: the divisor `b` is zero.

### Instruction `clt.u32`

Compares the two unsigned 32-bit integers on top of the stack, and pushes `1` if the first is less than the second, `0` otherwise.

```
u32 b = pop()
u32 a = pop()
push(a < b)
```

Details:
* Instruction code: `0x38`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a < b]`

### Instruction `cgt.u32`

Compares the two unsigned 32-bit integers on top of the stack, and pushes `1` if the first is greater than the second, `0` otherwise.

```
u32 b = pop()
u32 a = pop()
push(a > b)
```

Details:
* Instruction code: `0x39`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a > b]`

### Instruction `shl.u32`

Shifts the 32-bit integer left by the given number of bits, filling with zeros.

```
u32 b = pop()
u32 a = pop()
push(a << b)
```

Details:
* Instruction code: `0x3a`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a << b]`

Notes:
* the shift count is not masked at runtime: a count of `32` or more is undefined behavior in C (x86 hosts typically use only the low 5 bits of the count).

### Instruction `shr.u32`

Shifts the unsigned 32-bit integer right by the given number of bits, filling with zeros (logical shift).

```
u32 b = pop()
u32 a = pop()
push(a >> b)
```

Details:
* Instruction code: `0x3b`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a >> b]`

Notes:
* the shift count is not masked at runtime: a count of `32` or more is undefined behavior in C (x86 hosts typically use only the low 5 bits of the count).

### Instruction `sar.u32`

Shifts the signed 32-bit integer right by the given number of bits, replicating the sign bit (arithmetic shift).

```
i32 b = pop()
i32 a = pop()
push(a >> b)
```

Details:
* Instruction code: `0x3c`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a >> b]`

Notes:
* both operands are treated as signed 32-bit integers.
* the shift count is not masked at runtime: a count of `32` or more, or a negative count, is undefined behavior in C (x86 hosts typically use only the low 5 bits of the count).

### Instruction `xor.u32`

Computes the bitwise exclusive or of the two 32-bit integers on top of the stack.

```
u32 b = pop()
u32 a = pop()
push(a ^ b)
```

Details:
* Instruction code: `0x3d`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a ^ b]`

### Instruction `u32.2i64`

Converts the unsigned 32-bit integer on top of the stack to a 64-bit integer, using zero extension.

```
u32 a = pop()
push(i64(a))
```

Details:
* Instruction code: `0x3e`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a] → […, i64(a)]`

Traps:
* `stackOverflow`: there is no room on the stack for the extra slot.

Notes:
* the high 32 bits of the result are always zero.

### Instruction `u32.ext`

Applies a bitwise operation with an immediate operand to the 32-bit integer on top of the stack: masks the low bits, or shifts left, right or arithmetic right.

```
u32 a = pop()
u32 n = imm.u8 & 0x1f
switch (imm.u8 >> 5) {
    case 0: push(a & ((1 << n) - 1))    // and: keep the low n bits
    case 1: push(a << n)                // shl
    case 2: push(a >> n)                // shr: logical
    case 3: push(i32(a) >> n)         // sar: arithmetic
    default: illegalInstruction
}
```

Details:
* Instruction code: `0x3f`
* Instruction length: `2` bytes
* Stack inputs: `1` slot
* Stack outputs: `1` slot
* Immediate operand: `imm.u8`: bits `7‥5` select the operation (`0`: and, `1`: shl, `2`: shr, `3`: sar), bits `4‥0` hold the bit count `n`
* Stack effect: `[…, a] → […, a op n]`

Traps:
* `illegalInstruction`: the operation selector (bits `7‥5` of `imm.u8`) is greater than `3`.

Notes:
* when instruction folding is enabled, the compiler emits this instruction in place of `load.c32` followed by `shl.u32`, `shr.u32`, `sar.u32` (constant shift count, masked to its low 5 bits) or `and.u32` (constant mask of the form `2^n - 1`).
* with the extended folding enabled, it also replaces the multiplication (`mul.u32`, `mul.i32`), division (`div.u32`) and remainder (`rem.u32`) by a constant power of two with `shl`, `shr` and `and`.

### Instruction `cmt.u64`

Inverts each bit of the 64-bit integer on top of the stack (bitwise complement).

```
u64 a = pop()
push(~a)
```

Details:
* Instruction code: `0x40`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a] → […, ~a]`

### Instruction `and.u64`

Computes the bitwise and of the two 64-bit integers on top of the stack.

```
u64 b = pop()
u64 a = pop()
push(a & b)
```

Details:
* Instruction code: `0x41`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a & b]`

### Instruction `or.u64`

Computes the bitwise or of the two 64-bit integers on top of the stack.

```
u64 b = pop()
u64 a = pop()
push(a | b)
```

Details:
* Instruction code: `0x42`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a | b]`

### Instruction `mul.u64`

Multiplies the two unsigned 64-bit integers on top of the stack.

```
u64 b = pop()
u64 a = pop()
push(a * b)
```

Details:
* Instruction code: `0x43`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a * b]`

Notes:
* the result wraps around modulo 2^64.

### Instruction `div.u64`

Divides the two unsigned 64-bit integers on top of the stack.

```
u64 b = pop()
u64 a = pop()
push(a / b)
```

Details:
* Instruction code: `0x44`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a / b]`

Traps:
* `divisionByZero`: the divisor `b` is zero.

### Instruction `rem.u64`

Computes the remainder of the unsigned division of the two 64-bit integers on top of the stack.

```
u64 b = pop()
u64 a = pop()
push(a % b)
```

Details:
* Instruction code: `0x45`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a % b]`

Traps:
* `divisionByZero`: the divisor `b` is zero.

### Instruction `clt.u64`

Compares the two unsigned 64-bit integers on top of the stack, and pushes `1` if the first is less than the second, `0` otherwise.

```
u64 b = pop()
u64 a = pop()
push(a < b)
```

Details:
* Instruction code: `0x48`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a < b]`

### Instruction `cgt.u64`

Compares the two unsigned 64-bit integers on top of the stack, and pushes `1` if the first is greater than the second, `0` otherwise.

```
u64 b = pop()
u64 a = pop()
push(a > b)
```

Details:
* Instruction code: `0x49`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a > b]`

### Instruction `shl.u64`

Shifts the 64-bit integer left by the given number of bits, filling with zeros.

```
u32 b = pop()
u64 a = pop()
push(a << b)
```

Details:
* Instruction code: `0x4a`
* Instruction length: `1` byte
* Stack inputs: `3` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a << b]`

Notes:
* the shift count `b` is a 32-bit integer.
* the shift count is not masked at runtime: a count of `64` or more is undefined behavior in C (x86 hosts typically use only the low 6 bits of the count).

### Instruction `shr.u64`

Shifts the unsigned 64-bit integer right by the given number of bits, filling with zeros (logical shift).

```
u32 b = pop()
u64 a = pop()
push(a >> b)
```

Details:
* Instruction code: `0x4b`
* Instruction length: `1` byte
* Stack inputs: `3` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a >> b]`

Notes:
* the shift count `b` is a 32-bit integer.
* the shift count is not masked at runtime: a count of `64` or more is undefined behavior in C (x86 hosts typically use only the low 6 bits of the count).

### Instruction `sar.u64`

Shifts the signed 64-bit integer right by the given number of bits, replicating the sign bit (arithmetic shift).

```
i32 b = pop()
i64 a = pop()
push(a >> b)
```

Details:
* Instruction code: `0x4c`
* Instruction length: `1` byte
* Stack inputs: `3` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a >> b]`

Notes:
* the shift count `b` is a signed 32-bit integer.
* the shift count is not masked at runtime: a count of `64` or more, or a negative count, is undefined behavior in C (x86 hosts typically use only the low 6 bits of the count).

### Instruction `xor.u64`

Computes the bitwise exclusive or of the two 64-bit integers on top of the stack.

```
u64 b = pop()
u64 a = pop()
push(a ^ b)
```

Details:
* Instruction code: `0x4d`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a ^ b]`

### Instruction `neg.i32`

Negates the 32-bit signed integer on top of the stack.

```
i32 a = pop()
push(-a)
```

Details:
* Instruction code: `0x50`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a] → […, -a]`

Notes:
* negating the minimum value (`-2147483648`) overflows; the overflow is not detected.

### Instruction `add.i32`

Adds the two 32-bit integers on top of the stack.

```
i32 b = pop()
i32 a = pop()
push(a + b)
```

Details:
* Instruction code: `0x51`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a + b]`

Notes:
* overflow is not detected; the result for out-of-range values is undefined in C (it typically wraps around).

### Instruction `sub.i32`

Subtracts the 32-bit integer on top of the stack from the one below it.

```
i32 b = pop()
i32 a = pop()
push(a - b)
```

Details:
* Instruction code: `0x52`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a - b]`

Notes:
* overflow is not detected; the result for out-of-range values is undefined in C (it typically wraps around).

### Instruction `mul.i32`

Multiplies the two 32-bit signed integers on top of the stack.

```
i32 b = pop()
i32 a = pop()
push(a * b)
```

Details:
* Instruction code: `0x53`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a * b]`

Notes:
* only the low 32 bits of the product are kept; overflow is not detected; the result for out-of-range values is undefined in C (it typically wraps around).

### Instruction `div.i32`

Divides the 32-bit signed integer below the top of the stack by the one on top.

```
i32 b = pop()
i32 a = pop()
push(a / b)
```

Details:
* Instruction code: `0x54`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a / b]`

Traps:
* `divisionByZero`: the divisor `b` is zero, or the division overflows (`-2147483648 / -1`).

Notes:
* the quotient is truncated toward zero.

### Instruction `rem.i32`

Computes the remainder of the signed division of the two 32-bit integers on top of the stack.

```
i32 b = pop()
i32 a = pop()
push(a % b)
```

Details:
* Instruction code: `0x55`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a % b]`

Traps:
* `divisionByZero`: the divisor `b` is zero, or the dividend is `-2147483648` and the divisor is `-1` (the result would be `0`, but the division overflows).

Notes:
* the result has the sign of the dividend `a` (C semantics: `a == (a / b) * b + a % b`).

### Instruction `ceq.i32`

Compares the two 32-bit integers on top of the stack for equality.

```
i32 b = pop()
i32 a = pop()
push(a == b)
```

Details:
* Instruction code: `0x57`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a == b]`

Notes:
* the result is a 32-bit integer: `1` if the condition holds, `0` otherwise.

### Instruction `clt.i32`

Checks if the 32-bit signed integer below the top of the stack is less than the one on top.

```
i32 b = pop()
i32 a = pop()
push(a < b)
```

Details:
* Instruction code: `0x58`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a < b]`

Notes:
* the result is a 32-bit integer: `1` if the condition holds, `0` otherwise.

### Instruction `cgt.i32`

Checks if the 32-bit signed integer below the top of the stack is greater than the one on top.

```
i32 b = pop()
i32 a = pop()
push(a > b)
```

Details:
* Instruction code: `0x59`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a > b]`

Notes:
* the result is a 32-bit integer: `1` if the condition holds, `0` otherwise.

### Instruction `i32.2bool`

Converts the 32-bit integer on top of the stack to a boolean.

```
i32 a = pop()
push(a != 0)
```

Details:
* Instruction code: `0x5a`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a] → […, a != 0]`

Notes:
* the result is `1` if `a` is non-zero, `0` otherwise.

### Instruction `i32.2i64`

Converts the 32-bit signed integer on top of the stack to a 64-bit integer using sign extension.

```
i32 a = pop()
push(i64(a))
```

Details:
* Instruction code: `0x5b`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a] → […, i64(a)]`

Traps:
* `stackOverflow`: there is no room on the stack for the extra slot.

### Instruction `i32.2f32`

Converts the 32-bit signed integer on top of the stack to a 32-bit floating point value.

```
i32 a = pop()
push(f32(a))
```

Details:
* Instruction code: `0x5c`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a] → […, f32(a)]`

Notes:
* values with a magnitude above 2^24 may be rounded, as `f32` has a 24-bit significand.

### Instruction `i32.2f64`

Converts the 32-bit signed integer on top of the stack to a 64-bit floating point value.

```
i32 a = pop()
push(f64(a))
```

Details:
* Instruction code: `0x5d`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a] → […, f64(a)]`

Traps:
* `stackOverflow`: there is no room on the stack for the extra slot.

Notes:
* the conversion is exact.

### Instruction `neg.i64`

Negates the 64-bit signed integer on top of the stack.

```
i64 a = pop()
push(-a)
```

Details:
* Instruction code: `0x60`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a] → […, -a]`

Notes:
* negating the minimum value (`-9223372036854775808`) overflows; the overflow is not detected.

### Instruction `add.i64`

Adds the two 64-bit integers on top of the stack.

```
i64 b = pop()
i64 a = pop()
push(a + b)
```

Details:
* Instruction code: `0x61`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a + b]`

Notes:
* overflow is not detected; the result for out-of-range values is undefined in C (it typically wraps around).

### Instruction `sub.i64`

Subtracts the 64-bit integer on top of the stack from the one below it.

```
i64 b = pop()
i64 a = pop()
push(a - b)
```

Details:
* Instruction code: `0x62`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a - b]`

Notes:
* overflow is not detected; the result for out-of-range values is undefined in C (it typically wraps around).

### Instruction `mul.i64`

Multiplies the two 64-bit signed integers on top of the stack.

```
i64 b = pop()
i64 a = pop()
push(a * b)
```

Details:
* Instruction code: `0x63`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a * b]`

Notes:
* only the low 64 bits of the product are kept; overflow is not detected; the result for out-of-range values is undefined in C (it typically wraps around).

### Instruction `div.i64`

Divides the 64-bit signed integer below the top of the stack by the one on top.

```
i64 b = pop()
i64 a = pop()
push(a / b)
```

Details:
* Instruction code: `0x64`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a / b]`

Traps:
* `divisionByZero`: the divisor `b` is zero, or the division overflows (`-9223372036854775808 / -1`).

Notes:
* the quotient is truncated toward zero.

### Instruction `rem.i64`

Computes the remainder of the signed division of the two 64-bit integers on top of the stack.

```
i64 b = pop()
i64 a = pop()
push(a % b)
```

Details:
* Instruction code: `0x65`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a % b]`

Traps:
* `divisionByZero`: the divisor `b` is zero, or the dividend is `-9223372036854775808` and the divisor is `-1` (the result would be `0`, but the division overflows).

Notes:
* the result has the sign of the dividend `a` (C semantics: `a == (a / b) * b + a % b`).

### Instruction `ceq.i64`

Compares the two 64-bit integers on top of the stack for equality.

```
i64 b = pop()
i64 a = pop()
push(a == b)
```

Details:
* Instruction code: `0x67`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a == b]`

Notes:
* the result is a 32-bit integer: `1` if the condition holds, `0` otherwise.

### Instruction `clt.i64`

Checks if the 64-bit signed integer below the top of the stack is less than the one on top.

```
i64 b = pop()
i64 a = pop()
push(a < b)
```

Details:
* Instruction code: `0x68`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a < b]`

Notes:
* the result is a 32-bit integer: `1` if the condition holds, `0` otherwise.

### Instruction `cgt.i64`

Checks if the 64-bit signed integer below the top of the stack is greater than the one on top.

```
i64 b = pop()
i64 a = pop()
push(a > b)
```

Details:
* Instruction code: `0x69`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a > b]`

Notes:
* the result is a 32-bit integer: `1` if the condition holds, `0` otherwise.

### Instruction `i64.2i32`

Converts the 64-bit integer on top of the stack to a 32-bit integer, keeping only the low 32 bits.

```
i64 a = pop()
push(i32(a))
```

Details:
* Instruction code: `0x6a`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a] → […, i32(a)]`

Notes:
* the high 32 bits are discarded; no range check is made.

### Instruction `i64.2bool`

Converts the 64-bit integer on top of the stack to a boolean.

```
i64 a = pop()
push(a != 0)
```

Details:
* Instruction code: `0x6b`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a] → […, a != 0]`

Notes:
* the result is a 32-bit integer: `1` if `a` is non-zero, `0` otherwise.

### Instruction `i64.2f32`

Converts the 64-bit signed integer on top of the stack to a 32-bit floating point value.

```
i64 a = pop()
push(f32(a))
```

Details:
* Instruction code: `0x6c`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a] → […, f32(a)]`

Notes:
* values with a magnitude above 2^24 may be rounded, as `f32` has a 24-bit significand.

### Instruction `i64.2f64`

Converts the 64-bit signed integer on top of the stack to a 64-bit floating point value.

```
i64 a = pop()
push(f64(a))
```

Details:
* Instruction code: `0x6d`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a] → […, f64(a)]`

Notes:
* values with a magnitude above 2^53 may be rounded, as `f64` has a 53-bit significand.

### Instruction `neg.f32`

Negates the 32-bit floating point value on top of the stack.

```
f32 a = pop()
push(-a)
```

Details:
* Instruction code: `0x70`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a] → […, -a]`

Notes:
* only the sign is flipped: `-0.0` is produced from `0.0` and the sign of `NaN` is inverted.

### Instruction `add.f32`

Adds the two 32-bit floating point values on top of the stack.

```
f32 b = pop()
f32 a = pop()
push(a + b)
```

Details:
* Instruction code: `0x71`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a + b]`

### Instruction `sub.f32`

Subtracts the 32-bit floating point value on top of the stack from the one below it.

```
f32 b = pop()
f32 a = pop()
push(a - b)
```

Details:
* Instruction code: `0x72`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a - b]`

### Instruction `mul.f32`

Multiplies the two 32-bit floating point values on top of the stack.

```
f32 b = pop()
f32 a = pop()
push(a * b)
```

Details:
* Instruction code: `0x73`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a * b]`

### Instruction `div.f32`

Divides the 32-bit floating point value below the top of the stack by the value on top of it.

```
f32 b = pop()
f32 a = pop()
push(a / b)
```

Details:
* Instruction code: `0x74`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a / b]`

Notes:
* division by zero does not trap: execution continues with the IEEE 754 result (`±inf` or `NaN`); when running with the debugger attached, the debugger is notified.

### Instruction `rem.f32`

Computes the floating point remainder of dividing the 32-bit floating point value below the top of the stack by the value on top of it.

```
f32 b = pop()
f32 a = pop()
push(fmodf(a, b))
```

Details:
* Instruction code: `0x75`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, fmod(a, b)]`

Notes:
* the result is computed with the C function `fmodf`: it is truncated toward zero, so it has the sign of `a`.
* a zero divisor does not trap: execution continues with the IEEE 754 result (`NaN`); when running with the debugger attached, the debugger is notified.

### Instruction `ceq.f32`

Compares the two 32-bit floating point values on top of the stack for equality.

```
f32 b = pop()
f32 a = pop()
push(i32(a == b))
```

Details:
* Instruction code: `0x77`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a == b]`

Notes:
* the result is `1` if the condition holds, `0` otherwise.
* `NaN` is not equal to anything, including itself; `-0.0` is equal to `0.0`.

### Instruction `clt.f32`

Checks whether the 32-bit floating point value below the top of the stack is less than the value on top of it.

```
f32 b = pop()
f32 a = pop()
push(i32(a < b))
```

Details:
* Instruction code: `0x78`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a < b]`

Notes:
* the result is `1` if the condition holds, `0` otherwise.
* every comparison involving `NaN` yields `0`.

### Instruction `cgt.f32`

Checks whether the 32-bit floating point value below the top of the stack is greater than the value on top of it.

```
f32 b = pop()
f32 a = pop()
push(i32(a > b))
```

Details:
* Instruction code: `0x79`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a > b]`

Notes:
* the result is `1` if the condition holds, `0` otherwise.
* every comparison involving `NaN` yields `0`.

### Instruction `f32.2i32`

Converts the 32-bit floating point value on top of the stack to a 32-bit signed integer.

```
f32 a = pop()
push(i32(a))
```

Details:
* Instruction code: `0x7a`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a] → […, i32(a)]`

Notes:
* the value is truncated toward zero; the result is undefined (in C) if the truncated value does not fit in the target type or `a` is `NaN`.

### Instruction `f32.2i64`

Converts the 32-bit floating point value on top of the stack to a 64-bit signed integer.

```
f32 a = pop()
push(i64(a))
```

Details:
* Instruction code: `0x7b`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a] → […, i64(a)]`

Traps:
* `stackOverflow`: there is no room on the stack for the extra slot.

Notes:
* the value is truncated toward zero; the result is undefined (in C) if the truncated value does not fit in the target type or `a` is `NaN`.

### Instruction `f32.2bool`

Converts the 32-bit floating point value on top of the stack to a boolean.

```
f32 a = pop()
push(i32(a != 0 && !isNaN(a)))
```

Details:
* Instruction code: `0x7c`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a] → […, bool(a)]`

Notes:
* the result is `1` if `a` is non-zero and not `NaN`, `0` otherwise.
* `NaN` converts to `0` (unlike in C, where `NaN` is truthy).

### Instruction `f32.2f64`

Converts the 32-bit floating point value on top of the stack to a 64-bit floating point value.

```
f32 a = pop()
push(f64(a))
```

Details:
* Instruction code: `0x7d`
* Instruction length: `1` byte
* Stack inputs: `1` slot
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a] → […, f64(a)]`

Traps:
* `stackOverflow`: there is no room on the stack for the extra slot.

Notes:
* the conversion is exact.

### Instruction `load.f32`

Pushes a 32-bit floating point constant onto the stack.

```
push(imm.x32)
```

Details:
* Instruction code: `0x7f`
* Instruction length: `5` bytes
* Stack inputs: `0` slots
* Stack outputs: `1` slot
* Immediate operand: `imm.x32`: the 32-bit constant to push
* Stack effect: `[…] → […, imm]`

Traps:
* `stackOverflow`: there is no room on the stack for the extra slot.

Notes:
* temporary opcode: it is executed exactly like `load.c32`, and may be replaced by it.

### Instruction `neg.f64`

Negates the 64-bit floating point value on top of the stack.

```
f64 a = pop()
push(-a)
```

Details:
* Instruction code: `0x80`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a] → […, -a]`

Notes:
* only the sign is flipped: `-0.0` is produced from `0.0` and the sign of `NaN` is inverted.

### Instruction `add.f64`

Adds the two 64-bit floating point values on top of the stack.

```
f64 b = pop()
f64 a = pop()
push(a + b)
```

Details:
* Instruction code: `0x81`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a + b]`

### Instruction `sub.f64`

Subtracts the 64-bit floating point value on top of the stack from the one below it.

```
f64 b = pop()
f64 a = pop()
push(a - b)
```

Details:
* Instruction code: `0x82`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a - b]`

### Instruction `mul.f64`

Multiplies the two 64-bit floating point values on top of the stack.

```
f64 b = pop()
f64 a = pop()
push(a * b)
```

Details:
* Instruction code: `0x83`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a * b]`

### Instruction `div.f64`

Divides the 64-bit floating point value below the top of the stack by the value on top of it.

```
f64 b = pop()
f64 a = pop()
push(a / b)
```

Details:
* Instruction code: `0x84`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a / b]`

Notes:
* division by zero does not trap: execution continues with the IEEE 754 result (`±inf` or `NaN`); when running with the debugger attached, the debugger is notified.

### Instruction `rem.f64`

Computes the floating point remainder of dividing the 64-bit floating point value below the top of the stack by the value on top of it.

```
f64 b = pop()
f64 a = pop()
push(fmod(a, b))
```

Details:
* Instruction code: `0x85`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, fmod(a, b)]`

Notes:
* the result is computed with the C function `fmod`: it is truncated toward zero, so it has the sign of `a`.
* a zero divisor does not trap: execution continues with the IEEE 754 result (`NaN`); when running with the debugger attached, the debugger is notified.

### Instruction `ceq.f64`

Compares the two 64-bit floating point values on top of the stack for equality.

```
f64 b = pop()
f64 a = pop()
push(i32(a == b))
```

Details:
* Instruction code: `0x87`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a == b]`

Notes:
* the result is `1` if the condition holds, `0` otherwise.
* `NaN` is not equal to anything, including itself; `-0.0` is equal to `0.0`.

### Instruction `clt.f64`

Checks whether the 64-bit floating point value below the top of the stack is less than the value on top of it.

```
f64 b = pop()
f64 a = pop()
push(i32(a < b))
```

Details:
* Instruction code: `0x88`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a < b]`

Notes:
* the result is `1` if the condition holds, `0` otherwise.
* every comparison involving `NaN` yields `0`.

### Instruction `cgt.f64`

Checks whether the 64-bit floating point value below the top of the stack is greater than the value on top of it.

```
f64 b = pop()
f64 a = pop()
push(i32(a > b))
```

Details:
* Instruction code: `0x89`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a > b]`

Notes:
* the result is `1` if the condition holds, `0` otherwise.
* every comparison involving `NaN` yields `0`.

### Instruction `f64.2i32`

Converts the 64-bit floating point value on top of the stack to a 32-bit signed integer.

```
f64 a = pop()
push(i32(a))
```

Details:
* Instruction code: `0x8a`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a] → […, i32(a)]`

Notes:
* the value is truncated toward zero; the result is undefined (in C) if the truncated value does not fit in the target type or `a` is `NaN`.

### Instruction `f64.2i64`

Converts the 64-bit floating point value on top of the stack to a 64-bit signed integer.

```
f64 a = pop()
push(i64(a))
```

Details:
* Instruction code: `0x8b`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `2` slots
* Immediate operand: none
* Stack effect: `[…, a] → […, i64(a)]`

Notes:
* the value is truncated toward zero; the result is undefined (in C) if the truncated value does not fit in the target type or `a` is `NaN`.

### Instruction `f64.2f32`

Converts the 64-bit floating point value on top of the stack to a 32-bit floating point value.

```
f64 a = pop()
push(f32(a))
```

Details:
* Instruction code: `0x8c`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a] → […, f32(a)]`

Notes:
* the value is rounded to the nearest representable value; values out of range become `±inf`.

### Instruction `f64.2bool`

Converts the 64-bit floating point value on top of the stack to a boolean.

```
f64 a = pop()
push(i32(a != 0 && !isNaN(a)))
```

Details:
* Instruction code: `0x8d`
* Instruction length: `1` byte
* Stack inputs: `2` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a] → […, bool(a)]`

Notes:
* the result is `1` if `a` is non-zero and not `NaN`, `0` otherwise.
* `NaN` converts to `0` (unlike in C, where `NaN` is truthy).

### Instruction `load.f64`

Pushes a 64-bit floating point constant onto the stack.

```
push(imm.x64)
```

Details:
* Instruction code: `0x8f`
* Instruction length: `9` bytes
* Stack inputs: `0` slots
* Stack outputs: `2` slots
* Immediate operand: `imm.x64`: the 64-bit constant to push
* Stack effect: `[…] → […, imm]`

Traps:
* `stackOverflow`: there is no room on the stack for the extra slots.

Notes:
* temporary opcode: it is executed exactly like `load.c64`, and may be replaced by it.

### Instruction `neg.v4f`

Negates each lane of the 32-bit floating point vector on top of the stack.

```
v4f a = pop()
push(v4f(-a.x, -a.y, -a.z, -a.w))
```

Details:
* Instruction code: `0x90`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `4` slots
* Immediate operand: none
* Stack effect: `[…, a] → […, -a]`

Notes:
* lane `x` (lane 0) is stored at the lowest address, which is the top slot of the vector on the stack.

### Instruction `add.v4f`

Adds the two 32-bit floating point vectors on top of the stack, lane by lane.

```
v4f b = pop()
v4f a = pop()
push(v4f(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w))
```

Details:
* Instruction code: `0x91`
* Instruction length: `1` byte
* Stack inputs: `8` slots
* Stack outputs: `4` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a + b]`

### Instruction `sub.v4f`

Subtracts the 32-bit floating point vector on top of the stack from the one below it, lane by lane.

```
v4f b = pop()
v4f a = pop()
push(v4f(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w))
```

Details:
* Instruction code: `0x92`
* Instruction length: `1` byte
* Stack inputs: `8` slots
* Stack outputs: `4` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a - b]`

### Instruction `mul.v4f`

Multiplies the two 32-bit floating point vectors on top of the stack, lane by lane.

```
v4f b = pop()
v4f a = pop()
push(v4f(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w))
```

Details:
* Instruction code: `0x93`
* Instruction length: `1` byte
* Stack inputs: `8` slots
* Stack outputs: `4` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a * b]`

### Instruction `div.v4f`

Divides the 32-bit floating point vector below the top of the stack by the one on top, lane by lane.

```
v4f b = pop()
v4f a = pop()
push(v4f(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w))
```

Details:
* Instruction code: `0x94`
* Instruction length: `1` byte
* Stack inputs: `8` slots
* Stack outputs: `4` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a / b]`

Notes:
* division by zero does not trap: the lane receives the IEEE 754 result (`±inf` or `NaN`).

### Instruction `ceq.v4f`

Compares the two 32-bit floating point vectors on top of the stack, and pushes `1` if all four lanes are equal, `0` otherwise.

```
v4f b = pop()
v4f a = pop()
push(i32(a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w))
```

Details:
* Instruction code: `0x97`
* Instruction length: `1` byte
* Stack inputs: `8` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a == b]`

Notes:
* lanes are compared as floating point values: a `NaN` lane never compares equal, `-0.0` equals `+0.0`.

### Instruction `min.v4f`

Computes the lane by lane minimum of the two 32-bit floating point vectors on top of the stack.

```
v4f b = pop()
v4f a = pop()
push(v4f(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w)))
```

Details:
* Instruction code: `0x98`
* Instruction length: `1` byte
* Stack inputs: `8` slots
* Stack outputs: `4` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, min(a, b)]`

Notes:
* each lane is computed as `a > b ? b : a`, so if either operand is `NaN` the lane of `a` is kept.

### Instruction `max.v4f`

Computes the lane by lane maximum of the two 32-bit floating point vectors on top of the stack.

```
v4f b = pop()
v4f a = pop()
push(v4f(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w)))
```

Details:
* Instruction code: `0x99`
* Instruction length: `1` byte
* Stack inputs: `8` slots
* Stack outputs: `4` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, max(a, b)]`

Notes:
* each lane is computed as `a < b ? b : a`, so if either operand is `NaN` the lane of `a` is kept.

### Instruction `dp3.v4f`

Computes the 3-component dot product of the two 32-bit floating point vectors on top of the stack.

```
v4f b = pop()
v4f a = pop()
push(a.x * b.x + a.y * b.y + a.z * b.z)
```

Details:
* Instruction code: `0x9a`
* Instruction length: `1` byte
* Stack inputs: `8` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, dp3(a, b)]`

Notes:
* the `w` lanes of both operands are ignored.
* the result is a single `f32` value.

### Instruction `dp4.v4f`

Computes the 4-component dot product of the two 32-bit floating point vectors on top of the stack.

```
v4f b = pop()
v4f a = pop()
push(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w)
```

Details:
* Instruction code: `0x9b`
* Instruction length: `1` byte
* Stack inputs: `8` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, dp4(a, b)]`

Notes:
* the result is a single `f32` value.

### Instruction `dph.v4f`

Computes the homogeneous dot product of the two 32-bit floating point vectors on top of the stack: the 3-component dot product plus the `w` lane of the first operand.

```
v4f b = pop()
v4f a = pop()
push(a.x * b.x + a.y * b.y + a.z * b.z + a.w)
```

Details:
* Instruction code: `0x9c`
* Instruction length: `1` byte
* Stack inputs: `8` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, dph(a, b)]`

Notes:
* the `w` lane of `b` is ignored, the `w` lane of `a` is added to the result.
* the result is a single `f32` value.

### Instruction `swz.p4x`

Shuffles the four 32-bit slots on top of the stack, each result lane is selected by a 2-bit field of the immediate operand.

```
s0 = sp((imm.u8 >> 0) & 3)
s1 = sp((imm.u8 >> 2) & 3)
s2 = sp((imm.u8 >> 4) & 3)
s3 = sp((imm.u8 >> 6) & 3)
sp(0) = s0
sp(1) = s1
sp(2) = s2
sp(3) = s3
```

Details:
* Instruction code: `0x9d`
* Instruction length: `2` bytes
* Stack inputs: `4` slots
* Stack outputs: `4` slots
* Immediate operand: `imm.u8`: selector, bits `2k+1‥2k` give the source lane of result lane `k`
* Stack effect: `[…, a3, a2, a1, a0] → […, r3, r2, r1, r0]`

Notes:
* lane `k` is the slot `sp(k)`: lane 0 (`x`) is on top of the stack, lane 3 (`w`) is the deepest.
* the shuffle is type-agnostic, it works on any 4 x 32-bit vector.
* example: `swz.p4x 0x44` (`0b01_00_01_00`) results in `[…, a1, a0, a1, a0]` (`xyxy`); `0xe4` is the identity, `0x00` broadcasts lane `x`.

### Instruction `load.m128`

Loads a 128-bit value from the memory address given by the immediate operand onto the stack.

```
a4 = mem[imm.i24 +  0].x32
a3 = mem[imm.i24 +  4].x32
a2 = mem[imm.i24 +  8].x32
a1 = mem[imm.i24 + 12].x32
push(a1)
push(a2)
push(a3)
push(a4)
```

Details:
* Instruction code: `0x9e`
* Instruction length: `4` bytes
* Stack inputs: `0` slots
* Stack outputs: `4` slots
* Immediate operand: `imm.i24`: absolute memory address of the value
* Stack effect: `[…] → […, a1, a2, a3, a4]`

Traps:
* `illegalMemoryAccess`: the address is `null` or negative, or `address + 16` is past the end of the memory.
* `stackOverflow`: there is no room on the stack for the new slots.

Notes:
* compressed form of `load.ref address` followed by `load.i128`.
* the 16 bytes are copied verbatim: the top slot `a4` receives bytes `0‥3`, the deepest slot `a1` receives bytes `12‥15`.
* the transfer is type-agnostic, it is used for any 128-bit value: `v4f`, `v2d`, 16-byte records, ...
* the alignment of the address is not checked.

### Instruction `store.m128`

Stores the 128-bit value on top of the stack to the memory address given by the immediate operand.

```
a4 = pop()
a3 = pop()
a2 = pop()
a1 = pop()
mem[imm.i24 +  0].x32 = a4
mem[imm.i24 +  4].x32 = a3
mem[imm.i24 +  8].x32 = a2
mem[imm.i24 + 12].x32 = a1
```

Details:
* Instruction code: `0x9f`
* Instruction length: `4` bytes
* Stack inputs: `4` slots
* Stack outputs: `0` slots
* Immediate operand: `imm.i24`: absolute memory address of the value
* Stack effect: `[…, a1, a2, a3, a4] → […]`

Traps:
* `illegalMemoryAccess`: the address points into the read-only region (constants and code), is negative, or `address + 16` is past the end of the memory.

Notes:
* compressed form of `load.ref address` followed by `store.i128`.
* the 4 slots are copied verbatim: the slot pushed last (`a4`) is written to bytes `0‥3`, the slot pushed first (`a1`) to bytes `12‥15`.
* the transfer is type-agnostic, it is used for any 128-bit value: `v4f`, `v2d`, 16-byte records, ...
* the alignment of the address is not checked.

### Instruction `neg.v2d`

Negates each lane of the 64-bit floating point vector on top of the stack.

```
v2d a = pop()
push(v2d(-a.x, -a.y))
```

Details:
* Instruction code: `0xa0`
* Instruction length: `1` byte
* Stack inputs: `4` slots
* Stack outputs: `4` slots
* Immediate operand: none
* Stack effect: `[…, a] → […, -a]`

Notes:
* lane `x` (lane 0) is stored at the lowest address, which is the top two slots of the vector on the stack.

### Instruction `add.v2d`

Adds the two 64-bit floating point vectors on top of the stack, lane by lane.

```
v2d b = pop()
v2d a = pop()
push(v2d(a.x + b.x, a.y + b.y))
```

Details:
* Instruction code: `0xa1`
* Instruction length: `1` byte
* Stack inputs: `8` slots
* Stack outputs: `4` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a + b]`

### Instruction `sub.v2d`

Subtracts the 64-bit floating point vector on top of the stack from the one below it, lane by lane.

```
v2d b = pop()
v2d a = pop()
push(v2d(a.x - b.x, a.y - b.y))
```

Details:
* Instruction code: `0xa2`
* Instruction length: `1` byte
* Stack inputs: `8` slots
* Stack outputs: `4` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a - b]`

### Instruction `mul.v2d`

Multiplies the two 64-bit floating point vectors on top of the stack, lane by lane.

```
v2d b = pop()
v2d a = pop()
push(v2d(a.x * b.x, a.y * b.y))
```

Details:
* Instruction code: `0xa3`
* Instruction length: `1` byte
* Stack inputs: `8` slots
* Stack outputs: `4` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a * b]`

### Instruction `div.v2d`

Divides the 64-bit floating point vector below the top of the stack by the one on top, lane by lane.

```
v2d b = pop()
v2d a = pop()
push(v2d(a.x / b.x, a.y / b.y))
```

Details:
* Instruction code: `0xa4`
* Instruction length: `1` byte
* Stack inputs: `8` slots
* Stack outputs: `4` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a / b]`

Notes:
* division by zero does not trap: the lane receives the IEEE 754 result (`±inf` or `NaN`).

### Instruction `ceq.v2d`

Compares the two 64-bit floating point vectors on top of the stack, and pushes `1` if both lanes are equal, `0` otherwise.

```
v2d b = pop()
v2d a = pop()
push(i32(a.x == b.x && a.y == b.y))
```

Details:
* Instruction code: `0xa7`
* Instruction length: `1` byte
* Stack inputs: `8` slots
* Stack outputs: `1` slot
* Immediate operand: none
* Stack effect: `[…, a, b] → […, a == b]`

Notes:
* lanes are compared as floating point values: a `NaN` lane never compares equal, `-0.0` equals `+0.0`.

### Instruction `min.v2d`

Computes the lane by lane minimum of the two 64-bit floating point vectors on top of the stack.

```
v2d b = pop()
v2d a = pop()
push(v2d(min(a.x, b.x), min(a.y, b.y)))
```

Details:
* Instruction code: `0xa8`
* Instruction length: `1` byte
* Stack inputs: `8` slots
* Stack outputs: `4` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, min(a, b)]`

Notes:
* each lane is computed as `a > b ? b : a`, so if either operand is `NaN` the lane of `a` is kept.

### Instruction `max.v2d`

Computes the lane by lane maximum of the two 64-bit floating point vectors on top of the stack.

```
v2d b = pop()
v2d a = pop()
push(v2d(max(a.x, b.x), max(a.y, b.y)))
```

Details:
* Instruction code: `0xa9`
* Instruction length: `1` byte
* Stack inputs: `8` slots
* Stack outputs: `4` slots
* Immediate operand: none
* Stack effect: `[…, a, b] → […, max(a, b)]`

Notes:
* each lane is computed as `a < b ? b : a`, so if either operand is `NaN` the lane of `a` is kept.

# Command Line Reference
To compile and execute a script file, the cmpl application must be invoked with the corresponding arguments.

>cmpl \[global options\]... \[file \[file options\]...\]...

## Global options

- `-run[*]`: run at full speed, no debug information available
	- `/g /G`: dump global variable values (/G includes builtin types and functions)
	- `/m /M`: dump memory usage (/M includes heap allocations)
	- `/t`: add debug information for stack-traces

- `-debug[*]`: run with attached debugger, pausing on uncaught errors and break points
	- `/g /G`: dump global variable values (/G includes builtin types and functions)
	- `/m /M`: dump memory usage (/M includes heap allocations)
	- `/p /P`: dump caught errors (/P includes stacktrace)
	- `/t /T`: dump executed instructions (/T includes values on stack)
	- `/s /S`: pause on any error (/S pauses on startup)

- `-profile[*]`: run code with profiler: coverage, method tracing
	- `/g /G`: dump global variable values (/G includes builtin types and functions)
	- `/m /M`: dump memory usage (/M includes heap allocations)
	- `/p /P`: dump executed statements (/P include all functions and statements)
	- `/t /T`: trace execution with timestamps (/T includes instructions)

- `-std<file>`: specify custom standard library file (empty file name disables std library compilation).

- `-mem<int>[kKmMgG]`: override memory usage for compiler and runtime(heap)

- `-log[*]<int> <file>`: set logger for: compilation errors and warnings, runtime debug messages
	- `<int>`: set the default log(warning) level 0 - 15
	- `/a`: append to the log file
	- `/d`: dump to the log file

- `-dump[?] <file>`: set output for: dump(symbols, assembly, abstract syntax tree, coverage, call tree)
	- `.scite`: dump api for SciTE text editor
	- `.json`: dump api and profile data in javascript object notation format
	- `.md`: dump documentation in markdown format

- `-api[*]`: dump symbols
	- `/a /A`: include all library symbols(/A includes builtins)
	- `/d`: dump details of symbol
	- `/p`: dump params and fields

- `-doc[*]`: dump documentation
	- `/a /A`: include all library symbols(/A includes builtins)
	- `/d`: dump details of symbol
	- `/p`: dump params and fields

- `-use[*]`: dump usages
	- `/a /A`: include all library symbols(/A includes builtins)
	- `/d`: dump details of symbol
	- `/p`: dump params and fields

- `-asm[*]<int>`: dump assembled code: jmp +80
	- `/a /A`: include all library symbols(/A includes builtins)
	- `/d`: dump details of symbol
	- `/p`: dump params and fields
	- `/g`: use global address: jmp @0x003d8c
	- `/n`: prefer names over addresses: jmp <main+80>
	- `/s`: print source code statements

- `-ast[*]`: dump syntax tree
	- `/a /A`: include all library symbols(/A includes builtins)
	- `/d`: dump details of symbol
	- `/p`: dump params and fields
	- `/t`: dump sub-expression type information
	- `/l`: do not expand statements (print on single line)
	- `/b`: don't keep braces ('{') on the same line
	- `/e`: don't keep `else if` constructs on the same line

## Files with options

filename followed by switches
- `<file>`: if file extension is (.so|.dll|.wasm) load as library else compile
- `-w[a|<int>]`: set or disable warning level for current file
- `-b[*]<int>`: break point on <int> line in current file
	- `/[p|P]`: print only, do not pause (/P includes stack trace)
	- `/[v]`: after execution evaluate the top of the stack (partially implemented)
	- `/o`: one shot breakpoint, disable after first hit

## Examples

>cmpl test.cmpl

- compile only the file `test.cmpl`, no global option (run, debug or profile) is specified

>cmpl -run test.cmpl

- compile and execute the file `test.cmpl` at full speed

>cmpl -debug test.cmpl

- compile and execute the file `test.cmpl` with additional checks
- in case of an error, the debugger pauses the execution at the fault location

>cmpl -profile test.cmpl

- compile, execute and measure the file `test.cmpl`
- when the execution finishes, the collected data is dumped to the console, or to the specified file

>cmpl -debug gl.so -w0 gl.cmpl -w0 test.cmpl -wa -b12 -b/o15 -b/p19

- execute in debug mode
- import `gl.so`
	- with no warnings
- compile `gl.cmpl`
	- with no warnings
- compile `test.cmpl`
	- show all warnings
	- break execution each time line 12 is hit
	- break execution when line 15 is first hit
	- print message when line 19 is hit

>cmpl -profile/t -api/a/d/p -dump.json test.json test.cmpl

- execute in profile mode, including the call tree in the dump
- include all symbols with details, parameters and fields in the dump
- dump in json format to the file `test.json`, which can be opened with the web inspector

>cmpl -api -dump.json api.json

- dump builtin symbols to `api.json` file (types, functions, variables, aliases)

>cmpl -doc/a -dump.md Cmpl.md cmplDoc/Cmpl.cmpl

- generate this documentation in markdown format to the file `Cmpl.md`


