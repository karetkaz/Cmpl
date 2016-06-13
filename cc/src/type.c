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
	+uint64
	float32
	float64

	pointer
	variant
	typename        // compilers internal type reprezentation structure
	?function       //
	object          // base class of all reference types.

#typedefs
	@int: alias for int32
	@long: alias for int64
	@float: alias for float32
	@double: alias for float64

	@char: alias for uint8 / uint16

	@var: variant
	@array: variant[]

#constants
	@true: emit(bool, i32(1));
	@false: emit(bool, i32(0));
	@null: emit(pointer, i32(0));

Derived data types:
	slice: struct {const pointer array; const int length;}
	variant: struct variant {const pointer value; const typename type;}
	TODO: delegate: struct delegate {const pointer function; const pointer closure;}

User defined types:
	pointers arrays and slices:
		TODO: pointers are unsized
			ex: int* a;
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
			define Complex(variant var) = emit(Complex&, ref(var.type == Complex ? &var.data : null));

	function

TODO's:
	struct initialization:
		struct Demo {
			// normal member variable
			int32 a;

			// constant member variable, compiler error if not initialized when a new instance is created.
			const int32 b;

			// global variable hidden in this class.
			static int32 c;

			// global variable, compiler error if not initialized when declared.
			static const int32 d = 0;
		}
		Demo a = {a: 12, b: 88};
		Demo b = Demo(12, 88);
	array initialization:
		Demo a1[] = {Demo(1,2), Demo(2,3), ...};
		? Demo a3[] = [Demo]{Demo(1,2), Demo(2,3), ...}

*******************************************************************************/

#include "internal.h"

static symn promote(symn lht, symn rht);
static ccToken typeTo(astn ast, symn type);

/// Enter a new scope.
void enter(ccContext cc, astn ast) {
	cc->nest += 1;
	(void)ast;
}

/// Leave current scope.
symn leave(ccContext cc, symn dcl, int mode) {
	int i;
	rtContext rt = cc->rt;
	symn result = NULL;

	cc->nest -= 1;

	// clear from table
	for (i = 0; i < TBLS; i++) {
		symn def = cc->deft[i];
		while (def && def->nest > cc->nest) {
			def = def->next;
		}
		cc->deft[i] = def;
	}

	// clear from stack
	while (cc->vars && cc->nest < cc->vars->nest) {
		symn sym = cc->vars;

		// pop from stack
		cc->vars = sym->defs;

		sym->next = NULL;

		// declared in: (structure, function or whatever)
		sym->decl = dcl ? dcl : cc->func;

		if (mode & ATTR_stat) {
			sym->stat = 1;
		}
		if (!sym->stat && !cc->siff && sym->call && sym->init && sym->init->kind == STMT_beg) {
			warn(cc->rt, 1, sym->file, sym->line, "marking function to be static: `%-T`", sym);
			sym->stat = 1;
		}

		// if not inside a static if, link to all
		sym->defs = cc->defs;
		cc->defs = sym;

		if (!cc->siff) {
			if (sym->stat && sym->gdef == NULL) {
				sym->gdef = cc->gdef;
				cc->gdef = sym;
			}
		}

		// TODO: any field can have default value.
		if (dcl != NULL && dcl->kind == TYPE_rec) {
			dieif(dcl->call, "FixMe");
			if (sym->kind == CAST_ref && !sym->stat && sym->init) {
				warn(cc->rt, 8, sym->file, sym->line, "ignoring initialization of non static member `%+T`", sym);
			}
		}

		sym->next = result;
		result = sym;
	}
	//cc->defs = defs;

	return result;
}

/// Install a symbol(typename or variable)
symn install(ccContext cc, const char* name, ccToken kind, ccKind cast, unsigned size, symn type, astn init) {
	symn def;

	if (cc == NULL || name == NULL || kind == TYPE_any) {
		fatal(ERR_INTERNAL_ERROR);
		return NULL;
	}

	if (init && init->type == NULL) {
		fatal("FixMe '%s'", name);
		return NULL;
	}

	if ((kind & KIND_mask) == TYPE_rec) {
		logif((kind & (ATTR_stat | ATTR_const)) != (ATTR_stat | ATTR_const), "symbol `%s` should be declared as static and constant", name);
		kind |= ATTR_stat | ATTR_const;
	}

	if ((def = newdefn(cc, kind & 0xff))) {
		size_t length = strlen(name) + 1;
		unsigned hash = rehash(name, length) % TBLS;
		def->name = mapstr(cc, (char*)name, length, hash);
		def->nest = cc->nest;
		def->type = type;
		def->init = init;
		def->size = size;
		def->cast = cast;

		if (kind & ATTR_stat)
			def->stat = 1;

		if (kind & ATTR_const)
			def->cnst = 1;

		switch (kind & 0xff) {
			default:
				fatal(ERR_INTERNAL_ERROR);
				return NULL;

			// user type
			//~ case TYPE_ptr:
			case CAST_arr:	// string
			case TYPE_rec:
				def->offs = vmOffset(cc->rt, def);
				//~ def->pack = size;
				break;

			// variable
			case TYPE_def:
			case CAST_ref:
				break;

			case EMIT_opc:
				def->offs = size;
				def->size = 0;
				break;
		}

		def->next = cc->deft[hash];
		cc->deft[hash] = def;

		def->defs = cc->vars;
		cc->vars = def;
	}
	return def;
}

//~ TODO: we should handle redefinition in this function
symn declare(ccContext s, ccToken kind, astn tag, symn typ) {
	symn def;

	if (!tag || tag->kind != CAST_ref) {
		fatal("%+t must be an identifier", tag);
		return 0;
	}

	def = install(s, tag->ref.name, kind, TYPE_any, 0, typ, NULL);

	if (def != NULL) {

		def->file = tag->file;
		def->line = tag->line;
		def->colp = tag->colp;
		def->used = tag;

		tag->type = typ;
		tag->ref.link = def;

		tag->kind = TYPE_def;
		switch (kind & 0xff) {
			default:
				fatal(ERR_INTERNAL_ERROR);
				return NULL;

			case TYPE_rec:			// typedefn struct
				if (typ != NULL) {
					def->size = typ->size;
				}
				break;

			case TYPE_def:			// typename
			case CAST_ref:			// variable
				tag->kind = kind & 0xff;
				if (typ != NULL) {
					def->cast = typ->cast;
					def->size = sizeOf(typ, 1);
				}
				break;
		}
		addUsage(def, tag);
	}

	return def;
}

//~ TODO: !!! args are linked in a list by next !!??
symn lookup(ccContext cc, symn sym, astn ref, astn args, int raise) {
	symn asref = 0;
	symn best = 0;
	int found = 0;

	dieif(!ref || ref->kind != CAST_ref, "FixMe");

	// linearize args
	/*if (args && args->kind == OPER_com) {
		astn next = NULL;
		while (args->kind == OPER_com) {
			args->op.rhso->next = next;
			next = args->op.rhso;
			args = args->op.lhso;
		}
		args->next = next;
	}// */

	for (; sym; sym = sym->next) {
		int hascast = 0;
		astn argval = args;				// arguments
		symn param = sym->prms;			// parameters

		// exclude nameless symbols: arrays, inline functions.
		if (sym->name == NULL)
			continue;

		// exclude non matching names
		if (strcmp(sym->name, ref->ref.name) != 0)
			continue;

		if (!args && sym->call) {
			// keep the first match.
			if (sym->kind == CAST_ref) {
				if (asref == NULL)
					asref = sym;
				found += 1;
			}
			continue;
		}

		if (args != NULL) {
			if (!sym->call) {
				// TODO: enable basic type casts: float32(1)
				int isBasicCast = 0;

				symn sym2 = sym;
				while (sym2 != NULL) {
					if (sym2->kind != TYPE_def)
						break;
					if (sym2->init)
						break;
					sym2 = sym2->type;
				}

				if (!args->next && sym2 != NULL && isType(sym2)) {
					switch(sym2->cast) {
						default:
							break;
						case TYPE_rec:
							// Complex x = Complex(Complex(3,4));
							if (sym2 == args->type) {
								isBasicCast = 1;
							}
								// variant(variable)
							else if (sym2 == cc->type_var) {
								isBasicCast = 1;
							}
							break;

						case CAST_ref:
							// enable pointer(ref) and typename(ref)
							if (sym2 == cc->type_ptr || sym2 == cc->type_rec) {
								isBasicCast = 1;
							}
							break;

						case CAST_vid:
							// void(0);
							if (sym2 == cc->type_vid) {
								isBasicCast = 1;
							}
							break;
						case CAST_bit:
						case CAST_u32:
						case CAST_i32:
						case CAST_i64:
						case CAST_f32:
						case CAST_f64:
							isBasicCast = 1;
							break;
					}
				}

				if (!isBasicCast)
					continue;
			}

			while (argval && param) {

				if (!canAssign(cc, param, argval, 0)) {
					break;
				}

				// if null is passed by ref it will be as a cast
				if (argval->kind == CAST_ref && argval->ref.link == cc->null_ref) {
					hascast += 1;
				}

				else if (!canAssign(cc, param, argval, 1)) {
					hascast += 1;
				}

				// TODO: hascast += argget->cst2 != 0;

				argval = argval->next;
				param = param->next;
			}

			if (sym->call && (argval || param)) {
				debug("%-T(%+t, %-T)", sym, argval, param);
				continue;
			}
		}

		dieif(cc->func && cc->func->nest != cc->maxlevel - 1, "FIXME %d, %d", cc->func->nest, cc->maxlevel);
		// TODO: sym->decl && sym->decl->call && sym->decl != s->func
		if (cc->func && !cc->siff && cc->func->gdef && sym->nest && !sym->stat && sym->nest < cc->maxlevel) {
			error(cc->rt, ref->file, ref->line, "invalid use of local symbol `%t`.", ref);
		}

		// perfect match
		if (hascast == 0)
			break;

		// keep first match
		if (!best)
			best = sym;

		found += 1;
		// if we are here then sym is found, but it has implicit cast in it
		debug("%+t%s is probably %-T%s:%K", ref, args ? "()" : "", sym, sym->call ? "()" : "", sym->kind);
	}

	if (sym == NULL && best) {
		if (found > 1) {
			warn(cc->rt, 2, ref->file, ref->line, "using overload `%-T` of %d declared symbols.", best, found);
		}
		sym = best;
	}

	if (sym == NULL && asref) {
		if (found == 1 || cc->siff) {
			debug("as ref `%-T`(%?+t)(%d)", asref, args, ref->line);
			sym = asref;
		}
		else if (raise) {
			error(cc->rt, ref->file, ref->line, "there are %d overloads for `%T`", found, asref);
		}
	}

	if (sym != NULL) {
		if (sym->kind == TYPE_def && !sym->init) {
			sym = sym->type;
		}
	}

	return sym;
}

symn typeCheck(ccContext s, symn loc, astn ast) {
	astn ref = 0, args = 0;
	symn result = NULL;
	astn dot = NULL;
	symn sym = 0;

	if (ast == NULL) {
		fatal(ERR_INTERNAL_ERROR);
		return NULL;
	}

	ast->cast = TYPE_any;
	switch (ast->kind) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return NULL;

		case OPER_fnc: {
			astn fun = ast->op.lhso;
			args = ast->op.rhso;

			if (fun == NULL) {
				symn rht = typeCheck(s, NULL, ast->op.rhso);
				if (!rht || !castTo(ast->op.rhso, castOf(rht))) {
					traceAst(ast);
					return NULL;
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
					if (!typeCheck(s, lin, arg)) {
						if (!lin || !typeCheck(s, NULL, arg)) {
							traceAst(arg);
							return NULL;
						}
						if (!(arg->kind == OPER_fnc && isType(linkOf(arg->op.lhso)))) {
							if (arg->type->cast == CAST_ref) {
								warn(s->rt, 2, arg->file, arg->line, "argument `%+t` is passed by reference", arg);
							}
							else {
								warn(s->rt, 2, arg->file, arg->line, "argument `%+t` is passed by value: %-T", arg,
									 arg->type);
							}
							//~ warn(s->s, 2, arg->file, arg->line, "emit type cast expected: '%+t'", arg);
						}
					}

					args->op.rhso->next = linearize;
					linearize = args->op.rhso;
					args = args->op.lhso;
				}

				if (lin && args && args->kind == TYPE_rec) {
					//emit's first arg is 'struct'
					args->kind = CAST_ref;
					args->type = s->emit_opc;
					args->ref.link = s->emit_opc;
				}
				else if (!typeCheck(s, lin, args)) {
					if (!lin || !typeCheck(s, NULL, args)) {
						traceAst(args);
						return NULL;
					}
					// emit's first arg can be a type (static cast)
					if (!isTypeExpr(args) && !(args->kind == OPER_fnc && isType(linkOf(args)))) {
						if (args->type->cast == CAST_ref) {
							warn(s->rt, 2, args->file, args->line, "argument `%+t` is passed by reference", args);
						}
						else {
							warn(s->rt, 2, args->file, args->line, "argument `%+t` is passed by value: %-T", args,
								 args->type);
						}
						//~ warn(s->s, 2, args->file, args->line, "emit type cast expected: '%+t'", args);
					}
				}

				args->next = linearize;
			}

			if (fun) switch (fun->kind) {
					case OPER_dot: 	// Math.isNan ???
						if (!(loc = typeCheck(s, loc, fun->op.lhso))) {
							traceAst(ast);
							return NULL;
						}
						dot = fun;
						ref = fun->op.rhso;
						break;

					case EMIT_opc: {
						astn arg;
						for (arg = args; arg; arg = arg->next) {
							arg->cast = arg->type->cast;
						}
						if (!(loc = typeCheck(s, loc, fun))) {
							traceAst(ast);
							return NULL;
						}
						return ast->type = args ? args->type : NULL;
					}

					case CAST_ref:
						ref = ast->op.lhso;
						break;

					default:
						fatal(ERR_INTERNAL_ERROR);
						return NULL;
				}

			if (args == NULL) {
				args = s->void_tag;
			}

		} break;
		case OPER_dot: {
			sym = typeCheck(s, loc, ast->op.lhso);
			if (sym == NULL) {
				traceAst(ast);
				return NULL;
			}
			if (!castTo(ast->op.lhso, CAST_ref)) {
				traceAst(ast);
				return NULL;
			}

			loc = sym;
			ref = ast->op.rhso;
		} break;
		case OPER_idx: {
			symn lht = typeCheck(s, loc, ast->op.lhso);
			symn rht = typeCheck(s, loc, ast->op.rhso);

			if (!lht || !rht) {
				traceAst(ast);
				return NULL;
			}
			if (!castTo(ast->op.lhso, CAST_ref)) {
				traceAst(ast);
				return NULL;
			}
			if (!castTo(ast->op.rhso, TYPE_int)) {
				traceAst(ast);
				return NULL;
			}

			// base type of array;
			return ast->type = lht->type;
		} break;

		case OPER_adr:		// '&'
		case OPER_pls:		// '+'
		case OPER_mns:		// '-'
		case OPER_cmt:		// '~'
		case OPER_not: {	// '!'
			symn rht = typeCheck(s, loc, ast->op.rhso);
			ccToken cast;

			if (!rht || loc) {
				traceAst(ast);
				return NULL;
			}
			if ((cast = typeTo(ast, rht))) {
				ast->type = rht;
				if (ast->kind == OPER_not) {
					ast->type = s->type_bol;
					ast->cast = CAST_bit;
				}
				if (!castTo(ast->op.rhso, cast)) {
					traceAst(ast);
					return NULL;
				}
				switch (cast) {
					default:
						break;

					case CAST_u32:
					case CAST_i32:
					case CAST_i64:
						return ast->type;

					case CAST_f32:
					case CAST_f64:
						if (ast->kind != OPER_cmt)
							return ast->type;

						ast->type = 0;
						error(s->rt, ast->file, ast->line, "invalid cast(%+t)", ast);
						return NULL;
				}
				return ast->type;
			}
			fatal(FATAL_UNIMPLEMENTED_OPERATOR, ast, NULL, rht, ast);
		} break;

		case OPER_add:		// '+'
		case OPER_sub:		// '-'
		case OPER_mul:		// '*'
		case OPER_div:		// '/'
		case OPER_mod: {	// '%'
			symn lht = typeCheck(s, loc, ast->op.lhso);
			symn rht = typeCheck(s, loc, ast->op.rhso);

			if (!lht || !rht || loc) {
				traceAst(ast);
				return NULL;
			}
			if (lht->cast && rht->cast) {
				symn typ = promote(lht, rht);
				ccToken cast = castOf(typ);
				if (!castTo(ast->op.lhso, cast)) {
					traceAst(ast);
					return NULL;
				}
				if (!castTo(ast->op.rhso, cast)) {
					traceAst(ast);
					return NULL;
				}
				if (!castTo(ast, cast)) {
					traceAst(ast);
					return NULL;
				}
				return ast->type = typ;
			}
			fatal(FATAL_UNIMPLEMENTED_OPERATOR, ast, lht, rht, ast);
		} break;

		case OPER_shl:		// '>>'
		case OPER_shr:		// '<<'
		case OPER_and:		// '&'
		case OPER_ior:		// '|'
		case OPER_xor: {	// '^'
			symn lht = typeCheck(s, loc, ast->op.lhso);
			symn rht = typeCheck(s, loc, ast->op.rhso);
			ccToken cast;

			if (!lht || !rht || loc) {
				traceAst(ast);
				return NULL;
			}
			if ((cast = typeTo(ast, promote(lht, rht)))) {
				if (!castTo(ast->op.lhso, cast)) {
					traceAst(ast);
					return NULL;
				}
				if (!castTo(ast->op.rhso, cast)) {
					traceAst(ast);
					return NULL;
				}
				switch (cast) {
					default:
						break;

					case CAST_u32:
					case CAST_i32:
					case CAST_i64:
						return ast->type;

					case CAST_f32:
					case CAST_f64:
						ast->type = 0;
						error(s->rt, ast->file, ast->line, "invalid cast(%+t)", ast);
						return NULL;
				}
			}
			fatal(FATAL_UNIMPLEMENTED_OPERATOR, ast, lht, rht, ast);
		} break;

		case OPER_equ:		// '=='
		case OPER_neq:		// '!='
		case OPER_lte:		// '<'
		case OPER_leq:		// '<='
		case OPER_gte:		// '>'
		case OPER_geq: {	// '>='
			symn lht = typeCheck(s, loc, ast->op.lhso);
			symn rht = typeCheck(s, loc, ast->op.rhso);
			ccToken cast = TYPE_any;

			if (!lht || !rht || loc) {
				traceAst(ast);
				return NULL;
			}

			if (ast->kind == OPER_equ || ast->kind == OPER_neq) {
				symn lhl = ast->op.lhso->kind == CAST_ref ? ast->op.lhso->ref.link : NULL;
				symn rhl = ast->op.rhso->kind == CAST_ref ? ast->op.rhso->ref.link : NULL;

				if (lhl == s->null_ref && rhl == s->null_ref)
					cast = CAST_ref;

				else if (lhl == s->null_ref && rhl && (rhl->cast == CAST_ref || rhl->cast == CAST_arr))
					cast = CAST_ref;

				else if (rhl == s->null_ref && lhl && (lhl->cast == CAST_ref || lhl->cast == CAST_arr))
					cast = CAST_ref;

				// bool isint32 = type == int32;
				if (rhl == rht && lht == s->type_rec) {
					cast = CAST_ref;
				}
				if (lhl == lht && rht == s->type_rec) {
					cast = CAST_ref;
				}// * /

				// comparing enum variables
				if (rht->cast == ENUM_kwd && lht->cast == ENUM_kwd) {
				}
				else if (rht->cast == ENUM_kwd) {
					rht = rht->type;
					if (!typeTo(ast->op.rhso, rht)) {
						traceAst(ast);
						return NULL;
					}
				}
				else if (lht->cast == ENUM_kwd) {
					lht = lht->type;
					if (!typeTo(ast->op.lhso, lht)) {
						traceAst(ast);
						return NULL;
					}
				}
			}

			if (cast || (cast = castOf(promote(lht, rht)))) {
				if (!typeTo(ast, s->type_bol)) {
					traceAst(ast);
					return NULL;
				}
				if (!castTo(ast->op.lhso, cast)) {
					traceAst(ast);
					return NULL;
				}
				if (!castTo(ast->op.rhso, cast)) {
					traceAst(ast);
					return NULL;
				}
				return ast->type;
			}

			fatal(FATAL_UNIMPLEMENTED_OPERATOR, ast, lht, rht, ast);
		} break;

		case OPER_any:		// '&&'
		case OPER_all: {	// '||'
			symn lht = typeCheck(s, loc, ast->op.lhso);
			symn rht = typeCheck(s, loc, ast->op.rhso);
			ccToken cast;

			if (!lht || !rht || loc) {
				traceAst(ast);
				return NULL;
			}
			if ((cast = typeTo(ast, promote(lht, rht)))) {
				if (!castTo(ast->op.lhso, CAST_bit)) {
					traceAst(ast);
					return NULL;
				}
				if (!castTo(ast->op.rhso, CAST_bit)) {
					traceAst(ast);
					return NULL;
				}
				ast->type = s->type_bol;
				return ast->type;
			}
			fatal(FATAL_UNIMPLEMENTED_OPERATOR, ast, lht, rht, ast);
		} break;

		case OPER_sel: {	// '?:'
			symn cmp = typeCheck(s, loc, ast->op.test);
			symn lht = typeCheck(s, loc, ast->op.lhso);
			symn rht = typeCheck(s, loc, ast->op.rhso);
			ccToken cast;

			if (!cmp || !lht || !rht || loc) {
				traceAst(ast);
				return NULL;
			}
			if ((cast = typeTo(ast, promote(lht, rht)))) {
				if (!castTo(ast->op.test, CAST_bit)) {
					traceAst(ast);
					return NULL;
				}
				if (!castTo(ast->op.lhso, cast)) {
					traceAst(ast);
					return NULL;
				}
				if (!castTo(ast->op.rhso, cast)) {
					traceAst(ast);
					return NULL;
				}
				return ast->type;
			}
			fatal(FATAL_UNIMPLEMENTED_OPERATOR, ast, lht, rht, ast);
		} break;

		case OPER_com: {	// ','
			symn lht = typeCheck(s, loc, ast->op.lhso);
			symn rht = typeCheck(s, loc, ast->op.rhso);

			if (!lht || !rht || loc) {
				traceAst(ast);
				return NULL;
			}
			if (lht->cast && rht->cast) {
				symn typ = promote(lht, rht);
				ccToken cast = castOf(typ);
				if (!castTo(ast->op.lhso, cast)) {
					traceAst(ast);
					return NULL;
				}
				if (!castTo(ast->op.rhso, cast)) {
					traceAst(ast);
					return NULL;
				}
				return ast->type = typ;
			}
			fatal(FATAL_UNIMPLEMENTED_OPERATOR, ast, lht, rht, ast);
		} break;

			// operator set
		case ASGN_set: {	// ':='
			symn lht = typeCheck(s, loc, ast->op.lhso);
			symn rht = typeCheck(s, loc, ast->op.rhso);
			symn var = linkOf(ast->op.lhso);
			ccToken cast;

			if (!lht || !rht || !var || loc) {
				traceAst(ast);
				return NULL;
			}

			if (var->cnst) {
				error(s->rt, ast->file, ast->line, ERR_ASSIGN_TO_CONST, ast);
			}
			if (lht->cast != ENUM_kwd && rht->cast == ENUM_kwd) {
				rht = rht->type;
				if (!typeTo(ast->op.rhso, rht)) {
					traceAst(ast);
					return NULL;
				}
			}
			if (!(cast = canAssign(s, lht, ast->op.rhso, 0))) {
				traceAst(ast);
				return NULL;
			}
			if (!castTo(ast->op.rhso, cast)) {
				traceAst(ast);
				return NULL;
			}
			if (!castTo(ast->op.lhso, cast)) {
				traceAst(ast);
				return NULL;
			}
			/*if (!castTo(ast, cast)) {
				traceAst(ast);
				return NULL;
			}*/

			/*/ HACK: arrays of references dont casts to ref.
			if (lht->kind == CAST_arr) {// && lht->type->cast == CAST_ref) {
				ast->op.rhso->cst2 = TYPE_any;
			}*/

			ast->type = lht;
			return ast->type;
		} break;

			// operator get
		case CAST_ref:
			ref = ast;
			if (ast->ref.link != NULL) {
				if (ast->ref.hash == (unsigned)-1) {
					return ast->type;
				}
			}
			break;

		case EMIT_opc:
			return ast->type = s->emit_opc;

		case TYPE_int:
		case TYPE_flt:
		case TYPE_str: {
			if (loc) {
				traceAst(ast);
				return NULL;
			}
			switch (ast->kind) {
				case TYPE_int: return typeTo(ast, s->type_i32) ? s->type_i32 : NULL;
				case TYPE_flt: return typeTo(ast, s->type_f64) ? s->type_f64 : NULL;
				case TYPE_str: return typeTo(ast, s->type_str) ? s->type_str : NULL;
				default: break;
			}
		} break;
	}

	if (ref && ref != s->void_tag) {
		sym = s->deft[ref->ref.hash];

		if (loc != NULL) {
			sym = loc->flds;
		}

		if ((sym = lookup(s, sym, ref, args, 1)) != NULL) {
			// using function args
			switch (sym->kind) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					return NULL;

				case TYPE_def:
					result = sym->type;
					//~ debug("%T:%T in `%+t` (%d)", sym, result, ast, ast->line);
					break;

				case EMIT_opc:
				case CAST_ref:
					result = sym->type;
					break;

				case CAST_arr:
				case TYPE_rec:
					result = sym;
					break;
			}

			//~ TODO: HACK: type cast with one argument
			if (isType(sym) && args && !args->next) {			// cast
				if (!castTo(args, sym->cast)) {
					trace("%t: %K", args, sym->cast);
					return NULL;
				}
			}

			if (sym->call) {
				astn argval = args;			// argument
				symn param = sym->prms;		// parameter

				while (param && argval) {
					if (!castTo(argval, castOf(param->type))) {
						trace("%+t: %K", argval, castOf(param->type));
						return NULL;
					}

					// TODO: review
					if (param->cast == CAST_ref || argval->type->cast == CAST_ref) {
						if (!castTo(argval, param->cast)) {
							trace("%t: %K", argval, param->cast);
							return NULL;
						}
					}

					// if swap(a, b) is written instad of swap(&a, &b)
					if (param->cast == CAST_ref && argval->type->cast != CAST_ref) {
						symn lnk = argval->kind == CAST_ref ? argval->ref.link : NULL;
						if (argval->kind != OPER_adr && lnk && lnk->cast != CAST_ref && lnk->type->cast != CAST_ref) {
							warn(s->rt, 2, argval->file, argval->line,
								 "argument `%+t` is not explicitly passed by reference", argval);
						}
					}

					param = param->next;
					argval = argval->next;
				}

				if (!args) {
					result = s->type_ptr;
				}
			}

			ref->kind = CAST_ref;
			ref->ref.link = sym;
			ref->type = result;
			ast->type = result;

			addUsage(sym, ref);

			if (dot != NULL) {
				dot->type = result;
			}
		}
		else
			s->root = ref;
	}
	return result;
}

ccToken canAssign(ccContext cc, symn var, astn val, int strict) {
	symn lnk = linkOf(val);
	symn typ = var;
	symn cast;

	dieif(!var, "FixMe");
	dieif(!val, "FixMe");

	if (val->type == NULL) {
		return TYPE_any;
	}

	// assigning null or pass by reference
	if (lnk == cc->null_ref) {
		if (var && var->type == cc->type_var) {
			return CAST_ref;
		}
		if (var->cast == CAST_ref) {
			return CAST_ref;
		}
	}

	// assigning a typename or pass by reference
	if (lnk && (lnk->kind == TYPE_rec || lnk->kind == CAST_arr)) {
		if (var->type == cc->type_rec) {
			return CAST_ref;
		}
	}

	if (var->kind == CAST_ref || var->kind == TYPE_def) {
		typ = var->type;

		// assigning a function
		if (var->call) {
			symn fun = linkOf(val);
			symn arg1 = var->prms;
			symn arg2 = NULL;
			struct astNode atag;

			atag.kind = CAST_ref;
			atag.type = typ;
			atag.cast = var->cast;
			atag.ref.link = var;

			if (fun && fun->kind == EMIT_opc) {
				val->type = var;
				return CAST_ref;
			}
			if (fun && canAssign(cc, fun->type, &atag, 1)) {
				arg2 = fun->prms;
				while (arg1 && arg2) {

					atag.type = arg2->type;
					atag.cast = arg2->cast;
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
				return TYPE_any;
			}
			if (arg1 || arg2) {
				trace("%-T != %-T", arg1, arg2);
				return TYPE_any;
			}
			// function is by ref
			return CAST_ref;
		}
		else if (!strict) {
			strict = var->cast == CAST_ref;
		}
		// assign pointer to reference
		if (var->cast == CAST_ref && val->type == cc->type_ptr) {
			// TODO: warn
			return CAST_ref;
		}
	}

	if (typ == val->type) {
		return var->cast;
	}

	// assign enum.
	if (lnk && lnk->cast == ENUM_kwd) {
		if (typ == lnk->type->type) {
			return lnk->type->type->cast;
		}
	}
	else if (typ->cast == ENUM_kwd) {
		symn t;
		for (t = typ->flds; t != NULL; t = t->next) {
			if (t == lnk) {
				return lnk->cast;
			}
		}
	}

	// assign array
	if (typ->kind == CAST_arr) {
		struct astNode atag;
		symn vty = val->type;

		memset(&atag, 0, sizeof(atag));
		atag.kind = CAST_ref;
		atag.type = vty ? vty->type : NULL;
		atag.cast = atag.type ? atag.type->cast : TYPE_any;
		atag.ref.link = NULL;
		atag.ref.name = ".generated token";

		//~ check if subtypes are assignable
		if (canAssign(cc, typ->type, &atag, strict)) {
			// assign to dynamic array
			if (typ->init == NULL) {
				return CAST_arr;
			}
			if (typ->size == val->type->size) {
				// TODO: return <?>
				return CAST_ref;
			}
		}

		if (!strict) {
			return canAssign(cc, var->type, val, strict);
		}
	}

	if (!strict && (cast = promote(typ, val->type))) {
		// TODO: return <?> val->cast ?
		return cast->cast;
	}

	//~ /*TODO: hex32 can be passed as int32 by ref
	if (val->type && typ->cast == val->type->cast) {
		if (typ->size == val->type->size) {
			switch (typ->cast) {
				default:
					break;

				case CAST_u32:
				case CAST_i32:
				case CAST_i64:
				case CAST_f32:
				case CAST_f64:
					return typ->cast;
			}
		}
	}

	debug("%-T := %-T(%+t)", var, val->type, val);
	return TYPE_any;
}

void addUsage(symn sym, astn tag) {
	if (sym == NULL || tag == NULL) {
		return;
	}
	if (tag->ref.used != NULL) {
#ifdef DEBUGGING	// extra check: if this node is linked (.used) it must be in the list
		astn usage;
		for (usage = sym->used; usage; usage = usage->ref.used) {
			if (usage == tag) {
				break;
			}
		}
		dieif(usage == NULL, "usage not found in the list: %+t(%s:%u:%u)", tag, tag->file, tag->line, tag->colp);
#endif
		return;
	}
	if (sym->used != tag) {
		tag->ref.used = sym->used;
		sym->used = tag;
	}
}

int countUsages(symn sym) {
	int result = 0;
	astn usage;
	for (usage = sym->used; usage; usage = usage->ref.used) {
		result += 1;
	}
	return result;
}

// TODO: remove function
static inline ccToken castKind(symn typ) {
	if (typ != NULL) {
		switch (typ->cast) {
			default:
				break;
			case ENUM_kwd:
				return castKind(typ->type);
			case CAST_vid:
				return CAST_vid;
			case CAST_bit:
				return CAST_bit;
			case CAST_u32:
			case CAST_i32:
			case CAST_i64:
				return TYPE_int;
			case CAST_f32:
			case CAST_f64:
				return TYPE_flt;
			case CAST_ref:
				return CAST_ref;
		}
	}
	return TYPE_any;
}
// determine the resulting type of a OP b
static symn promote(symn lht, symn rht) {
	symn result = 0;
	if (lht && rht) {
		if (lht == rht) {
			result = lht;
		}
		else switch (castKind(rht)) {
				default:
					break;
				case CAST_bit:
				case TYPE_int: switch (castKind(lht)) {
						default:
							break;
						case CAST_bit:
						case TYPE_int:
							//~ TODO: bool + int is bool; if sizeof(bool) == 4
							if (lht->cast == CAST_bit && lht->size == rht->size) {
								result = rht;
							}
							else {
								result = lht->size >= rht->size ? lht : rht;
							}
							break;

						case TYPE_flt:
							result = lht;
							break;

					} break;
				case TYPE_flt: switch (castKind(lht)) {
						default:
							break;
						case CAST_bit:
						case TYPE_int:
							result = rht;
							break;

						case TYPE_flt:
							result = lht->size >= rht->size ? lht : rht;
							break;
					} break;
			}
	}
	else if (rht) {
		result = rht;
	}

	return result;
}
//~ TODO: check / remove function
static ccToken typeTo(astn ast, symn type) {
	if (!ast) return TYPE_any;

	while (ast->kind == OPER_com) {
		if (!typeTo(ast->op.rhso, type)) {
			traceAst(ast);
			return TYPE_any;
		}
		ast = ast->op.lhso;
	}

	return castTo(ast, castOf(ast->type = type));
}
