# To be implemented
- slice operator:
```
int a[200];
int b = a[10 : 20];
OR? int b = a[10 , 20];
OR? int b = a[10 : 20];
OR? int b = a[10 ... 20];
```
- default type initializer

	builtin types should have default initializer, except pointers and typename.
		ex:
			bool.class.init = false;
			int32.class.init = 0;
			float32.class.init = 0.0;

		than:
			int a; // no compile error(a == 0).
			typename b; // error: b is uninitialized.

	enum types should not have default initializer, so it is an error to declare a non initialized enum.
		example:
			[TODO]

	bool enum type should have false as the default initializer.
- default field initializer
	error should be raised if a struct is instantiated without initializing fields without default field initializer.
