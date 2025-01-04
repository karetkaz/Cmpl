#include "printer.h"
#include "compiler.h"
#include "tree.h"
#include "type.h"

// determine the resulting type of a OP b
// TODO: remove this, and declare each operator in the language
static symn promote(ccContext cc, symn lht, symn rht) {
	if (rht == NULL) {
		return NULL;
	}
	switch (refCast(rht)) {
		default: break;
		case CAST_i32:
		case CAST_u32:
			if (rht->size < 4 && cc != NULL) {
				rht = cc->type_i32;
			}
	}

	if (lht == NULL) {
		return rht;
	}
	switch (refCast(lht)) {
		default: break;
		case CAST_i32:
		case CAST_u32:
			if (lht->size < 4 && cc != NULL) {
				lht = cc->type_i32;
			}
	}

	if (lht == rht) {
		return rht;
	}
	switch (refCast(rht)) {
		default: break;

		case CAST_val:
			if (cc != NULL && lht == cc->type_ptr) {
				// ex: null == x
				return lht;
			}
			break;

		case CAST_ptr:
		case CAST_obj:
			switch (refCast(lht)) {
				default: break;

				case CAST_val:
					if (cc != NULL && rht == cc->type_ptr) {
						// ex: x == pointer(y)
						return rht;
					}
					break;

				case CAST_ptr:
				case CAST_obj:
					return rht;
			}
			break;

		case CAST_i32:
		case CAST_i64:
		case CAST_u32:
		case CAST_u64:
			switch (refCast(lht)) {
				default: break;
				case CAST_i32:
				case CAST_i64:
					// prefer unsigned in case the same size
					return lht->size > rht->size ? lht : rht;

				case CAST_u32:
				case CAST_u64:
					// prefer unsigned in case the same size
					return lht->size >= rht->size ? lht : rht;

				case CAST_f32:
				case CAST_f64:
					return lht;
			}
			break;

		case CAST_f32:
		case CAST_f64:
			switch (refCast(lht)) {
				default: break;

				case CAST_i32:
				case CAST_i64:
				case CAST_u32:
				case CAST_u64:
					return rht;

				case CAST_f32:
				case CAST_f64:
					return lht->size >= rht->size ? lht : rht;
			}
			break;
	}
	return NULL;
}

// todo: remove this allowing non null references to be assigned null
static inline int canAssignOptional(symn sym) {
	return castOf(sym) != CAST_any;
}
ccKind canAssign(ccContext ctx, symn variable, astn value, int strict) {
	if (variable == NULL || value == NULL || value->type == NULL) {
		dieif(!variable, ERR_INTERNAL_ERROR);
		dieif(!value, ERR_INTERNAL_ERROR);
		return CAST_any;
	}

	symn valueRef = linkOf(value, 1);
	symn type = variable->type;
	if (isTypename(variable)) {
		// canAssign(ctx, int32, 0, strict)
		type = variable;
	}

	// deny list
	if (!isOptional(variable) && !canAssignOptional(variable)) {
		if (valueRef != NULL && valueRef == ctx->null_ref) {
			// info(ctx->rt, value->file, value->line, ERR_INVALID_VALUE_ASSIGN, variable, value);
			return CAST_any;
		}
		if (valueRef != NULL && isOptional(valueRef)) {
			return CAST_any;
		}
		if (isOptional(value->type)) {
			return CAST_any;
		}
	}

	dieif(type == NULL, ERR_INTERNAL_ERROR);

	if (isIndirect(variable) && (value->type == ctx->type_ptr || value->type == ctx->type_var)) {
		// FIXME: pointers and variants can be assigned to any reference
		return refCast(type);
	}

	if (valueRef != NULL && (type == ctx->type_ptr || type == ctx->type_var)) {
		// assigning a variable to a pointer or variant
		return refCast(type);
	}

	if (value->type == ctx->emit_opc && variable->size == value->type->size) {
		// can assign with result of emit, only if is the same size
		return refCast(type);
	}

	// functions can be assigned only if the parameter types matches
	if (variable->type == ctx->type_fun) {
		if (valueRef == NULL || valueRef->type != ctx->type_fun) {
			// function can be assigned to function
			return CAST_any;
		}

		symn arg1 = variable->params;
		symn arg2 = valueRef->params;
		struct astNode temp = {0};

		temp.kind = TOKEN_var;
		temp.type = type;
		temp.id.link = variable;

		while (arg1 != NULL && arg2 != NULL) {
			temp.type = arg2->type;
			temp.id.link = arg2;

			if (!canAssign(ctx, arg1, &temp, 1)) {
				dbgInfo("%T ~ %T", arg1, arg2);
				break;
			}

			arg1 = arg1->next;
			arg2 = arg2->next;
		}

		if (arg1 != NULL || arg2 != NULL) {
			dbgInfo("%T ~ %T", arg1, arg2);
			return CAST_any;
		}

		// function is by ref
		strict = 1;
	}

	if (strict && !isIndirect(variable) != !isIndirect(valueRef ? valueRef : type)) {
		if (isIndirect(variable) && isMutable(variable)) {
			return CAST_any;
		}
	}

	ccKind varCast = castOf(variable);
	if (!strict && type != variable) {
		strict = varCast == CAST_ptr || varCast == CAST_obj;
	}

	// assigning a value to an object or base type
	for (symn sym = value->type; sym != NULL; sym = sym->type) {
		if (sym == type) {
			return varCast;
		}
		// TODO: there should be a check if object is in the type hierarchy
		if ((varCast != CAST_ptr && varCast != CAST_obj) || (castOf(sym) != CAST_ptr && castOf(sym) != CAST_obj)) {
			// check base type only if it is a reference type
			break;
		}
		if (isArrayType(sym)) {
			break;
		}
		if (isFunction(sym)) {
			break;
		}
		if (sym == ctx->type_rec) {
			// typename is the last type in the chain
			break;
		}
	}

	// Assign enum
	if (castOf(type) == CAST_enm) {
		if (valueRef == NULL) {
			return CAST_any;
		}
		if (type == valueRef->type) {
			// Enum eVal = Enum.value;
			// Enum eVar = eVal;
			return varCast;
		}
		return CAST_any;
	}
	// Assign array
	if (castOf(type) == CAST_arr) {
		if (valueRef == ctx->null_ref) {
			// todo: do not allow assigning null to array
			return castOf(type);
		}
		struct astNode temp = {0};
		symn vty = value->type;

		temp.kind = TOKEN_var;
		temp.type = vty ? vty->type : NULL;
		temp.id.link = NULL;
		temp.id.name = ".generated token";

		//~ check if subtypes are assignable
		if (canAssign(ctx, type->type, &temp, strict)) {
			// assign to dynamic array
			if (type->init == NULL) {
				return CAST_arr;
			}
			if (type->size == value->type->size) {
				// TODO: return <?>
				return KIND_var;
			}
		}

		if (!strict) {
			return canAssign(ctx, variable->type, value, strict);
		}
	}

	if (type == value->type) {
		return refCast(type);
	}

	if (!strict && (type = promote(ctx, type, value->type))) {
		// TODO: remove promotion
		return refCast(type);
	}

	dbgInfo("%?s:%?u: %T := %T(%t)", value->file, value->line, variable, value->type, value);
	return CAST_any;
}

static symn typeCheckRef(ccContext ctx, symn loc, astn ref, astn args, ccKind filter, int raise) {
	dieif(ctx->rt->vm.nfc, "Compiler state closed");
	if (ref == NULL || ref->kind != TOKEN_var) {
		dbgTraceAst(ref);
		return NULL;
	}

	symn sym = NULL;
	if (ref->id.link != NULL) {
		sym = ref->id.link;
	}
	else if (loc != NULL) {
		if (!isTypename(loc)) {
			// try to lookup members first.
			sym = lookup(ctx, loc->type->fields, ref, args, KIND_var, 0);
			if (sym == NULL) {
				// length of fixed size array is an inline static value
				sym = lookup(ctx, loc->type->fields, ref, args, 0, 0);
			}
		}

		if (sym == NULL) {
			sym = lookup(ctx, loc->fields, ref, args, filter, raise);
		}
	}
	else {
		// first lookup in the current scope
		loc = ctx->symbolStack[ref->id.hash % lengthOf(ctx->symbolStack)];
		sym = lookup(ctx, loc, ref, args, 0, 0);

		// lookup parameters, fields, etc.
		for (loc = ctx->owner; loc != NULL; loc = loc->owner) {
			if (sym != NULL && sym->nest > loc->nest) {
				// symbol found: scope is higher than this parameter
				break;
			}
			symn field = lookup(ctx, loc->fields, ref, args, 0, 0);
			if (field == NULL) {
				// symbol was not found at this location
				continue;
			}
			sym = field;
		}
	}

	if (sym == NULL) {
		if (raise) {
			if (loc == NULL) {
				loc = ctx->symbolStack[ref->id.hash % lengthOf(ctx->symbolStack)];
			}
			// lookup again to raise the error
			lookup(ctx, loc, ref, args, 0, raise);
		}
		return NULL;
	}

	symn type = sym->type;
	if (args != NULL) {
		if (isInvokable(sym)) {
			// resulting type is the type of result parameter
			type = sym->params->type;
		}
		if (isTypename(sym)) {
			// resulting type is the type of the cast
			type = sym;
		}
	}
	else if (isInline(sym) && sym->init != NULL && !isInvokable(sym)) {
		// resulting type is the type of the inline expression
		type = sym->init->type;
	}

	if (args != NULL && isInvokable(sym)) {
		astn arg = chainArgs(args);
		if (args == ctx->void_tag) {
			arg = NULL;
		}
		symn prm = sym->params->next;   // skip to first parameter
		if (sym == ctx->libc_dbg) {
			prm = prm->next;            // skip hidden param: file
			prm = prm->next;            // skip hidden param: line
		}
		while (arg != NULL && prm != NULL) {
			// reference arguments must be given as: `&var`
			if (arg->kind != OPER_adr && isIndirect(prm) && !isIndirect(prm->type) && isMutable(prm)) {
				if (!(filter == ARGS_this && args->kind == OPER_com && arg == args->op.lhso)) {
					warn(ctx->rt, arg->file, arg->line, WARN_PASS_ARG_BY_REF, arg, sym);
				}
			}

			if ((prm->kind & ARGS_varg) == ARGS_varg) {
				arg = arg->next;
				continue;
			}
			arg = arg->next;
			prm = prm->next;
		}
		if (prm && (prm->kind & ARGS_varg) == ARGS_varg) {
			prm = prm->next;
		}
		dieif(arg || prm, ERR_INTERNAL_ERROR": %t(%t)[%t => %T]", ref, args, arg, prm);
	}

	// raise error or warning if accessing private symbols
	if (sym->unit != ctx->unit && sym->doc == NULL) {
		raiseLevel level = ref->file == NULL ? raiseDebug : ctx->errPrivateAccess ? raiseError : raiseWarn;
		printLog(ctx->rt, level, ref->file, ref->line, ERR_PRIVATE_DECLARATION, sym);
		if (sym->file && sym->line) {
			printLog(ctx->rt, level, sym->file, sym->line, INFO_PREVIOUS_DEFINITION, sym);
		}
	}

	dieif(ref->kind != TOKEN_var, ERR_INTERNAL_ERROR);
	ref->id.link = sym;
	ref->type = sym->type;
	addUsage(sym, ref);
	return type;
}

symn typeCheck(ccContext ctx, symn loc, astn ast, int raise) {
	symn lType = NULL;
	symn rType = NULL;
	symn type = NULL;
	astn tag = NULL;
	astn arg = NULL;

	if (ast == NULL) {
		fatal(ERR_INTERNAL_ERROR);
		return NULL;
	}

	// do not type-check again
	if (ast->type != NULL) {
		return ast->type;
	}

	switch (ast->kind) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return NULL;

		case OPER_fnc: {
			astn ref = ast->op.lhso;
			astn args = ast->op.rhso;

			if (ref == NULL && args == NULL) {
				// int a = ();
				if (raise) {
					error(ctx->rt, ast->file, ast->line, ERR_INVALID_TYPE, ast);
				}
				dbgTraceAst(ast);
				return NULL;
			}
			if (ref == NULL) {
				// int a = (3 + 6);
				rType = typeCheck(ctx, loc, args, raise);
				ast->op.rhso = castTo(ctx, args, rType);
				ast->type = rType;
				return rType;
			}
			if (args == NULL) {
				// int a = func();
				args = ctx->void_tag;
			}

			if (ref->kind == RECORD_kwd) {
				// typename t = struct(a);
				rType = typeCheck(ctx, loc, args, 0);
				if (rType == NULL) {
					// hack: the expression can not be type checked
					args->type = ctx->null_ref;
				}
				if (ast->op.rhso == NULL) {
					ast->op.rhso = args;
				}

				ref->type = ast->type = ctx->type_rec;
				return ast->type;
			}
			if (ref->kind == INLINE_kwd) {
				// emit may contain instructions, but those are hidden in emit (like: emit.i64.sub).
				typeCheck(ctx, ctx->emit_opc, args, 0);

				// lookup first in emit scope, then what failed in current scope
				rType = typeCheck(ctx, loc, args, raise);

				if (rType == NULL) {
					dbgTraceAst(ast);
					return NULL;
				}

				ref->type = ast->type = ctx->emit_opc;
				return ast->type;
			}
			if (ref->kind == TOKEN_var) {
				type = ctx->symbolStack[ref->id.hash % lengthOf(ctx->symbolStack)];
				lookup(ctx, type, ref, NULL, 0, 0);
			}

			// lookup arguments in the current scope
			rType = typeCheck(ctx, loc, args, raise);
			if (rType == NULL) {
				dbgTraceAst(ast);
				return NULL;
			}

			if (ref->kind == OPER_dot) {    // float32.sin(args)
				if (!typeCheck(ctx, loc, ref->op.lhso, raise)) {
					dbgTraceAst(ast);
					return NULL;
				}
				loc = linkOf(ref->op.lhso, 1);
				if (loc != NULL && isTypename(loc)) {
					ref->type = ctx->type_fun;
					ref = ref->op.rhso;
				}
				else {
					/* lookup order for `a.add(b)`
						1. search for matching method: `a.add(b)`
						2. search for extension method: `add(a, b)`
						3. search for virtual method: `a.add(a, b)`
						4. search for static method: `T.add(a, b)`
					*/

					if (loc == NULL) {
						// in case `a` is an expression use its type
						loc = ref->op.lhso->type;
					}

					// 1. search for the matching method: a.add(b)
					type = typeCheckRef(ctx, loc, ref->op.rhso, args, KIND_var, 0);
					if (type != NULL) {
						ref->type = type;
						dbgInfo("exact function: %t", ast);
						return ast->type = type;
					}

					astn argThis = ref->op.lhso;
					// convert instance to parameter and try to lookup again
					if (args == ctx->void_tag) {
						args = argThis;
					}
					else {
						args = argNode(ctx, argThis, args);
					}
					ref = ref->op.rhso;

					// 2. search for extension method: add(a, b)
					type = typeCheckRef(ctx, NULL, ref, args, ARGS_this, 0);
					if (type != NULL) {
						ast->op.lhso = ref;
						ast->op.rhso = args;
						dbgInfo("extension function: %t", ast);
						return ast->type = type;
					}

					// 3. search for virtual method: a.add(a, b)
					// 4. search for static method: T.add(a, b)
					type = typeCheckRef(ctx, loc, ref, args, ARGS_this, raise);
					if (type != NULL) {
						ast->op.rhso = args;
						ast->op.lhso->type = type;
						if (isStatic(linkOf(ref, 0))) {
							// replace variable with typename for case 4.
							ast->op.lhso->op.lhso = lnkNode(ctx, loc->type);
						}
						return ast->type = type;
					}

					ast->type = type;
					return type;
				}
			}

			if (loc != NULL && isTypename(loc)) {
				// lookup only static members
				type = typeCheckRef(ctx, loc, ref, args, ATTR_stat, raise);
			} else {
				type = typeCheckRef(ctx, loc, ref, args, 0, raise);
			}
			if (type == NULL) {
				dbgTraceAst(ast);
				return NULL;
			}
			ast->type = type;
			return type;
		}

		case OPER_dot:
			if (loc == ctx->emit_opc && ast->op.lhso->kind == TOKEN_var) {
				// Fixme: if we have add.i32 and add was already checked, force a new lookup
				ast->op.lhso->type = NULL;
				ast->op.lhso->id.link = NULL;
			}
			lType = typeCheck(ctx, loc, ast->op.lhso, raise);
			// if left side is a function or emit, we need to lookup parameters declared for the function
			loc = linkOf(ast->op.lhso, 1);
			if (loc == NULL) {
				loc = lType;
			}
			if (isTypeExpr(ast->op.lhso)) {
				rType = typeCheckRef(ctx, loc, ast->op.rhso, NULL, ATTR_stat, raise);
			} else {
				rType = typeCheckRef(ctx, loc, ast->op.rhso, NULL, KIND_var, 0);
				if (rType == NULL) {
					rType = typeCheckRef(ctx, loc, ast->op.rhso, NULL, 0, raise);
				}
			}

			if (!lType || !rType || !loc) {
				dbgTraceAst(ast);
				return NULL;
			}
			ast->op.lhso = castTo(ctx, ast->op.lhso, lType);
			ast->op.rhso = castTo(ctx, ast->op.rhso, rType);
			ast->type = rType;
			return rType;

		case OPER_idx:
			if (ast->op.lhso != NULL) {
				lType = typeCheck(ctx, loc, ast->op.lhso, raise);
			}
			if (ast->op.rhso != NULL) {
				rType = typeCheck(ctx, NULL, ast->op.rhso, raise);
			}

			if (!lType || !rType) {
				dbgTraceAst(ast);
				return NULL;
			}

			if (ast->op.rhso && ast->op.rhso->kind == PNCT_dot3) {
				if (!isArrayType(lType)) {
					dbgTraceAst(ast);
					return NULL;
				}
				type = lType;
			} else {
				type = lType->type;
			}

			// base type of array;
			ast->type = type;
			return type;

		case OPER_adr:		// '&'
		case PNCT_dot3:		// '...'
		case OPER_pls:		// '+'
		case OPER_mns:		// '-'
		case OPER_cmt:		// '~'
			rType = typeCheck(ctx, loc, ast->op.rhso, raise);
			if (!rType) {
				dbgTraceAst(ast);
				return NULL;
			}

			tag = tagNode(ctx, token_tbl[ast->kind].op);
			arg = ast->op.rhso;
			type = typeCheckRef(ctx, NULL, tag, arg, 0, 0);
			if (type != NULL) {
				ast->kind = OPER_fnc;
				ast->type = type;
				ast->op.lhso = tag;
				ast->op.rhso = arg;
				return type;
			}
			recycle(ctx, tag);

			if (ast->kind == OPER_adr || ast->kind == PNCT_dot3) {
				if ((type = promote(NULL, NULL, rType)) == NULL) {
					error(ctx->rt, ast->file, ast->line, ERR_INVALID_TYPE, ast);
				}
			} else {
				if ((type = promote(ctx, NULL, rType)) == NULL) {
					error(ctx->rt, ast->file, ast->line, ERR_INVALID_TYPE, ast);
				}
			}
			ast->op.rhso = castTo(ctx, ast->op.rhso, type);
			ast->type = type;
			return type;

		case OPER_not:		// '!'
			rType = typeCheck(ctx, loc, ast->op.rhso, raise);

			if (!rType) {
				dbgTraceAst(ast);
				return NULL;
			}
			type = ctx->type_bol;
			ast->op.rhso = castTo(ctx, ast->op.rhso, type);
			ast->type = type;
			return type;

		case OPER_add:		// '+'
		case OPER_sub:		// '-'
		case OPER_mul:		// '*'
		case OPER_div:		// '/'
		case OPER_mod:		// '%'
			lType = typeCheck(ctx, loc, ast->op.lhso, raise);
			rType = typeCheck(ctx, loc, ast->op.rhso, raise);
			if (lType == NULL || rType == NULL) {
				dbgTraceAst(ast);
				return NULL;
			}

			tag = tagNode(ctx, token_tbl[ast->kind].op);
			arg = argNode(ctx, ast->op.lhso, ast->op.rhso);
			type = typeCheckRef(ctx, NULL, tag, arg, 0, 0);
			if (type != NULL) {
				ast->kind = OPER_fnc;
				ast->type = type;
				ast->op.lhso = tag;
				ast->op.rhso = arg;
				return type;
			}
			recycle(ctx, tag);
			recycle(ctx, arg);

			if ((type = promote(ctx, lType, rType)) == NULL) {
				error(ctx->rt, ast->file, ast->line, ERR_INVALID_TYPE, ast);
			}
			ast->op.lhso = castTo(ctx, ast->op.lhso, type);
			ast->op.rhso = castTo(ctx, ast->op.rhso, type);
			ast->type = type;
			return type;

		case OPER_shl:		// '>>'
		case OPER_shr:		// '<<'
			lType = typeCheck(ctx, loc, ast->op.lhso, raise);
			rType = typeCheck(ctx, loc, ast->op.rhso, raise);
			if (lType == NULL || rType == NULL) {
				dbgTraceAst(ast);
				return NULL;
			}

			tag = tagNode(ctx, token_tbl[ast->kind].op);
			arg = argNode(ctx, ast->op.lhso, ast->op.rhso);
			type = typeCheckRef(ctx, NULL, tag, arg, 0, 0);
			if (type != NULL) {
				ast->kind = OPER_fnc;
				ast->type = type;
				ast->op.lhso = tag;
				ast->op.rhso = arg;
				return type;
			}
			recycle(ctx, tag);
			recycle(ctx, arg);

			if ((type = promote(ctx, lType, rType)) == NULL) {
				error(ctx->rt, ast->file, ast->line, ERR_INVALID_TYPE, ast);
			}
			ast->op.lhso = castTo(ctx, ast->op.lhso, type);
			ast->op.rhso = castTo(ctx, ast->op.rhso, ctx->type_i32);

			if (type != NULL) {
				switch (refCast(type)) {
					default:
						error(ctx->rt, ast->file, ast->line, ERR_INVALID_OPERATOR, ast, lType, rType);
						break;

					case CAST_i32:
					case CAST_i64:

					case CAST_u32:
					case CAST_u64:
						break;
				}
			}
			ast->type = type;
			return type;

		case OPER_and:		// '&'
		case OPER_ior:		// '|'
		case OPER_xor:		// '^'
			lType = typeCheck(ctx, loc, ast->op.lhso, raise);
			rType = typeCheck(ctx, loc, ast->op.rhso, raise);
			if (lType == NULL || rType == NULL) {
				dbgTraceAst(ast);
				return NULL;
			}

			tag = tagNode(ctx, token_tbl[ast->kind].op);
			arg = argNode(ctx, ast->op.lhso, ast->op.rhso);
			type = typeCheckRef(ctx, NULL, tag, arg, 0, 0);
			if (type != NULL) {
				ast->kind = OPER_fnc;
				ast->type = type;
				ast->op.lhso = tag;
				ast->op.rhso = arg;
				return type;
			}
			recycle(ctx, tag);
			recycle(ctx, arg);

			if ((type = promote(ctx, lType, rType)) == NULL) {
				error(ctx->rt, ast->file, ast->line, ERR_INVALID_TYPE, ast);
			}
			ast->op.lhso = castTo(ctx, ast->op.lhso, type);
			ast->op.rhso = castTo(ctx, ast->op.rhso, type);

			if (type != NULL) {
				switch (refCast(type)) {
					default:
						error(ctx->rt, ast->file, ast->line, ERR_INVALID_OPERATOR, ast, lType, rType);
						break;

					case CAST_bit:

					case CAST_i32:
					case CAST_i64:

					case CAST_u32:
					case CAST_u64:
						break;
				}
			}
			ast->type = type;
			return type;

		case OPER_ceq:		// '=='
		case OPER_cne:		// '!='
		case OPER_clt:		// '<'
		case OPER_cle:		// '<='
		case OPER_cgt:		// '>'
		case OPER_cge:		// '>='
			lType = typeCheck(ctx, loc, ast->op.lhso, raise);
			rType = typeCheck(ctx, loc, ast->op.rhso, raise);
			if (lType == NULL || rType == NULL) {
				dbgTraceAst(ast);
				return NULL;
			}

			tag = tagNode(ctx, token_tbl[ast->kind].op);
			arg = argNode(ctx, ast->op.lhso, ast->op.rhso);
			type = typeCheckRef(ctx, NULL, tag, arg, 0, 0);
			if (type != NULL) {
				ast->kind = OPER_fnc;
				ast->type = type;
				ast->op.lhso = tag;
				ast->op.rhso = arg;
				return type;
			}
			recycle(ctx, tag);
			recycle(ctx, arg);

			if ((type = promote(ctx, lType, rType)) == NULL) {
				error(ctx->rt, ast->file, ast->line, ERR_INVALID_TYPE, ast);
			}
			ast->op.lhso = castTo(ctx, ast->op.lhso, type);
			ast->op.rhso = castTo(ctx, ast->op.rhso, type);
			type = ctx->type_bol;
			ast->type = type;
			return type;

		case OPER_all:		// '&&'
		case OPER_any:		// '||'
			lType = typeCheck(ctx, loc, ast->op.lhso, raise);
			rType = typeCheck(ctx, loc, ast->op.rhso, raise);

			if (!lType || !rType) {
				dbgTraceAst(ast);
				return NULL;
			}
			type = ctx->type_bol;
			ast->op.lhso = castTo(ctx, ast->op.lhso, type);
			ast->op.rhso = castTo(ctx, ast->op.rhso, type);
			ast->type = type;
			return type;

		case OPER_sel:		// '?:'
			dieif(ast->op.rhso == NULL || ast->op.rhso->kind != OPER_cln, ERR_INTERNAL_ERROR);
			lType = typeCheck(ctx, loc, ast->op.rhso->op.lhso, raise);
			rType = typeCheck(ctx, loc, ast->op.rhso->op.rhso, raise);
			type = typeCheck(ctx, loc, ast->op.lhso, raise);

			if (!lType || !rType || !type) {
				dbgTraceAst(ast);
				return NULL;
			}
			if ((type = promote(ctx, lType, rType)) == NULL) {
				error(ctx->rt, ast->file, ast->line, ERR_INVALID_TYPE, ast);
			}
			ast->op.lhso = castTo(ctx, ast->op.lhso, ctx->type_bol);
			ast->op.rhso->op.lhso = castTo(ctx, ast->op.rhso->op.lhso, type);
			ast->op.rhso->op.rhso = castTo(ctx, ast->op.rhso->op.rhso, type);
			ast->op.rhso->type = type;
			ast->type = type;
			return type;

		case OPER_com:	// ','
			lType = typeCheck(ctx, loc, ast->op.lhso, raise);
			rType = typeCheck(ctx, loc, ast->op.rhso, raise);

			if (!lType || !rType) {
				dbgTraceAst(ast);
				return NULL;
			}
			ast->op.lhso = castTo(ctx, ast->op.lhso, lType);
			ast->op.rhso = castTo(ctx, ast->op.rhso, rType);
			type = ctx->type_vid;
			ast->type = type;
			return type;

		// operator set
		case INIT_set:		// ':='
		case ASGN_set:		// ':='
			lType = typeCheck(ctx, loc, ast->op.lhso, raise);
			rType = typeCheck(ctx, NULL, ast->op.rhso, raise);

			if (!lType || !rType) {
				dbgTraceAst(ast);
				return NULL;
			}

			if (!isMutableVar(ast->op.lhso) && ast->kind != INIT_set) {
				error(ctx->rt, ast->file, ast->line, ERR_INVALID_CONST_ASSIGN, ast);
			}
			if (!canAssign(ctx, lType, ast->op.rhso, 0)) {
				if (!canAssign(ctx, lType, ast->op.rhso, 0)) {
					error(ctx->rt, ast->file, ast->line, ERR_INVALID_VALUE_ASSIGN, lType, ast);
					return NULL;
				}
			}

			ast->op.rhso = castTo(ctx, ast->op.rhso, lType);
			ast->type = lType;
			return lType;

		// variable
		case TOKEN_var:
			type = typeCheckRef(ctx, loc, ast, NULL, 0, raise);
			if (type == NULL) {
				dbgTraceAst(ast);
				return NULL;
			}
			if (isEnumType(type)) {
				type = type->type;
			}
			ast->type = type;
			return type;
	}

	fatal(ERR_INTERNAL_ERROR": unimplemented: %.t (%T, %T): %t", ast, lType, rType, ast);
	return NULL;
}
