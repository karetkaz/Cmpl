#include "ccvm.h"
#include <string.h>

//~ TODO: this should be in ccState
symn type_vid = NULL;
symn type_bol = NULL;
symn type_u32 = NULL;
symn type_i32 = NULL;
symn type_i64 = NULL;
symn type_f32 = NULL;
symn type_f64 = NULL;
//~ symn type_ptr = 0;
symn type_str = NULL;
symn null_ref = NULL;

/** define
	symn math = define(cc, type_vid, refnode(cc, "math", TYPE_def), NULL);
	if (math) {
		symn def = math;
		enter(cc, NULL);
		//~ if (def && def = install(cc, TYPE_def, "define pi = 3.14", 0, 0));	// or use code below(no parsing)
		if (def && def = define(cc, type_f32, refnode(cc, "pi", TYPE_def), fltnode(cc, 3.14)));
		if (def && def = define(cc, type_f32, refnode(cc, "pi_2", TYPE_def), fltnode(cc, 3.14 / 2)));
		...
		if (def && def = install(cc, -1, "define isNan(double x) = bool(x == x)"));

		symn install(ccState, int kind, char* str, int sizeoffset);
		if ((typ = install(cc, TYPE_int, "i32", TYPE_i32, 4))) {
			symn def = typ;
			enter(cc, NULL);
			if (def && def = install(cc, EMIT_opc, "mul", u32_mul, type_u32)) {
				def->type = type_u32;
			}
			typ->args = leave(ss, typ);
		}
	}
**/
/** lookup
	symn def = lookup(cc, cc->glob, refnode(cc, "math"), NULL);
	if (isType(def) && def = lookup(cc, def->args, refnode(cc, "pi"), NULL))
	    fputfmt("%-T", def) => "math.pi";
**/

//~ Install
symn newdefn(ccState s, int kind) {
	symn def = NULL;

	if (s->_end - s->_beg > (int)sizeof(struct symn)) {
		def = (symn)s->_beg;
		s->_beg += sizeof(struct symn);
	}
	else {
		fatal("memory overrun");
	}

	memset(def, 0, sizeof(struct symn));
	def->kind = kind;
	return def;
}

symn installex(ccState s, const char* name, int kind, unsigned size, symn type, astn init) {
	symn def = newdefn(s, kind & 0xff);
	unsigned hash = 0;
	dieif(!s || !name || !kind, "FixMe(s, %s, %t)", name, kind);
	if (def != NULL) {
		def->nest = s->nest;
		def->name = (char*)name;
		def->type = type;
		def->init = init;
		def->size = size;

		if (init && !init->type) {
			debug("FixMe '%s'", name); // null, true, false
			init->type = type;
		}

		def->cast = (kind >> 16) & 0xff;

		if (kind & symn_call)
			def->call = 1;

		if (kind & symn_read)
			def->read = 1;

		switch (kind & 0xff) {
			default:
				fatal("FixMe (%+k)", init);
				return NULL;

			// usertype
			case TYPE_def:
			case TYPE_rec:
				if (def->cast)
					def->pack = size;

			// variable
			case TYPE_ref:
			case EMIT_opc:
				break;
		}

		hash = rehash(name, -1) % TBLS;
		def->next = s->deft[hash];
		s->deft[hash] = def;

		def->defs = s->defs;
		s->defs = def;
	}
	return def;
}
symn install(ccState s, const char* name, int kind, int cast, unsigned size) {
	symn typ = NULL;

	kind |= cast << 16;

	return installex(s, name, kind, size, typ, 0);
}

/*symn installvar(ccState s, const char* name, symn type) {
	return installex(s, name, TYPE_ref | symn_read, QUAL_sta, type->size, type, 0);
}// */

symn installtyp(state s, const char* name, unsigned size) {
	//~ type_i64 = install(cc, "int64", TYPE_rec | symn_read, TYPE_i64, 8);
	return install(s->cc, name, TYPE_rec | symn_read, TYPE_rec, size);
}

// promote
static inline int castkind(int cast) {
	switch (cast) {
		case TYPE_vid: return TYPE_vid;
		case TYPE_bit: return TYPE_bit;
		case TYPE_u32:
		case TYPE_i32:
		case TYPE_i64: return TYPE_int;
		case TYPE_f32:
		case TYPE_f64: return TYPE_flt;
		case TYPE_ref: return TYPE_ref;
		//~ case 0: return 0;
	}
	//~ debug("failed: %t", cast);
	return 0;
}
symn promote(symn lht, symn rht) {
	symn pro = 0;
	if (lht && rht) {
		switch (castkind(rht->cast)) {
			case TYPE_bit:
			case TYPE_int: switch (castkind(lht->cast)) {
				case TYPE_bit:
				case TYPE_int:		// TODO: bool + int == bool; if sizeof(bool) == 4
					if (lht->cast == TYPE_bit && lht->size == rht->size)
						pro = rht;
					else
						pro = lht->size >= rht->size ? lht : rht;
					break;

				case TYPE_flt:
					pro = lht;
					break;

			} break;
			case TYPE_flt: switch (castkind(lht->cast)) {
				case TYPE_bit:
				case TYPE_int:
					pro = rht;
					break;

				case TYPE_flt:
					pro = lht->size >= rht->size ? lht : rht;
					break;
			} break;
		}
	}
	else if (rht) {
		pro = rht;
	}
	return pro;
}

symn lookup(ccState s, symn sym, astn ref, astn args) {
	symn best = 0;
	int found = 0;

	dieif(/* !sym ||  */!ref || ref->kind != TYPE_ref, "FixMe");

	for (; sym; sym = sym->next) {
		int hascast = 0;
		astn argv = args;			// arg values
		symn args = sym->args;		// arg symbols

		// array types dont have names.
		if (!sym->name)
			continue;

		// check name
		if (strcmp(sym->name, ref->id.name) != 0)
			continue;

		if (args && !sym->call) {
			if (!isType(sym))	// TODO: probably a cast
				continue;
		}

		while (argv && args) {
			symn typ = args->type;

			if (args->cast == TYPE_ref) {
				//~ debug("byref: %k", argv);
				if (argv->kind == TYPE_ref
				 && argv->id.link == null_ref)
				argv->type = typ;
				argv->cst2 = TYPE_ref;
			}

			if (args->cast == TYPE_def) {
				//~ debug("as is: %k", argv);
				if (argv->kind == TYPE_ref
				 && argv->id.link == null_ref)
				argv->type = typ;
				argv->cst2 = TYPE_ref;
			}

			if (!typ || typ == argv->type || promote(typ, argv->type)) {
				hascast += typ != argv->type;
				argv = argv->next;
				args = args->next;
				continue;
			}
			break;
		}

		if (sym->call && (argv || args)) {
			continue;
		}

		if (hascast == 0)
			break;

		// keep first match
		if (!best)
			best = sym;

		found += 1;
		// if we are here then sym is found, but it has implicit cast in it
		//~ debug("%+k is probably %-T", ref, sym);
	}

	if (sym == NULL && best) {
		if (found > 1)
			warn(s->s, 2, s->file, ref->line, "using overload `%-T`", best, found);
		sym = best;
	}

	if (sym != NULL) {
		astn argv = args;
		symn args = sym->args;

		while (args && argv) {
			if (args->cast == TYPE_ref)
				argv->cst2 = TYPE_ref;
			argv = argv->next;
			args = args->next;
		}
	}

	/*if (sym && sym->kind == TYPE_def && sym->init == NULL) {
		//~ if ()
		sym = sym->type;
	}// */

	return sym;
}

// TODO: this is define, not declare
symn declare(ccState s, int kind, astn tag, symn typ) {
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
		default: fatal("FixMe");

		case TYPE_rec:			// typedefn struct
			if (typ != NULL)
				def->size = typ->size;
			break;

		case TYPE_def:			// typename
		case TYPE_ref:			// variable
			tag->kind = kind;
			break;
	}

	return def;
}

int isType(symn sym) {
	if (sym) switch(sym->kind) {
		//~ case TYPE_vid:
		//~ case TYPE_bit:
		//~ case CNST_int:
		//~ case CNST_flt:
		//~ case TYPE_str:

		case TYPE_arr:
		case TYPE_rec:
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

	debug("%t(%+k):(%d)", ast->kind, ast, ast->line);
	return 0;
}// */

symn linkOf(astn ast, int njr2) {
	if (!ast) return 0;

	if (ast->kind == OPER_fnc)
		return linkOf(ast->op.lhso, njr2);

	// TODO: static variables ?
	if (ast->kind == OPER_dot)		// i32.mad
		return linkOf(ast->op.rhso, njr2);
		//~ return njr ? linkOf(ast->op.rhso, njr) : NULL;

	// get base
	if (ast->kind == OPER_idx)
		return linkOf(ast->op.lhso, njr2);
		//~ return njr ? linkOf(ast->op.lhso, njr) : NULL;

	if (ast->kind == EMIT_opc)
		return emit_opc;

	dieif(ast->kind != TYPE_ref, "unexpected kind: %t(%+k)", ast->kind, ast);

	//~ if (!njr && isType(ast->id.link))
		//~ return NULL;

	/*if (isType(ast->id.link)) {
		if (ast->id.link->kind == TYPE_def)
			return linkOf(ast->id.link->init, njr2);
		//~ if (!njr)
			//~ return NULL;
	}// */

	if (isType(ast->id.link))
		return ast->type;

	//~ if (ast->id.link->kind == TYPE_def)
		//~ return linkOf(ast->id.link->init, njr2);

	return ast->id.link;
}// */
//~ symn typeOf(astn ast) {return ast ? ast->type : NULL;}

long sizeOf(symn typ) {
	if (typ) switch (typ->kind) {
		//~ default: fatal("FixMe");
		//~ case TYPE_vid:
		//~ case TYPE_bit:
		//~ case TYPE_int:
		//~ case TYPE_flt:
		case TYPE_rec:
		case EMIT_opc:
			return typ->size;

		case TYPE_arr:
			return typ->size * sizeOf(typ->type);

		//~ case TYPE_def:
			//~ return sizeOf(typ->type);
	}
	fatal("failed(%t): %-T", typ ? typ->kind : 0, typ);
	return 0;
}

/** Cast
 * returns one of (TYPE_bit, ref, u32, i32, i64, f32, f64)
**/
int castOf(symn typ) {
	if (typ) switch (typ->kind) {

		case TYPE_def:
			return castOf(typ->type);

		//~ case TYPE_vid:
			//~ return typ->kind;

		case EMIT_opc:
		case TYPE_arr:
		case TYPE_rec:
			return typ->cast;
	}
	debug("failed(%t): %?-T", typ ? typ->kind : 0, typ);
	return 0;
}
int castTo(astn ast, int cast) {
	if (!ast) return 0;
	// TODO: check validity / Remove function
	//~ debug("cast to (%d)", cast);
	return ast->cst2 = cast;
}
int castTy(astn ast, symn type) {
	if (!ast) return 0;
	// TODO: check validity / Remove function
	//~ if (ast->type == emit_opc)
		//~ return EMIT_opc;

	return castTo(ast, castOf(ast->type = type));
}

symn typecheck(ccState cc, symn loc, astn ast) {
	astn ref = 0, args = 0;
	astn dot = NULL;	// temp
	symn sym = 0;

	if (!ast) {
		return 0;
	}

	ast->cst2 = 0;

	switch (ast->kind) {
		default:
			fatal("FixMe: %t(%+k)", ast->kind, ast);
			break;

		case OPER_fnc: {
			astn fun = ast->op.lhso;

			if (fun == NULL) {
				symn rht = typecheck(cc, 0, ast->op.rhso);
				if (!rht || !castTo(ast->op.rhso, castOf(rht))) {
					debug("%T('%k', %+k): %t", rht, ast, ast, castOf(rht));
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
					if (!typecheck(cc, lin, arg)) {
						if (!lin || !typecheck(cc, NULL, arg)) {
							debug("arg(%+k)", arg);
							return 0;
						}
						if (!(arg->kind == OPER_fnc && isType(linkOf(arg->op.lhso, 0)))) {
							debug("%T", linkOf(arg->op.lhso, 0));
							warn(cc->s, 2, cc->file, arg->line, "emit type cast expected: '%+k'", arg);
						}
					}
					args->op.rhso->next = next;
					next = args->op.rhso;
					args = args->op.lhso;
				}
				if (!typecheck(cc, lin, args)) {
					if (!lin || !typecheck(cc, 0, args)) {
						debug("arg(%+k)", args);
						return 0;
					}
					if (!istype(args) && !(args->kind == OPER_fnc && isType(linkOf(args, 0)))) {
						debug("%T", linkOf(args, 0));
						warn(cc->s, 2, cc->file, args->line, "emit type cast expected: '%+k'", args);
					}
					//~ if (!istype(args)) warn(cc->s, 2, cc->file, args->line, "emit type cast expected: '%+k'", args);
				}
				args->next = next;
			}

			if (fun) switch (fun->kind) {
				case OPER_dot: {	// math.isNan ???
					astn call = ast->op.lhso;
					if (!(loc = typecheck(cc, loc, call->op.lhso))) {
						debug("%+k:%T", ast, loc);
						return 0;
					}
					dot = call;
					//~ loc = call->op.lhso->type;
					ref = call->op.rhso;
				} break;// */
				case EMIT_opc: {
					// TODO: type of 'emit()' will be emit, to match all types
					//~      type of 'emit' will be emit by ref, to match all types
					//~      use: "int a = emit;" or "int a = emit();"
					//~ return ast->type = args ? args->type : type_vid;
					return ast->type = args ? args->type : emit_opc;
				} break;
				case TYPE_ref:
					ref = ast->op.lhso;
					break;
				default:
					fatal("FixMe: %t(%+k)", ast->kind, ast);
			}// */

			if (args == NULL)
				args = cc->argz;

		} break;
		case OPER_dot: {
			//~ debug("%+k", ast);
			sym = typecheck(cc, loc, ast->op.lhso);
			if (sym == NULL) {
				debug("lookup %+k in %T", ast->op.lhso, loc);
				return 0;
			}
			if (!castTo(ast->op.lhso, TYPE_ref)) {
				debug("%T('%k', %+k): %t", sym, ast, ast, TYPE_ref);
				return 0;
			}
			/*if (!castTo(ast->op.lhso, castOf(sym))) {
				debug("%T('%k', %+k): %t", sym, ast, ast, TYPE_ref);
				return 0;
			}// */
			loc = sym;
			ref = ast->op.rhso;
		} break;
		case OPER_idx: {
			symn lht = typecheck(cc, loc, ast->op.lhso);
			symn rht = typecheck(cc, loc, ast->op.rhso);

			if (!lht || !rht) {
				debug("cast(%T, %T): %+k", lht, rht, ast);
				return NULL;
			}
			if (!castTo(ast->op.lhso, TYPE_ref)) {
				debug("%T('%k', %+k): %t", rht, ast, ast, TYPE_ref);
				return 0;
			}
			if (!castTo(ast->op.rhso, TYPE_int)) {
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
			int cast;
			symn rht = typecheck(cc, loc, ast->op.rhso);

			if (!rht || loc) {
				debug("cast(%T)[%k]: %+k", rht, ast, ast);
				return NULL;
			}
			if ((cast = castTy(ast, rht))) {
				ast->type = rht;
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
						error(cc->s, ast->line, "invalid cast(%+k)", ast);
						return 0;
				}
				return ast->type;
			}
			fatal("operator %k (%T): %+k", ast, rht, ast);
		} break;

		case OPER_add:		// '+'
		case OPER_sub:		// '-'
		case OPER_mul:		// '*'
		case OPER_div:		// '/'
		case OPER_mod: {	// '%'
			symn lht = typecheck(cc, loc, ast->op.lhso);
			symn rht = typecheck(cc, loc, ast->op.rhso);

			if (!lht || !rht || loc) {
				debug("cast(%T, %T)[%k]: %+k", lht, rht, ast, ast);
				return NULL;
			}
			if (lht->cast && rht->cast) {
				symn typ = promote(lht, rht);
				int cast = castOf(typ);
				if (!castTo(ast->op.lhso, cast)) {
					debug("%T('%k', %+k): %-T", lht, ast, ast, typ);
					return 0;
				}
				if (!castTo(ast->op.rhso, cast)) {
					debug("%T('%k', %+k): %t", rht, ast, ast, cast);
					return 0;
				}
				return ast->type = typ;
			}// */
			fatal("operator %k (%T, %T): %+k", ast, lht, rht, ast);
		} break;

		case OPER_shl:		// '>>'
		case OPER_shr:		// '<<'
		case OPER_and:		// '&'
		case OPER_ior:		// '|'
		case OPER_xor: {	// '^'
			int cast;
			symn lht = typecheck(cc, loc, ast->op.lhso);
			symn rht = typecheck(cc, loc, ast->op.rhso);

			if (!lht || !rht || loc) {
				debug("cast(%T, %T): %T", lht, rht, 0);
				return NULL;
			}
			if ((cast = castTy(ast, promote(lht, rht)))) {
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
						error(cc->s, ast->line, "invalid cast(%+k)", ast);
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
			int cast;
			symn lht = typecheck(cc, loc, ast->op.lhso);
			symn rht = typecheck(cc, loc, ast->op.rhso);

			if (!lht || !rht || loc) {
				debug("cast(%T, %T)", lht, rht);
				return NULL;
			}
			//~ if (lht->cast && rht->cast) {
			if ((cast = castOf(promote(lht, rht)))) {
				if (!castTy(ast, type_bol)) {
					debug("%T('%k', %+k): %t", lht, ast, ast, cast);
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

		case OPER_lor:		// '&&'
		case OPER_lnd: {	// '||'
			int cast;
			symn lht = typecheck(cc, loc, ast->op.lhso);
			symn rht = typecheck(cc, loc, ast->op.rhso);

			if (!lht || !rht || loc) {
				debug("cast(%T, %T)", lht, rht);
				return NULL;
			}
			if ((cast = castTy(ast, promote(lht, rht)))) {
				if (!castTo(ast->op.lhso, TYPE_bit)) {
					debug("%T('%k', %+k): %t", lht, ast, ast, cast);
					return 0;
				}
				if (!castTo(ast->op.rhso, TYPE_bit)) {
					debug("%T('%k', %+k): %t", rht, ast, ast, cast);
					return 0;
				}
				ast->type = type_bol;
				return ast->type;
			}
			fatal("operator %k (%T %T): %+k", ast, lht, rht, ast);
		} break;

		case OPER_sel: {	// '?:'
			int cast;
			symn cmp = typecheck(cc, loc, ast->op.test);
			symn lht = typecheck(cc, loc, ast->op.lhso);
			symn rht = typecheck(cc, loc, ast->op.rhso);

			if (!cmp || !lht || !rht || loc) {
				debug("cast(%T, %T)[%k]", lht, rht, ast);
				return NULL;
			}
			if ((cast = castTy(ast, promote(lht, rht)))) {
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

		// operator set
		case ASGN_set: {	// ':='
			symn lht = typecheck(cc, loc, ast->op.lhso);
			symn rht = typecheck(cc, loc, ast->op.rhso);

			if (!lht || !rht || loc) {
				debug("cast(%T, %T)", lht, rht);
				return NULL;
			}

			/* this didn't work with i += 1; same ast for i
			if (!castTo(ast->op.lhso, TYPE_ref)) {
				debug("%T('%k', %+k): %t", rht, ast, ast, TYPE_ref);
				return 0;
			}// */
			if (!castTo(ast->op.rhso, castOf(lht))) {
				debug("%T('%k', %+k): %t", rht, ast, ast, castOf(lht));
				return 0;
			}
			ast->type = lht;
			return ast->type;
		} break;

		// operator get
		case TYPE_ref:
			ref = ast;
			break;

		case TYPE_int:
		case TYPE_flt:
		case TYPE_str:
		case EMIT_opc: {
			if (loc) {
				debug("cast()");
				return NULL;
			}
			switch (ast->kind) {
				case TYPE_int: 
					if (ast->con.cint != (int32_t)ast->con.cint) {
						//~ debug("alma %k @(%d)", ast, ast->line);
						return ast->type = type_i64;
					}
					return ast->type = type_i32;
				case TYPE_flt: return ast->type = type_f64;
				case TYPE_str: return ast->type = type_str;
				case EMIT_opc: return ast->type = emit_opc;
			}
		} break;
	}

	if (ref != NULL) {
		sym = loc ? loc->args : cc->deft[ref->id.hash];
		if ((sym = lookup(cc, sym, ref, args))) {
			symn typ = NULL;

			switch (sym->kind) {
				default: fatal("FixMe");

				case TYPE_def:
					if (sym->type && sym->type->kind == TYPE_ref)
						sym = sym->type;
					typ = sym->type;
					//~ debug("%T:%T in `%+k` (%d)", sym, typ, ast, ast->line);
					break;

				case EMIT_opc:
				case TYPE_ref:
				//~ case TYPE_def:
					typ = sym->type;
					break;

				//~ case TYPE_arr:
				case TYPE_rec:
					typ = sym;
					break;
			}

			//~ debug("call: %+k(%+k)", ref, args);
			/*if (ast->line) {
				debug("%T:%T in `%+k` (%d)", sym, typ, ast, ast->line);
				//~ info(s->s, s->file, ast->line, "%T:%T in `%+k` (%d)", sym, typ, ast, ast->line);
			}// */

			//~ /*
			if (ast->kind == OPER_fnc && isType(sym)) {
				if (!castTo(args, castOf(sym))) {
					debug("%k:%t %+k", args, castOf(sym), args);
					return 0;
				}
				//~ /* TODO: fix this calling fixargs ?
				if (!castTo(ast, castOf(sym))) {
					debug("%k:%t %+k", args, castOf(sym), args);
					return 0;
				}// */
				//~ ast->op.lhso = NULL;
			}
			else if (ast->kind == OPER_fnc) {
				symn arg = sym->call ? sym->args : NULL;
				while (args && arg) {
					//~ TODO: if function: arg->call

					if (!castTo(args, castOf(arg->type))) {
						debug("%k:%t %+k", args, castOf(arg->type), args);
						return 0;
					}
					if (arg->cast == TYPE_ref) {
						if (args->type != arg->type) {
							if (args->type->cast != arg->type->cast
							||  args->type->size != arg->type->size) {
								debug("%+k(%+k: %+T)", ast, args, arg);
								return 0;
							}
						}
						args->cst2 = TYPE_ref;
					}
					args = args->next;
					arg = arg->next;
				}
				if (args || arg) {
					debug("%+k(%+k: %+T)", ast, args, arg);
					return 0;
				}
			}// */

			/*if (sym->used != ref) {
				ref->id.nuse = sym->used;
				sym->used = ref;
			}// */

			ref->kind = TYPE_ref;
			ref->id.link = sym;
			ref->type = typ;
			ast->type = typ;
			if (dot)
				dot->type = typ;

			//~ TODO: review this: functions, arrays and strings are passed by reference

			//~ if (typ != sym)
				//~ sym->cast = typ->cast;

			//~ debug("%k: %T", ref, sym);
		}
		else
			cc->root = ref;
	}

	return ast->type;
}

int padded(int offs, int align) {
	dieif(align != (align & -align), "FixMe");
	return align + ((offs - 1) & ~(align - 1));
}

/*int argsize(symn sym, int align) {
	symn arg;
	int stdiff = 0;
	for (arg = sym->args; arg; arg = arg->next) {
		stdiff += padded(sizeOf(arg->type), align);
	}
	return stdiff;
}// */

int fixargs(symn sym, int align, int stbeg) {
	symn arg;
	int stdiff = 0;
	//~ int stbeg = sizeOf(sym->type);
	for (arg = sym->args; arg; arg = arg->next) {
		arg->offs = stbeg + stdiff;
		if (arg->cast != TYPE_ref)
			stdiff += padded(sizeOf(arg->type), align);
		else
			stdiff += padded(4, align);
	}
	//~ because args are evaluated from right to left
	for (arg = sym->args; arg; arg = arg->next) {
		arg->offs = stdiff - arg->offs;
	}
	return stdiff;

}

//~ scoping
void enter(ccState s, astn ast) {
	//~ dieif(!s->_cnt, "FixMe: invalid ccState");
	s->nest += 1;

	/* using(type)
	if (with != NULL) {
		with = with->args;

		while (with) {
			installex(s, with->name, TYPE_def, 0, with, NULL);

			/ *
			int h = rehash(with->name, -1) % TBLS;

			with->uses = with->next;

			with->nest = s->nest;

			with->next = s->deft[h];
			s->deft[h] = with;

			// * /
			with = with->next;
		}
	}// */
}
symn leave(ccState s, symn dcl) {
	symn arg = NULL;
	int i;

	s->nest -= 1;

	// clear from table
	for (i = 0; i < TBLS; i++) {
		symn def = s->deft[i];
		while (def && def->nest > s->nest) {
			symn tmp = def;
			def = def->next;
			tmp->next = NULL;
			//~ tmp->next = tmp->uses;
		}
		s->deft[i] = def;
	}

	// clear from stack
	while (s->defs && s->nest < s->defs->nest) {
		symn sym = s->defs;

		//~ debug("%d:%?T.%+T", s->nest, dcl, sym);
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
