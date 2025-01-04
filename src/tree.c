#include "tree.h"
#include "type.h"
#include "compiler.h"
#include "printer.h"

const struct tokenRec token_tbl[256] = {
#define TOKEN_DEF(Name, Type, Args, Op, Text) {Text, Op, Type, Args},
#include "token.i"
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ create and recycle node
astn newNode(ccContext ctx, ccToken kind) {
	astn result = ctx->tokPool;
	rtContext rt = ctx->rt;

	dieif(rt->vm.nfc, "Can not create symbols while generating bytecode");

	if (result == NULL) {
		// allocate memory from temporary storage.
		rt->_end -= sizeof(struct astNode);
		if (rt->_beg >= rt->_end) {
			fatal(ERR_MEMORY_OVERRUN);
			return NULL;
		}
		result = (astn) rt->_end;
	}
	else {
		ctx->tokPool = result->next;
	}

	memset(result, 0, sizeof(struct astNode));
	result->kind = kind;
	return result;
}

astn dupNode(ccContext ctx, astn node) {
	astn result = newNode(ctx, node->kind);
	*result = *node;
	return result;
}

void recycle(ccContext ctx, astn node) {
	dieif(ctx->rt->vm.nfc, "Compiler state closed");
	if (node == NULL) {
		return;
	}
	node->next = ctx->tokPool;
	ctx->tokPool = node;
}

static void recycleTree(ccContext ctx, astn ast) {
	// TODO recycle recursively
	recycle(ctx, ast);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ make a constant valued node
astn intNode(ccContext ctx, int64_t value) {
	astn ast = newNode(ctx, TOKEN_val);
	if (ast != NULL) {
		ast->type = ctx->type_i64;
		ast->cInt = value;
	}
	return ast;
}

astn fltNode(ccContext ctx, float64_t value) {
	astn ast = newNode(ctx, TOKEN_val);
	if (ast != NULL) {
		ast->type = ctx->type_f64;
		ast->cFlt = value;
	}
	return ast;
}

astn strNode(ccContext ctx, const char *value) {
	astn ast = newNode(ctx, TOKEN_val);
	if (ast != NULL) {
		ast->type = ctx->type_str;
		ast->id.name = value;
		ast->id.hash = -1;
	}
	return ast;
}

astn lnkNode(ccContext ctx, symn ref) {
	astn result = newNode(ctx, TOKEN_var);
	if (result != NULL) {
		result->type = isTypename(ref) ? ref : ref->type;
		result->id.name = ref->name;
		result->id.link = ref;
		result->id.hash = -1;
	}
	return result;
}

astn opNode(ccContext ctx, symn type, ccToken kind, astn lhs, astn rhs) {
	astn result = newNode(ctx, kind);
	if (result != NULL) {
		result->op.lhso = lhs;
		result->op.rhso = rhs;
		result->type = type;
	}
	return result;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ get value of constant node
int bolValue(astn ast) {
	if (ast != NULL && ast->type != NULL && ast->kind == TOKEN_val) {
		switch (refCast(ast->type)) {
			default:
				break;

			case CAST_bit:
			case CAST_i32:
			case CAST_i64:
			case CAST_u32:
			case CAST_u64:
				return ast->cInt != 0;

			case CAST_f32:
			case CAST_f64:
				return ast->cFlt != 0;
		}
	}
	fatal(ERR_INVALID_CONST_EXPR, ast);
	return 0;
}

int64_t intValue(astn ast) {
	if (ast != NULL && ast->type != NULL && ast->kind == TOKEN_val) {
		switch (refCast(ast->type)) {
			default:
				break;

			case CAST_bit:
			case CAST_i32:
			case CAST_i64:
			case CAST_u32:
			case CAST_u64:
			case CAST_ptr:
				return ast->cInt;

			case CAST_f32:
			case CAST_f64:
				return (int64_t) ast->cFlt;
		}
	}
	fatal(ERR_INVALID_CONST_EXPR, ast);
	return 0;
}

float64_t fltValue(astn ast) {
	if (ast != NULL && ast->type != NULL && ast->kind == TOKEN_val) {
		switch (refCast(ast->type)) {
			default:
				break;

			case CAST_bit:
			case CAST_i32:
			case CAST_i64:
			case CAST_u32:
			case CAST_u64:
				return (float64_t) ast->cInt;

			case CAST_f32:
			case CAST_f64:
				return ast->cFlt;
		}
	}
	fatal(ERR_INVALID_CONST_EXPR, ast);
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ utility functions

ccKind eval(ccContext ctx, astn res, astn ast) {
	struct astNode lhs, rhs;

	if (ast == NULL || ast->type == NULL) {
		fatal(ERR_INTERNAL_ERROR);
		return CAST_any;
	}

	if (res == NULL) {
		// result may be null(not needed)
		res = &rhs;
	}

	if (res == ast) {
		// result may be null(not needed)
		ccKind result = eval(ctx, &rhs, ast);
		if (result != CAST_any) {
			if (token_tbl[res->kind].args) {
				recycleTree(ctx, res->op.lhso);
				recycleTree(ctx, res->op.rhso);
			}
			res->kind = rhs.kind;
			res->type = rhs.type;
			res->stmt = rhs.stmt;
		}
		return result;
	}

	symn type = ast->type;
	if (isEnumType(type)) {
		type = type->type;
	}
	ccKind cast = CAST_any;
	switch (castOf(type)) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return CAST_any;

		case CAST_bit:
			cast = CAST_bit;
			break;

		case CAST_i32:
		case CAST_u32:
		case CAST_i64:
		case CAST_u64:
			cast = CAST_i64;
			break;

		case CAST_f32:
		case CAST_f64:
			cast = CAST_f64;
			break;

		case CAST_ptr:
			cast = CAST_ptr;
			break;

		case CAST_vid:
		case CAST_val:
		case CAST_arr:
		case CAST_var:
		case CAST_obj:
			dbgInfo("not evaluable: %t", ast);
			return CAST_any;
	}

	memset(res, 0, sizeof(struct astNode));
	switch (ast->kind) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return CAST_any;

		case OPER_com:
		case STMT_beg:
		case TOKEN_opc:
			return CAST_any;

		case OPER_dot:
			// array.length is constant for fixed size arrays
			if (isArrayType(ast->op.lhso->type)) {
				symn arr = linkOf(ast->op.lhso, 0);
				if (!arr || castOf(arr) != CAST_val) {
					// not a fixed size array
					return CAST_any;
				}
				symn len = linkOf(ast->op.rhso, 0);
				if (!len || !isInline(len)) {
					// length must be an inline expression
					return CAST_any;
				}
				if (!isStatic(len) || isMutable(len)) {
					// extra caution, const and static should be also set for length
					return CAST_any;
				}
				return eval(ctx, res, len->init);
			}
			if (!isTypeExpr(ast->op.lhso)) {
				return CAST_any;
			}
			return eval(ctx, res, ast->op.rhso);

		case OPER_fnc: {
			astn func = ast->op.lhso;
			astn args = ast->op.rhso;
			if (func != NULL) {
				if (func->kind == RECORD_kwd) {
					// struct(variable)
					res->kind = TOKEN_val;
					res->type = ctx->type_rec;
					res->cInt = args->type->offs;
					break;
				}

				// can not evaluate functions
				dbgTraceAst(ast);
				return CAST_any;
			}

			if (args == NULL || !eval(ctx, res, args)) {
				// evaluate parametrized sub expressions: (3 + 5) * 3
				dbgTraceAst(ast);
				return CAST_any;
			}
			break;
		}

		case OPER_adr:
		case PNCT_dot3:
		case OPER_idx:
			return CAST_any;

		case TOKEN_val:
			*res = *ast;
			break;

		case TOKEN_var: {
			symn var = ast->id.link;		// link
			if (isInline(var)) {
				if (!eval(ctx, res, var->init)) {
					return CAST_any;
				}
				cast = refCast(type);
			}
			else if (isTypename(var)) {
				cast = refCast(type);
				type = ctx->type_rec;
				res->kind = TOKEN_val;
				res->cInt = var->offs;
			}
			else {
				return CAST_any;
			}
			break;
		}

		case OPER_pls:		// '+'
			if (!eval(ctx, res, ast->op.rhso)) {
				return CAST_any;
			}
			break;

		case OPER_mns:		// '-'
			if (!eval(ctx, res, ast->op.rhso)) {
				return CAST_any;
			}

			dieif(res->kind != TOKEN_val, ERR_INTERNAL_ERROR);
			switch (refCast(res->type)) {
				default:
					return CAST_any;

				case CAST_i32:
				case CAST_i64:
				case CAST_u32:
				case CAST_u64:
					res->cInt = -res->cInt;
					break;

				case CAST_f32:
				case CAST_f64:
					res->cFlt = -res->cFlt;
					break;
			}
			break;

		case OPER_cmt:			// '~'
			if (!eval(ctx, res, ast->op.rhso)) {
				return CAST_any;
			}

			dieif(res->kind != TOKEN_val, ERR_INTERNAL_ERROR);
			switch (refCast(res->type)) {
				default:
					return CAST_any;

				case CAST_i32:
				case CAST_i64:
				case CAST_u32:
				case CAST_u64:
					res->cInt = ~res->cInt;
					break;
			}
			break;

		case OPER_not:			// '!'
			if (!eval(ctx, res, ast->op.rhso)) {
				return CAST_any;
			}

			dieif(res->kind != TOKEN_val, ERR_INTERNAL_ERROR);
			switch (refCast(res->type)) {
				default:
					return CAST_any;

				case CAST_bit:
				case CAST_i32:
				case CAST_i64:
				case CAST_u32:
				case CAST_u64:
				case CAST_ptr:
				case CAST_obj:
					res->type = ctx->type_bol;
					res->cInt = !res->cInt;
					break;

				case CAST_f32:
				case CAST_f64:
					res->type = ctx->type_bol;
					res->cInt = !res->cFlt;
					break;
			}
			break;

		case OPER_add:			// '+'
		case OPER_sub:			// '-'
		case OPER_mul:			// '*'
		case OPER_div:			// '/'
		case OPER_mod:			// '%'
			if (!eval(ctx, &lhs, ast->op.lhso)) {
				return CAST_any;
			}

			if (!eval(ctx, &rhs, ast->op.rhso)) {
				return CAST_any;
			}

			dieif(lhs.kind != TOKEN_val, ERR_INTERNAL_ERROR);
			dieif(rhs.kind != TOKEN_val, ERR_INTERNAL_ERROR);
			logif(lhs.type != rhs.type, ERR_INTERNAL_ERROR": %t", ast);

			res->kind = TOKEN_val;
			switch (refCast(lhs.type)) {
				default:
					return CAST_any;

				case CAST_i32:
				case CAST_i64:
				case CAST_u32:
				case CAST_u64:
					res->type = ctx->type_i64;
					switch (ast->kind) {
						default:
							fatal(ERR_INTERNAL_ERROR);
							return CAST_any;

						case OPER_add:
							res->cInt = lhs.cInt + rhs.cInt;
							break;

						case OPER_sub:
							res->cInt = lhs.cInt - rhs.cInt;
							break;

						case OPER_mul:
							res->cInt = lhs.cInt * rhs.cInt;
							break;

						case OPER_div:
							if (rhs.cInt == 0) {
								error(ctx->rt, ast->file, ast->line, "Division by zero: %t", ast);
								res->cInt = 0;
								break;
							}
							res->cInt = lhs.cInt / rhs.cInt;
							break;

						case OPER_mod:
							if (rhs.cInt == 0) {
								error(ctx->rt, ast->file, ast->line, "Division by zero: %t", ast);
								res->cInt = 0;
								break;
							}
							res->cInt = lhs.cInt % rhs.cInt;
							break;
					}
					break;

				case CAST_f32:
				case CAST_f64:
					res->type = ctx->type_f64;
					switch (ast->kind) {
						default:
							fatal(ERR_INTERNAL_ERROR);
							return CAST_any;

						case OPER_add:
							res->cFlt = lhs.cFlt + rhs.cFlt;
							break;

						case OPER_sub:
							res->cFlt = lhs.cFlt - rhs.cFlt;
							break;

						case OPER_mul:
							res->cFlt = lhs.cFlt * rhs.cFlt;
							break;

						case OPER_div:
							res->cFlt = lhs.cFlt / rhs.cFlt;
							break;

						/* FIXME add floating modulus
						case OPER_mod:
							res->cFlt = fmod(lhs.cFlt, rhs.cFlt);
							break;
						*/
					}
					break;
			}
			break;

		case OPER_ceq:			// '=='
		case OPER_cne:			// '!='
		case OPER_clt:			// '<'
		case OPER_cle:			// '<='
		case OPER_cgt:			// '>'
		case OPER_cge:			// '>='
			if (!eval(ctx, &lhs, ast->op.lhso)) {
				return CAST_any;
			}

			if (!eval(ctx, &rhs, ast->op.rhso)) {
				return CAST_any;
			}

			dieif(lhs.kind != TOKEN_val, ERR_INTERNAL_ERROR);
			dieif(rhs.kind != TOKEN_val, ERR_INTERNAL_ERROR);
			dieif(ast->type != ctx->type_bol, ERR_INTERNAL_ERROR);
			logif(lhs.type != rhs.type, "%?s:%?u", ast->file, ast->line);	// might happen: (struct(int) == null)

			res->kind = TOKEN_val;
			res->type = ast->type;

			switch (refCast(rhs.type)) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					res->kind = TOKEN_any;
					return CAST_any;

				case CAST_bit:
				case CAST_i32:
				case CAST_i64:
				case CAST_u32:
				case CAST_u64:
				case CAST_ptr:
				case CAST_obj:
					switch (ast->kind) {
						default:
							fatal(ERR_INTERNAL_ERROR);
							res->kind = TOKEN_any;
							return CAST_any;

						case OPER_ceq:
							res->cInt = lhs.cInt == rhs.cInt;
							break;

						case OPER_cne:
							res->cInt = lhs.cInt != rhs.cInt;
							break;

						case OPER_clt:
							// FIXME: signed or unsigned compare
							res->cInt = lhs.cInt  < rhs.cInt;
							break;

						case OPER_cle:
							// FIXME: signed or unsigned compare
							res->cInt = lhs.cInt <= rhs.cInt;
							break;

						case OPER_cgt:
							// FIXME: signed or unsigned compare
							res->cInt = lhs.cInt  > rhs.cInt;
							break;

						case OPER_cge:
							// FIXME: signed or unsigned compare
							res->cInt = lhs.cInt >= rhs.cInt;
							break;
					}
					break;

				case CAST_f32:
				case CAST_f64:
					switch (ast->kind) {
						default:
							fatal(ERR_INTERNAL_ERROR);
							res->kind = TOKEN_any;
							return CAST_any;

						case OPER_ceq:
							res->cInt = lhs.cFlt == rhs.cFlt;
							break;

						case OPER_cne:
							res->cInt = lhs.cFlt != rhs.cFlt;
							break;

						case OPER_clt:
							res->cInt = lhs.cFlt < rhs.cFlt;
							break;

						case OPER_cle:
							res->cInt = lhs.cFlt <= rhs.cFlt;
							break;

						case OPER_cgt:
							res->cInt = lhs.cFlt > rhs.cFlt;
							break;

						case OPER_cge:
							res->cInt = lhs.cFlt >= rhs.cFlt;
							break;
					}
					break;
			}
			break;

		case OPER_shr:			// '>>'
		case OPER_shl:			// '<<'
		case OPER_and:			// '&'
		case OPER_ior:			// '|'
		case OPER_xor:			// '^'
			if (!eval(ctx, &lhs, ast->op.lhso)) {
				return CAST_any;
			}

			if (!eval(ctx, &rhs, ast->op.rhso)) {
				return CAST_any;
			}

			dieif(lhs.kind != TOKEN_val, ERR_INTERNAL_ERROR);
			dieif(rhs.kind != TOKEN_val, ERR_INTERNAL_ERROR);
			logif(lhs.type != rhs.type, "%?s:%?u", ast->file, ast->line);	// might happen: uint64(x) >> int32(1)
			dieif(ast->type != ctx->type_i32
				&& ast->type != ctx->type_i64
				&& ast->type != ctx->type_u32
				&& ast->type != ctx->type_u64
			, ERR_INTERNAL_ERROR);

			res->kind = TOKEN_val;
			res->type = ast->type;
			switch (refCast(lhs.type)) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					res->kind = TOKEN_any;
					return CAST_any;

				case CAST_bit:
				case CAST_i32:
				case CAST_i64:
				case CAST_u32:
				case CAST_u64:
					switch (ast->kind) {
						default:
							fatal(ERR_INTERNAL_ERROR);
							res->kind = TOKEN_any;
							return CAST_any;

						case OPER_shr:
							res->cInt = lhs.cInt >> rhs.cInt;
							break;

						case OPER_shl:
							// FIXME: signed or unsigned shift
							res->cInt = lhs.cInt << rhs.cInt;
							break;

						case OPER_and:
							res->cInt = lhs.cInt & rhs.cInt;
							break;

						case OPER_xor:
							res->cInt = lhs.cInt ^ rhs.cInt;
							break;

						case OPER_ior:
							res->cInt = lhs.cInt | rhs.cInt;
							break;
					}
					break;
			}
			break;

		case OPER_all:
		case OPER_any:
			if (!eval(ctx, &lhs, ast->op.lhso)) {
				return CAST_any;
			}

			if (!eval(ctx, &rhs, ast->op.rhso)) {
				return CAST_any;
			}

			dieif(lhs.kind != TOKEN_val, ERR_INTERNAL_ERROR);
			dieif(rhs.kind != TOKEN_val, ERR_INTERNAL_ERROR);
			dieif(lhs.type != ctx->type_bol, ERR_INTERNAL_ERROR);
			dieif(rhs.type != ctx->type_bol, ERR_INTERNAL_ERROR);
			dieif(ast->type != ctx->type_bol, ERR_INTERNAL_ERROR);

			// TODO: try reconsidering these operators to behave like in lua or js
			res->kind = TOKEN_val;
			res->type = ast->type;
			switch (ast->kind) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					res->kind = TOKEN_any;
					return CAST_any;

				case OPER_any:
					res->cInt = lhs.cInt || rhs.cInt;
					break;

				case OPER_all:
					res->cInt = lhs.cInt && rhs.cInt;
					break;
			}
			break;

		case OPER_sel:
			if (!eval(ctx, &lhs, ast->op.lhso)) {
				return CAST_any;
			}

			dieif(lhs.kind != TOKEN_val, ERR_INTERNAL_ERROR);
			dieif(ast->op.rhso == NULL || ast->op.rhso->kind != OPER_cln, ERR_INTERNAL_ERROR);
			return eval(ctx, res, bolValue(&lhs) ? ast->op.rhso->op.lhso : ast->op.rhso->op.rhso);

		case INIT_set:
		case ASGN_set:
			return CAST_any;
	}

	if (res->type != NULL && cast != refCast(res->type)) {
		res->kind = TOKEN_val;
		switch (cast) {
			default:
				fatal(ERR_INTERNAL_ERROR);
				return CAST_any;

			case CAST_bit:
				res->cInt = bolValue(res);
				res->type = type;
				break;

			case CAST_i64:
				res->cInt = intValue(res);
				res->type = type;
				break;

			case CAST_f64:
				res->cFlt = fltValue(res);
				res->type = type;
				break;
		}
	}

	switch (res->kind) {
		default:
			fatal(ERR_INTERNAL_ERROR": %t", res);
			res->type = NULL;
			return CAST_any;

		case TOKEN_val:
			res->type = type;
			break;
	}

	return cast;
}

symn linkOf(astn ast, int follow) {
	if (ast == NULL) {
		return NULL;
	}

	if (ast->kind == INLINE_kwd) {
		return ast->type;
	}
	if (ast->kind == RECORD_kwd) {
		return ast->type;
	}
	if (ast->kind == OPER_fnc) {
		if (ast->op.lhso == NULL) {
			// (rand(9.7)) => rand
			return linkOf(ast->op.rhso, follow);
		}
		// rand(9.7) => rand
		//return linkOf(ast->op.lhso);
	}

	if (ast->kind == OPER_dot) {
		// int.size => size
		return linkOf(ast->op.rhso, follow);
	}

	if (ast->kind == OPER_adr) {
		// &buff => buff
		return linkOf(ast->op.rhso, 0);
	}

	if (ast->kind == PNCT_dot3) {
		// ...buff => buff
		return linkOf(ast->op.rhso, 0);
	}

	if (ast->kind == TOKEN_var) {
		symn lnk = ast->id.link;
		if (!follow) {
			return lnk;
		}
		while (lnk != NULL) {
			if (lnk->kind != KIND_def) {
				break;
			}
			if (lnk->init == NULL || lnk->init->kind != TOKEN_var) {
				break;
			}
			lnk = linkOf(lnk->init, follow);
		}
		return lnk;
	}

	return NULL;
}

void addUsage(symn sym, astn tag) {
	if (sym == NULL || tag == NULL) {
		return;
	}
	if (tag->id.used != NULL) {
#ifdef DEBUGGING	// extra check: if this node is linked (.used) it must be in the list
		astn usage;
		for (usage = sym->use; usage; usage = usage->id.used) {
			if (usage == tag) {
				break;
			}
		}
		logif(usage == NULL, ERR_INTERNAL_ERROR);
#endif
		return;
	}
	if (sym->use != tag) {
		tag->id.used = sym->use;
		sym->use = tag;
	}
}

int isTypeExpr(astn ast) {
	if (ast == NULL) {
		return 0;
	}

	switch (ast->kind) {
		default:
			break;

		case OPER_dot:
			return isTypeExpr(ast->op.lhso) && isTypeExpr(ast->op.rhso);

		case TOKEN_var:
			return ast->id.link && isTypename(ast->id.link);
	}

	return 0;
}

static int isMutableAst(astn ast, int varOnly) {
	if (ast == NULL) {
		return 0;
	}

	switch (ast->kind) {
		default:
			break;

		case OPER_idx:
			return isMutableAst(ast->op.lhso, 0);

		case OPER_dot:
			return isMutableAst(ast->op.rhso, 0) || isMutableAst(ast->op.lhso, 1);

		case TOKEN_var:
			if (ast->id.link != NULL) {
				symn link = ast->id.link;
				switch (link->kind & MASK_kind) {
					case KIND_def:
					case KIND_typ:
					case KIND_fun:
						if (varOnly) {
							break;
						}
						// fall through
					case KIND_var:
						return isMutable(link);
				}
			}
			break;
	}

	return 0;
}
int isMutableVar(astn ast) {
	return isMutableAst(ast, 0);
}

astn argNode(ccContext ctx, astn lhs, astn rhs) {
	if (lhs == NULL) {
		return rhs;
	}
	return opNode(ctx, ctx->type_vid, OPER_com, lhs, rhs);
}

astn chainArgs(astn args) {
	if (args == NULL) {
		return NULL;
	}
	if (args->kind == OPER_com) {
		astn lhs = chainArgs(args->op.lhso);
		astn rhs = chainArgs(args->op.rhso);
		args = lhs;
		while (lhs->next != NULL) {
			lhs = lhs->next;
		}
		lhs->next = rhs;
	} else {
		args->next = NULL;
	}
	return args;
}
