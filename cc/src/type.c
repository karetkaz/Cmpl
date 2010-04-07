#include "ccvm.h"
#include <string.h>

symn type_vid = NULL;
symn type_bol = NULL;		// ==
symn type_u32 = NULL;
symn type_i32 = NULL;		// emit
symn type_i64 = NULL;
symn type_f32 = NULL;
symn type_f64 = NULL;
symn type_v4f = NULL;
symn type_v2d = NULL;

symn type_str = NULL;

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

symn installex(ccEnv s, const char* name, int kind, unsigned size, symn type, astn init) {
	symn def = newdefn(s, kind & 0xff);
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
		if (kind & symn_call)
			def->call = 1;

		if (kind & symn_read)
			def->read = 1;

		switch (kind & 0xff) {
			default: fatal("FixMe!");

			// basetype
			case TYPE_vid:
			case TYPE_bit:
			case TYPE_int:
			case TYPE_flt:
			//~ case TYPE_arr:
				def->algn = size;

			// usertype
			case TYPE_def:
			case TYPE_enu:
			case TYPE_rec:

			// variable
			case TYPE_ref:
			case EMIT_opc:
				break;

		}

			if (name)
			hash = rehash(name, strlen(name)) % TBLS;

		def->next = s->deft[hash];
		s->deft[hash] = def;

		def->defs = s->defs;
		s->defs = def;
	}
	return def;
}
symn install(ccEnv s, const char* name, int kind, int cast, unsigned size) {
	symn typ = NULL;

	// declare
	if (kind == -1)
		return instlibc(s, name);
/*
	switch (kind & 0xff) {
		default: fatal("FixMe!");

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
// */
	return installex(s, name, kind, size, typ, 0);
}

// TODO: this is define, not declare
symn declare(ccEnv s, int kind, astn tag, symn typ) {
	symn def;

	if (!tag || tag->kind != TYPE_ref) {
		debug("declare: astn('%k') is null or not an identifyer", tag);
		return 0;
	}

	def = installex(s, tag->id.name, kind, 0, typ, NULL);
	if (!def) return NULL;

	def->line = tag->line;
	def->file = s->file;

	tag->type = typ;
	tag->id.link = def;

	tag->kind = TYPE_def;
	switch (kind) {
		default: fatal("FixMe!");

		case TYPE_enu:			// typedefn enum
		case TYPE_rec:			// typedefn struct
			if (typ != NULL)
				def->size = typ->size;
			break;

		case TYPE_def:			// typename
		case TYPE_ref:			// variable
			tag->kind = kind;
			break;
	}

	//~ debug("declare:install('%T')", def);
	return def;
}

int isType(symn sym) {
	/* Old Code
	if (!sym) return 0;

	if (sym->kind == EMIT_opc)
		return 0;

	if (sym->kind == TYPE_ref)
		return 0;

	if (sym->kind == TYPE_def)
		return sym->init == NULL;

	//~ debug("%+T", sym);
	return 1;		// type
	*/

	if (sym) switch(sym->kind) {
		//~ case TYPE_any:
		case TYPE_vid:
		case TYPE_bit:
		case TYPE_int:
		case TYPE_flt:
		case TYPE_arr:

		case TYPE_enu:
		case TYPE_rec:
		//~ case TYPE_cls:
			return 1;

		case TYPE_def:
			return sym->init == NULL;
	}
	//debug("%T is not a type", sym);
	return 0;
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

symn linkOf(astn ast, int njr) {
	if (!ast) return 0;

	// TODO: in case of static variables
	if (ast->kind == OPER_dot)		// i32.mad
		return njr ? linkOf(ast->op.rhso, njr) : NULL;

	// get base
	if (ast->kind == OPER_idx)
		return njr ? linkOf(ast->op.lhso, njr) : NULL;

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
 * returns one of (u32, i32, i64, f32, f64, p4x, TYPE_bit)
**/
int castId(symn typ) {// return typ->cast;

	if (typ == type_bol)
		return TYPE_bit;

	dieif(!isType(typ), "FixMe");

	if (typ) switch (typ->kind) {
		//~ default: fatal("FixMe");
		case TYPE_vid: return TYPE_vid;
		case TYPE_bit: switch (typ->size) {
			default: fatal("FixMe!");
			case 1: case 2:
			case 4: return TYPE_u32;
			//~ case 8: return TYPE_u64;
		} break;
		case TYPE_int: switch (typ->size) {
			default: fatal("FixMe!");
			case 1: case 2:
			case 4: return TYPE_i32;
			case 8: return TYPE_i64;
		} break;
		case TYPE_flt: switch (typ->size) {
			default: fatal("FixMe!");
			case 4: return TYPE_f32;
			case 8: return TYPE_f64;
		} break;
		//~ case TYPE_u32:
		//~ case TYPE_i32:
		//~ case TYPE_i64:
		//~ case TYPE_f32:
		//~ case TYPE_f64:
		//~ case TYPE_p4x:
			//~ return typ->kind;

		case TYPE_def:
		case TYPE_enu:
			return castId(typ->type);

		case TYPE_arr:
		case TYPE_rec:
			//~ return 0;
			return typ->kind;
	}
	debug("failed(%t): %-T", typ ? typ->kind : 0, typ);
	return 0;
}

int castTT(symn lht, symn rht) {
	int kind = 0, size = 0;

	if (lht && rht) {
		//~ kind = lht->kind;
		size = lht->size;
		switch (rht->kind) {
			case TYPE_bit:
			case TYPE_int: switch (lht->kind) {
				case TYPE_bit:
					kind = TYPE_bit;
					if (size < rht->size)
						size = rht->size;
					break;

				case TYPE_int:
					kind = TYPE_int;
					if (size < rht->size)
						size = rht->size;
					break;

				case TYPE_flt:
					kind = TYPE_flt;
					size = lht->size;
					break;

			} break;
			case TYPE_flt: switch (lht->kind) {
				case TYPE_bit:
				case TYPE_int:
				case TYPE_flt:
					kind = TYPE_flt;
					if (size < rht->size)
						size = rht->size;
					break;
			} break;
		}
	}
	else if (rht) {
		kind = rht->kind;
		size = rht->size;
	}
	switch (kind) {
		//~ case TYPE_vid: return TYPE_vid;
		case TYPE_bit: switch (size) {
			case 1: case 2:
			case 4: return TYPE_u32;
			//~ case 8: return TYPE_u64;
		} break;
		case TYPE_int: switch (size) {
			case 1: case 2:
			case 4: return TYPE_i32;
			case 8: return TYPE_i64;
		} break;
		case TYPE_flt: switch (size) {
			case 4: return TYPE_f32;
			case 8: return TYPE_f64;
		} break;
	}
	debug("(%T, %T)", lht, rht);
	return 0;
}

int castTo(astn ast, int cast) {
	if (!ast) return 0;
	// TODO: check validity
	return ast->cst2 = cast;
}
int typeOp(astn ast, symn lht, symn rht) {
	switch (castTT(lht, rht)) {
		case TYPE_u32:
			ast->type = type_u32;
			return TYPE_u32;

		case TYPE_i32:
			ast->type = type_i32;
			return TYPE_i32;
		case TYPE_i64:
			ast->type = type_i64;
			return TYPE_i64;

		case TYPE_f32:
			ast->type = type_f32;
			return TYPE_f32;

		case TYPE_f64:
			ast->type = type_f64;
			return TYPE_f64;

		default:
			fatal("FixMe");
			return 0;
	}
}

symn lookup(ccEnv s, symn sym, astn ref, astn args) {
	symn best = 0;
	int found = 0;

	dieif(!ref || ref->kind != TYPE_ref, "FixMe!");

	for (; sym; sym = sym->next) {
		int hascast = 0;
		astn arg = args;			// callee arguments
		symn par = sym->args;		// caller arguments
		if (!sym->name) continue;
		if (strcmp(sym->name, ref->id.name) != 0) continue;
		//~ debug("%k ?= %T", ref, sym);

		while (arg && par) {
			symn typ = par->type;

			if (!typ || typ == arg->type || castTT(typ, arg->type)) {
				hascast += typ != arg->type;
				arg = arg->next;
				par = par->next;
				continue;
			}
			break;
		}

		if ((arg || par) && sym->call)
			continue;

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
			warn(s->s, 2, s->file, ref->line, "using overload `%-T`", best, found);
		sym = best;
	}

	return sym;
}
symn typecheck(ccEnv s, symn loc, astn ast) {
	astn ref = 0, args = 0;
	symn sym = 0;

	if (!ast) {
		return 0;
	}

	s->err = ast;

	ast->cst2 = 0;

	switch (ast->kind) {
		default: fatal("FixMe!: %t(%+k)", ast->kind, ast);

		case OPER_dot: {
			sym = typecheck(s, loc, ast->op.lhso);
			if (sym == NULL) {
				debug("lookup %+k in %T", ast->op.lhso, loc);
				return 0;
			}
			loc = sym;//ast->op.lhso->type;//id.link;
			ref = ast->op.rhso;
			//~ debug("%k:%T:%T", ref, loc, sym);
		} break;
		case OPER_fnc: {
			astn fun = ast->op.lhso;

			if (fun == NULL) {
				symn rht = typecheck(s, 0, ast->op.rhso);
				if (!castTo(ast->op.rhso, castId(rht))) {
					debug("%T('%k', %+k): %t", rht, ast, ast, castId(rht));
					return 0;
				}
				return ast->type = rht;
			}

			if ((args = ast->op.rhso)) {
				astn next = 0;
				symn lin = NULL;
				if (fun->kind == EMIT_opc) {
					lin = emit_opc;
				}
				while (args->kind == OPER_com) {
					astn arg = args->op.rhso;
					if (!typecheck(s, lin, arg)) {
						if (!lin || !typecheck(s, 0, arg)) {
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
				if (!typecheck(s, lin, args)) {
					if (!lin || !typecheck(s, 0, args)) {
						debug("arg(%+k)", args);
						return 0;
					}
				}
				args->next = next;
			}
			else fatal("TODO");

			if (fun) switch (fun->kind) {
				/*case OPER_dot: {	// math.isNan ???
					if (!typecheck(s, loc, ast->op.lhso->op.lhso)) {
						debug("%+k:%T", ast, loc);
						return 0;
					}
					loc = ast->op.lhso->op.lhso->type;
					ref = ast->op.lhso->op.rhso;
				} break;// */
				case EMIT_opc: {
					// emit returns the type of the opcode, firs argument
					if (args)
						return ast->type = args->type;
				} break;
				case TYPE_ref:
					ref = ast->op.lhso;
					break;
				default: fatal("FixMe!: %t(%+k)", ast->kind, ast);
			}// */
		} break;
		case OPER_idx: {
			symn lht = typecheck(s, 0, ast->op.lhso);
			symn rht = typecheck(s, 0, ast->op.rhso);

			if (!lht || !rht) {
				debug("cast(%T, %T): %T", lht, rht, 0);
				return NULL;
			}
			if (!castTo(ast->op.lhso, TYPE_ref)) {
				debug("%T('%k', %+k): %t", rht, ast, ast, TYPE_ref);
				return 0;
			}
			if (!castTo(ast->op.lhso, TYPE_int)) {
				debug("%T('%k', %+k): %t", lht, ast, ast, TYPE_int);
				return 0;
			}

			// base type of array;
			return ast->type = lht->type;
		} break;

		case OPER_pls:		// '+'
		case OPER_mns:		// '-'
		case OPER_cmt:		// '~'
		case OPER_not: {	// '!'
			symn rht = typecheck(s, 0, ast->op.rhso);
			int cast;

			if (!rht || loc) {
				debug("cast(%T)[%k]: %+k", rht, ast, ast);
				return NULL;
			}

			ast->type = rht;
			if ((cast = castId(rht))) {
				if (ast->kind == OPER_not) {
					ast->type = type_bol;
					ast->cst2 = TYPE_bit;
				}
				if (!castTo(ast->op.rhso, cast)) {
					debug("%T('%k', %+k): %t", rht, ast, ast, cast);
					return 0;
				}
				switch (cast) {
					case TYPE_u32:
					case TYPE_i32:
					case TYPE_i64:
						return ast->type;

					case TYPE_f32:
					case TYPE_f64:
						if (ast->kind != OPER_cmt)
							return ast->type;

						ast->type = 0;
						error(s->s, ast->line, "invalid cast(%+k)", ast);
						return 0;
				}
				return ast->type;
			}

			/*if (!castTo(ast->op.rhso, castId(rht))) {
				debug("%k %T: %+k: %t", ast, rht, ast, castId(rht));
				return 0;
			}*/
		} break;

		case OPER_add:		// '+'
		case OPER_sub:		// '-'
		case OPER_mul:		// '*'
		case OPER_div:		// '/'
		case OPER_mod: {	// '%'
			symn lht = typecheck(s, 0, ast->op.lhso);
			symn rht = typecheck(s, 0, ast->op.rhso);
			int cast;

			if (!lht || !rht || loc) {
				debug("cast(%T, %T)[%k]: %+k", lht, rht, ast, ast);
				return NULL;
			}

			if ((cast = typeOp(ast, lht, rht))) {
				if (!castTo(ast->op.lhso, cast)) {
					debug("%T('%k', %+k): %t", lht, ast, ast, cast);
					return 0;
				}
				if (!castTo(ast->op.rhso, cast)) {
					debug("%T('%k', %+k): %t", rht, ast, ast, cast);
					return 0;
				}
				return ast->type;
			}

			fatal("operator %k (%T %T): %+k", ast, lht, rht, ast);
		} break;

		case OPER_shl:		// '>>'
		case OPER_shr:		// '<<'
		case OPER_and:		// '&'
		case OPER_ior:		// '|'
		case OPER_xor: {	// '^'
			symn lht = typecheck(s, 0, ast->op.lhso);
			symn rht = typecheck(s, 0, ast->op.rhso);
			int cast;

			if (!lht || !rht || loc) {
				debug("cast(%T, %T): %T", lht, rht, 0);
				return NULL;
			}

			if ((cast = typeOp(ast, lht, rht))) {
				if (!castTo(ast->op.lhso, cast)) {
					debug("%T('%k', %+k): %t", lht, ast, ast, cast);
					return 0;
				}
				if (!castTo(ast->op.rhso, cast)) {
					debug("%T('%k', %+k): %t", rht, ast, ast, cast);
					return 0;
				}
				switch (cast) {
					case TYPE_u32:
					case TYPE_i32:
					case TYPE_i64:
						return ast->type;

					case TYPE_f32:
					case TYPE_f64:
						ast->type = 0;
						error(s->s, ast->line, "invalid cast(%+k)", ast);
						return 0;
				}
			}

			fatal("operator %k (%T %T): %+k", ast, lht, rht, ast);
		} break;

		case OPER_equ:		// '=='
		case OPER_neq:		// '!='
		case OPER_lte:		// '<'
		case OPER_leq:		// '<='
		case OPER_gte:		// '>'
		case OPER_geq: {	// '>='
			symn lht = typecheck(s, 0, ast->op.lhso);
			symn rht = typecheck(s, 0, ast->op.rhso);
			int cast;

			if (!lht || !rht || loc) {
				debug("cast(%T, %T)", lht, rht);
				return NULL;
			}

			if ((cast = typeOp(ast, lht, rht))) {
				ast->type = type_bol;
				if (!castTo(ast->op.lhso, cast)) {
					debug("%T('%k', %+k): %t", lht, ast, ast, cast);
					return 0;
				}
				if (!castTo(ast->op.rhso, cast)) {
					debug("%T('%k', %+k): %t", rht, ast, ast, cast);
					return 0;
				}
				return ast->type;
			}

			fatal("operator %k (%T %T): %+k", ast, lht, rht, ast);
		} break;

		case OPER_lor:		// '&&'
		case OPER_lnd: {	// '||'
			symn lht = typecheck(s, 0, ast->op.lhso);
			symn rht = typecheck(s, 0, ast->op.rhso);
			int cast;

			if (!lht || !rht || loc) {
				debug("cast(%T, %T)", lht, rht);
				return NULL;
			}

			if ((cast = typeOp(ast, lht, rht))) {
				ast->type = type_bol;
				if (!castTo(ast->op.lhso, TYPE_bit)) {
					debug("%T('%k', %+k): %t", lht, ast, ast, cast);
					return 0;
				}
				if (!castTo(ast->op.rhso, TYPE_bit)) {
					debug("%T('%k', %+k): %t", rht, ast, ast, cast);
					return 0;
				}
				return ast->type;
			}

			fatal("operator %k (%T %T): %+k", ast, lht, rht, ast);
		} break;

		case OPER_sel: {	// '?:'
			symn cmp = typecheck(s, 0, ast->op.test);
			symn lht = typecheck(s, 0, ast->op.lhso);
			symn rht = typecheck(s, 0, ast->op.rhso);
			int cast;

			if (!cmp || !lht || !rht || loc) {
				debug("cast(%T, %T)[%k]", lht, rht, ast);
				return NULL;
			}

			if ((cast = typeOp(ast, lht, rht))) {
				if (!castTo(ast->op.test, TYPE_bit)) {
					debug("%T('%k', %+k): %t", cmp, ast, ast, TYPE_bit);
					return 0;
				}
				if (!castTo(ast->op.lhso, cast)) {
					debug("%T('%k', %+k): %t", lht, ast, ast, cast);
					return 0;
				}
				if (!castTo(ast->op.rhso, cast)) {
					debug("%T('%k', %+k): %t", rht, ast, ast, cast);
					return 0;
				}
				return ast->type;
			}
			fatal("operator %k (%T %T): %+k", ast, lht, rht, ast);
		} break;

		case ASGN_set: {	// ':='
			symn lht = typecheck(s, 0, ast->op.lhso);
			symn rht = typecheck(s, 0, ast->op.rhso);
			int cast;

			if (!lht || !rht || loc) {
				debug("cast(%T, %T)", lht, rht);
				return NULL;
			}

			if ((cast = typeOp(ast, lht, rht))) {
				ast->type = lht;
				if (!castTo(ast->op.rhso, cast)) {
					debug("%T('%k', %+k): %t", rht, ast, ast, cast);
					return 0;
				}
				return ast->type;
			}

			fatal("operator %k (%T %T): %+k", ast, lht, rht, ast);
		} break;

		case TYPE_int:
		case TYPE_flt:
		case CNST_str: {
			if (loc) {
				debug("cast()");
				return NULL;
			}
			switch (ast->kind) {
				case TYPE_int: return ast->type = type_i32;
				case TYPE_flt: return ast->type = type_f64;
				case CNST_str: return ast->type = type_str;
			}
		} break;

		case TYPE_ref:
			ref = ast;
			break;
		//~ case EMIT_opc: return 0;
	}

	if (ref != NULL) {
		sym = loc ? loc->args : s->deft[ref->id.hash];
		if ((sym = lookup(s, sym, ref, args))) {
			symn typ = NULL;
			switch (sym->kind) {
				default: fatal("FixMe!");
				case EMIT_opc:
				//~ case TYPE_enu:
				case TYPE_def:
				case TYPE_ref:
					typ = sym->type;
					break;

				// typename
				case TYPE_vid:
				case TYPE_bit:
				//~ case TYPE_u32:
				case TYPE_int:
				//~ case TYPE_i32:
				//~ case TYPE_i64:
				case TYPE_flt:
				//~ case TYPE_f32:
				//~ case TYPE_f64:
				//~ case TYPE_arr:
				//~ case TYPE_ptr:

				case TYPE_enu:
				case TYPE_rec:
					typ = sym;
					break;
			}

			//~ /*
			if (ast->kind == OPER_fnc && isType(sym)) {
				if (!castTo(args, castId(sym))) {
					debug("%k:%t %+k", args, castId(sym), args);
					return 0;
				}
				//~ /* TODO: fixme
				if (!castTo(ast, castId(sym))) {
					debug("%k:%t %+k", args, castId(sym), args);
					return 0;
				}// */
				ast->op.lhso = NULL; 
			}
			else {
				symn arg = sym->call ? sym->args : NULL;
				while (args && arg) {
					int cast = castId(arg->type);
					//~ debug("arg(arg: %+k, %t)", args, cast);

					//~ if (arg->load)
						//~ cast = TYPE_ref;

					if (!castTo(args, cast)) {
						debug("%k:%t %+k", args, cast, args);
						return 0;
					}
					args = args->next;
					arg = arg->next;
				}
				if (args || arg) {
					debug("%+k(%+k: %+T)", ast, args, arg);
					return 0;
				}
			}// */

			/*if (ast->kind == OPER_fnc) {
				symn arg = sym->call ? sym->args : NULL;
				while (args && arg) {
					int cast = castId(arg->type);

					//~ if (arg->load)
						//~ cast = TYPE_ref;

					if (!castTo(args, cast)) {
						debug("%k:%t %+k", args, cast, args);
						return 0;
					}
					args = args->next;
					arg = arg->next;
				}
				if (args || arg) {
					debug("%+k(%+k: %+T)", ast, args, arg);
					return 0;
				}
			}
			if (typ && !castTo(ref, castId(typ))) {
				debug("%k:%t %+k", ref, castId(typ), ast);
				return 0;
			}// */

			ref->kind = TYPE_ref;
			ref->id.link = sym;
			ref->type = typ;
			ast->type = typ;

			//~ debug("%k: %T", ref, sym);
			//~ debug("%k('%+k'): %T.%T:%T", ref, ast, loc, sym, typ);
		}
	}

	//~ if (ast->line) debug("%k('%+k'): %T.%T", ref, ast, loc, ast->type);
	if (ast->type)
		s->err = 0;

	return ast->type;
}

int padded(int offs, int align) {
	return align + ((offs - 1) & ~(align - 1));
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

//~ scoping
void enter(ccEnv s, symn def) {
	dieif(!s->_cnt, "FixMe: invalid ccEnv");
	s->nest += 1;
	//~ debug("enter(%d, %?T)", s->nest, def);
	//~ s->scope[s->nest].csym = def;
	//~ s->scope[s->nest].stmt = NULL;
}
symn leave(ccEnv s, symn dcl) {
	int i;
	symn arg = 0;
	dieif(s->_cnt <= 0, "FixMe: invalid ccEnv");
	s->nest -= 1;

	// clear from table
	for (i = 0; i < TBLS; i++) {
		symn def = s->deft[i];
		while (def && def->nest > s->nest) {
			symn tmp = def;
			def = def->next;
			tmp->next = 0;
		}
		s->deft[i] = def;
	}

	// clear from stack
	while (s->defs && s->nest < s->defs->nest) {
		symn sym = s->defs;

		sym->decl = dcl;

		s->defs = sym->defs;
		sym->defs = s->all;
		s->all = sym;

		sym->next = arg;
		arg = sym;
	}

	return arg;
}

// }
