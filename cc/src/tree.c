// { #include "tree.c"
//~ tree.c - tree related stuff ------------------------------------------------
#include <string.h>
#include <math.h>
#include "ccvm.h"

astn newnode(ccEnv s, int kind) {
	astn ast;
	if (s->tokp) {
		ast = s->tokp;
		s->tokp = ast->next;
	}
	else {
		ast = (astn)s->buffp;
		s->buffp += sizeof(struct astn);
		//~ if (s->buffp >= !!!)
	}
	memset(ast, 0, sizeof(struct astn));
	ast->kind = kind;
	return ast;
}

astn fltnode(ccEnv s, flt64t v) {
	astn ast = newnode(s, CNST_flt);
	//~ ast->type = type_f64;
	ast->con.cflt = v;
	return ast;
}
astn intnode(ccEnv s, int64t v) {
	astn ast = newnode(s, CNST_int);
	//~ ast->type = type_i32;
	ast->con.cint = v;
	return ast;
}
astn fh8node(ccEnv s, uns64t v) {
	astn ast = newnode(s, CNST_flt);
	//~ ast->type = type_f64;
	ast->con.cint = v;
	return ast;
}
astn strnode(ccEnv s, char * v) {
	astn ast = newnode(s, CNST_str);
	//~ ast->type = type_str;
	ast->con.cstr = v;
	return ast;
}
astn cpynode(ccEnv s, astn src) {
	astn ast = newnode(s, 0);
	*ast = *src;
	return ast;
}
void eatnode(ccEnv s, astn ast) {
	if (!ast) return;
	ast->next = s->tokp;
	s->tokp = ast;
}

signed constbol(astn ast) {
	if (ast) switch (ast->kind) {
		case CNST_int: return ast->con.cint != 0;
		case CNST_flt: return ast->con.cflt != 0;
	}
	fatal("not a constant");
}
int64t constint(astn ast) {
	if (ast) switch (ast->kind) {
		case CNST_int: return (int64t)ast->con.cint;
		case CNST_flt: return (int64t)ast->con.cflt;
	}
	fatal("not a constant");
}
flt64t constflt(astn ast) {
	if (ast) switch (ast->kind) {
		case CNST_int: return (flt64t)ast->con.cint;
		case CNST_flt: return (flt64t)ast->con.cflt;
	}
	fatal("not a constant");
}

int eval(astn res, astn ast, int get) {
	struct astn lhs, rhs;
	int cast;

	if (!ast) return 0;
	if (!res) res = &rhs;

	if ((cast = ast->cast) < TYPE_enu) {
		cast = tok_tbl[cast].type;
	}

	switch (ast->kind) {
		default: fatal("no Ip here");

		// TODO: 
		case OPER_dot: return eval(res, ast->op.rhso, get);

		case OPER_fnc:
			return 0;

		case CNST_int:
		case CNST_flt:
		case CNST_str: *res = *ast; break;
		/*case TYPE_ref: {
			if (istype(ast->rhso)) return 0;
			ret = eval(res, ast->rhso, get);
		} break;
		case OPER_dot: {
			if (!istype(ast->lhso)) return 0;
			return eval(res, ast->rhso, get);
		} break;
		case OPER_fnc: {
			if (ast->lhso && !istype(ast->lhso)) return 0;
			ret = eval(res, ast->rhso, castcon(0, ast->lhso));
		} break;*/
		case TYPE_ref: {
			symn var = ast->id.link;		// link
			if (var && var->kind == TYPE_def && var->init) {
				return eval(res, var->init, get);
			}
			return 0;
		} break;

		case OPER_pls: {		// '+'
			if (!eval(res, ast->op.rhso, get))
				return 0;
		} break;
		case OPER_mns: {		// '-'
			if (!eval(res, ast->op.rhso, get))
				return 0;
			switch (res->kind) {
				default: return 0;
				case CNST_int: res->con.cint = -res->con.cint; break;
				case CNST_flt: res->con.cflt = -res->con.cflt; break;
			}
		} break;
		case OPER_cmt: {		// '~'
			if (!eval(res, ast->op.rhso, get))
				return 0;
			switch (res->kind) {
				default: return 0;
				case CNST_int: res->con.cint = ~res->con.cint; break;
				//~ case CNST_flt: res->con.cflt = 1. / res->con.cflt; break;
			}
		} break;

		case OPER_not: {		// '!'

			dieif(get != TYPE_bit, " fault");
			if (!eval(res, ast->op.rhso, get))
				return 0;

			switch (res->kind) {
				default: return 0;
				case CNST_int: res->kind = CNST_int; res->con.cint = !res->con.cint; break;
				case CNST_flt: res->kind = CNST_int; res->con.cint = !res->con.cflt; break;
			}
		} break;

		//~ /*
		case OPER_add:			// '+'
		case OPER_sub:			// '-'
		case OPER_mul:			// '*'
		case OPER_div:			// '/'
		case OPER_mod: {		// '%'
			if (!eval(&lhs, ast->op.lhso, cast))
				return 0;
			if (!eval(&rhs, ast->op.rhso, cast))
				return 0;
			dieif(lhs.kind != rhs.kind, "eval operator %k (%t, %t): %t", ast, lhs.kind, rhs.kind, get);

			switch (rhs.kind) {
				default: return 0;
				case CNST_int: {
					res->kind = CNST_int;
					switch (ast->kind) {
						default: fatal("no Ip here");
						case OPER_add: res->con.cint = lhs.con.cint + rhs.con.cint; break;		// '+'
						case OPER_sub: res->con.cint = lhs.con.cint - rhs.con.cint; break;		// '-'
						case OPER_mul: res->con.cint = lhs.con.cint * rhs.con.cint; break;		// '*'
						case OPER_div: res->con.cint = lhs.con.cint / rhs.con.cint; break;		// '/'
						case OPER_mod: res->con.cint = lhs.con.cint % rhs.con.cint; break;		// '%'
					}
				} break;
				case CNST_flt: {
					res->kind = CNST_flt;
					switch (ast->kind) {
						default: fatal("no Ip here");
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
			if (!eval(&lhs, ast->op.lhso, cast))
				return 0;
			if (!eval(&rhs, ast->op.rhso, cast))
				return 0;
			dieif(lhs.kind != rhs.kind, "eval operator %k (%t, %t): %t", ast, lhs.kind, rhs.kind, get);
			switch (rhs.kind) {
				default: return 0;
				case CNST_int: {
					res->kind = CNST_int;
					switch (ast->kind) {
						default: fatal("no Ip here");
						case OPER_neq: res->con.cint = lhs.con.cint != rhs.con.cint; break;
						case OPER_equ: res->con.cint = lhs.con.cint == rhs.con.cint; break;
						case OPER_gte: res->con.cint = lhs.con.cint  > rhs.con.cint; break;
						case OPER_geq: res->con.cint = lhs.con.cint >= rhs.con.cint; break;
						case OPER_lte: res->con.cint = lhs.con.cint  < rhs.con.cint; break;
						case OPER_leq: res->con.cint = lhs.con.cint <= rhs.con.cint; break;
					}
				} break;
				case CNST_flt: {
					res->kind = CNST_int;
					switch (ast->kind) {
						default: fatal("no Ip here");
						case OPER_neq: res->con.cint = lhs.con.cflt != rhs.con.cflt; break;
						case OPER_equ: res->con.cint = lhs.con.cflt == rhs.con.cflt; break;
						case OPER_gte: res->con.cint = lhs.con.cflt  > rhs.con.cflt; break;
						case OPER_geq: res->con.cint = lhs.con.cflt >= rhs.con.cflt; break;
						case OPER_lte: res->con.cint = lhs.con.cflt  < rhs.con.cflt; break;
						case OPER_leq: res->con.cint = lhs.con.cflt <= rhs.con.cflt; break;
					}
				} break;
			}
		} break;

		case OPER_shr:			// '>>'
		case OPER_shl:			// '<<'
		case OPER_and:			// '&'
		case OPER_ior:			// '|'
		case OPER_xor: {		// '^'
			if (!eval(&lhs, ast->op.lhso, cast))
				return 0;
			if (!eval(&rhs, ast->op.rhso, cast))
				return 0;
			dieif(lhs.kind != rhs.kind, "eval operator %k (%t, %t): %t", ast, lhs.kind, rhs.kind, get);

			switch (rhs.kind) {
				default:
					debug("eval(%+k) : %t", ast->op.rhso, rhs.kind);
					return 0;
				case CNST_int: {
					res->kind = CNST_int;
					switch (ast->kind) {
						default: fatal("no Ip here");
						case OPER_shr: res->con.cint = lhs.con.cint >> rhs.con.cint; break;
						case OPER_shl: res->con.cint = lhs.con.cint << rhs.con.cint; break;
						case OPER_and: res->con.cint = lhs.con.cint  & rhs.con.cint; break;
						case OPER_xor: res->con.cint = lhs.con.cint  ^ rhs.con.cint; break;
						case OPER_ior: res->con.cint = lhs.con.cint  | rhs.con.cint; break;
					}
				} break;
			}
		} break;// */

		case OPER_lnd:
		case OPER_lor: {
			dieif(get != TYPE_bit, " fault");
			if (!eval(&lhs, ast->op.lhso, cast))
				return 0;
			if (!eval(&rhs, ast->op.rhso, cast))
				return 0;
			dieif(lhs.kind != rhs.kind, "eval operator %k (%t, %t): %t", ast, lhs.kind, rhs.kind, get);

			res->kind = CNST_int;
			switch (ast->kind) {
				case OPER_lor: res->con.cint = lhs.con.cint || rhs.con.cint; break;
				case OPER_lnd: res->con.cint = lhs.con.cint && rhs.con.cint; break;
			}
		} break;
		case OPER_sel: {		// '?:'
			if (!eval(&lhs, ast->op.test, TYPE_bit))
				return 0;
			return eval(res, lhs.con.cint ? ast->op.lhso : ast->op.rhso, cast);
		} break;
	}
	if (get != res->kind) switch (get) {
		default: fatal("no Ip here");

		case TYPE_any:		// no cast
			break;

		case TYPE_bit:
			res->con.cint = constbol(res);
			res->kind = CNST_int;
			break;

		case TYPE_int:
			res->con.cint = constint(res);
			res->kind = CNST_int;
			break;

		case TYPE_flt:
			res->con.cflt = constflt(res);
			res->kind = CNST_flt;
			break;
	}

	switch (res->kind) {
		case CNST_flt: res->type = type_f64; break;
		case CNST_int: res->type = type_i32; break;
	}

	return res->kind;
}

/*int cmptree(astn lhs, astn rhs) {
	if (lhs == rhs) return 1;

	if (!lhs && rhs) return 0;
	if (lhs && !rhs) return 0;

	if (lhs->kind != rhs->kind)
		return 0;
	switch (rhs->kind) {
		case CNST_int: return lhs->cint == rhs->cint;
		case CNST_flt: return lhs->cflt == rhs->cflt;
		case CNST_str: return lhs->name == rhs->name;
		//~ case TYPE_ref: return lhs->link == rhs->link;

		case OPER_add:
		case OPER_sub:
		case OPER_mul:
		case OPER_div:
		case OPER_mod:
		case OPER_idx:
		case OPER_fnc:
			return samenode(lhs->lhso, rhs->lhso)
				&& samenode(lhs->rhso, rhs->rhso);
	}
	return 0;
}// */
