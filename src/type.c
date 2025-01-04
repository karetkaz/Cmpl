#include "type.h"
#include "tree.h"
#include "printer.h"
#include "compiler.h"
#include "util.h"
#include "code.h"

symn ccBegin(ccContext ctx, const char *name) {
	if (ctx == NULL || name == NULL) {
		dbgTrace(ERR_INTERNAL_ERROR);
		return NULL;
	}
	symn result = install(ctx, name, ATTR_stat | KIND_typ | CAST_vid, 0, ctx->type_rec, NULL);
	if (result == NULL) {
		return NULL;
	}
	enter(ctx, result->tag, result);
	result->doc = type_doc_builtin;
	return result;
}

symn ccExtend(ccContext ctx, symn sym) {
	if (ctx == NULL) {
		dbgTrace(ERR_INTERNAL_ERROR);
		return NULL;
	}
	if (sym == NULL) {
		// cannot extend nothing
		return NULL;
	}
	if (sym->fields != NULL) {
		dbgTrace(ERR_INTERNAL_ERROR);
		return NULL;
	}
	enter(ctx, sym->tag, sym);
	return sym;
}

symn ccEnd(ccContext ctx, symn sym) {
	if (ctx == NULL) {
		dbgTrace(ERR_INTERNAL_ERROR);
		return NULL;
	}
	if (ctx->owner != sym) {
		dbgTrace(ERR_INTERNAL_ERROR);
		return NULL;
	}
	symn fields = leave(ctx, sym->kind & (ATTR_stat | MASK_kind), 0, 0, NULL, NULL);
	if (sym != NULL) {
		sym->fields = fields;
	}
	return fields;
}

symn ccDefInt(ccContext ctx, const char *name, int64_t value) {
	if (ctx == NULL || name == NULL) {
		dbgTrace(ERR_INTERNAL_ERROR);
		return NULL;
	}
	name = ccUniqueStr(ctx, name, -1, -1);
	symn result = install(ctx, name, ATTR_stat | KIND_def | CAST_i64, 0, ctx->type_i64, intNode(ctx, value));
	if (result == NULL) {
		return NULL;
	}
	result->doc = type_doc_builtin;
	return result;
}

symn ccDefFlt(ccContext ctx, const char *name, double value) {
	if (ctx == NULL || name == NULL) {
		dbgTrace(ERR_INTERNAL_ERROR);
		return NULL;
	}
	name = ccUniqueStr(ctx, name, -1, -1);
	symn result = install(ctx, name, ATTR_stat | KIND_def | CAST_f64, 0, ctx->type_f64, fltNode(ctx, value));
	if (result == NULL) {
		return NULL;
	}
	result->doc = type_doc_builtin;
	return result;
}

symn ccDefStr(ccContext ctx, const char *name, const char *value) {
	if (ctx == NULL || name == NULL) {
		dbgTrace(ERR_INTERNAL_ERROR);
		return NULL;
	}
	name = ccUniqueStr(ctx, name, -1, -1);
	if (value != NULL) {
		value = ccUniqueStr(ctx, value, -1, -1);
	}
	symn result = install(ctx, name, ATTR_stat | KIND_def | CAST_arr, 0, ctx->type_str, strNode(ctx, value));
	if (result == NULL) {
		return NULL;
	}
	result->doc = type_doc_builtin;
	return result;
}

symn ccDefVar(ccContext ctx, const char *name, symn type) {
	if (ctx == NULL || name == NULL) {
		dbgTrace(ERR_INTERNAL_ERROR);
		return NULL;
	}
	name = ccUniqueStr(ctx, name, -1, -1);
	symn result = install(ctx, name, KIND_var | refCast(type), 0, type, NULL);
	if (result == NULL) {
		return NULL;
	}
	result->doc = type_doc_builtin;
	return result;
}

symn ccAddType(ccContext ctx, const char *name, unsigned size, int refType) {
	if (ctx == NULL || name == NULL) {
		dbgTrace(ERR_INTERNAL_ERROR);
		return NULL;
	}
	symn result = install(ctx, name, ATTR_stat | KIND_typ | (refType ? CAST_ptr : CAST_val), size, ctx->type_rec, NULL);
	if (result == NULL) {
		return NULL;
	}
	result->doc = type_doc_builtin;
	return result;
}

// fixme: remove
symn ccLookup(ccContext ctx, symn scope, char *name) {
	rtContext rt = ctx->rt;
	struct astNode ast = {0};
	ast.kind = TOKEN_var;
	ast.id.name = name;
	ast.id.hash = rehash(name, -1);
	if (scope == NULL) {
		if (rt->main != NULL) {
			// code was generated, globals are in the main fields
			scope = rt->main->fields;
		}
		else if (rt->cc != NULL) {
			// code was not executed, main not generated
			scope = rt->cc->symbolStack[ast.id.hash % lengthOf(ctx->symbolStack)];
		}
	}
	return lookup(rt->cc, scope, &ast, NULL, 0, 1);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Low level methods

static symn aliasOf(symn sym) {
	while (sym != NULL) {
		if (!isInline(sym)) {
			break;
		}
		if (isInvokable(sym)) {
			break;
		}
		if (sym->init == NULL) {
			sym = sym->type;
		}
		else if (sym->init->kind == TOKEN_var) {
			sym = sym->init->id.link;
		}
		else {
			break;
		}
	}
	return sym;
}

static int isOwnerScope(ccContext ctx) {
	if (ctx->nest <= 0 || ctx->owner == NULL) {
		// global or out of scope
		return 0;
	}
	if (!isTypename(ctx->owner)) {
		return 0;
	}
	astn ast = ctx->scopeStack[ctx->nest - 1];
	if (ctx->owner != linkOf(ast, 0)) {
		return 0;
	}
	return 1;
}


symn newSymn(ccContext ctx, ccKind kind) {
	rtContext rt = ctx->rt;
	symn result = NULL;

	dieif(rt->vm.nfc, "Can not create symbols while generating bytecode");
	rt->_beg = padPointer(rt->_beg, vm_mem_align);
	if (rt->_beg >= rt->_end) {
		fatal(ERR_MEMORY_OVERRUN);
		return NULL;
	}

	result = (symn)rt->_beg;
	rt->_beg += sizeof(struct symNode);
	memset(result, 0, sizeof(struct symNode));
	result->kind = kind;

	return result;
}

void enter(ccContext ctx, astn node, symn owner) {
	dieif(ctx->rt->vm.nfc, "Compiler state closed");

	dieif(ctx->nest >= maxTokenCount, "Out of scope");
	ctx->scopeStack[ctx->nest] = node;
	ctx->nest += 1;

	if (owner != NULL) {
		owner->owner = ctx->owner;
		ctx->owner = owner;
	}
}

symn leave(ccContext ctx, ccKind mode, size_t align, size_t baseSize, size_t *outSize, symn result) {
	dieif(ctx->rt->vm.nfc, "Compiler state closed");
	symn sym;
	symn owner = ctx->owner;

	ctx->nest -= 1;
	if (owner && owner->nest >= ctx->nest) {
		ctx->owner = owner->owner;
	} else {
		owner = NULL;
	}

	// clear from symbol table
	for (size_t i = 0; i < lengthOf(ctx->symbolStack); i++) {
		for (sym = ctx->symbolStack[i]; sym && sym->nest > ctx->nest; sym = sym->next) {
		}
		ctx->symbolStack[i] = sym;
	}

	// clear from scope stack
	for (sym = ctx->scope; sym && sym->nest > ctx->nest; sym = sym->scope) {
		sym->kind |= mode & MASK_attr;
		sym->owner = owner;

		// add to the list of scope symbols
		sym->next = result;
		result = sym;

		if (isInline(sym) && isInvokable(sym) && !isStatic(sym)) {
			if ((mode & MASK_kind) == KIND_typ) {
				// inline declarations are static
				sym->kind |= ATTR_stat;
			}
		}

		if (isTypename(sym) && !isStatic(sym)) {
			// types must be marked as static
			fatal(ERR_INTERNAL_ERROR);
		}
		// add to the list of global symbols
		if (isStatic(sym)) {
			if (!ctx->inStaticIfFalse && sym->global == NULL) {
				sym->global = ctx->global;
				ctx->global = sym;
			}
		}
	}
	ctx->scope = sym;

	// fix padding
	size_t offs = baseSize;
	size_t size = baseSize;
	switch (mode & MASK_kind) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			break;

		case KIND_typ:
			// update offsets
			for (sym = result; sym != NULL; sym = sym->next) {
				if (isStatic(sym)) {
					continue;
				}
				if (sym->owner != owner) {
					break;
				}
				sym->offs = padOffset(offs, align < sym->size ? align : sym->size);
				if (offs < sym->offs) {
					debug(ctx->rt, sym->file, sym->line, WARN_PADDING_ALIGNMENT, sym, sym->offs - offs, offs, sym->offs);
				}
				offs = sym->offs + sym->size;
				if (size < offs) {
					size = offs;
				}
			}
			size_t padded = padOffset(size, align);
			if (align && size != padded) {
				const char *file = owner ? owner->file : NULL;
				int line = owner ? owner->line : 0;
				debug(ctx->rt, file, line, WARN_PADDING_ALIGNMENT, owner, padded - size, size, padded);
				size = padded;
			}
			break;

		case KIND_fun:
			// update offsets
			for (sym = result; sym != NULL; sym = sym->next) {
				if (isStatic(sym)) {
					continue;
				}
				sym->size = padOffset(sym->size, align);
				sym->offs = offs += sym->size;
			}
			break;

		case KIND_def:
			//
			break;
	}

	if (outSize != NULL) {
		*outSize = size;
	}

	return result;
}

symn install(ccContext ctx, const char *name, ccKind kind, size_t size, symn type, astn init) {
	if (ctx == NULL || name == NULL) {
		fatal(ERR_INTERNAL_ERROR);
		return NULL;
	}

	// initializer must be type checked
	if (init && init->type == NULL) {
		fatal(ERR_INTERNAL_ERROR);
		return NULL;
	}

	if (size == 0 && (kind & MASK_kind) == KIND_var) {
		size = refSize(type);
	}

	symn def = newSymn(ctx, kind);
	if (def != NULL) {
		size_t length = strlen(name) + 1;
		unsigned hash = rehash(name, length);
		def->name = ccUniqueStr(ctx, (char *) name, length, hash);
		def->owner = ctx->owner;
		def->nest = ctx->nest;
		def->type = type;
		def->init = init;
		def->size = size;

		if ((kind & MASK_kind) == KIND_typ) {
			def->offs = vmOffset(ctx->rt, def);
			if (type != NULL && type != ctx->type_rec) {
				int maxChain = 16;
				symn obj = type;
				while (obj != NULL) {
					if ((maxChain -= 1) < 0) {
						error(ctx->rt, ctx->file, ctx->line, ERR_DECLARATION_COMPLEX, def);
						break;
					}

					// stop on typename (int32 base type is typename)
					if (obj == ctx->type_rec) {
						break;
					}

					// stop on object (inheritance chain)
					if (obj == ctx->type_obj) {
						def->kind = (def->kind & ~MASK_cast) | CAST_obj;
						break;
					}
					obj = obj->type;
				}
				if (obj == NULL) {
					error(ctx->rt, ctx->file, ctx->line, ERR_DECLARATION_COMPLEX, def);
				}
			}
		}

		if (isOwnerScope(ctx)) {
			def->next = ctx->owner->fields;
			ctx->owner->fields = def;
		} else {
			hash %= lengthOf(ctx->symbolStack);
			def->next = ctx->symbolStack[hash];
			ctx->symbolStack[hash] = def;
		}
		def->scope = ctx->scope;
		ctx->scope = def;
	}
	return def;
}

symn lookup(ccContext ctx, symn sym, astn ref, astn arguments, ccKind filter, int raise) {
	symn best = NULL;
	int found = 0;

	dieif(!ref || ref->kind != TOKEN_var, ERR_INTERNAL_ERROR);

	astn chainedArgs = chainArgs(arguments);
	if (arguments == ctx->void_tag) {
		chainedArgs = NULL;
	}

	for (; sym; sym = sym->next) {

		if (sym->name == NULL || strcmp(sym->name, ref->id.name) != 0) {
			// exclude anonymous symbols or non-matching names
			continue;
		}

		if (arguments != NULL && !isInvokable(sym) && !isTypename(aliasOf(sym))) {
			// exclude non-invocable values looking up invocation
			continue;
		}

		if (isFiltered(sym, filter)) {
			continue;
		}

		// lookup function by name (without arguments)
		symn parameter = sym->params;
		if (parameter != NULL && arguments == NULL) {
			if (best && best->owner && sym->owner && best->owner != sym->owner) {
				// declared in base class
				continue;
			}
			// keep the first match.
			if (best == NULL) {
				best = sym;
			}
			found += 1;
			continue;
		}

		int hasCast = 0;
		if (arguments != NULL) {
			astn argument = chainedArgs;

			if (parameter != NULL) {
				// first argument starts next to result.
				parameter = parameter->next;
			}

			if (sym == ctx->libc_dbg) {
				dieif(parameter == NULL, ERR_INTERNAL_ERROR);
				dieif(parameter->next == NULL, ERR_INTERNAL_ERROR);
				dieif(parameter->next->next == NULL, ERR_INTERNAL_ERROR);
				// debug has 2 hidden params: file and line.
				parameter = parameter->next->next;
			}

			while (parameter != NULL && argument != NULL) {
				if (!canAssign(ctx, parameter, argument, 0)) {
					break;
				}
				if (!canAssign(ctx, parameter, argument, 1)) {
					hasCast += 1;
				}

				if ((parameter->kind & ARGS_varg) != 0 && argument->kind != PNCT_dot3) {
					dieif(castOf(parameter->type) != CAST_arr, ERR_INTERNAL_ERROR);
					dieif(parameter->next != NULL, ERR_INTERNAL_ERROR);
					argument = argument->next;
					continue;
				}
				parameter = parameter->next;
				argument = argument->next;
			}

			if (argument != NULL && parameter != NULL) {
				// parameters and arguments can not be assigned
				continue;
			}
			else if (parameter != NULL) {
				// more parameter than arguments
				if ((parameter->kind & ARGS_varg) == 0) {
					continue;
				}
			}
			else if (argument != NULL) {
				// type cast can have one single argument: `int(3.14)`
				if (argument != arguments) {
					continue;
				}
				symn base = aliasOf(sym);

				// type cast must have on the left a typename.
				if (!isTypename(base)) {
					continue;
				}

				// allow type cast of emit: `Complex(emit(...))`
				if (argument->type != ctx->emit_opc) {
					// but only for types defined by user
					if (castOf(base) == CAST_val) {
						continue;
					}
				}
			}
		}

		// return first perfect match
		if (hasCast == 0) {
			break;
		}

		// keep first match as the best one
		if (best == NULL) {
			best = sym;
		}

		found += 1;
		// if we are here then sym is found, but it has implicit cast in it
		dbgInfo("%?s:%?u: %t(%t) is probably %T", ref->file, ref->line, ref, arguments, sym);
	}

	if (sym != NULL) {
		// perfect match found without casts
		return aliasOf(sym);
	}

	if (best != NULL && found == 1) {
		// found one single compatible match, use it.
		return aliasOf(best);
	}

	if (best == NULL) {
		// not found
		if (raise) {
			error(ctx->rt, ref->file, ref->line, ERR_UNDEFINED_DECLARATION, ref);
		}
		return NULL;
	}

	if (arguments == NULL) {
		// found multiple variables or functions with the same name
		if (raise) {
			error(ctx->rt, ref->file, ref->line, ERR_MULTIPLE_OVERLOADS, found, ref);
		}
		return NULL;
	}

	if (raise) {
		warn(ctx->rt, ref->file, ref->line, WARN_USING_BEST_OVERLOAD, best, found);
	}
	return aliasOf(best);
}

astn castTo(ccContext ctx, astn ast, symn type) {
	dieif(ctx->rt->vm.nfc, "Compiler state closed");
	if (type == NULL) {
		return ast;
	}
	if (ast->type == NULL) {
		ast->type = type;
		return ast;
	}
	if (ast->type == type) {
		// no update required.
		return ast;
	}

	debug(ctx->rt, ast->file, ast->line, WARN_ADDING_IMPLICIT_CAST, type, ast, ast->type);
	astn value = newNode(ctx, OPER_fnc);
	value->file = ast->file;
	value->line = ast->line;
	value->op.rhso = ast;
	value->type = type;
	return value;
}

size_t argsSize(symn function) {
	size_t result = 0;
	if (!isFunction(function)) {
		fatal(ERR_INTERNAL_ERROR);
		return 0;
	}
	for (symn prm = function->params; prm; prm = prm->next) {
		if (isInline(prm)) {
			continue;
		}
		size_t offs = prm->offs;
		if (result < offs) {
			result = offs;
		}
	}
	return result;
}
