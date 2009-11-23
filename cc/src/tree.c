// { #include "tree.c"
//~ tree.c - tree related stuff ------------------------------------------------
#include <string.h>
#include <math.h>
#include "pvmc.h"

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
	ast->cnst.cflt = v;
	return ast;
}
astn intnode(ccEnv s, int64t v) {
	astn ast = newnode(s, CNST_int);
	ast->cnst.cint = v;
	return ast;
}
astn fh8node(ccEnv s, uns64t v) {
	astn ast = newnode(s, CNST_flt);
	ast->cnst.cint = v;
	return ast;
}
astn strnode(ccEnv s, char * v) {
	astn ast = newnode(s, CNST_str);
	ast->cnst.cstr = v;
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
		case CNST_int: return ast->cnst.cint != 0;
		case CNST_flt: return ast->cnst.cflt != 0;
	}
	debug("%+k", ast);
	return 0;
}
int64t constint(astn ast) {
	if (ast) switch (ast->kind) {
		case CNST_int: return (int64t)ast->cnst.cint;
		case CNST_flt: return (int64t)ast->cnst.cflt;
	}
	debug("%+k", ast);
	return 0;
}
flt64t constflt(astn ast) {
	if (ast) switch (ast->kind) {
		case CNST_int: return (flt64t)ast->cnst.cint;
		case CNST_flt: return (flt64t)ast->cnst.cflt;
	}
	debug("%+k", ast);
	return 0;
}

int eval(astn res, astn ast, int get) {
	struct astn lhs, rhs;
	int cast;

	if (!ast) return 0;
	if (!res) res = &rhs;

	if ((cast = ast->cast) < TYPE_enu) {
		cast = tok_tbl[cast].type;
	}

	//~ if (!get) get = cast;

	switch (ast->kind) {

		// TODO: 
		case OPER_dot: return eval(res, ast->op.rhso, get);

		default:
			debug("%t(%+k)", ast->kind, ast);
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
			symn typ = ast->type;		// type
			symn var = ast->id.link;		// link
			dieif(!typ || !var, "%+t (%T || %T)", ast->kind, typ, var);

			if (var->kind == TYPE_def) {
				if (var->init)
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
				case CNST_int: res->cnst.cint = -res->cnst.cint; break;
				case CNST_flt: res->cnst.cflt = -res->cnst.cflt; break;
			}
		} break;
		case OPER_cmt: {		// '~'
			if (!eval(res, ast->op.rhso, get))
				return 0;
			switch (res->kind) {
				default: return 0;
				case CNST_int: res->cnst.cint = ~res->cnst.cint; break;
				case CNST_flt: res->cnst.cflt = 1. / res->cnst.cflt; break;
				//TODO: case CNST_flt: debug("Expression must be integral");
			}
		} break;

		case OPER_not: {		// '!'

			dieif(get != TYPE_bit, "");
			if (!eval(res, ast->op.rhso, get))
				return 0;

			switch (res->kind) {
				default: return 0;
				case CNST_int: res->kind = CNST_int; res->cnst.cint = !res->cnst.cint; break;
				case CNST_flt: res->kind = CNST_int; res->cnst.cint = !res->cnst.cflt; break;
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
						case OPER_add: res->cnst.cint = lhs.cnst.cint + rhs.cnst.cint; break;		// '+'
						case OPER_sub: res->cnst.cint = lhs.cnst.cint - rhs.cnst.cint; break;		// '-'
						case OPER_mul: res->cnst.cint = lhs.cnst.cint * rhs.cnst.cint; break;		// '*'
						case OPER_div: res->cnst.cint = lhs.cnst.cint / rhs.cnst.cint; break;		// '/'
						case OPER_mod: res->cnst.cint = lhs.cnst.cint % rhs.cnst.cint; break;		// '%'
					}
				} break;
				case CNST_flt: {
					res->kind = CNST_flt;
					switch (ast->kind) {
						case OPER_add: res->cnst.cflt = lhs.cnst.cflt + rhs.cnst.cflt; break;		// '+'
						case OPER_sub: res->cnst.cflt = lhs.cnst.cflt - rhs.cnst.cflt; break;		// '-'
						case OPER_mul: res->cnst.cflt = lhs.cnst.cflt * rhs.cnst.cflt; break;		// '*'
						case OPER_div: res->cnst.cflt = lhs.cnst.cflt / rhs.cnst.cflt; break;		// '/'
						case OPER_mod: res->cnst.cflt = fmod(lhs.cnst.cflt, rhs.cnst.cflt); break;		// '%'
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
						case OPER_neq: res->cnst.cint = lhs.cnst.cint != rhs.cnst.cint; break;
						case OPER_equ: res->cnst.cint = lhs.cnst.cint == rhs.cnst.cint; break;
						case OPER_gte: res->cnst.cint = lhs.cnst.cint  > rhs.cnst.cint; break;
						case OPER_geq: res->cnst.cint = lhs.cnst.cint >= rhs.cnst.cint; break;
						case OPER_lte: res->cnst.cint = lhs.cnst.cint  < rhs.cnst.cint; break;
						case OPER_leq: res->cnst.cint = lhs.cnst.cint <= rhs.cnst.cint; break;
					}
				} break;
				case CNST_flt: {
					res->kind = CNST_int;
					switch (ast->kind) {
						case OPER_neq: res->cnst.cint = lhs.cnst.cflt != rhs.cnst.cflt; break;
						case OPER_equ: res->cnst.cint = lhs.cnst.cflt == rhs.cnst.cflt; break;
						case OPER_gte: res->cnst.cint = lhs.cnst.cflt  > rhs.cnst.cflt; break;
						case OPER_geq: res->cnst.cint = lhs.cnst.cflt >= rhs.cnst.cflt; break;
						case OPER_lte: res->cnst.cint = lhs.cnst.cflt  < rhs.cnst.cflt; break;
						case OPER_leq: res->cnst.cint = lhs.cnst.cflt <= rhs.cnst.cflt; break;
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
						case OPER_shr: res->cnst.cint = lhs.cnst.cint >> rhs.cnst.cint; break;
						case OPER_shl: res->cnst.cint = lhs.cnst.cint << rhs.cnst.cint; break;
						case OPER_and: res->cnst.cint = lhs.cnst.cint  & rhs.cnst.cint; break;
						case OPER_xor: res->cnst.cint = lhs.cnst.cint  ^ rhs.cnst.cint; break;
						case OPER_ior: res->cnst.cint = lhs.cnst.cint  | rhs.cnst.cint; break;
					}
				} break;
			}
		} break;// */

		case OPER_lnd:
		case OPER_lor: {
			dieif(get != TYPE_bit, "");
			if (!eval(&lhs, ast->op.lhso, cast))
				return 0;
			if (!eval(&rhs, ast->op.rhso, cast))
				return 0;
			dieif(lhs.kind != rhs.kind, "eval operator %k (%t, %t): %t", ast, lhs.kind, rhs.kind, get);

			res->kind = CNST_int;
			switch (ast->kind) {
				case OPER_lor: res->cnst.cint = lhs.cnst.cint || rhs.cnst.cint; break;
				case OPER_lnd: res->cnst.cint = lhs.cnst.cint && rhs.cnst.cint; break;
			}
		} break;
		case OPER_sel: {		// '?:'
			if (!eval(&lhs, ast->op.test, TYPE_bit))
				return 0;
			return eval(res, lhs.cnst.cint ? ast->op.lhso : ast->op.rhso, cast);
		} break;
	}
	if (get != res->kind) switch (get) {
		default: {
			int ret = res->kind;
			debug("eval(%k, %t):%t", ast, get, ret);
		} return 0;

		case TYPE_any:		// no cast
			break;

		case TYPE_bit:
			res->cnst.cint = constbol(res);
			res->kind = CNST_int;
			break;

		case TYPE_int:
		//~ case TYPE_u32:
		//~ case TYPE_i32:
		//~ case TYPE_i64:
			res->cnst.cint = constint(res);
			res->kind = CNST_int;
			break;

		case TYPE_flt:
		//~ case TYPE_f32:
		//~ case TYPE_f64:
			res->cnst.cflt = constflt(res);
			res->kind = CNST_flt;
			break;
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
