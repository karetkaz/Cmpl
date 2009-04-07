//{ #include "tree.c"
#include "ccvm.h"
//~ tree.c - tree related stuff ------------------------------------------------
#include <string.h>
#include <math.h>

node newnode(state s, int kind) {
	node ast;
	if (s->tokp) {
		ast = s->tokp;
		s->tokp = s->tokp->next;
	}
	else {
		ast = (node)s->buffp;
		s->buffp += sizeof(struct astn_t);
		//~ if (s->buffp >= !!!)
	}
	memset(ast, 0, sizeof(struct astn_t));
	ast->kind = kind;
	return ast;
}

node fltnode(state s, flt64t v) {
	node ast = newnode(s, CNST_flt);
	ast->cflt = v;
	return ast;
}

node intnode(state s, int64t v) {
	node ast = newnode(s, CNST_int);
	ast->cint = v;
	return ast;
}

node fh8node(state s, uns64t v) {
	node ast = newnode(s, CNST_flt);
	ast->cint = v;
	return ast;
}

node strnode(state s, char * v) {
	node ast = newnode(s, CNST_str);
	ast->name = v;
	return ast;
}

void delnode(state s, node ast) {
	if (!ast) return;
	ast->next = s->tokp;
	s->tokp = ast;
}

signed constbol(node ast) {
	if (ast) switch (ast->kind) {
		//! case CNST_uns : return ast->cint;
		case CNST_int : return ast->cint != 0;
		case CNST_flt : return ast->cflt != 0;
	}
	debug("constbol('%k')", ast);
	return 0;
}

int64t constint(node ast) {
	if (ast) switch (ast->kind) {
		//! case CNST_uns : return ast->cint;
		case CNST_int : return (int64t)ast->cint;
		case CNST_flt : return (int64t)ast->cflt;
	}
	debug("constint('%k')", ast);
	return 0;
}

flt64t constflt(node ast) {
	if (ast) switch (ast->kind) {
		//! case CNST_uns : return ast->cint;
		case CNST_int : return (flt64t)ast->cint;
		case CNST_flt : return (flt64t)ast->cflt;
	}
	debug("constflt('%k')", ast);
	return 0;
}

/*compare constant node usually with 0, 1, -1*/
signed constceq(node ast, int val) {
	if (ast) switch (ast->kind) {
		//! case CNST_uns : return ast->cint == val;
		case CNST_int : return ast->cint == val;
		case CNST_flt : return ast->cflt == val;
	}
	return 0;
}

int castcon(int lht, int rht) {
	if (lht == CNST_int && rht == CNST_int) return CNST_int;
	//~ if (lht == CNST_int && rht == CNST_flt) return CNST_flt;
	//~ if (lht == CNST_flt && rht == CNST_int) return CNST_flt;
	if (lht == CNST_flt && rht == CNST_flt) return CNST_flt;
	return 0;
}// */

int eval(node res, node ast, int get) {
	int lht, rht, ret = 0;
	struct astn_t lhs, rhs;

	if (!res) res = &rhs;
	if (!ast) return 0;

	switch (ast->kind) {

		default : return 0;

		case CNST_int : ret = (*res = *ast).kind; break;
		case CNST_flt : *res = *ast; ret = ast->kind; break;
		case CNST_str : *res = *ast; ret = ast->kind; break;
		case TYPE_ref : {
			if (istype(ast->rhso)) return 0;
			ret = eval(res, ast->rhso, get);
		} break;
		case OPER_dot : {
			if (!istype(ast->lhso)) return 0;
			return eval(res, ast->rhso, get);
		} break;
		case OPER_fnc : {
			if (ast->lhso && !istype(ast->lhso)) return 0;
			ret = eval(res, ast->rhso, typeId(ast->lhso));
		} break;

		case OPER_pls : {		// '+'
			ret = eval(res, ast->rhso, 0);
		} break;
		case OPER_mns : {		// '-'
			ret = eval(res, ast->rhso, get);
			switch (res->kind = ret) {
				default : return 0;
				case CNST_int : res->cint = -res->cint; break;
				case CNST_flt : res->cflt = -res->cflt; break;
			}
		} break;

		//~ /*
		case OPER_add : 		// '+'
		case OPER_sub : 		// '-'
		case OPER_mul : 		// '*'
		case OPER_div : 		// '/'
		case OPER_mod : {		// '%'
			ret = castid(ast->lhso, ast->rhso);
			lht = eval(&lhs, ast->lhso, ret);
			rht = eval(&rhs, ast->rhso, ret);
			switch (castcon(lht, rht)) {
				default : return 0;
				case CNST_int : {
					int64t lhv = constint(&lhs);
					int64t rhv = constint(&rhs);
					ret = res->kind = CNST_int;
					switch (ast->kind) {
						case OPER_add : res->cint = lhv + rhv; break;		// '+'
						case OPER_sub : res->cint = lhv - rhv; break;		// '-'
						case OPER_mul : res->cint = lhv * rhv; break;		// '*'
						case OPER_div : res->cint = lhv / rhv; break;		// '/'
						case OPER_mod : res->cint = lhv % rhv; break;		// '%'
					}
				} break;
				case CNST_flt : {
					flt64t lhv = constflt(&lhs);
					flt64t rhv = constflt(&rhs);
					ret = res->kind = CNST_flt;
					switch (ast->kind) {
						case OPER_add : res->cflt = lhv + rhv; break;		// '+'
						case OPER_sub : res->cflt = lhv - rhv; break;		// '-'
						case OPER_mul : res->cflt = lhv * rhv; break;		// '*'
						case OPER_div : res->cflt = lhv / rhv; break;		// '/'
						case OPER_mod : res->cflt = fmod(lhv, rhv); break;	// '%'
					}
				} break;
			}
		} break;// */

		//~ /*
		case OPER_neq : 		// '!='
		case OPER_equ : 		// '=='
		case OPER_gte : 		// '>'
		case OPER_geq : 		// '>='
		case OPER_lte : 		// '<'
		case OPER_leq : {		// '<='
			ret = castid(ast->lhso, ast->rhso);
			lht = eval(&lhs, ast->lhso, ret);
			rht = eval(&rhs, ast->rhso, ret);
			switch (castcon(lht, rht)) {
				default : return 0;
				case CNST_int : {
					int64t lhv = constint(&lhs);
					int64t rhv = constint(&rhs);
					ret = res->kind = CNST_int;
					switch (ast->kind) {
						case OPER_neq : res->cint = lhv != rhv; break;
						case OPER_equ : res->cint = lhv == rhv; break;
						case OPER_gte : res->cint = lhv  > rhv; break;
						case OPER_geq : res->cint = lhv >= rhv; break;
						case OPER_lte : res->cint = lhv  < rhv; break;
						case OPER_leq : res->cint = lhv <= rhv; break;
					}
				} break;
				case CNST_flt : {
					flt64t lhv = constflt(&lhs);
					flt64t rhv = constflt(&rhs);
					ret = res->kind = CNST_int;
					switch (ast->kind) {
						case OPER_neq : res->cint = lhv != rhv; break;
						case OPER_equ : res->cint = lhv == rhv; break;
						case OPER_gte : res->cint = lhv  > rhv; break;
						case OPER_geq : res->cint = lhv >= rhv; break;
						case OPER_lte : res->cint = lhv  < rhv; break;
						case OPER_leq : res->cint = lhv <= rhv; break;
					}
				} break;
			}
		} break;// */

		//~ /*
		case OPER_shr : 		// '>>'
		case OPER_shl : 		// '<<'
		case OPER_and : 		// '&'
		case OPER_xor : 		// '^'
		case OPER_ior : {		// '|'
			ret = castid(ast->lhso, ast->rhso);
			lht = eval(&lhs, ast->lhso, ret);
			rht = eval(&rhs, ast->rhso, ret);
			switch (castcon(lht, rht)) {
				default : return 0;
				case CNST_int : {
					int64t lhv = constint(&lhs);
					int64t rhv = constint(&rhs);
					ret = res->kind = CNST_int;
					switch (ast->kind) {
						case OPER_shr : res->cint = lhv >> rhv; break;
						case OPER_shl : res->cint = lhv << rhv; break;
						case OPER_and : res->cint = lhv  & rhv; break;
						case OPER_xor : res->cint = lhv  ^ rhv; break;
						case OPER_ior : res->cint = lhv  | rhv; break;
					}
				} break;
			}
		} break;// */

		//~ case OPER_lnd : {} break;
		//~ case OPER_lor : {} break;
		/*case OPER_sel : {		// '?:'
			if (eval(res, ast->test, TYPE_bol)) {
				node tmp = constbol(res) ? ast->lhso : ast->rhso;
				ret = eval(res, tmp, castcon(lht, rht));
			}
		} break;*/
	}
	if (ret && get) switch (get) {
		default : debug("error %04x:%s", get, tok_tbl[get%tok_last].name); return 0;
		case TYPE_bol : if (res) res->cint = constbol(res); return res->kind = CNST_int;
		case TYPE_u32 : 
		case TYPE_i32 : if (res) res->cint = constint(res); return res->kind = CNST_int;
		case TYPE_f32 : 
		case TYPE_f64 : if (res) res->cflt = constflt(res); return res->kind = CNST_flt;
	}// */
	return ret;
}
//}
