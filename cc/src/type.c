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

symn newdefn(ccState s, ccToken kind) {
	state rt = s->s;
	symn def = NULL;

	rt->_beg = paddptr(rt->_beg, 8);
	if(rt->_beg >= rt->_end) {
		fatal(ERR_INTERNAL_ERROR);
		return NULL;
	}

	def = (symn)rt->_beg;
	rt->_beg += sizeof(struct symNode);
	memset(def, 0, sizeof(struct symNode));
	def->kind = kind;

	return def;
}

/// install a symbol(typename or variable)
symn install(ccState s, const char* name, ccToken kind, ccToken cast, unsigned size, symn type, astn init) {
	symn def;

	if (s == NULL || name == NULL || kind == TYPE_any) {
		fatal(ERR_INTERNAL_ERROR);
		return NULL;
	}

	if (init && init->type == NULL) {
		fatal("FixMe '%s'", name);
		return NULL;
	}

	if ((kind & 0xff) == TYPE_rec) {
		logif((kind & (ATTR_stat | ATTR_const)) != (ATTR_stat | ATTR_const), "symbol `%s` should be declared as static and constant", name);
		kind |= ATTR_stat | ATTR_const;
	}

	if ((def = newdefn(s, kind & 0xff))) {
		size_t length = strlen(name) + 1;
		unsigned hash = rehash(name, length) % TBLS;
		def->name = mapstr(s, (char*)name, length, hash);
		def->nest = s->nest;
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
			case TYPE_arr:	// string
			case TYPE_rec:
				def->offs = vmOffset(s->s, def);
				//~ def->pack = size;
				break;

			// variable
			case TYPE_def:
			case TYPE_ref:
				break;

			case EMIT_opc:
				def->offs = size;
				def->size = 0;
				break;
		}

		def->next = s->deft[hash];
		s->deft[hash] = def;

		def->defs = s->s->defs;
		s->s->defs = def;
	}
	return def;
}

/// Install a type; @see state.api.ccAddType
symn ccAddType(state rt, const char* name, unsigned size, int refType) {
	//~ dieif(!rt->cc, "FixMe");
	return install(rt->cc, name, ATTR_stat | ATTR_const | TYPE_rec, refType ? TYPE_ref : TYPE_rec, size, rt->cc->type_rec, NULL);
}

//#{ libc.c ---------------------------------------------------------------------
static symn installref(state rt, const char* prot, astn* argv) {
	astn root, args;
	symn result = NULL;
	int warn, errc = rt->errc;

	if (!ccOpen(rt, NULL, 0, (char*)prot)) {
		trace("FixMe");
		return NULL;
	}

	warn = rt->cc->warn;

	// enable all warnings
	rt->cc->warn = 9;
	root = decl_var(rt->cc, &args, decl_NoDefs | decl_NoInit);

	dieif(root == NULL, "error declaring: %s", prot);

	dieif(!skip(rt->cc, STMT_do), "`;` expected declaring: %s", prot);

	dieif(ccDone(rt->cc) != 0, "FixMe");

	dieif(root->kind != TYPE_ref, "FixMe %+t", root);

	if ((result = root->ref.link)) {
		dieif(result->kind != TYPE_ref, "FixMe");
		*argv = args;
		result->cast = TYPE_any;
	}

	rt->cc->warn = warn;
	return errc == rt->errc ? result : NULL;
}

/// Install a native function; @see state.api.ccAddCall
symn ccAddCall(state rt, int libc(libcArgs), void* data, const char* proto) {
	symn param, sym = NULL;
	int stdiff = 0;
	astn args = NULL;

	dieif(libc == NULL || !proto, "FixMe");

	//~ from: int64 zxt(int64 val, int offs, int bits);
	//~ make: define zxt(int64 val, int offs, int bits) = emit(int64, libc(25), i64(val), i32(offs), i32(bits));

	if ((sym = installref(rt, proto, &args))) {
		struct libc* lc = NULL;
		symn link = newdefn(rt->cc, EMIT_opc);
		astn libcinit;
		size_t libcpos = rt->cc->libc ? rt->cc->libc->pos + 1 : 0;

		dieif(rt->_end - rt->_beg < (ptrdiff_t)sizeof(struct libc), "FixMe");

		rt->_end -= sizeof(struct libc);
		lc = (struct libc*)rt->_end;
		lc->next = rt->cc->libc;
		rt->cc->libc = lc;

		link->name = "libc";
		link->offs = opc_libc;
		link->type = sym->type;
		link->init = intnode(rt->cc, libcpos);

		libcinit = lnknode(rt->cc, link);
		stdiff = fixargs(sym, vm_size, 0);

		// glue the new libcinit argument
		if (args && args != rt->cc->void_tag) {
			astn narg = newnode(rt->cc, OPER_com);
			astn arg = args;
			narg->op.lhso = libcinit;

			if (1) {
				symn s = NULL;
				astn arg = args;
				while (arg->kind == OPER_com) {
					astn n = arg->op.rhso;
					s = linkOf(n);
					arg = arg->op.lhso;
					if (s && n) {
						n->cst2 = s->cast;
					}
				}
				s = linkOf(arg);
				if (s && arg) {
					arg->cst2 = s->cast;
				}
			}

			if (arg->kind == OPER_com) {
				while (arg->op.lhso->kind == OPER_com) {
					arg = arg->op.lhso;
				}
				narg->op.rhso = arg->op.lhso;
				arg->op.lhso = narg;
			}
			else {
				narg->op.rhso = args;
				args = narg;
			}
		}
		else {
			args = libcinit;
		}

		libcinit = newnode(rt->cc, OPER_fnc);
		libcinit->op.lhso = rt->cc->emit_tag;
		libcinit->type = sym->type;
		libcinit->op.rhso = args;

		sym->kind = TYPE_def;
		sym->init = libcinit;
		sym->offs = libcpos;

		lc->call = libc;
		lc->data = data;
		lc->pos = libcpos;
		lc->sym = sym;

		lc->chk = stdiff / 4;

		stdiff -= sizeOf(sym->type, 1);
		lc->pop = stdiff / 4;

		// make non reference parameters symbolic by default
		for (param = sym->prms; param; param = param->next) {
			if (param->cast != TYPE_ref && !param->call) {
				param->kind = TYPE_def;
			}
		}
	}
	else {
		error(rt, NULL, 0, "install(`%s`)", proto);
	}

	return sym;
}
//#}

// promote
static inline int castkind(symn typ) {
	if (typ != NULL) {
		switch (typ->cast) {
			default:
				break;
			case ENUM_kwd:
				return castkind(typ->type);
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
	}
	return 0;
}
static symn promote(symn lht, symn rht) {
	symn result = 0;
	if (lht && rht) {
		if (lht == rht) {
			result = lht;
		}
		else switch (castkind(rht)) {
			default:
				break;
			case TYPE_bit:
			case TYPE_int: switch (castkind(lht)) {
				default:
					break;
				case TYPE_bit:
				case TYPE_int:
					//~ TODO: bool + int is bool; if sizeof(bool) == 4
					if (lht->cast == TYPE_bit && lht->size == rht->size) {
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
			case TYPE_flt: switch (castkind(lht)) {
				default:
					break;
				case TYPE_bit:
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

	//~ logif(DEBUGGING > 4 && result == NULL, "promote failed(%T, %T)", lht, rht);
	return result;
}

void addUsage(symn sym, astn tag) {
	if (sym == NULL || tag == NULL) {
		return;
	}
	if (tag->ref.used != NULL) {
		#ifdef DEBUGGING
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

ccToken canAssign(ccState cc, symn var, astn val, int strict) {
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
			return TYPE_ref;
		}
		if (var->cast == TYPE_ref) {
			return TYPE_ref;
		}
	}

	// assigning a typename or pass by reference
	if (lnk && (lnk->kind == TYPE_rec || lnk->kind == TYPE_arr)) {
		if (var->type == cc->type_rec) {
			return TYPE_ref;
		}
	}

	if (var->kind == TYPE_ref || var->kind == TYPE_def) {
		typ = var->type;

		// assigning a function
		if (var->call) {
			symn fun = linkOf(val);
			symn arg1 = var->prms;
			symn arg2 = NULL;
			struct astNode atag;

			atag.kind = TYPE_ref;
			atag.type = typ;
			atag.cst2 = var->cast;
			atag.ref.link = var;

			if (fun && fun->kind == EMIT_opc) {
				val->type = var;
				return TYPE_ref;
			}
			if (fun && canAssign(cc, fun->type, &atag, 1)) {
				arg2 = fun->prms;
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
				return TYPE_any;
			}
			if (arg1 || arg2) {
				trace("%-T != %-T", arg1, arg2);
				return TYPE_any;
			}
			// function is by ref
			return TYPE_ref;
		}
		else if (!strict) {
			strict = var->cast == TYPE_ref;
		}
		// assign pointer to reference
		if (var->cast == TYPE_ref && val->type == cc->type_ptr) {
			// TODO: warn
			return TYPE_ref;
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
	if (typ->kind == TYPE_arr) {
		struct astNode atag;
		symn vty = val->type;

		memset(&atag, 0, sizeof(atag));
		atag.kind = TYPE_ref;
		atag.type = vty ? vty->type : NULL;
		atag.cst2 = atag.type ? atag.type->cast : TYPE_any;
		atag.ref.link = NULL;
		atag.ref.name = ".generated token";

		//~ check if subtypes are assignable
		if (canAssign(cc, typ->type, &atag, strict)) {
			// assign to dynamic array
			if (typ->init == NULL) {
				return TYPE_arr;
			}
			if (typ->size == val->type->size) {
				// TODO: return <?>
				return TYPE_ref;
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

				case TYPE_u32:
				case TYPE_i32:
				case TYPE_i64:
				case TYPE_f32:
				case TYPE_f64:
					return typ->cast;
			}
		}
	}

	debug("%-T := %-T(%+t)", var, val->type, val);
	return TYPE_any;
}

//~ TODO: !!! args are linked in a list by next !!??
symn lookup(ccState cc, symn sym, astn ref, astn args, int raise) {
	symn asref = 0;
	symn best = 0;
	int found = 0;

	dieif(!ref || ref->kind != TYPE_ref, "FixMe");

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

		// there are nameless symbols: functions, arrays.
		if (sym->name == NULL)
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

				if (!args->next && sym2 != NULL && istype(sym2)) {
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

						case TYPE_ref:
							// enable pointer(ref) and typename(ref)
							if (sym2 == cc->type_ptr || sym2 == cc->type_rec) {
								isBasicCast = 1;
							}
							break;

						case TYPE_vid:
							// void(0);
							if (sym2 == cc->type_vid) {
								isBasicCast = 1;
							}
							break;
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

			while (argval && param) {

				if (!canAssign(cc, param, argval, 0)) {
					break;
				}

				// if null is passed by ref it will be as a cast
				if (argval->kind == TYPE_ref && argval->ref.link == cc->null_ref) {
					hascast += 1;
				}

				else if (!canAssign(cc, param, argval, 1)) {
					hascast += 1;
				}

				// TODO: hascast += argval->cst2 != 0;

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
			error(cc->s, ref->file, ref->line, "invalid use of local symbol `%t`.", ref);
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
			warn(cc->s, 2, ref->file, ref->line, "using overload `%-T` of %d declared symbols.", best, found);
		}
		sym = best;
	}

	if (sym == NULL && asref) {
		if (found == 1 || cc->siff) {
			debug("as ref `%-T`(%?+t)(%d)", asref, args, ref->line);
			sym = asref;
		}
		else if (raise) {
			error(cc->s, ref->file, ref->line, "there are %d overloads for `%T`", found, asref);
		}
	}

	if (sym != NULL) {
		if (sym->kind == TYPE_def && !sym->init) {
			sym = sym->type;
		}
	}

	return sym;
}

//~ TODO: we should handle redefinition in this function
symn declare(ccState s, ccToken kind, astn tag, symn typ) {
	symn def;

	if (!tag || tag->kind != TYPE_ref) {
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
			case TYPE_ref:			// variable
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

int istype(const symn sym) {
	if (sym == NULL) {
		return 0;
	}

	switch (sym->kind) {
		default:
			break;

		case TYPE_arr:
		case TYPE_rec:
			return sym->kind;

		case TYPE_def:
			if (sym->init == NULL) {
				return istype(sym->type);
			}
			break;
	}
	//~ trace("%T is not a type", sym);
	return 0;
}

int usages(symn sym) {
	int result = 0;
	astn usage;
	for (usage = sym->used; usage; usage = usage->ref.used) {
		result += 1;
	}
	return result;
}

//~ TODO: this should be calculated by fixargs() and replaced by (var|typ)->size
size_t sizeOf(symn sym, int varSize) {
	if (sym) switch (sym->kind) {
		default:
			break;
		//~ case TYPE_vid:
		//~ case TYPE_bit:
		//~ case TYPE_int:
		//~ case TYPE_flt:

		case EMIT_opc:
			if (sym->cast == TYPE_ref) {
				return vm_size;
			}
			return (size_t) sym->size;

		case TYPE_rec:
		case TYPE_arr: switch (sym->cast) {
			case TYPE_ref: if (varSize) {
				return vm_size;
			}
			case TYPE_arr: if (varSize) {
				return 2 * vm_size;
			}
			default:
				return sym->size;
		}

		case TYPE_def:
		case TYPE_ref: switch (sym->cast) {
			case TYPE_ref:
				return vm_size;
			case TYPE_arr:
				return 2 * vm_size;
			default:
				return sizeOf(sym->type, 0);
		}
	}
	fatal("failed(%K): %-T", sym ? sym->kind : 0, sym);
	return 0;
}

/** Cast
 * returns one of (TYPE_bit, ref, u32, i32, i64, f32, f64)
**/
ccToken castOf(symn typ) {
	if (typ) switch (typ->kind) {

		default:
			break;

		case TYPE_def:
			return castOf(typ->type);

		//~ case TYPE_vid:
			//~ return typ->kind;

		case TYPE_arr:
			// static sized arrays cast to pointer
			if (typ->init == NULL)
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
	debug("failed(%K): %?-T", typ ? typ->kind : 0, typ);
	return TYPE_any;
}
ccToken castTo(astn ast, ccToken cto) {
	ccToken atc = TYPE_any;
	if (!ast) {
		return TYPE_any;
	}
	//~ TODO: check validity / Remove function

	atc = ast->type ? ast->type->cast : TYPE_any;
	if (cto != atc) switch (cto) {
		case TYPE_any:
			return atc;

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
			debug("cast(%+t) to %K/%K", ast, cto, atc);
			//~ return 0;
			break;
	}
	return ast->cst2 = cto;
}
static ccToken typeTo(astn ast, symn type) {
	if (!ast) return TYPE_any;

	//~ TODO: check validity / Remove function

	while (ast->kind == OPER_com) {
		if (!typeTo(ast->op.rhso, type)) {
			traceAst(ast);
			return TYPE_any;
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

	if (ast == NULL) {
		fatal(ERR_INTERNAL_ERROR);
		return NULL;
	}

	ast->cst2 = TYPE_any;
	switch (ast->kind) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return NULL;

		case OPER_fnc: {
			astn fun = ast->op.lhso;
			args = ast->op.rhso;

			if (fun == NULL) {
				symn rht = typecheck(s, NULL, ast->op.rhso);
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
					if (!typecheck(s, lin, arg)) {
						if (!lin || !typecheck(s, NULL, arg)) {
							traceAst(arg);
							return NULL;
						}
						if (!(arg->kind == OPER_fnc && istype(linkOf(arg->op.lhso)))) {
							if (arg->type->cast == TYPE_ref) {
								warn(s->s, 2, arg->file, arg->line, "argument `%+t` is passed by reference", arg);
							}
							else {
								warn(s->s, 2, arg->file, arg->line, "argument `%+t` is passed by value: %-T", arg, arg->type);
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
					args->kind = TYPE_ref;
					args->type = s->emit_opc;
					args->ref.link = s->emit_opc;
				}
				else if (!typecheck(s, lin, args)) {
					if (!lin || !typecheck(s, NULL, args)) {
						traceAst(args);
						return NULL;
					}
					// emit's first arg can be a type (static cast)
					if (!isType(args) && !(args->kind == OPER_fnc && istype(linkOf(args)))) {
						if (args->type->cast == TYPE_ref) {
							warn(s->s, 2, args->file, args->line, "argument `%+t` is passed by reference", args);
						}
						else {
							warn(s->s, 2, args->file, args->line, "argument `%+t` is passed by value: %-T", args, args->type);
						}
						//~ warn(s->s, 2, args->file, args->line, "emit type cast expected: '%+t'", args);
					}
				}

				args->next = linearize;
			}

			if (fun) switch (fun->kind) {
				case OPER_dot: 	// math.isNan ???
					if (!(loc = typecheck(s, loc, fun->op.lhso))) {
						traceAst(ast);
						return NULL;
					}
					dot = fun;
					ref = fun->op.rhso;
					break;

				case EMIT_opc: {
					astn arg;
					for (arg = args; arg; arg = arg->next) {
						arg->cst2 = arg->type->cast;
					}
					if (!(loc = typecheck(s, loc, fun))) {
						traceAst(ast);
						return NULL;
					}
					return ast->type = args ? args->type : NULL;
				}

				case TYPE_ref:
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
			sym = typecheck(s, loc, ast->op.lhso);
			if (sym == NULL) {
				traceAst(ast);
				return NULL;
			}
			if (!castTo(ast->op.lhso, TYPE_ref)) {
				traceAst(ast);
				return NULL;
			}

			loc = sym;
			ref = ast->op.rhso;
		} break;
		case OPER_idx: {
			symn lht = typecheck(s, loc, ast->op.lhso);
			symn rht = typecheck(s, loc, ast->op.rhso);

			if (!lht || !rht) {
				traceAst(ast);
				return NULL;
			}
			if (!castTo(ast->op.lhso, TYPE_ref)) {
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
			symn rht = typecheck(s, loc, ast->op.rhso);
			ccToken cast;

			if (!rht || loc) {
				traceAst(ast);
				return NULL;
			}
			if ((cast = typeTo(ast, rht))) {
				ast->type = rht;
				if (ast->kind == OPER_not) {
					ast->type = s->type_bol;
					ast->cst2 = TYPE_bit;
				}
				if (!castTo(ast->op.rhso, cast)) {
					traceAst(ast);
					return NULL;
				}
				switch (cast) {
					default:
						break;

					case TYPE_u32:
					case TYPE_i32:
					case TYPE_i64:
						return ast->type;

					case TYPE_f32:
					case TYPE_f64:
						if (ast->kind != OPER_cmt)
							return ast->type;

						ast->type = 0;
						error(s->s, ast->file, ast->line, "invalid cast(%+t)", ast);
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
			symn lht = typecheck(s, loc, ast->op.lhso);
			symn rht = typecheck(s, loc, ast->op.rhso);

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
			symn lht = typecheck(s, loc, ast->op.lhso);
			symn rht = typecheck(s, loc, ast->op.rhso);
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

					case TYPE_u32:
					case TYPE_i32:
					case TYPE_i64:
						return ast->type;

					case TYPE_f32:
					case TYPE_f64:
						ast->type = 0;
						error(s->s, ast->file, ast->line, "invalid cast(%+t)", ast);
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
			symn lht = typecheck(s, loc, ast->op.lhso);
			symn rht = typecheck(s, loc, ast->op.rhso);
			ccToken cast = TYPE_any;

			if (!lht || !rht || loc) {
				traceAst(ast);
				return NULL;
			}

			if (ast->kind == OPER_equ || ast->kind == OPER_neq) {
				symn lhl = ast->op.lhso->kind == TYPE_ref ? ast->op.lhso->ref.link : NULL;
				symn rhl = ast->op.rhso->kind == TYPE_ref ? ast->op.rhso->ref.link : NULL;

				if (lhl == s->null_ref && rhl == s->null_ref)
					cast = TYPE_ref;

				else if (lhl == s->null_ref && rhl && (rhl->cast == TYPE_ref || rhl->cast == TYPE_arr))
					cast = TYPE_ref;

				else if (rhl == s->null_ref && lhl && (lhl->cast == TYPE_ref || lhl->cast == TYPE_arr))
					cast = TYPE_ref;

				// bool isint32 = type == int32;
				if (rhl == rht && lht == s->type_rec) {
					cast = TYPE_ref;
				}
				if (lhl == lht && rht == s->type_rec) {
					cast = TYPE_ref;
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

		case OPER_lor:		// '&&'
		case OPER_lnd: {	// '||'
			symn lht = typecheck(s, loc, ast->op.lhso);
			symn rht = typecheck(s, loc, ast->op.rhso);
			ccToken cast;

			if (!lht || !rht || loc) {
				traceAst(ast);
				return NULL;
			}
			if ((cast = typeTo(ast, promote(lht, rht)))) {
				if (!castTo(ast->op.lhso, TYPE_bit)) {
					traceAst(ast);
					return NULL;
				}
				if (!castTo(ast->op.rhso, TYPE_bit)) {
					traceAst(ast);
					return NULL;
				}
				ast->type = s->type_bol;
				return ast->type;
			}
			fatal(FATAL_UNIMPLEMENTED_OPERATOR, ast, lht, rht, ast);
		} break;

		case OPER_sel: {	// '?:'
			symn cmp = typecheck(s, loc, ast->op.test);
			symn lht = typecheck(s, loc, ast->op.lhso);
			symn rht = typecheck(s, loc, ast->op.rhso);
			ccToken cast;

			if (!cmp || !lht || !rht || loc) {
				traceAst(ast);
				return NULL;
			}
			if ((cast = typeTo(ast, promote(lht, rht)))) {
				if (!castTo(ast->op.test, TYPE_bit)) {
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
			//~ TODO: FixMe
			symn lht = typecheck(s, loc, ast->op.lhso);
			symn rht = typecheck(s, loc, ast->op.rhso);

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
			symn lht = typecheck(s, loc, ast->op.lhso);
			symn rht = typecheck(s, loc, ast->op.rhso);
			symn var = linkOf(ast->op.lhso);
			ccToken cast;

			if (!lht || !rht || !var || loc) {
				traceAst(ast);
				return NULL;
			}

			if (var->cnst) {
				error(s->s, ast->file, ast->line, ERR_ASSIGN_TO_CONST, ast);
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
			if (lht->kind == TYPE_arr) {// && lht->type->cast == TYPE_ref) {
				ast->op.rhso->cst2 = TYPE_any;
			}*/

			ast->type = lht;
			return ast->type;
		} break;

		// operator get
		case TYPE_ref:
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

		if ((sym = lookup(s, sym, ref, args, 1))) {

			// using function args
			if (sym) switch (sym->kind) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					return NULL;

				case TYPE_def:
					result = sym->type;
					//~ debug("%T:%T in `%+t` (%d)", sym, result, ast, ast->line);
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

			//~ TODO: hack: type cast if one argument
			if (istype(sym) && args && !args->next) {			// cast
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
					if (param->cast == TYPE_ref || argval->type->cast == TYPE_ref) {
						if (!castTo(argval, param->cast)) {
							trace("%t: %K", argval, param->cast);
							return NULL;
						}
					}

					// if swap(a, b) is written instad of swap(&a, &b)
					if (param->cast == TYPE_ref && argval->type->cast != TYPE_ref) {
						symn lnk = argval->kind == TYPE_ref ? argval->ref.link : NULL;
						if (argval->kind != OPER_adr && lnk && lnk->cast != TYPE_ref && lnk->type->cast != TYPE_ref) {
							warn(s->s, 2, argval->file, argval->line, "argument `%+t` is not explicitly passed by reference", argval);
						}
					}

					param = param->next;
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

size_t fixargs(symn sym, unsigned int align, size_t base) {
	symn arg;
	size_t stdiff = 0;
	int isCall = sym->call;
	for (arg = sym->prms; arg; arg = arg->next) {

		if (arg->kind != TYPE_ref)
			continue;

		if (arg->stat)
			continue;

		// functions are byRef in structs and params
		if (arg->call) {
			arg->cast = TYPE_ref;
		}

		arg->size = sizeOf(arg, 1);

		//TODO: remove check: dynamic size arrays are represented as pointer+length
		if (arg->type->kind == TYPE_arr && arg->type->init == NULL) {
			dieif(arg->size != 2 * vm_size, ERR_INTERNAL_ERROR);
			dieif(arg->cast != TYPE_arr, ERR_INTERNAL_ERROR);
			//arg->size = 2 * vm_size;
			//arg->cast = TYPE_arr;
		}

		//~ HACK: static sized array types are passed by reference.
		if (isCall && arg->type->kind == TYPE_arr) {
			//~ static size arrays are passed as pointer
			if (arg->type->init != NULL) {
				arg->cast = TYPE_ref;
				arg->size = vm_size;
			}
		}

		arg->nest = 0;
		arg->offs = align ? base + stdiff : base;
		stdiff += padded(arg->size, align);

		if (align == 0 && stdiff < arg->size) {
			stdiff = arg->size;
		}
	}
	//~ because args are evaluated from right to left
	if (isCall) {
		for (arg = sym->prms; arg; arg = arg->next) {
			arg->offs = stdiff - arg->offs;
		}
	}
	return stdiff;
}

//~ scoping
void enter(ccState cc, astn ast) {
	//~ dieif(!s->_cnt, "FixMe: invalid ccState");
	cc->nest += 1;

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
symn leave(ccState cc, symn dcl, int mkstatic) {
	int i;
	state rt = cc->s;
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
	while (rt->defs && cc->nest < rt->defs->nest) {
		symn sym = rt->defs;

		// pop from stack
		rt->defs = sym->defs;

		sym->next = NULL;

		// declared in: (structure, function or wathever)
		sym->decl = dcl ? dcl : cc->func;

		if (mkstatic) {
			sym->stat = 1;
		}
		if (!sym->stat && !cc->siff && sym->call && sym->init && sym->init->kind == STMT_beg) {
			warn(cc->s, 1, sym->file, sym->line, "marking function to be static: `%-T`", sym);
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
			if (sym->kind == TYPE_ref && !sym->stat && sym->init) {
				warn(cc->s, 8, sym->file, sym->line, "ignoring initialization of non static member `%+T`", sym);
			}
		}

		sym->next = result;
		result = sym;
	}
	//cc->defs = defs;

	return result;
}
