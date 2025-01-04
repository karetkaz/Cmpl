/**
 * Abstract syntax tree and related operations
 */
#ifndef CMPL_TREE_H
#define CMPL_TREE_H

#include <cmpl.h>

/// Tokens
typedef enum {
#define TOKEN_DEF(NAME, TYPE, SIZE, OP, STR) NAME,
	#include "token.i"
	tok_last
} ccToken;

struct tokenRec {
	const char *name;
	const char *op;
	const int type;
	const int args;
};
extern const struct tokenRec token_tbl[256];

/// Abstract syntax tree node
struct astNode {
	ccToken		kind;				// token kind: operator, statement, ...
	int32_t		line;				// line position of token
	const char* file;				// file name of the token belongs to
	symn		type;				// token type: return
	astn		next;				// next token / next argument / next statement
	union {
		//char *cStr;				// constant string value (use: ref.name)
		int64_t cInt;				// constant integer value
		float64_t cFlt;				// constant floating point value
		struct {					// KIND_var: identifier
			const char *name;		// name of identifier
			unsigned hash;			// hash code for 'name'
			symn	link;			// symbol to variable
			astn	used;			// next usage of variable
		} id;
		struct {					// OPER_xxx: operator
			astn	lhso;			// left-hand side operand
			astn	rhso;			// right-hand side operand
			int		prec;			// precedence level
		} op;
		struct {					// STMT_xxx: statement
			astn	stmt;			// statement / then block
			astn	step;			// increment / else block
			astn	test;			// condition: if, for
			astn	init;			// for statement init
		} stmt;
		struct {					// STMT_ret: return
			symn func;				// return from function
			astn value;				// return the value of expression
			size_t offs;			// jump instruction offset
		} ret;
		struct {					// STMT_brk, STMT_con: break & continue
			symn scope;				// scope variables
			int32_t nest;			// stack size
		} jmp;
		struct {                    // OPER_opc
			int code;               // instruction code
			size_t args;            // instruction argument
		} opc;
		struct {					// STMT_beg: list
			astn head;
			astn tail;
		} lst;
	};
};

/// Allocate a tree node.
astn newNode(ccContext ctx, ccToken kind);

/// Duplicate a tree node.
astn dupNode(ccContext ctx, astn node);

/// Recycle node, so it may be reused.
void recycle(ccContext ctx, astn node);

/// Allocate a constant integer node.
astn intNode(ccContext ctx, int64_t value);

/// Allocate a constant float node.
astn fltNode(ccContext ctx, float64_t value);

/// Allocate a constant string node.
astn strNode(ccContext ctx, const char *value);

/// Allocate node which is a link to a reference.
astn lnkNode(ccContext ctx, symn ref);

/// Allocate an operator tree node.
astn opNode(ccContext ctx, symn type, ccToken kind, astn lhs, astn rhs);

// return constant values of nodes
int bolValue(astn ast);
int64_t intValue(astn ast);
float64_t fltValue(astn ast);

/**
 * Try to evaluate a constant expression.
 *
 * @param res Place the result here.
 * @param ast Abstract syntax tree to be evaluated.
 * @return Type of result: [TYPE_err, CAST_i64, CAST_f64, ...]
 * TODO: to be deleted; use vm to evaluate constants.
 */
ccKind eval(ccContext ctx, astn res, astn ast);

/**
 * Get the symbol(variable) linked to expression.
 * @param ast Abstract syntax tree to be checked.
 * @param follow Follow symbol links.
 * @return null if not an identifier.
 * @note if input is a the symbol for variable a is returned.
 * @note if input is a.b the symbol for member b is returned.
 * @note if input is a(1, 2) the symbol for function a is returned.
 */
symn linkOf(astn ast, int follow);

/**
 * Check if an expression is a type.
 *
 * @param ast Abstract syntax tree to be checked.
 * @return true or false.
 */
int isTypeExpr(astn ast);

/**
 * Check if a qualified variable expression is constant.
 *
 * @param ast Abstract syntax tree to be checked.
 * @return true or false.
 */
int isMutableVar(astn ast);

/**
 * Construct arguments.
 *
 * @param ctx compiler context.
 * @param lhs arguments or first argument.
 * @param rhs next argument.
 * @return if lhs is null return (rhs) else return (lhs, rhs).
 */
astn argNode(ccContext ctx, astn lhs, astn rhs);

/**
 * Construct reference node.
 *
 * @param ctx compiler context.
 * @param name name of the node.
 * @return the new node.
 */
astn tagNode(ccContext ctx, const char *name);

/**
 * Chain the arguments through ast.next link.
 * @param args root node of arguments tree.
 * todo: remove
 */
astn chainArgs(astn args);

#endif
