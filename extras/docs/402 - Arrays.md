## Arrays

The array type is a collection of zero or more elements of the same type.

### Fixed-size arrays
The fixed size array will allocate the space required for all the elements
- Are passed to functions by reference.
- The compiler knows the type of elements and also the length.

**Example**
```cmpl
int a[2] = {42, 97};
```

#### Fixed-size array initialization

- [x] value initialization:
  ```cmpl
  int x[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  ```
- [x] undefined initialization can be used when the array will be populated later:
  ```cmpl
  int x[10] = {...};
  ```
- [ ] zero initialization can be used to initialize all the elements with zero:
  ```cmpl
  int x[10] = {0};
  ```
- [ ] initialize all elements with the same value:
  ```cmpl
  int x[10] = {...: 4};
  ```
- [ ] initialize the first n elements then the rest with the same value:
  ```cmpl
  int x[10] = {1, 2, 3, ...: 64};
  ```
- [ ] initialization with method:
  ```cmpl
  int x[10] = {...(int i) { return i * 2; } };
  ```
- [ ] initialize the first n elements then the rest with the method:
  ```cmpl
  int x[10] = {1, 2, 3, ...(int i) { return i * 2; } };
  ```
- [ ] initialize all elements then override some of them:
  ```cmpl
  string xmlEscape[255] = {
  ...: null;
  '"': "&quot;";
  '\'': "&apos;";
  '<': "&lt;";
  '>': "&gt;";
  '&': "&amp;";
  };
  ```

### Dynamic-size arrays
Slices are a pair of a pointer to the first element and an integer containing the length of the slice.
- Are initialized, assigned and passed to functions by reference followed by the length.
- The compiler knows the type of elements and the length is known at runtime.

**Example**
```cmpl
int a[] = {42, 97, 13};
```

#### Dynamic-size array initialization
if the initializer does not contain the rest initializer, the length is inferred from the initializer.
then a fixed size array can be created and assigned to the slice.
the previous example is the same as:

```cmpl
int a[3] = {42, 97, 13};
int a[] = a;
```

### Unknown-size arrays
Pointers are arrays without length. This type is helpful to pass data to native functions.
Use `pointer` for the special `void *` type from c, when the type of elements is unknown.
- Are initialized, assigned and passed to functions by reference.
- The compiler knows the type of elements, the developer the length of the array.

**Example**
```cmpl
int a[*];
```

### Array slicing
A slice can be created from a fixed-size array, a slice or an unknown-size array using the slice operator:

```cmpl
int a[10] = {};
int b[] = a[...4];      // the first 4 elements: a[0], … a[3]
int c[] = a[2 ... 5];   // elements a[2], a[3] and a[4] (start is inclusive, end is exclusive)
int d[] = a[3 ... 3];   // empty slice
```

- The start is optional and defaults to 0: `a[...end]` is the same as `a[0 ... end]`.
- The bounds are checked: `start <= end` must hold, and `end` must not exceed the length of the sliced array.
  Constant bounds on fixed-size arrays are verified at compile time, otherwise at runtime,
  aborting the execution with `illegalArrayAccess`.
- Unknown-size arrays (`int a[*]`) have no length, only the `start <= end` relation is validated.

### Associative arrays
[TODO: implementation]

**Example**
```cmpl
double constants[string] = {
	pi: Math.pi;
	"e": Math.e;
};

assert(constants["pi"] == Math.pi);
assert(constants["e"] == Math.e);
assert(constants.pi == Math.pi);
assert(constants.e == Math.e);
```

Collection types(Array, Stack, Queue, Set, Bag, Map, …) are part of the run-time library.
