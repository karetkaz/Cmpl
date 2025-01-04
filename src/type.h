/**
 * Type system
 * manage type declarations
 * type-check the syntax tree
 */
#ifndef CMPL_TYPE_H
#define CMPL_TYPE_H

#include <cmpl.h>

/**
 * Begin a namespace (static struct) or scope.
 *
 * @param ctx compiler context.
 * @param name Name of the namespace.
 * @return Defined symbol, null on error (error is logged).
 */
symn ccBegin(ccContext ctx, const char *name);
/**
 * Extend a namespace (static struct) with static members.
 *
 * @param ctx compiler context.
 * @param sym Namespace to be extended.
 * @return type if it can be extended.
 */
symn ccExtend(ccContext ctx, symn sym);
/**
 * Close the namespace returned by ccBegin or ccExtend.
 *
 * @param ctx compiler context.
 * @param sym Namespace to be closed.
 * @note Makes all declared variables static.
*/
symn ccEnd(ccContext ctx, symn sym);

/**
 * Define an integer constant.
 *
 * @param ctx compiler context.
 * @param name Name of the constant.
 * @param value Value of the constant.
 * @return Defined symbol, null on error.
 */
symn ccDefInt(ccContext ctx, const char *name, int64_t value);
/**
 * Define a floating point constant.
 *
 * @param ctx compiler context.
 * @param name Name of the constant.
 * @param value Value of the constant.
 * @return Defined symbol, null on error.
 */
symn ccDefFlt(ccContext ctx, const char *name, double value);
/**
 * Define a string constant.
 *
 * @param ctx compiler context.
 * @param name Name of the constant.
 * @param value Value of the constant.
 * @return Defined symbol, null on error.
 */
symn ccDefStr(ccContext ctx, const char *name, const char *value);

/**
 * Define a variable or reference.
 *
 * @param ctx compiler context.
 * @param name Name of the variable.
 * @param type Type of the variable.
 * @return Defined symbol, null on error.
 */
symn ccDefVar(ccContext ctx, const char *name, symn type);

/**
 * Define a new type.
 *
 * @param ctx compiler context.
 * @param name Name of the type.
 * @param size Size of the type.
 * @param refType Value or Reference type.
 * @return Defined symbol, null on error.
 * @see plugins/file.c
 */
symn ccAddType(ccContext ctx, const char *name, unsigned size, int refType);

/**
 * Define a native function.
 *
 * @param ctx compiler context.
 * @param call The native c function wrapper.
 * @param proto Prototype of the function.
 * @return Defined symbol, null on error.
 * @usage
	static vmError f64sin(nfcContext ctx) {
		float64_t x = argF64(ctx, 0);       // get first argument
		retF64(ctx, sin(x));                // set the return value
		return noError;                     // do not abort execution
	}
	...
	if (!rt->api.ccDefCall(rt, f64sin, "float64 sin(float64 x)")) {
		// error was reported, terminate by returning error count.
		return rt->errors;
	}
 */
symn ccAddCall(ccContext ctx, vmError call(nfcContext), const char *proto);

/**
 * Compile the given file or text as a unit.
 *
 * @param ctx compiler context.
 * @param init function installing types and native functions.
 * @param file File name of input.
 * @param line First line of input.
 * @param text If not null, this will be compiled instead of the file.
 * @return Root node of the compilation unit.
 */
astn ccAddUnit(ccContext ctx, const char *file, int line, char *text);

/**
 * Generate bytecode from the compiled syntax tree.
 *
 * @param ctx compiler context.
 * @param debug generate debug info
 * @return error code/count, 0 on success.
 */
int ccGenCode(ccContext ctx, int debug);

/**
 * Lookup a symbol by name.
 *
 * @param ctx compiler context.
 * @param scope search in the members of this symbol.
 * @param name name of the symbol to be found.
 * @return the first found symbol.
 */
symn ccLookup(ccContext ctx, symn scope, char *name);

/// same as castOf forcing arrays to cast as reference
static inline ccKind refCast(symn sym) {
	ccKind got = castOf(sym);
	if (got == CAST_arr && isArrayType(sym)) {
		symn len = sym->fields;
		if (len == NULL || isStatic(len)) {
			// pointer or fixed size array
			return CAST_ptr;
		}
	}
	if (got == CAST_enm && isEnumType(sym)) {
		return refCast(sym->type);
	}
	return got;
}

static inline size_t refSize(symn sym) {
	switch (refCast(sym)) {
		default:
			return sym->size;

		case CAST_ptr:
		case CAST_obj:
			return sizeof(vmOffs);

		case CAST_arr:
		case CAST_var:
			return 2 * sizeof(vmOffs);
	}
}

/**
 * check if the given filter excludes the given symbol or not
 * isFiltered(staticVariable, KIND_var | ATTR_stat) == 0
 * isFiltered(localVariable, KIND_var | ATTR_stat) != 0
 * @param sym the symbol to be checked
 * @param filter the filter to be applied
 * @return 0 if is not excluded
 */
static inline int isFiltered(symn sym, ccKind filter) {
	ccKind filterStat = filter & ATTR_stat;
	if (filterStat != 0 && (sym->kind & ATTR_stat) != filterStat) {
		return 1;
	}
	ccKind filterKind = filter & MASK_kind;
	if (filterKind != 0 && (sym->kind & MASK_kind) != filterKind) {
		return 1;
	}
	ccKind filterCast = filter & MASK_cast;
	if (filterCast != 0 && (sym->kind & MASK_cast) != filterCast) {
		return 1;
	}
	return 0;
}

size_t argsSize(symn function);

/// Allocate a symbol node.
symn newSymn(ccContext ctx, ccKind kind);

/// Enter a new scope.
void enter(ccContext ctx, astn node, symn owner);

/// Leave current scope.
symn leave(ccContext ctx, ccKind mode, size_t align, size_t baseSize, size_t *size, symn result);

/// Install a new symbol in the current scope (typename, variable, function or alias).
symn install(ccContext ctx, const char *name, ccKind kind, size_t size, symn type, astn init);

/// Lookup an identifier.
symn lookup(ccContext ctx, symn sym, astn ast, astn args, ccKind filter, int raise);

/**
 * Check if a value can be assigned to a symbol.
 *
 * @param ctx compiler context.
 * @param variable variable to be assigned to.
 * @param value value to be assigned.
 * @param strict Strict mode: casts are not enabled.
 * @return cast of the assignment if it can be done.
 */
ccKind canAssign(ccContext ctx, symn variable, astn value, int strict);

/**
 * Type Check an expression.
 *
 * @param ctx compiler context.
 * @param loc Override scope.
 * @param ast Tree to check.
 * @param raise raise the error or keep silent when type checking
 * @return Type of expression.
 */
symn typeCheck(ccContext ctx, symn loc, astn ast, int raise);

/// change the type of a tree node (replace or add implicit cast).
astn castTo(ccContext ctx, astn ast, symn type);

/**
 * @brief Add usage of the symbol.
 * @param sym Symbol typename or variable.
 * @param tag The node of the usage.
 */
void addUsage(symn sym, astn tag);

#endif
