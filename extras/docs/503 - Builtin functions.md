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

