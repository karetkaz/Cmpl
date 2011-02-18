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
symn type_str = NULL;
symn type_ptr = NULL;
symn null_ref = NULL;

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
//~ symn installex(ccState s, const char* name, int kind, symn type, int cast, unsigned size, astn init) {
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

		//~ def->cast = cast;

		if (kind & symn_call)
			def->call = 1;

		if (kind & symn_stat)
			def->stat = 1;

		if (kind & symn_read)
			def->read = 1;

		switch (kind & 0xff) {
			default:
				fatal("FixMe (%+k)", init);
				return NULL;

			// usertype
			case TYPE_def:
			case TYPE_rec:
				//~ if (def->cast)
				def->pack = size;

			// variable
			case TYPE_ref:
				break;
			case EMIT_opc:
				def->size = 0;
				def->offs = size;
				break;
		}

		hash = rehash(name, -1) % TBLS;
		def->next = s->deft[hash];
		s->deft[hash] = def;

		def->defs = s->s->defs;
		s->s->defs = def;
	}
	return def;
}

symn install(ccState s, const char* name, int kind, int cast, unsigned size) {
	symn ref = installex(s, name, kind, size, NULL, NULL);
	if (ref != NULL) {
		ref->cast = cast;
	}
	return ref;
}

symn installtyp(state s, const char* name, unsigned size) {
	//~ type_i64 = install(cc, "int64", TYPE_rec | symn_read, TYPE_i64, 8);
	return install(s->cc, name, TYPE_rec | symn_read, TYPE_rec, size);
}

/*symn installvar(state s, const char* name, symn type, unsigned offset) {
	//~ return installex2(s->cc, name, TYPE_ref | symn_read, offset, type, NULL);
	symn ref = installex(s->cc, name, TYPE_ref | symn_read, type->size, type, 0);
	if (ref != NULL) {
		ref->offs = offset;
	}
	return ref;
}// */

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

int check(astn arg, symn ref) {
	//~ symn typ = ref->type;

	dieif(!ref, "FixMe");
	dieif(!ref->type, "FixMe");

	if (ref->call) {	//
		symn arg1 = ref->args;
		symn arg2 = arg->id.link->args;
		debug("function byref: %+k", arg);
		//~ debug("%-T / %-T", arg->id.link, ref);

		if (arg->type != ref->type) {
			debug("%+k: %+T", arg, ref);
			return 0;
		}

		while (arg1 && arg2) {
			if (arg1->type != arg2->type) {
				debug("FixMe");
				return 0;
			}
			if (arg1->cast != arg2->cast) {
				debug("FixMe");
				return 0;
			}
			arg1 = arg1->next;
			arg2 = arg2->next;
		}

		if (arg1 || arg2) {
			debug("FixMe");
			return 0;
		}
	}

	//~ debug("%-T == %-T", arg->type, ref->type);
	return 1;
}

symn lookup(ccState s, symn sym, astn ref, astn args) {
	symn asref = 0;
	symn best = 0;
	int found = 0;

	dieif(/* !sym ||  */!ref || ref->kind != TYPE_ref, "FixMe");

	for (; sym; sym = sym->next) {
		int hascast = 0;
		astn argval = args;				// arg values
		symn argsym = sym->args;		// arg symbols

		// array types dont have names.
		if (!sym->name)
			continue;

		// check name
		if (strcmp(sym->name, ref->id.name) != 0)
			continue;

		if (!argval && sym->call && sym->kind == TYPE_ref) {
			//~ debug("%k lookup as ref ?", ref);
			asref = sym;
		}

		while (argval && argsym) {
			symn typ = argsym->type;

			if (argval->kind == TYPE_ref && argval->id.link == null_ref) {
				if (argsym->cast == TYPE_ref || argsym->cast == TYPE_def) {
					typ = argval->type;
				}
			}

			/*if (argsym->cast == TYPE_ref) {
				//~ debug("byref: %k", argv);
				if (argval->kind == TYPE_ref
				 && argval->id.link == null_ref)
				//~ argval->type = typ;
				//~ argval->cst2 = TYPE_ref;
			}

			/ *if (argsym->cast == TYPE_def) {
				//~ debug("as is: %k", argv);
				if (argval->kind == TYPE_ref
				 && argval->id.link == null_ref)
				argval->type = typ;
				argval->cst2 = TYPE_ref;
			}// */

			if (typ == argval->type || promote(typ, argval->type)) {
				hascast += typ != argval->type;
				argval = argval->next;
				argsym = argsym->next;
				continue;
			}
			break;
		}

		/*if (args) {
			debug("%T:(%+k, %-T)", sym, argval, argsym);
			debug("%T:(%-T, %-T)", sym, argval->type, argsym->type);
		}// */

		if (sym->call && (argval || argsym)) {
			continue;
		}

		// rerfect match
		if (hascast == 0)
			break;

		// keep first match
		if (!best)
			best = sym;

		found += 1;
		// if we are here then sym is found, but it has implicit cast in it
		//~ debug("%+k is probably %-T", ref, sym);
	}

	//~ if (args) debug("%T: %+k(%+k)", sym, ref, args);

	if (sym == NULL && best) {
		if (found > 1)
			warn(s->s, 2, s->file, ref->line, "using overload `%-T` of %d", best, found);
		sym = best;
	}

	//~ /*
	if (sym == NULL && asref) {
		//~ warn(s->s, 3, s->file, ref->line, "using as reference `%-T`", asref, found);
		sym = asref;
	}// */

	if (sym != NULL) {
		astn argval = args;
		symn argsym = sym->args;

		while (argsym && argval) {

			//~ if (argsym->cast == TYPE_ref)
				//~ argval->cst2 = TYPE_ref;

			if (argval->kind == TYPE_ref && argval->id.link == null_ref) {
				if (argsym->cast != TYPE_ref && argsym->cast != TYPE_def) {
					debug("%k", argval);
					return 0;
				}
			}
			else if (!check(argval, argsym)) {
				debug("%k", argval);
				return 0;
			}

			argval = argval->next;
			argsym = argsym->next;
		}

		if (args && (argval || argsym)) {
			if (!isType(sym)) {
				debug("%k", ref);
				return NULL;
			}
		}

		if (sym->kind == TYPE_def && !sym->init) {
			sym = sym->type;
		}

	}

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

	if (def != NULL) {

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
		//~ def->cast = typ ? typ->cast : TYPE_any;
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
	//~ debug("%T is not a type", sym);
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

	//~ debug("%t(%+k):(%d)", ast->kind, ast, ast->line);
	return 0;
}// */

symn linkOf(astn ast) {
	if (!ast) return 0;

	if (ast->kind == OPER_fnc)
		return linkOf(ast->op.lhso);

	// TODO: static variables ?
	if (ast->kind == OPER_dot)		// i32.mad
		return linkOf(ast->op.rhso);
		//~ return njr ? linkOf(ast->op.rhso, njr) : NULL;

	// get base
	if (ast->kind == OPER_idx)
		return linkOf(ast->op.lhso);
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

		case TYPE_ref:
			if (typ->cast == TYPE_ref)
				return 4;
			return sizeOf(typ->type);
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
	//~ debug("cast `%+k` to (%t)", ast, cast);
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
	symn result = NULL;
	astn dot = NULL;
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
						if (!(arg->kind == OPER_fnc && isType(linkOf(arg->op.lhso)))) {
							debug("%T", linkOf(arg->op.lhso));
							warn(cc->s, 2, cc->file, arg->line, "emit type cast expected: '%+k'", arg);
						}
					}
					if (lin)
						arg->cst2 = arg->type->cast;
					args->op.rhso->next = next;
					next = args->op.rhso;
					args = args->op.lhso;
				}
				if (!typecheck(cc, lin, args)) {
					if (!lin || !typecheck(cc, 0, args)) {
						debug("arg(%+k)", args);
						return 0;
					}
					if (!istype(args) && !(args->kind == OPER_fnc && isType(linkOf(args)))) {
						debug("%T", linkOf(args));
						warn(cc->s, 2, cc->file, args->line, "emit type cast expected: '%+k'", args);
					}
				}
				if (lin)
					args->cst2 = args->type->cast;
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

			if (args == NULL) {
				//~ debug("FixMe");
				args = cc->argz;
			}

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
			int cast = 0;
			symn lht = typecheck(cc, loc, ast->op.lhso);
			symn rht = typecheck(cc, loc, ast->op.rhso);

			if (!lht || !rht || loc) {
				debug("cast(%T, %T)", lht, rht);
				return NULL;
			}
			//~ if (lht->cast && rht->cast) {
			if (lht == rht && lht == type_ptr) {
				// TODO: here should be a nother check
				if (ast->kind == OPER_equ || ast->kind == OPER_neq) {
					cast = TYPE_ref;
				}
			}
			if (cast || (cast = castOf(promote(lht, rht)))) {
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

			fatal("operator %k (%-T %-T): %+k", ast, lht, rht, ast);
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
				/*case TYPE_int: {
					int is32 = ast->con.cint != (int32_t)ast->con.cint;
					return castTy(ast, is32 ? type_i32 : type_i64);
				}*/
				case TYPE_int: return castTy(ast, type_i32) ? type_i32 : NULL;
				case TYPE_flt: return castTy(ast, type_f64) ? type_f64 : NULL;
				case TYPE_str: return castTy(ast, type_str) ? type_str : NULL;
				case EMIT_opc: return ast->type = emit_opc;
			}
		} break;
	}

	if (ref != NULL) {
		sym = loc ? loc->args : cc->deft[ref->id.hash];

		if ((sym = lookup(cc, sym, ref, args))) {

			//~ debug("%k", ref);
			// using function args

			if (sym) switch (sym->kind) {
				default: fatal("FixMe");

				case TYPE_def:
					result = sym->type;
					//~ debug("%T:%T in `%+k` (%d)", sym, result, ast, ast->line);
					break;

				case EMIT_opc:
				case TYPE_ref:
					result = sym->type;
					break;

				//~ case TYPE_arr:
				case TYPE_rec:
					result = sym;
					break;
			}

			if (isType(sym) && args && !args->next) {			// cast
				if (!castTo(args, sym->cast)) {
					debug("%k:%t %+k", args, castOf(args->type), args);
					return 0;
				}//*/
			}

			if (sym->call) {
				astn argval = args;
				symn argsym = sym->args;

				while (argsym && argval) {
					if (!castTy(argval, argval->type)) {
						debug("%k:%t %+k", argval, castOf(argval->type), argval);
						return 0;
					}

					if (argsym->cast == TYPE_ref || argval->type->cast == TYPE_ref) {
						if (!castTo(argval, TYPE_ref)) {
							debug("%k:%t", argval, TYPE_ref);
							return 0;
						}
					}

					//~ debug("cast Arg(%+k): %t", args, args->cst2);

					argsym = argsym->next;
					argval = argval->next;
				}
			}

			ref->kind = TYPE_ref;
			ref->id.link = sym;
			ref->type = result;
			ast->type = result;

			if (dot)
				dot->type = result;

			if (sym->call && !args) {
				result = type_ptr;
			}

			//~ if (typ != sym)
				//~ sym->cast = typ->cast;

			//~ if (sym->line) debug("%k: %-T: %-T", ref, sym, result);
		}
		else
			cc->root = ref;
	}

	//~ if (!result) debug("%k in %T", ref, loc);

	return result;
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
		arg->size = sizeOf(arg);
		//~ arg->size = arg->type->size;
		arg->offs = stbeg + stdiff;
		stdiff += padded(arg->size, align);
		//~ debug("sizeof(%-T):%d", arg, arg->offs);
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
	(void)ast;
}
symn leave(ccState s, symn dcl) {
	state rt = s->s;
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
	while (rt->defs && s->nest < rt->defs->nest) {
		symn sym = rt->defs;

		//~ debug("%d:%?T.%+T", s->nest, dcl, sym);
		sym->decl = dcl;

		rt->defs = sym->defs;
		sym->defs = s->all;
		s->all = sym;

		sym->next = arg;
		arg = sym;
	}

	return arg;
}

// }
