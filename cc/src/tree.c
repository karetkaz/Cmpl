// { #include "tree.c"
#include "main.h"
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

void eatnode(state s, node ast) {
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

/*signed constceq(node ast, int val) {
	//~ compare constant node usually with 0, 1, -1
	if (ast) switch (ast->kind) {
		//! case CNST_uns : return ast->cint == val;
		case CNST_int : return ast->cint == val;
		case CNST_flt : return ast->cflt == val;
	}
	return 0;
}
int castcon(node lhs, node rhs) {
	int lht = lhs ? lhs->kind : 0;
	int rht = rhs ? rhs->kind : 0;
	if (lhs == NULL) return rht;
	if (lht == CNST_int && rht == CNST_int) return CNST_int;
	//~ if (lht == CNST_int && rht == CNST_flt) return CNST_flt;
	//~ if (lht == CNST_flt && rht == CNST_int) return CNST_flt;
	if (lht == CNST_flt && rht == CNST_flt) return CNST_flt;
	return 0;
}// */

int eval(node res, node ast, int get) {
	struct astn_t lhs, rhs;

	if (!ast) return 0;
	if (!res) res = &rhs;

	switch (ast->kind) {

		default : return 0;

		case CNST_int :
		case CNST_flt :
		case CNST_str : *res = *ast; break;
		/*case TYPE_ref : {
			if (istype(ast->rhso)) return 0;
			ret = eval(res, ast->rhso, get);
		} break;
		case OPER_dot : {
			if (!istype(ast->lhso)) return 0;
			return eval(res, ast->rhso, get);
		} break;
		case OPER_fnc : {
			if (ast->lhso && !istype(ast->lhso)) return 0;
			ret = eval(res, ast->rhso, castcon(0, ast->lhso));
		} break;*/

		case OPER_pls : {		// '+'
			if (!eval(res, ast->rhso, get))
				return 0;
		} break;
		case OPER_mns : {		// '-'
			if (!eval(res, ast->rhso, get))
				return 0;
			switch (res->kind) {
				default : return 0;
				case CNST_int : res->cint = -res->cint; break;
				case CNST_flt : res->cflt = -res->cflt; break;
			}
		} break;
		case OPER_cmt : {		// '~'
			if (!eval(res, ast->rhso, get))
				return 0;
			switch (res->kind) {
				default : return 0;
				case CNST_int : res->cint = ~res->cint; break;
				case CNST_flt : res->cint = 1/res->cflt; break;
				//~ case CNST_flt : debug("Expression must be integral");
			}
		} break;

		case OPER_not : {		// '!'

			dieif(get != TYPE_bit);
			if (!eval(res, ast->rhso, get))
				return 0;

			switch (res->kind) {
				default : return 0;
				case CNST_int : res->kind = CNST_int; res->cint = !res->cint; break;
				case CNST_flt : res->kind = CNST_int; res->cint = !res->cflt; break;
				//~ case CNST_flt : debug("Expression must be integral");
			}
		} break;

		//~ /*
		case OPER_add : 		// '+'
		case OPER_sub : 		// '-'
		case OPER_mul : 		// '*'
		case OPER_div : 		// '/'
		case OPER_mod : {		// '%'
			if (!eval(&lhs, ast->lhso, get))
				return 0;
			if (!eval(&rhs, ast->rhso, get))
				return 0;
			dieif(lhs.kind != rhs.kind);

			switch (rhs.kind) {
				default : return 0;
				case CNST_int : {
					res->kind = CNST_int;
					switch (ast->kind) {
						case OPER_add : res->cint = lhs.cint + rhs.cint; break;		// '+'
						case OPER_sub : res->cint = lhs.cint - rhs.cint; break;		// '-'
						case OPER_mul : res->cint = lhs.cint * rhs.cint; break;		// '*'
						case OPER_div : res->cint = lhs.cint / rhs.cint; break;		// '/'
						case OPER_mod : res->cint = lhs.cint % rhs.cint; break;		// '%'
					}
				} break;
				case CNST_flt : {
					res->kind = CNST_flt;
					switch (ast->kind) {
						case OPER_add : res->cflt = lhs.cflt + rhs.cflt; break;		// '+'
						case OPER_sub : res->cflt = lhs.cflt - rhs.cflt; break;		// '-'
						case OPER_mul : res->cflt = lhs.cflt * rhs.cflt; break;		// '*'
						case OPER_div : res->cflt = lhs.cflt / rhs.cflt; break;		// '/'
						case OPER_mod : res->cflt = fmod(lhs.cflt, rhs.cflt); break;		// '%'
					}
				} break;
			}
		} break;// */

		case OPER_neq : 		// '!='
		case OPER_equ : 		// '=='
		case OPER_gte : 		// '>'
		case OPER_geq : 		// '>='
		case OPER_lte : 		// '<'
		case OPER_leq : {		// '<='
			if (!eval(&lhs, ast->lhso, get))
				return 0;
			if (!eval(&rhs, ast->rhso, get))
				return 0;
			dieif(lhs.kind != rhs.kind);

			switch (rhs.kind) {
				default : return 0;
				case CNST_int : {
					res->kind = CNST_int;
					switch (ast->kind) {
						case OPER_neq : res->cint = lhs.cint != rhs.cint; break;
						case OPER_equ : res->cint = lhs.cint == rhs.cint; break;
						case OPER_gte : res->cint = lhs.cint  > rhs.cint; break;
						case OPER_geq : res->cint = lhs.cint >= rhs.cint; break;
						case OPER_lte : res->cint = lhs.cint  < rhs.cint; break;
						case OPER_leq : res->cint = lhs.cint <= rhs.cint; break;
					}
				} break;
				case CNST_flt : {
					res->kind = CNST_int;
					switch (ast->kind) {
						case OPER_neq : res->cint = lhs.cflt != rhs.cflt; break;
						case OPER_equ : res->cint = lhs.cflt == rhs.cflt; break;
						case OPER_gte : res->cint = lhs.cflt  > rhs.cflt; break;
						case OPER_geq : res->cint = lhs.cflt >= rhs.cflt; break;
						case OPER_lte : res->cint = lhs.cflt  < rhs.cflt; break;
						case OPER_leq : res->cint = lhs.cflt <= rhs.cflt; break;
					}
				} break;
			}
		} break;// */

		case OPER_shr : 		// '>>'
		case OPER_shl : 		// '<<'
		case OPER_and : 		// '&'
		case OPER_ior : 		// '|'
		case OPER_xor : {		// '^'
			if (!eval(&lhs, ast->lhso, get))
				return 0;
			if (!eval(&rhs, ast->rhso, get))
				return 0;
			dieif(lhs.kind != rhs.kind);

			switch (rhs.kind) {
				default : return 0;
				case TYPE_uns : 
				case TYPE_int : {
					res->kind = CNST_int;
					switch (ast->kind) {
						case OPER_shr : res->cint = lhs.cint >> rhs.cint; break;
						case OPER_shl : res->cint = lhs.cint << rhs.cint; break;
						case OPER_and : res->cint = lhs.cint  & rhs.cint; break;
						case OPER_xor : res->cint = lhs.cint  ^ rhs.cint; break;
						case OPER_ior : res->cint = lhs.cint  | rhs.cint; break;
					}
				} break;
			}
		} break;// */

		case OPER_lnd : 
		case OPER_lor : {
			dieif(get != TYPE_bit);
			if (!eval(&lhs, ast->lhso, get))
				return 0;
			if (!eval(&rhs, ast->rhso, get))
				return 0;

			dieif(lhs.kind != rhs.kind);
			res->kind = CNST_int;
			switch (ast->kind) {
				case OPER_lor : res->cint = lhs.cint || rhs.cint; break;
				case OPER_lnd : res->cint = lhs.cint && rhs.cint; break;
			}
		} break;
		case OPER_sel : {		// '?:'
			if (!eval(&lhs, ast->test, TYPE_bit))
				return 0;
			return eval(res, lhs.cint ? ast->lhso : ast->rhso, get);
		} break;
	}
	if (get != res->kind) switch (get) {
		default : {
			int ret = res->kind;
			debug("eval(%02x):%02x", get, ret);
			debug("eval(%s):%s", tok_tbl[get&15].name, tok_tbl[ret&15].name);
			//~ debug("lookup(%04x):%s", get, tok_tbl[get&15].name);
		} return 0;

		case TYPE_any : return res->kind;

		case TYPE_bit : res->cint = constbol(res); res->kind = CNST_int; break;

		case TYPE_uns :
		case TYPE_u32 :
		//~ case TYPE_u64 :

		case TYPE_int :
		case TYPE_i32 :
		case TYPE_i64 : res->cint = constint(res); res->kind = CNST_int; break;

		case TYPE_flt :
		case TYPE_f32 :
		case TYPE_f64 : res->cflt = constflt(res); res->kind = CNST_flt; break;
	}// */
	return res->kind;
}

//~ node fold();
// }
