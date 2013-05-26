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

astn newnode(ccState s, int kind) {
	state rt = s->s;
	astn ast = 0;
	if (s->tokp) {
		ast = s->tokp;
		s->tokp = ast->next;
	}
	else if (rt->_end - rt->_beg > (int)sizeof(struct astn)){
		//~ ast = (astn)rt->_beg;
		//~ rt->_beg += sizeof(struct astn);
		rt->_end -= sizeof(struct astn);
		ast = (astn)rt->_end;
	}
	else {
		fatal("memory overrun");
	}
	memset(ast, 0, sizeof(struct astn));
	ast->kind = kind;
	return ast;
}

astn tagnode(ccState s, char* id) {
	astn ast = NULL;
	if (s && id) {
		ast = newnode(s, TYPE_ref);
		if (ast != NULL) {
			int slen = strlen(id);
			ast->kind = TYPE_ref;
			ast->type = ast->ref.link = 0;
			ast->ref.hash = rehash(id, slen + 1) % TBLS;
			ast->ref.name = mapstr(s, id, slen + 1, ast->ref.hash);
		}
	}
	return ast;
}

astn opnode(ccState s, int kind, astn lhs, astn rhs) {
	astn result = newnode(s, kind);
	if (result) {
		result->op.lhso = lhs;
		result->op.rhso = rhs;
	}
	return result;
}

/// make a node witch si a link to a reference
astn lnknode(ccState s, symn ref) {
	astn result = newnode(s, TYPE_ref);
	if (result) {
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
	ast->type = s->type_i32;
	ast->con.cint = v;
	return ast;
}
astn fltnode(ccState s, float64_t v) {
	astn ast = newnode(s, TYPE_flt);
	ast->type = s->type_f64;
	ast->con.cflt = v;
	return ast;
}
astn strnode(ccState s, char* v) {
	astn ast = newnode(s, TYPE_str);
	ast->ref.hash = -1;
	ast->ref.name = v;
	return ast;
}

void eatnode(ccState s, astn ast) {
	if (!ast) return;
	ast->next = s->tokp;
	s->tokp = ast;
}

int32_t constbol(astn ast) {
	if (ast) switch (ast->kind) {
		case TYPE_bit:
		case TYPE_int:
			return ast->con.cint != 0;
		case TYPE_flt:
			return ast->con.cflt != 0;
		default:
			break;
	}
	fatal("not a constant %+k", ast);
	return 0;
}
int64_t constint(astn ast) {
	if (ast) switch (ast->kind) {
		case TYPE_int:
			return (int64_t)ast->con.cint;
		case TYPE_flt:
			return (int64_t)ast->con.cflt;
		//~ case TYPE_ref: {
			//~ symn lnk = ast->ref.link;
			//~ if (lnk && lnk->kind == TYPE_def) {
				//~ return constint(lnk->init);
			//~ }
		//~ }
		default:
			break;
	}
	fatal("not a constant %+k", ast);
	return 0;
}
float64_t constflt(astn ast) {
	if (ast) switch (ast->kind) {
		case TYPE_int:
			return (float64_t)ast->con.cint;
		case TYPE_flt:
			return (float64_t)ast->con.cflt;
		default:
			break;
	}
	fatal("not a constant %+k", ast);
	return 0;
}

//~ TODO: eval should use cgen and vmExec
int eval(astn res, astn ast) {
	symn type = NULL;
	ccToken cast = TYPE_any;
	struct astn lhs, rhs;

	if (!ast)
		return 0;

	if (!res)
		res = &rhs;

	type = ast->type;
	switch (ast->cst2) {
		case TYPE_bit: cast = TYPE_bit; break;

		//~ case TYPE_int:
		case TYPE_i32: cast = TYPE_int; break;
		case TYPE_u32: cast = TYPE_int; break;
		case TYPE_i64: cast = TYPE_int; break;
		//~ case TYPE_u64: cast = TYPE_int; break;

		//~ case TYPE_flt:
		case TYPE_f32: cast = TYPE_flt; break;
		case TYPE_f64: cast = TYPE_flt; break;

		default:
			debug("(%+k):%t / %d", ast, ast->cst2, ast->line);

		case TYPE_arr:
		case TYPE_rec:
		case TYPE_ref:
		case TYPE_vid:
			return 0;
		case 0:
		//~ case TYPE_def:
			//~ return 0;
			cast = ast->cst2;
	}

	switch (ast->kind) {
		default:
			//~ fatal("FixMe %t: %+k", ast->kind, ast);
			debug("FixMe %+k", ast);
			return 0;

		case STMT_beg:
			return 0;

		case OPER_dot: {
			if (!isType(ast->op.lhso)) return 0;
			return eval(res, ast->op.rhso);
		} break;
		case OPER_fnc: {
			astn lhs = ast->op.lhso;

			if (lhs && !isType(lhs))
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
			else
				return 0;
		} break;

		case OPER_pls: {		// '+'
			if (!eval(res, ast->op.rhso))
				return 0;
		} break;
		case OPER_mns: {		// '-'
			if (!eval(res, ast->op.rhso))
				return 0;
			switch (res->kind) {
				default: return 0;
				case TYPE_int: res->con.cint = -res->con.cint; break;
				case TYPE_flt: res->con.cflt = -res->con.cflt; break;
			}
		} break;
		case OPER_cmt: {		// '~'
			if (!eval(res, ast->op.rhso))
				return 0;
			switch (res->kind) {
				default: return 0;
				case TYPE_int: res->con.cint = ~res->con.cint; break;
				//~ case TYPE_flt: res->con.cflt = 1. / res->con.cflt; break;
			}
		} break;

		case OPER_not: {		// '!'
			logif(ast->cst2 != TYPE_bit, "FixMe %+k", ast);
			if (!eval(res, ast->op.rhso))
				return 0;

			switch (res->kind) {
				default: return 0;
				case TYPE_int: res->kind = TYPE_int; res->con.cint = !res->con.cint; break;
				case TYPE_flt: res->kind = TYPE_int; res->con.cint = !res->con.cflt; break;
			}
		} break;

		//~ / *
		case OPER_add:			// '+'
		case OPER_sub:			// '-'
		case OPER_mul:			// '*'
		case OPER_div:			// '/'
		case OPER_mod: {		// '%'
			if (!eval(&lhs, ast->op.lhso))
				return 0;
			if (!eval(&rhs, ast->op.rhso))
				return 0;
			dieif(lhs.kind != rhs.kind, "eval operator %k (%t, %t): %t", ast, lhs.kind, rhs.kind, ast->cst2);

			switch (rhs.kind) {
				default: return 0;
				case TYPE_int: {
					res->kind = TYPE_int;
					switch (ast->kind) {
						default: fatal("FixMe");
						case OPER_add: res->con.cint = lhs.con.cint + rhs.con.cint; break;		// '+'
						case OPER_sub: res->con.cint = lhs.con.cint - rhs.con.cint; break;		// '-'
						case OPER_mul: res->con.cint = lhs.con.cint * rhs.con.cint; break;		// '*'
						case OPER_div: 
						case OPER_mod: {
							if (rhs.con.cint) switch (ast->kind) {
								default: fatal("FixMe");
								case OPER_div: res->con.cint = lhs.con.cint / rhs.con.cint; break;		// '/'
								case OPER_mod: res->con.cint = lhs.con.cint % rhs.con.cint; break;		// '%'
							}
							else {
								error(NULL, NULL, 0, "Division by zero: %+k", ast);
								res->con.cint = 0;
							}
						}
					}
				} break;
				case TYPE_flt: {
					res->kind = TYPE_flt;
					switch (ast->kind) {
						default: fatal("FixMe: %+k", ast);
						case OPER_add: res->con.cflt = lhs.con.cflt + rhs.con.cflt; break;		// '+'
						case OPER_sub: res->con.cflt = lhs.con.cflt - rhs.con.cflt; break;		// '-'
						case OPER_mul: res->con.cflt = lhs.con.cflt * rhs.con.cflt; break;		// '*'
						case OPER_div: res->con.cflt = lhs.con.cflt / rhs.con.cflt; break;		// '/'
						case OPER_mod: res->con.cflt = fmod(lhs.con.cflt, rhs.con.cflt); break;		// '%'
					}
				} break;
			}
		} break;

		case OPER_neq:			// '!='
		case OPER_equ:			// '=='
		case OPER_gte:			// '>'
		case OPER_geq:			// '>='
		case OPER_lte:			// '<'
		case OPER_leq: {		// '<='
			if (!eval(&lhs, ast->op.lhso))
				return 0;
			if (!eval(&rhs, ast->op.rhso))
				return 0;
			dieif(lhs.kind != rhs.kind, "eval operator %k (%t, %t): %t", ast, lhs.kind, rhs.kind, ast->cst2);
			res->kind = TYPE_bit;
			switch (rhs.kind) {
				default: return 0;
				case TYPE_int: switch (ast->kind) {
					default: fatal("FixMe");
					case OPER_neq: res->con.cint = lhs.con.cint != rhs.con.cint; break;
					case OPER_equ: res->con.cint = lhs.con.cint == rhs.con.cint; break;
					case OPER_gte: res->con.cint = lhs.con.cint  > rhs.con.cint; break;
					case OPER_geq: res->con.cint = lhs.con.cint >= rhs.con.cint; break;
					case OPER_lte: res->con.cint = lhs.con.cint  < rhs.con.cint; break;
					case OPER_leq: res->con.cint = lhs.con.cint <= rhs.con.cint; break;
				} break;
				case TYPE_flt: switch (ast->kind) {
					default: fatal("FixMe");
					case OPER_neq: res->con.cint = lhs.con.cflt != rhs.con.cflt; break;
					case OPER_equ: res->con.cint = lhs.con.cflt == rhs.con.cflt; break;
					case OPER_gte: res->con.cint = lhs.con.cflt  > rhs.con.cflt; break;
					case OPER_geq: res->con.cint = lhs.con.cflt >= rhs.con.cflt; break;
					case OPER_lte: res->con.cint = lhs.con.cflt  < rhs.con.cflt; break;
					case OPER_leq: res->con.cint = lhs.con.cflt <= rhs.con.cflt; break;
				} break;
			}
		} break;

		case OPER_shr:			// '>>'
		case OPER_shl:			// '<<'
		case OPER_and:			// '&'
		case OPER_ior:			// '|'
		case OPER_xor: {		// '^'
			if (!eval(&lhs, ast->op.lhso))
				return 0;
			if (!eval(&rhs, ast->op.rhso))
				return 0;
			dieif(lhs.kind != rhs.kind, "eval operator %k (%t, %t): %t", ast, lhs.kind, rhs.kind, ast->cst2);

			switch (rhs.kind) {
				default:
					debug("eval(%+k) : %t", ast->op.rhso, rhs.kind);
					return 0;
				case TYPE_int: {
					res->kind = TYPE_int;
					switch (ast->kind) {
						default: fatal("FixMe");
						case OPER_shr: res->con.cint = lhs.con.cint >> rhs.con.cint; break;
						case OPER_shl: res->con.cint = lhs.con.cint << rhs.con.cint; break;
						case OPER_and: res->con.cint = lhs.con.cint  & rhs.con.cint; break;
						case OPER_xor: res->con.cint = lhs.con.cint  ^ rhs.con.cint; break;
						case OPER_ior: res->con.cint = lhs.con.cint  | rhs.con.cint; break;
					}
				} break;
			}
		} break;

		case OPER_lnd:
		case OPER_lor: {
			if (!eval(&lhs, ast->op.lhso))
				return 0;
			if (!eval(&rhs, ast->op.rhso))
				return 0;
			dieif(lhs.kind != rhs.kind, "eval operator %k (%t, %t): %t", ast, lhs.kind, rhs.kind, ast->cst2);

			res->kind = TYPE_bit;
			switch (ast->kind) {
				default: fatal("FixMe");
				case OPER_lor: res->con.cint = lhs.con.cint || rhs.con.cint; break;
				case OPER_lnd: res->con.cint = lhs.con.cint && rhs.con.cint; break;
			}
		} break;
		case OPER_sel: {
			if (!eval(&lhs, ast->op.test))
				return 0;
			return eval(res, lhs.con.cint ? ast->op.lhso : ast->op.rhso);
		} break;

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
		default: fatal("FixMe");

		case TYPE_vid:
		case TYPE_any:
			break;

		case TYPE_bit:
			res->con.cint = constbol(res);
			res->kind = TYPE_int;
			break;

		case TYPE_int:
			res->con.cint = constint(res);
			res->kind = TYPE_int;
			break;

		case TYPE_flt:
			res->con.cflt = constflt(res);
			res->kind = TYPE_flt;
			break;
	}

	switch (res->kind) {
		default:
			fatal("FixMe %t", res->kind);
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
