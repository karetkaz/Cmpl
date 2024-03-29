/**
 * Debugger core functions.
 */
#ifndef CMPL_DBG_H
#define CMPL_DBG_H

#include "cmpl.h"
#include "utils.h"

/// Debugger context
struct dbgContextRec {
	rtContext rt;
	vmError (*debug)(dbgContext ctx, vmError, size_t ss, void *sp, size_t caller, size_t callee);

	struct arrBuffer functions;
	struct arrBuffer statements;
	size_t freeMem, usedMem;
	symn tryExec;	// the symbol of tryExec function
};

/**
 * Initialize debugger context.
 *
 * @param rt Runtime context.
 * @param onError function to be executed when an error occurs.
 * @return debugger context.
 */
dbgContext dbgInit(rtContext rt, vmError (*debug)(dbgContext ctx, vmError, size_t ss, void *sp, size_t caller, size_t callee));

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
struct dbgNode {
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
};

/**
 * Check if the current stacktrace contains tryExec, if an error can terminate the execution of the vm.
 * @param ctx debug context
 * @return true or false
 */
int isChecked(dbgContext ctx);

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
dbgn getDbgStatement(rtContext ctx, char *file, int line);
dbgn addDbgStatement(rtContext ctx, size_t start, size_t end, astn tag);

#endif
