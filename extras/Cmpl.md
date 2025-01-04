
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
* [x] overloading operators: `inline *(vec4f a, vec4f b) = inline(vec4f(a), vec4f(b), mul.p4f);`
* [x] inline the content of a file: `inline "./file.cmpl";`


### Conditional compile-time code generation and declarations with `static if` statement.
* [x] `static if (int == int32) { /* 32 bit platform code */ }`
* [x] conditional inlining `static if (int == int32) { inline "file"; }`


### Functions
* [x] Extend any class with custom methods using [Uniform Function Call Syntax](https://en.wikipedia.org/wiki/Uniform_Function_Call_Syntax)
* [x] If a function name is a type name, it is treated as a constructor, and should return an instance of that type.


### `new` and `this` are not part of the language.
* [x] Create and initialize objects like in JavaScript `complex a = {re: 42, im: 2};`
* [x] `this` is not needed using [Uniform Function Call Syntax](https://en.wikipedia.org/wiki/Uniform_Function_Call_Syntax).


### no access level modifiers like: `public`, `protected`, `private`, ...
* [x] accessing an undocument symbol from a different translation unit will raise a high-level warning that can be treated as an error.
* [x] deprecation and any other annotations should be specified in the documentation comment.

# Lexical structure

## Comments

* line comments: `// ...`
* block comments: `/* ... */`

## Identifiers

Identifiers are named references of types, variables and functions.

**[Syntax](Cmpl.g4)**

```antlrv4
Identifier: Letter (Letter | Number)*;
```

## Keywords

Keywords are reserved words, which can not be used as identifiers.

* break
* continue
* else
* emit
* enum
* for
* if
* inline
* return
* static
* struct

## Literals

[Literals](https://en.wikipedia.org/wiki/Literal_(computer_programming)) are compile time constant values.

**[Syntax](Cmpl.g4)**

```antlrv4
Literal
    : '0'[bB][0-1]+
    | '0'[oO]?[0-7]+
    | '0'[xX][0-9a-fA-F]+
    | Decimal Suffix?
    | Decimal Exponent Suffix?
    | '.' Number+ Exponent? Suffix?
    | Decimal '.' Number* Exponent? Suffix?
    | '\'' .*? '\''
    | '"' .*? '"'
    ;
```

### Integer and floating-point literals

Integers can be represented in decimal, binary, octal, or hexadecimal format,
while floating point literals can be specified only in decimal format.

#### Decimal prefix

The following prefixes may be used to specify the radix of an integer:

* binary: `0b` or `0B`

* octal: `0` or `0o` or `0O`

* hexadecimal: `0x` or `0X`

#### Decimal suffix

The following suffixes are available to override the type of a decimal value:

* `d`: results a int32 constant, ex: `inline i32Value = 3d;`

* `D`: results a int64 constant, ex: `inline i64Value = 3D;`

* `u`: results a uint32 constant, ex: `inline u32Value = 3u;`

* `U`: results a uint64 constant, ex: `inline u64Value = 3U;`

* `f`: results a float32 constant, ex: `inline f32Value = 3f;`

* `F`: results a float64 constant, ex: `inline f64Value = 3F;`

### Character and string literals

[TODO: documentation]

#### Escape sequences

Escape sequences are used to define certain special characters within string literals.

The following escape sequences are available:

* `\'`: single quote

* `\"`: double quote

* `\?`: question mark

* `\\`: backslash

* `\a`: audible bell

* `\b`: backspace

* `\f`: form feed - new page

* `\n`: line feed - new line

* `\r`: carriage return

* `\t`: horizontal tab

* `\v`: vertical tab

* `\nnn`: arbitrary octal value, maximum of 3 characters.

* `\xnn`: arbitrary hexadecimal value, always 2 characters.

* `\<new line>`: wraps on next line.

- Octal escape sequences have a limit of three octal digits,
but terminate at the first character that is not a valid octal digit if encountered sooner.

- Hexadecimal escape sequences parses always the next two characters.

#### Multi line Strings

A string literal is multi line if it starts with a backslash followed by a newline.

**Example**

```cmpl
string html = "\
<html>
  <head/>
  <body>
    <p>
      Hello<br/>
      Multiline<br/>
      World<br/>
    </p>
  </body>
</html>
";
```

## Operators

[TODO: documentation]

* !, ~, -, +, &

* (), [], .

* +, -, \*, /, %

* &, |, ^, <<, >>

* ==, !=

* <, <=, >, >=

* ||, &&, ?:

* =, +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=, >>>=

* ,

# Expressions

Expressions are responsible for computing values.

**[Syntax](Cmpl.g4)**

```antlrv4
expression
    : Literal                                                                                        # LiteralExpression
    | Identifier                                                                                  # IdentifierExpression
    | '(' expressionList? ')'                                                                  # ParenthesizedExpression
    | expression '(' expressionList? ')'                                                        # FunctionCallExpression
    | '[' expressionList? ']'                                                                  # ParenthesizedExpression
    | expression '[' expressionList? ']'                                                         # ArrayAccessExpression
    | expression '[' expression '..' expression ']'                                               # ArraySliceExpression
    | expression '.' Identifier                                                                 # MemberAccessExpression
    | unary expression                                                                                 # UnaryExpression
    | expression arithmetic expression                                                            # ArithmeticExpression
    | expression bitwise expression                                                                  # BitwiseExpression
    | expression relational expression                                                            # RelationalExpression
    | expression equality expression                                                                # EqualityExpression
    | expression ('='| '*=' | '/=' | '%=' | '+=' | '-=' ) expression                              # AssignmentExpression
    | expression ('&=' | '|=' | '^=' | '<<=' | '>>=') expression                                  # AssignmentExpression
    | expression ('&&' | '||') expression                                                            # LogicalExpression
    | expression '?' expression ':' expression                                                   # ConditionalExpression
    ;
```

## Primary expression
[TODO: documentation]

## Unary expressions

Unary expressions are composed from an unary operator and one operand.

- `!`: logical not. Results true or false.

- `~`: complement. Valid for integer types.

- `-`: unary minus. Change the sign of a number.

- `+`: unary plus. ???

**[Syntax](Cmpl.g4)**

```
unary: ('&' | '+' | '-' | '~' | '!');
```

## Binary expressions

Binary expressions are composed from a binary operator and two operands.

### Arithmetic expression

Are used to return new values computed from the two operands.

- `+, -` Additive operators

- `*, /, %` Multiplicative operators

**[Syntax](Cmpl.g4)**

```
arithmetic: ('*' | '/' | '%' | '+' | '-');
```

### Bitwise expression

May be used on integer types only.

- `&, |, ^` Bitwise operators

- `<<, >>` Bit shift operators

**[Syntax](Cmpl.g4)**

```
bitwise: ('&' | '|' | '^' | '<<' | '>>');
```

### Relational expressions

Are used to compare two operands, if one is les or greater than the other.
The result is a boolean value(true or false).

**[Syntax](Cmpl.g4)**

```
relational: ('<' | '<=' | '>' | '>=');
```

### Equality expressions

Are used to check if the left and right operands are equal or not.
The result is a boolean value(true or false).

**[Syntax](Cmpl.g4)**

```
equality: ('==' | '!=');
```

### Assignment expressions

This operator assigns the value on the right to the operand on the lef side.

**Example**

```
a = 5;
a += 3 + b;
```

Composed operators are expanded, this means that `a += 3 + b` is converted to `a = a + (3 + b)`.

### Logical expressions

[TODO: implementation]

- `||`: logical or operator returns true if **any** of its operands is true.

- `&&`: logical and operator returns true if **all** of its operands are true.

**Example**

```
bool variable = a() || b() || c() || d();
```

The value of `variable` will be true if any of the functions(a, b, c, d) is evaluated to true.
The evaluation stops when the first expression is evaluated to true.


**Example**

```
bool variable = a() && b() && c() && d();
```

The value of `variable` will be true if all of the functions(a, b, c, d) are evaluated to true.
The evaluation stops when the first expression is evaluated to false.

## Ternary expression

The `?:` (conditional) operator is the only ternary operator with 3 operands:

- test operand: based on its value the second or the third operand is returned.

- true operand: evaluated and returned only if the first operand evaluates to true.

- false operand: evaluated and returned only if the first operand evaluates to false.

**Example**

```
int variable = a() ? b() : c();
```

First `a()` is evaluated, and if its value is:

- true: `b()` is evaluated and returned

- false: `c()` is evaluated and returned

## Operator precedence table

| Operator                            | Title                     | Example                   | Associates    |
|-------------------------------------|---------------------------|---------------------------|---------------|
| 15: Primary                         |                           |                           | left to right |
| ( )                                 | Function call             | sqrt(x)                   |               |
| [ ]                                 | Array subscript           | values[10]                |               |
| .                                   | Member access             | child.name                |               |
| 14: Unary                           |                           |                           | right to left |
| +                                   | Unary plus                | +a                        |               |
| -                                   | Unary minus               | -a                        |               |
| ~                                   | Bitwise not               | ~077                      |               |
| !                                   | Logical not               | !ready                    |               |
| 13: Multiplicative                  |                           |                           | left to right |
| *                                   | Multiplication            | i * j                     |               |
| /                                   | Division                  | i / j                     |               |
| %                                   | Modulus                   | i % j                     |               |
| 12: Additive                        |                           |                           | left to right |
| +                                   | Addition                  | value + i                 |               |
| -                                   | Subtraction               | x - 10                    |               |
| 11: Bit shift                       |                           |                           | left to right |
| <<                                  | Left shift                | byte << 4                 |               |
| \>>                                 | Right shift               | i >> 2                    |               |
| 10: Relational                      |                           |                           | left to right |
| <                                   | Less than                 | i < 10                    |               |
| <=                                  | Less than or equal to     | i <= j                    |               |
| \>                                  | Greater than              | i > 0                     |               |
| \>=                                 | Greater than or equal to  | count >= 90               |               |
| 9: Equality                         |                           |                           | left to right |
| ==                                  | Equal to                  | result == 0               |               |
| !=                                  | Not equal to              | c != EOF                  |               |
| 8: Bitwise AND                      |                           |                           | left to right |
| &                                   | Bitwise AND               | word & 077                |               |
| 7: Bitwise XOR                      |                           |                           | left to right |
| ^                                   | Bitwise XOR               | word1 ^ word2             |               |
| 6: Bitwise OR                       |                           |                           | left to right |
| &#124;                              | Bitwise OR                | word &#124; bits          |               |
| 5: Logical AND                      |                           |                           | left to right |
| &&                                  | Logical AND               | j > 0 && j < 10           |               |
| 4: Logical OR                       |                           |                           | left to right |
| &#124;&#124;                        | Logical OR                | i > 80 &#124;&#124; ready |               |
| 3: Conditional                      |                           |                           | right to left |
| ?:                                  | Conditional operator      | a > b ? a : b             |               |
| 2: Assignment                       |                           |                           | right to left |
| = /= %= += -= &= ^= &#124;= <<= >>= | Assignment operators      |                           |               |
| 1: Collection                       |                           |                           | left to right |
| ,                                   | Comma operator            | i = 10, j = 0             |               |

# Statements
Statements are the basic blocks of a program.

**[Syntax](Cmpl.g4)**
```antlrv4
statement
    : ';'                                                                                               # EmptyStatement
    | 'inline' Literal ';'                                                                            # IncludeStatement
    | qualifiers? '{' statementList '}'                                                              # CompoundStatement
    | qualifiers? 'if' '(' for_init ')' statement ('else' statement)?                                      # IfStatement
    | qualifiers? 'for' '(' for_init? ';' expression? ';' expression? ')' statement                       # ForStatement
    | 'for' '(' variable ':' expression ')' statement                                                 # ForEachStatement
    | 'return' initializer? ';'                                                                        # ReturnStatement
    | 'break' ';'                                                                                       # BreakStatement
    | 'continue' ';'                                                                                 # ContinueStatement
    | expression ';'                                                                               # ExpressionStatement
    | declaration                                                                                 # DeclarationStatement
    ;
```

## Block statement
Block statement groups zero ore more statement as a single statement.

## If statement
The if statement is a selection statement, that can be used to execute a section of code, only if a condition is met.

The main purpose of the if statement is to handle exceptional cases in the control flow.

### Static if statement
The static if construct can be used as a compile time check.

**Example**
```
// if double is not defined, define it as float64
static if (struct(double) == null) {
inline double = float64;
}
```

- if the condition evaluates to true:
  - the declarations contained by the block will be exposed to the outer scope.

- if the condition evaluates to false:
  - declarations contained by the block will be exposed only to the inner scope.
  - statements inside the scope will be type-checked.
  - compilation error will be reported as warnings.
  - inlined files will be not processed.
  - code generation will be skipped.

## For statement
The for statement is a repetition statement, that can be used to execute a section of code in a loop, while a condition is met.

The syntax of the for statement is similar to c like language for statement syntax.

### For-each statement
The for-each statement enumerates the elements of an iterable.

To use the foreach like form of the for statement, two functions are required to be defined:
* `iterator`: this function prepares the iterator from a type.
	* the argument for this function is the object you want to iterate.
	* it should return an iterable type(this will be the first argument of the next function).
* `next`: this function advances to the next iterable element, it may be defined also in the iterator.
	* the first argument is the object returned by the iterator function. This has to be passed by reference.
	* the second argument is optional, in case we want to iterate with a value, and not the iterator object. This argument must be passed by reference or as inout.
	* it must return a boolean value: true if there was a next element, false otherwise.

**Example**
```
struct Range {
	int min;
	int max;
}
inline Range(int min, int max) = { min: min, max: max };

struct RangeIterator {
	int current;
	int end;

	// make RangeIterator iterable using an int
	static bool next(RangeIterator &it, int &value) {
		if (this.current >= this.end) {
			return false;
		}

		value = it.current;
		it.current += 1;
		return true;
	}
}

// make the iterator for the Range type (make Range iterable)
RangeIterator iterator(Range r) {
	return {
		current: r.min;
		end: r.max;
	};
};

// now we can iterate over the range with an int value
for (int i : Range(10, 20)) {
	println(i);
}

```

### Static for statement
[TODO: implementation]

The static construct of the statement expands inline the statement of the loop.

**Example**
```
static for (int i = 0; i < 5; i += 1) {
	print(i);
}
```

generates exactly the same code like:

```
print(0);
print(1);
print(2);
print(3);
print(4);
```

## Break statement
The break statement terminates the execution of the innermost enclosing loop.

[TODO: example]

## Continue statement
The continue statement terminates the current and begins the next iteration of the innermost enclosing loop.

[TODO: example]

## Return statement
The return statement terminates the execution of the current function.

[TODO: example]
- If an expression is given, before returning the expression is assigned to the result parameter of the function.
- It is allowed to return an expression of type void (invocation of a function returning void),
even if the function specifies a void return type. The expression will be evaluated, but nothing will be returned.

## Declaration statement
Declaration statement declares a typename, function, variable or an alias.

## Expression statement
Expression statements is an expression terminated with ';'

Only some expressions can be used to form a statement:
- assignment: `a = 2;`
- invocation: `foo();`

Expression statements such as `a * 4;` are considered invalid.

# Declarations

Declaration adds new variables, functions or types to the program.

**[Syntax](Cmpl.g4)**

```antlrv4
declaration
    : qualifiers? 'enum' identifier? (':' typename)? '{' propertyList '}'                              # EnumDeclaration
    | qualifiers? 'struct' identifier? (':' (Literal | typename))? '{' declarationList '}'             # TypeDeclaration
    | qualifiers? 'inline' identifier ('(' parameterList? ')')? '=' initializer ';'                   # PropertyOperator
    | qualifiers? 'inline' '('')' ('(' parameterList? ')')? '=' initializer ';'                        # InvokerOperator
    | qualifiers? 'inline' '['']' ('(' parameterList? ')')? '=' initializer ';'                        # IndexerOperator
    | qualifiers? 'inline' unary ('(' parameterList? ')')? '=' initializer ';'                           # UnaryOperator
    | qualifiers? 'inline' arithmetic ('(' parameterList? ')')? '=' initializer ';'                 # ArithmeticOperator
    | qualifiers? 'inline' bitwise ('(' parameterList? ')')? '=' initializer ';'                       # BitwiseOperator
    | qualifiers? 'inline' relational ('(' parameterList? ')')? '=' initializer ';'                 # RelationalOperator
    | qualifiers? 'inline' equality ('(' parameterList? ')')? '=' initializer ';'                     # EqualityOperator
    | qualifiers? (variable | function) ( '=' initializer)? ';'                                    # VariableDeclaration
    | qualifiers? function '{' statementList '}'                                                # FunctionImplementation
    ;
```

## Variables

A [Variable](https://en.wikipedia.org/wiki/Variable_(computer_science)) is
a symbolic name associated with a value, this value may be changed at runtime.

### Static variables

Variables marked with the `static` attribute will point to the same global memory.
Initialization of all static variables are executed when the main function is executed.

**Example**: call count

```cmpl
int countCalls() {
	// global count variable
	static int count = 0;

	return count += 1;
}
```

**Example**: instance count

```
struct instance {
	// global count variable
	static int count = 0;

	// instance variable
	int instanceNr;
}

// construct the next instance
inline instance() = {
	instanceNr: instance.count += 1;
};
```

### References

Variables marked with one of the following suffixes, will become references to variables:

* `!`: immutable reference - the referenced value is read only
* `?`: optional reference - the referenced value can be null
* `&`: mutable reference - the referenced value is writeable
* `&?`: mutable optional reference - [TODO]

**Note**: for constant values static immutable references can be used:
```cmpl
static double constant! = 3.1415;
```


### Initialization

[TODO]

**Example**: initializer function

```
// call the initializer function
Complex x = Complex(1, 2);

```

**Example**: literal initialization

```
// literal initialization
Complex x = {re: 1, im: 2};

// literal initialization with type
Model x = Sphere {x:0, y:0, z:0, radius: 20};
```

**Example**: array initialization

```
// initialize all elements with the value 4
int a[100] = {*: 4};

// initialize the first 3 elements with the given values
// the rest will be initalized with 64
int a[100] = {1, 2, 3, *: 64};

// initialize all elements with null, then override some of them
string xmlEscape[255] = {
	*: null;
	'"': "&quot;";
	'\'': "&apos;";
	'<': "&lt;";
	'>': "&gt;";
	'&': "&amp;";
};
```

#### Default type initializer

Some of the builtin types have default type initializer (int, float, ...),
and some must be initialized when a new instance is created (pointer, variant, typename, function).
All enumerated types should have no default initializer, so they must be explicitly initialized.

**Example**

```
int a;       // ok, a is initialized with default type initializer.
typename b;  // error: variable `b` must be initialized.
```

#### Default field initializer

All constant fields of a record must be initialized when creating an instance.

**Example**
```
Complex x;                     // error: variable `x` must be initialized.
Complex x = {};                // error: all fields of `x` must be initialized.
Complex x = {re: 2};           // ok, `im` initialized with default field initializer.
Complex x = {re: 2; im: -1;};  // ok, all fields are initialized.
```

## Arrays

The array type is a collection of zero or more elements of the same type.

### Fixed-size arrays
The fixed size array will allocate the space required for all the elements

**Example**
```
int a[2] = {42, 97};
```

- Are passed to functions by reference.
- Type of elements and length is known by the compiler.

### Dynamic-size arrays
Slices are a pair of a pointer to the first element and an integer containing the length of the slice.

**Example**
```
int a[] = {42, 97, 13};
```

- Are initialized, assigned and passed to functions by reference followed by the length.
- Type of elements is known by the compiler.
- The length is known at runtime.

### Unknown-size arrays
Pointers are arrays without length. This type is helpful to pass data to native functions.
Use `pointer` for the special `void *` type from c, when the type of elements is unknown.

**Example**
```
int a[*];
```

- Are initialized, assigned and passed to functions by reference.
- Type of elements is known by the compiler.
- The length may be known by the developer.

### Associative arrays
[TODO: implementation]

**Example**

```
double constants[string] = {
	pi: Math.pi;
	"e": Math.e;
};

assert(constants["pi"] == Math.pi);
assert(constants["e"] == Math.e);
assert(constants.e == Math.e);
```

Collection types(Array, Stack, Queue, Set, Bag, Map, …) are part of the run-time library.

## Functions
[TODO: documentation]

- Functions that are not implemented, will be converted to function references.
- Functions references must be initialized, or implemented (re-declared as implemented function).
- Functions are assigned and passed to functions as delegates:
	- for inline functions the current stack pointer is pushed to give access to local variables.
	- for member functions the objects reference is pushed to give access to the _self_ variable.

**Example**
```
float64 sum(float64 data...) {
	double sum = 0;
	foreach(data, inline void(float64 value) {
		// sum is a closure variable.
		sum += value;
	});
	return sum;
}
```

### Forward declared functions
Forward declared functions are functions that can be used before they are implemented.
Every forward declared function will be converted to a function reference (can decrease performance).

**Example**
```
// forward declaration of function (no implementation).
double forward();

// invoke the function.
double x = forward();

// implementaton of forward declared function.
double forward() {
	return Math.pi;
}
```

### Function references
Function references can be initialized to point to a function.
If a function's parameter is a function it is converted to a function reference.

**Example**
```
// referencing the sin function
double reference(double value) = Math.sin;

// invoke the function.
double x = reference();

// compare is a function reference.
void sort(int values[], int compare(int a, int b)) {
	...
}
```

### Functions with special names

If a function name is a type name, by convention it is considered to be a constructor for that type,
so it should return an instance of the type represented by its name.

**Example**
```cmpl
inline string(bool value) = value ? "true" : "false";  // ok returns string.
inline string(bool value) = value ? 42 : 0;  // w2 function 'string' returns 'int'
int string() { ... } // w2 function 'string' returns 'int'
```

<!-- todo:
## Special functions (pointer, typename and variant)
```cmpl
int value = 42;

// get the offset of the variable:
pointer p = pointer(value);

// get the type of the variable:
typename t = struct(value);

// using typename inside the static if test expression
// returns null if the symbol is not defined
static if (struct(float) == null) {
  // float is not defined, so define it
  inline float = float32;
}

// variant construction:
variant v = variant(value)
// does the same as:
variant v = variant(struct(value), pointer(value));
```
-->

## Enumerations
An enumeration declares a list of named constants all of a given type.

- It may also add a new type, the enumeration type, if the enumeration name is specified.
- The enumeration type is a reference type, and its instances may be assigned
only with null or the values listed inside the enumeration.
- The type of the enumerated values can be specified.
If not specified the type of elements of the enum will be int.
- Every enumeration variable declaration must be initialized.

**Example**
```
struct vec3f {
	float x;
	float y;
	float z;
}

enum coord: vec3f {
	x: vec3f(1, 0, 0);
	y: vec3f(0, 1, 0);
	z: vec3f(0, 0, 1);
}

coord e1 = coord.x;             // ok: enum variable can be assigned with a value from the enumeration.
coord e2 = e1;                  // ok: enum variable can be assigned with the same type of variable.
e2 = coord.x;                   // ok.
e2 = e1;                        // ok.

vec3f v1 = coord.y;             // ok: values from enum are of type vec3f, so it can be assigned.
vec3f v2 = e1;                  // ok: enum variables can be assigned to variables of their base type.
v2 = coord.x;                   // ok.
v2 = e2;                        // ok.

coord e3 = v1;                  // error: enum variable can be assigned only from a value from the enumeration.
e3 = v1;                        // error.
coord e0;                       // error: enum variable must be initialized.
```

### Anonymous enumeration
An anonymous enumeration does not create the enumeration type,
and it exposes the enumerated values to the declaring scope.

**Example**
```
struct complex {
	float64 re;       // real
	float64 im = 0;   // imaginary

	enum: complex {
		zero: { re: 0 };
		unit: { re: 1 };
	}
}
```

### Indexing enumeration
The enumerated type can be indexed with ordinal(int) or name(string).

**Example**: ordinal
```
assert(coord[1] == coord.y);
```

**Example**: name
```
assert(coord["y"] == coord.y);
```

### Iterating enumeration
The values of an enumerated type can be iterated using the for-each statement.

**Example**
```
for (coord elem: coord) {
	...
}
```

## Records

[Records](https://en.wikipedia.org/wiki/Record_(computer_science)) are user specified compound types.

Records may contain only instance or static members and methods:

### Members

[TODO: documentation]

### Static members

```
struct Math {
	static float64 pi! = 3.14159265358979323846264338327950288419716939937510582097494459;
	...
	static float64 min(float64 a, float64 b) {
		return a < b ? a : b;
	}
	...
}

float64 two_pi = 2 * Math.pi;

float64 minimum = Math.min(a, b);
```

if a member/method inside the record is declared `static`, it will become a global variable/function, and
it will be accessible only through the declaring type:

- `pi` is a `static` member, and can be accessed as `Math.pi`.

- `min` is a `static` method, and can be accessed as `Math.min`.

- if a `static` method and is initialized, it will become a [function reference](#function-references)

- if a `static` method is not implemented, it will become a [forward declared function](#forward-declared-functions)

### Methods

[TODO: fix documentation]

**Example**

```
struct TextReader: Closeable {
	ByteReader reader!;

	// abstract method
	int decode(char chars[], ByteReader reader);

	// virtual method
	int read(TextReader this, char chars[]) {
		return this.decode(chars, this.reader);
	}

	// static method
	static int read(TextReader this) {
		char chars[1];
		result = this.read(chars);
		if (result > 0) {
			result = chars[0];
		}
		return result;
	}

	// overrided method
	void close(TextReader this) {
		this.reader.close();
	}

	// ...
}
```

if a method inside the record is not implemented or initialized, it will behave as an **abstract** method.

- the method must be overridden in a record that extends this record.

- the method will be looked up at runtime, not at compile time.

if a method inside the record is initialized, it will behave as a **virtual** method.

- the method can be overridden in a record that extends this record.

- the method will be looked up at runtime, not at compile time.

- should be used only to delegate the implementation.

- it will declare only the member function.

if a method inside the record is implemented, it will behave as a **virtual** method.

- the record will declare both the member and the static function with the same name.

- the method can be overridden in a record that extends this record.

- the method will be looked up at runtime, not at compile time.

In the example:

- `reader` is declared as constant reference, so it must be initialized on instance creation, and can not be changed by assignment.

	- the compiler will reject assignments as `instance.reader = null;`

- `int decode(char chars[], ByteReader reader)` is **abstract**, it must be overridden in the inheritance chain.

- `int read(TextReader this, char chars[])` is **virtual**, it can be overridden in the inheritance chain.
	- invoked as `TextReader.read(instance)` ensures that the original method is invoked.
	- invoked as `instance.read()` might call an _extension_ method.

- `int read(TextReader this)` is **static** method, it will be looked up at compile time.
	- invoked as `TextReader.read(instance, buff)` ensures that the original method is invoked.
	- invoked as `instance.read(buff)` might call an _extension_ or an _overridden_ method.

- `void close(Utf8Reader this)` overrides the abstract method from the base class `Closeable`.

### Static methods

[TODO: fix documentation]

### Static records

Records which are declared `static` will have all members static (sort of a namespaces).
These types of records will have no size, and can not be instantiated.
The best example of its usage is `static struct Math {...}`

### Packed records

A record can be packed with a small integer of a power of 2. [0, 1, 2, 4, 8, 16, 32],
this means that the compiler will not generate gaps between the members to achieve best performance.

**Example**

```
struct union32: 0 {
	int32 i32;
	float32 f32;
}

union32 fi32 = { i32: 3 };
```

- When a struct is packed with 0, it becomes a c like union,
meaning every member of it will start at the same memory location.

**Example**

```cmpl
struct instruction: 1 {
	uint8 opc;	// operation
	int32 arg;	// argument
}
```

- When a record is packed with 1, the compiler will not generate any gap between the members,
this may result in very expensive memory access to the members.

### Extended records

Extended records are usually allocated on the heap, and are automatically released,
when there are no more references pointing to the object.
The compiler however should allocate on the heap only variables that escape their scope.
The base type of every extended type is the builtin type `object`.

**Example**

```
struct Complex: object {
	double re;
	double im = 0;
}

Complex c1 = { re: 8 };
```

The variable `c1`:

- might be allocated on heap.

- is initialized with: `re: 8` and `im: 0`;

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

## Operator overloading

Operators can be overloaded using the `inline` keyword.

### Type construction/conversion operator

**Example**

```
inline complex(float64 re) = { re: re };
complex a = complex(9);
```

**Example**

```
struct Celsius { double degrees; }
struct Fahrenheit { double degrees; }

// explicit initialization
inline Celsius(float64 value) = { degrees: value };
inline Fahrenheit(float64 value) = { degrees: value };

// explicit conversion
inline Celsius(Fahrenheit value) = Celsius((value.degrees - 32) / 1.8);
inline Fahrenheit(Celsius value) = Fahrenheit(value.degrees * 1.8 + 32);

// implicit conversion
inline (float64 value) = Celsius(value);
inline (float64 value) = Fahrenheit(value);
inline (Fahrenheit value) = Celsius(value);
inline (Celsius value) = Fahrenheit(value);

Celsius boilC = Celsius(100.);                 // => inline Celsius(float64 value)
Fahrenheit boilF = Fahrenheit(boilC);          // => inline Fahrenheit(Celsius value)

Celsius boilImplicitC = 100.;                  // => inline (float64 value) = Celsius(value)
Fahrenheit boilImplicitF = boilC;              // => inline (Celsius value) = Fahrenheit(value)
```

### Unary and binary operators

**Example**

```
inline -(Complex a) = Complex(-a.re, -a.im);
inline +(Complex a, Complex b) = Complex(a.re + b.re, a.im + b.im);

Complex a = Complex(3);                 //  3 + 0 * i
Complex b = -a;                         // -3 + 0 * i
Complex c = a + a;                      //  6 + 0 * i
```

### Property/Extension operators

**Example**

```
Complex a = Complex(3);                 // 3 + 0 * i
Complex b = Complex(5, 1);              // 5 + 1 * i

inline abs(Complex a) = Math.hypot(a.re, a.im);
double x = a.abs();

inline add(Complex a, Complex b) = Complex(a.re + b.re, a.im + b.im);
Complex sum = a.add(b);

inline [](Complex c, int idx] = idx == 0 ? c.re : idx == 1 ? c.im : Math.Nan;
inline [](Complex c, string idx] = idx == "re" ? c.re : idx == "im" ? c.im : Math.Nan;
inline ()(Complex c, int idx) = c[i];

float64 re = a[0];        // => inline [](Complex c, int idx)
float64 im = a["im"];     // => inline [](Complex c, string idx)
float64 re2 = a(0);       // => inline ()(Complex c, int idx)
```

# Type system
Every declared type is also a static variable referencing its metadata,
the internal representation of the symbol used by the compiler which is exposed to the run-time.

## Builtin types
The most fundamental types the virtual machine can work with, are exposed to the compiler.

### void
The type void is an empty type with no values.
No variables of this type can be instantiated.
Mostly used when function does not return a value.
It can also be used as a function to void(finalize) a variable.

### bool
May take on the values true and false only.
true and false are defined constants in the language.

### char
1 byte, ASCII character.

### int8
1 byte, signed (two's complement).
Covers values from -128 to 127.

### int16
2 bytes, signed (two's complement),
Covers values from -32,768 to 32,767.

### int32
4 bytes, signed (two's complement).
Covers values from -2,147,483,648 to 2,147,483,647.

### int64
8 bytes signed (two's complement).
Covers values from -9,223,372,036,854,775,808 to +9,223,372,036,854,775,807.

### uint8
1 byte, unsigned (two's complement).
Covers values from 0 to 255.

### uint16
2 bytes, unsigned (two's complement).
Covers values from 0 to 65,535.

### uint32
4 bytes, unsigned (two's complement).
Covers values from 0 to 4,294,967,295.

### uint64
8 bytes, unsigned (two's complement).
Covers values from 0 to 18,446,744,073,709,551,615.

### float32
4 bytes, IEEE 754.
Covers a range from 1.40129846432481707e-45 to 3.40282346638528860e+38
(positive or negative).

### float64
8 bytes IEEE 754.
Covers a range from 4.94065645841246544e-324 to 1.79769313486231570e+308
(positive or negative).

### pointer
`pointer` is a data type whose value refers directly to (or "points to") another value.
It contains utility functions for low level memory operations (memset, memcpy, ...).

Using it as a function with an identifier argument, it will return the address of the variable.
This might be useful to compare if variable references point to the same value or object.

### variant
`variant` is a dynamic type, which carries the type of the value, and a pointer to the value.

it may be defined as:
```
struct variant {
	typename type!;
	pointer value!;
}
```

### typename
`typename` is the compilers internal symbol representation structure.
It contains static utility functions for reflection.

Using it as a function with an identifier argument, it will return the type of the variable:
- if the identifier is not defined, it will return `null`
- if the identifier is a type, it will return `typename`
- if the identifier is a variable, it will return its type
- if the identifier is a variant variable, it will extract the type

**Example**
```
// if integer is not defined, define it as int32
static if (struct(integer) == null) {
inline integer = int32;
}
```

**Example**
```
// check if int128 is defined and is a type.
bool hasInt128 = struct(int128) == typename;
```

### function
`function` is the base type of all functions.

### object
[TODO: implementation]

`object` is the base type for all heap allocated reference types.

Every type inherited from object will be reference counted,
and destroyed when there are no more references pointing to it.

## Builtin aliases
Builtin type aliases:
- `int`: alias for `int32` or `int64` depending on the word size of the vm

Builtin constants:
- `null`: pointer value, representing missing data.

## Builtin functions
[TODO: documentation]

### The `raise` builtin function.
The function can be used to log a message with or without the current stacktrace and even to abort the execution.

`void raise(int level, int trace, string message, variant details...);`

Arguments:
- `level`: level to be used:
	- `raise.abort`
	- `raise.error`
	- `raise.warn`
	- `raise.info`
	- `raise.debug`
	- `raise.verbose`

- `trace`: prints the last n stack trace, available only in debug mode.
- `message`: the message to be printed.
- `details`: array of variant data to be inspected, useful in assertions.

In case the raise function is invoked with `abort` the message is logged, and the execution of the application is aborted.
Using other levels the execution continues, and the message is logged only if the corresponding level is enabled.
In addition, the function logs the file name and line number where the function was invoked.

### The `tryExec` builtin function.
There is no `try` `catch` statement built into the language to handle exceptions.
The `tryExec` function can be used to recover the execution if it was aborted by an error.

`int tryExec(pointer args, void action(pointer args));`

the function executes the given action, returning 0 if it was executed with success.

possible return values are defined by the implementation enumeration:

```
enum {
	noError,
	illegalState,
	stackOverflow,
	divisionByZero,
	illegalMemoryAccess,
	illegalInstruction,
	nativeCallError
}
```

* [TODO] the function should return errors instead of codes
* [TODO] expose the enumeration of errors to the compiler
* [TODO] raise should abort including user defined errors


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
|        | 0        | 1        | 2         | 3         | 4         | 5        | 6        | 7         | 8          | 9         | a        | b        | c         | d         | e          | f            |
|:------:|:---------|:---------|:----------|:----------|:----------|:---------|:---------|:----------|:-----------|:----------|:---------|:---------|:----------|:----------|:-----------|:-------------|
| 0:sys  | nop      | nfc      | call      | ret       | jmp       | jnz      | jz       | ~~fork~~  | ~~join~~   | ~~trap~~  | not      | inc      | mad       | -         | inc.sp     | ~~copy.mem~~ |
| 1:stk  | load.c32 | load.c64 | load.z32  | load.z64  | load.z128 | dup.x32  | dup.x64  | dup.x128  | set.x32    | set.x64   | set.x128 | mov.x32  | mov.x64   | mov.x128  | load.sp    | ~~load.ref~~ |
| 2:mem  | load.is8 | load.iu8 | load.is16 | load.iu16 | load.i32  | load.i64 | store.i8 | store.i16 | store.i32  | store.i64 | load.m32 | load.m64 | store.m32 | store.m64 | -          | -            |
| 3:u32  | cmt      | or       | xor       | mul       | div       | rem      | -        | and       | clt        | cgt       | shl      | shr      | sar       | ext.bit   | cvt.i64    | -            |
| 4:u64  | cmt      | or       | xor       | mul       | div       | rem      | -        | and       | clt        | cgt       | shl      | shr      | sar       | -         | -          | -            |
| 5:i32  | neg      | add      | sub       | mul       | div       | rem      | -        | ceq       | clt        | cgt       | cvt.bool | cvt.i64  | cvt.f32   | cvt.f64   | -          | -            |
| 6:i64  | neg      | add      | sub       | mul       | div       | rem      | -        | ceq       | clt        | cgt       | cvt.i32  | cvt.bool | cvt.f32   | cvt.f64   | -          | -            |
| 7:f32  | neg      | add      | sub       | mul       | div       | rem      | -        | ceq       | clt        | cgt       | cvt.i32  | cvt.i64  | cvt.bool  | cvt.f64   | -          | ~~load.f32~~ |
| 8:f64  | neg      | add      | sub       | mul       | div       | rem      | -        | ceq       | clt        | cgt       | cvt.i32  | cvt.i64  | cvt.f32   | cvt.bool  | -          | ~~load.f64~~ |
| 9:p128 | neg      | add      | sub       | mul       | div       | -        | -        | ceq       | min        | max       | dp3      | dp4      | dph       | swz       | load.i128  | load.m128    |
| a:p128 | neg      | add      | sub       | mul       | div       | -        | -        | ceq       | min        | max       | -        | -        | -         | -         | store.i128 | store.m128   |
| b:p256 | -        | -        | -         | -         | -         | -        | -        | -         | -          | -         | -        | -        | -         | -         | -          | -            |
| c:p512 | -        | -        | -         | -         | -         | -        | -        | -         | -          | -         | -        | -        | -         | -         | -          | -            |
| d:extU | -        | -        | -         | -         | -         | -        | -        | -         | -          | -         | -        | -        | -         | -         | -          | -            |
| e:extI | -        | -        | -         | -         | -         | -        | -        | -         | -          | -         | -        | -        | -         | -         | -          | -            |
| f:extF | -        | -        | -         | -         | -         | -        | -        | -         | -          | -         | -        | -        | -         | -         | -          | -            |

### Instruction `nop`

Performs no operation.

* Instruction code: `0x00`
* Instruction length: 1 byte
* Requires 0 operands: […
* Returns 0 values: […

```
// nothing
```

### Instruction `nfc`

Performs native ("C") function call.

* Instruction code: `0x01`
* Instruction length: 4 bytes
* Requires 0 operands: […, `result`, `params`
* Returns 0 values: […, `result`, `params`

```
callNative(imm.u24)
```

### Instruction `call`

Performs function call.

* Instruction code: `0x02`
* Instruction length: 1 byte
* Requires 1 operand: […, `result`, `params`, `FP`
* Returns 1 value: […, `result`, `params`, `IP`

```
FP = pop();   pop function pointer
push(IP);     push current instruction pointer (return address)
IP = FP;      jump to execute the function
```

### Instruction `ret`

Performs the return from a function call, a.k.a. an indirect jump.

* Instruction code: `0x03`
* Instruction length: 1 byte
* Requires 1 operand: […, `IP`
* Returns 0 value: […

```
IP = pop();     set instruction pointer to the previously saved one
```

### Instruction `jmp`

Performs an unconditional jump.

* Instruction code: `0x04`
* Instruction length: 4 bytes
* Requires 0 operands: […
* Returns 0 values: […

```
IP += imm.i24;     add the immediate value of the instruction to the instruction pointer
```

### Instruction `jnz`

Performs a conditional jump, if the top element on the stack is non-zero.

* Instruction code: `0x05`
* Instruction length: 4 bytes
* Requires 1 operand: […, `c`
* Returns 0 value: […

```
c = pop();
if (c != 0) {
	IP += imm.i24;
}
```

### Instruction `jz`

Performs a conditional jump, if the top element on the stack is zero.

* Instruction code: `0x06`
* Instruction length: 4 bytes
* Requires 1 operand: […, `c`
* Returns 0 value: […

```
c = pop();
if (c == 0) {
	IP += imm.i24;
}
```

### Instruction `not.b32`

Performs logical not.

* Instruction code: `0x09`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 1 value: […, `!a`

```
a = pop();
push(!a);
```

### Instruction `inc.i32`

Performs the increment of the top element of the stack.

* Instruction code: `0x0a`
* Instruction length: 4 bytes
* Requires 1 operand: […, `a`
* Returns 1 value: […, `a + imm`

```
a = pop();
push(a + imm.i24);
```

### Instruction `mad.u32`

Performs addition with immediate value multiplication.

* Instruction code: `0x0b`
* Instruction length: 4 bytes
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a + b * imm`

```
uint32 b = pop();
uint32 a = pop();
push(a + b * imm.u24)
```

### Instruction `inc.sp`

Performs stack memory management (allocation or free)

* Instruction code: `0x0e`
* Instruction length: 4 bytes

```
sp += imm.i24;
```

### Instruction `copy.mem`

[todo] temporary opcode, replace with native function call pointer.move(dst, src, count).


### Instruction `dup.x32`

Makes a copy of the nth stack element.

* Instruction code: `0x10`
* Instruction length: 2 bytes
* Requires 0 operands: […, `a`
* Returns 1 values: […, `a`, `a`
	* stack change example for: dup.x32(0)

```
a = sp(imm.u8);
push(a);
```

### Instruction `dup.x64`

Makes a copy of the nth stack element.

* Instruction code: `0x11`
* Instruction length: 2 bytes
* Requires 0 operands: […, `a1`, `a2`, `b`
* Returns 2 values: […, `a1`, `a2`, `b`, `a1`, `a2`
	* stack change example for: dup.x64(1)

```
a1 = sp(imm.u8 + 0);
a2 = sp(imm.u8 + 1);
push(a1, a2);
```

### Instruction `dup.x128`

Makes a copy of the nth stack element.

* Instruction code: `0x12`
* Instruction length: 2 bytes
* Requires 0 operands: […, `a1`, `a2`, `a3`, `a4`
* Returns 4 values: […, `a1`, `a2`, `a3`, `a4`, `a1`, `a2`, `a3`, `a4`
	* stack change example for: dup.x128(0)

```
a1 = sp(imm.u8 + 0);
a2 = sp(imm.u8 + 1);
a3 = sp(imm.u8 + 2);
a4 = sp(imm.u8 + 3);
push(a1, a2, a3, a4);
```

### Instruction `set.x32`

Update the value of the nth stack element.

* Instruction code: `0x13`
* Instruction length: 2 bytes
* Requires 1 operand: […, `b`, `a`
* Returns 0 value: […, `a`
	* stack change example for: set.x32(1)

```
if (imm.u8 < 1) {
	move1(sp + imm.u8, sp);
	drop(imm.u8);
	return;
}

a = pop();
sp[imm.u8 - 1] = a;
```

### Instruction `set.x64`

Update the value of the nth stack element.

* Instruction code: `0x14`
* Instruction length: 2 bytes
* Requires 1 operand: […, `c`, `b`, `a`, `a1`, `a2`
* Returns 0 value: […, `a1`, `a2`, `a`
	* stack change example for: set.x64(3)

```
if (imm.u8 < 2) {
	move2(sp + imm.u8, sp);
	drop(imm.u8);
	return;
}

a2 = pop();
a1 = pop();
sp[imm.u8 - 2] = a2;
sp[imm.u8 - 1] = a1;
```

### Instruction `set.x128`

Update the value of the nth stack element.

* Instruction code: `0x15`
* Instruction length: 2 bytes
* Requires 4 operands: […, `a`, `b`, `c`, `d`, `a1`, `a2`, `a3`, `a4`
* Returns 0 values: […, `a1`, `a2`, `a3`, `a4`
	* stack change example for: set.x128(4)

```
if (imm.u8 < 4) {
	move4(sp + imm.u8, sp);
	drop(imm.u8);
	return;
}

a4 = pop();
a3 = pop();
a2 = pop();
a1 = pop();
sp[imm.u8 - 4] = a4;
sp[imm.u8 - 3] = a3;
sp[imm.u8 - 2] = a2;
sp[imm.u8 - 1] = a1;
```

### Instruction `mov.x32`

Performs 32-bit data copy from the source to the destination position on the stack.

* Instruction code: `0x16`
* Instruction length: 3 bytes
* Requires 0 operands: […, `a`, `b`, `c`, `d`
* Returns 0 values: […, `c`, `b`, `c`, `d`
	* stack change example for: mov.x32(dst: 3, src: 1)

```
value = st[imm.src];
st[imm.dst] = value;
```

### Instruction `mov.x64`

Performs 64-bit data copy from the source to the destination positions on the stack.

* Instruction code: `0x17`
* Instruction length: 3 bytes
* Requires 0 operands: […, `a1`, `a2`, `b1`, `b2`, `c1`, `c2`
* Returns 0 values: […, `c1`, `c2`, `b1`, `b2`, `c1`, `c2`
	* stack change example for: mov.x64(dst: 4, src: 0)

```
value1 = sp(imm.src + 0);
value2 = sp(imm.src + 1);
sp[imm.dst + 0] = value1;
sp[imm.dst + 1] = value2;
```

### Instruction `mov.x128`

Performs 128-bit data copy from the source to the destination positions on the stack.

* Instruction code: `0x18`
* Instruction length: 3 bytes
* Requires 0 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 0 values: […, `a1`, `a2`, `a3`, `a4`, `a1`, `a2`, `a3`, `a4`
	* stack change example for: mov.x128(dst: 0, src: 4)

```
a1 = sp(imm.src + 0);
a2 = sp(imm.src + 1);
a2 = sp(imm.src + 2);
a2 = sp(imm.src + 3);
sp[imm.dst + 0] = a1;
sp[imm.dst + 1] = a2;
sp[imm.dst + 2] = a3;
sp[imm.dst + 3] = a4;
```

### Instruction `load.z32`

Push zero as a 32-bit value to the stack.

* Instruction code: `0x19`
* Instruction length: 1 byte
* Requires 0 operands: […
* Returns 1 values: […, 0

```
push(0);
```

### Instruction `load.z64`

Push zero as a 64-bit value to the stack.

* Instruction code: `0x1a`
* Instruction length: 1 byte
* Requires 0 operands: […
* Returns 2 values: […, 0, 0

```
push(0, 0);
```

### Instruction `load.z128`

Push zero as a 128-bit value to the stack.

* Instruction code: `0x1b`
* Instruction length: 1 byte
* Requires 0 operands: […
* Returns 4 values: […, 0, 0, 0, 0

```
push(0, 0, 0, 0);
```

### Instruction `load.c32`

Push constant as a 32-bit value to the stack.

* Instruction code: `0x1c`
* Instruction length: 5 bytes
* Requires 0 operands: […
* Returns 1 values: […, `a`

```
push(imm.u32);
```

### Instruction `load.c64`

Push constant as a 64-bit value to the stack.

* Instruction code: `0x1d`
* Instruction length: 9 bytes
* Requires 0 operands: […
* Returns 2 values: […, `a1`, `a2`

```
push(imm.u64);
```

### Instruction `load.sp`

Performs local variable(on stack) address loading.

* Instruction code: `0x1e`
* Instruction length: 4 bytes
* Requires 0 operands: […
* Returns 1 values: […, `ref`

```
push(sp + imm.i24);
```

### Instruction `load.ref`

[todo] temporary opcode, alias for load.c32 or load.c64 depending on pointer size.


### Instruction `load.is8`

Performs 8-bit indirect memory transfer from memory to stack with sign extend.

* Instruction code: `0x20`
* Instruction length: 1 byte
* Requires 1 operand: […, `ref`
* Returns 1 value: […, `val`

```
ref = pop();
val.i32 = mem(ref).i8;  // sign extended
push(val);
```

### Instruction `load.iu8`

Performs 8-bit indirect memory transfer from memory to stack with zero extend.

* Instruction code: `0x21`
* Instruction length: 1 byte
* Requires 1 operand: […, `ref`
* Returns 1 value: […, `val`

```
ref = pop();
val.i32 = mem(ref).u8;  // zero extended
push(val);
```

### Instruction `load.is16`

Performs 16-bit indirect memory transfer from memory to stack with sign extend.

* Instruction code: `0x22`
* Instruction length: 1 byte
* Requires 1 operand: […, `ref`
* Returns 1 value: […, `val`

```
ref = pop();
val = mem(ref).i16;  // sign extended
push(val);
```

### Instruction `load.iu16`

Performs 16-bit indirect memory transfer from memory to stack with zero extend.

* Instruction code: `0x23`
* Instruction length: 1 byte
* Requires 1 operand: […, `ref`
* Returns 1 value: […, `val`

```
ref = pop();
val = mem(ref).u16;  // zero extended
push(val);
```

### Instruction `load.i32`

Performs 32-bit indirect memory transfer from memory to stack.

* Instruction code: `0x24`
* Instruction length: 1 byte
* Requires 1 operand: […, `ref`
* Returns 1 value: […, `val`

```
ref = pop();
val = mem(ref).u32;
push(val);
```

### Instruction `load.i64`

Performs 64-bit indirect memory transfer from memory to stack.

* Instruction code: `0x25`
* Instruction length: 1 byte
* Requires 1 operand: […, `ref`
* Returns 2 value: […, `a1`, `a2`

```
ref = pop();
a1, a2 = mem(ref).u64;
push(a1, a2);
```

### Instruction `load.i128`

Performs 128-bit indirect memory transfer from memory to stack.

* Instruction code: `0x26`
* Instruction length: 1 byte
* Requires 1 operand: […, `ref`
* Returns 4 value: […, `a1`, `a2`, `a3`, `a4`

```
ref = pop();
a1, a2, a3, a4 = mem(ref).u128;
push(a1, a2, a3, a4);
```

### Instruction `store.i8`

Performs 8-bit indirect memory transfer from stack to memory.

* Instruction code: `0x27`
* Instruction length: 1 byte
* Requires 2 operands: […, `val`, `ref`
* Returns 0 values: […

```
ref = pop();
val = pop();
mem[ref].u8 = val.u8;
```

### Instruction `store.i16`

Performs 16-bit indirect memory transfer from stack to memory.

* Instruction code: `0x28`
* Instruction length: 1 byte
* Requires 2 operands: […, `val`, `ref`
* Returns 0 values: […

```
ref = pop();
val = pop();
mem[ref].u16 = val.u16;
```

### Instruction `store.i32`

Performs 32-bit indirect memory transfer from stack to memory.

* Instruction code: `0x29`
* Instruction length: 1 byte
* Requires 2 operands: […, `val`, `ref`
* Returns 0 values: […

```
ref = pop();
val = pop();
mem[ref] = val;
```

### Instruction `store.i64`

Performs 64-bit indirect memory transfer from stack to memory.

* Instruction code: `0x2a`
* Instruction length: 1 byte
* Requires 3 operands: […, `a1`, `a2`, `ref`
* Returns 0 values: […

```
ref = pop();
a2 = pop();
a1 = pop();
mem[ref] = a1, a2;
```

### Instruction `store.i128`

Performs 128-bit indirect memory transfer from stack to memory.

* Instruction code: `0x2b`
* Instruction length: 1 byte
* Requires 5 operands: […, `a1`, `a2`, `a3`, `a4`, `ref`
* Returns 0 values: […

```
ref = pop();
a4 = pop();
a3 = pop();
a2 = pop();
a1 = pop();
mem[ref] = a1, a2, a3, a4;
```

### Instruction `load.m32`

Loads 32-bit value from the memory to the stack.

* Instruction code: `0x2c`
* Instruction length: 4 bytes
* Requires 0 operands: […
* Returns 1 values: […, `a`

compressed version for:
* `load.ref ref`
* `load.i32`

```
a = mem(imm.u24);
push(a);
```

### Instruction `load.m64`

Loads 64-bit value from the memory to the stack.

* Instruction code: `0x2d`
* Instruction length: 4 bytes
* Requires 0 operands: […
* Returns 2 values: […, `a1`, `a2`

compressed version for:
* `load.ref ref`
* `load.i64`

```
a1, a2 = mem2(imm.u24);
push(a1, a2);
```

### Instruction `store.m32`

Transfers 32-bit value from the stack to the memory.

* Instruction code: `0x2e`
* Instruction length: 4 bytes
* Requires 1 operand: […, `a`
* Returns 0 value: […

compressed version for:
* `load.ref ref`
* `store.i32`

```
a = pop();
mem[imm.u24] = a;
```

### Instruction `store.m64`

Transfers 64-bit value from the stack to the memory.

* Instruction code: `0x2f`
* Instruction length: 4 bytes
* Requires 2 operands: […, `a1`, `a2`
* Returns 0 values: […

compressed version for:
* `load.ref ref`
* `store.i64`

```
a2 = pop();
a1 = pop();
mem[imm.u24] = a1, a2;
```


### Instruction `cmt.b32`

Performs inversion of each bit in the top 32-bit integer operand on the stack.

* Instruction code: `0x30`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 1 value: […, `~a`

```
uint32 a = pop()
push(~a)
```

### Instruction `and.b32`

Performs bitwise and between the two top 32-bit integer operands on the stack.

* Instruction code: `0x31`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a & b`

```
uint32 b = pop()
uint32 a = pop()
push(a & b)
```

### Instruction `or.b32`

Performs bitwise or between the two top 32-bit integer operands on the stack.

* Instruction code: `0x32`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a | b`

```
uint32 b = pop()
uint32 a = pop()
push(a | b)
```

### Instruction `mul.u32`

Performs unsigned multiplication of the two top 32-bit integer operands on the stack.

* Instruction code: `0x33`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a * b`

```
uint32 b = pop()
uint32 a = pop()
push(a * b)
```

### Instruction `div.u32`

Performs unsigned division of the two top 32-bit integer operands on the stack.

* Instruction code: `0x34`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a / b`

```
uint32 b = pop()
uint32 a = pop()
push(a / b)
```

### Instruction `mod.u32`

Performs unsigned remainder of the two top 32-bit integer operands on the stack.

* Instruction code: `0x35`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a % b`

```
uint32 b = pop()
uint32 a = pop()
push(a % b)
```

### Instruction `xor.b32`

Performs bitwise exclusive or between the two top 32-bit integer operands on the stack.

* Instruction code: `0x36`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a ^ b`

```
uint32 b = pop()
uint32 a = pop()
push(a ^ b)
```

### Instruction `clt.u32`

Performs unsigned less than comparison between the two top 32-bit integer operands on the stack.

* Instruction code: `0x38`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a < b`

```
uint32 b = pop()
uint32 a = pop()
push(a < b)
```

### Instruction `cgt.u32`

Performs unsigned greater than comparison between the two top 32-bit integer operands on the stack.

* Instruction code: `0x39`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a > b`

```
uint32 b = pop()
uint32 a = pop()
push(a > b)
```

### Instruction `shl.b32`

Performs bitwise left shift of the 32-bit integer operand.

* Instruction code: `0x3a`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a << b`

```
uint32 b = pop()
uint32 a = pop()
push(a << b)
```

### Instruction `shr.b32`

Performs bitwise right shift of the 32-bit unsigned integer operand.

* Instruction code: `0x3b`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a >> b`

```
uinte32 b = pop()
uinte32 a = pop()
push(a >> b)
```

### Instruction `sar.b32`

Performs arithmetic right shift of the 32-bit signed integer operand.

* Instruction code: `0x3c`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a >>> b`

```
uint32 b = pop()
int32 a = pop()
push(a >>> b)
```

### Instruction `u32.2i64`

Performs 32-bit to 64-bit integer conversion using zero extend.

* Instruction code: `0x3e`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 2 value: […, `0`, `a`

```
uint32 a = pop()
push(uint64(a))
```

### Instruction `b32.ext`

Performs bitwise and, shl, shr with immediate value.

* Instruction code: `0x3f`
* Instruction length: 2 bytes
* Requires 1 operand: […, `a`
* Returns 1 value: […, `a op imm`

```
a = pop();
b = imm.u8 & 0x3f
op = imm.u8 >> 6;
swotch(op) {
	case 0:
		push(a & ((1 << b) - 1))
		break
	case 1:
		push(a << b)
		break
	case 2:
		push(int32(a) >> b)
		break
	case 3:
		push(uint32(a) >> b)
		break
}
```


### Instruction `cmt.b64`

Performs inversion of each bit in the top 64-bit integer operand on the stack.

* Instruction code: `0x40`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 2 values: […, `~a`, `~a`

```
uint64 a = pop2()
push(~a)
```

### Instruction `and.b64`

Performs bitwise and between the two top 64-bit integer operands on the stack.

* Instruction code: `0x41`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a & b`, `a & b`

```
uint64 b = pop2()
uint64 a = pop2()
push(a & b)
```

### Instruction `or.b64`

Performs bitwise or between the two top 64-bit integer operands on the stack.

* Instruction code: `0x42`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a | b`, `a | b`

```
uint64 b = pop2()
uint64 a = pop2()
push(a | b)
```

### Instruction `mul.u64`

Performs unsigned multiplication on the two top 64-bit integer operands on the stack.

* Instruction code: `0x43`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a * b`, `a * b`

```
uint64 b = pop2()
uint64 a = pop2()
push(a * b)
```

### Instruction `div.u64`

Performs unsigned division on the two top 64-bit integer operands on the stack.

* Instruction code: `0x44`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a / b`, `a / b`

```
uint64 b = pop2()
uint64 a = pop2()
push(a / b)
```

### Instruction `mod.u64`

Performs unsigned remainder between the two top 64-bit integer operands on the stack.

* Instruction code: `0x45`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a % b`, `a % b`

```
uint64 b = pop2()
uint64 a = pop2()
push(a % b)
```

### Instruction `xor.b64`

Performs bitwise exclusive or between the two top 64-bit integer operands on the stack.

* Instruction code: `0x46`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a ^ b`, `a ^ b`

```
uint64 b = pop2()
uint64 a = pop2()
push(a ^ b)
```

### Instruction `clt.u64`

Performs unsigned less than comparison between the two top 64-bit integer operands on the stack.

* Instruction code: `0x48`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 1 values: […, `a < b`

```
uint64 b = pop2()
uint64 a = pop2()
push(a < b)
```

### Instruction `cgt.u64`

Performs unsigned greater than comparison between the two top 64-bit integer operands on the stack.

* Instruction code: `0x49`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 1 values: […, `a < b`

```
uint64 b = pop2()
uint64 a = pop2()
push(a > b)
```

### Instruction `shl.b64`

Performs bitwise left shift of the 64-bit integer operand.

* Instruction code: `0x4a`
* Instruction length: 1 byte
* Requires 3 operands: […, `a`, `a`, `b`
* Returns 2 values: […, `a << b`, `a << b`

```
uint32 b = pop()
uint64 a = pop2()
push(a << b)
```

### Instruction `shr.b64`

Performs bitwise right shift of the 64-bit unsigned integer operand.

* Instruction code: `0x4b`
* Instruction length: 1 byte
* Requires 3 operands: […, `a`, `a`, `b`
* Returns 2 values: […, `a >> b`, `a >> b`

```
uinte32 b = pop()
uinte64 a = pop2()
push(a >> b)
```

### Instruction `sar.b64`

Performs arithmetic right shift of the 64-bit signed integer operand.

* Instruction code: `0x4c`
* Instruction length: 1 byte
* Requires 3 operands: […, `a`, `a`, `b`
* Returns 2 values: […, `a >>> b`, `a >>> b`

```
uint32 b = pop()
int64 a = pop2()
push(a >>> b)
```


### Instruction `neg.i32`

Performs sign change on the topmost 32-bit integer operand.

* Instruction code: `0x50`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 1 value: […, `-a`

```
int32 a = pop()
push(-a)
```

### Instruction `add.i32`

Performs addition of the two top 32-bit integer operands on the stack.

* Instruction code: `0x51`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a + b`

```
int32 b = pop()
int32 a = pop()
push(a + b)
```

### Instruction `sub.i32`

Performs subtraction of the two top 32-bit integer operands on the stack.

* Instruction code: `0x52`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a - b`

```
int32 b = pop()
int32 a = pop()
push(a - b)
```

### Instruction `mul.i32`

Performs signed multiplication of the two top 32-bit integer operands on the stack.

* Instruction code: `0x53`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a * b`

```
int32 b = pop()
int32 a = pop()
push(a * b)
```

### Instruction `div.i32`

Performs signed division between the two top 32-bit integer operands on the stack.

* Instruction code: `0x54`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a / b`

```
int32 b = pop()
int32 a = pop()
push(a / b)
```

### Instruction `mod.i32`

Performs signed remainder of the two top 32-bit integer operands on the stack.

* Instruction code: `0x55`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a % b`

```
int32 b = pop()
int32 a = pop()
push(a % b)
```

### Instruction `ceq.i32`

Performs equality comparison of the two top 32-bit integer operands on the stack.

* Instruction code: `0x57`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a == b`

```
int32 b = pop()
int32 a = pop()
push(a == b)
```

### Instruction `clt.i32`

Performs signed less than comparison of the two top 32-bit integer operands on the stack.

* Instruction code: `0x58`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a < b`

```
int32 b = pop()
int32 a = pop()
push(a < b)
```

### Instruction `cgt.i32`

Performs signed greater than comparison of the two top 32-bit integer operands on the stack.

* Instruction code: `0x59`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a > b`

```
int32 b = pop()
int32 a = pop()
push(a > b)
```

### Instruction `i32.2bool`

Performs 32-bit integer to boolean conversion.

* Instruction code: `0x5a`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 1 value: […, `a != 0`

```
int32 a = pop()
push(a != 0)
```

### Instruction `i32.2i64`

Performs 32-bit to 64-bit integer conversion using sign extend.

* Instruction code: `0x5b`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 2 value: […, `a`, `a`

```
int32 a = pop()
push(int64(a))
```

### Instruction `i32.2f32`

Performs 32-bit integer to 32-bit floating point conversion.

* Instruction code: `0x5c`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 1 value: […, `a`

```
int32 a = pop()
push(float32(a))
```

### Instruction `i32.2f64`

Performs 32-bit integer to 64-bit floating point conversion.

* Instruction code: `0x5d`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 2 value: […, `a`, `a`

```
int32 a = pop()
push(float64(a))
```


### Instruction `neg.i64`

Performs sign change on the topmost 64-bit integer operand.

* Instruction code: `0x60`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 2 values: […, `-a`, `-a`

```
int64 a = pop2()
push(-a)
```

### Instruction `add.i64`

Performs addition of the two top 64-bit integer operands on the stack.

* Instruction code: `0x61`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a + b`, `a + b`

```
int64 b = pop2()
int64 a = pop2()
push(a + b)
```

### Instruction `sub.i64`

Performs subtraction of the two top 64-bit integer operands on the stack.

* Instruction code: `0x62`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a - b`, `a - b`

```
int64 b = pop2()
int64 a = pop2()
push(a - b)
```

### Instruction `mul.i64`

Performs signed multiplication of the two top 64-bit integer operands on the stack.

* Instruction code: `0x63`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a * b`, `a * b`

```
int64 b = pop2()
int64 a = pop2()
push(a * b)
```

### Instruction `div.i64`

Performs signed division between the two top 64-bit integer operands on the stack.

* Instruction code: `0x64`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a / b`, `a / b`

```
int64 b = pop2()
int64 a = pop2()
push(a / b)
```

### Instruction `mod.i64`

Performs signed remainder of the two top 64-bit integer operands on the stack.

* Instruction code: `0x65`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a % b`, `a % b`

```
int64 b = pop2()
int64 a = pop2()
push(a % b)
```

### Instruction `ceq.i64`

Performs equality comparison between the two top 64-bit integer operands on the stack.

* Instruction code: `0x67`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 1 values: […, `a == b`

```
int64 b = pop2()
int64 a = pop2()
push(a == b)
```

### Instruction `clt.i64`

Performs signed less than comparison of the two top 64-bit integer operands on the stack.

* Instruction code: `0x68`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 1 values: […, `a < b`

```
int64 b = pop2()
int64 a = pop2()
push(a < b)
```

### Instruction `cgt.i64`

Performs signed greater than comparison of the two top 64-bit integer operands on the stack.

* Instruction code: `0x69`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 1 values: […, `a > b`

```
int64 b = pop2()
int64 a = pop2()
push(a > b)
```

### Instruction `i64.2i32`

Performs 64-bit to 32-bit integer conversion keeping only the low bits.

* Instruction code: `0x6a`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 1 values: […, `a`

```
int64 a = pop2()
push(int32(a))
```

### Instruction `i64.2bool`

Performs 64-bit integer to boolean conversion.

* Instruction code: `0x6b`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 1 values: […, `a != 0`

```
int64 a = pop2()
push(a != 0)
```

### Instruction `i64.2f32`

Performs 64-bit integer to 32-bit floating point conversion.

* Instruction code: `0x6c`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 1 values: […, `a`

```
int64 a = pop2()
push(float32(a))
```

### Instruction `i64.2f64`

Performs 64-bit integer to 64-bit floating point conversion.

* Instruction code: `0x6d`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 2 values: […, `a`, `a`

```
int64 a = pop2()
push(float64(a))
```


### Instruction `neg.f32`

Performs sign change on the topmost 32-bit floating point operand.

* Instruction code: `0x70`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 1 value: […, `-a`

```
float32 a = pop()
push(-a)
```

### Instruction `add.f32`

Performs addition of the two top 32-bit floating point operands on the stack.

* Instruction code: `0x71`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a + b`

```
float32 b = pop()
float32 a = pop()
push(a + b)
```

### Instruction `sub.f32`

Performs subtraction of the two top 32-bit floating point operands on the stack.

* Instruction code: `0x72`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a - b`

```
float32 b = pop()
float32 a = pop()
push(a - b)
```

### Instruction `mul.f32`

Performs multiplication of the two top 32-bit floating point operands on the stack.

* Instruction code: `0x73`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a * b`

```
float32 b = pop()
float32 a = pop()
push(a * b)
```

### Instruction `div.f32`

Performs division between the two top 32-bit floating point operands on the stack.

* Instruction code: `0x74`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a / b`

```
float32 b = pop()
float32 a = pop()
push(a / b)
```

### Instruction `mod.f32`

Performs remainder of the two top 32-bit floating point operands on the stack.

* Instruction code: `0x75`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a % b`

```
float32 b = pop()
float32 a = pop()
push(a % b)
```

### Instruction `ceq.f32`

Performs equality comparison of the two top 32-bit floating point operands on the stack.

* Instruction code: `0x77`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a == b`

```
float32 b = pop()
float32 a = pop()
push(a == b)
```

### Instruction `clt.f32`

Performs less than comparison of the two top 32-bit floating point operands on the stack.

* Instruction code: `0x78`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a < b`

```
float32 b = pop()
float32 a = pop()
push(a < b)
```

### Instruction `cgt.f32`

Performs greater than comparison of the two top 32-bit floating point operands on the stack.

* Instruction code: `0x79`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `b`
* Returns 1 values: […, `a > b`

```
float32 b = pop()
float32 a = pop()
push(a > b)
```

### Instruction `f32.2i32`

Performs 32-bit floating point to 32-bit integer conversion.

* Instruction code: `0x7a`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 1 value: […, `a`

```
float32 a = pop()
push(int32(a))
```

### Instruction `f32.2i64`

Performs 32-bit floating point to 64-bit integer conversion.

* Instruction code: `0x7b`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 2 value: […, `a`, `a`

```
float32 a = pop()
push(int64(a))
```

### Instruction `f32.2bool`

Performs 32-bit floating point to boolean conversion.

* Instruction code: `0x7c`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 1 value: […, `a != 0`

```
float32 a = pop()
push(a != 0)
```

### Instruction `f32.2f64`

Performs 32-bit to 64-bit floating point conversion.

* Instruction code: `0x7d`
* Instruction length: 1 byte
* Requires 1 operand: […, `a`
* Returns 2 value: […, `a`, `a`

```
float32 a = pop()
push(float64(a))
```

### Instruction `load.f32`

[todo] temporary opcode, alias for load.c32.


### Instruction `neg.f64`

Performs sign change on the topmost 64-bit floating point operand.

* Instruction code: `0x80`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 2 values: […, `-a`, `-a`

```
float64 a = pop2()
push(-a)
```

### Instruction `add.f64`

Performs addition of the two top 64-bit floating point operands on the stack.

* Instruction code: `0x81`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a + b`, `a + b`

```
float64 b = pop2()
float64 a = pop2()
push(a + b)
```

### Instruction `sub.f64`

Performs subtraction of the two top 64-bit floating point operands on the stack.

* Instruction code: `0x82`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a - b`, `a - b`

```
float64 b = pop2()
float64 a = pop2()
push(a - b)
```

### Instruction `mul.f64`

Performs multiplication of the two top 64-bit floating point operands on the stack.

* Instruction code: `0x83`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a * b`, `a * b`

```
float64 b = pop2()
float64 a = pop2()
push(a * b)
```

### Instruction `div.f64`

Performs division between the two top 64-bit floating point operands on the stack.

* Instruction code: `0x84`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a / b`, `a / b`

```
float64 b = pop2()
float64 a = pop2()
push(a / b)
```

### Instruction `mod.f64`

Performs remainder of the two top 64-bit floating point operands on the stack.

* Instruction code: `0x85`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 2 values: […, `a % b`, `a % b`

```
float64 b = pop2()
float64 a = pop2()
push(a % b)
```

### Instruction `ceq.f64`

Performs equality comparison of the two top 64-bit floating point operands on the stack.

* Instruction code: `0x87`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 1 values: […, `a == b`

```
float64 b = pop2()
float64 a = pop2()
push(a == b)
```

### Instruction `clt.f64`

Performs less than comparison of the two top 64-bit floating point operands on the stack.

* Instruction code: `0x88`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 1 values: […, `a < b`

```
float64 b = pop2()
float64 a = pop2()
push(a < b)
```

### Instruction `cgt.f64`

Performs greater than comparison of the two top 64-bit floating point operands on the stack.

* Instruction code: `0x89`
* Instruction length: 1 byte
* Requires 4 operands: […, `a`, `a`, `b`, `b`
* Returns 1 values: […, `a > b`

```
float64 b = pop2()
float64 a = pop2()
push(a > b)
```

### Instruction `f64.2i32`

Performs 64-bit floating point to 32-bit integer conversion.

* Instruction code: `0x8a`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 1 values: […, `a`

```
float64 a = pop2()
push(int32(a))
```

### Instruction `f64.2i64`

Performs 64-bit floating point to 64-bit integer conversion.

* Instruction code: `0x8b`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 2 values: […, `a`, `a`

```
float64 a = pop2()
push(int64(a))
```

### Instruction `f64.2f32`

Performs 64-bit to 32-bit floating point conversion.

* Instruction code: `0x8c`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 1 values: […, `a`

```
float64 a = pop2()
push(float32(a))
```

### Instruction `f64.2bool`

Performs 64-bit floating point to boolean conversion.

* Instruction code: `0x8d`
* Instruction length: 1 byte
* Requires 2 operands: […, `a`, `a`
* Returns 1 values: […, `a != 0`

```
float64 a = pop2()
push(a != 0)
```

### Instruction `load.f64`

[todo] temporary opcode, alias for load.c64.


### Instruction `neg.v4f`

Performs per component sign change on the topmost 32-bit floating point vector operand.

* Instruction code: `0x90`
* Instruction length: 1 byte
* Requires 4 operands: […, `a1`, `a2`, `a3`, `a4`
* Returns 4 values: […, `-a1`, `-a2`, `-a3`, `-a4`

```
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(-a1, -a2, -a3, -a4)
```

### Instruction `add.v4f`

Performs per component addition of the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x91`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 4 values: […, `a1 + b1`, `a2 + b2`, `a3 + b3`, `a4 + b4`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(a1 + b1, a2 + b2, a3 + b3, a4 + b4)
```

### Instruction `sub.v4f`

Performs per component subtraction of the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x92`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 4 values: […, `a1 - b1`, `a2 - b2`, `a3 - b3`, `a4 - b4`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(a1 - b1, a2 - b2, a3 - b3, a4 - b4)
```

### Instruction `mul.v4f`

Performs per component multiplication of the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x93`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 4 values: […, `a1 * b1`, `a2 * b2`, `a3 * b3`, `a4 * b4`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(a1 * b1, a2 * b2, a3 * b3, a4 * b4)
```

### Instruction `div.v4f`

Performs per component division of the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x94`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 4 values: […, `a1 / b1`, `a2 / b2`, `a3 / b3`, `a4 / b4`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(a1 / b1, a2 / b2, a3 / b3, a4 / b4)
```

### Instruction `ceq.v4f`

Performs per component equality comparison of the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x97`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 1 values: […, `a1 == b1 && a2 == b2 && a3 == b3 && a4 == b4`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(a1 == b1 && a2 == b2 && a3 == b3 && a4 == b4)
```

### Instruction `min.v4f`

Performs per component minimum calculation on the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x98`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 4 values: […, `min(a1, b1)`, `min(a2, b2)`, `min(a3, b3)`, `min(a4, b4)`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(min(a1, b1), min(a2, b2), min(a3, b3), min(a4, b4))
```

### Instruction `max.v4f`

Performs per component maximum calculation on the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x99`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 4 values: […, `max(a1, b1)`, `max(a2, b2)`, `max(a3, b3)`, `max(a4, b4)`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(max(a1, b1), max(a2, b2), max(a3, b3), max(a4, b4))
```

### Instruction `dp3.v4f`

Performs 3 element dot product calculation on the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x9a`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 1 values: […, `a1 * b1 + a2 * b2 + a3 * b3`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(a1 * b1 + a2 * b2 + a3 * b3)
```

### Instruction `dp4.v4f`

Performs 4 element dot product calculation on the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x9b`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 1 values: […, `a1 * b1 + a2 * b2 + a3 * b3 + a4 * b4`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(a1 * b1 + a2 * b2 + a3 * b3 + a4 * b4)
```

### Instruction `dph.v4f`

Performs homogeneous dot product calculation on the two top 32-bit floating point vector operands on the stack.

* Instruction code: `0x9c`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a2`, `a3`, `a4`, `b1`, `b2`, `b3`, `b4`
* Returns 1 values: […, `a1 * b1 + a2 * b2 + a3 * b3 + a4`

```
float32 b4 = pop()
float32 b3 = pop()
float32 b2 = pop()
float32 b1 = pop()
float32 a4 = pop()
float32 a3 = pop()
float32 a2 = pop()
float32 a1 = pop()
push(a1 * b1 + a2 * b2 + a3 * b3 + a4)
```

### Instruction `swz.p4x`

Performs a component wise shuffling of the elements.

* Instruction code: `0x9d`
* Instruction length: 2 bytes
* Requires 4 operands: […, `a`, `b`, `c`, `d`
* Returns 4 values: […, `a`, `b`, `a`, `b`
	* stack change example for: swz.p4x 0x44

```
float32 d = pop()
float32 c = pop()
float32 b = pop()
float32 a = pop()
shuffle(&a, &b, &c, &d, imm.u8)
push(a, b, c, d)
```

### Instruction `load.m128`

Loads 128-bit value from the memory to the stack.

* Instruction code: `0x9e`
* Instruction length: 4 bytes
* Requires 0 operands: […
* Returns 4 values: […, `a1`, `a2`, `a3`, `a4`

compressed version for:
* load.ref var
* load.i128

```
a1, a2, a3, a4 = mem4(imm.u24);
push(a1, a2, a3, a4);
```

### Instruction `store.m128`

Transfers 128-bit value from the stack to the memory.

* Instruction code: `0x9f`
* Instruction length: 4 bytes
* Requires 4 operands: […, `a1`, `a2`, `a3`, `a4`
* Returns 0 values: […

compressed version for:
* `load.ref ref`
* `store.i128`

```
a4 = pop();
a3 = pop();
a2 = pop();
a1 = pop();
mem[imm.u24] = a1, a2, a3, a4;
```


### Instruction `neg.v2d`

Performs per component sign change on the topmost 64-bit floating point vector operand.

* Instruction code: `0xa0`
* Instruction length: 1 byte
* Requires 4 operands: […, `a1`, `a1`, `a2`, `a2`
* Returns 4 values: […, `-a1`, `-a1`, `-a2`, `-a2`

```
float64 a2 = pop2()
float64 a1 = pop2()
push(-a1, -a2)
```

### Instruction `add.v2d`

Performs per component addition of the two top 64-bit floating point vector operands on the stack.

* Instruction code: `0xa1`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a1`, `a2`, `a2`, `b1`, `b1`, `b2`, `b2`
* Returns 4 values: […, `a1 + b1`, `a1 + b1`, `a2 + b2`, `a2 + b2`

```
float64 b2 = pop2()
float64 b1 = pop2()
float64 a2 = pop2()
float64 a1 = pop2()
push(a1 + b1, a2 + b2)
```

### Instruction `sub.v2d`

Performs per component subtraction of the two top 64-bit floating point vector operands on the stack.

* Instruction code: `0xa2`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a1`, `a2`, `a2`, `b1`, `b1`, `b2`, `b2`
* Returns 4 values: […, `a1 - b1`, `a1 - b1`, `a2 - b2`, `a2 - b2`

```
float64 b2 = pop2()
float64 b1 = pop2()
float64 a2 = pop2()
float64 a1 = pop2()
push(a1 - b1, a2 - b2)
```

### Instruction `mul.v2d`

Performs per component multiplication of the two top 64-bit floating point vector operands on the stack.

* Instruction code: `0xa3`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a1`, `a2`, `a2`, `b1`, `b1`, `b2`, `b2`
* Returns 4 values: […, `a1 * b1`, `a1 * b1`, `a2 * b2`, `a2 * b2`

```
float64 b2 = pop2()
float64 b1 = pop2()
float64 a2 = pop2()
float64 a1 = pop2()
push(a1 * b1, a2 * b2)
```

### Instruction `div.v2d`

Performs per component division of the two top 64-bit floating point vector operands on the stack.

* Instruction code: `0xa4`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a1`, `a2`, `a2`, `b1`, `b1`, `b2`, `b2`
* Returns 4 values: […, `a1 / b1`, `a1 / b1`, `a2 / b2`, `a2 / b2`

```
float64 b2 = pop2()
float64 b1 = pop2()
float64 a2 = pop2()
float64 a1 = pop2()
push(a1 / b1, a2 / b2)
```

### Instruction `ceq.v2d`

Performs per component equality comparison of the two top 64-bit floating point vector operands on the stack.

* Instruction code: `0xa7`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a1`, `a2`, `a2`, `b1`, `b1`, `b2`, `b2`
* Returns 1 values: […, `a1 == b1 && a2 == b2`

```
float64 b2 = pop2()
float64 b1 = pop2()
float64 a2 = pop2()
float64 a1 = pop2()
push(a1 == b1 && a2 == b2)
```

### Instruction `min.v2d`

Performs per component minimum calculation on the two top 64-bit floating point vector operands on the stack.

* Instruction code: `0xa8`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a1`, `a2`, `a2`, `b1`, `b1`, `b2`, `b2`
* Returns 4 values: […, `min(a1, b1)`, `min(a1, b1)`, `min(a2, b2)`, `min(a2, b2)`

```
float64 b2 = pop2()
float64 b1 = pop2()
float64 a2 = pop2()
float64 a1 = pop2()
push(min(a1, b1), min(a2, b2))
```

### Instruction `max.v2d`

Performs per component maximum calculation on the two top 64-bit floating point vector operands on the stack.

* Instruction code: `0xa9`
* Instruction length: 1 byte
* Requires 8 operands: […, `a1`, `a1`, `a2`, `a2`, `b1`, `b1`, `b2`, `b2`
* Returns 4 values: […, `max(a1, b1)`, `max(a1, b1)`, `max(a2, b2)`, `max(a2, b2)`

```
float64 b2 = pop2()
float64 b1 = pop2()
float64 a2 = pop2()
float64 a1 = pop2()
push(max(a1, b1), max(a2, b2))
```

# Command Line Reference

To compile and execute a script file, the cmpl application must be invoked with the corresponding arguments.

>cmpl \[global.options\]... \[file \[file.options\]...\]...

## Examples

### run

>cmpl -run test.cmpl

When the application is started in **run** mode, the compiled code is executed at full speed.

### debug

>cmpl -debug test.cmpl

When the application is started in **debug** mode, the compiled code is executed with some additional checks.
In case of an error, the debugger will pause the execution of the code at the fault location.

### profile

>cmpl -profile test.cmpl

When the application is started in **profile** mode, the compiled code is executed and measured.
When the execution finishes, the collected data is dumped to the console, or to the specified file.

### compile

>cmpl test.cmpl

If there is no global (run, debug or profile) option specified, the given source file(s) will be compiled only.

### debug with breakpoints

>cmpl -debug gl.ext.so -w0 gl.ext.cmpl -w0 test.gl.cmpl -wa -b12 -b/o15 -b/o/L19

- Start execution in debug mode `-debug`.
- Import extension library `gl.ext.so`
	- `-w0` Suppress all warnings.
- Compile extension source `gl.ext.cmpl`
	- `-w0` Suppress all warnings.
- Compile source `test.gl.cmpl`
	- `-wa` Show all warnings.
	- `-b12` Break execution each time when line 12 is hit.
	- `-b/o15` Break execution when line 15 is first hit.
	- `-b/o/L19` Print message and stack trace when line 19 is first hit.

### profile and dump to JSON

>cmpl -profile/t -api/a/d/p -dump.json test.stdc.json "test.stdc.cmpl"

- Start execution in profile mode `-profile/t`.
	- `/t` include call tree in dump
- Include symbols in dump `-api/a/d/p`
	- `/a` include all(builtin) symbols in the dump
	- `/d` include symbol details in the dump
	- `/p` include function parameters and record fields
- dump in json format `-dump.json` to file `test.stdc.json`.
- compile source file `test.stdc.cmpl`

If JSON output format is used, the generated file can be opened with the Inspector.html tool to visualize the result.

## Global options

- `-run[*]`               run at full speed, but without: debug information, stacktrace, bounds checking, ...
	- `/g /G`             dump global variable values (/G includes types and functions)
	- `/m /M`             dump memory usage (/M includes heap allocations)

- `-debug[*]`             run with attached debugger, pausing on uncaught errors and break points
	- `/g /G`             dump global variable values (/G includes types and functions)
	- `/m /M`             dump memory usage (/M includes heap allocations)
	- `/p /P`             dump caught errors (/P includes stacktrace)
	- `/t /T`             trace execution of invocations (/T includes instructions)
	- `/s /S`             pause on any error (/S pauses on startup)

- `-profile[*]`           run code with profiler: coverage, method tracing
	- `/g /G`             dump global variable values (/G includes types and functions)
	- `/m /M`             dump memory usage (/M includes heap allocations)
	- `/p /P`             dump executed statements (/P include all functions and statements)
	- `/t /T`             trace execution with timestamps (/T includes instructions)

- `-std<file>`            specify custom standard library file (empty file name disables std library compilation).

- `-mem<int>[kKmMgG]`     override memory usage for compiler and runtime(heap)

- `-log[*]<int> <file>`   set logger for: compilation errors and warnings, runtime debug messages
	- `<int>`             set the default log(warning) level 0 - 15
	- `/a`                append to the log file
	- `/d`                dump to the log file

- `-dump[?] <file>`       set output for: dump(symbols, assembly, abstract syntax tree, coverage, call tree)
	- `.scite`            dump api for SciTE text editor
	- `.json`             dump api and profile data in javascript object notation format

- `-api[*]`               dump symbols
	- `/a /A`             include all library symbols(/A includes builtins)
	- `/d`                dump details of symbol
	- `/p`                dump params and fields

- `-doc[*]`               dump documentation
	- `/a /A`             include all library symbols(/A includes builtins)
	- `/d`                dump details of symbol
	- `/p`                dump params and fields

- `-use[*]`               dump usages
	- `/a /A`             include all library symbols(/A includes builtins)
	- `/d`                dump details of symbol
	- `/p`                dump params and fields

- `-asm[*]<int>`          dump assembled code: jmp +80
	- `/a /A`             include all library symbols(/A includes builtins)
	- `/d`                dump details of symbol
	- `/p`                dump params and fields
	- `/g`                use global address: jmp @0x003d8c
	- `/n`                prefer names over addresses: jmp <main+80>
	- `/s`                print source code statements

- `-ast[*]`               dump syntax tree
	- `/a /A`             include all library symbols(/A includes builtins)
	- `/d`                dump details of symbol
	- `/p`                dump params and fields
	- `/t`                dump sub-expression type information
	- `/l`                do not expand statements (print on single line)
	- `/b`                don't keep braces ('{') on the same line
	- `/e`                don't keep `else if` constructs on the same line


## Compilation units

After the global options are processed, the remaining arguments are processed as compilation units.

- `<files with options>`  filename followed by switches
	- `<file>`            if file extension is (.so|.dll) load as library else compile
	- `-w[a|x|<int>]`     set or disable warning level for current file
	- `-b[*]<int>`        break point on <int> line in current file
		- `/[l|L]`        print only, do not pause (/L includes stack trace)
		- `/o`            one shot breakpoint, disable after first hit

