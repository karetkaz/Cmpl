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

	// declare
	if (kind == -1)
		return instlibc(s, name);

	switch (kind & 0xff) {
		default: fatal("NoIpHere");

		// basetype
		case TYPE_vid:
		case TYPE_bit:
		case TYPE_int:
		case TYPE_flt:
		case TYPE_p4x:
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

	return installex(s, kind, name, size, typ, 0);
}

// TODO: this is define, not declare
symn declare(ccEnv s, int kind, astn tag, symn typ) {
	symn def;

	if (!tag || tag->kind != TYPE_ref) {
		debug("declare: astn('%k') is null or not an identifyer", tag);
		return 0;
	}

	def = installex(s, kind, tag->id.name, 0, typ, NULL);
	if (!def) return NULL;

	def->line = tag->line;
	def->file = s->file;

	tag->type = typ;
	tag->id.link = def;

	tag->kind = TYPE_def;
	switch (kind) {
		default: fatal("NoIpHere");

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
		case TYPE_p4x:
		case TYPE_ar3:

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

symn linkTo(astn ast, symn sym) {
	symn typ = NULL;
	if (!ast) return 0;
	switch (sym->kind) {
		default: fatal("NoIpHere");
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

	// TODO: in case of static variables
	if (ast->kind == OPER_dot)
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
		case TYPE_rec: return 0;
	}
	debug("failed(%t, %t)", lht, rht);
	return 0;
}
int castId(symn typ) {
	if (typ) switch (typ->kind) {
		default: fatal("NoIpHere");
		case TYPE_vid: return TYPE_vid;
		case TYPE_bit: switch (typ->size) {
			default: fatal("NoIpHere");
			case 1: case 2:
			case 4: return TYPE_u32;
			//~ case 8: return TYPE_u64;
		} break;
		case TYPE_int: switch (typ->size) {
			default: fatal("NoIpHere");
			case 1: case 2:
			case 4: return TYPE_i32;
			case 8: return TYPE_i64;
		} break;
		case TYPE_flt: switch (typ->size) {
			default: fatal("NoIpHere");
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
int castOp(symn lht, symn rht, int uns) {
	return castTT(castId(lht), castId(rht), uns);
}
int castTy(astn ast, int cast) {
	if (ast->type) {
		//~ debug("ast->type:%t(%T)", cast, ast->type);
		switch (cast) {
			case TYPE_bit: cast = TYPE_u32; break;
			case TYPE_vid: return ast->cast = cast;
		}
		return ast->cast = castTT(castId(ast->type), cast, 0);
	}
	else switch (ast->cast = cast) {
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

int castTo(astn ast, symn type) {
	if (ast->type) {
		ast->type = type;
		//~ debug("ast->type");
		return ast->cast = castTT(castId(ast->type), castId(type), 0);
	}
	else switch (ast->cast = castId(type)) {		// ????
		case TYPE_u32: ast->type = type_u32; break;

		case TYPE_i32: ast->type = type_i32; break;
		case TYPE_i64: ast->type = type_i64; break;

		case TYPE_f32: ast->type = type_f32; break;
		case TYPE_f64: ast->type = type_f64; break;

		default:
			debug("%T", type);
			return 0;
	}
	return ast->cast;
}

//~ TODO: with args == NULL can be a call
symn lookup(ccEnv s, symn sym, astn ref, astn args) {
	symn best = 0;
	int found = 0;

	dieif(!ref || ref->kind != TYPE_ref, "?");

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
			
			if (!typ || typ == arg->type || castOp(typ, arg->type, 0)) {
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

	if (sym) {
		symn par = sym->args;		// caller arguments
		astn arg = args;			// callee arguments
		linkTo(ref, sym);
		while (arg && par) {
			int cast = castId(par->type);
			if (arg->cast != cast) {
				if (!castTy(arg, cast))
					error(s->s, arg->line, "cannot set cast to %T", par->type);
			}
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
/*May Use: lookin
symn lookin(ccEnv s, symn loc, astn ref, astn args) {
	symn sym = NULL;
	while (loc) {
		sym = lookup(s, loc->args, ref, args);
		if (sym)
			break;
		loc = loc->type;
	}
	return sym;
}*/
symn typecheck(ccEnv s, symn loc, astn ast) {
	astn ref = 0, args = 0;
	symn sym = 0;

	if (!ast) {
		return 0;
	}

	switch (ast->kind) {
		default: fatal("NoIpHere: %t(%+k)", ast->kind, ast);

		case OPER_dot: {
			sym = typecheck(s, loc, ast->op.lhso);
			if (sym == NULL) {
				debug("lookup %+k in %T", ast->op.lhso, loc);
				return 0;
			}
			loc = ast->op.lhso->type;
			ref = ast->op.rhso;
		} break;
		case OPER_fnc: {
			astn fun = ast->op.lhso;

			if (fun == NULL) {
				ast->type = typecheck(s, 0, ast->op.rhso);
				ast->cast = castId(ast->type);
				return ast->type;
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
				case EMIT_opc: {
					if (!args)
						return ast->type = type_vid;
					//~ args->cast = TYPE_vid;
					args->cast = castId(args->type);
					//~ error(s, ast->lhso->line, "emit must have ")
					return ast->type = args->type;
				} break;
				case OPER_dot: {	// math.isNan ???
					if (!typecheck(s, loc, ast->op.lhso->op.lhso)) {
						debug("%+k:%T", ast, loc);
						return 0;
					}
					loc = ast->op.lhso->op.lhso->type;
					ref = ast->op.lhso->op.rhso;
				} break;// */
				case TYPE_ref: ref = ast->op.lhso; break;
				default: fatal("NoIpHere: %t(%+k)", ast->kind, ast);
			}// */
			//~ if (!args)
				//~ args = type_vid;
		} break;
		case OPER_idx: {
			symn lht = typecheck(s, 0, ast->op.lhso);
			symn rht = typecheck(s, 0, ast->op.rhso);

			//~ ast->op.link = 0;
			if (!lht || !rht || args) {
				debug("cast(%T, %T): %T", lht, rht, 0);
				return NULL;
			}

			dieif(!lht->type, "%+k", ast);
			debug("TODO(%+k):sizeof(%+T)=%d", ast, lht->type, lht->type->size);
			return ast->type = lht->type;
			return 0;
		} break;

		case OPER_pls:		// '+'
		case OPER_mns: {	// '-'
			symn rht = typecheck(s, 0, ast->op.rhso);

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
			symn rht = typecheck(s, 0, ast->op.rhso);

			if (!rht) {
				debug("cast(%T)[%k]: %+k", rht, ast, ast);
				return NULL;
			}

			switch (castTy(ast, castId(rht))) {
				case 0: break;

				case TYPE_u32:
				case TYPE_i32:
				case TYPE_i64:
					return ast->type;

				case TYPE_f32:
				case TYPE_f64:
					error(s->s, ast->line, "invalid cast(%+k)", ast);
					return 0;
			}
			fatal("%k %T: %+k):%t", ast, rht, ast, castId(rht));
		} break;

		case OPER_not: {
			symn rht = typecheck(s, 0, ast->op.rhso);

			if (!rht) {
				debug("cast(%T)[%k]: %+k", rht, ast, ast);
				return NULL;
			}

			ast->type = type_bol;
			switch (ast->cast = castId(rht)) {
				case 0: break;
				case TYPE_u32:
				case TYPE_i32:
				case TYPE_i64:
				case TYPE_f32:
				case TYPE_f64:
					return ast->type;
			}
			fatal("%k %T: %+k):%t", ast, rht, ast, castId(rht));
		} break;

		case OPER_add:
		case OPER_sub:
		case OPER_mul:
		case OPER_div:
		case OPER_mod: {	// 'lhs op rhs' => op(lhs, rhs)
			symn lht = typecheck(s, 0, ast->op.lhso);
			symn rht = typecheck(s, 0, ast->op.rhso);

			if (!lht || !rht) {
				debug("cast(%T, %T)[%k]: %+k", lht, rht, ast, ast);
				return NULL;
			}

			//~ see if both sides are basic types
			//TODO: if (ast->op.rhso->cast && ast->op.lhso->cast) ;

			if (!castTy(ast, castOp(lht, rht, 0))) {
				/*
				
				astn lhs = ast->op.lhso;
				astn rhs = ast->op.rhso;
				astn call = newnode(s, TYPE_ref);
				astn arg = newnode(s, OPER_com);
				call->id.name = (char*)tok_tbl[ast->kind].name;

				arg->op.lhso = lhs;
				arg->op.rhso = rhs;

				ast->kind = OPER_fnc;
				ast->op.lhso = call;
				ast->op.rhso = arg;
				ref = call;
				//~ */
				fatal("%T %k %T: %+k):%t", lht, ast, rht, ast, castOp(lht, rht, 0));
				//~ return 0;

				// 'lhs + rhs' => tok_name[OPER_add] '(lhs, rhs)'

				//~ args = ast->lhso;
				//~ ast->rhso->next = 0;
				//~ args->next = ast->rhso;
				//~ fun = tok_tbl[ast->kind].name;
				//~ ref = ast;
			}

			//~ fatal("(%+k):%t", ast, ast->cast);

		} break;

		case OPER_shl:		// '>>'
		case OPER_shr:		// '<<'
		case OPER_and:		// '&'
		case OPER_ior:		// '|'
		case OPER_xor: {	// '^'
			symn lht = typecheck(s, 0, ast->op.lhso);
			symn rht = typecheck(s, 0, ast->op.rhso);

			if (!lht || !rht) {
				debug("cast(%T, %T): %T", lht, rht, 0);
				return NULL;
			}
			switch (castTy(ast, castOp(lht, rht, ast->kind == OPER_shl))) {
				case 0: break;

				case TYPE_u32:
				case TYPE_i32:
				case TYPE_i64:
					return ast->type;

				case TYPE_f32:
				case TYPE_f64:
					error(s->s, ast->line, "invalid cast(%+k)", ast);
					return 0;
			}
			fatal("(%+k):%t", ast, ast->cast);
		} break;

		case OPER_equ:		// '=='
		case OPER_neq:		// '!='
		case OPER_lte:		// '<'
		case OPER_leq:		// '<='
		case OPER_gte:		// '>'
		case OPER_geq: {	// '>='
			symn lht = typecheck(s, 0, ast->op.lhso);
			symn rht = typecheck(s, 0, ast->op.rhso);

			if (!lht || !rht) {
				debug("cast(%T, %T)", lht, rht);
				return NULL;
			}

			ast->type = type_bol;
			switch (ast->cast = castOp(lht, rht, 1)) {
				case 0: break;
				case TYPE_u32:
				case TYPE_i32:
				case TYPE_i64:
				case TYPE_f32:
				case TYPE_f64:
					return ast->type;
			}
			fatal("(%+k):%t", ast, ast->cast);
		} break;

		case OPER_lor:
		case OPER_lnd: {
			symn lht = typecheck(s, 0, ast->op.lhso);
			symn rht = typecheck(s, 0, ast->op.rhso);

			if (!lht || !rht) {
				debug("cast(%T, %T)", lht, rht);
				return NULL;
			}

			ast->cast = TYPE_bit;
			ast->type = type_bol;
		} break;

		case OPER_sel: {
			symn cmp = typecheck(s, 0, ast->op.test);
			symn lht = typecheck(s, 0, ast->op.lhso);
			symn rht = typecheck(s, 0, ast->op.rhso);

			if (!cmp || !lht || !rht) {
				debug("cast(%T, %T)[%k]", lht, rht, ast);
				return NULL;
			}

			if (!castTy(ast, castOp(lht, rht, 0))) {
				fatal("%T %k %T: %+k):%t", lht, ast, rht, ast, castOp(lht, rht, 0));
			}
		} break;

		case ASGN_set: {	// ':='
			symn lht = typecheck(s, 0, ast->op.lhso);
			symn rht = typecheck(s, 0, ast->op.rhso);

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

		case TYPE_int:
		case TYPE_flt:
		case CNST_str: {
			if (loc || args) {
				debug("cast()[]");
				return NULL;
			}
			switch (ast->kind) {
				case TYPE_int: ast->type = type_i32; break;
				case TYPE_flt: ast->type = type_f64; break;
				case CNST_str: ast->type = type_str; break;
			}
		} break;

		case TYPE_ref: ref = ast; break;
		//~ case EMIT_opc: return 0;
	}

	if (ref != NULL) {
		sym = loc ? loc->args : s->deft[ref->id.hash];

		//~ debug("lookup(sym, %+k, %+k):%T", ref, args, sym);
		sym = lookup(s, sym, ref, args);

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

//~ scoping TODO: enter(ccEnv s, astn def)
void enter(ccEnv s, symn def) {
	dieif(!s->_cnt, "invalid ccEnv");
	s->nest += 1;
	//~ debug("enter(%d, %?T)", s->nest, def);
	//~ s->scope[s->nest].csym = def;
	//~ s->scope[s->nest].stmt = NULL;
}

symn leave(ccEnv s, symn dcl) {
	int i;
	symn arg = 0;
	dieif(s->_cnt <= 0, "invalid ccEnv");
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
