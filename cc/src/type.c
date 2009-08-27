// { #include "type.c"
#include "main.h"
//~ type.c ---------------------------------------------------------------------
#include <string.h>

defn type_vid = 0;
defn type_bol = 0;
defn type_u32 = 0;

defn type_i32 = 0;
defn type_i64 = 0;
defn type_f32 = 0;
defn type_f64 = 0;
defn type_str = 0;

//~ defn type_ptr = 0;

extern int rehash(register const char* str, unsigned size);

//~ Install
defn newdefn(state s, int kind) {
	defn def = (defn)s->buffp;
	s->buffp += sizeof(struct symn_t);
	memset(def, 0, sizeof(struct symn_t));
	def->kind = kind;
	return def;
}

defn install(state s, int kind, const char* name, unsigned size) {
	defn typ = 0, def = 0;
	unsigned hash = 0;

	switch (kind) {		// check
		// declare
		defn instlibc(state s, const char* name);
		case -1: return instlibc(s, name);

		// constant
		case CNST_int: typ = type_i64; kind = TYPE_def; break;
		case CNST_flt: typ = type_f64; kind = TYPE_def; break;
		case CNST_str: typ = type_str; kind = TYPE_def; break;

		// basetype
		case TYPE_vid:
		case TYPE_bit:
		case TYPE_int:
		case TYPE_flt:

		case TYPE_u32:
		case TYPE_i32:
		case TYPE_i64:
		case TYPE_f32:
		case TYPE_f64:
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
			fatal(s, "install '%s'", name);	// this
			return 0;
	}

	def = newdefn(s, kind);
	def->kind = kind;
	def->nest = s->nest;
	def->type = typ;
	def->name = (char*)name;
	def->size = size;

	hash = rehash(name, strlen(name)) % TBLS;
	def->next = s->deft[hash];
	s->deft[hash] = def;

	def->defs = s->defs;
	s->defs = def;

	def->all = s->all;
	s->all = def;

	return def;
}
defn declare(state s, int kind, node tag, defn rtyp, defn args) {
	int hash = tag ? tag->hash : 0;
	defn def;// = s->deft[hash];

	if (!tag || tag->kind != TYPE_ref) {
		debug("declare: node('%k') is null or not an identifyer", tag);
		return 0;
	}

	// if declared on current level raise an error
	for (def = s->deft[hash]; def; def = def->next) {
		if (def->nest < s->nest) break;
		if (def->name && strcmp(def->name, tag->name) == 0) {
			error(s, tag->line, "redefinition of '%T'", def);
			if (def->file && def->line)
				info(s, def->file, def->line, "first defined here");
 		}
	}

	def = newdefn(s, kind);
	def->line = tag->line;
	def->file = s->file;

	def->name = tag->name;
	def->nest = s->nest;

	def->type = rtyp;
	def->args = args;
	//~ tag->type = rtyp;

	switch (kind) {
		default: fatal(s, "declare:install('%T')", def);

		//~ case TYPE_enu:			// typedefn enum
		//~ case TYPE_rec:			// typedefn struct

		//~ case TYPE_val:			// constant
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

	def->all = s->all;
	s->all = def;

	//~ debug("declare:install('%T')", def);
	return def;
}

/* iscall()
node iscnst(node ast) ;
defn iscast(node ast) ;		// returns type def
defn isemit(node ast) ;		// returns type ref
defn islibc(node ast) {		// returns type ref
	defn res;
	if (!ast || ast->kind != OPER_fnc) return 0;
	if (ast->lhso) switch (ast->lhso->kind) {
		case TYPE_ref: res = ast->lhso->link; break;
		case OPER_dot: res = ast->lhso->rhso ? ast->lhso->rhso->link : 0;
	}
	return res && res->libc ? res : 0;
}

int iscast(node ast) {		// type (...)
	if (!ast) return 0;
	if (ast->kind != OPER_fnc) return 0;
	return istype(ast->lhso);
	//~ if (!(ast = ast->lhso)) return 0;
	//~ if (ast->kind != TYPE_ref) return 0;
	//~ if (ast->link) return 0;
	//~ return ast->type;
}

defn iscall(node ast) {		// returns type ref
	if (!ast) return 0;
	if (ast->kind == OPER_fnc)
		ast = ast->lhso;
	if (ast->kind != TYPE_ref) return 0;
	if (ast->stmt || ast->link) return 0;
	return ast->type;
}*/
int istype(node ast) {		// type[.type]*
	if (!ast) return 0;

	//~ dieif(ast->type == 0);
	//~ dieif(ast->link == 0);
	//~ dieif(ast->kind != TYPE_ref);
	//~ dieif(ast->kind == TYPE_def);

	if (ast->kind == TYPE_def) {
		return ast->link && ast->link->kind != TYPE_ref;
	}
	if (ast->kind != TYPE_ref) return 0;
	//~ return ast->link && ast->link->kind != TYPE_ref;

	if (ast->link && ast->link->kind == TYPE_ref) return 0;
	return ast->type->kind;
}
int isemit(node ast) {		// emit (...)
	if (!ast) return 0;
	if (ast->kind != OPER_fnc) return 0;

	if (!ast->lhso) return 0;
	if (ast->lhso->kind != EMIT_opc) return 0;

	return EMIT_opc;
}
int islval(node ast) {
	if (!ast) return 0;
	//~ if (ast->kind != OPER_fnc) return 0;
	if (ast->kind == TYPE_def) {	// mean new
		if (!ast->link) return 0;
		return ast->cast == TYPE_ref;
	}
	if (ast->kind == TYPE_ref) {	// variable
		return 1;
	}
	return 0;
}// */

defn linkOf(node ast) {
	if (ast && ast->kind == OPER_dot)
		ast = ast->rhso;
	if (ast && ast->kind == TYPE_ref)
		return ast->link;
	debug("linkof:%s",tok_tbl[ast->kind].name);
	return 0;
}// */
//~ defn typeOf(node ast);
//~ long sizeOf(node ast);

//~ LookUp
/** Cast
 * returns one of (u32, i32, i64, f32, f64{, p4f, p4d})
**/
int castId(defn typ) {
	if (typ) switch (typ->kind) {
		default: fatal(0, "%T", typ);
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
			return typ->kind;

		case TYPE_enu:	// TYPE_v2d
		case TYPE_arr:
		case TYPE_rec:
			return typ->kind;
	}
	return 0;
}
int castOp(defn lht, defn rht) {
	switch (castId(lht)) {
		case TYPE_u32: switch (castId(rht)) {
			case TYPE_u32: return TYPE_u32;
			case TYPE_i32: return TYPE_i32;
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
defn lookup(state s, defn loc, node ast) {
	node ref = 0, args = 0;
	defn typ = 0, sym = 0;
	if (!ast) {
		return 0;
	}
	switch (ast->kind) {
		case OPER_dot: {
			if (!lookup(s, loc, ast->lhso)) {
				debug("lookup %+k", ast);// TODO: rem
				return 0;
			}
			loc = ast->lhso->type;
			ref = ast->rhso;
		} break;
		case OPER_fnc: {
			defn lin = isemit(ast) ? emit_opc : 0;
			args = ast->rhso;

			if (args) {
				node next = 0;
				while (args->kind == OPER_com) {
					node arg = args->rhso;
					if (!lookup(s, lin, arg)) {
						if (!lin || !lookup(s, 0, arg)) {
							debug("arg(%+k)", arg);
							return 0;
						}
					}
					// if by ref...
					args->rhso->next = next;
					next = args->rhso;
					args = args->lhso;
				}
				if (!lookup(s, lin, args)) {
					if (!lin || !lookup(s, 0, args)) {
						debug("arg(%k)", args);
						return 0;
					}
				}
				args->next = next;
				//~ args->cast = castId(args->type);
			}

			if (ast->lhso == 0) {			// a * (2 - 3)
				if (ast->rhso != args) {
					debug("ast->rhso != args");
					return NULL;
				}
				ast->type = lookup(s, 0, ast->rhso);
				ast->cast = castId(ast->type);
				return ast->type;
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
						debug("lookup %+k:%T", ast, loc);// TODO: rem
						return 0;
					}
					loc = ast->lhso->lhso->type;
					ref = ast->lhso->rhso;
				} break;
				default: ref = ast->lhso; break;
			}
		} break;
		case OPER_idx: {
			defn lht = lookup(s, 0, ast->lhso);
			defn rht = lookup(s, 0, ast->rhso);

			ast->init = 0;
			if (!lht || !rht) {
				debug("cast(%T, %T): %T", lht, rht, 0);
				return NULL;
			}
			/*switch (ast->cast = castOp(lht, rht)) {
				case TYPE_u32: return ast->type = type_u32;
				case TYPE_i32: {
					if (castId(lht) == TYPE_u32)
						return ast->type = type_u32;
					return ast->type = type_i32;
				}
				case TYPE_i64:
				case TYPE_f32:
				case TYPE_f64: {
					debug("invalid cast(%+k)", ast);
					return 0;
				}
				//~ case 0: // not a builtin type, find it.
			}*/

			dieif(!lht->type);
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
			defn ret = lookup(s, 0, ast->rhso);
			if ((cast = castId(ret))) {
				// TODO: float.cmt: invalid
				ast->rhso->cast = cast;
				return ast->type = ret;
			}
			return ast->type = NULL;
		} break;
		case OPER_not: {		// '!'
			defn ret = lookup(s, 0, ast->rhso);
			if (castId(ret)) {
				return ast->type = type_bol;
			}
			return ast->type = NULL;
		} break;

		case OPER_add:
		case OPER_sub:
		case OPER_mul:
		case OPER_div:
		case OPER_mod: {		// 'lhs op rhs' => op(lhs, rhs)
			defn lht = lookup(s, 0, ast->lhso);
			defn rht = lookup(s, 0, ast->rhso);

			ast->init = 0;
			if (!lht || !rht) {
				debug("cast(%T, %T)", lht, rht);
				return NULL;
			}
			switch (ast->cast = castOp(lht, rht)) {
				case TYPE_u32: return ast->type = type_u32;
				case TYPE_i32: return ast->type = type_i32;
				case TYPE_i64: return ast->type = type_i64;
				case TYPE_f32: return ast->type = type_f32;
				case TYPE_f64: return ast->type = type_f64;
			}
			dieif(ast->cast);

			debug("(%+k)", ast);

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
		case OPER_xor: {		// '^'
			defn lht = lookup(s, 0, ast->lhso);
			defn rht = lookup(s, 0, ast->rhso);

			ast->init = 0;
			if (!lht || !rht) {
				debug("cast(%T, %T): %T", lht, rht, 0);
				return NULL;
			}
			switch (ast->cast = castOp(lht, rht)) {
				case TYPE_u32: return ast->type = type_u32;
				case TYPE_i32:
				case TYPE_i64:
					if (ast->kind == OPER_shl)
						if (castId(lht) == TYPE_u32)
							return ast->type = type_u32;
					return ast->type = type_i32;
				case TYPE_f32:
				case TYPE_f64: {
					error(s, ast->line, "invalid cast(%+k)", ast);
					return 0;
				}
			}
			dieif(ast->cast);

			debug("(%+k)", ast);
			return 0;
		} break;

		case OPER_equ:		// '=='
		case OPER_neq:		// '!='
		case OPER_lte:		// '<'
		case OPER_leq:		// '<='
		case OPER_gte:		// '>'
		case OPER_geq: {		// '>='
			defn lht = lookup(s, 0, ast->lhso);
			defn rht = lookup(s, 0, ast->rhso);
			//~ defn ret = castTy(lht, rht);

			ast->init = 0;
			if (!lht || !rht) {
				debug("cast(%T, %T)", lht, rht);
				return NULL;
			}
			switch (ast->cast = castOp(lht, rht)) {
				case TYPE_u32:
				case TYPE_i32:
				case TYPE_i64:
				case TYPE_f32:
				case TYPE_f64: return ast->type = type_bol;
			}
			dieif(ast->cast);
			debug("(%+k)", ast);
			return 0;
		} break;

		case ASGN_set: {		// ':='
			defn lht = lookup(s, 0, ast->lhso);
			defn rht = lookup(s, 0, ast->rhso);

			ast->init = 0;
			ast->type = lht;

			if (!lht || !rht)
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

		case CNST_int: ast->cast = TYPE_i32; return ast->type = type_i32;
		case CNST_flt: ast->cast = TYPE_f64; return ast->type = type_f64;
		case TYPE_val:
		case TYPE_ref: ref = ast; break;
		case EMIT_opc: return 0;

		case OPER_lor: 
		case OPER_lnd: {
			defn lht = lookup(s, 0, ast->lhso);
			defn rht = lookup(s, 0, ast->rhso);
			if (castId(lht) && castId(rht)) {
				ast->cast = TYPE_bit;
				return ast->type = type_bol;
			}
			return NULL;
		} break;

		/*case OPER_sel: {
			defn cmp = lookup(s, 0, ast->test);
			defn lht = lookup(s, 0, ast->lhso);
			defn rht = lookup(s, 0, ast->rhso);
			//~ defn ret = castTy(lht, rht);
			if (castId(cmp)) {
				&& (ast->cast = castId(ret)))
				return ast->type = ret;
			}
			return NULL;
		} break;*/

		default: {
			debug("invalid lookup `%s` in %T", tok_tbl[ast->kind].name, loc);
		} return NULL;

	}

	//~ debug("%k[%d] in %T{%+k}", ref, ast->hash, loc, ast);

	sym = loc ? loc->args : s->deft[ref->hash];

	for (; sym; sym = sym->next) {
		node arg = args;			// callee arguments
		defn par = sym->args;		// caller arguments
		//~ if (!sym->name) continue;
		if (!sym->name || strcmp(sym->name, ref->name) != 0) continue;
		//~ debug("%k ?= %T", ref, sym);

		switch (sym->kind) {
			/*case TYPE_val: {
				//~ debug("%k ^= %T(%?k)", ref, sym->type, sym->init);
				//~ if (args && !args->cast) continue;
				ref->kind = TYPE_val;
				ref->type = ast->type = sym->type;
				//~ ast->cast = castId(sym->type);
				ref->rhso = sym->init;
				ref->link = sym;
			} break;*/
			case TYPE_def: {
				//~ debug("%k ^= %T(%?k)", ref, sym->type, sym->init);
				//~ if (args && !args->cast) continue;
				ref->kind = sym->init ? TYPE_val : TYPE_def;
				ref->type = ast->type = sym->type;
				//~ ast->cast = castId(sym->type);
				ref->rhso = sym->init;
				ref->link = sym;
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
			//~ case TYPE_ptr:

			case TYPE_enu:
			case TYPE_rec: {
				//~ if (args) continue;	// cast probably
				ref->kind = TYPE_ref;
				ref->type = ast->type = sym;
				ast->cast = castId(sym);
				ref->stmt = 0;
				ref->link = sym;
			} break;// */

			// variable/operator/property/function ?
			case EMIT_opc:
			case TYPE_ref: {
				// TODO lot to do here
				//~ debug("%k $= %T(%?k)", ref, sym->type, sym->init);
				ref->kind = TYPE_ref;
				ast->type = ref->type = sym->type;
				//~ ast->cast = ref->cast = castId(sym->type);
				//~ ref->cast = sym->type->kind;
				//~ ast->rhso = sym->init;	// init|body
				ref->link = sym;
				sym->used = 1;
			} break;
			default: {
				error(s, ast->line, "undeclared `%k`", ast);
				sym = declare(s, TOKN_err, ast, NULL, NULL);
				sym->type = type_i32;
				ast->kind = TYPE_ref;
				ast->type = type_i32;
				ast->link = sym;
			} break;
		}

		while (arg && par) {
			defn typ = par->type;
			//~ debug("%T:(%T, %T)", sym, typ, arg->type);
			if (typ == arg->type || castOp(typ, arg->type)) {
				arg->cast = castId(typ);
				arg = arg->next;
				par = par->next;
				continue;
			}
			break;
		}
		if (!arg && !typ)
			break;
		//~ debug("%k[%d] is %T{%+k}", ref, ast->hash, loc, ast);
		// if we are here then sym is found.
		//~ break;
	}

	//~ debug("lookup(%k in `%?T`) is %T[%T]", ref, loc, ast->type, sym);
	return typ = ast->type;
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
