/*******************************************************************************
 *   File: type.c
 *   Date: 2011/06/23
 *   Desc: type system
 *******************************************************************************

Types are special kind of variables.


Basic types

	void

	bool

	int8
	int16
	int32
	int64

	uint8
	uint16
	uint32

	float32
	float64

	pointer
	typename		// compilers internal type reprezentation structure
	?function

#typedefs
	@int: alias for int32
	@long: alias for int64
	@float: alias for float32
	@double: alias for float64

	@char: alias for uint8 / uint16
	@string: alias for char[]

	@var: variant
	@array: variant[]

#constants
	@true: emit(bool, i32(1));
	@false: emit(bool, i32(0));
	@null: emit(pointer, i32(0));

Derived data types:
	slice: struct {const pointer data; const int length;}
	[TODO] variant: struct &variant {const typename type; const byte data[0];}
	[TODO] delegate: struct {const pointer function; const pointer data;}

User defined types:
	pointers arrays and slices:
		TODO: pointers are unsized
			ex: int *a;
			are passed by reference,
		arrays are fixed-size:
			ex: int a[2];
			are passed by reference,
		slices are Dinamic-size arrays:
			ex: int a[];
			is a combination of pointer and length.
			where type of data is known by the compiler, the length by runtime.

	struct:
		when declaring a struct there will be declared the folowing initializer:
			with all members, in case packing is default,
				??? and fixed size arrays are not contained by the structure.
			from pointer: static cast
			from variant:
		ex: for struct Complex {double re; double im};
		will be defined:
			define Complex(double re, double im) = emit(Complex, double(re), double(im));
			define Complex(pointer ptr) = emit(Complex&, ref(ptr));
			define Complex(variant var) = emit(Complex&, ref(var.type == Complex ? &var.data : null));

	function

TODO's:
	struct initialization:
		struct alma {
			int32 a;
			int32 b;
			int64 x;
		}
		alma a1 = {a: 12, x: 88};
		alma a2 = alma(12, 0, 88);
	array initialization:
		alma a1[] = [alma(1,2,3), alma(2,2,3), ...]
		alma a1[] = [alma(1,2,3), alma(2,2,3), ...]
		alma a1[] = alma[]{alma(1,2,3), ...}

*******************************************************************************/

#include "core.h"
#include <string.h>

//~ symn null_ref = NULL;
//~ symn emit_opc = NULL;

/// allocate a symbol
symn newdefn(ccState s, int kind) {
	state rt = s->s;
	symn def = NULL;

	if (rt->_end - rt->_beg > (int)sizeof(struct symn)) {
		def = (symn)rt->_beg;
		rt->_beg += sizeof(struct symn);
		memset(def, 0, sizeof(struct symn));
		def->kind = kind;
	}
	else {
		fatal("memory overrun");
	}

	return def;
}

/// install a symbol(type or variable)
symn install(ccState s, const char* name, int kind, int cast, unsigned size, symn type, astn init) {
	unsigned hash = 0;
	symn def;

	dieif(!s || !name || !kind, "FixMe(s, %s, %t)", name, kind);

	if ((def = newdefn(s, kind & 0xff))) {
		def->nest = s->nest;
		def->name = (char*)name;
		def->type = type;
		def->init = init;
		def->size = size;
		def->cast = cast;

		if (init && !init->type) {
			debug("FixMe '%s'", name); // null, true, false
			init->type = type;
		}

		if (kind & ATTR_stat)
			def->stat = 1;

		if (kind & ATTR_const)
			def->cnst = 1;

		switch (kind & 0xff) {
			default:
				fatal("FixMe (%t)", kind& 0xff);
				return NULL;

			// user type
			//~ case TYPE_ptr:
			case TYPE_arr:
			case TYPE_rec:
				def->offs = -vmOffset(s->s, def);
				//~ def->pack = size;
				break;

			// variable
			case TYPE_def:
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

symn install_typ(state rt, const char* name, unsigned size, int refType) {
	//~ dieif(!rt->cc, "FixMe");
	return install(rt->cc, name, ATTR_const | TYPE_rec, refType ? TYPE_ref : TYPE_rec, size, rt->cc->type_rec, NULL);
}

/// used to add length property to arrays.
symn addarg(ccState s, symn sym, const char* name, int kind, symn typ, astn init) {
	symn args = sym->args;

	enter(s, NULL);
	install(s, name, kind, 0, 0, typ, init);
	sym->args = leave(s, sym, 0);

	// non static member
	if (sym->args) {
		sym->args->next = args;
	}
	else {
		sym->args = args;
	}
	return sym->args;
}

// promote
static inline int castkind(int cast) {
	switch (cast) {
		case TYPE_vid:
			return TYPE_vid;
		case TYPE_bit:
			return TYPE_bit;
		case TYPE_u32:
		case TYPE_i32:
		case TYPE_i64:
			return TYPE_int;
		case TYPE_f32:
		case TYPE_f64:
			return TYPE_flt;
		case TYPE_ref:
			return TYPE_ref;
	}
	return 0;
}
static symn promote(symn lht, symn rht) {
	symn pro = 0;
	if (lht && rht) {
		if (lht == rht) {
			pro = lht;
		}
		else switch (castkind(rht->cast)) {
			case TYPE_bit:
			case TYPE_int: switch (castkind(lht->cast)) {
				case TYPE_bit:
				case TYPE_int:
					TODO("bool + int is bool; if sizeof(bool) == 4")
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

int canAssign(ccState cc, symn var, astn val, int strict) {
	symn typ = var;
	symn lnk = linkOf(val);

	dieif(!var, "FixMe");
	dieif(!val, "FixMe");

	// assigning null or pass by reference
	if (val->kind == TYPE_ref && val->ref.link == cc->null_ref) {
		// if parameter is byRef or type is byRef
		if (var->cast == TYPE_ref || typ->cast == TYPE_ref) {
			trace("canAssign null to: %-T", var);
			return 1;
		}
	}

	// assigning typename or pass by reference
	if (lnk && lnk->kind == TYPE_rec) {
		if (var->type == val->type->type)
			return 1;
	}

	if (var->kind == TYPE_ref) {

		typ = var->type;

		// assigning a function
		if (var->call) {
			symn fun = linkOf(val);
			symn arg1 = var->args;
			symn arg2 = NULL;
			struct astn atag;

			atag.kind = TYPE_ref;
			atag.type = typ;
			atag.cst2 = var->cast;
			atag.ref.link = var;

			if (fun && canAssign(cc, fun->type, &atag, 1)) {
				arg2 = fun->args;
				while (arg1 && arg2) {

					atag.type = arg2->type;
					atag.cst2 = arg2->cast;
					atag.ref.link = arg2;

					if (!canAssign(cc, arg1, &atag, 1)) {
						trace("%-T != %-T", arg1, arg2);
						break;
					}

					arg1 = arg1->next;
					arg2 = arg2->next;
				}
			}
			else {
				trace("%-T != %-T", typ, fun);
				return 0;
			}
			if (arg1 || arg2) {
				trace("%-T != %-T", arg1, arg2);
				return 0;
			}
			return 1;
		}
		else if (!strict) {
			strict = var->cast == TYPE_ref;
		}
		if (var->cast == TYPE_ref && val->type == cc->type_ptr)
			return 1;
	}

	if (typ == val->type)
		return 1;

	if (typ->kind == TYPE_arr) {
		symn vty = val->type;
		struct astn atag;
		atag.kind = TYPE_ref;
		atag.type = vty ? vty->type : NULL;
		atag.cst2 = atag.type ? atag.type->cast : 0;
		atag.ref.link = NULL;
		atag.ref.name = ".generated token";

		if (canAssign(cc, typ->type, &atag, strict)) {
			// assign to dynamic array
			if (typ->size == -1) {
				return 1;
			}
			if (typ->size == val->type->size) {
				return 1;
			}
		}

		//~ trace("assign `%k` to `%-T`(%t)", val, var, var->type->cast);
	}

	if (!strict && promote(typ, val->type))
		return 1;

	//~ /*TODO(hex32 can be passed as int32 by ref)
	if (val->type && typ->cast == val->type->cast) {
		if (typ->size == val->type->size) {
			switch (typ->cast) {
				default:break;
				case TYPE_u32:
				case TYPE_i32:
				case TYPE_i64:
				case TYPE_f32:
				case TYPE_f64:
					return 1;
			}
		}
	}

	trace("can not assign `%+k` to `%-T`(%t)", val, var, typ->cast);
	return 0;
}

TODO("!!! args are linked in a list by next !!??")
symn lookup(ccState s, symn sym, astn ref, astn args, int raise) {
	symn asref = 0;
	symn best = 0;
	int found = 0;

	dieif(!ref || ref->kind != TYPE_ref, "FixMe");

	// linearize args
	/*if (args && args->kind == OPER_com) {
		astn next = NULL;

		//~ if (args->kind == OPER_com)
			//~ debug("lookup: %k(%+k)", ref, args);

		while (args->kind == OPER_com) {
			args->op.rhso->next = next;
			next = args->op.rhso;
			args = args->op.lhso;
		}
		args->next = next;
	}// */

	for (; sym; sym = sym->next) {
		int hascast = 0;
		astn argval = args;				// arg values
		symn argsym = sym->args;		// arg symbols

		// there are nameless symbols: functions, array types.
		if (!sym->name)
			continue;

		// check name
		if (strcmp(sym->name, ref->ref.name) != 0)
			continue;

		if (!args && sym->call) {
			// keep the first match.
			if (sym->kind == TYPE_ref) {
				if (asref == NULL)
					asref = sym;
				found += 1;
			}
			continue;
		}

		if (args) {
			if (!sym->call) {
				TODO("enable basic type casts: float32(1)")
				int isBasicCast = 0;

				symn sym2 = sym;
				while (sym2) {
					if (sym2->kind != TYPE_def)
						break;
					if (sym2->init)
						break;
					sym2 = sym2->type;
				}

				if (!args->next && isType(sym2)) {
					switch(sym2->cast) {
						default:break;
						case TYPE_rec:
						case TYPE_ref:
						case TYPE_vid:
						case TYPE_bit:
						case TYPE_u32:
						case TYPE_i32:
						case TYPE_i64:
						case TYPE_f32:
						case TYPE_f64:
							isBasicCast = 1;
							break;
					}
				}

				if (!isBasicCast)
					continue;
			}

			while (argval && argsym) {

				if (!canAssign(s, argsym, argval, 0))
					break;

				//~ debug("%+k%s is probably %-T%s:%t", ref, args ? "()" : "", sym, sym->call ? "()" : "", sym->kind);

				// if null is passed by ref it will be as a cast
				if (argval->kind == TYPE_ref && argval->ref.link == s->null_ref) {
					hascast += 1;
				}

				else if (!canAssign(s, argsym, argval, 1)) {
					hascast += 1;
				}

				// TODO: hascast += argval->cst2 != 0;

				argval = argval->next;
				argsym = argsym->next;
			}

			if (sym->call && (argval || argsym)) {
				//~ debug("%-T(%+k, %-T)", sym, argval, argsym);
				continue;
			}
		}

		dieif(s->func && s->func->nest != s->maxlevel - 1, "FIXME %d, %d", s->func->nest, s->maxlevel);
		// TODO: sym->decl && sym->decl->call && sym->decl != s->func
		if (s->func && !s->siff && s->func->gdef && sym->nest && !sym->stat && sym->nest < s->maxlevel) {
			error(s->s, ref->file, ref->line, "invalid use of local symbol `%k`.", ref);
		}

		// rerfect match
		if (hascast == 0)
			break;

		// keep first match
		if (!best)
			best = sym;

		found += 1;
		// if we are here then sym is found, but it has implicit cast in it
		trace("%+k%s is probably %-T%s:%t", ref, args ? "()" : "", sym, sym->call ? "()" : "", sym->kind);
	}

	if (sym == NULL && best) {
		if (found > 1)
			warn(s->s, 2, ref->file, ref->line, "using overload `%-T` of %d", best, found);
		sym = best;
	}

	if (sym == NULL && asref) {
		if (found == 1 || s->siff) {
			trace("as ref `%-T`(%?+k)(%d)", asref, args, ref->line);
			sym = asref;
		}
		else if (raise) {
			error(s->s, ref->file, ref->line, "there are %d overloads for `%T`", found, asref);
		}
	}

	if (sym != NULL) {
		if (sym->kind == TYPE_def && !sym->init) {
			sym = sym->type;
		}
		if (sym->used != ref) {
			// in emit we can lookup 2x
			ref->ref.used = sym->used;
			sym->used = ref;
		}
	}

	return sym;
}

TODO("we should handle redefinition in this function")
symn declare(ccState s, int kind, astn tag, symn typ) {
	symn def;

	if (!tag || tag->kind != TYPE_ref) {
		trace("declare: astn('%k') is null or not an identifyer", tag);
		return 0;
	}

	def = install(s, tag->ref.name, kind, 0, 0, typ, NULL);

	if (def != NULL) {

		def->line = tag->line;
		def->file = s->file;

		tag->type = typ;
		tag->ref.link = def;

		tag->kind = TYPE_def;
		switch (kind) {
			default:
				fatal("FixMe");
				break;

			case TYPE_rec:			// typedefn struct
				if (typ != NULL)
					def->size = typ->size;
				break;

			case TYPE_def:			// typename
			case TYPE_ref:			// variable
				tag->kind = kind;
				break;
		}
		//~ /*
		if (typ) {
			def->cast = typ->cast;
		}// */
	}

	return def;
}

int isType(symn sym) {
	if (sym) switch (sym->kind) {
		default:break;
		case TYPE_arr:
		case TYPE_rec:
			return 1;

		case TYPE_def:
			return sym->init == NULL;
	}
	//~ trace("%T is not a type", sym);
	return 0;
}
int istype(astn ast) {
	if (!ast) return 0;

	if (ast->kind == EMIT_opc)
		return 0;

	if (ast->kind == OPER_dot)
		return istype(ast->op.lhso) && istype(ast->op.rhso);

	if (ast->kind == TYPE_ref)
		return isType(ast->ref.link);

	//~ trace("%t(%+k):(%d)", ast->kind, ast, ast->line);
	return 0;
}

extern symn emit_opc_;
symn linkOf(astn ast) {
	if (!ast) return 0;

	if (ast->kind == EMIT_opc)
		return emit_opc_;

	if (ast->kind == OPER_fnc)
		return linkOf(ast->op.lhso);

	if (ast->kind == OPER_dot)
		return linkOf(ast->op.rhso);

	if (ast->kind == OPER_idx)
		return linkOf(ast->op.lhso);

	if (ast->kind == TYPE_ref && ast->ref.link) {
		// skip type defs
		symn lnk = ast->ref.link;
		if (lnk->kind == TYPE_def && lnk->init->kind == TYPE_ref) {
			lnk = linkOf(lnk->init);
		}
		return lnk;
	}

	trace("%t(%+k)", ast->kind, ast);
	return NULL;
}

TODO("this should be calculated by fixargs() and replaced by (var|typ)->size")
long sizeOf(symn typ) {
	if (typ) switch (typ->kind) {
		default:
			break;
		//~ case TYPE_vid:
		//~ case TYPE_bit:
		//~ case TYPE_int:
		//~ case TYPE_flt:

		case TYPE_arr:
			if (typ->size < 0)
				return 8;
			return typ->size * sizeOf(typ->type);

		case EMIT_opc:
		case TYPE_rec:
			if (typ->cast == TYPE_ref)
				return vm_size;
			return typ->size;
		case TYPE_def:
		case TYPE_ref:
			if (typ->cast == TYPE_ref)
				return vm_size;
			return sizeOf(typ->type);
	}
	fatal("failed(%t): %-T", typ ? typ->kind : 0, typ);
	return 0;
}

/** Cast
 * returns one of (TYPE_bit, ref, u32, i32, i64, f32, f64)
**/
int castOf(symn typ) {
	if (typ) switch (typ->kind) {

		default:break;

		case TYPE_def:
			return castOf(typ->type);

		//~ case TYPE_vid:
			//~ return typ->kind;

		case TYPE_arr:
			// static sized arrays cast to pointer
			if (typ->size < 0)
				return TYPE_arr;
			return TYPE_ref;

		case EMIT_opc:
			return typ->cast;

		case TYPE_rec:
			// refFix
			if (typ->cast == TYPE_ref)
				return TYPE_ptr;
			return typ->cast;
	}
	debug("failed(%t): %?-T", typ ? typ->kind : 0, typ);
	return 0;
}
int castTo(astn ast, int cto) {
	int atc = 0;
	if (!ast) return 0;
	TODO("check validity / Remove function");

	atc = ast->type ? ast->type->cast : 0;
	if (cto != atc) switch (cto) {
		case TYPE_vid:		// void(true): can cast 2 to void !!!
		case TYPE_bit:
		case TYPE_u32:
		case TYPE_i32:
		case TYPE_i64:
		case TYPE_f32:
		case TYPE_f64: switch (atc) {
			//~ case TYPE_vid:
			case TYPE_bit:
			case TYPE_u32:
			case TYPE_i32:
			case TYPE_i64:
			case TYPE_f32:
			case TYPE_f64:
				break;

			default:
				goto error;
		} break;
		case TYPE_ref:
		case TYPE_arr:
			break;
		default:
		error:
			trace("cast(%+k) to %t/%t", ast, cto, atc);
			break;
			//~ return 0;
	}
	//~ debug("cast `%+k` to (%t)", ast, cto);
	return ast->cst2 = cto;
}
static int typeTo(astn ast, symn type) {
	if (!ast) return 0;

	TODO("check validity / Remove function");

	while (ast->kind == OPER_com) {
		if (!typeTo(ast->op.rhso, type)) {
			trace("%k", ast);
			return 0;
		}
		ast = ast->op.lhso;
	}

	return castTo(ast, castOf(ast->type = type));
}

symn typecheck(ccState s, symn loc, astn ast) {
	astn ref = 0, args = 0;
	symn result = NULL;
	astn dot = NULL;
	symn sym = 0;

	dieif (!ast, "FixMe");

	ast->cst2 = 0;

	switch (ast->kind) {
		default:
			fatal("FixMe: %t(%+k)", ast->kind, ast);
			break;

		case OPER_fnc: {
			astn fun = ast->op.lhso;
			args = ast->op.rhso;

			if (fun == NULL) {
				symn rht = typecheck(s, NULL, ast->op.rhso);
				if (!rht || !castTo(ast->op.rhso, castOf(rht))) {
					debug("%T('%k', %+k): %t", rht, ast, ast, castOf(rht));
					return 0;
				}
				return ast->type = rht;
			}
			if (args != NULL) {
				astn linearize = NULL;
				symn lin = NULL;

				if (fun->kind == EMIT_opc) {
					lin = s->emit_opc;
				}

				while (args->kind == OPER_com) {
					astn arg = args->op.rhso;
					if (!typecheck(s, lin, arg)) {
						if (!lin || !typecheck(s, NULL, arg)) {
							trace("arg(%+k)", arg);
							return NULL;
						}
						if (!(arg->kind == OPER_fnc && isType(linkOf(arg->op.lhso)))) {
							if (arg->type->cast == TYPE_ref) {
								warn(s->s, 2, arg->file, arg->line, "argument `%+k` is passed by reference", arg);
							}
							else {
								warn(s->s, 2, arg->file, arg->line, "argument `%+k` is passed by value: %-T", arg, arg->type);
							}
							//~ warn(s->s, 2, arg->file, arg->line, "emit type cast expected: '%+k'", arg);
						}
					}

					args->op.rhso->next = linearize;
					linearize = args->op.rhso;
					args = args->op.lhso;
				}

				if (lin && args && args->kind == TYPE_rec) {
					//emit's first arg is 'struct'
					args->kind = TYPE_ref;
					args->type = s->emit_opc;
					args->ref.link = s->emit_opc;
				}
				else if (!typecheck(s, lin, args)) {
					if (!lin || !typecheck(s, NULL, args)) {
						trace("arg(%+k)", args);
						return NULL;
					}
					// emit's first arg can be a type (static cast)
					if (!istype(args) && !(args->kind == OPER_fnc && isType(linkOf(args)))) {
						if (args->type->cast == TYPE_ref) {
							warn(s->s, 2, args->file, args->line, "argument `%+k` is passed by reference", args);
						}
						else {
							warn(s->s, 2, args->file, args->line, "argument `%+k` is passed by value: %-T", args, args->type);
						}
						//~ warn(s->s, 2, args->file, args->line, "emit type cast expected: '%+k'", args);
					}
				}

				args->next = linearize;
			}

			if (fun) switch (fun->kind) {
				case OPER_dot: {	// math.isNan ???
					astn call = ast->op.lhso;
					if (!(loc = typecheck(s, loc, call->op.lhso))) {
						debug("%+k:%T", ast, loc);
						return 0;
					}
					dot = call;
					//~ loc = call->op.lhso->type;
					ref = call->op.rhso;
				} break;
				case EMIT_opc: {
					astn arg = args;
					for (arg = args; arg; arg = arg->next)
						arg->cst2 = arg->type->cast;
					return ast->type = args ? args->type : NULL;
				} break;
				case TYPE_ref:
					ref = ast->op.lhso;
					break;
				default:
					fatal("FixMe: %t(%+k)", ast->kind, ast);
			}

			if (args == NULL) {
				args = s->void_tag;
			}

		} break;
		case OPER_dot: {
			//~ debug("%+k", ast);
			sym = typecheck(s, loc, ast->op.lhso);
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
			symn lht = typecheck(s, loc, ast->op.lhso);
			symn rht = typecheck(s, loc, ast->op.rhso);

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

		case OPER_adr:		// '+'
		case OPER_pls:		// '+'
		case OPER_mns:		// '-'
		case OPER_cmt:		// '~'
		case OPER_not: {	// '!'
			int cast;
			symn rht = typecheck(s, loc, ast->op.rhso);

			if (!rht || loc) {
				debug("cast(%T)[%k]: %+k", rht, ast, ast);
				return NULL;
			}
			if ((cast = typeTo(ast, rht))) {
				ast->type = rht;
				if (ast->kind == OPER_not) {
					ast->type = s->type_bol;
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
						error(s->s, ast->file, ast->line, "invalid cast(%+k)", ast);
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
			symn lht = typecheck(s, loc, ast->op.lhso);
			symn rht = typecheck(s, loc, ast->op.rhso);

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
			symn lht = typecheck(s, loc, ast->op.lhso);
			symn rht = typecheck(s, loc, ast->op.rhso);

			if (!lht || !rht || loc) {
				debug("cast(%T, %T): %T", lht, rht, 0);
				return NULL;
			}
			if ((cast = typeTo(ast, promote(lht, rht)))) {
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
						error(s->s, ast->file, ast->line, "invalid cast(%+k)", ast);
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
			symn lht = typecheck(s, loc, ast->op.lhso);
			symn rht = typecheck(s, loc, ast->op.rhso);

			if (!lht || !rht || loc) {
				debug("cast(%T, %T) : %+k", lht, rht, ast);
				return NULL;
			}

			if (ast->kind == OPER_equ || ast->kind == OPER_neq) {
				symn lhl = ast->op.lhso->kind == TYPE_ref ? ast->op.lhso->ref.link : NULL;
				symn rhl = ast->op.rhso->kind == TYPE_ref ? ast->op.rhso->ref.link : NULL;

				if (lhl == s->null_ref && rhl == s->null_ref)
					cast = TYPE_ref;

				else if (lhl == s->null_ref && rhl && rhl->cast == TYPE_ref)
					cast = TYPE_ref;

				else if (rhl == s->null_ref && lhl && lhl->cast == TYPE_ref)
					cast = TYPE_ref;

				// bool isint32 = type == int32;
				if (rhl == rht && lht == s->type_rec) {
					cast = TYPE_ref;
				}
				if (lhl == lht && rht == s->type_rec) {
					cast = TYPE_ref;
				}// * /

			}

			if (cast || (cast = castOf(promote(lht, rht)))) {
				if (!typeTo(ast, s->type_bol)) {
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
			symn lht = typecheck(s, loc, ast->op.lhso);
			symn rht = typecheck(s, loc, ast->op.rhso);

			if (!lht || !rht || loc) {
				debug("cast(%T, %T)", lht, rht);
				return NULL;
			}
			if ((cast = typeTo(ast, promote(lht, rht)))) {
				if (!castTo(ast->op.lhso, TYPE_bit)) {
					debug("%T('%k', %+k): %t", lht, ast, ast, cast);
					return 0;
				}
				if (!castTo(ast->op.rhso, TYPE_bit)) {
					debug("%T('%k', %+k): %t", rht, ast, ast, cast);
					return 0;
				}
				ast->type = s->type_bol;
				return ast->type;
			}
			fatal("operator %k (%T %T): %+k", ast, lht, rht, ast);
		} break;

		case OPER_sel: {	// '?:'
			int cast;
			symn cmp = typecheck(s, loc, ast->op.test);
			symn lht = typecheck(s, loc, ast->op.lhso);
			symn rht = typecheck(s, loc, ast->op.rhso);

			if (!cmp || !lht || !rht || loc) {
				debug("cast(%T, %T)[%k]", lht, rht, ast);
				return NULL;
			}
			if ((cast = typeTo(ast, promote(lht, rht)))) {
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

		case OPER_com: {	// ','
			TODO("FixMe")
			symn lht = typecheck(s, loc, ast->op.lhso);
			symn rht = typecheck(s, loc, ast->op.rhso);

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
			}
			fatal("operator %k (%T, %T): %+k", ast, lht, rht, ast);
		} break;
		// */

		// operator set
		case ASGN_set: {	// ':='
			symn lht = typecheck(s, loc, ast->op.lhso);
			symn rht = typecheck(s, loc, ast->op.rhso);
			symn var = linkOf(ast->op.lhso);

			if (!lht || !rht || loc) {
				debug("cast(%T, %T)", lht, rht);
				return NULL;
			}

			// HACK: pointer can be assigned to referencetypes.
			if (rht == s->type_ptr && var->cast == TYPE_ref) {
			}
			else if (!promote(lht, rht)) {
				trace("promote(%-T, %-T)", lht, rht);
				trace("%+k", ast);
				return 0;
			}
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
				case TYPE_int: return typeTo(ast, s->type_i32) ? s->type_i32 : NULL;
				case TYPE_flt: return typeTo(ast, s->type_f64) ? s->type_f64 : NULL;
				case TYPE_str: return typeTo(ast, s->type_str) ? s->type_str : NULL;
				case EMIT_opc: return /* ast->ref.link =  */ast->type = s->emit_opc;
				default: break;
			}
		} break;
	}

	if (ref && ref != s->void_tag) {
		sym = s->deft[ref->ref.hash];

		if (loc != NULL) {
			sym = loc->args;
		}

		if ((sym = lookup(s, sym, ref, args, 1))) {

			//~ debug("%k(%d)", ref, ast->line);
			// using function args

			if (sym) switch (sym->kind) {
				default:
					fatal("FixMe");
					break;

				case TYPE_def:
					result = sym->type;
					//~ debug("%T:%T in `%+k` (%d)", sym, result, ast, ast->line);
					break;

				case EMIT_opc:
				case TYPE_ref:
					result = sym->type;
					break;

				case TYPE_arr:
				case TYPE_rec:
					result = sym;
					break;
			}

			TODO("ugly hack")
			if (isType(sym) && args && !args->next) {			// cast
				if (!castTo(args, sym->cast)) {
					debug("%k:%t", args, castOf(args->type));
					return 0;
				}
			}

			if (sym->call) {
				astn argval = args;			// argument
				symn argsym = sym->args;	// parameter

				while (argsym && argval) {
					if (!castTo(argval, castOf(argsym->type))) {
						debug("%k:%t %+k", argval, castOf(argsym->type), argval);
						return 0;
					}

					// TODO: review
					if (argsym->cast == TYPE_ref || argval->type->cast == TYPE_ref) {
						if (!castTo(argval, argsym->cast)) {
							debug("%k:%t", argval, argsym->cast);
							return 0;
						}
					}

					//* if swap(a, b) is written instad of swap(&a, &b)
					if (argsym->cast == TYPE_ref && argval->type->cast != TYPE_ref) {
						symn lnk = argval->kind == TYPE_ref ? argval->ref.link : NULL;
						if (argval->kind != OPER_adr && lnk && lnk->cast != TYPE_ref) {
							warn(s->s, 2, argval->file, argval->line, "argument `%+k` is not explicitly passed by reference", argval);
						}
					}// */

					argsym = argsym->next;
					argval = argval->next;
				}

				if (!args) {
					result = s->type_ptr;
				}
			}

			ref->kind = TYPE_ref;
			ref->ref.link = sym;
			ref->type = result;
			ast->type = result;

			if (dot)
				dot->type = result;
		}
		else
			s->root = ref;
	}

	//~ if (ast->kind == OPER_fnc) {debug("%k is %-T: %-T", ref, sym, result);}
	//~ if (!result) debug("%+k in %-T", ast, loc);

	return result;
}

int fixargs(symn sym, int align, int stbeg) {
	symn arg;
	int stdiff = 0;
	int isCall = sym->call;
	//~ int stbeg = sizeOf(sym->type);
	for (arg = sym->args; arg; arg = arg->next) {

		if (arg->kind != TYPE_ref)
			continue;

		if (arg->stat)
			continue;

		// functions are byRef in structs and params
		if (arg->call) {
			arg->cast = TYPE_ref;
		}

		/*/ referenced types are passed by reference.
		if (isCall && arg->type->kind == TYPE_ref) {
			arg->cast = TYPE_ref;
		}// */

		arg->size = sizeOf(arg);

		// array types are passed by reference.
		if (isCall && arg->type->kind == TYPE_arr) {
			if (arg->type->size < 0) {		//~ dinamic size arrays are passed by pointer+length
				arg->cast = TYPE_arr;
				arg->size = 2 * vm_size;
			}
			else {							//~ static size arrays are passed as pointer
				arg->cast = TYPE_ref;
				arg->size = vm_size;
			}
		}

		arg->nest = 0;
		//~ arg->size = arg->type->size;
		arg->offs = align ? stbeg + stdiff : stbeg;
		stdiff += padded(arg->size, align);

		if (align == 0 && stdiff < arg->size)
			stdiff = arg->size;

		//~ debug("sizeof(%-T):%d", arg, arg->offs);
	}
	//~ because args are evaluated from right to left
	if (isCall) {
		for (arg = sym->args; arg; arg = arg->next) {
			arg->offs = stdiff - arg->offs;
		}
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
			install(s, with->name, TYPE_def, 0, with, NULL);

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
symn leave(ccState s, symn dcl, int mkstatic) {
	state rt = s->s;

	//~ int isFunc = dcl && dcl->call;
	// nonstatic declarations
	symn loc = NULL, lastloc = NULL;

	// static declarations
	symn sta = NULL;

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
		// declared in: (module, structure, function or wathever)
		sym->decl = dcl ? dcl : s->func;

		// pop from stack
		rt->defs = sym->defs;

		if (mkstatic) {
			sym->stat = 1;
		}

		// if not inside a static if, link to all
		if (!s->siff) {
			sym->defs = s->defs;
			s->defs = sym;
			if (sym->stat) {
				sym->gdef = rt->gdef;
				rt->gdef = sym;
			}
		}

		// public symbol if not starts with _
		//if (isFunc || !sym->name || *sym->name != '_')
		{
			if (sym->stat) {
				sym->next = sta;
				sta = sym;
			}
			else {
				sym->next = loc;
				loc = sym;
				if (!lastloc) {
					lastloc = sym;
				}
			}
		}
	}

	if (sta) {
		if (dcl) {
			dcl->sdef = sta;
		}
		else if (s->func) {
			s->func->sdef = sta;
		}
	}

	if (mkstatic) {
		loc = sta;
	}
	else if (lastloc) {
		lastloc->next = sta;
	}

	return loc;
}
