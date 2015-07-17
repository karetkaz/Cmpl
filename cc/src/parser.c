/*******************************************************************************
 *   File: lexer.c
 *   Date: 2011/06/23
 *   Desc: input and lexer
 *******************************************************************************

Lexical elements

*/
#include "core.h"

/** Construct arguments.
 * @brief construct a new node for function arguments
 * @param cc compiler context.
 * @param lhs arguments or first argument.
 * @param rhs next argument.
 * @return if lhs is null return (rhs) else return (lhs, rhs).
 */
static inline astn argnode(ccState cc, astn lhs, astn rhs) {
	return lhs ? opnode(cc, OPER_com, lhs, rhs) : rhs;
}

/** Construct block node.
 * @brief construct a new node for block of statements
 * @param cc compiler context.
 * @param node next argument.
 * @return if lhs is null return (rhs) else return (lhs, rhs).
 */
static inline astn blockNode(ccState cc, astn node) {
	astn block = newnode(cc, STMT_beg);
	block->cst2 = TYPE_vid;
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
static inline astn tagnode(ccState cc, char* name) {
	astn ast = NULL;
	if (cc != NULL && name != NULL) {
		ast = newnode(cc, TYPE_ref);
		if (ast != NULL) {
			size_t slen = strlen(name);
			ast->kind = TYPE_ref;
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
static symn addLength(ccState cc, symn sym, astn init) {
	/* TODO: review return value.
	 *
	 * dynamic array size kind: ATTR_const | TYPE_ref
	 * static array size kind: ATTR_stat | ATTR_const | TYPE_def
	 *     using static we get the warning accessing static member through instance.
	 */
	ccToken kind = ATTR_const | (init != NULL ? TYPE_def : TYPE_ref);
	symn args = sym->flds;
	symn result = NULL;

	enter(cc, NULL);
	result = install(cc, "length", kind, TYPE_i32, vm_size, cc->type_i32, init);
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
static void redefine(ccState cc, symn sym) {
	struct astNode tag;
	symn ptr;

	if (sym == NULL) {
		return;
	}

	memset(&tag, 0, sizeof(tag));
	tag.kind = TYPE_ref;
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
		error(cc->s, sym->file, sym->line, "redefinition of `%-T`", sym);
		if (ptr->file && ptr->line) {
			info(cc->s, ptr->file, ptr->line, "first defined here as `%-T`", ptr);
		}
	}
}

/** TODO: to be deleted. replace with js like initialization.
 * @brief make constructor using fields as arguments.
 * @param cc compiler context.
 * @param rec the record.
 * @return the constructors symbol.
 */
static symn ctorArg(ccState cc, symn rec) {
	symn ctor = install(cc, rec->name, TYPE_def, TYPE_any, 0, rec, NULL);
	if (ctor != NULL) {
		astn root = NULL;
		symn arg, newarg;

		// TODO: params should not be dupplicated: ctor->prms = rec->prms
		enter(cc, NULL);
		for (arg = rec->flds; arg; arg = arg->next) {

			if (arg->stat)
				continue;

			if (arg->kind != TYPE_ref)
				continue;

			newarg = install(cc, arg->name, TYPE_def, arg->type->cast, 0, arg->type, NULL);
			//~ newarg = install(cc, arg->name, TYPE_ref, TYPE_rec, 0, arg->type, NULL);
			//~ newarg->cast = arg->type->cast;

			if (newarg != NULL) {
				astn tag;
				newarg->call = arg->call;
				newarg->prms = arg->prms;

				tag = lnknode(cc, newarg);
				tag->cst2 = arg->cast;
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
		cast = ast->cst2;
	}

	if (!eval(&tmp, ast)) {
		trace("%+k", ast);
		return 0;
	}

	ast->kind = tmp.kind;
	ast->cint = tmp.cint;

	switch (ast->cst2 = cast) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return 0;

		case TYPE_vid:
		case TYPE_any:
		case TYPE_rec:
			return 0;

		case TYPE_bit:
			ast->cint = constbol(ast);
			ast->kind = TYPE_int;
			break;

		case TYPE_u32:
		case TYPE_i32:
		case TYPE_i64:
			ast->cint = constint(ast);
			ast->kind = TYPE_int;
			break;

		case TYPE_f32:
		case TYPE_f64:
			ast->cflt = constflt(ast);
			ast->kind = TYPE_flt;
			break;
	}

	return ast->kind;
}

//#{~~~~~~~~~ Parser ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//~ TODO: to be deleted: use expr instead, then check if it is a typename.
static astn type(ccState cc/* , int mode */) {	// type(.type)*
	symn def = NULL;
	astn tok, list = NULL;
	while ((tok = next(cc, TYPE_ref))) {

		symn loc = def ? def->flds : cc->deft[tok->ref.hash];
		symn sym = lookup(cc, loc, tok, NULL, 0);

		tok->next = list;
		list = tok;

		def = sym;

		if (!istype(sym)) {
			def = NULL;
			break;
		}

		if ((tok = next(cc, OPER_dot))) {
			tok->next = list;
			list = tok;
		}
		else
			break;

	}
	if (!def && list) {
		while (list) {
			astn back = list;
			list = list->next;
			backTok(cc, back);
			//~ trace("not a type `%k`", back);
		}
		list = NULL;
	}
	else if (list) {
		list->type = def;
	}
	return list;
}

static astn stmt(ccState, int mode);	// parse statement		(mode: enable decl)
static astn decl(ccState, int mode);	// parse declaration	(mode: enable spec)
//~ astn args(ccState, int mode);		// parse arguments		(mode: ...)
//~ astn unit(ccState, int mode);		// parse unit			(mode: script/unit)

/** Parse qualifiers.
 * @brief scan for qualifiers: const static ?paralell
 * @param cc compiler context.
 * @param mode scannable qualifiers.
 * @return qualifiers.
 */
static int qual(ccState cc, int mode) {
	int result = 0;
	astn ast;

	while ((ast = peekTok(cc, TYPE_any))) {
		trloop("%k", peekTok(cc, 0));
		switch (ast->kind) {

			default:
				//~ trace("next %k", peek(s));
				return result;

			case ATTR_const:
				if (!(mode & ATTR_const))
					return result;

				if (result & ATTR_const) {
					error(cc->s, ast->file, ast->line, "qualifier `%t` declared more than once", ast);
				}
				result |= ATTR_const;
				skip(cc, TYPE_any);
				break;

			case ATTR_stat:
				if (!(mode & ATTR_stat))
					return result;

				if (result & ATTR_stat) {
					error(cc->s, ast->file, ast->line, "qualifier `%t` declared more than once", ast);
				}
				result |= ATTR_stat;
				skip(cc, TYPE_any);
				break;
		}
	}

	return result;
}

/** TODO: remove typechecking.
 * @brief Parse expression.
 * @param cc compiler context.
 * @param mode
 * @return
 */
static astn expr(ccState cc, int mode) {
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

	while ((tok = next(cc, TYPE_any))) {					// parse
		int pri = level << 4;
		trloop("%k", peek(cc));
		// statements are not allowed in expressions !!!!
		if (tok->kind >= STMT_beg && tok->kind <= STMT_end) {
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
				if (tok->kind != STMT_do) {
					*--prec = tok;
				}
			} break;

			default: {
				if (tok_tbl[tok->kind].argc) {			// operator
					if (unary) {
						*post++ = 0;
						error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
					}
					unary = 1;
					goto tok_op;
				}
				else {
					if (!unary)
						error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
					unary = 0;
					goto tok_id;
				}
			} break;

			case PNCT_lp: {			// '('
				if (unary)			// a + (3*b)
					*post++ = 0;
				else if (peekTok(cc, PNCT_rp)) {	// a()
					unary = 2;
				}
				else {
					unary = 1;
				}

				tok->kind = OPER_fnc;
				sym[++level] = '(';

			} goto tok_op;
			case PNCT_rp: {			// ')'
				if (unary == 2 && sym[level] == '(') {
					tok->kind = STMT_do;
					*post++ = NULL;
					level -= 1;
				}
				else if (!unary && sym[level] == '(') {
					tok->kind = STMT_do;
					level -= 1;
				}
				else if (level == 0) {
					backTok(cc, tok);
					tok = 0;
					break;
				}
				else {
					error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
					break;
				}
				unary = 0;
			} goto tok_op;
			case PNCT_lc: {			// '['
				if (!unary)
					tok->kind = OPER_idx;
				else {
					error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
					//~ break?;
				}
				sym[++level] = '[';
				unary = 1;
			} goto tok_op;
			case PNCT_rc: {			// ']'
				if (!unary && sym[level] == '[') {
					tok->kind = STMT_do;
					level -= 1;
				}
				else if (!level) {
					backTok(cc, tok);
					tok = 0;
					break;
				}
				else {
					error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
					break;
				}
				unary = 0;
			} goto tok_op;
			case PNCT_qst: {		// '?'
				if (unary) {
					*post++ = 0;		// ???
					error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
				}
				tok->kind = OPER_sel;
				sym[++level] = '?';
				unary = 1;
			} goto tok_op;
			case PNCT_cln: {		// ':'
				if (!unary && sym[level] == '?') {
					tok->kind = STMT_do;
					level -= 1;
				}
				else if (level == 0) {
					backTok(cc, tok);
					tok = 0;
					break;
				}
				else {
					error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
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
					error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
				unary = 1;
			} goto tok_op;
			case OPER_cmt: {		// '~'
				if (!unary)
					error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
				unary = 1;
			} goto tok_op;
			case OPER_com: {		// ','
				if (unary)
					error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
				// skip trailing commas
				switch (test(cc)) {
					case STMT_end:
					case PNCT_rc:
					case PNCT_rp:
						warn(cc->s, 1, tok->file, tok->line, "skipping trailing comma before `%k`", peekTok(cc, 0));
						continue;
					default:
						break;
				}
				unary = 1;
			} goto tok_op;
		}
		if (post >= prec) {
			error(cc->s, cc->file, cc->line, "Expression too complex");
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
		error(cc->s, cc->file, cc->line, "missing %s, %k", missing, peekTok(cc, TYPE_any));
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
		if (!typecheck(cc, NULL, tok)) {
			if (cc->root != NULL) {
				error(cc->s, tok->file, tok->line, "invalid expression: `%+k` in `%+k`", cc->root, tok);
			}
			else {
				error(cc->s, tok->file, tok->line, "invalid expression: `%+k`", tok);
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
static astn args(ccState cc, int mode) {
	astn root = NULL;

	if (peekTok(cc, PNCT_rp)) {
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

		if (!skip(cc, OPER_com)) {
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
static astn init_var(ccState cc, symn var) {
	if (skip(cc, ASGN_set)) {
		symn typ = var->type;
		ccToken cast = var->cast;
		int mkcon = var->cnst;
		ccToken arrayInit = TYPE_any;

		if (typ->kind == TYPE_arr) {
			if (skip(cc, STMT_beg)) {
				arrayInit = STMT_end;
			}
			else if (skip(cc, PNCT_lc)) {
				arrayInit = PNCT_rc;
			}
			else if (skip(cc, PNCT_lp)) {
				arrayInit = PNCT_rp;
			}
		}
		var->init = expr(cc, TYPE_def);
		if (arrayInit != TYPE_any) {
			skiptok(cc, arrayInit, 1);
		}

		if (var->init != NULL) {

			//~ TODO: try to typecheck the same as using ASGN_set
			// assigning an emit expression: ... x = emit(struct, ...)
			if (var->init->type == cc->emit_opc) {
				var->init->type = var->type;
				var->init->cst2 = cast;
				if (mkcon) {
					error(cc->s, var->file, var->line, "invalid constant initialization `%+k`", var->init);
				}
			}

			// assigning to an array
			else if (arrayInit) {
				astn init = var->init;
				symn base = var->prms;
				int nelem = 1;

				// TODO: base should not be stored in var->args !!!
				cast = base ? base->cast : TYPE_any;

				if (base == typ->type && typ->init == NULL) {
					mkcon = 0;
				}

				while (init->kind == OPER_com) {
					astn val = init->op.rhso;
					if (!canAssign(cc, base, val, 0)) {
						trace("canAssignCast(%-T, %+k, %t)", base, val, cast);
						return NULL;
					}
					if (!castTo(val, cast)) {
						trace("canAssignCast(%-T, %+k, %t)", base, val, cast);
						return NULL;
					}
					if (mkcon && !mkConst(val, cast)) {
						trace("canAssignCast(%-T, %+k, %t)", base, val, cast);
						//~ return NULL;
					}
					init = init->op.lhso;
					nelem += 1;
				}
				if (!canAssign(cc, base, init, 0)) {
					trace("canAssignArray(%-T, %+k, %t)", base, init, cast);
					return NULL;
				}
				if (!castTo(init, cast)) {
					trace("canAssignCast(%-T, %+k, %t)", base, init, cast);
					return NULL;
				}
				if (mkcon && !mkConst(init, cast)) {
					trace("canAssignCast(%-T, %+k, %t)", base, init, cast);
					//~ return NULL;
				}

				// int a[] = {1, 2, 3, 4, 5, 6};
				// TODO:! var initialization parsing should not modify the variable.
				if (base == typ->type && typ->init == NULL && var->cnst) {
					typ->init = intnode(cc, nelem);
					// ArraySize
					typ->size = nelem * typ->type->size;
					typ->offs = (size_t) nelem;
					typ->cast = TYPE_ref;
					typ->stat = 1;
					var->cast = TYPE_any;
					var->size = typ->size;

					addLength(cc, typ, typ->init);
				}
				else if (typ->init == NULL) {
					// can not initialize a slice with values.
					error(cc->s, var->file, var->line, "invalid slice initialization `%+k`", var->init);
					return NULL;
				}
				var->init->type = var->prms;
			}

			// check if value can be assigned to variable.
			if (canAssign(cc, var, var->init, cast == TYPE_ref)) {

				// assign enum variable to base type.
				if (var->cast != ENUM_kwd && var->init->type->cast == ENUM_kwd) {
					astn ast = var->init;
					symn eTyp = ast->type->type;
					if (!castTo(ast, castOf(ast->type = eTyp))) {
						trace("%+k", ast);
						return NULL;
					}
				}

				// TODO:! var initialization parsing should not modify the variable.
				if (var->cast == TYPE_arr) {
					var->size = 2 * vm_size;
				}

				//~ trace("%+T -> %t (%s:%u)", var, cast, var->file, var->line);
				if (!castTo(var->init, cast)) {
					trace("%t", cast);
					return NULL;
				}
			}
			else {
				error(cc->s, var->file, var->line, "invalid initialization `%-T` := `%+k`", var, var->init);
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
static astn decl_alias(ccState cc) {
	astn tag = NULL;
	symn def = NULL;
	symn typ = NULL;

	if (!skiptok(cc, TYPE_def, 1)) {	// 'define'
		trace("%+k", tag);
		return NULL;
	}

	if (!(tag = next(cc, TYPE_ref))) {	// name?
		error(cc->s, cc->file, cc->line, "Identifyer expected");
		skiptok(cc, STMT_do, 1);
		return NULL;
	}

	if (skip(cc, ASGN_set)) {				// const: define PI = 3.14...;
		// can not call decl_init
		// TODO: check if locals are used.
		astn val = expr(cc, TYPE_def);
		if (val != NULL) {
			def = declare(cc, TYPE_def, tag, val->type);
			if (!isType(val)) {
				if (isConst(val)) {
					def->stat = 1;
					def->cnst = 1;
				}
				else if (isStatic(cc, val)) {
					def->stat = 1;
				}
				def->init = val;
			}
			typ = def->type;
		}
		else {
			error(cc->s, cc->file, cc->line, "expression expected");
		}
	}
	else if (skip(cc, PNCT_lp)) {			// inline: define isNan(float64 x) = x != x;
		astn init = NULL;
		symn param;

		def = declare(cc, TYPE_def, tag, NULL);
		def->name = NULL; // disable lookup as variable.

		enter(cc, NULL);
		args(cc, TYPE_def);
		skiptok(cc, PNCT_rp, 1);

		if (skip(cc, ASGN_set)) {
			// define sqr(int a) = (a * a);
			if ((init = expr(cc, TYPE_def))) {
				typ = init->type;
			}
		}

		if (!(param = leave(cc, def, 0))) {
			param = cc->void_tag->ref.link;
		}

		def->name = tag->ref.name;
		def->stat = isStatic(cc, init);
		def->cnst = isConst(init);
		def->type = typ;
		def->prms = param;
		def->init = init;
		def->call = 1;

		//* TODO: in-out parameters: remove
		// TODO: this should go to fixargs
		for (param = def->prms; param; param = param->next) {
			// is explicitly set to be cached.
			if (param->cast == TYPE_ref) {
				param->cast = param->type->cast;
			}
			else {
				int used = usages(param) - 1;
				// warn to cache if it is used more than once
				if (used > 1) {
					warn(cc->s, 8, param->file, param->line, "parameter `%T` may be cached (used %d times in expression)", param, used);
				}
				else {
					param->kind = TYPE_def;
				}
			}
		}// */

		if (init == NULL) {
			error(cc->s, cc->file, cc->line, "expression expected");
		}
	}
	else {
		error(cc->s, tag->file, tag->line, "typename excepted");
		return NULL;
	}

	if (!skiptok(cc, STMT_do, 1)) {
		trace("%+k", tag);
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
static astn decl_enum(ccState cc) {
	symn def = NULL, base = cc->type_i32;
	astn tok, tag;

	int enuminc = 1;
	int64_t lastval = -1;

	if (!skiptok(cc, ENUM_kwd, 1)) {	// 'enum'
		trace("%+k", peekTok(cc, 0));
		return NULL;
	}

	tag = next(cc, TYPE_ref);			// ref?

	if (skip(cc, PNCT_cln)) {			// ':' type
		base = NULL;
		if ((tok = expr(cc, TYPE_def))) {
			if (isType(tok)) {
				base = linkOf(tok);
			}
			else {
				error(cc->s, cc->file, cc->line, "typename expected, got `%k`", tok);
			}
		}
		else {
			error(cc->s, cc->file, cc->line, "typename expected, got `%k`", peekTok(cc, TYPE_any));
		}
		enuminc = base && (base->cast == TYPE_i32 || base->cast == TYPE_i64);
	}

	if (!skiptok(cc, STMT_beg, 1)) {	// '{'
		trace("%+k", peekTok(cc, 0));
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
		trloop("%k", peek(cc));

		if (peekTok(cc, STMT_end)) {
			break;
		}

		if ((tok = next(cc, TYPE_ref))) {
			// HACK: declare as a variable to be assignable, then revert it to be an alias.
			symn ref = declare(cc, TYPE_ref, tok, base);
			ref->stat = ref->cnst = 1;
			redefine(cc, ref);

			if (init_var(cc, ref)) {
				if (!mkConst(ref->init, TYPE_any)) {
					warn(cc->s, 2, ref->file, ref->line, "constant expected, got: `%+k`", ref->init);
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
				error(cc->s, ref->file, ref->line, "constant expected");
			}
			ref->kind = TYPE_def;
		}
		skiptok(cc, STMT_do, 1);
	}

	if (def != NULL) {
		def->flds = leave(cc, def, 1);
		def->prms = base->prms;
		def->cast = ENUM_kwd;
	}

	if (!skiptok(cc, STMT_end, 1)) {	// '}'
		trace("%+k", tag);
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
static astn decl_struct(ccState cc, int Attr) {
	symn type = NULL, base = NULL;
	astn tok, tag;

	int byref = 0;
	unsigned pack = vm_size;

	if (!skiptok(cc, TYPE_rec, 1)) {	// 'struct'
		trace("%+k", peekTok(cc, 0));
		return NULL;
	}

	tag = next(cc, TYPE_ref);			// ref

	if (skip(cc, PNCT_cln)) {			// ':' type or pack
		if ((tok = expr(cc, TYPE_def))) {
			if (tok->kind == TYPE_int) {
				if (tok->cint > 16 || (tok->cint & -tok->cint) != tok->cint) {
					error(cc->s, tok->file, tok->line, "alignment must be a small power of two, not %k", tok);
				}
				else if (Attr == ATTR_stat) {
					error(cc->s, cc->file, cc->line, "alignment can not be applied to static struct");
				}
				pack = (unsigned) tok->cint;
			}
			else if (isType(tok)) {
				base = linkOf(tok);
			}
			else {
				error(cc->s, cc->file, cc->line, "alignment must be an integer constant");
			}
		}
		else {
			error(cc->s, cc->file, cc->line, "alignment must be an integer constant, got `%k`", peekTok(cc, TYPE_any));
		}
	}

	if (!skiptok(cc, STMT_beg, 1)) {	// '{'
		trace("%+k", peekTok(cc, 0));
		return NULL;
	}

	if (tag == NULL) {
		error(cc->s, cc->file, cc->line, "identifier expected");
		tag = tagnode(cc, ".anonymous");
		tag->kind = TYPE_def;
		enter(cc, tag);
	}
	type = declare(cc, ATTR_stat | ATTR_const | TYPE_rec, tag, cc->type_rec);
	tag->kind = TYPE_def;
	enter(cc, tag);

	while (peekTok(cc, TYPE_any)) {			// struct members
		trloop("%k", peekTok(cc, 0));

		if (peekTok(cc, STMT_end)) {
			break;
		}

		if (!decl(cc, 0)) {
			error(cc->s, cc->file, cc->line, "declaration expected, got: `%+k`", peekTok(cc, TYPE_any));
			skiptok(cc, STMT_do, 0);
		}
	}

	type->flds = leave(cc, type, (Attr & ATTR_stat) != 0);
	type->prms = type->flds;
	type->size = fixargs(type, pack, 0);

	if (!skiptok(cc, STMT_end, 1)) {	// '}'
		trace("%+k", peekTok(cc, 0));
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
		type->cast = TYPE_vid;
	}
	else {
		if (type->flds && pack == vm_size && !byref) {
			ctorArg(cc, type);
		}

		if (!type->flds && !type->type) {
			warn(cc->s, 2, cc->file, cc->line, "empty declaration");
		}
	}

	// Error checking
	if (base && base->cast != TYPE_ref) {
		// TODO: to be removed: declarations like: struct hex32: int32 { };
		if (skip(cc, STMT_do)) {
			warn(cc->s, 1, tag->file, tag->line, "deprecated declaration of `%k`", tag);
		}
		else {
			error(cc->s, tag->file, tag->line, "must extend a reference type, not `%+T`", base);
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
astn decl_var(ccState cc, astn* argv, int mode) {
	symn typ, ref = NULL;
	astn tag = NULL;
	int inout, byref;

	if (!(tag = type(cc))) {
		trace("%+k", peekTok(cc, 0));
		return NULL;
	}
	typ = tag->type;

	inout = skip(cc, OPER_lnd);
	byref = inout || skip(cc, OPER_and);

	if (typ->cast == TYPE_ref) {
		if (byref) {
			warn(cc->s, 2, tag->file, tag->line, "ignoring &, %-T is already a reference type", typ);
		}
		//byref = 1;
	}

	if (!(tag = next(cc, TYPE_ref))) {
		if (mode != TYPE_def) {
			trace("id expected, not `%k` at(%s:%u)", peekTok(cc, 0), cc->file, cc->line);
			return NULL;
		}
		tag = tagnode(cc, "");
	}

	ref = declare(cc, TYPE_ref, tag, typ);
	ref->size = byref ? (size_t) vm_size : sizeOf(typ, 1);

	if (argv != NULL) {
		*argv = NULL;
	}

	if (skip(cc, PNCT_lp)) {			// int a(...)
		astn argroot;
		enter(cc, tag);
		argroot = args(cc, TYPE_ref);
		ref->prms = leave(cc, ref, 0);
		skiptok(cc, PNCT_rp, 1);

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
			error(cc->s, tag->file, tag->line, "TODO: declaration returning reference: `%+T`", ref);
		}
		byref = 0;
	}

	if (skip(cc, PNCT_lc)) {			// int a[...]
		symn arr = newdefn(cc, TYPE_arr);
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

		if (!peekTok(cc, PNCT_rc)) {
			astn init = expr(cc, TYPE_def);
			typ->init = init;

			if (init != NULL) {
				struct astNode val;
				if (eval(&val, init) == TYPE_int) {
					// add static const length property to array type.
					addLength(cc, typ, init);
					typ->size = (int) val.cint * typ->type->size;
					typ->offs = (size_t) val.cint;
					typ->cast = TYPE_ref;
					typ->init = init;
					ref->cast = TYPE_any;

					dynarr = 0;
					if (val.cint < 0) {
						error(cc->s, init->file, init->line, "positive integer constant expected, got `%+k`", init);
					}
				}
				else {
					error(cc->s, init->file, init->line, "integer constant expected declaring array `%T`", ref);
				}
			}
		}
		if (dynarr) {
			// add dynamic length property to array type.
			symn len = addLength(cc, typ, NULL);
			dieif(len == NULL, "FixMe");
			ref->size = 2 * vm_size;	// slice is a struct {pointer data, int32 length}
			ref->cast = TYPE_arr;
			len->offs = vm_size;		// offset for length.
			typ->cast = TYPE_arr;
		}
		else {
			typ->cast = TYPE_ref;
			typ->stat = 1;
		}
		skiptok(cc, PNCT_rc, 1);

		// Multi dimensional arrays
		while (skip(cc, PNCT_lc)) {
			tmp = newdefn(cc, TYPE_arr);
			trloop("%k", peekTok(cc, 0));
			tmp->type = typ->type;
			//~ typ->decl = tmp;
			tmp->decl = typ;
			typ->type = tmp;
			tmp->size = -1;
			//~ tmp->offs = vmOffset(cc->s, tmp);
			//~ tmp->stat = 1;
			typ = tmp;

			if (!peekTok(cc, PNCT_rc)) {
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
					error(cc->s, init->file, init->line, "integer constant expected");
				}
			}
			if (typ->init == NULL) {
				// add dynamic length property to array type.
				symn len = addLength(cc, typ, NULL);
				dieif(len == NULL, "FixMe");
				ref->size = 2 * vm_size;	// slice is a struct {pointer data, int32 length}
				ref->cast = TYPE_arr;
				len->offs = vm_size;		// offset of length property.
				typ->cast = TYPE_arr;
			}
			else {
				typ->cast = TYPE_ref;
				typ->stat = 1;
			}

			if (dynarr != (typ->init == NULL)) {
				error(cc->s, ref->file, ref->line, "invalid mixing of dynamic sized arrays");
			}

			skiptok(cc, PNCT_rc, 1);
		}

		ref->prms = base;	// fixme (temporarly used)
		dynarr = base->size;
		for (; typ; typ = typ->decl) {
			typ->size = (size_t) (dynarr *= typ->offs);
			//~ typ->offs = vmOffset(cc->s, typ);
		}
		ref->size = byref ? (size_t) vm_size : sizeOf(arr, 0);

		if (inout || byref || base->cast == TYPE_ref) {
			// int& a[200] a contains 200 references to integers
			// int&& a[200] a contains 200 references to integers
			error(cc->s, tag->file, tag->line, "TODO: declaring array of references: `%+T`", ref);
		}
		if (ref->call) {
			// int rgbCopy(int a, int b)[8] = [rgbCpy, rgbAnd, ...];
			error(cc->s, tag->file, tag->line, "TODO: declaring array of functions: `%+T`", ref);
		}
		byref = 0;
	}

	/* TODO: in-out parameters
	if (inout) {
		ref->cast = TYPE_def;
	}
	else */if (byref) {
		ref->cast = TYPE_ref;
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
static astn decl(ccState cc, int mode) {
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

				result = install(cc, "result", TYPE_ref, TYPE_any, sizeOf(typ, 1), typ, NULL);
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
					symn arg = install(cc, tmp->name, TYPE_ref, TYPE_any, 0, NULL, NULL);
					if (arg != NULL) {
						arg->type = tmp->type;
						arg->flds = tmp->flds;
						arg->prms = tmp->prms;
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

				backTok(cc, newnode(cc, STMT_do));
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
					ref->cast = TYPE_ref;
				}
				if (Attr & ATTR_const) {
					ref->cnst = 1;
				}
				if (!init_var(cc, ref)) {
					trace("%+k", tag);
					return NULL;
				}

			}
		}
		//TODO: ?removed: dieif(!ref->call && ref->size == 0, "FixMe: %-T", ref);

		// for (int a : range(10, 20)) ...
		if ((mode & decl_ItDecl) != 0) {
			if (peekTok(cc, PNCT_cln)) {
				backTok(cc, newnode(cc, STMT_do));
			}
		}

		cc->pfmt = ref;
		skiptok(cc, STMT_do, 1);

		// TODO: functions type == return type
		if (typ->cast == TYPE_vid && !ref->call) {	// void x;
			error(cc->s, ref->file, ref->line, "invalid variable declaration of type `%+T`", typ);
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
		error(cc->s, file, line, "qualifier `const` can not be applied to `%+k`", tag);
	}
	if (Attr & ATTR_stat) {
		error(cc->s, file, line, "qualifier `static` can not be applied to `%+k`", tag);
	}

	//~ dieif(Attr, "FixMe: %+k", tag);
	return tag;
}

/** Parse statement list.
 * @brief read a list of statements.
 * @param cc compiler context.
 * @param mode raise error if (decl / stmt) found
 * @return parsed syntax tree.
 */
static astn block(ccState cc, int mode) {
	astn head = NULL;
	astn tail = NULL;

	while (peekTok(cc, TYPE_any)) {
		int stop = 0;
		astn node;
		trloop("%k", peekTok(cc, 0));

		switch (test(cc)) {
			default:
				break;
			case TYPE_any :	// end of file
			case PNCT_rp :
			case PNCT_rc :
			case STMT_end :
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
static astn stmt(ccState cc, int mode) {
	astn node = NULL;
	ccToken qual = TYPE_any;			// static | parallel

	//~ invalid start of statement
	switch (test(cc)) {
		default:
			break;
		case STMT_end:
		case PNCT_rp:
		case PNCT_rc:
		case 0:	// end of file
			trace("%+k", peekTok(cc, 0));
			skiptok(cc, TYPE_any, 1);
			return NULL;
	}
	if (!peekTok(cc, TYPE_any)) {
		trace("null");
		return NULL;
	}

	// check statement construct
	if ((node = next(cc, ATTR_stat))) {			// 'static' ('if' | 'for')
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
	else if ((node = next(cc, ATTR_paral))) {		// 'parallel' ('{' | 'for')
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
	if (skip(cc, STMT_do)) {				// ;
		warn(cc->s, 2, cc->file, cc->line, WARN_EMPTY_STATEMENT);
	}
	else if ((node = next(cc, STMT_beg))) {	// { ... }
		int newscope = !mode;
		astn blk;

		if (newscope) {
			enter(cc, node);
		}

		if ((blk = block(cc, 0)) != NULL) {
			node->stmt.stmt = blk;
			node->type = cc->type_vid;
			node->cst2 = qual;
			qual = TYPE_any;
		}
		else {
			// eat code like: {{;{;};{}{}}}
			eatnode(cc, node);
			node = 0;
		}

		if (newscope) {
			leave(cc, NULL, 0);
		}

		skiptok(cc, STMT_end, 1);
	}
	else if ((node = next(cc, STMT_if ))) {	// if (...)
		int siffalse = cc->siff;
		int newscope = 1;

		skiptok(cc, PNCT_lp, 1);
		node->stmt.test = expr(cc, TYPE_bit);
		skiptok(cc, PNCT_rp, 1);

		// static if (true) does not need entering a new scope.
		// every declaration should be available outside if.
		if (qual == ATTR_stat) {
			struct astNode value;
			if (!eval(&value, node->stmt.test) || !peekTok(cc, STMT_beg)) {
				error(cc->s, node->file, node->line, "invalid static if construct");
				trace("%+k", peekTok(cc, 0));
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
					warn(cc->s, 4, cc->file, cc->line, WARN_USE_BLOCK_STATEMENT, node->stmt.stmt);
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
		if (skip(cc, STMT_els)) {		// optionally parse else part
			if (newscope) {
				leave(cc, NULL, 0);
				enter(cc, node);
			}
			node->stmt.step = stmt(cc, 1);
			if (node->stmt.step != NULL) {
				switch (node->stmt.step->kind) {
					default:
						warn(cc->s, 4, cc->file, cc->line, WARN_USE_BLOCK_STATEMENT, node->stmt.step);
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
		node->cst2 = qual;
		qual = TYPE_any;

		cc->siff = siffalse;
	}
	else if ((node = next(cc, STMT_for))) {	// for (...)

		enter(cc, node);
		skiptok(cc, PNCT_lp, 1);
		if (!skip(cc, STMT_do)) {		// init or iterate
			node->stmt.init = decl(cc, decl_NoDefs|decl_ItDecl);

			if (node->stmt.init == NULL) {
				node->stmt.init = expr(cc, TYPE_vid);
				skiptok(cc, STMT_do, 1);
			}
			else if (skip(cc, PNCT_cln)) {		// iterate
				//~ for (Type value : iterable) {...}
				astn iton = expr(cc, TYPE_vid); // iterate on expression

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
							symn x = declare(cc, TYPE_ref, itTmp, sym->type);
							x->init = itInit;
							sym = x;

							// make for statement initializer a block having 2 declarations.
							node->stmt.init = newnode(cc, STMT_beg);
							node->stmt.init->type = cc->type_vid;
							node->stmt.init->cst2 = TYPE_rec;

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

					if (itStep == NULL || !typecheck(cc, NULL, sym->init)) {
						itStep = opnode(cc, OPER_fnc, tagnode(cc, "next"), NULL);
						error(cc->s, node->file, node->line, "iterator not defined for `%T`", iton->type);
						info(cc->s, node->file, node->line, "a function `%k(%T)` should be declared", itCtor, iton->type);
					}
					else if (typecheck(cc, NULL, itStep) != cc->type_bol) {
						error(cc->s, node->file, node->line, "iterator not found for `%+k`: %+k", iton, itStep);
						if (itStep->op.rhso->kind == OPER_com) {
							info(cc->s, node->file, node->line, "a function `%k(%T &, %T): %T` should be declared", itStep->op.lhso, itStep->op.rhso->op.lhso->type, itStep->op.rhso->op.rhso->type, cc->type_bol);
						}
						else {
							info(cc->s, node->file, node->line, "a function `%k(%T &): %T` should be declared", itStep->op.lhso, itStep->op.rhso->type, cc->type_bol);
						}
					}
					else {
						symn itFunNext = linkOf(itStep);
						if (itFunNext != NULL && itFunNext->prms != NULL) {
							if (itFunNext->prms->cast != TYPE_ref) {
								error(cc->s, itFunNext->file, itFunNext->line, "iterator arguments are not by ref");
							}
							if (itFunNext->prms->next && (itFunNext->prms->next->cast != TYPE_ref && itFunNext->prms->next->cast != TYPE_arr)) {
								error(cc->s, itFunNext->file, itFunNext->line, "iterator arguments are not by ref");
							}
						}
					}

					itStep->cst2 = TYPE_bit;
					node->stmt.test = itStep;
					sym->init->cst2 = TYPE_rec;
				}
				if (qual == ATTR_stat) {
					error(cc->s, node->file, node->line, "invalid use of static iteration");
				}
				backTok(cc, newnode(cc, STMT_do));
			}
		}
		if (!skip(cc, STMT_do)) {		// test
			node->stmt.test = expr(cc, TYPE_bit);
			skiptok(cc, STMT_do, 1);
		}
		if (!skip(cc, PNCT_rp)) {		// incr
			node->stmt.step = expr(cc, TYPE_vid);
			skiptok(cc, PNCT_rp, 1);
		}
		node->stmt.stmt = stmt(cc, 1);	// no new scope & disable decl
		if (node->stmt.stmt != NULL) {	// suggest using `for (...) {...}`
			if (node->stmt.stmt->kind != STMT_beg) {
				warn(cc->s, 4, cc->file, cc->line, WARN_USE_BLOCK_STATEMENT, node->stmt.stmt);
				node->stmt.stmt = blockNode(cc, node->stmt.stmt);
			}
		}
		leave(cc, NULL, 0);

		node->type = cc->type_vid;
		node->cst2 = qual;
		qual = TYPE_any;
	}
	else if ((node = next(cc, STMT_brk))) {	// break;
		node->type = cc->type_vid;
		skiptok(cc, STMT_do, 1);
	}
	else if ((node = next(cc, STMT_con))) {	// continue;
		node->type = cc->type_vid;
		skiptok(cc, STMT_do, 1);
	}
	else if ((node = next(cc, STMT_ret))) {	// return;
		node->type = cc->type_vid;
		if (cc->func != NULL) {
			symn result = cc->func->flds;
			if (!peekTok(cc, STMT_do)) {
				astn val = expr(cc, TYPE_vid);		// do lookup
				if (val == NULL) {/* error */ }
				else if (val->kind == TYPE_ref && val->ref.link == result) {
					// skip 'return result;' statements
				}
				else if (result->type == cc->type_vid && val->type == result->type) {
					// return void expression from a function returning void.
					node->stmt.stmt = val;
				}
				else {
					node->stmt.stmt = opnode(cc, ASGN_set, lnknode(cc, result), val);
					node->stmt.stmt->type = val->type;
					if (!typecheck(cc, NULL, node->stmt.stmt)) {
						error(cc->s, val->file, val->line, "invalid return value: `%+k`", val);
					}
				}
			}
			else if (result->type != cc->type_vid) {
				warn(cc->s, 4, cc->file, cc->line, "returning from function with no value");
			}
		}
		skiptok(cc, STMT_do, 1);
	}
	else if ((node = decl(cc, TYPE_any))) {	// declaration
		if (mode) {
			error(cc->s, node->file, node->line, "unexpected declaration `%+k`", node);
		}
	}
	else if ((node = expr(cc, TYPE_vid))) {	// expression
		skiptok(cc, STMT_do, 1);
		switch (node->kind) {
			default:
				warn(cc->s, 1, node->file, node->line, WARN_INVALID_EXPRESSION_STATEMENT, node);
				break;

			case OPER_fnc:
			case ASGN_set:
				break;
		}
	}
	else if ((node = peekTok(cc, TYPE_any))) {
		error(cc->s, node->file, node->line, "unexpected token: `%k`", node);
		skiptok(cc, STMT_do, 1);
	}
	else {
		error(cc->s, cc->file, cc->line, "unexpected end of file");
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
static astn parse(ccState cc, int asUnit, int warn) {
	astn root, oldRoot = cc->root;
	int level = cc->nest;

	cc->warn = warn;

	if (0) {
		astn ast, prev = NULL;
		while ((ast = next(cc, TYPE_any))) {
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
			unit->cst2 = TYPE_vid;
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
			error(cc->s, cc->file, cc->line, "unexpected token: %k", peekTok(cc, TYPE_any));
			return 0;
		}

		if (cc->nest != level) {
			error(cc->s, cc->file, cc->line, "premature end of file: %d", cc->nest);
			return NULL;
		}
	}

	return root;
}

/// @see: header
ccState ccOpen(state rt, char* file, int line, char* text) {
	ccState cc = rt->cc;

	if (cc == NULL) {
		// initilaize only once.
		cc = ccInit(rt, creg_def, NULL);
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

/// Compile file or text; @see state.api.ccAddCode
int ccAddCode(state rt, int warn, char* file, int line, char* text) {
	ccState cc = ccOpen(rt, file, line, text);
	if (cc == NULL) {
		error(rt, NULL, 0, "can not open: %s", file);
		return 0;
	}
	parse(cc, 0, warn);
	return ccDone(cc) == 0;
}

/// @see: header
int ccAddUnit(state rt, int unit(state), int warn, char *file) {
	int unitCode = unit(rt);
	if (unitCode == 0 && file != NULL) {
		return ccAddCode(rt, warn, file, 1, NULL);
	}
	return unitCode == 0;
}

/// @see: header
int ccDone(ccState cc) {
	astn ast;

	// not initialized
	if (cc == NULL)
		return -1;

	// check no token left to read
	if ((ast = peekTok(cc, TYPE_any))) {
		error(cc->s, ast->file, ast->line, "unexpected: `%k`", ast);
		return -1;
	}

	// check we are on the same nesting level.
	if (cc->fin.nest != cc->nest) {
		error(cc->s, cc->file, cc->line, "unexpected: end of input: %s", cc->fin._buf);
		return -1;
	}

	// close input
	source(cc, 0, NULL);

	// return errors
	return cc->s->errc;
}
