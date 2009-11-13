#include "pvmc.h"
#include <string.h>

symn type_vid = NULL;
symn type_bol = NULL;
symn type_u32 = NULL;
symn type_i32 = NULL;
symn type_i64 = NULL;
symn type_f32 = NULL;
symn type_f64 = NULL;

symn type_f32x4 = NULL;
symn type_f64x2 = NULL;


//~ symn type_str = 0;
//~ symn type_ptr = 0;

extern int rehash(register const char* str, unsigned size);

//~ Install
symn newdefn(ccEnv s, int kind) {
	symn def = (symn)s->buffp;
	s->buffp += sizeof(struct symn);
	memset(def, 0, sizeof(struct symn));
	def->kind = kind;
	return def;
}

symn instlibc(ccEnv s, const char* name);

symn install(ccEnv s, int kind, const char* name, unsigned size) {
	symn typ = 0, def = 0;
	unsigned hash = 0;

	switch (kind) {		// check
		// declare
		case -1: return instlibc(s, name);

		// constant
		case CNST_int: typ = type_i64; kind = TYPE_def; break;
		case CNST_flt: typ = type_f64; kind = TYPE_def; break;
		//TODO: case CNST_str: typ = type_str; kind = TYPE_def; break;

		// basetype
		case TYPE_vid:
		case TYPE_bit:
		case TYPE_int:
		case TYPE_flt:

		//~ case TYPE_u32:
		//~ case TYPE_i32:
		//~ case TYPE_i64:
		//~ case TYPE_f32:
		//~ case TYPE_f64:
		case TYPE_p4x:
		//~ case TYPE_ptr:

		// usertype
		case TYPE_def:
		case TYPE_enu:
		case TYPE_rec:

		// variable
		case TYPE_ref:
		case EMIT_opc:
			break;

		default:
			fatal("%t(%s)", kind, name);	// this
			return 0;
	}

	def = newdefn(s, kind);
	//~ def->kind = kind;
	def->nest = s->nest;
	def->type = typ;
	def->name = (char*)name;
	def->size = size;

	hash = rehash(name, strlen(name)) % TBLS;
	def->next = s->deft[hash];
	s->deft[hash] = def;

	def->defs = s->defs;
	s->defs = def;

	return def;
}
symn declare(ccEnv s, int kind, astn tag, symn rtyp, symn args) {
	int hash = tag ? tag->hash : 0;
	symn def;// = s->deft[hash];

	if (!tag || tag->kind != TYPE_ref) {
		debug("declare: astn('%k') is null or not an identifyer", tag);
		return 0;
	}

	// if declared on current level raise error
	// TODO: use lookin here;
	def = lookin(s, s->deft[hash], tag, NULL);
	if (def && def->nest == s->nest) {
		error(s->s, tag->line, "redefinition of '%T'", def);
		if (def->file && def->line)
			info(s->s, def->file, def->line, "first defined here");
	}

	/*for (def = s->deft[hash]; def; def = def->next) {
		if (def->nest < s->nest) break;
		if (def->name && strcmp(def->name, tag->name) == 0) {
			error(s->s, tag->line, "redefinition of '%T'", def);
			if (def->file && def->line)
				info(s->s, def->file, def->line, "first defined here");
 		}
	}*/

	def = newdefn(s, kind);
	def->line = tag->line;
	def->file = s->file;

	def->name = tag->name;
	def->nest = s->nest;

	def->type = rtyp;
	def->args = args;
	//~ tag->type = rtyp;

	switch (kind) {
		default: fatal("%t('%+k')", kind, tag);

		//~ case CNST_int:			// typedefn enum
		case TYPE_enu:			// typedefn enum
		case TYPE_rec:			// typedefn struct

		case TYPE_def:			// typename
		case TYPE_ref: {		// variable
			tag->kind = kind;
			tag->type = rtyp;
			tag->link = def;
			//~ def->size = typ->size;
		} break;
	}

	// hash
	def->next = s->deft[hash];
	s->deft[hash] = def;

	// link
	def->defs = s->defs;
	s->defs = def;

	//~ def->all = s->all;
	//~ s->all = def;

	//~ debug("declare:install('%T')", def);
	return def;
}

/* iscall()
astn iscnst(astn ast) ;
symn iscast(astn ast) ;		// returns type def
symn isemit(astn ast) ;		// returns type ref
symn islibc(astn ast) {		// returns type ref
	symn res;
	if (!ast || ast->kind != OPER_fnc) return 0;
	if (ast->lhso) switch (ast->lhso->kind) {
		case TYPE_ref: res = ast->lhso->link; break;
		case OPER_dot: res = ast->lhso->rhso ? ast->lhso->rhso->link : 0;
	}
	return res && res->libc ? res : 0;
}

int iscast(astn ast) {		// type (...)
	if (!ast) return 0;
	if (ast->kind != OPER_fnc) return 0;
	return istype(ast->lhso);
	//~ if (!(ast = ast->lhso)) return 0;
	//~ if (ast->kind != TYPE_ref) return 0;
	//~ if (ast->link) return 0;
	//~ return ast->type;
}

symn iscall(astn ast) {		// returns type ref
	if (!ast) return 0;
	if (ast->kind == OPER_fnc)
		ast = ast->lhso;
	if (ast->kind != TYPE_ref) return 0;
	if (ast->stmt || ast->link) return 0;
	return ast->type;
}*/

/*astn iscdef(astn ast) {
	if (!ast) return 0;
	if (ast->kind == TYPE_ref)
		return 0;
	if (!ast->link)
		return 0;
	return ast->link->init;
}*/

int isType(symn sym) {return sym && sym->kind != TYPE_ref;}

int istype(astn ast) {
	if (!ast) return 0;

	if (ast->kind == TYPE_ref) {
		//~ return ast->link && ast->link->kind != TYPE_ref;
		return isType(ast->link);
	}

	if (ast->kind == EMIT_opc)
		return 0;

	debug("%t(%+k)", ast->kind, ast);
	return 0;
}// */
int isemit(astn ast) {		// emit (...)
	if (!ast) return 0;
	if (ast->kind != OPER_fnc) return 0;

	if (!ast->lhso) return 0;
	if (ast->lhso->kind != EMIT_opc) return 0;

	return EMIT_opc;
}// */
/*int islval(astn ast) {
	if (!ast) return 0;
	//~ if (ast->kind != OPER_fnc) return 0;

	if (ast->kind == TYPE_ref) {	// variable
		return ast->link && ast->link->kind == TYPE_ref;
	}
	return 0;
}// */

symn linkOf(astn ast) {
	if (ast && ast->kind == OPER_dot)
		ast = ast->rhso;
	dieif(!ast, "");
	dieif(ast->kind != TYPE_ref, "%+k", ast);
	return ast->link;
}// */
//~ symn typeOf(astn ast);
//~ long sizeOf(astn ast);

//~ LookUp
/** Cast
 * returns one of (u32, i32, i64, f32, f64{, p4f, p4d})
**/
int castId(symn typ) {
	if (typ) switch (typ->kind) {
		default: fatal("%T", typ);
		case TYPE_vid: return TYPE_vid;
		case TYPE_bit: switch (typ->size) {
			case 1: case 2:
			case 4: return TYPE_u32;
			//~ case 8: return TYPE_u64;
		} break;
		case TYPE_int: switch (typ->size) {
			case 1: case 2:
			case 4: return TYPE_i32;
			case 8: return TYPE_i64;
		} break;
		case TYPE_flt: switch (typ->size) {
			case 4: return TYPE_f32;
			case 8: return TYPE_f64;
		} break;

		case TYPE_u32:
		case TYPE_i32:
		case TYPE_i64:
		case TYPE_f32:
		case TYPE_f64:
		case TYPE_p4x:
			return typ->kind;

		case TYPE_enu:	// TYPE_v2d
		case TYPE_arr:
		case TYPE_rec:
			return typ->kind;
	}
	return 0;
}
int castOp(symn lht, symn rht, int uns) {
	switch (castId(lht)) {
		case TYPE_u32: switch (castId(rht)) {
			case TYPE_u32: return TYPE_u32;
			case TYPE_i32: return uns ? TYPE_u32 : TYPE_i32;
			case TYPE_i64: return TYPE_i64;
			case TYPE_f32: return TYPE_f32;
			case TYPE_f64: return TYPE_f64;
		} break;
		case TYPE_i32: switch (castId(rht)) {
			case TYPE_u32: return TYPE_u32;
			case TYPE_i32: return TYPE_i32;
			case TYPE_i64: return TYPE_i64;
			case TYPE_f32: return TYPE_f32;
			case TYPE_f64: return TYPE_f64;
		} break;
		case TYPE_i64: switch (castId(rht)) {
			case TYPE_u32: return TYPE_i64;
			case TYPE_i32: return TYPE_i64;
			case TYPE_i64: return TYPE_i64;
			case TYPE_f32: return TYPE_f32;
			case TYPE_f64: return TYPE_f64;
		} break;
		case TYPE_f32: switch (castId(rht)) {
			case TYPE_u32: return TYPE_f32;
			case TYPE_i32: return TYPE_f32;
			case TYPE_i64: return TYPE_f32;
			case TYPE_f32: return TYPE_f32;
			case TYPE_f64: return TYPE_f64;
		} break;
		case TYPE_f64: switch (castId(rht)) {
			case TYPE_u32: return TYPE_f64;
			case TYPE_i32: return TYPE_f64;
			case TYPE_i64: return TYPE_f64;
			case TYPE_f32: return TYPE_f64;
			case TYPE_f64: return TYPE_f64;
		} break;
	}
	return 0;
}// */

symn lookup(ccEnv s, symn loc, astn ast);
symn lookin(ccEnv s, symn sym, astn ref, astn args) {
	symn best = 0;
	int found = 0;
	for (; sym; sym = sym->next) {
		int hascast = 0;
		astn arg = args;			// callee arguments
		symn par = sym->args;		// caller arguments
		if (!sym->name) continue;
		if (strcmp(sym->name, ref->name) != 0) continue;
		//~ debug("%k ?= %T", ref, sym);

		while (arg && par) {
			symn typ = par->type;
			//~ debug("%T:(%T, %T)", sym, typ, arg->type);
			/*TODO: testRemove: if (typ == type_vid) {
				arg = arg->next;
				par = par->next;
				continue;
			}// */
			if (typ == arg->type || castOp(typ, arg->type, 0)) {
				hascast += typ != arg->type;
				//~ arg->cast = castId(typ);
				arg = arg->next;
				par = par->next;
				continue;
			}
			break;
		}

		if ((arg || par) && sym->kind == TYPE_ref) {		// just in case of functions
			debug("%+T == %+k", par, ref);
			continue;
		}

		if (hascast == 0)
			break;

		// keep first match
		if (best == NULL)
			best = sym;
		
		found += 1;
		// if we are here then sym is found.
		//~ debug("%+k is %T", ref, sym);
		//~ break;
	}

	//~ debug("%+k is %T", ref, sym);

	if (sym == NULL && best) {
		if (found > 1)
			warn(s->s, 1, s->file, ref->line, "best of %d overload is `%+T`", found, best);
		//~ typ = best->type;
		sym = best;
	}

	//~ if (s->file) info(s->s, s->file, ref->line, "%+k is %+T", ref, sym);

	if (sym) {
		symn par = sym->args;		// caller arguments
		astn arg = args;			// callee arguments
		symn typ = NULL;
		switch (sym->kind) {
			case EMIT_opc:
			case TYPE_def:
			case TYPE_ref: {
				//~ debug("%+k := %t(%T) : %+T = %+k", ref, sym->kind, sym, sym->type, sym->init);
				typ = sym->type;
			} break;

			// typename
			case TYPE_vid:
			case TYPE_bit:
			case TYPE_u32:
			case TYPE_int:
			case TYPE_i32:
			case TYPE_i64:
			case TYPE_flt:
			case TYPE_f32:
			case TYPE_f64:
			case TYPE_p4x:
			//~ case TYPE_ptr:

			case TYPE_enu:
			case TYPE_rec: {
				typ = sym;
			} break;
		}
		while (arg && par) {
			arg->cast = castId(par->type);
			//~ debug("arg(%+k): %t", arg, arg->cast);
			arg = arg->next;
			par = par->next;
		}
		if (typ != sym && args && args->next) {
			debug("%+k == %T", ref, sym);
			//~ else look for ".ctor(args)" in sym or what the fuck ?
			dieif(arg || par, "fatal(%+k)", ref);
		}

		//ref->cast = castId(typ);
		ref->kind = TYPE_ref;
		ref->type = typ;
		ref->link = sym;	// typ == sym ? NULL : sym;
		ref->stmt = 0;

	}

	return sym;
}

symn lookup(ccEnv s, symn loc, astn ast) {
	astn ref = 0, args = 0;
	symn sym = 0;

	if (!ast) {
		return 0;
	}

	switch (ast->kind) {
		case OPER_dot: {
			sym = lookup(s, loc, ast->lhso);
			if (sym == NULL) {
				debug("lookup %+k in %T", ast->lhso, loc);
				return 0;
			}
			loc = ast->lhso->type;
			ref = ast->rhso;
		} break;
		case OPER_fnc: {
			symn lin = NULL;
			if (ast->lhso == NULL) {
				ast->type = lookup(s, 0, ast->rhso);
				ast->cast = castId(ast->type);
				return ast->type;
			}
			if (ast->lhso->kind == EMIT_opc) {
				lin = emit_opc;
			}

			if ((args = ast->rhso)) {
				astn next = 0;
				while (args->kind == OPER_com) {
					astn arg = args->rhso;
					if (!lookup(s, lin, arg)) {
						if (!lin || !lookup(s, 0, arg)) {
							debug("arg(%+k)", arg);
							return 0;
						}
						warn(s->s, 2, s->file, arg->line, "emit type cast expected: '%+k'", arg);
						debug("arg(%+k)", arg);
					}
					args->rhso->next = next;
					next = args->rhso;
					args = args->lhso;
				}
				//~ debug("arg(%+k, %T)", args, lin);
				if (!lookup(s, lin, args)) {
					if (!lin || !lookup(s, 0, args)) {
						debug("arg(%+k)", args);
						return 0;
					}
				}
				args->next = next;
			}

			switch (ast->lhso->kind) {
				case EMIT_opc: {
					if (!args)
						return ast->type = type_vid;
					args->cast = TYPE_vid;
					//~ error(s, ast->lhso->line, "emit must have ")
					return ast->type = args->type;
				} break;
				case OPER_dot: {
					if (!lookup(s, loc, ast->lhso->lhso)) {
						debug("%+k:%T", ast, loc);
						return 0;
					}
					loc = ast->lhso->lhso->type;
					ref = ast->lhso->rhso;
				} break;
				default: ref = ast->lhso; break;
			}//*/

		} break;
		case OPER_idx: {
			symn lht = lookup(s, 0, ast->lhso);
			symn rht = lookup(s, 0, ast->rhso);

			ast->init = 0;
			if (!lht || !rht || args) {
				debug("cast(%T, %T): %T", lht, rht, 0);
				return NULL;
			}

			dieif(!lht->type, "%+k", ast);
			if (ast->lhso && ast->lhso->kind != OPER_idx)
				debug("TODO(%+k):sizeof(%+T)=%d", ast->lhso, lht, lht->size);
			debug("TODO(%+k):sizeof(%+T)=%d", ast, lht->type, lht->type->size);
			return ast->type = lht->type;
			return 0;
		} break;

		case OPER_pls:		// '+'
		case OPER_mns:		// '-'
		case OPER_cmt: {	// '~'
			int cast;
			// 'lhs op rhs' => op(lhs, rhs)
			symn ret = lookup(s, 0, ast->rhso);
			if ((cast = castId(ret))) {
				// TODO: float.cmt: invalid
				ast->rhso->cast = cast;
				return ast->type = ret;
			}
			return ast->type = NULL;
		} break;
		case OPER_not: {	// '!'
			symn ret = lookup(s, 0, ast->rhso);
			if (castId(ret)) {
				return ast->type = type_bol;
			}
			return ast->type = NULL;
		} break;

		case OPER_add:
		case OPER_sub:
		case OPER_mul:
		case OPER_div:
		case OPER_mod: {	// 'lhs op rhs' => op(lhs, rhs)
			symn lht = lookup(s, 0, ast->lhso);
			symn rht = lookup(s, 0, ast->rhso);

			ast->init = 0;
			if (!lht || !rht) {
				debug("cast(%T, %T)[%k]", lht, rht, ast);
				return NULL;
			}
			switch (ast->cast = castOp(lht, rht, 0)) {
				case TYPE_u32: return ast->type = type_u32;
				case TYPE_i32: return ast->type = type_i32;
				case TYPE_i64: return ast->type = type_i64;
				case TYPE_f32: return ast->type = type_f32;
				case TYPE_f64: return ast->type = type_f64;
				//~ case TYPE_p4x: return ast->type = type_f32x4;
			}
			dieif(ast->cast, "");

			debug("(%+k):%t", ast, ast->cast);

			// 'lhs + rhs' => tok_name[OPER_add] '(lhs, rhs)'
			//~ args = ast->lhso;
			//~ ast->rhso->next = 0;
			//~ args->next = ast->rhso;
			//~ fun = tok_tbl[ast->kind].name;
			return 0;
		} break;

		case OPER_shl:		// '>>'
		case OPER_shr:		// '<<'
		case OPER_and:		// '&'
		case OPER_ior:		// '|'
		case OPER_xor: {	// '^'
			symn lht = lookup(s, 0, ast->lhso);
			symn rht = lookup(s, 0, ast->rhso);

			ast->init = 0;
			if (!lht || !rht) {
				debug("cast(%T, %T): %T", lht, rht, 0);
				return NULL;
			}
			switch (ast->cast = castOp(lht, rht, ast->kind == OPER_shl)) {
				case TYPE_u32: return ast->type = type_u32;
				case TYPE_i32: return ast->type = type_i32;
				case TYPE_i64: return ast->type = type_i64;
					//~ if (ast->kind == OPER_shl)
						//~ if (castId(lht) == TYPE_u32)
							//~ return ast->type = type_u32;
					//~ return ast->type = type_i32;
				case TYPE_f32:
				case TYPE_f64: {
					error(s->s, ast->line, "invalid cast(%+k)", ast);
					return 0;
				}
			}
			dieif(ast->cast, "");

			debug("(%+k)", ast);
			return 0;
		} break;

		case OPER_equ:		// '=='
		case OPER_neq:		// '!='
		case OPER_lte:		// '<'
		case OPER_leq:		// '<='
		case OPER_gte:		// '>'
		case OPER_geq: {	// '>='
			symn lht = lookup(s, 0, ast->lhso);
			symn rht = lookup(s, 0, ast->rhso);
			//~ symn ret = castTy(lht, rht);

			ast->init = 0;
			if (!lht || !rht) {
				debug("cast(%T, %T)", lht, rht);
				return NULL;
			}
			switch (ast->cast = castOp(lht, rht, 1)) {
				case TYPE_u32:
				case TYPE_i32:
				case TYPE_i64:
				case TYPE_f32:
				case TYPE_f64: return ast->type = type_bol;
			}
			dieif(ast->cast, "");
			debug("(%+k)", ast);
			return 0;
		} break;

		case ASGN_set: {	// ':='
			symn lht = lookup(s, 0, ast->lhso);
			symn rht = lookup(s, 0, ast->rhso);

			ast->init = 0;
			ast->type = lht;

			if (!lht || !rht || args)
				return NULL;

			ast->type = lht;
			ast->cast = castId(lht);

			if (ast->cast)
				return ast->type;

			// 'lhs + rhs' => tok_name[OPER_add] '(lhs, rhs)'

			//~ fun = tok_tbl[ast->kind].name;			// __add
			//~ args = ast->lhso;
			//~ ast->rhso->next = 0;
			//~ args->next = ast->rhso;
			debug("(%+k)", ast);
			return 0;
		} break;

		case CNST_int: {
			if (loc || args) {
				debug("cast()[]");
				return NULL;
			}
			ast->cast = TYPE_i32;
			return ast->type = type_i32;
		}
		case CNST_flt: {
			if (loc || args) {
				debug("cast()[]");
				return NULL;
			}
			ast->cast = TYPE_f64;
			return ast->type = type_f64;
		}

		case TYPE_ref: ref = ast; break;
		case EMIT_opc: return 0;

		case OPER_lor:
		case OPER_lnd: {
			symn lht = lookup(s, 0, ast->lhso);
			symn rht = lookup(s, 0, ast->rhso);
			if (castId(lht) && castId(rht)) {
				ast->cast = TYPE_bit;
				return ast->type = type_bol;
			}
			return NULL;
		} break;

		case OPER_sel: {
			symn cmp = lookup(s, 0, ast->test);
			symn lht = lookup(s, 0, ast->lhso);
			symn rht = lookup(s, 0, ast->rhso);

			ast->init = 0;
			if (!cmp || !lht || !rht || args) {
				debug("cast(%T, %T)[%k]", lht, rht, ast);
				return NULL;
			}
			switch (ast->cast = castOp(lht, rht, 0)) {
				case TYPE_u32: return ast->type = type_u32;
				case TYPE_i32: return ast->type = type_i32;
				case TYPE_i64: return ast->type = type_i64;
				case TYPE_f32: return ast->type = type_f32;
				case TYPE_f64: return ast->type = type_f64;
			}
		} return NULL;

		default: {
			debug("invalid lookup(%s:%d) `%t` in %T", s->file, ast->line, ast->kind, loc);
		} return NULL;
	}

	//~ debug("%k[%d] in %T{%+k}", ref, ast->hash, loc, ast);

	sym = loc ? loc->args : s->deft[ref->hash];

	sym = lookin(s, sym, ref, args);

	ast->type = ref->type;
	ast->cast = castId(ast->type);

	/* typ = ast->type;
	if (typ == NULL && loc == NULL) {
		error(s->s, ast->line, "undeclared identifyer `%k`", ref);
		declare(s, TYPE_ref, ref, type_i32, NULL);
	}*/
	return ast->type;
}

int align(int offs, int pack, int size) {
	switch (pack < size ? pack : size) {
		default:
		case  0: return 0;
		case  1: return offs;
		case  2: return (offs +  1) & (~1);
		case  4: return (offs +  3) & (~3);
		case  8: return (offs +  7) & (~7);
		case 16: return (offs + 15) & (~15);
		case 32: return (offs + 31) & (~31);
	}
}

// }
