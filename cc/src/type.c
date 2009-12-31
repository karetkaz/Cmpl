#include "ccvm.h"
#include <string.h>

symn type_vid = NULL;
symn type_bol = NULL;
symn type_u32 = NULL;
symn type_i32 = NULL;
symn type_i64 = NULL;
symn type_f32 = NULL;
symn type_f64 = NULL;
symn type_str = NULL;

symn type_f32x4 = NULL;
symn type_f64x2 = NULL;

//~ symn type_ptr = 0;

extern int rehash(register const char* str, unsigned size);

//~ Install
symn newdefn(ccEnv s, int kind) {
	symn def = NULL;

	if (s->_cnt > sizeof(struct symn)){
		def = (symn)s->_ptr;
		s->_ptr += sizeof(struct symn);
		s->_cnt -= sizeof(struct symn);
	}
	else {
		fatal("memory overrun");
	}

	memset(def, 0, sizeof(struct symn));
	def->kind = kind;
	return def;
}

symn instlibc(ccEnv s, const char* name);

symn installex(ccEnv s, int kind, const char* name, unsigned size, symn type, astn init) {
	symn def = newdefn(s, kind);
	unsigned hash = 0;
	if (def != NULL) {
		def->nest = s->nest;
		def->name = (char*)name;
		def->type = type;
		def->init = init;
		def->size = size;
		if (init) {
			if (init->type == 0)
				init->type = type;
		}

		hash = rehash(name, strlen(name)) % TBLS;
		def->next = s->deft[hash];
		s->deft[hash] = def;

		def->defs = s->defs;
		s->defs = def;
	}
	return def;
}
symn install(ccEnv s, int kind, const char* name, unsigned size) {
	symn typ = NULL;
	switch (kind) {
		default: fatal("no Ip here");

		// declare
		case -1: return instlibc(s, name);

		// constant
		//~ case CNST_int: typ = type_i64; kind = TYPE_def; break;
		//~ case CNST_flt: typ = type_f64; kind = TYPE_def; break;
		//TODO: case CNST_str: typ = type_str; kind = TYPE_def; break;

		// basetype
		case TYPE_vid:
		case TYPE_bit:
		case TYPE_int:
		case TYPE_flt:
		//~ case TYPE_arr:

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

	}

	return installex(s, kind, name, size, typ, 0);
}

// TODO: this is define, not declare
symn declare(ccEnv s, int kind, astn tag, symn typ) {
	symn def;// = s->deft[tag->id.hash];

	if (!tag || tag->kind != TYPE_ref) {
		debug("declare: astn('%k') is null or not an identifyer", tag);
		return 0;
	}

	/*def = lookin(s, s->deft[tag->id.hash], tag);
	if (def && def->nest == s->nest) {
		error(s->s, tag->line, "redefinition of '%T'", def);
		if (def->file && def->line)
			info(s->s, def->file, def->line, "first defined here");
	}// */

	def = installex(s, kind, tag->id.name, 0, typ, NULL);
	if (def) {
		def->line = tag->line;
		def->file = s->file;
	}

	//~ def->name = tag->name;
	//~ def->nest = s->nest;

	//~ def->type = rtyp;
	//~ def->args = args;

	tag->type = typ;
	tag->kind = TYPE_def;
	tag->id.link = def;

	switch (kind) {
		default: fatal("no Ip here");

		//~ case CNST_int:			// typedefn enum
		case TYPE_enu:			// typedefn enum
		case TYPE_rec:			// typedefn struct

		case TYPE_def:			// typename
		case TYPE_ref: {		// variable
			tag->kind = kind;
			//~ tag->link = def;
		} break;
	}

	//~ debug("declare:install('%T')", def);
	return def;
}

int isType(symn sym) {
	if (!sym) return 0;

	if (sym->kind == EMIT_opc)
		return 0;

	if (sym->kind == TYPE_ref)
		return 0;

	if (sym->kind == TYPE_def)
		return sym->init == NULL;

	//~ debug("%+T", sym);
	return 1;		// type
}
int istype(astn ast) {
	if (!ast) return 0;

	if (ast->kind == EMIT_opc)
		return 0;

	if (ast->kind == OPER_dot)
		return istype(ast->op.lhso) && istype(ast->op.rhso);

	if (ast->kind == TYPE_ref)
		return isType(ast->id.link);

	debug("%t(%+k)", ast->kind, ast);
	return 0;
}// */

symn linkTo(astn ast, symn sym) {
	symn typ = NULL;
	if (!ast) return 0;
	switch (sym->kind) {
		default: fatal("no Ip here");
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
		//~ case TYPE_arr:
		//~ case TYPE_ptr:

		case TYPE_enu:
		case TYPE_rec: {
			typ = sym;
		} break;
	}
	ast->kind = TYPE_ref;
	ast->type = typ;
	ast->id.link = sym;
	return sym;
}

symn linkOf(astn ast, int njr) {
	if (!ast) return 0;

	if (ast->kind == OPER_dot)
		return linkOf(ast->op.rhso, njr);

	if (ast->kind == OPER_dot)
		return linkOf(ast->op.rhso, njr);

	if (ast->kind == OPER_fnc)
		return linkOf(ast->op.lhso, njr);

	if (ast->kind == EMIT_opc)
		return emit_opc;

	dieif(ast->kind != TYPE_ref, "unexpected kind: %t(%+k)", ast->kind, ast);

	if (!njr && isType(ast->id.link))
		return NULL;

	return ast->id.link;
}// */
//~ symn typeOf(astn ast) {return ast ? ast->type : NULL;}
//~ long sizeOf(astn ast) {return ast && ast->type ? ast->type->size : 0;}

/** Cast
 * returns one of (u32, i32, i64, f32, f64{, p4f, p4d})
**/
int castTT(int lht, int rht, int uns) {
	switch (lht) {
		case TYPE_u32: switch (rht) {
			case TYPE_u32: return TYPE_u32;
			case TYPE_i32: return uns ? TYPE_u32 : TYPE_i32;
			case TYPE_i64: return TYPE_i64;
			case TYPE_f32: return TYPE_f32;
			case TYPE_f64: return TYPE_f64;
		} break;
		case TYPE_i32: switch (rht) {
			case TYPE_u32: return TYPE_u32;
			case TYPE_i32: return TYPE_i32;
			case TYPE_i64: return TYPE_i64;
			case TYPE_f32: return TYPE_f32;
			case TYPE_f64: return TYPE_f64;
		} break;
		case TYPE_i64: switch (rht) {
			case TYPE_u32: return TYPE_i64;
			case TYPE_i32: return TYPE_i64;
			case TYPE_i64: return TYPE_i64;
			case TYPE_f32: return TYPE_f32;
			case TYPE_f64: return TYPE_f64;
		} break;
		case TYPE_f32: switch (rht) {
			case TYPE_u32: return TYPE_f32;
			case TYPE_i32: return TYPE_f32;
			case TYPE_i64: return TYPE_f32;
			case TYPE_f32: return TYPE_f32;
			case TYPE_f64: return TYPE_f64;
		} break;
		case TYPE_f64: switch (rht) {
			case TYPE_u32: return TYPE_f64;
			case TYPE_i32: return TYPE_f64;
			case TYPE_i64: return TYPE_f64;
			case TYPE_f32: return TYPE_f64;
			case TYPE_f64: return TYPE_f64;
		} break;
	}
	debug("failed(%t, %t)", lht, rht);
	return 0;
}

/* int castId(symn typ)
 * returns internal type for symbol
**/
int castId(symn typ) {
	if (typ) switch (typ->kind) {
		default: fatal("no Ip here");
		case TYPE_vid: return TYPE_vid;
		case TYPE_bit: switch (typ->size) {
			default: fatal("no Ip here");
			case 1: case 2:
			case 4: return TYPE_u32;
			//~ case 8: return TYPE_u64;
		} break;
		case TYPE_int: switch (typ->size) {
			default: fatal("no Ip here");
			case 1: case 2:
			case 4: return TYPE_i32;
			case 8: return TYPE_i64;
		} break;
		case TYPE_flt: switch (typ->size) {
			default: fatal("no Ip here");
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

		case TYPE_ar3:
		case TYPE_enu:
		case TYPE_rec:
			return typ->kind;
	}
	debug("failed(%+T)", typ);
	return 0;
}

// promote
int castOp(symn lht, symn rht, int uns) {
	return castTT(castId(lht), castId(rht), uns);
}

// returns internal type or 0 on fail
int castTo(astn ast, int cast) {
	if (!ast)
		return 0;
	
	if (!ast->type)
		return ast->cast = cast;

	switch (cast) {
		case TYPE_bit: cast = TYPE_u32; break;
		case TYPE_vid: return ast->cast = cast;
	}
	return ast->cast = castTT(castId(ast->type), cast, 0);
}

// returns internal type or 0 on fail
int castTy(astn ast, int cast) {
	switch (ast->cast = cast) {
		case TYPE_u32: ast->type = type_u32; break;
		case TYPE_i32: ast->type = type_i32; break;
		case TYPE_i64: ast->type = type_i64; break;
		case TYPE_f32: ast->type = type_f32; break;
		case TYPE_f64: ast->type = type_f64; break;
		default:
			debug("%t", cast);
			return 0;
	}
	return cast;
}

symn lookin(ccEnv s, symn sym, astn ref, astn args) {
	symn best = 0;
	int found = 0;
	for (; sym; sym = sym->next) {
		int hascast = 0;
		astn arg = args;			// callee arguments
		symn par = sym->args;		// caller arguments
		if (!sym->name) continue;
		if (strcmp(sym->name, ref->id.name) != 0) continue;
		//~ debug("%k ?= %T", ref, sym);

		while (arg && par) {
			symn typ = par->type;
			//~ debug("%T:(%T, %T)", sym, typ, arg->type);
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
			//~ debug("%k != `%+T` : (%T, %k)", ref, sym, par, arg);
			//~ debug("%+T == %+k", par, ref);
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

	if (sym == NULL && best) {
		if (found > 1)
			warn(s->s, 1, s->file, ref->line, "best of %d overload is `%+T`", found, best);
		//~ typ = best->type;
		sym = best;
	}

	//~ debug("%+k := %+T", ref, sym);
	//~ if (s->file) info(s->s, s->file, ref->line, "%+k is %+T", ref, sym);

	if (sym) {
		symn par = sym->args;		// caller arguments
		astn arg = args;			// callee arguments
		linkTo(ref, sym);
		while (arg && par) {
			int cast = castId(par->type);
			if (arg->cast != cast) {
				//~ astn rhs = cpynode(s, arg);
				//~ arg->kind = OPER_fnc;
				//~ arg->op.lhso = NULL;
				//~ arg->op.rhso = rhs;
				//~ arg->type = NULL;
				if (!castTy(arg, cast))
					error(s->s, arg->line, "cannot set cast to %T", par->type);
			}

			//~ debug("arg(%+k): %t", arg, arg->cast);
			arg = arg->next;
			par = par->next;
		}
		if (ref->type != sym && args && args->next) {
			//~ debug("%+k == %T", ref, sym);
			//~ else look for ".ctor(args)" in sym or what the fuck ?
			dieif(arg || par, "fatal(%+k)", ref);
		}
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
		default: fatal("no Ip here %t(%+k)", ast->kind, ast);

		case OPER_dot: {
			sym = lookup(s, loc, ast->op.lhso);
			if (sym == NULL) {
				debug("lookup %+k in %T", ast->op.lhso, loc);
				return 0;
			}
			loc = ast->op.lhso->type;
			ref = ast->op.rhso;
		} break;
		case OPER_fnc: {
			symn lin = NULL;
			if (ast->op.lhso == NULL) {
				ast->type = lookup(s, 0, ast->op.rhso);
				ast->cast = castId(ast->type);
				return ast->type;
			}
			if (ast->op.lhso->kind == EMIT_opc) {
				lin = emit_opc;
			}

			if ((args = ast->op.rhso)) {
				astn next = 0;
				while (args->kind == OPER_com) {
					astn arg = args->op.rhso;
					if (!lookup(s, lin, arg)) {
						if (!lin || !lookup(s, 0, arg)) {
							debug("arg(%+k)", arg);
							return 0;
						}
						warn(s->s, 2, s->file, arg->line, "emit type cast expected: '%+k'", arg);
						debug("arg(%+k)", arg);
					}
					args->op.rhso->next = next;
					next = args->op.rhso;
					args = args->op.lhso;
				}
				if (!lookup(s, lin, args)) {
					if (!lin || !lookup(s, 0, args)) {
						debug("arg(%+k)", args);
						return 0;
					}
				}
				args->next = next;
			}

			switch (ast->op.lhso->kind) {
				case EMIT_opc: {
					if (!args)
						return ast->type = type_vid;
					args->cast = TYPE_vid;
					//~ error(s, ast->lhso->line, "emit must have ")
					return ast->type = args->type;
				} break;
				case OPER_dot: {
					if (!lookup(s, loc, ast->op.lhso->op.lhso)) {
						debug("%+k:%T", ast, loc);
						return 0;
					}
					loc = ast->op.lhso->op.lhso->type;
					ref = ast->op.lhso->op.rhso;
				} break;
				case TYPE_ref: ref = ast->op.lhso; break;
				default: fatal("no Ip here %t(%+k)", ast->kind, ast);
			}//*/

		} break;
		case OPER_idx: {
			symn lht = lookup(s, 0, ast->op.lhso);
			symn rht = lookup(s, 0, ast->op.rhso);
			/*if ((args = ast->op.rhso)) {
				astn next = 0;
				while (args->kind == OPER_com) {
					astn arg = args->op.rhso;
						debug("arg(%+k)", arg);
					if (!lookup(s, NULL, arg)) {
						debug("arg(%+k)", arg);
						return 0;
					}
					args->op.rhso->next = next;
					next = args->op.rhso;
					args = args->op.lhso;
				}
				debug("arg(%+k)", args);
				if (!lookup(s, NULL, args)) {
					debug("arg(%+k)", args);
					return 0;
				}
				args->next = next;
			}*/

			//~ ast->op.link = 0;
			if (!lht || !rht || args) {
				debug("cast(%T, %T): %T", lht, rht, 0);
				return NULL;
			}

			dieif(!lht->type, "%+k", ast);
			//~ if (kindOf(ast->op.lhso) != OPER_idx) debug("TODO(%+k):sizeof(%+T)=%d", ast->op.lhso, lht, lht->size);
			debug("TODO(%+k):sizeof(%+T)=%d", ast, lht->type, lht->type->size);
			return ast->type = lht->type;
			return 0;
		} break;

		case OPER_pls:		// '+'
		case OPER_mns: {	// '-'
			symn rht = lookup(s, 0, ast->op.rhso);

			if (!rht) {
				debug("cast(%T)[%k]: %+k", rht, ast, ast);
				return NULL;
			}

			if (!castTy(ast, castId(rht))) {
				fatal("%k %T: %+k):%t", ast, rht, ast, castId(rht));
				// 'lhs + rhs' => tok_name[OPER_add] '(lhs, rhs)'
				//~ args = ast->lhso;
				//~ ast->rhso->next = 0;
				//~ args->next = ast->rhso;
				//~ fun = tok_tbl[ast->kind].name;
				//~ ref = ast;
				//~ return ast->type;
			}
		} break;
		case OPER_cmt: {
			symn rht = lookup(s, 0, ast->op.rhso);

			if (!rht) {
				debug("cast(%T)[%k]: %+k", rht, ast, ast);
				return NULL;
			}

			switch (castTy(ast, castId(rht))) {
				case TYPE_u32:
				case TYPE_i32:
				case TYPE_i64: return ast->type;
				//~ case TYPE_f32:
				//~ case TYPE_f64:
				default: fatal("%k %T: %+k):%t", ast, rht, ast, castId(rht));
			}
		} break;
		//~ case OPER_not:

		case OPER_add:
		case OPER_sub:
		case OPER_mul:
		case OPER_div:
		case OPER_mod: {	// 'lhs op rhs' => op(lhs, rhs)
			symn lht = lookup(s, 0, ast->op.lhso);
			symn rht = lookup(s, 0, ast->op.rhso);

			//~ ast->op.link = 0;
			if (!lht || !rht) {
				debug("cast(%T, %T)[%k]: %+k", lht, rht, ast, ast);
				return NULL;
			}

			if (!castTy(ast, castOp(lht, rht, 0))) {
				fatal("%T %k %T: %+k):%t", lht, ast, rht, ast, castOp(lht, rht, 0));
				// 'lhs + rhs' => tok_name[OPER_add] '(lhs, rhs)'
				//~ args = ast->lhso;
				//~ ast->rhso->next = 0;
				//~ args->next = ast->rhso;
				//~ fun = tok_tbl[ast->kind].name;
				//~ ref = ast;
			}

		} break;

		case OPER_shl:		// '>>'
		case OPER_shr:		// '<<'
		case OPER_and:		// '&'
		case OPER_ior:		// '|'
		case OPER_xor: {	// '^'
			symn lht = lookup(s, 0, ast->op.lhso);
			symn rht = lookup(s, 0, ast->op.rhso);

			if (!lht || !rht) {
				debug("cast(%T, %T): %T", lht, rht, 0);
				return NULL;
			}
			switch (castTy(ast, castOp(lht, rht, ast->kind == OPER_shl))) {
				case TYPE_f32:
				case TYPE_f64:
					error(s->s, ast->line, "invalid cast(%+k)", ast);
					return 0;

				case TYPE_u32:
				case TYPE_i32:
				case TYPE_i64:
					return ast->type;
			}
			fatal("(%+k):%t", ast, ast->cast);
		} break;

		case OPER_equ:		// '=='
		case OPER_neq:		// '!='
		case OPER_lte:		// '<'
		case OPER_leq:		// '<='
		case OPER_gte:		// '>'
		case OPER_geq: {	// '>='
			symn lht = lookup(s, 0, ast->op.lhso);
			symn rht = lookup(s, 0, ast->op.rhso);

			//~ ast->op.link = 0;
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
			fatal("(%+k):%t", ast, ast->cast);
		} break;

		case OPER_lor:
		case OPER_lnd: {
			symn lht = lookup(s, 0, ast->op.lhso);
			symn rht = lookup(s, 0, ast->op.rhso);

			if (!lht || !rht) {
				debug("cast(%T, %T)", lht, rht);
				return NULL;
			}

			ast->cast = TYPE_bit;
			ast->type = type_bol;
		} break;

		case OPER_sel: {
			symn cmp = lookup(s, 0, ast->op.test);
			symn lht = lookup(s, 0, ast->op.lhso);
			symn rht = lookup(s, 0, ast->op.rhso);

			if (!cmp || !lht || !rht) {
				debug("cast(%T, %T)[%k]", lht, rht, ast);
				return NULL;
			}

			if (!castTy(ast, castOp(lht, rht, 0))) {
				fatal("%T %k %T: %+k):%t", lht, ast, rht, ast, castOp(lht, rht, 0));
			}
		} break;

		case ASGN_set: {	// ':='
			symn lht = lookup(s, 0, ast->op.lhso);
			symn rht = lookup(s, 0, ast->op.rhso);

			//~ ast->op.link = 0;
			ast->type = lht;

			if (!lht || !rht || args)
				return NULL;

			ast->type = lht;
			ast->cast = castId(lht);

			if (ast->cast)
				return ast->type;

			fatal("TODO");
		} break;

		case CNST_int:
		case CNST_flt:
		case CNST_str: {
			if (loc || args) {
				debug("cast()[]");
				return NULL;
			}
			switch (ast->kind) {
				case CNST_int: ast->type = type_i32; break;
				case CNST_flt: ast->type = type_f64; break;
				case CNST_str: ast->type = type_str; break;
			}
		} break;

		case TYPE_ref: ref = ast; break;
		//~ case EMIT_opc: return 0;
	}

	if (ref != NULL) {
		sym = loc ? loc->args : s->deft[ref->id.hash];

		//~ debug("lookin(sym, %+k, %+k):%T", ref, args, sym);
		sym = lookin(s, sym, ref, args);

		if (sym) {
			ast->type = ref->type;
			ast->cast = castId(ast->type);
			if (!ast->cast)
				debug("%T(%+k)", sym, ast);
		}
	}

	//~ if (ast->line) debug("%?T(%k: ref(%k)): %+k", ast->type, ast, ref, ast);

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
