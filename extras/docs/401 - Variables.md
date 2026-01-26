## Variables

A [Variable](https://en.wikipedia.org/wiki/Variable_(computer_science)) is
a symbolic name associated with a value and may be changed at runtime.

### Static variables

Variables marked with the `static` attribute will point to the same global memory.
Initialization of all static variables is executed when the main function is executed.

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

Variables marked with one of the following suffixes will become references to variables:

* `?`: optional reference: the reference can be null or point to a valid value
* `&`: mutable reference: the value is writeable through the reference
* `!`: const reference: the value is read-only through the reference

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

#### Default type initializer

Some of the builtin types have default type initializer (int, float, ...),
and some must be initialized when a new instance is created (pointer, variant, typename, function).
All enumerated types should have no default initializer, so they must be explicitly initialized.

**Example**
```
int a;       // ok, a is initialized with default type initializer: `0`
typename b;  // error: variable `b` must be initialized.
```

#### Default field initializer

All fields of a record must be initialized when creating an instance, if they don't have a default field initializer.

**Example**
```
struct Complex {
	int re;         // no default field initializer, must be initialized explicitly
	int im = 0;     // default field initializer: will be initialized with `0` if not specified
};

Complex x;                     // error: variable `x` must be initialized.
Complex x = {};                // error: all fields of `x` must be initialized.
Complex x = {re: 2};           // ok, `im` initialized with default field initializer.
Complex x = {re: 2; im: -1;};  // ok, all fields are initialized.
```
