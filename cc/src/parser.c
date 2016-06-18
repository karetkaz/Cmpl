/*******************************************************************************
 *   File: lexer.c
 *   Date: 2011/06/23
 *   Desc: input and lexer
 *******************************************************************************

Lexical elements

*/
#include "internal.h"

const struct tok_inf tok_tbl[255] = {
	#define TOKDEF(NAME, TYPE, SIZE, STR) {TYPE, SIZE, STR},
	#include "defs.inl"
};

/** Construct arguments.
 * @brief construct a new node for function arguments
 * @param cc compiler context.
 * @param lhs arguments or first argument.
 * @param rhs next argument.
 * @return if lhs is null return (rhs) else return (lhs, rhs).
 */
static inline astn argnode(ccContext cc, astn lhs, astn rhs) {
	return lhs ? opnode(cc, OPER_com, lhs, rhs) : rhs;
}

/** Construct block node.
 * @brief construct a new node for block of statements
 * @param cc compiler context.
 * @param node next argument.
 * @return if lhs is null return (rhs) else return (lhs, rhs).
 */
static inline astn blockNode(ccContext cc, astn node) {
	astn block = newnode(cc, STMT_beg);
	block->cast = CAST_vid;
	block->type = cc->type_vid;
	if (node != NULL) {
		block->file = node->file;
		block->line = node->line;
		block->stmt.stmt = node;
	}
	return block;
}

/** Construct reference node.
 * @brief construct a new node for variables.
 * @param cc compiler context.
 * @param name name of the node.
 * @return the new node.
 */
static inline astn tagnode(ccContext cc, char* name) {
	astn ast = NULL;
	if (cc != NULL && name != NULL) {
		ast = newnode(cc, CAST_ref);
		if (ast != NULL) {
			size_t slen = strlen(name);
			ast->kind = CAST_ref;
			ast->file = cc->file;
			ast->line = cc->line;
			ast->type = NULL;
			ast->ref.link = NULL;
			ast->ref.hash = rehash(name, slen + 1) % TBLS;
			ast->ref.name = mapstr(cc, name, slen + 1, ast->ref.hash);
		}
	}
	return ast;
}

/** Add length property.
 * @brief add length property to array type
 * @param cc compiler context.
 * @param sym the symbol of the array.
 * @param init size of the length:
 *     constant int node for static length arrays,
 *     null for dynamic length arrays
 * @return symbol of the length property.
 */
static symn addLength(ccContext cc, symn sym, astn init) {
	/* TODO: review return value.
	 *
	 * dynamic array size kind: ATTR_const | CAST_ref
	 * static array size kind: ATTR_stat | ATTR_const | TYPE_def
	 *     using static we get the warning accessing static member through instance.
	 */
	ccToken kind = ATTR_const | (init != NULL ? TYPE_def : CAST_ref);
	symn args = sym->flds;
	symn result = NULL;

	enter(cc, NULL);
	result = install(cc, "length", kind, CAST_i32, vm_size, cc->type_i32, init);
	sym->flds = leave(cc, sym, 0);
	dieif(sym->flds != result, "FixMe");

	// non static member
	if (sym->flds) {
		sym->flds->next = args;
	}
	else {
		sym->flds = args;
	}
	return result;
}

/** Check for redefinition of symbol.
 * @brief Check for redefinition of symbol.
 * @param cc compiler context.
 * @param sym symbol to check.
 */
//~ TODO: move to type.c.
static void redefine(ccContext cc, symn sym) {
	struct astNode tag;
	symn ptr;

	if (sym == NULL) {
		return;
	}

	memset(&tag, 0, sizeof(tag));
	tag.kind = CAST_ref;
	// checking symbols with the same hash code.
	for (ptr = sym->next; ptr; ptr = ptr->next) {
		symn arg1 = ptr->prms;
		symn arg2 = sym->prms;

		// there should be no anonimus declarationd with this hash
		if (ptr->name == NULL)
			continue;

		// may override default initializer
		if (sym->call != ptr->call)
			continue;

		// hash may match but name be different.
		if (strcmp(sym->name, ptr->name) != 0)
			continue;

		while (arg1 && arg2) {
			tag.ref.link = arg1;
			tag.type = arg1->type;
			if (!canAssign(cc, arg2, &tag, 1)) {
				tag.ref.link = arg2;
				tag.type = arg2->type;
				if (!canAssign(cc, arg1, &tag, 1)) {
					break;
				}
			}
			arg1 = arg1->next;
			arg2 = arg2->next;
		}

		if (arg1 == NULL && arg2 == NULL) {
			break;
		}
	}

	if (ptr != NULL && (ptr->nest >= sym->nest)) {
		error(cc->rt, sym->file, sym->line, "redefinition of `%-T`", sym);
		if (ptr->file && ptr->line) {
			info(cc->rt, ptr->file, ptr->line, "first defined here as `%-T`", ptr);
		}
	}
}

/**
 * @brief Fix structure member offsets / function arguments.
 * @param sym Symbol of struct or function.
 * @param align Alignment of members.
 * @param base Size of base / return.
 * @return Size of struct / functions param size.
 */
// TODO: remove (should be handled by leave)
static size_t fixargs(symn sym, unsigned int align, size_t base) {
	symn arg;
	size_t stdiff = 0;
	int isCall = sym->call;
	for (arg = sym->prms; arg; arg = arg->next) {

		if (arg->kind != CAST_ref)
			continue;

		if (arg->stat)
			continue;

		// functions are byRef in structs and params
		if (arg->call) {
			arg->cast = CAST_ref;
		}

		arg->size = sizeOf(arg, 1);

		//TODO: remove check: dynamic size arrays are represented as pointer+length
		if (arg->type->kind == CAST_arr && arg->type->init == NULL) {
			dieif(arg->size != 2 * vm_size, ERR_INTERNAL_ERROR);
			dieif(arg->cast != CAST_arr, ERR_INTERNAL_ERROR);
			//arg->size = 2 * vm_size;
			//arg->cast = CAST_arr;
		}

		//~ HACK: static sized array types are passed by reference.
		if (isCall && arg->type->kind == CAST_arr) {
			//~ static size arrays are passed as pointer
			if (arg->type->init != NULL) {
				arg->cast = CAST_ref;
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

/**
 * @brief make constructor using fields as arguments.
 * @param cc compiler context.
 * @param rec the record.
 * @return the constructors symbol.
 */
// TODO: to be deleted. replace with new initialization.
static symn ctorArg(ccContext cc, symn rec) {
	symn ctor = install(cc, rec->name, TYPE_def, TYPE_any, 0, rec, NULL);
	if (ctor != NULL) {
		astn root = NULL;
		symn arg, newarg;

		// TODO: params should not be dupplicated: ctor->prms = rec->prms
		enter(cc, NULL);
		for (arg = rec->flds; arg; arg = arg->next) {

			if (arg->stat)
				continue;

			if (arg->kind != CAST_ref)
				continue;

			newarg = install(cc, arg->name, TYPE_def, arg->type->cast, 0, arg->type, NULL);
			//~ newarg = install(cc, arg->name, CAST_ref, TYPE_rec, 0, arg->type, NULL);
			//~ newarg->cast = arg->type->cast;

			if (newarg != NULL) {
				astn tag;
				newarg->call = arg->call;
				newarg->prms = arg->prms;

				tag = lnknode(cc, newarg);
				tag->cast = arg->cast;
				root = argnode(cc, root, tag);
			}
		}
		ctor->prms = leave(cc, ctor, 0);

		ctor->kind = TYPE_def;
		ctor->call = 1;
		ctor->cnst = 1;	// returns constant (if arguments are constants)
		ctor->init = opnode(cc, OPER_fnc, cc->emit_tag, root);
		ctor->init->type = rec;
	}
	return ctor;
}

// TODO: to be deleted.
static int mkConst(astn ast, ccToken cast) {
	struct astNode tmp;

	if (ast == NULL) {
		fatal(ERR_INTERNAL_ERROR);
		return 0;
	}

	if (cast == TYPE_any) {
		cast = ast->cast;
	}

	if (!eval(&tmp, ast)) {
		traceAst(ast);
		return 0;
	}

	ast->kind = tmp.kind;
	ast->cint = tmp.cint;

	switch (ast->cast = cast) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return 0;

		case CAST_vid:
		case TYPE_any:
		case TYPE_rec:
			return 0;

		case CAST_bit:
			ast->cint = constbol(ast);
			ast->kind = TYPE_int;
			break;

		case CAST_u32:
		case CAST_i32:
		case CAST_i64:
			ast->cint = constint(ast);
			ast->kind = TYPE_int;
			break;

		case CAST_f32:
		case CAST_f64:
			ast->cflt = constflt(ast);
			ast->kind = TYPE_flt;
			break;
	}

	return ast->kind;
}

//#{~~~~~~~~~ Parser ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static astn stmt(ccContext, int mode);	// parse statement		(mode: enable decl)
static astn decl(ccContext, int mode);	// parse declaration	(mode: enable spec)
//~ astn args(ccContext, int mode);		// parse arguments		(mode: ...)
//~ astn unit(ccContext, int mode);		// parse unit			(mode: script/unit)
static astn decl_var(ccContext cc, astn* argv, int mode);

/** Parse qualifiers.
 * @brief scan for qualifiers: const static ?paralell
 * @param cc compiler context.
 * @param mode scannable qualifiers.
 * @return qualifiers.
 */
static int qual(ccContext cc, int mode) {
	int result = 0;
	astn ast;

	while ((ast = peekTok(cc, TYPE_any))) {
		traceLoop("%t", peekTok(cc, 0));
		switch (ast->kind) {

			default:
				return result;

			case ATTR_const:
				if (!(mode & ATTR_const))
					return result;

				if (result & ATTR_const) {
					error(cc->rt, ast->file, ast->line, "qualifier `%K` declared more than once", ast);
				}
				result |= ATTR_const;
				skipTok(cc, TYPE_any, 0);
				break;

			case ATTR_stat:
				if (!(mode & ATTR_stat))
					return result;

				if (result & ATTR_stat) {
					error(cc->rt, ast->file, ast->line, "qualifier `%K` declared more than once", ast);
				}
				result |= ATTR_stat;
				skipTok(cc, TYPE_any, 0);
				break;
		}
	}

	return result;
}

//~ TODO: to be deleted: use expr instead, then check if it is a typename.
static astn type(ccContext cc/* , int mode */) {	// type(.type)*
	symn def = NULL;
	astn tok, typ = NULL;
	while ((tok = nextTok(cc, CAST_ref))) {

		symn loc = def ? def->flds : cc->deft[tok->ref.hash];
		symn sym = lookup(cc, loc, tok, NULL, 0);

		tok->next = typ;
		typ = tok;

		def = sym;

		if (!isType(sym)) {
			def = NULL;
			break;
		}

		if ((tok = nextTok(cc, OPER_dot))) {
			tok->next = typ;
			typ = tok;
		}
		else
			break;

	}
	if (!def && typ) {
		while (typ) {
			astn back = typ;
			typ = typ->next;
			backTok(cc, back);
		}
		typ = NULL;
	}
	else if (typ) {
		typ->type = def;
		addUsage(def, typ);
	}
	return typ;
}

/** TODO: remove typechecking.
 * @brief Parse expression.
 * @param cc compiler context.
 * @param mode
 * @return
 */
static astn expr(ccContext cc, int mode) {
	astn buff[TOKS], tok;
	const astn* base = buff + TOKS;
	astn* prec = (astn*)base;
	astn* post = buff;
	char sym[TOKS];						// symbol stack {, [, (, ?
	int unary = 1;						// start in unary mode
	int level = 0;						// precedence, top of sym

	sym[level] = 0;
	if (!peekTok(cc, TYPE_any)) {
		trace("null");
		return NULL;
	}

	while ((tok = nextTok(cc, TYPE_any))) {					// parse
		int pri = level << 4;
		traceLoop("%t", peek(cc));
		// statements are not allowed in expressions !!!!
		if (tok->kind == RIGHT_crl || (tok->kind >= STMT_beg && tok->kind <= STMT_end)) {
			backTok(cc, tok);
			tok = 0;
			break;
		}
		switch (tok->kind) {
			tok_id: {
				*post++ = tok;
			} break;
			tok_op: {
				int oppri = tok_tbl[tok->kind].type;
				tok->op.prec = pri | (oppri & 0x0f);
				if (oppri & 0x10) while (prec < base) {
					if ((*prec)->op.prec <= tok->op.prec)
						break;
					*post++ = *prec++;
				}
				else while (prec < base) {
					if ((*prec)->op.prec < tok->op.prec)
						break;
					*post++ = *prec++;
				}
				if (tok->kind != STMT_end) {
					*--prec = tok;
				}
			} break;

			default: {
				if (tok_tbl[tok->kind].argc) {			// operator
					if (unary) {
						*post++ = 0;
						error(cc->rt, tok->file, tok->line, ERR_SYNTAX_ERR_BEFORE, tok);
					}
					unary = 1;
					goto tok_op;
				}
				else {
					if (!unary)
						error(cc->rt, tok->file, tok->line, ERR_SYNTAX_ERR_BEFORE, tok);
					unary = 0;
					goto tok_id;
				}
			} break;

			case LEFT_par: {			// '('
				if (unary)			// a + (3*b)
					*post++ = 0;
				else if (peekTok(cc, RIGHT_par)) {	// a()
					unary = 2;
				}
				else {
					unary = 1;
				}

				tok->kind = OPER_fnc;
				sym[++level] = '(';

			} goto tok_op;
			case RIGHT_par: {			// ')'
				if (unary == 2 && sym[level] == '(') {
					tok->kind = STMT_end;
					*post++ = NULL;
					level -= 1;
				}
				else if (!unary && sym[level] == '(') {
					tok->kind = STMT_end;
					level -= 1;
				}
				else if (level == 0) {
					backTok(cc, tok);
					tok = 0;
					break;
				}
				else {
					error(cc->rt, tok->file, tok->line, ERR_SYNTAX_ERR_BEFORE, tok);
					break;
				}
				unary = 0;
			} goto tok_op;
			case LEFT_sqr: {			// '['
				if (!unary)
					tok->kind = OPER_idx;
				else {
					error(cc->rt, tok->file, tok->line, ERR_SYNTAX_ERR_BEFORE, tok);
					//~ break?;
				}
				sym[++level] = '[';
				unary = 1;
			} goto tok_op;
			case RIGHT_sqr: {			// ']'
				if (!unary && sym[level] == '[') {
					tok->kind = STMT_end;
					level -= 1;
				}
				else if (!level) {
					backTok(cc, tok);
					tok = 0;
					break;
				}
				else {
					error(cc->rt, tok->file, tok->line, ERR_SYNTAX_ERR_BEFORE, tok);
					break;
				}
				unary = 0;
			} goto tok_op;
			case PNCT_qst: {		// '?'
				if (unary) {
					*post++ = 0;		// ???
					error(cc->rt, tok->file, tok->line, ERR_SYNTAX_ERR_BEFORE, tok);
				}
				tok->kind = OPER_sel;
				sym[++level] = '?';
				unary = 1;
			} goto tok_op;
			case PNCT_cln: {		// ':'
				if (!unary && sym[level] == '?') {
					tok->kind = STMT_end;
					level -= 1;
				}
				else if (level == 0) {
					backTok(cc, tok);
					tok = 0;
					break;
				}
				else {
					error(cc->rt, tok->file, tok->line, ERR_SYNTAX_ERR_BEFORE, tok);
					break;
				}
				unary = 1;
			} goto tok_op;
			case OPER_add: {		// '+'
				if (unary)
					tok->kind = OPER_pls;
				unary = 1;
			} goto tok_op;
			case OPER_sub: {		// '-'
				if (unary)
					tok->kind = OPER_mns;
				unary = 1;
			} goto tok_op;
			case OPER_and: {		// '&'
				if (unary)
					tok->kind = OPER_adr;
				unary = 1;
			} goto tok_op;
			case OPER_not: {		// '!'
				if (!unary)
					error(cc->rt, tok->file, tok->line, ERR_SYNTAX_ERR_BEFORE, tok);
				unary = 1;
			} goto tok_op;
			case OPER_cmt: {		// '~'
				if (!unary)
					error(cc->rt, tok->file, tok->line, ERR_SYNTAX_ERR_BEFORE, tok);
				unary = 1;
			} goto tok_op;
			case OPER_com: {		// ','
				if (unary)
					error(cc->rt, tok->file, tok->line, ERR_SYNTAX_ERR_BEFORE, tok);
				// skip trailing commas
				switch (test(cc)) {
					case RIGHT_crl:
					case RIGHT_sqr:
					case RIGHT_par:
						warn(cc->rt, 1, tok->file, tok->line, WARN_TRAILING_COMMA, peekTok(cc, 0));
						continue;
					default:
						break;
				}
				unary = 1;
			} goto tok_op;
		}
		if (post >= prec) {
			error(cc->rt, cc->file, cc->line, "Expression too complex");
			return 0;
		}
		if (!tok)
			break;
	}
	if (unary || level) {							// error
		char* missing = "expression";
		if (level) switch (sym[level]) {
			default:
				fatal(ERR_INTERNAL_ERROR);
				return NULL;

			case '(': missing = "')'"; break;
			case '[': missing = "']'"; break;
			case '?': missing = "':'"; break;
		}
		error(cc->rt, cc->file, cc->line, "missing %s, %t", missing, peekTok(cc, TYPE_any));
	}
	else if (prec > buff) {							// build
		astn* ptr;
		astn* lhs;

		while (prec < buff + TOKS)					// flush
			*post++ = *prec++;
		*post = NULL;

		// we have the postfix form, build the tree
		for (lhs = ptr = buff; ptr < post; ptr += 1) {
			if ((tok = *ptr) == NULL) {}			// skip
			else if (tok_tbl[tok->kind].argc) {		// oper
				int argc = tok_tbl[tok->kind].argc;
				if ((lhs -= argc) < buff) {
					fatal(ERR_INTERNAL_ERROR);
					return NULL;
				}
				switch (argc) {
					default:
						fatal(ERR_INTERNAL_ERROR);
						return NULL;

					case 1:
						//~ tok->op.test = NULL;
						//~ tok->op.lhso = NULL;
						tok->op.rhso = lhs[0];
						break;

					case 2:
						//~ tok->op.test = NULL;
						tok->op.lhso = lhs[0];
						tok->op.rhso = lhs[1];
						break;
					case 3:
						tok->op.test = lhs[0];
						tok->op.lhso = lhs[1];
						tok->op.rhso = lhs[2];
						break;
				}
				switch (tok->kind) {
					default:
						break;

					case ASGN_add:
					case ASGN_sub:
					case ASGN_mul:
					case ASGN_div:
					case ASGN_mod:
					case ASGN_shl:
					case ASGN_shr:
					case ASGN_and:
					case ASGN_ior:
					case ASGN_xor: {
						astn tmp = NULL;
						switch (tok->kind) {
							default:
								fatal(ERR_INTERNAL_ERROR);
								return NULL;

							case ASGN_add:
								tmp = newnode(cc, OPER_add);
								break;
							case ASGN_sub:
								tmp = newnode(cc, OPER_sub);
								break;
							case ASGN_mul:
								tmp = newnode(cc, OPER_mul);
								break;
							case ASGN_div:
								tmp = newnode(cc, OPER_div);
								break;
							case ASGN_mod:
								tmp = newnode(cc, OPER_mod);
								break;
							case ASGN_shl:
								tmp = newnode(cc, OPER_shl);
								break;
							case ASGN_shr:
								tmp = newnode(cc, OPER_shr);
								break;
							case ASGN_and:
								tmp = newnode(cc, OPER_and);
								break;
							case ASGN_ior:
								tmp = newnode(cc, OPER_ior);
								break;
							case ASGN_xor:
								tmp = newnode(cc, OPER_xor);
								break;
						}
						tmp->op.rhso = tok->op.rhso;
						tmp->op.lhso = tok->op.lhso;
						tmp->file = tok->file;
						tmp->line = tok->line;
						tok->kind = ASGN_set;
						tok->op.rhso = tmp;
						break;
					}
				}
			}
			*lhs++ = tok;
		}
	}

	if (mode && tok) {								// check
		astn root = cc->root;
		cc->root = NULL;
		if (!typeCheck(cc, NULL, tok)) {
			if (cc->root != NULL) {
				error(cc->rt, tok->file, tok->line, "invalid expression: `%+t` in `%+t`", cc->root, tok);
			}
			else {
				error(cc->rt, tok->file, tok->line, "invalid expression: `%+t`", tok);
			}
		}
		cc->root = root;
	}
	dieif(mode == 0, ERR_INTERNAL_ERROR);
	return tok;
}

/** Parse arguments.
 * @brief Parse the parameters of a function declaration.
 * @param cc compiler context.
 * @param mode
 * @return expression tree.
 */
static astn args(ccContext cc, int mode) {
	astn root = NULL;

	if (peekTok(cc, RIGHT_par)) {
		return cc->void_tag;
	}

	while (peekTok(cc, TYPE_any)) {
		int attr = qual(cc, ATTR_const);
		astn arg = decl_var(cc, NULL, mode);

		if (arg != NULL) {
			root = argnode(cc, root, arg);
			if (attr & ATTR_const) {
				arg->ref.link->cnst = 1;
			}
		}

		if (!skipTok(cc, OPER_com, 0)) {
			break;
		}
	}
	return root;
}

/** Parse initialization.
 * @brief parse the initializer of a declaration.
 * @param cc compiler context.
 * @param var the declared variable symbol.
 * @return the initializer expression.
 */
static astn init_var(ccContext cc, symn var) {
	if (skipTok(cc, ASGN_set, 0)) {
		symn typ = var->type;
		ccToken cast = var->cast;
		int mkcon = var->cnst;
		ccToken arrayInit = TYPE_any;

		if (typ->kind == CAST_arr) {
			if (skipTok(cc, STMT_beg, 0)) {
				arrayInit = RIGHT_crl;
			}
			else if (skipTok(cc, LEFT_sqr, 0)) {
				arrayInit = RIGHT_sqr;
			}
			else if (skipTok(cc, LEFT_par, 0)) {
				arrayInit = RIGHT_par;
			}
		}
		var->init = expr(cc, TYPE_def);
		if (arrayInit != TYPE_any) {
			skipTokens(cc, arrayInit, 1);
		}

		if (var->init != NULL) {

			//~ TODO: try to type check the same as using ASGN_set
			// assigning an emit expression: ... x = emit(struct, ...)
			if (var->init->type == cc->emit_opc) {
				var->init->type = var->type;
				var->init->cast = cast;
				if (mkcon) {
					error(cc->rt, var->file, var->line, ERR_CONST_INIT, var->init);
				}
			}

			// assigning to an array
			else if (arrayInit) {
				astn init = var->init;
				symn base = var->arrB;
				int nelem = 1;

				// TODO: base should not be stored in var->args !!!
				cast = base ? base->cast : TYPE_any;

				if (base == typ->type && typ->init == NULL) {
					mkcon = 0;
				}

				while (init->kind == OPER_com) {
					astn val = init->op.rhso;
					if (!canAssign(cc, base, val, 0)) {
						trace("canAssignCast(%-T, %+t, %K)", base, val, cast);
						return NULL;
					}
					if (!castTo(val, cast)) {
						trace("canAssignCast(%-T, %+t, %K)", base, val, cast);
						return NULL;
					}
					if (mkcon && !mkConst(val, cast)) {
						trace("canAssignCast(%-T, %+t, %K)", base, val, cast);
						//~ return NULL;
					}
					init = init->op.lhso;
					nelem += 1;
				}
				if (!canAssign(cc, base, init, 0)) {
					trace("canAssignArray(%-T, %+t, %K)", base, init, cast);
					return NULL;
				}
				if (!castTo(init, cast)) {
					trace("canAssignCast(%-T, %+t, %K)", base, init, cast);
					return NULL;
				}
				if (mkcon && !mkConst(init, cast)) {
					trace("canAssignCast(%-T, %+t, %K)", base, init, cast);
					//~ return NULL;
				}

				// int a[] = {1, 2, 3, 4, 5, 6};
				// TODO:! var initialization parsing should not modify the variable.
				if (base == typ->type && typ->init == NULL && var->cnst) {
					typ->init = intnode(cc, nelem);
					// ArraySize
					typ->size = nelem * typ->type->size;
					typ->offs = (size_t) nelem;
					typ->cast = CAST_ref;
					typ->stat = 1;
					var->cast = TYPE_any;
					var->size = typ->size;

					addLength(cc, typ, typ->init);
				}
				else if (typ->init == NULL) {
					// can not initialize a slice with values.
					error(cc->rt, var->file, var->line, "invalid slice initialization `%+t`", var->init);
					return NULL;
				}
				var->init->type = var->arrB;
			}

			// check if value can be assigned to variable.
			if (canAssign(cc, var, var->init, cast == CAST_ref)) {

				// assign enum variable to base type.
				if (var->cast != ENUM_kwd && var->init->type->cast == ENUM_kwd) {
					astn ast = var->init;
					symn eTyp = ast->type->type;
					if (!castTo(ast, castOf(ast->type = eTyp))) {
						traceAst(ast);
						return NULL;
					}
				}

				// TODO:! var initialization parsing should not modify the variable.
				if (var->cast == CAST_arr) {
					var->size = 2 * vm_size;
				}

				//~ trace("%+T -> %K (%s:%u)", var, cast, var->file, var->line);
				if (!castTo(var->init, cast)) {
					trace("%K", cast);
					return NULL;
				}
			}
			else {
				error(cc->rt, var->file, var->line, "invalid initialization `%-T` := `%+t`", var, var->init);
				return NULL;
			}
		}
		else {
			// var->init is null and expr raised the error.
			//~ error(s->s, var->file, var->line, "invalid initialization of `%-T`", var);
		}
	}
	return var->init;
}

/** Parse alias declaration.
 * @brief parse the declaration of an alias/inline expression.
 * @param cc compiler context.
 * @return root of declaration.
 */
static astn decl_alias(ccContext cc) {
	astn tag = NULL;
	symn def = NULL;
	symn typ = NULL;

	if (!skipTokens(cc, TYPE_def, 1)) {	// 'define'
		traceAst(tag);
		return NULL;
	}

	if (!(tag = nextTok(cc, CAST_ref))) {	// name?
		error(cc->rt, cc->file, cc->line, "Identifyer expected");
		skipTokens(cc, STMT_end, 1);
		return NULL;
	}

	if (skipTok(cc, ASGN_set, 0)) {				// const: define PI = 3.14...;
		// can not call decl_init
		// TODO: check if locals are used.
		astn val = expr(cc, TYPE_def);
		if (val != NULL) {
			def = declare(cc, TYPE_def, tag, val->type);
			if (!isTypeExpr(val)) {
				if (isConstExpr(val)) {
					def->stat = 1;
					def->cnst = 1;
				}
				else if (isStaticExpr(cc, val)) {
					def->stat = 1;
				}
				def->init = val;
			}
			typ = def->type;
		}
		else {
			error(cc->rt, cc->file, cc->line, "expression expected");
		}
	}
	else if (skipTok(cc, LEFT_par, 0)) {			// inline: define isNan(float64 x) = x != x;
		astn init = NULL;
		symn param;

		def = declare(cc, TYPE_def, tag, NULL);
		def->name = NULL; // disable lookup as variable.

		enter(cc, NULL);
		args(cc, TYPE_def);
		skipTokens(cc, RIGHT_par, 1);

		if (skipTok(cc, ASGN_set, 0)) {
			// define sqr(int a) = (a * a);
			if ((init = expr(cc, TYPE_def))) {
				typ = init->type;
			}
		}

		if (!(param = leave(cc, def, 0))) {
			param = cc->void_tag->ref.link;
		}

		def->name = tag->ref.name;
		def->stat = isStaticExpr(cc, init);
		def->cnst = isConstExpr(init);
		def->type = typ;
		def->prms = param;
		def->init = init;
		def->call = 1;

		//* TODO: in-out parameters: remove
		// TODO: this should go to fixargs
		for (param = def->prms; param; param = param->next) {
			// is explicitly set to be cached.
			if (param->cast == CAST_ref) {
				param->cast = param->type->cast;
			}
			else {
				int used = countUsages(param) - 1;
				// warn to cache if it is used more than once
				if (used > 1) {
					warn(cc->rt, 16, param->file, param->line,
						 "parameter `%T` may be cached (used %d times in expression)", param, used);
				}
				else {
					param->kind = TYPE_def;
				}
			}
		}// */

		if (init == NULL) {
			error(cc->rt, cc->file, cc->line, "expression expected");
		}
	}
	else {
		error(cc->rt, tag->file, tag->line, "typename excepted");
		return NULL;
	}

	if (!skipTokens(cc, STMT_end, 1)) {
		traceAst(tag);
		return NULL;
	}

	tag->kind = TYPE_def;
	tag->type = typ;
	tag->ref.link = def;
	return tag;
}

/** Parse enum declaration.
 * @brief parse the declaration of an enumeration.
 * @param cc compiler context.
 * @return root of declaration.
 */
static astn decl_enum(ccContext cc) {
	symn def = NULL, base = cc->type_i32;
	astn tok, tag;

	int enuminc = 1;
	int64_t lastval = -1;

	if (!skipTokens(cc, ENUM_kwd, 1)) {	// 'enum'
		traceAst(peekTok(cc, 0));
		return NULL;
	}

	tag = nextTok(cc, CAST_ref);			// ref?

	if (skipTok(cc, PNCT_cln, 0)) {			// ':' type
		base = NULL;
		if ((tok = expr(cc, TYPE_def))) {
			if (isTypeExpr(tok)) {
				base = linkOf(tok);
			}
			else {
				error(cc->rt, cc->file, cc->line, "typename expected, got `%t`", tok);
			}
		}
		else {
			error(cc->rt, cc->file, cc->line, "typename expected, got `%t`", peekTok(cc, TYPE_any));
		}
		enuminc = base && (base->cast == CAST_i32 || base->cast == CAST_i64);
	}

	if (!skipTokens(cc, STMT_beg, 1)) {	// '{'
		traceAst(peekTok(cc, 0));
		return NULL;
	}

	if (tag != NULL) {
		def = declare(cc, ATTR_stat | ATTR_const | TYPE_rec, tag, base);
		tag->kind = TYPE_def;
		enter(cc, tag);
	}
	else {
		tag = tagnode(cc, ".anonymous");
		tag->ref.link = base;
		tag->type = base;
	}

	while (peekTok(cc, TYPE_any)) {			// enum members
		traceLoop("%t", peek(cc));

		if (peekTok(cc, RIGHT_crl)) {
			break;
		}

		if ((tok = nextTok(cc, CAST_ref))) {
			// HACK: declare as a variable to be assignable, then revert it to be an alias.
			symn ref = declare(cc, CAST_ref, tok, base);
			ref->stat = ref->cnst = 1;
			redefine(cc, ref);

			if (init_var(cc, ref)) {
				if (!mkConst(ref->init, TYPE_any)) {
					warn(cc->rt, 2, ref->file, ref->line, "constant expected, got: `%+t`", ref->init);
				}
				else if (enuminc) {
					lastval = constint(ref->init);
				}
			}
			else if (enuminc) {
				ref->init = intnode(cc, lastval += 1);
				ref->init->type = base;
			}
			else {
				error(cc->rt, ref->file, ref->line, "constant expected");
			}
			ref->kind = TYPE_def;
		}
		skipTokens(cc, STMT_end, 1);
	}

	if (def != NULL) {
		def->flds = leave(cc, def, ATTR_stat);
		def->prms = base->prms;
		def->cast = ENUM_kwd;
	}

	if (!skipTokens(cc, RIGHT_crl, 1)) {	// '}'
		traceAst(tag);
		return NULL;
	}

	// Error checking
	// ... ?

	return tag;
}

/** Parse struct declaration.
 * @brief parse the declaration of a structure.
 * @param cc compiler context.
 * @param attr attributes: static or const.
 * @return root of declaration.
 */
static astn decl_struct(ccContext cc, int Attr) {
	symn type = NULL, base = NULL;
	astn tok, tag;

	int byref = 0;
	unsigned pack = vm_size;

	if (!skipTokens(cc, TYPE_rec, 1)) {	// 'struct'
		traceAst(peekTok(cc, 0));
		return NULL;
	}

	tag = nextTok(cc, CAST_ref);			// ref

	if (skipTok(cc, PNCT_cln, 0)) {			// ':' type or pack
		if ((tok = expr(cc, TYPE_def))) {
			if (tok->kind == TYPE_int) {
				if (tok->cint > 16 || (tok->cint & -tok->cint) != tok->cint) {
					error(cc->rt, tok->file, tok->line, "alignment must be a small power of two, not %t", tok);
				}
				else if (Attr == ATTR_stat) {
					error(cc->rt, cc->file, cc->line, "alignment can not be applied to static struct");
				}
				pack = (unsigned) tok->cint;
			}
			else if (isTypeExpr(tok)) {
				base = linkOf(tok);
			}
			else {
				error(cc->rt, cc->file, cc->line, "alignment must be an integer constant");
			}
		}
		else {
			error(cc->rt, cc->file, cc->line, "alignment must be an integer constant, got `%t`", peekTok(cc, TYPE_any));
		}
	}

	if (!skipTokens(cc, STMT_beg, 1)) {	// '{'
		traceAst(peekTok(cc, 0));
		return NULL;
	}

	if (tag == NULL) {
		error(cc->rt, cc->file, cc->line, "identifier expected");
		tag = tagnode(cc, ".anonymous");
	}
	type = declare(cc, ATTR_stat | ATTR_const | TYPE_rec, tag, cc->type_rec);
	tag->kind = TYPE_def;
	enter(cc, tag);

	while (peekTok(cc, TYPE_any)) {			// struct members
		traceLoop("%t", peekTok(cc, 0));

		if (peekTok(cc, RIGHT_crl)) {
			break;
		}

		if (!decl(cc, 0)) {
			error(cc->rt, cc->file, cc->line, "declaration expected, got: `%+t`", peekTok(cc, TYPE_any));
			skipTokens(cc, STMT_end, 0);
		}
	}

	type->flds = leave(cc, type, Attr & ATTR_stat);
	type->prms = type->flds;
	type->size = fixargs(type, pack, 0);

	if (!skipTokens(cc, RIGHT_crl, 1)) {	// '}'
		traceAst(peekTok(cc, 0));
		return tag;
	}

	if (base != NULL) {
		type->size += base->size;
		type->pfmt = base->pfmt;
		cc->pfmt = type;
		type->cast = base->cast;
	}
	else {
		type->cast = TYPE_rec;
	}

	if ((Attr & ATTR_stat) != 0 || type->size == 0) {
		// make uninstantiable
		type->cast = CAST_vid;
	}
	else {
		if (type->flds && pack == vm_size && !byref) {
			ctorArg(cc, type);
		}

		if (!type->flds && !type->type) {
			warn(cc->rt, 2, cc->file, cc->line, "empty declaration");
		}
	}

	// Error checking
	if (base && base->cast != CAST_ref) {
		// TODO: to be removed: declarations like: struct hex32: int32 { };
		if (skipTok(cc, STMT_end, 0)) {
			warn(cc->rt, 1, tag->file, tag->line, "deprecated declaration of `%t`", tag);
		}
		else {
			error(cc->rt, tag->file, tag->line, "must extend a reference type, not `%+T`", base);
		}
	}

	if (Attr == ATTR_stat) {
		// TODO: static structs can not allign 'static struct Struct: 1'
		// TODO: static structs can not inherit 'static struct Struct: object'

		//error(cc->s, tag->file, tag->line, "alignment can not be applied static struct: `%?T`", def);
	}

	//tag->type = def;
	return tag;
}

/** Parse variable or function declaration.
 * @brief parse the declaration of a variable or function.
 * @param cc compiler context.
 * @param argv out parameter for function arguments.
 * @param mode
 * @return parsed syntax tree.
 */
static astn decl_var(ccContext cc, astn* argv, int mode) {
	symn typ, ref = NULL;
	astn tag = NULL;
	int inout, byref;

	if (!(tag = type(cc))) {
		//~ traceAst(peekTok(cc, 0));
		return NULL;
	}
	typ = tag->type;

	inout = skipTok(cc, OPER_all, 0);
	byref = inout || skipTok(cc, OPER_and, 0);

	if (typ->cast == CAST_ref) {
		if (byref) {
			warn(cc->rt, 2, tag->file, tag->line, "ignoring &, %-T is already a reference type", typ);
		}
		//byref = 1;
	}

	if (!(tag = nextTok(cc, CAST_ref))) {
		if (mode != TYPE_def) {
			trace("id expected, not `%t` at(%s:%u)", peekTok(cc, 0), cc->file, cc->line);
			return NULL;
		}
		tag = tagnode(cc, "");
	}

	ref = declare(cc, CAST_ref, tag, typ);
	ref->size = byref ? (size_t) vm_size : sizeOf(typ, 1);

	if (argv != NULL) {
		*argv = NULL;
	}

	if (skipTok(cc, LEFT_par, 0)) {			// int a(...)
		astn argroot;
		enter(cc, tag);
		argroot = args(cc, CAST_ref);
		ref->prms = leave(cc, ref, 0);
		skipTokens(cc, RIGHT_par, 1);

		ref->call = 1;
		ref->size = vm_size;

		if (ref->prms == NULL) {
			ref->prms = cc->void_tag->ref.link;
			argroot = cc->void_tag;
		}

		if (argv) {
			*argv = argroot;
		}

		if (inout || byref) {
			// int& find(int array[], int element);
			// int&& find(int array[], int element);
			error(cc->rt, tag->file, tag->line, "TODO: declaration returning reference: `%+T`", ref);
		}
		byref = 0;
	}

	if (skipTok(cc, LEFT_sqr, 0)) {			// int a[...]
		symn arr = newdefn(cc, CAST_arr);
		symn tmp = arr;
		symn base = typ;
		int dynarr = 1;		// dynamic sized array: slice

		tmp->type = typ;
		tmp->size = -1;
		typ = tmp;

		ref->type = typ;
		tag->type = typ;
		//~ arr->offs = vmOffset(cc->s, arr);
		//~ arr->stat = 1;

		if (!peekTok(cc, RIGHT_sqr)) {
			astn init = expr(cc, TYPE_def);
			typ->init = init;

			if (init != NULL) {
				struct astNode val;
				if (eval(&val, init) == TYPE_int) {
					// add static const length property to array type.
					addLength(cc, typ, init);
					typ->size = (int) val.cint * typ->type->size;
					typ->offs = (size_t) val.cint;
					typ->cast = CAST_ref;
					typ->init = init;
					ref->cast = TYPE_any;

					dynarr = 0;
					if (val.cint < 0) {
						error(cc->rt, init->file, init->line, "positive integer constant expected, got `%+t`", init);
					}
				}
				else {
					error(cc->rt, init->file, init->line, "integer constant expected declaring array `%T`", ref);
				}
			}
		}
		if (dynarr) {
			// add dynamic length property to array type.
			symn len = addLength(cc, typ, NULL);
			dieif(len == NULL, "FixMe");
			ref->size = 2 * vm_size;	// slice is a struct {pointer data, int32 length}
			ref->cast = CAST_arr;
			len->offs = vm_size;		// offset for length.
			typ->cast = CAST_arr;
		}
		else {
			typ->cast = CAST_ref;
			typ->stat = 1;
		}
		skipTokens(cc, RIGHT_sqr, 1);

		// Multi dimensional arrays
		while (skipTok(cc, LEFT_sqr, 0)) {
			tmp = newdefn(cc, CAST_arr);
			traceLoop("%t", peekTok(cc, 0));
			tmp->type = typ->type;
			//~ typ->decl = tmp;
			tmp->decl = typ;
			typ->type = tmp;
			tmp->size = -1;
			//~ tmp->offs = vmOffset(cc->s, tmp);
			//~ tmp->stat = 1;
			typ = tmp;

			if (!peekTok(cc, RIGHT_sqr)) {
				struct astNode val;
				astn init = expr(cc, TYPE_def);
				if (eval(&val, init) == TYPE_int) {
					//~ ArraySize
					addLength(cc, typ, init);
					typ->size = (int) val.cint * typ->type->size;
					typ->offs = (size_t) val.cint;
					typ->init = init;
				}
				else {
					error(cc->rt, init->file, init->line, "integer constant expected");
				}
			}
			if (typ->init == NULL) {
				// add dynamic length property to array type.
				symn len = addLength(cc, typ, NULL);
				dieif(len == NULL, "FixMe");
				ref->size = 2 * vm_size;	// slice is a struct {pointer data, int32 length}
				ref->cast = CAST_arr;
				len->offs = vm_size;		// offset of length property.
				typ->cast = CAST_arr;
			}
			else {
				typ->cast = CAST_ref;
				typ->stat = 1;
			}

			if (dynarr != (typ->init == NULL)) {
				error(cc->rt, ref->file, ref->line, "invalid mixing of dynamic sized arrays");
			}

			skipTokens(cc, RIGHT_sqr, 1);
		}

		ref->arrB = base;
		dynarr = base->size;
		for (; typ; typ = typ->decl) {
			typ->size = (size_t) (dynarr *= typ->offs);
			//~ typ->offs = vmOffset(cc->s, typ);
		}
		ref->size = byref ? (size_t) vm_size : sizeOf(arr, 0);

		if (inout || byref || base->cast == CAST_ref) {
			// int& a[200] a contains 200 references to integers
			// int&& a[200] a contains 200 references to integers
			error(cc->rt, tag->file, tag->line, "TODO: declaring array of references: `%+T`", ref);
		}
		if (ref->call) {
			// int rgbCopy(int a, int b)[8] = [rgbCpy, rgbAnd, ...];
			error(cc->rt, tag->file, tag->line, "TODO: declaring array of functions: `%+T`", ref);
		}
		byref = 0;
	}

	/* TODO: in-out parameters
	if (inout) {
		ref->cast = TYPE_def;
	}
	else */if (byref) {
		ref->cast = CAST_ref;
	}
	return tag;
}

/** Parse declaration.
 * @brief parse the declaration of an alias, enum, struct, variable or function.
 * @param cc compiler context.
 * @param argv out parameter for function arguments.
 * @param mode
 * @return parsed syntax tree.
 */
static astn decl(ccContext cc, int mode) {
	int Attr = qual(cc, ATTR_const | ATTR_stat);
	int line = cc->line;
	char* file = cc->file;
	astn tag = NULL;

	if (peekTok(cc, TYPE_def)) {				// define
		if ((tag = decl_alias(cc)) != NULL) {

			//? enable static and const qualifiers
			//~ Attr &= ~(ATTR_stat | ATTR_const);

			redefine(cc, tag->ref.link);
		}
	}
	else if (peekTok(cc, ENUM_kwd)) {			// enum
		if ((tag = decl_enum(cc)) != NULL) {
			redefine(cc, tag->ref.link);
		}
	}
	else if (peekTok(cc, TYPE_rec)) {			// struct
		if ((tag = decl_struct(cc, Attr)) != NULL) {

			// static structures are valid
			Attr &= ~(ATTR_stat);

			redefine(cc, tag->ref.link);
		}
	}
	else if ((tag = decl_var(cc, NULL, 0))) {	// variable, function, ...
		symn typ = tag->type;
		symn ref = tag->ref.link;

		if ((mode & decl_NoInit) == 0) {
			/* function declaration with implementation.
			 * int sqr(int a) {return a * a;}
			 *		static const non indirect reference.
			 */
			if (ref->call && peekTok(cc, STMT_beg)) {
				symn tmp, result = NULL;
				int maxlevel = cc->maxlevel;

				enter(cc, tag);
				cc->maxlevel = cc->nest;
				ref->gdef = cc->func;
				cc->func = ref;

				result = install(cc, "result", CAST_ref, TYPE_any, sizeOf(typ, 1), typ, NULL);
				ref->flds = result;

				if (result) {
					result->decl = ref;

					// result is the first argument
					result->offs = sizeOf(result, 1);
					// TODO: ref->stat = 1;
					ref->size = result->offs + fixargs(ref, vm_size, 0 -result->offs);
				}

				// reinstall all args
				for (tmp = ref->prms; tmp; tmp = tmp->next) {
					//~ TODO: make just a symlink to the symbol, not a copy of it.
					//~ TODO: install(cc, tmp->name, TYPE_def, 0, 0, tmp->type, tmp->used);
					symn arg = install(cc, tmp->name, CAST_ref, TYPE_any, 0, NULL, NULL);
					if (arg != NULL) {
						arg->type = tmp->type;
						arg->flds = tmp->flds;
						arg->prms = tmp->prms;
						arg->arrB = tmp->arrB;
						arg->cast = tmp->cast;
						arg->size = tmp->size;
						arg->offs = tmp->offs;
						arg->Attr = tmp->Attr;
					}
				}

				ref->stat = (Attr & ATTR_stat) != 0;
				ref->init = stmt(cc, 1);

				if (ref->init == NULL) {
					ref->init = newnode(cc, STMT_beg);
					ref->init->type = cc->type_vid;
				}

				cc->func = cc->func->gdef;
				cc->maxlevel = maxlevel;
				ref->gdef = NULL;

				ref->flds = leave(cc, ref, 0);
				dieif(ref->flds != result, "FixMe %-T", ref);

				backTok(cc, newnode(cc, STMT_end));
			}
			/* function reference declaration.
			 * TODO: int sqrt(int a);		// forward declaration.
			 * TODO: 	static const indirect reference.
			 * int sqrt(int a) = sqrt_386;	// initialized 
			 *		static indirect reference.
			 */
			else if (ref->call) {
				ref->size = vm_size;
			}

			// variable or function initialization.
			if (peekTok(cc, ASGN_set)) {
				if (ref->call) {
					ref->cast = CAST_ref;
				}
				if (Attr & ATTR_const) {
					ref->cnst = 1;
				}
				if (!init_var(cc, ref)) {
					traceAst(tag);
					return NULL;
				}

			}
		}
		//TODO: ?removed: dieif(!ref->call && ref->size == 0, "FixMe: %-T", ref);

		// for (int a : range(10, 20)) ...
		if ((mode & decl_ItDecl) != 0) {
			if (peekTok(cc, PNCT_cln)) {
				backTok(cc, newnode(cc, STMT_end));
			}
		}

		cc->pfmt = ref;
		skipTokens(cc, STMT_end, 1);

		// TODO: functions type == return type
		if (typ->cast == CAST_vid && !ref->call) {	// void x;
			error(cc->rt, ref->file, ref->line, "invalid variable declaration of type `%+T`", typ);
		}

		if (Attr & ATTR_stat) {
			ref->stat = 1;
		}

		if (Attr & ATTR_const) {
			ref->cnst = 1;
		}

		Attr &= ~(ATTR_stat | ATTR_const);		// static and const qualifiers are valid
		tag->kind = TYPE_def;
		redefine(cc, ref);
	}

	if (Attr & ATTR_const) {
		error(cc->rt, file, line, "qualifier `const` can not be applied to `%+t`", tag);
	}
	if (Attr & ATTR_stat) {
		error(cc->rt, file, line, "qualifier `static` can not be applied to `%+t`", tag);
	}

	return tag;
}

/** Parse statement list.
 * @brief read a list of statements.
 * @param cc compiler context.
 * @param mode raise error if (decl / stmt) found
 * @return parsed syntax tree.
 */
static astn block(ccContext cc, int mode) {
	astn head = NULL;
	astn tail = NULL;

	while (peekTok(cc, TYPE_any)) {
		int stop = 0;
		astn node;
		traceLoop("%t", peekTok(cc, 0));

		switch (test(cc)) {
			default:
				break;
			case TYPE_any:	// end of file
			case RIGHT_par:
			case RIGHT_sqr:
			case RIGHT_crl:
				stop = 1;
				break;
		}
		if (stop) {
			break;
		}

		if ((node = stmt(cc, 0))) {
			if (tail == NULL) {
				head = node;
			}
			else {
				tail->next = node;
			}

			tail = node;
		}
	}

	return head;
	(void)mode;
}

/** Parse statement.
 * @brief parse a statement.
 * @param cc compiler context.
 * @param mode
 * @return parsed syntax tree.
 */
static astn stmt(ccContext cc, int mode) {
	astn node = NULL;
	ccToken qual = TYPE_any;			// static | parallel

	//~ invalid start of statement
	switch (test(cc)) {
		default:
			break;
		case RIGHT_crl:
		case RIGHT_par:
		case RIGHT_sqr:
		case 0:	// end of file
			traceAst(peekTok(cc, 0));
			skipTokens(cc, TYPE_any, 1);
			return NULL;
	}
	if (!peekTok(cc, TYPE_any)) {
		trace("null");
		return NULL;
	}

	// check statement construct
	if ((node = nextTok(cc, ATTR_stat))) {			// 'static' ('if' | 'for')
		switch (test(cc)) {
			case STMT_if:		// compile time if
			case STMT_for:		// loop unroll
				qual = ATTR_stat;
				break;
			default:
				backTok(cc, node);
				break;
		}
		node = NULL;
	}
	else if ((node = nextTok(cc, ATTR_paral))) {		// 'parallel' ('{' | 'for')
		switch (test(cc)) {
			case STMT_beg:		// parallel task
			case STMT_for:		// parallel loop
				qual = ATTR_paral;
				break;
			default:
				backTok(cc, node);
				break;
		}
		node = NULL;
	}

	// scan the statement
	if (skipTok(cc, STMT_end, 0)) {				// ;
		warn(cc->rt, 2, cc->file, cc->line, WARN_EMPTY_STATEMENT);
	}
	else if ((node = nextTok(cc, STMT_beg))) {	// { ... }
		int newscope = !mode;
		astn blk;

		if (newscope) {
			enter(cc, node);
		}

		if ((blk = block(cc, 0)) != NULL) {
			node->stmt.stmt = blk;
			node->type = cc->type_vid;
			node->cast = qual;
			qual = TYPE_any;
		}
		else {
			// remove nodes like: {{;{;};{}{}}}
			eatnode(cc, node);
			node = 0;
		}

		if (newscope) {
			leave(cc, NULL, 0);
		}

		skipTokens(cc, RIGHT_crl, 1);
	}
	else if ((node = nextTok(cc, STMT_if))) {	// if (...)
		int siffalse = cc->siff;
		int newscope = 1;

		skipTokens(cc, LEFT_par, 1);
		node->stmt.test = expr(cc, CAST_bit);
		skipTokens(cc, RIGHT_par, 1);

		// static if (true) does not need entering a new scope.
		// every declaration should be available outside if.
		if (qual == ATTR_stat) {
			struct astNode value;
			if (!eval(&value, node->stmt.test) || !peekTok(cc, STMT_beg)) {
				error(cc->rt, node->file, node->line, "invalid static if construct");
				traceAst(peekTok(cc, 0));
			}
			if (constbol(&value)) {
				newscope = 0;
			}

			cc->siff |= newscope;
		}

		if (newscope) {
			enter(cc, node);
		}
		node->stmt.stmt = stmt(cc, 1);	// no new scope & disable decl
		if (node->stmt.stmt != NULL) {	// suggest using `if (...) {...}`
			switch (node->stmt.stmt->kind) {
				default:
					warn(cc->rt, 4, cc->file, cc->line, WARN_USE_BLOCK_STATEMENT, node->stmt.stmt);
					node->stmt.stmt = blockNode(cc, node->stmt.stmt);
					break;

				// do not sugest using block statement for return, break and continue.
				case STMT_beg:
				case STMT_ret:
				case STMT_brk:
				case STMT_con:
					break;
			}
		}
		if (skipTok(cc, ELSE_kwd, 0)) {		// optionally parse else part
			if (newscope) {
				leave(cc, NULL, 0);
				enter(cc, node);
			}
			node->stmt.step = stmt(cc, 1);
			if (node->stmt.step != NULL) {
				switch (node->stmt.step->kind) {
					default:
						warn(cc->rt, 4, cc->file, cc->line, WARN_USE_BLOCK_STATEMENT, node->stmt.step);
						node->stmt.step = blockNode(cc, node->stmt.step);
						break;

					// do not sugest using block statement for else part if it is a block or if statement.
					case STMT_beg:
					case STMT_if:
						break;
				}
			}
		}
		if (newscope) {
			leave(cc, NULL, 0);
		}


		node->type = cc->type_vid;
		node->cast = qual;
		qual = TYPE_any;

		cc->siff = siffalse;
	}
	else if ((node = nextTok(cc, STMT_for))) {	// for (...)

		enter(cc, node);
		skipTokens(cc, LEFT_par, 1);
		if (!skipTok(cc, STMT_end, 0)) {		// init or iterate
			node->stmt.init = decl(cc, decl_NoDefs|decl_ItDecl);

			if (node->stmt.init == NULL) {
				node->stmt.init = expr(cc, CAST_vid);
				skipTokens(cc, STMT_end, 1);
			}
			else if (skipTok(cc, PNCT_cln, 0)) {		// iterate
				//~ for (Type value : iterable) {...}
				astn iton = expr(cc, CAST_vid); // iterate on expression

				dieif(node->stmt.init->kind != TYPE_def, "declaration expected");

				if (iton != NULL && iton->type != NULL) {
					// iterator step function.
					astn itStep = NULL;
					// variable to iterate with.
					astn itVar = node->stmt.init;

					// iterator initializer: `iterator(iterable)`
					astn itCtor = tagnode(cc, "iterator");
					symn sym = lookup(cc, cc->deft[itCtor->ref.hash], itCtor, iton, 0);

					if (sym != NULL) {
						// iterator constructor: `iterator(iterable)`
						astn itInit = opnode(cc, OPER_fnc, itCtor, iton);
						astn itNext = tagnode(cc, "next");
						itInit->type = sym->type;

						// for (Iterator it : iterable) ...
						if (sym->type == itVar->type) {

							// initialize iterator: Iterator it = iterator(iterable);
							symn x = linkOf(itVar);
							x->init = itInit;
							sym = x;

							// iterate using the iterator function: `bool next(Iterator &it)`
							itStep = opnode(cc, OPER_fnc, itNext, opnode(cc, OPER_adr, NULL, tagnode(cc, itVar->ref.name)));
						}
						// for (Type value : iterable) ...
						else {

							// declare iterator: Iterator .it = iterator(iterable);
							astn itTmp = tagnode(cc, ".it");
							symn x = declare(cc, CAST_ref, itTmp, sym->type);
							x->init = itInit;
							sym = x;

							// make for statement initializer a block having 2 declarations.
							node->stmt.init = newnode(cc, STMT_beg);
							node->stmt.init->type = cc->type_vid;
							node->stmt.init->cast = TYPE_rec;

							// set the 2 declaration in the block.
							node->stmt.init->stmt.stmt = itVar;
							itVar->next = itTmp;
							itTmp->next = NULL;

							// make node
							itTmp->kind = TYPE_def;
							itTmp->file = itVar->file;
							itTmp->line = itVar->line;

							// iterate using the iterator function: `bool next(Iterator &it, Type &&value)`
							itStep = opnode(cc, OPER_fnc, itNext,
								opnode(cc, OPER_com,
									opnode(cc, OPER_adr, NULL, tagnode(cc, itTmp->ref.name)),
									opnode(cc, OPER_adr, NULL, tagnode(cc, itVar->ref.name))
								)
							);
						}
					}

					if (itStep == NULL || !typeCheck(cc, NULL, sym->init)) {
						itStep = opnode(cc, OPER_fnc, tagnode(cc, "next"), NULL);
						error(cc->rt, node->file, node->line, "iterator not defined for `%T`", iton->type);
						info(cc->rt, node->file, node->line, "a function `%t(%T)` should be declared", itCtor, iton->type);
					}
					else if (typeCheck(cc, NULL, itStep) != cc->type_bol) {
						error(cc->rt, node->file, node->line, "iterator not found for `%+t`: %+t", iton, itStep);
						if (itStep->op.rhso->kind == OPER_com) {
							info(cc->rt, node->file, node->line, "a function `%t(%T &, %T): %T` should be declared", itStep->op.lhso, itStep->op.rhso->op.lhso->type, itStep->op.rhso->op.rhso->type, cc->type_bol);
						}
						else {
							info(cc->rt, node->file, node->line, "a function `%t(%T &): %T` should be declared", itStep->op.lhso, itStep->op.rhso->type, cc->type_bol);
						}
					}
					else {
						symn itFunNext = linkOf(itStep);
						if (itFunNext != NULL && itFunNext->prms != NULL) {
							if (itFunNext->prms->cast != CAST_ref) {
								error(cc->rt, itFunNext->file, itFunNext->line, "iterator arguments are not by ref");
							}
							if (itFunNext->prms->next && (itFunNext->prms->next->cast != CAST_ref && itFunNext->prms->next->cast != CAST_arr)) {
								error(cc->rt, itFunNext->file, itFunNext->line, "iterator arguments are not by ref");
							}
						}
					}

					itStep->cast = CAST_bit;
					node->stmt.test = itStep;
					sym->init->cast = TYPE_rec;
				}
				if (qual == ATTR_stat) {
					error(cc->rt, node->file, node->line, "invalid use of static iteration");
				}
				backTok(cc, newnode(cc, STMT_end));
			}
		}
		if (!skipTok(cc, STMT_end, 0)) {		// test
			node->stmt.test = expr(cc, CAST_bit);
			skipTokens(cc, STMT_end, 1);
		}
		if (!skipTok(cc, RIGHT_par, 0)) {		// incr
			node->stmt.step = expr(cc, CAST_vid);
			skipTokens(cc, RIGHT_par, 1);
		}
		node->stmt.stmt = stmt(cc, 1);	// no new scope & disable decl
		if (node->stmt.stmt != NULL) {	// suggest using `for (...) {...}`
			if (node->stmt.stmt->kind != STMT_beg) {
				warn(cc->rt, 4, cc->file, cc->line, WARN_USE_BLOCK_STATEMENT, node->stmt.stmt);
				node->stmt.stmt = blockNode(cc, node->stmt.stmt);
			}
		}
		leave(cc, NULL, 0);

		node->type = cc->type_vid;
		node->cast = qual;
		qual = TYPE_any;
	}
	else if ((node = nextTok(cc, STMT_brk))) {	// break;
		node->type = cc->type_vid;
		skipTokens(cc, STMT_end, 1);
	}
	else if ((node = nextTok(cc, STMT_con))) {	// continue;
		node->type = cc->type_vid;
		skipTokens(cc, STMT_end, 1);
	}
	else if ((node = nextTok(cc, STMT_ret))) {	// return;
		node->type = cc->type_vid;
		if (cc->func != NULL) {
			symn result = cc->func->flds;
			if (!peekTok(cc, STMT_end)) {
				astn val = expr(cc, CAST_vid);		// do lookup
				if (val == NULL) {/* error */ }
				else if (val->kind == CAST_ref && val->ref.link == result) {
					// skip 'return result;' statements
				}
				else if (result->type == cc->type_vid && val->type == result->type) {
					// return void expression from a function returning void.
					node->stmt.stmt = val;
				}
				else {
					node->stmt.stmt = opnode(cc, ASGN_set, lnknode(cc, result), val);
					node->stmt.stmt->type = val->type;
					if (!typeCheck(cc, NULL, node->stmt.stmt)) {
						error(cc->rt, val->file, val->line, "invalid return value: `%+t`", val);
					}
				}
			}
			else if (result->type != cc->type_vid) {
				warn(cc->rt, 4, cc->file, cc->line, "returning from function with no value");
			}
		}
		skipTokens(cc, STMT_end, 1);
	}
	else if ((node = decl(cc, TYPE_any))) {	// declaration
		if (mode) {
			error(cc->rt, node->file, node->line, "unexpected declaration `%+t`", node);
		}
	}
	else if ((node = expr(cc, CAST_vid))) {	// expression
		skipTokens(cc, STMT_end, 1);
		switch (node->kind) {
			default:
				warn(cc->rt, 1, node->file, node->line, WARN_INVALID_EXPRESSION_STATEMENT, node);
				break;

			case OPER_fnc:
			case ASGN_set:
				break;
		}
		node = wrapStmt(cc, node);
	}
	else if ((node = peekTok(cc, TYPE_any))) {
		error(cc->rt, node->file, node->line, "unexpected token: `%t`", node);
		skipTokens(cc, STMT_end, 1);
	}
	else {
		error(cc->rt, cc->file, cc->line, "unexpected end of file");
	}

	dieif(qual != 0, "FixMe");

	return node;
}

//#}

/**
 * @brief parse the source setted by source.
 * @param cc
 * @param asUnit
 * @param warn
 * @return
 */
static astn parse(ccContext cc, int asUnit, int warn) {
	astn root, oldRoot = cc->root;
	int level = cc->nest;

	cc->warn = warn;

	if (0) {
		astn ast, prev = NULL;
		while ((ast = nextTok(cc, TYPE_any))) {
			if (prev == NULL) {
				prev = ast;
			}
			else {
				prev->next = ast;
			}
			prev = ast;
		}
		backTok(cc, prev);
	}

	if ((root = block(cc, 0))) {
		if (asUnit != 0) {
			//! disables globals_on_stack values after execution.
			astn unit = newnode(cc, STMT_beg);
			unit->type = cc->type_vid;
			unit->file = cc->file;
			unit->line = 1;
			unit->cast = CAST_vid;
			unit->stmt.stmt = root;
			root = unit;
		}

		if (oldRoot) {
			astn end = oldRoot->stmt.stmt;
			if (end != NULL) {
				while (end->next) {
					end = end->next;
				}

				end->next = root;
			}
			else {
				oldRoot->stmt.stmt = root;
			}
			root = oldRoot;
		}
		cc->root = root;

		if (peekTok(cc, TYPE_any)) {
			error(cc->rt, cc->file, cc->line, "unexpected token: %t", peekTok(cc, TYPE_any));
			return 0;
		}

		if (cc->nest != level) {
			error(cc->rt, cc->file, cc->line, "premature end of file: %d", cc->nest);
			return NULL;
		}
	}

	return root;
}

/**
 * @brief Open a file or text for compilation.
 * @param rt runtime context.
 * @param file file name of input.
 * @param line first line of input.
 * @param text if not null, this will be compiled instead of the file.
 * @return compiler context or null on error.
 * @note invokes ccInit if not initialized.
 */
static ccContext ccOpen(rtContext rt, char* file, int line, char* text) {
	ccContext cc = rt->cc;

	if (cc == NULL) {
		// initilaize only once.
		cc = ccInit(rt, install_def, NULL);
		if (cc == NULL) {
			return NULL;
		}
	}

	if (source(cc, text == NULL, text ? text : file) != 0) {
		return NULL;
	}

	if (file != NULL) {
		cc->file = mapstr(cc, file, -1, -1);
	}
	else {
		cc->file = NULL;
	}

	cc->fin.nest = cc->nest;
	cc->line = line;
	return cc;
}

/**
 * @brief Close input file, ensuring it ends correctly.
 * @param cc compiler context.
 * @return number of errors.
 */
static int ccClose(ccContext cc) {
	astn ast;

	// not initialized
	if (cc == NULL)
		return -1;

	// check no token left to read
	if ((ast = peekTok(cc, TYPE_any))) {
		error(cc->rt, ast->file, ast->line, "unexpected: `%t`", ast);
		return -1;
	}

	// check we are on the same nesting level.
	if (cc->fin.nest != cc->nest) {
		error(cc->rt, cc->file, cc->line, "unexpected: end of input: %s", cc->fin._buf);
		return -1;
	}

	// close input
	source(cc, 0, NULL);

	// return errors
	return cc->rt->errCount;
}

/// Compile file or text; @see rtContext.api.ccDefCode
astn ccAddUnit(rtContext rt, int warn, char *file, int line, char *text) {
	ccContext cc = ccOpen(rt, file, line, text);
	if (cc == NULL) {
		error(rt, NULL, 0, "can not open: %s", file);
		return 0;
	}
	astn unit = parse(cc, 0, warn);
	if (ccClose(cc) != 0) {
		return NULL;
	}
	return unit;
}

/// @see: header
int ccAddLib(rtContext rt, int libInit(rtContext), int warn, char *file) {
	int errorCode = libInit(rt);
	if (errorCode == 0 && file != NULL) {
		return ccAddUnit(rt, warn, file, 1, NULL) != NULL;
	}
	return errorCode == 0;
}

static symn installref(rtContext rt, const char* prot, astn* argv) {
	astn root, args;
	symn result = NULL;
	int warn, errc = rt->errCount;

	if (!ccOpen(rt, NULL, 0, (char*)prot)) {
		trace("FixMe");
		return NULL;
	}

	warn = rt->cc->warn;

	// enable all warnings
	rt->cc->warn = 9;
	root = decl_var(rt->cc, &args, decl_NoDefs | decl_NoInit);

	dieif(root == NULL, "error declaring: %s", prot);

	dieif(!skipTok(rt->cc, STMT_end, 0), "`;` expected declaring: %s", prot);

	dieif(ccClose(rt->cc) != 0, "FixMe");

	dieif(root->kind != CAST_ref, "FixMe %+t", root);

	if ((result = root->ref.link)) {
		dieif(result->kind != CAST_ref, "FixMe");
		*argv = args;
		result->cast = TYPE_any;
	}

	rt->cc->warn = warn;
	return errc == rt->errCount ? result : NULL;
}

/// Install a native function; @see rtContext.api.ccDefCall
symn ccDefCall(rtContext rt, vmError libc(libcContext), void *data, const char *proto) {
	symn param, sym = NULL;
	int stdiff = 0;
	astn args = NULL;

	dieif(libc == NULL || !proto, "FixMe");

	//~ from: int64 zxt(int64 val, int offs, int bits);
	//~ make: define zxt(int64 val, int offs, int bits) = emit(int64, libc(25), i64(val), i32(offs), i32(bits));

	if ((sym = installref(rt, proto, &args))) {
		struct libc* lc = NULL;
		symn link = newdefn(rt->cc, EMIT_kwd);
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
						n->cast = s->cast;
					}
				}
				s = linkOf(arg);
				if (s && arg) {
					arg->cast = s->cast;
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
		// TODO: libcall should be static
		//sym->stat = 1;
		//sym->cnst = 1;

		lc->call = libc;
		lc->data = data;
		lc->pos = libcpos;
		lc->sym = sym;

		lc->chk = stdiff / 4;

		stdiff -= sizeOf(sym->type, 1);
		lc->pop = stdiff / 4;

		// make non reference parameters symbolic by default
		for (param = sym->prms; param; param = param->next) {
			if (param->cast != CAST_ref && !param->call) {
				param->kind = TYPE_def;
			}
		}
	}
	else {
		error(rt, NULL, 0, "install(`%s`)", proto);
	}

	return sym;
}
