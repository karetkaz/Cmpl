#include "debuger.h"
#include "code.h"
#include "tree.h"
#include "compiler.h"
#include "printer.h"

dbgContext dbgInit(rtContext ctx, vmError onExec(dbgContext ctx, vmError, size_t ss, void *sp, size_t caller, size_t callee)) {
	dbgContext dbg = (dbgContext)(ctx->_beg = padPointer(ctx->_beg, vm_mem_align));
	ctx->_beg += sizeof(struct dbgContextRec);
	ctx->dbg = dbg;

	dieif(ctx->_beg >= ctx->_end, ERR_MEMORY_OVERRUN);
	memset(dbg, 0, sizeof(struct dbgContextRec));

	dbg->rt = ctx;
	dbg->debug = onExec;
	initBuff(&dbg->functions, 128, sizeof(struct dbgNode));
	initBuff(&dbg->statements, 128, sizeof(struct dbgNode));
	return dbg;
}

char* vmErrorMessage(vmError error) {
	switch (error) {
		case noError:
			return NULL;

		case illegalState:
			return "Invalid state";

		case illegalMemoryAccess:
			return "Invalid memory access";

		case illegalInstruction:
			return "Invalid instruction";

		case stackOverflow:
			return "Stack Overflow";

		case divisionByZero:
			return "Division by Zero";

		case nativeCallError:
			return "External call aborted execution";
	}

	return "Unknown error";
}

dbgn mapDbgFunction(rtContext ctx, size_t offset) {
	dbgContext dbg = ctx->dbg;
	if (dbg == NULL) {
		return NULL;
	}

	dbgn result = (dbgn)dbg->functions.ptr;
	size_t n = dbg->functions.cnt;
	for (size_t i = 0; i < n; ++i) {
		if (offset == result->start) {
			return result;
		}
		if (offset >= result->start) {
			if (offset < result->end) {
				return result;
			}
		}
		result++;
	}
	return NULL;
}
dbgn addDbgFunction(rtContext ctx, symn fun) {
	dbgContext dbg = ctx->dbg;
	if (dbg == NULL || fun == NULL) {
		return NULL;
	}

	dbgn result = NULL;
	size_t i = 0;
	for ( ; i < dbg->functions.cnt; ++i) {
		result = getBuff(&dbg->functions, i);
		if (fun->offs <= result->start) {
			break;
		}
	}

	if (result == NULL || fun->offs != result->start) {
		result = insBuff(&dbg->functions, i, NULL);
	}

	if (result != NULL) {
		memset(result, 0, dbg->functions.esz);
		result->func = fun;
		result->file = fun->file;
		result->line = fun->line;
		result->start = fun->offs;
		result->end = fun->offs + fun->size;
	}
	return result;
}

dbgn getDbgStatement(rtContext ctx, const char *file, int line) {
	dbgContext dbg = ctx->dbg;
	if (dbg == NULL) {
		return NULL;
	}

	dbgn result = (dbgn)dbg->statements.ptr;
	for (size_t i = 0; i < dbg->statements.cnt; ++i) {
		if (result->file && strcmp(file, result->file) == 0) {
			if (line == result->line) {
				return result;
			}
		}
		result++;
	}
	return NULL;
}
dbgn mapDbgStatement(rtContext ctx, size_t position, dbgn prev) {
	dbgContext dbg = ctx->dbg;
	if (dbg == NULL) {
		return NULL;
	}

	// TODO: use binary search to speed up mapping
	dbgn result = (dbgn)dbg->statements.ptr;
	size_t n = dbg->statements.cnt;
	for (size_t i = 0; i < n; ++i) {
		if (position >= result->start) {
			if (position < result->end) {
				if (prev < result) {
					return result;
				}
			}
		}
		result++;
	}
	return NULL;
}
dbgn addDbgStatement(rtContext ctx, size_t start, size_t end, astn tag) {
	dbgContext dbg = ctx->dbg;
	if (dbg == NULL || start >= end) {
		return NULL;
	}

	if (tag != NULL && tag->kind == STMT_beg) {
		// do not add block statement to debug.
		return NULL;
	}
	if (tag != NULL && tag->kind == STMT_sif) {
		// do not add `static if` to debug.
		return NULL;
	}

	size_t i = 0;
	dbgn result = NULL;
	for ( ; i < dbg->statements.cnt; ++i) {
		result = getBuff(&dbg->statements, i);
		if (start <= result->start) {
			break;
		}
	}

	result = insBuff(&dbg->statements, i, NULL);
	if (result == NULL) {
		// insertion failed
		return NULL;
	}

	memset(result, 0, dbg->statements.esz);
	if (tag != NULL) {
		result->stmt = tag;
		result->file = tag->file;
		result->line = tag->line;
	}
	result->start = start;
	result->end = end;
	return result;
}
