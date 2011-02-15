//~ tree.c - tree related stuff ------------------------------------------------
#include <string.h>
#include <math.h>
#include "ccvm.h"

astn newnode(ccState s, int kind) {
	astn ast = 0;
	if (s->tokp) {
		ast = s->tokp;
		s->tokp = ast->next;
	}
	else if (s->_end - s->_beg > (int)sizeof(struct astn)){
		//~ ast = (astn)s->_beg;
		//~ s->_beg += sizeof(struct astn);
		s->_end -= sizeof(struct astn);
		ast = (astn)s->_end;
	}
	else {
		fatal("memory overrun");
	}
	memset(ast, 0, sizeof(struct astn));
	ast->kind = kind;
	return ast;
}

/*
astn fltnode(ccState s, float64_t v) {
	astn ast = newnode(s, TYPE_flt);
	//~ ast->type = type_f64;
	ast->con.cflt = v;
	return ast;
}

astn strnode(ccState s, char * v) {
	astn ast = newnode(s, TYPE_str);
	ast->id.hash = -1;
	ast->id.name = v;
	return ast;
}

astn cpynode(ccState s, astn src) {
	astn ast = newnode(s, 0);
	*ast = *src;
	return ast;
}
// */

astn intnode(ccState s, int64_t v) {
	astn ast = newnode(s, TYPE_int);
	ast->type = type_i32;
	ast->con.cint = v;
	return ast;
}
void eatnode(ccState s, astn ast) {
	if (!ast) return;
	ast->next = s->tokp;
	s->tokp = ast;
}

signed constbol(astn ast) {
	if (ast) switch (ast->kind) {
		case TYPE_bit:
		case TYPE_int: return ast->con.cint != 0;
		case TYPE_flt: return ast->con.cflt != 0;
	}
	fatal("not a constant %+k", ast);
	return 0;
}
int64_t constint(astn ast) {
	if (ast) switch (ast->kind) {
		case TYPE_int: return (int64_t)ast->con.cint;
		case TYPE_flt: return (int64_t)ast->con.cflt;
	}
	fatal("not a constant %+k", ast);
	return 0;
}
float64_t constflt(astn ast) {
	if (ast) switch (ast->kind) {
		case TYPE_int: return (float64_t)ast->con.cint;
		case TYPE_flt: return (float64_t)ast->con.cflt;
	}
	fatal("not a constant %+k", ast);
	return 0;
}

// TODO: use cgen and exec;
int eval(astn res, astn ast) {
	struct astn lhs, rhs;
	int cast;

	if (!ast)
		return 0;

	if (!res)
		res = &rhs;

	switch (ast->cst2) {
		case TYPE_bit: cast = TYPE_bit; break;

		case TYPE_int: // TODO: define a = 9;	// what casts to int ? index ?
		case TYPE_u32: cast = TYPE_int; break;
		case TYPE_i32: cast = TYPE_int; break;

		//~ case TYPE_u64: cast = TYPE_int; break;
		case TYPE_i64: cast = TYPE_int; break;

		case TYPE_f32: cast = TYPE_flt; break;
		case TYPE_f64: cast = TYPE_flt; break;

		//~ case TYPE_arr:
		case TYPE_rec:
			return 0;

		default:
			debug("(%+k):%t / %d", ast, ast->cst2, ast->line);

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

		case OPER_dot: {
			if (!istype(ast->op.lhso)) return 0;
			return eval(res, ast->op.rhso);
		} break;
		case OPER_fnc: {
			astn lhs = ast->op.lhso;

			if (lhs && !istype(lhs))
				return 0;

			if (!eval(res, ast->op.rhso))
				return 0;

		} break;
		case OPER_idx:
			return 0;

		case TYPE_int:
		case TYPE_flt:
		case TYPE_str:
			*res = *ast;
			break;

		case TYPE_ref: {
			symn var = ast->id.link;		// link
			if (var && var->kind == TYPE_def && var->init) {
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

			dieif(ast->cst2 != TYPE_bit, "FixMe", ast);
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
								case OPER_div: res->con.cint = lhs.con.cint / rhs.con.cint; break;		// '/'
								case OPER_mod: res->con.cint = lhs.con.cint % rhs.con.cint; break;		// '%'
							}
							else {
								error(NULL, 0, "Division by zero");
								res->con.cint = 0;
							}
						}
					}
				} break;
				case TYPE_flt: {
					res->kind = TYPE_flt;
					switch (ast->kind) {
						default: fatal("FixMe");
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
		case EMIT_opc:
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
		case TYPE_bit: res->type = type_u32; break;
		case TYPE_flt: res->type = type_f64; break;
		case TYPE_int: res->type = type_i32; break;
		case TYPE_str: res->type = type_str; break;
		//~ case EMIT_opc: ast->type = emit_opc; break;
	}

	return res->kind;
}
