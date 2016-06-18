/*******************************************************************************
 *   File: tree.c
 *   Date: 2011/06/23
 *   Desc: tree operations
 *******************************************************************************
source code representation using abstract syntax tree
*******************************************************************************/
#include <string.h>
#include <math.h>
#include "internal.h"

symn newdefn(ccContext cc, ccToken kind) {
	rtContext rt = cc->rt;
	symn def = NULL;

	rt->_beg = paddptr(rt->_beg, 8);
	if(rt->_beg >= rt->_end) {
		fatal(ERR_INTERNAL_ERROR);
		return NULL;
	}

	def = (symn)rt->_beg;
	rt->_beg += sizeof(struct symNode);
	memset(def, 0, sizeof(struct symNode));
	def->kind = kind;

	return def;
}

astn newnode(ccContext cc, ccToken kind) {
	rtContext rt = cc->rt;
	astn ast = 0;
	if (cc->tokp) {
		ast = cc->tokp;
		cc->tokp = ast->next;
	}

	// allocate memory from temporary storage.
	rt->_end -= sizeof(struct astNode);
	if (rt->_beg >= rt->_end) {
		fatal(ERR_MEMORY_OVERRUN);
		return NULL;
	}

	ast = (astn)rt->_end;
	memset(ast, 0, sizeof(struct astNode));
	ast->kind = kind;
	return ast;
}

void eatnode(ccContext cc, astn ast) {
	if (!ast) return;
	ast->next = cc->tokp;
	cc->tokp = ast;
}

/// make a constant valued node
astn intnode(ccContext cc, int64_t v) {
	astn ast = newnode(cc, TYPE_int);
	if (ast != NULL) {
		ast->type = cc->type_i32;
		ast->cint = v;
	}
	return ast;
}
astn fltnode(ccContext cc, float64_t v) {
	astn ast = newnode(cc, TYPE_flt);
	if (ast != NULL) {
		ast->type = cc->type_f64;
		ast->cflt = v;
	}
	return ast;
}
astn strnode(ccContext cc, char *v) {
	astn ast = newnode(cc, TYPE_str);
	if (ast != NULL) {
		ast->type = cc->type_str;
		ast->ref.hash = -1;
		ast->ref.name = v;
	}
	return ast;
}
astn lnknode(ccContext cc, symn ref) {
	astn result = newnode(cc, CAST_ref);
	if (result != NULL) {
		result->type = ref->kind == CAST_ref ? ref->type : ref;
		result->ref.name = ref->name;
		result->ref.link = ref;
		result->ref.hash = -1;
		result->cast = ref->cast;
	}
	return result;
}
astn opnode(ccContext cc, ccToken kind, astn lhs, astn rhs) {
	astn result = newnode(cc, kind);
	if (result != NULL) {
		//~ TODO: dieif(tok_inf[kind].args == 0, "Erroro");
		result->op.lhso = lhs;
		result->op.rhso = rhs;
	}
	return result;
}

astn wrapStmt(ccContext cc, astn node) {
	astn result = newnode(cc, STMT_end);
	if (result != NULL) {
		result->type = cc->type_vid;
		result->cast = TYPE_any;
		result->file = node->file;
		result->line = node->line;
		result->stmt.stmt = node;
	}
	return result;
}
/// get value of constant node
int32_t constbol(astn ast) {
	if (ast != NULL) {
		switch (ast->kind) {
			case CAST_bit:
			case TYPE_int:
				return ast->cint != 0;
			case TYPE_flt:
				return ast->cflt != 0;
			default:
				break;
		}
	}
	fatal("not a constant %+t", ast);
	return 0;
}
int64_t constint(astn ast) {
	if (ast != NULL) {
		switch (ast->kind) {
			case TYPE_int:
				return (int64_t)ast->cint;
			case TYPE_flt:
				return (int64_t)ast->cflt;
				/*case CAST_ref: {
					symn lnk = ast->ref.link;
					if (lnk && lnk->kind == TYPE_def) {
						return constint(lnk->init);
					}
				}*/
			default:
				break;
		}
	}
	fatal("not a constant %+t", ast);
	return 0;
}
float64_t constflt(astn ast) {
	if (ast) {
		switch (ast->kind) {
			case TYPE_int:
				return (float64_t)ast->cint;
			case TYPE_flt:
				return (float64_t)ast->cflt;
			default:
				break;
		}
	}
	fatal("not a constant %+t", ast);
	return 0;
}
ccKind eval(astn res, astn ast) {
	symn type = NULL;
	ccKind cast;
	struct astNode lhs, rhs;

	if (ast == NULL)
		return TYPE_any;

	if (!res)
		res = &rhs;

	type = ast->type;
	switch (ast->cast) {
		default:
			trace("(%+t):%K(%s:%u)", ast, ast->cast, ast->file, ast->line);
			return TYPE_any;

		case CAST_bit:
			cast = CAST_bit;
			break;

			//~ case TYPE_int:
		case CAST_i32:
		case CAST_u32:
		case CAST_i64:
			//case TYPE_u64:
			cast = TYPE_int;
			break;

			//~ case TYPE_flt:
		case CAST_f32:
		case CAST_f64:
			cast = TYPE_flt;
			break;

		case TYPE_rec:
			switch (ast->kind) {
				default:
					return TYPE_any;

				case TYPE_int:
				case TYPE_flt:
				case TYPE_str:
					cast = ast->cast;
					break;
			}
			break;
			//*/

		case CAST_arr:
		case CAST_ref:
		case CAST_vid:
			return TYPE_any;

		case TYPE_any:
			cast = ast->cast;
			break;
	}

	switch (ast->kind) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return TYPE_any;

		case OPER_com:
		case STMT_beg:
			return TYPE_any;

		case OPER_dot:
			if (!isTypeExpr(ast->op.lhso))
				return TYPE_any;
			return eval(res, ast->op.rhso);

		case OPER_fnc: {
			astn func = ast->op.lhso;

			// evaluate only type casts.
			if (func && !isTypeExpr(func))
				return TYPE_any;

			if (!eval(res, ast->op.rhso))
				return TYPE_any;

		} break;
		case OPER_adr:
		case OPER_idx:
			return TYPE_any;

		case TYPE_int:
		case TYPE_flt:
		case TYPE_str:
			*res = *ast;
			break;

		case CAST_ref: {
			symn var = ast->ref.link;		// link
			if (var && var->kind == TYPE_def && var->init) {
				type = var->type;
				if (!eval(res, var->init))
					return TYPE_any;
			}
			else {
				return TYPE_any;
			}
		} break;

		case OPER_pls:		// '+'
			if (!eval(res, ast->op.rhso))
				return TYPE_any;
			break;

		case OPER_mns:		// '-'
			if (!eval(res, ast->op.rhso))
				return TYPE_any;

			switch (res->kind) {
				default:
					return TYPE_any;
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
				return TYPE_any;

			switch (res->kind) {

				default:
					return TYPE_any;

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
				return TYPE_any;

			dieif(ast->cast != CAST_bit, "FixMe %+t", ast);

			switch (res->kind) {

				default:
					return TYPE_any;

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
				return TYPE_any;

			if (!eval(&rhs, ast->op.rhso))
				return TYPE_any;

			dieif(lhs.kind != rhs.kind, "eval operator %t (%K, %K): %K", ast, lhs.kind, rhs.kind, ast->cast);

			switch (rhs.kind) {
				default:
					return TYPE_any;

				case TYPE_int:
					res->kind = TYPE_int;
					switch (ast->kind) {

						default:
							fatal(ERR_INTERNAL_ERROR);
							return TYPE_any;

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
								error(NULL, NULL, 0, "Division by zero: %+t", ast);
								res->cint = 0;
								break;
							}
							res->cint = lhs.cint / rhs.cint;
							break;

						case OPER_mod:
							if (rhs.cint == 0) {
								error(NULL, NULL, 0, "Division by zero: %+t", ast);
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
				return TYPE_any;

			if (!eval(&rhs, ast->op.rhso))
				return TYPE_any;

			dieif(lhs.kind != rhs.kind, "eval operator %t (%K, %K): %K", ast, lhs.kind, rhs.kind, ast->cast);

			res->kind = CAST_bit;
			switch (rhs.kind) {
				default:
					return TYPE_any;

				case TYPE_int:
					switch (ast->kind) {

						default:
							fatal(ERR_INTERNAL_ERROR);
							return TYPE_any;

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
							return TYPE_any;

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
				return TYPE_any;

			if (!eval(&rhs, ast->op.rhso)) {
				return TYPE_any;
			}

			dieif(lhs.kind != rhs.kind, "eval operator %t (%K, %K): %K", ast, lhs.kind, rhs.kind, ast->cast);

			switch (rhs.kind) {

				default:
					trace("eval(%+t) : %K", ast->op.rhso, rhs.kind);
					return TYPE_any;

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

		case OPER_all:
		case OPER_any:

			if (!eval(&lhs, ast->op.lhso))
				return TYPE_any;

			if (!eval(&rhs, ast->op.rhso))
				return TYPE_any;

			dieif(lhs.kind != rhs.kind, "eval operator %t (%K, %K): %K", ast, lhs.kind, rhs.kind, ast->cast);

			res->kind = CAST_bit;
			switch (ast->kind) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					return 0;

				case OPER_any:
					res->cint = lhs.cint || rhs.cint;
					break;

				case OPER_all:
					res->cint = lhs.cint && rhs.cint;
					break;
			}
			break;

		case OPER_sel:
			if (!eval(&lhs, ast->op.test))
				return TYPE_any;

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
		case EMIT_kwd:
			return TYPE_any;
	}

	if (cast != res->kind) switch (cast) {
			default:
				fatal(ERR_INTERNAL_ERROR);
				return 0;

			case CAST_vid:
			case TYPE_any:
			case TYPE_rec:
				break;

			case CAST_bit:
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
			return TYPE_any;

		case CAST_bit:
		case TYPE_int:
		case TYPE_flt:
		case TYPE_str:
			res->type = type;
			break;
	}

	return res->kind;
}

symn linkOf(astn ast) {
	if (ast == NULL) {
		return NULL;
	}

	if (ast->kind == EMIT_kwd) {
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

	if ((ast->kind == CAST_ref || ast->kind == TYPE_def) && ast->ref.link) {
		// skip type defs
		symn lnk = ast->ref.link;
		if (lnk->kind == TYPE_def && lnk->init != NULL && lnk->init->kind == CAST_ref) {
			lnk = linkOf(lnk->init);
		}
		return lnk;
	}

	//~ trace("%K(%+t)", ast->kind, ast);
	return NULL;
}

int isType(symn sym) {
	if (sym == NULL) {
		return 0;
	}

	switch (sym->kind) {
		default:
			break;

		case CAST_arr:
		case TYPE_rec:
			return sym->kind;

		case TYPE_def:
			if (sym->init == NULL) {
				return isType(sym->type);
			}
			break;
	}
	//~ trace("%T is not a type", sym);
	return 0;
}

int isTypeExpr(astn ast) {
	if (ast == NULL) {
		return 0;
	}

	if (ast->kind == EMIT_kwd) {
		return 0;
	}

	if (ast->kind == OPER_dot) {
		if (isTypeExpr(ast->op.lhso)) {
			return isTypeExpr(ast->op.rhso);
		}
		return 0;
	}

	if (ast->kind == CAST_ref) {
		return isType(ast->ref.link);
	}

	//~ trace("%K(%+t):(%d)", ast->kind, ast, ast->line);
	return 0;
}

int isConstExpr(astn ast) {
	struct astNode tmp;
	if (ast == NULL) {
		return 0;
	}

	// array initializer or function arguments
	while (ast->kind == OPER_com) {
		if (!isConstExpr(ast->op.rhso)) {
			debug("%+t", ast);
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
		return isConstExpr(ast->op.rhso);
	}

	else if (ast->kind == CAST_ref) {
		symn ref = linkOf(ast);
		if (ref && ref->cnst) {
			return 1;
		}
	}

		// string constant is constant
	else if (ast->kind == TYPE_str) {
		return 1;
	}

	traceAst(ast);
	return 0;
}

int isStaticExpr(ccContext cc, astn ast) {
	if (ast == NULL) {
		return 0;
	}
	switch (ast->kind) {
		default:
			break;

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
			return isStaticExpr(cc, ast->op.rhso);

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
			return isStaticExpr(cc, ast->op.lhso) && isStaticExpr(cc, ast->op.rhso);

		case OPER_all:		// '&&'
		case OPER_any:		// '||'
			return isStaticExpr(cc, ast->op.lhso) && isStaticExpr(cc, ast->op.rhso);

		case OPER_sel:		// '?:'
			return isStaticExpr(cc, ast->op.test) && isStaticExpr(cc, ast->op.lhso) && isStaticExpr(cc, ast->op.rhso);

		case ASGN_set:		// ':='
			return 0;
		//#}
		//#{ TVAL
		case TYPE_int:
		case TYPE_flt:
		case TYPE_str:
			return 1;

		case CAST_ref: {					// use (var, func, define)
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
		//~ case EMIT_kwd:
		//#}
	}
	traceAst(ast);
	return 0;
}

ccToken castOf(symn typ) {
	if (typ == NULL) {
		return TYPE_any;
	}
	switch (typ->kind) {

		default:
			break;

		case TYPE_def:
			return castOf(typ->type);

			//~ case CAST_vid:
			//~ return typ->kind;

		case CAST_arr:
			// static sized arrays cast to pointer
			if (typ->init == NULL)
				return CAST_arr;
			return CAST_ref;

		case EMIT_kwd:
			return typ->cast;

		case TYPE_rec:
			// refFix
			if (typ->cast == CAST_ref)
				return TYPE_ptr;
			return typ->cast;
	}
	debug("failed(%K): %?-T", typ ? typ->kind : 0, typ);
	return TYPE_any;
}

ccToken castTo(astn ast, ccToken cto) {
	ccToken atc = TYPE_any;
	if (!ast) {
		return TYPE_any;
	}

	atc = ast->type ? ast->type->cast : TYPE_any;
	if (cto != atc) switch (cto) {
		case TYPE_any:
			return atc;

		case CAST_vid:		// void(true): can cast 2 to void !!!
		case CAST_bit:
		case CAST_u32:
		case CAST_i32:
		case CAST_i64:
		case CAST_f32:
		case CAST_f64: switch (atc) {
			//~ case CAST_vid:
			case CAST_bit:
			case CAST_u32:
			case CAST_i32:
			case CAST_i64:
			case CAST_f32:
			case CAST_f64:
				break;

			default:
				goto error;
		} break;

		case CAST_ref:
		case CAST_arr:
			break;

		default:
		error:
			debug("cast(%+t) to %K/%K", ast, cto, atc);
			//~ return 0;
			break;
	}
	return ast->cast = cto;
}

size_t sizeOf(symn sym, int varSize) {
	if (sym != NULL) {
		switch (sym->kind) {
			default:
				break;
				//~ case CAST_vid:
				//~ case CAST_bit:
				//~ case TYPE_int:
				//~ case TYPE_flt:

			case EMIT_kwd:
				if (sym->cast == CAST_ref) {
					return vm_size;
				}
				return (size_t) sym->size;

			case TYPE_rec:
			case CAST_arr: switch (sym->cast) {
					case CAST_ref: if (varSize) {
							return vm_size;
						}
					case CAST_arr: if (varSize) {
							return 2 * vm_size;
						}
					default:
						return sym->size;
				}

			case TYPE_def:
			case CAST_ref: switch (sym->cast) {
					case CAST_ref:
						return vm_size;
					case CAST_arr:
						return 2 * vm_size;
					default:
						return sizeOf(sym->type, 0);
				}
		}
	}
	fatal("failed(%K): %-T", sym ? sym->kind : 0, sym);
	return 0;
}

