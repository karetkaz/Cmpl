/**
 * Debugger core functions.
 */
#ifndef CMPL_DEBUG_H
#define CMPL_DEBUG_H

#include <cmpl.h>
#include "util.h"

/// Debugger context
struct dbgContextRec {
	rtContext rt;
	vmError (*debug)(dbgContext ctx, vmError, size_t ss, void *sp, size_t caller, size_t callee);
	void (*dbgAlloc)(dbgContext ctx, void *mem, size_t size, char *kind);

	struct arrBuffer functions;
	struct arrBuffer statements;
	size_t freeMem, usedMem;
};

/// Break point actions
typedef enum {
	brkSkip = 0x00,     // do nothing
	brkPrint = 0x01,    // print when hit
	brkTrace = 0x02,    // trace when hit
	brkPause = 0x04,    // pause when hit
	brkValue = 0x08,    // print stack after hit
	brkOnce = 0x10,     // one shoot breakpoint (disabled after first hit)
} brkMode;

/// Debug info node
typedef struct dbgNode {
	// the statement tree
	astn stmt;

	// the declaring function
	symn func;

	// the declared symbol
	symn decl;

	// position in source file
	const char *file;
	int line;

	// position in byte code
	size_t start, end;

	// break point
	brkMode bp;

	// profile data
	int64_t hits;   // function or statement hit count
	int64_t fails;  // failed function calls / instructions
	int64_t total;  // total execution: function time / instruction count
	int64_t time;   // time spent executing function (self time)
} *dbgn;

/**
 * Initialize debugger context.
 *
 * @param ctx Runtime context.
 * @param debug function to be executed when an error occurs.
 * @return debugger context.
 */
dbgContext dbgInit(rtContext ctx, vmError (*debug)(dbgContext ctx, vmError, size_t ss, void *sp, size_t caller, size_t callee));

/**
 * Debugger function that prints runtime errors with stacktrace when available
 * @param ctx Debugger context
 * @param err Error why the function was invoked
 * @param ss Stack size
 * @param sp stack pointer(top of stack)
 * @param caller caller method offset
 * @param callee callee method offset
 * @return the given @param err is returned
 */
vmError dbgError(dbgContext ctx, vmError err, size_t ss, void *sp, size_t caller, size_t callee);

/**
 * Check if the current stacktrace contains tryExec, if an error can terminate the execution of the vm.
 * @param ctx debug context
 * @return true or false
 */
int isChecked(rtContext ctx);

/**
 * Convert an error code to message (string).
 *
 * @param error the error code
 * @return the error message for the given code
 */
char* vmErrorMessage(vmError error);

/**
 * Request debug information of function
 * @param ctx runtime context
 * @param offset the memory offset to lookup
 * @return the debug information for the given position
 */
dbgn mapDbgFunction(rtContext ctx, size_t offset);
dbgn addDbgFunction(rtContext ctx, symn fun);

dbgn mapDbgStatement(rtContext ctx, size_t position, dbgn prev);
dbgn getDbgStatement(rtContext ctx, const char *file, int line);
dbgn addDbgStatement(rtContext ctx, size_t start, size_t end, astn tag);

#endif
