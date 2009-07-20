// { #include "scan.c"
#include "main.h"
#include <string.h>
//~ scan.c - Parser ------------------------------------------------------------

node scan(state s, int mode);
node stmt(state s, int mode);
node decl(state s, int mode);
node expr(state s, int mode);

node scan(state s, int mode) {
	node tok, lst = 0, ast = 0;

	#if 0		// read ahead all tokens or not
	while (read_tok(s, tok = newnode(s, 0))) {
		if (!ast) s->_tok = tok;
		else ast->next = tok;
		ast = tok;
	}
	//~ if (s->errc) return 0;
	ast = 0;
	#endif

	trace(+32, "enter:scan(%?k)", peek(s));

	//~ enter(s, newNode(s, FILE_new));
	enter(s, 0);
	while (peek(s)) {
		if ((tok = stmt(s, 0))) {
			if (!ast) ast = lst = tok;
			else lst = lst->next = tok;
			if (mode && tok->kind != OPER_nop)
				error(s, ast->line, "declaration expected");
		}
	}
	s->glob = leave(s);

	if (s->nest)
		error(s, s->line, "premature end of file");
	trace(-32, "leave:scan('%k')", peek(s));

	if (ast) {
		node tmp = newnode(s, OPER_beg);
		//~ tmp->name = filename;
		tmp->rhso = ast;
		tmp->lhso = lst;
		ast = tmp;
	}

	return ast;
}

node stmt(state s, int mode) {
	node ast, tmp = peek(s);
	int qual = 0;// qual(s, mode);			// static | const | parallel
	trace(+16, "enter:stmt('%k')", peek(s));

	if (skip(s, OPER_end)) return 0;			// ;

	if (skip(s, OPER_nop)) ast = 0;			// ;
	else if ((ast = next(s, OPER_beg))) {	// {...}
		node end = 0;
		//~ ast->cast = qual;
		//~ ast->cast = TYPE_vid;
		enter(s, 0);
		while (peek(s) && !skip(s, OPER_end)) {
			if (!peek(s)) break;
			if ((tmp = stmt(s, 0))) {
				if (!end) ast->stmt = tmp;
				else end->next = tmp;
				end = tmp;
			}
		}
		ast->type = leave(s);
		if (!ast->stmt) {			// eat sort of {{;{;};{}{}}}
			eatnode(s, ast);
			ast = 0;
		}// */
	}
	else if ((ast = next(s, OPER_jmp))) {	// if (...)
		enter(s, 0);
		ast->cast = qual;
		//~ ast->cast = TYPE_vid;
		// TODO parallel if -> error;
		if (skip(s, PNCT_lp)) {
			ast->test = expr(s, 1);
				//~ ast->test->cast = TYPE_bit;
			//~ if (ast->test) ast->test->cast = TYPE_bit;
			if (skip(s, PNCT_rp)) {
				ast->stmt = stmt(s, 1);
				if (skip(s, OPER_els)) {
					ast->step = stmt(s, 1);
				}
				else ast->step = NULL;
			}
			else error(s, s->line, " ')' expected, not %k", peek(s));
		}
		else error(s, s->line, " '(' expected");
		ast->type = leave(s);
	}
	else if ((ast = next(s, OPER_for))) {	// for (...)
		enter(s, 0);
		//~ ast->cast = qual;
		//~ ast->cast = TYPE_vid;
		ast->init = 0;
		ast->test = 0;
		ast->step = 0;
		if (!skip(s, PNCT_lp)) {
			error(s, s->line, " '(' expected");
		}
		if (!skip(s, OPER_nop)) {		// init
			ast->init = decl(s, 0);
			if (!ast->init)
				ast->init = expr(s, 0);
			if (!skip(s, OPER_nop))
				error(s, s->line, " ';' expected");
		}
		if (!skip(s, OPER_nop)) {		// test 
			ast->test = expr(s, 1);
			if (!skip(s, OPER_nop)) {
				error(s, s->line, " ';' expected");
			}
		}
		if (!skip(s, PNCT_rp)) {		// incr
			ast->step = expr(s, 1);
			if (!skip(s, PNCT_rp)) {
				error(s, s->line, " ')' expected");
			}
		}
		ast->stmt = stmt(s, 1);
		ast->type = leave(s);
	}
	else if ((ast = decl(s, qual))) {		// type?
		node tmp = newnode(s, OPER_nop);
		tmp->line = ast->line;
		tmp->stmt = ast;
		ast = tmp;
		//~ ast->cast = qual;
		//~ ast->cast = TYPE_vid;

		if (ast->type && mode)
			error(s, s->line, "unexpected declaration");
	}
	else if ((ast = expr(s, 0))) {			// expr;
		node tmp = newnode(s, OPER_nop);
		tmp->line = ast->line;
		tmp->stmt = ast;
		ast = tmp;
		//~ ast->cast = qual;
		//~ ast->cast = TYPE_vid;

		if (!skip(s, OPER_nop)) {
			error(s, s->line, " ';' expected, not `%k`", peek(s));
			while (peek(s) && !skip(s, OPER_nop)) {
				if (skip(s, OPER_end)) return 0;
				skip(s, 0);
			}
		}
	}
	else {
		fatal(s, __FILE__, __LINE__, "00dfskj-ssx");
	}

	trace(-16, "leave:stmt('%+k')", ast);
	//~ if (ast) debug("%+k", ast->stmt);
	return ast;
}

defn spec(state s, int qual);
defn type(state s, int qual);
node dvar(state s, defn typ, int qual);

node decl(state s, int qual) {
	defn typ;
	if ((typ = spec(s, qual))) {
		node ast = newnode(s, TYPE_def);
		ast->type = typ;
		ast->name = typ->name;
		ast->cast = TYPE_def;
		//~ debug("%+T", typ);
		//~ debug("%+k", ast);
		return ast;
	}
	if ((typ = type(s, qual))) {
		node ast = dvar(s, typ, qual);
		if (ast)
			ast->type = typ;
		//~ debug("%+k", ast);
		return ast;
	}
	return NULL;
}

//~ decl_typ
//~ decl_var
//~ decl_fnc

node expr(state s, int mode) {
	node	buff[TOKS], *base = buff + TOKS;
	node	*post = buff, *prec = base, tok;
	char	sym[TOKS];							// symbol stack {, [, (, ?
	int	level = 0, unary = 1;						// precedence, top of sym , start in unary mode
	sym[level] = 0;

	trace(+2, "enter:expr('%k')", peek(s));
	while ((tok = next(s, 0))) {						// parse
		int pri = level << 4;
		switch (tok->kind) {
			default : {
				if (tok_tbl[tok->kind].argc) {			// operator
					if (unary) {
						*post++ = 0;
						error(s, tok->line, "syntax error before '%k'", tok);
					}
					unary = 1;
					goto tok_op;
				} else {
					if (!unary)
						error(s, tok->line, "syntax error before '%k'", tok);
					unary = 0;
					goto tok_id;
				}
			} break;
			case PNCT_lp  : {			// '('
				if (unary)			// a + (3*b)
					*post++ = 0;
				else if (skip(s, PNCT_rp)) {	// a()
					*post++ = 0;
					//~ tok->kind = OPER_fnc;
					//~ goto tok_op;
				}
				//~ else			// a(...)
				tok->kind = OPER_fnc;
				sym[++level] = '(';
				unary = 1;
			} goto tok_op;
			case PNCT_rp  : {			// ')'
				if (!unary && sym[level] == '(') {
					tok->kind = OPER_nop;
					level -= 1;
				}
				else if (level == 0) {
					back(s, tok);
					tok = 0;
					break;
				}
				else {
					error(s, tok->line, "syntax error before ')' token");
					break;
				}
				unary = 0;
			} goto tok_op;
			case PNCT_lc  : {			// '['
				if (!unary)
					tok->kind = OPER_idx;
				else error(s, tok->line, "syntax error before '[' token");
				sym[++level] = '[';
				unary = 1;
			} goto tok_op;
			case PNCT_rc  : {			// ']'
				if (!unary && sym[level] == '[') {
					tok->kind = OPER_nop;
					level -= 1;
				}
				else if (!level) {
					back(s, tok);
					tok = 0;
					break;
				}
				else {
					error(s, tok->line, "syntax error before ']' token");
					break;
				}
				unary = 0;
			} goto tok_op;
			case PNCT_qst : {			// '?'
				if (unary) {
					*post++ = 0;		// ???
					error(s, tok->line, "syntax error before '?' token");
				}
				tok->kind = OPER_sel;
				sym[++level] = '?';
				unary = 1;
			} goto tok_op;
			case PNCT_cln : {			// ':'
				if (!unary && sym[level] == '?') {
					tok->kind = OPER_nop;
					level -= 1;
				}
				else if (level == 0) {
					back(s, tok);
					tok = 0;
					break;
				}
				else {
					error(s, tok->line, "syntax error before ':' token");
					break;
				}
				unary = 1;
			} goto tok_op;
			case OPER_pls : {			// '+'
				if (!unary)
					tok->kind = OPER_add;
				unary = 1;
			} goto tok_op;
			case OPER_mns : {			// '-'
				if (!unary)
					tok->kind = OPER_sub;
				unary = 1;
			} goto tok_op;
			case OPER_not : {			// '!'
				if (!unary)
					error(s, tok->line, "syntax error before '!' token");
				unary = 1;
			} goto tok_op;
			case OPER_cmt : {			// '~'
				if (!unary)
					error(s, tok->line, "syntax error before '~' token");
				unary = 1;
			} goto tok_op;

			case OPER_com : 			// ','
				if (level || mode != TYPE_ref) {
					if (unary)
						error(s, tok->line, "syntax error before ',' token");
					unary = 1;
					goto tok_op;
				}
				// else fall
			case OPER_jmp : 			// 'if'
			case OPER_for : 			// 'for'
			case OPER_els : 			// 'else'
			case OPER_beg : 			// '{'
			case OPER_end : 			// '}'
			case OPER_nop : 			// ';'
				back(s, tok);
				tok = 0;
				break;
			tok_op: {
				int oppri = tok_tbl[tok->kind].type;
				tok->pril = pri | (oppri & 0x0f);
				if (oppri & 0x10) while (prec < buff + TOKS) {
					if ((*prec)->pril <= tok->pril)
						break;
					*post++ = *prec++;
				}
				else while (prec < buff + TOKS) {
					if ((*prec)->pril < tok->pril)
						break;
					*post++ = *prec++;
				}
				if (tok->kind != OPER_nop) {
					*--prec = tok;
				}
			} break;
			tok_id: {
				*post++ = tok;
			} break;
		}
		if (!tok) break;
		if (post >= prec) {
			error(s, s->line, "Expression too complex");
			return 0;
		}
	}
	if (unary || level) {							// error
		char c = level ? sym[level] : 0;
		error(s, s->line, "missing %s", c == '(' ? "')'" : c == '[' ? "']'" : c == '?' ? "':'" : "expression");
		//~ return 0;
	}
	else if (prec > buff) {							// build
		node *ptr, *lhs;
		while (prec < buff + TOKS)					// flush
			*post++ = *prec++;
		for (lhs = ptr = buff; ptr < post; ptr += 1) {
			if ((tok = *ptr) == NULL) ;				// skip
			else if (tok_tbl[tok->kind].argc) {		// oper
				int argc = tok_tbl[tok->kind].argc;
				if ((lhs -= argc) < buff) {
					fatal(s, "expr(<overflow>) %k", tok);
					return 0;
				}
				switch (argc) {
					case 1 : {
						tok->test = 0;
						tok->lhso = 0;
						tok->rhso = lhs[0];
					} break;
					case 2 : {
						tok->test = 0;
						tok->lhso = lhs[0];
						tok->rhso = lhs[1];
					} break;
					case 3 : {
						tok->test = lhs[0];
						tok->lhso = lhs[1];
						tok->rhso = lhs[2];
					} break;
					default:
						debug("expr(<>)");
				}
				#if TODO_REM_TEMP // TODO : remove replacer
				switch (tok->kind) {				// temporary only cgen
					case ASGN_add : {
						node tmp = newnode(s, OPER_add);
						tmp->rhso = tok->rhso;
						tmp->lhso = tok->lhso;
						tok->kind = ASGN_set;
						tok->rhso = tmp;
						//~ tmp->next = tok;
						//~ tok->next = tmp;
					} break;
					case ASGN_sub : {
						node tmp = newnode(s, OPER_sub);
						tmp->rhso = tok->rhso;
						tmp->lhso = tok->lhso;
						tok->kind = ASGN_set;
						tok->rhso = tmp;
					} break;
					case ASGN_mul : {
						node tmp = newnode(s, OPER_mul);
						tmp->rhso = tok->rhso;
						tmp->lhso = tok->lhso;
						tok->kind = ASGN_set;
						tok->rhso = tmp;
					} break;
					case ASGN_div : {
						node tmp = newnode(s, OPER_div);
						tmp->rhso = tok->rhso;
						tmp->lhso = tok->lhso;
						tok->kind = ASGN_set;
						tok->rhso = tmp;
					} break;
					case ASGN_mod : {
						node tmp = newnode(s, OPER_mod);
						tmp->rhso = tok->rhso;
						tmp->lhso = tok->lhso;
						tok->kind = ASGN_set;
						tok->rhso = tmp;
					} break;
					case ASGN_shl : {
						node tmp = newnode(s, OPER_shl);
						tmp->rhso = tok->rhso;
						tmp->lhso = tok->lhso;
						tok->kind = ASGN_set;
						tok->rhso = tmp;
					} break;
					case ASGN_shr : {
						node tmp = newnode(s, OPER_shr);
						tmp->rhso = tok->rhso;
						tmp->lhso = tok->lhso;
						tok->kind = ASGN_set;
						tok->rhso = tmp;
					} break;
					case ASGN_ior : {
						node tmp = newnode(s, OPER_ior);
						tmp->rhso = tok->rhso;
						tmp->lhso = tok->lhso;
						tok->kind = ASGN_set;
						tok->rhso = tmp;
					} break;
				}
				#endif
			}
			/*/ find duplicate sub-trees				// ????
			for (dup = buff; dup < lhs; dup += 1) {
				if (samenode(*dup, tok)) {
					print(0, INFO, s->file, tok->line, "+dupp: %+k", *dup);
					break;
				}
				//~ if (tok) print(0, 0, s->file, tok->line, "+dupp: %+k", *dup);
			} // */
			//~ tok->init = 0;
			*lhs++ = tok;
		}
	}
	if (tok && !lookup(s, 0, tok))
		error(s, tok->line, "bum %+k", tok);
	trace(-2, "leave:expr('%+k') %k", tok, peek(s));
	return tok;
}

extern int align(int offs, int pack, int size);

node dvar(state s, defn typ, int qual) {
	//~ node prev = 0, root = 0;
	node tag = 0;
	defn ref = 0;

	trace(+4, "enter:dclr('%k')", peek(s));

	if (!(tag = next(s, TYPE_ref))) {
		debug("id expected, not %k", peek(s));
		return 0;
	}

	ref = declare(s, TYPE_ref, tag, typ, 0);

	if (skip(s, ASGN_set))
		ref->init = expr(s, TYPE_ref);

	if (test(s, OPER_nop)) {
		tag->kind = TYPE_def;
		tag->cast = TYPE_ref;
		tag->line = ref->line;
		tag->link = ref;
		return tag;
	}

	if (test(s, PNCT_lp)) {		// function
		fatal(s, "function");
	}
	else if (test(s, PNCT_lc)) {	// array
		fatal(s, "array");
	}
	else if (skip(s, OPER_com)) {	// ,
		fatal(s, "multi");
	}
	else
		error(s, s->line, "unexpected %k");
	error(s, s->line, "unexpected %k");

	trace(-4, "leave:dclr('%-k')", root);
	debug("dclr('%+k')", tag);
	return tag;
}// */

defn type(state s, int qual) {
	node tok = 0;
	defn tmp = 0, def = 0;
	tmp = 0;
	while ((tok = peek(s))) {		// type(.type)*
		if (!lookup(s, tmp, tok)) break;
		if (!istype(tok)) break;
		next(s, 0);
		def = tok->type;
		if (!skip(s, OPER_dot)) break;
		tmp = def;
		//~ def = 0;
	}
	/*if (def) {
		while (skip(s, PNCT_lc)) {			// array	: int [20][30] | int[100](int)
			defn tmp = newdefn(s, TYPE_arr);
			if (!skip(s, PNCT_rc)) {
				tmp->init = expr(s, CNST_int);
				if (!skip(s, PNCT_rc)) {
					error(s, s->line, "missing ']'");
					return tmp;
				}
			}
			tmp->type = def;
			def = tmp;
		}// * /
		if (skip(s, PNCT_lp)) {				// function
			defn tmp = newdefn(s, TYPE_fnc);
			enter(s, 0);
			args(s, fl_type+fl_name);
			leave(s, 1, &tmp->args);
			tmp->type = def;
			def = tmp;
			//declare(s, TYPE_def, newnode(s, TYPE_ref), def, argv);
		}// * /
	}// */
	return def;
}

defn spec(state s, int qual) {
	node tok, tag = 0;
	defn tmp, def = 0;
	int offs = 0;
	int size = 0;
	int pack = 4;

	trace(+8, "enter:spec('%k')", peek(s));
	if (skip(s, TYPE_def)) {		// define
		//~ define name type			// typedef
		//~ define name = expr			// macro
		// errif(qual != 0);

		qual = 0;
		if ((tag = next(s, TYPE_ref))) {
			if (skip(s, ASGN_set)) {	// define PI = 3.14;
				node val = expr(s, 0);
				if (eval(NULL, val, 0)) {
					def = declare(s, TYPE_def, tag, val->type, 0);
					def->init = val;
				}
				else {
					error(s, val->line, "Constant expression expected", peek(s));
					def = declare(s, TYPE_def, tag, 0, 0);
					def->type = NULL;
					def->init = val;
				}
				trace(1, "define('%T' as '%k')", def, val);
			}
			else if ((def = type(s, qual))) {	// define sin math.sin;	???
				def = declare(s, TYPE_def, tag, def, 0);
			}
			else {
				error(s, tag->line, "typename excepted");
			}
			trace(1, "define('%T' as '%T')", def, def);
			if (!skip(s, OPER_nop)) {
				error(s, s->line, "unexcepted token '%k'", peek(s));
				if (!peek(s)) return 0;
				while (peek(s)) {
					if (skip(s, OPER_nop)) return 0;
					if (skip(s, OPER_end)) return 0;
					skip(s, 0);
				}
			}
		}
		else error(s, s->line, "Identifyer expected");
	}
	else if (skip(s, TYPE_rec)) {		// struct
		if (!(tag = next(s, TYPE_ref))) {	// name?
			tag = newnode(s, TYPE_ref);
			tag->line = s->line;
		}
		/*if (skip(s, PNCT_cln)) {			// pack?
			if ((tok = next(s, CNST_int))) {
				switch ((int)tok->cint) {
					default : 
						error(s, s->line, "alignment must be a small power of two, not %d", tok->cint);
						break;
					case  0 : pack =  0; break;
					case  1 : pack =  1; break;
					case  2 : pack =  2; break;
					case  4 : pack =  4; break;
					case  8 : pack =  8; break;
					case 16 : pack = 16; break;
				}
			}
		}// */
		if (skip(s, OPER_beg)) {			// body
			def = declare(s, TYPE_rec, tag, 0, 0);
			enter(s, def);
			//~ while (skipToken(s, 'inherit'));
			while (!skip(s, OPER_end)) {
				//~ int ql = qual(s, 0);
				defn typ = spec(s, qual);
				if (!typ) typ = type(s, qual);
				if (typ && (tok = next(s, TYPE_ref))) {
					offs = align(offs, pack, typ->size);
					tmp = declare(s, TYPE_ref, tok, typ, 0);
					tmp->size = offs;
					offs += typ->size;
					if (size < offs) size = offs;
				}
				if (!skip(s, OPER_nop)) {
					error(s, s->line, "unexcepted token '%k'", peek(s));
					while ((tok = peek(s))) {
						if (skip(s, OPER_nop)) break;
						if (skip(s, OPER_end)) break;
						skip(s, 0);
					}
				}
			}
			def->size = size;
			def->args = leave(s);
			if (!def->args)
				warn(s, 2, s->file, s->line, "empty declaration");
		}
	}
	//else if (skip(s, TYPE_cls));		// class
	else if (skip(s, TYPE_enu)) {		// enum
		if (!(tag = next(s, TYPE_ref))) {	// name?
			tag = newnode(s, TYPE_ref);
			tag->line = s->line;
		}
		if (tag && skip(s, OPER_beg)) {		// body
			if (tag->name) def = declare(s, TYPE_enu, tag, 0, 0);
			if (tag->name) enter(s, def);
			while (!skip(s, OPER_end)) {
				if ((tok = next(s, TYPE_ref))) {
					tmp = declare(s, CNST_int, tok, type_i32, 0);
					if (skip(s, ASGN_set)) {
						tmp->init = expr(s, CNST_int);
						if (eval(NULL, tmp->init, TYPE_int) != CNST_int)
							error(s, s->line, "integer constant expected");
						//~ if (iscons(tmp->init) != CNST_int)
							//~ error(s, s->line, "integer constant expected");
					}
					else {
						tmp->init = newnode(s, CNST_int);
						tmp->init->type = type_i32;
						tmp->init->cint = offs;
						offs += 1;
					}
				}
				/*else {
					error(s, s->line, "identifyer expected");
					return 0;
				}*/
				if (!skip(s, OPER_nop)) {
					// TODO error
					if (!(tok = peek(s))) return 0;
					error(s, tok->line, "unexcepted token '%k'", tok);
					while (!skip(s, OPER_nop)) {
						if (skip(s, OPER_end)) break;//return 0;
						skip(s, 0);
					}
				}
			}
			if (tag->name)
				def->args = leave(s);
		}
	}
	else {					// type
	}
	trace(-8, "leave:spec('%+T')", def);
	return def;
}

void enter(state s, defn def) {
	s->nest += 1;
	s->scope[s->nest].csym = def;
	//~ s->scope[s->nest].stmt = NULL;
	//~ return &s->scope[s->nest];
}

defn leave(state s) {
	int i;
	defn arg = 0;
	s->nest -= 1;

	// clear from table
	for (i = 0; i < TBLS; i++) {
		defn def = s->deft[i];
		while (def && def->nest > s->nest) {
			defn tmp = def;
			def = def->next;
			tmp->next = 0;
		}
		s->deft[i] = def;
	}

	// clear from stack
	while (s->defs) {
		defn sym = s->defs;

		if (sym->nest <= s->nest) break;
		s->defs = sym->defs;
		sym->next = arg;
		arg = sym;
	}

	return arg;
}

/*int args(state s, int mode) {
	int argc = 0;
	while (peek(s) && !skip(s, PNCT_rp)) {			// args
		int qual = 0;
		defn arg = 0, typ = type(s, qual);		// type
		node val = 0, tag = next(s, TYPE_ref);		// name
		if (skip(s, ASGN_set))
			val = expr(s, TYPE_ref);		// init (const)
		if (!tag)
			tag = newnode(s, TYPE_ref);
		if (typ && tag) {
			//~ trace(0, "function(arg:%d : %T %k);", argc, typ, tag);
			arg = declare(s, TYPE_ref, tag, typ, 0);
			arg->init = val;
			//~ arg->used = 1;
		}
		//~ if (typ && !(mode & fl_type))
			//~ error(s, tag->line, "unexpected %T", typ);
		//~ if (tag && !(mode & fl_name))
			//~ error(s, tag->line, "unexpected %k", tag);
		//~ if (val && !(mode & fl_init))
			//~ error(s, tag->line, "unexpected %+k", val);

		if (!skip(s, OPER_com)) {
			if (test(s, PNCT_rp)) continue;
			//~ if (!tmp) error(s, "missing declarator", s->file, s->line);
			error(s, s->line, "',' expected");
		}
		if (!peek(s)) break;
		argc++;
	}
	return argc;
}*/

/*int Qual() {
	while ((tok = peek(s))) {		// qual : const/packed/volatile
		if (tok->kind == QUAL_con) {	// constant
			if (qual & QUAL_con) error(s, tok->line, "duplicate qualifier: %k", tok);
			qual |= QUAL_con;
			skip(s, 0);
			continue;
		}
		if (tok->kind == QUAL_vol) {	// volatile
			if (qual & QUAL_vol) error(s, tok->line, "duplicate qualifier: %k", tok);
			qual |= QUAL_vol;
			skip(s, 0);
			continue;
		}
		break;
	}
}// */

/** scan -----------------------------------------------------------------------
 * scan a source
 * @param mode : script mode
**/
/** stmt -----------------------------------------------------------------------
 * scan a statement (mode ? enable decl : do not)
 * @param mode : enable or not <decl>.
 *	ex : "for( ; ; ) int a;", if mode == 0 => error
 * stmt : ';'
 *	| expr ';'
 *	| decl ';'
 *	| '{' stmt* '}'
 *	| 'static'? 'if' (expr) stmt ('else' stmt)?
 *	| ('static' | 'parallel')? 'for' '(' (decl | expr)? ';' expr?';' expr? ')' stmt
+*	| 'return' expr? ';'
+*	| 'continue' ';'
+*	| 'break' ';'
+*	| ID ':'
+*	| 'sync' ('(' ID ')')? '{' stmt '}'		// synchtonized
+*	| 'task' ('(' ID ')')? '{' stmt '}'		// parallel task
?*	| 'fork'

>* static if (...)		// compile time test
>* static for (...)		// loop unroll
>* parallel for (...)		// parallel loop
**/
/** decl -----------------------------------------------------------------------
 * scan a declaration, add to ast tree
 * @param mode : enable or not <expr>.
 * decl :

 * decl_var
 * 	typename <idtf> (= <expr>)?, (<idtf> (= <expr>)?)? \;
 + array
 * 	typename <idtf>[]... (= <expr>)?, (<idtf> (= <expr>)?)? \;
 + decl_fnc
 * 	typename <idtf> \( <args> \) ({<stmt>})? \;
 * 	typename <oper> \( <args> \) ({<stmt>})? \;
 * template
 * property

 *	define ...
 - 	define '{'(name'('constant)')'*'}'		// enum
 * 	define <idtf> = <cons>;				// typed, named const
 * 	define <idtf> <type>;				// typedef
 ? 	define <idtf>(args...) {stmt}			// inline
 *	struct (\:<pack>)? (tag) '{'(<decl>)*'}'	// record

 *	enumerate ...
 *	enum <idtf>? {(<idtf> (= cintexpr)?;)+}		// enum
 !	enum <idtf>\(...\) {(<idtf> \(... \);)+}	// enum

 *	typename
 *	template

 * var	(static)? <type> name = expr [, name = expr]*
 * arr	 $ (static)? <type> name'['expr']' (= '('(expr[,expr]*)')'|expr)?
X* fnc	 $ (static)? <type> name'('type name (= expr)? [,type name (= expr)?]*')'
 *

//~ template(typename t1, typename t2) pair;
//~ pair.t1

//~ template(typename ElementT, int size) class stack {
//~ 	ElementT bp[size], *sp = bp;
//~ }

//~ enum token(int prec, int argc, str name) {
//~ 	TYPE_i32(0x1, 0, "integer");
//~ 	STMT_for(0x0, 0, "for");
//~ 	OPER_add(0xc, 2, "__add");
//~ 	ASGN_mul(0x2, 2, "*=");
//~ }
**/
/** expr -----------------------------------------------------------------------
 * scan an expression
 * if mode then exit on ','
 * expr
 * 	 : ID
 * 	 | expr '[' expr ']'		// \arr	array
?* 	 | '[' expr ']'			// \ptr	pointer
 * 	 | expr '(' expr? ')'		// \fnc	function
 * 	 | '(' expr ')'			// \pri	precedence
 * 	 | OP expr			// \uop	unary
 * 	 | expr OP expr			// \bop	binary
 * 	 | expr '?' expr ':' expr	// \top	ternary
 * 	 ;
**/
// }
