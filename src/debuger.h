/**
 * Debugger core functions.
 */
#ifndef CMPL_DEBUG_H
#define CMPL_DEBUG_H

#include <cmpl.h>
#include <time.h>

#include "util.h"

/// Debugger context
struct dbgContextRec {
	rtContext rt;
	vmError (*debug)(dbgContext ctx, vmError, size_t pc, size_t ss, void *sp, int mode);
	void (*dbgAlloc)(dbgContext ctx, void *mem, size_t size, char *kind);

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
	// position in source file
	const char *file;
	int line;

	int group;

	// position in byte code
	size_t start, end;

	// the declaring function
	symn fun;

	// profiler data
	size_t exec;   // number of executed instructions
	size_t hits;   // function or statement hit count
	size_t traps;   // unsuccessful function call returns
	clock_t time;   // time spent executing function (self time)
	clock_t total;  // total execution: function time

	// the declared symbol to show the value in debugger
	symn decl;
	// the statement tree
	astn stmt;
	// break point
	brkMode bp;
} *dbgn;

/**
 * Initialize debugger context.
 *
 * @param ctx Runtime context.
 * @param debug function to be executed when an error occurs.
 * @return debugger context.
 */
dbgContext dbgInit(rtContext ctx, vmError (*debug)(dbgContext ctx, vmError, size_t pc, size_t ss, void *sp, int mode));

/**
 * Debugger function that prints runtime errors with stacktrace when available
 * @param ctx Debug context
 * @param error Error why the function was invoked
 * @param ss Stack size
 * @param sp stack pointer(top of stack)
 * @param pc program counter
 * @param mode 0: exec instruction, 1 call, -1 return
 * @return the given @param error is returned
 */
vmError dbgError(dbgContext ctx, vmError error, size_t pc, size_t ss, void *sp, int mode);

/**
 * Check if the current stacktrace contains tryExec, if an error can terminate the execution of the vm.
 * @param ctx Runtime context
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

dbgn mapDbgStatement(rtContext ctx, size_t position, dbgn prev);
dbgn getDbgStatement(rtContext ctx, const char *file, int line);
dbgn addDbgStatement(rtContext ctx, size_t start, size_t end, astn tag);

#endif
