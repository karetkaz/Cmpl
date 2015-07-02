/*******************************************************************************
 *   File: tree.c
 *   Date: 2011/06/23
 *   Desc: tree operations
 *******************************************************************************
source code representation using abstract syntax tree
*******************************************************************************/
#include <string.h>
#include <math.h>
#include "core.h"

astn newnode(ccState cc, ccToken kind) {
	state rt = cc->s;
	astn ast = 0;
	if (cc->tokp) {
		ast = cc->tokp;
		cc->tokp = ast->next;
	}

	// allocate memory from temporary storage.
	rt->_end -= sizeof(struct astNode);
	if (rt->_beg >= rt->_end) {
		trace(ERR_MEMORY_OVERRUN);
		return NULL;
	}

	ast = (astn)rt->_end;
	memset(ast, 0, sizeof(struct astNode));
	ast->kind = kind;
	return ast;
}
void eatnode(ccState s, astn ast) {
	if (!ast) return;
	ast->next = s->tokp;
	s->tokp = ast;
}


astn opnode(ccState s, ccToken kind, astn lhs, astn rhs) {
	astn result = newnode(s, kind);
	if (result != NULL) {
		//~ TODO: dieif(tok_inf[kind].args == 0, "Erroro");
		result->op.lhso = lhs;
		result->op.rhso = rhs;
	}
	return result;
}

astn lnknode(ccState s, symn ref) {
	astn result = newnode(s, TYPE_ref);

	if (result != NULL) {
		result->type = ref->kind == TYPE_ref ? ref->type : ref;
		result->ref.name = ref->name;
		result->ref.link = ref;
		result->ref.hash = -1;
		result->cst2 = ref->cast;
	}
	return result;
}

/// make a constant valued node
astn intnode(ccState s, int64_t v) {
	astn ast = newnode(s, TYPE_int);
	if (ast != NULL) {
		ast->type = s->type_i32;
		ast->cint = v;
	}
	return ast;
}
astn fltnode(ccState s, float64_t v) {
	astn ast = newnode(s, TYPE_flt);
	if (ast != NULL) {
		ast->type = s->type_f64;
		ast->cflt = v;
	}
	return ast;
}
astn strnode(ccState s, char* v) {
	astn ast = newnode(s, TYPE_str);
	if (ast != NULL) {
		ast->type = s->type_str;
		ast->ref.hash = -1;
		ast->ref.name = v;
	}
	return ast;
}

int32_t constbol(astn ast) {
	if (ast) switch (ast->kind) {
		case TYPE_bit:
		case TYPE_int:
			return ast->cint != 0;
		case TYPE_flt:
			return ast->cflt != 0;
		default:
			break;
	}
	fatal("not a constant %+k", ast);
	return 0;
}
int64_t constint(astn ast) {
	if (ast) switch (ast->kind) {
		case TYPE_int:
			return (int64_t)ast->cint;
		case TYPE_flt:
			return (int64_t)ast->cflt;
		/*case TYPE_ref: {
			symn lnk = ast->ref.link;
			if (lnk && lnk->kind == TYPE_def) {
				return constint(lnk->init);
			}
		}*/
		default:
			break;
	}
	fatal("not a constant %+k", ast);
	return 0;
}
float64_t constflt(astn ast) {
	if (ast) switch (ast->kind) {
		case TYPE_int:
			return (float64_t)ast->cint;
		case TYPE_flt:
			return (float64_t)ast->cflt;
		default:
			break;
	}
	fatal("not a constant %+k", ast);
	return 0;
}


int isStatic(ccState cc, astn ast) {
	if (ast) switch (ast->kind) {
		default:
			return 0;

		//#{ OPER
		case OPER_fnc:		// '()' emit/call/cast
		case OPER_idx:		// '[]'
		case OPER_dot:		// '.'
			return 0;

		case OPER_com:
		case OPER_not:		// '!'
		case OPER_pls:		// '+'
		case OPER_mns:		// '-'
		case OPER_cmt:		// '~'
			return isStatic(cc, ast->op.rhso);

		case OPER_shl:		// '>>'
		case OPER_shr:		// '<<'
		case OPER_and:		// '&'
		case OPER_ior:		// '|'
		case OPER_xor:		// '^'

		case OPER_equ:		// '=='
		case OPER_neq:		// '!='
		case OPER_lte:		// '<'
		case OPER_leq:		// '<='
		case OPER_gte:		// '>'
		case OPER_geq:		// '>='

		case OPER_add:		// '+'
		case OPER_sub:		// '-'
		case OPER_mul:		// '*'
		case OPER_div:		// '/'
		case OPER_mod:		// '%'
			return isStatic(cc, ast->op.lhso) && isStatic(cc, ast->op.rhso);

		case OPER_lnd:		// '&&'
		case OPER_lor:		// '||'
			return isStatic(cc, ast->op.lhso) && isStatic(cc, ast->op.rhso);

		case OPER_sel:		// '?:'
			return isStatic(cc, ast->op.test) && isStatic(cc, ast->op.lhso) && isStatic(cc, ast->op.rhso);

		case ASGN_set:		// ':='
			return 0;
		//#}
		//#{ TVAL
		case TYPE_int:
		case TYPE_flt:
		case TYPE_str:
			return 1;

		case TYPE_ref: {					// use (var, func, define)
			symn typ = ast->type;			// type
			symn var = ast->ref.link;		// link
			if (typ == NULL || var == NULL) {
				fatal(ERR_INTERNAL_ERROR);
				return 0;
			}
			// TODO: global variables are not always static.
			return var->stat || var->nest > cc->nest;
		}

		//~ case TYPE_def:					// new (var, func, define)
		//~ case EMIT_opc:
		//#}
	}
	return 0;
}

int isConst(astn ast) {
	if (ast != NULL) {
		struct astNode tmp;

		while (ast->kind == OPER_com) {
			if (!isConst(ast->op.rhso)) {
				debug("%+k", ast);
				return 0;
			}
			ast = ast->op.lhso;
		}

		if (eval(&tmp, ast)) {
			return 1;
		}

		if (ast->kind == OPER_fnc) {
			// check if it is an initializer or a pure function
			symn ref = linkOf(ast->op.lhso);
			if (ref && !ref->cnst) {
				return 0;
			}

			// check if arguments are constants
			return isConst(ast->op.rhso);
		}

		else if (ast->kind == TYPE_ref) {
			symn ref = linkOf(ast);
			if (ref && ref->cnst) {
				return 1;
			}
		}

		// string constant is constant
		else if (ast->kind == TYPE_str) {
			return 1;
		}
	}

	debug("%+k", ast);
	return 0;
}

int isType(astn ast) {
	if (!ast) return 0;

	if (ast->kind == EMIT_opc) {
		return 0;
	}

	if (ast->kind == OPER_dot) {
		if (isType(ast->op.lhso)) {
			return isType(ast->op.rhso);
		}
		return 0;
	}

	if (ast->kind == TYPE_ref) {
		return istype(ast->ref.link);
	}

	//~ trace("%t(%+k):(%d)", ast->kind, ast, ast->line);
	return 0;
}

//~ TODO: eval should use cgen and vmExec
int eval(astn res, astn ast) {
	symn type = NULL;
	ccToken cast;
	struct astNode lhs, rhs;

	if (ast == NULL)
		return 0;

	if (!res)
		res = &rhs;

	type = ast->type;
	switch (ast->cst2) {
		default:
			trace("(%+k):%t(%s:%u)", ast, ast->cst2, ast->file, ast->line);
			return 0;

		case TYPE_bit:
			cast = TYPE_bit;
			break;

		//~ case TYPE_int:
		case TYPE_i32:
		case TYPE_u32:
		case TYPE_i64:
		//case TYPE_u64:
			cast = TYPE_int;
			break;

		//~ case TYPE_flt:
		case TYPE_f32:
		case TYPE_f64:
			cast = TYPE_flt;
			break;

		case TYPE_rec:
			switch (ast->kind) {
				default:
					return 0;

				case TYPE_int:
				case TYPE_flt:
				case TYPE_str:
					cast = ast->cst2;
					break;
			}
			break;
			//*/

		case TYPE_arr:
		case TYPE_ref:
		case TYPE_vid:
			return 0;

		case TYPE_any:
			cast = ast->cst2;
			break;
	}

	switch (ast->kind) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return 0;

		case OPER_com:
		case STMT_beg:
			return 0;

		case OPER_dot:
			if (!isType(ast->op.lhso))
				return 0;
			return eval(res, ast->op.rhso);

		case OPER_fnc: {
			astn func = ast->op.lhso;

			// evaluate only type casts.
			if (func && !isType(func))
				return 0;

			if (!eval(res, ast->op.rhso))
				return 0;

		} break;
		case OPER_adr:
		case OPER_idx:
			return 0;

		case TYPE_int:
		case TYPE_flt:
		case TYPE_str:
			*res = *ast;
			break;

		case TYPE_ref: {
			symn var = ast->ref.link;		// link
			if (var && var->kind == TYPE_def && var->init) {
				type = var->type;
				if (!eval(res, var->init))
					return 0;
			}
			else {
				return 0;
			}
		} break;

		case OPER_pls:		// '+'
			if (!eval(res, ast->op.rhso))
				return 0;
			break;

		case OPER_mns:		// '-'
			if (!eval(res, ast->op.rhso))
				return 0;

			switch (res->kind) {
				default:
					return 0;
				case TYPE_int:
					res->cint = -res->cint;
					break;
				case TYPE_flt:
					res->cflt = -res->cflt;
					break;
			}
			break;

		case OPER_cmt:			// '~'

			if (!eval(res, ast->op.rhso))
				return 0;

			switch (res->kind) {

				default:
					return 0;

				case TYPE_int:
					res->cint = ~res->cint;
					break;

				/*case TYPE_flt:
					res->cflt = 1. / res->cflt;
					break;
				//? */
			}
			break;

		case OPER_not:			// '!'

			if (!eval(res, ast->op.rhso))
				return 0;

			logif(ast->cst2 != TYPE_bit, "FixMe %+k", ast);

			switch (res->kind) {

				default:
					return 0;

				case TYPE_int:
					res->cint = !res->cint;
					res->kind = TYPE_int;
					break;

				case TYPE_flt:
					res->cint = !res->cflt;
					res->kind = TYPE_int;
					break;
			}
			break;

		case OPER_add:			// '+'
		case OPER_sub:			// '-'
		case OPER_mul:			// '*'
		case OPER_div:			// '/'
		case OPER_mod:			// '%'
			if (!eval(&lhs, ast->op.lhso))
				return 0;

			if (!eval(&rhs, ast->op.rhso))
				return 0;

			dieif(lhs.kind != rhs.kind, "eval operator %k (%t, %t): %t", ast, lhs.kind, rhs.kind, ast->cst2);

			switch (rhs.kind) {
				default:
					return 0;

				case TYPE_int:
					res->kind = TYPE_int;
					switch (ast->kind) {

						default:
							fatal(ERR_INTERNAL_ERROR);
							return 0;

						case OPER_add:
							res->cint = lhs.cint + rhs.cint;
							break;

						case OPER_sub:
							res->cint = lhs.cint - rhs.cint;
							break;

						case OPER_mul:
							res->cint = lhs.cint * rhs.cint;
							break;

						case OPER_div:
							if (rhs.cint == 0) {
								error(NULL, NULL, 0, "Division by zero: %+k", ast);
								res->cint = 0;
								break;
							}
							res->cint = lhs.cint / rhs.cint;
							break;

						case OPER_mod:
							if (rhs.cint == 0) {
								error(NULL, NULL, 0, "Division by zero: %+k", ast);
								res->cint = 0;
								break;
							}
							res->cint = lhs.cint % rhs.cint;
							break;
					}
					break;

				case TYPE_flt:
					res->kind = TYPE_flt;
					switch (ast->kind) {

						default:
							fatal(ERR_INTERNAL_ERROR);
							return 0;

						case OPER_add:
							res->cflt = lhs.cflt + rhs.cflt;
							break;

						case OPER_sub:
							res->cflt = lhs.cflt - rhs.cflt;
							break;

						case OPER_mul:
							res->cflt = lhs.cflt * rhs.cflt;
							break;

						case OPER_div:
							res->cflt = lhs.cflt / rhs.cflt;
							break;

						case OPER_mod:
							res->cflt = fmod(lhs.cflt, rhs.cflt);
							break;
					}
					break;
			}
			break;

		case OPER_neq:			// '!='
		case OPER_equ:			// '=='
		case OPER_gte:			// '>'
		case OPER_geq:			// '>='
		case OPER_lte:			// '<'
		case OPER_leq:			// '<='

			if (!eval(&lhs, ast->op.lhso))
				return 0;

			if (!eval(&rhs, ast->op.rhso))
				return 0;

			dieif(lhs.kind != rhs.kind, "eval operator %k (%t, %t): %t", ast, lhs.kind, rhs.kind, ast->cst2);

			res->kind = TYPE_bit;
			switch (rhs.kind) {
				default:
					return 0;

				case TYPE_int:
					switch (ast->kind) {

						default:
							fatal(ERR_INTERNAL_ERROR);
							return 0;

						case OPER_neq:
							res->cint = lhs.cint != rhs.cint;
							break;

						case OPER_equ:
							res->cint = lhs.cint == rhs.cint;
							break;

						case OPER_gte:
							res->cint = lhs.cint  > rhs.cint;
							break;

						case OPER_geq:
							res->cint = lhs.cint >= rhs.cint;
							break;

						case OPER_lte:
							res->cint = lhs.cint  < rhs.cint;
							break;

						case OPER_leq:
							res->cint = lhs.cint <= rhs.cint;
							break;
					}
					break;
				case TYPE_flt:
					switch (ast->kind) {

						default:
							fatal(ERR_INTERNAL_ERROR);
							return 0;

						case OPER_neq:
							res->cint = lhs.cflt != rhs.cflt;
							break;

						case OPER_equ:
							res->cint = lhs.cflt == rhs.cflt;
							break;

						case OPER_gte:
							res->cint = lhs.cflt > rhs.cflt;
							break;

						case OPER_geq:
							res->cint = lhs.cflt >= rhs.cflt;
							break;

						case OPER_lte:
							res->cint = lhs.cflt < rhs.cflt;
							break;

						case OPER_leq:
							res->cint = lhs.cflt <= rhs.cflt;
							break;
					}
					break;
			}
			break;

		case OPER_shr:			// '>>'
		case OPER_shl:			// '<<'
		case OPER_and:			// '&'
		case OPER_ior:			// '|'
		case OPER_xor:			// '^'

			if (!eval(&lhs, ast->op.lhso))
				return 0;

			if (!eval(&rhs, ast->op.rhso)) {
				return 0;
			}

			dieif(lhs.kind != rhs.kind, "eval operator %k (%t, %t): %t", ast, lhs.kind, rhs.kind, ast->cst2);

			switch (rhs.kind) {

				default:
					trace("eval(%+k) : %t", ast->op.rhso, rhs.kind);
					return 0;

				case TYPE_int:
					res->kind = TYPE_int;
					switch (ast->kind) {

						default:
							fatal(ERR_INTERNAL_ERROR);
							return 0;

						case OPER_shr:
							res->cint = lhs.cint >> rhs.cint;
							break;

						case OPER_shl:
							res->cint = lhs.cint << rhs.cint;
							break;

						case OPER_and:
							res->cint = lhs.cint & rhs.cint;
							break;

						case OPER_xor:
							res->cint = lhs.cint ^ rhs.cint;
							break;

						case OPER_ior:
							res->cint = lhs.cint | rhs.cint;
							break;
					}
					break;
			}
			break;

		case OPER_lnd:
		case OPER_lor:

			if (!eval(&lhs, ast->op.lhso))
				return 0;

			if (!eval(&rhs, ast->op.rhso))
				return 0;

			dieif(lhs.kind != rhs.kind, "eval operator %k (%t, %t): %t", ast, lhs.kind, rhs.kind, ast->cst2);

			res->kind = TYPE_bit;
			switch (ast->kind) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					return 0;

				case OPER_lor:
					res->cint = lhs.cint || rhs.cint;
					break;

				case OPER_lnd:
					res->cint = lhs.cint && rhs.cint;
					break;
			}
			break;

		case OPER_sel:
			if (!eval(&lhs, ast->op.test))
				return 0;

			return eval(res, lhs.cint ? ast->op.lhso : ast->op.rhso);

		case ASGN_set:
		//~ case ASGN_add:
		//~ case ASGN_sub:
		//~ case ASGN_mul:
		//~ case ASGN_div:
		//~ case ASGN_mod:
		//~ case ASGN_shl:
		//~ case ASGN_shr:
		//~ case ASGN_and:
		//~ case ASGN_ior:
		//~ case ASGN_xor:
		case EMIT_opc:
			return 0;
	}

	if (cast != res->kind) switch (cast) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return 0;

		case TYPE_vid:
		case TYPE_any:
		case TYPE_rec:
			break;

		case TYPE_bit:
			res->cint = constbol(res);
			res->kind = TYPE_int;
			break;

		case TYPE_int:
			res->cint = constint(res);
			res->kind = TYPE_int;
			break;

		case TYPE_flt:
			res->cflt = constflt(res);
			res->kind = TYPE_flt;
			break;
	}

	switch (res->kind) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			res->type = NULL;
			return 0;

		case TYPE_bit:
		case TYPE_int:
		case TYPE_flt:
		case TYPE_str:
			res->type = type;
			break;
	}

	return res->kind;
}

symn linkOf(astn ast) {
	if (!ast) return 0;

	if (ast->kind == EMIT_opc) {
		return ast->type;
	}

	if (ast->kind == OPER_fnc) {
		return linkOf(ast->op.lhso);
	}

	if (ast->kind == OPER_dot) {
		return linkOf(ast->op.rhso);
	}

	if (ast->kind == OPER_idx) {
		return linkOf(ast->op.lhso);
	}

	if ((ast->kind == TYPE_ref || ast->kind == TYPE_def) && ast->ref.link) {
		// skip type defs
		symn lnk = ast->ref.link;
		if (lnk->kind == TYPE_def && lnk->init != NULL && lnk->init->kind == TYPE_ref) {
			lnk = linkOf(lnk->init);
		}
		return lnk;
	}

	//~ trace("%t(%+k)", ast->kind, ast);
	return NULL;
}
