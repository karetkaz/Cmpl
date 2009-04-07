//{ #include "code.c"
#include "ccvm.h"
//~ scan.c - Parser ------------------------------------------------------------

//~ TODO: this realy sucks

node scan(state s, int mode);
node stmt(state s, int mode);
node decl(state s, int mode);
node expr(state s, int mode);
defn spec(state s, int *kind);
node dvar(state s, defn typ, int qual);
int args(state s, int mode);
enum {
	//~ fl_void = 0,
	fl_type = 1,
	fl_name = 2,
	fl_init = 4,
};

// TODO temp only

static void dumpxml(FILE *fout, node ast, int lev, char* text) {
	if (ast) switch (ast->kind) {
		//{ STMT
		case CODE_inf : {
			dumpxml(fout, ast->cinf.stmt, lev, text);
		} break;
		case OPER_nop : {
			//~ fputmsg(fout, "%I<STMT id='%d' expr='%?k'>\n", lev, ast->kind, ast);
			dumpxml(fout, ast->stmt, lev, "expr");
			//~ fputmsg(fout, "%I</STMT>\n", lev);
		} break;
		case OPER_beg : {
			node l = ast->stmt;
			fputmsg(fout, "%I<list id='%d' type='%?T'>\n", lev, ast->kind, ast->type, ast->type, ast->line, tok_tbl[ast->kind].name, ast);
			for (l = ast->stmt; l; l = l->next)
				dumpxml(fout, l, lev + 1, text);
			fputmsg(fout, "%I</list>\n", lev, ast->kind, ast->type, ast->type, ast->line, tok_tbl[ast->kind].name, ast);
		} break;
		case OPER_jmp : {
			fputmsg(fout, "%I<stmt id='%d' stmt='if (%?+k)'>\n", lev, ast->kind, ast->test);
			dumpxml(fout, ast->test, lev + 1, "test");
			dumpxml(fout, ast->stmt, lev + 1, "stmt");
			fputmsg(fout, "%I</stmt>\n", lev);
		} break;
		case OPER_for : {
			fputmsg(fout, "%I<stmt id='%d' stmt='for (%?+k; %?+k; %?+k)'>\n", lev, ast->kind, ast->init, ast->test, ast->step);
			dumpxml(fout, ast->init, lev + 1, "init");
			dumpxml(fout, ast->test, lev + 1, "test");
			dumpxml(fout, ast->step, lev + 1, "step");
			dumpxml(fout, ast->stmt, lev + 1, "stmt");
			fputmsg(fout, "%I</stmt>\n", lev);
		} break;
		//}

		//{ OPER
		case OPER_fnc : break;		// '()'
		case OPER_dot : 		// '.'
		case OPER_idx : {		// '[]'
		} //break;

		case OPER_pls : 		// '+'
		case OPER_mns : 		// '-'
		case OPER_cmt : 		// '~'
		case OPER_not : {		// '!'
		} //break;

		//~ case OPER_pow : 		// '**'
		case OPER_add : 		// '+'
		case OPER_sub : 		// '-'
		case OPER_mul : 		// '*'
		case OPER_div : 		// '/'
		case OPER_mod : {		// '%'
		} //break;

		case OPER_shl : 		// '>>'
		case OPER_shr : 		// '<<'
		case OPER_and : 		// '&'
		case OPER_ior : 		// '|'
		case OPER_xor : {		// '^'
		} //break;

		case OPER_equ : 		// '=='
		case OPER_neq : 		// '!='
		case OPER_lte : 		// '<'
		case OPER_leq : 		// '<='
		case OPER_gte : 		// '>'
		case OPER_geq : {		// '>='
		} //break;

		//~ case OPER_lnd : 		// '&&'
		//~ case OPER_lor : 		// '||'
		//~ case OPER_sel : 		// '?:'
		//~ case OPER_min : 		// '?<'
		//~ case OPER_max : 		// '?>'

		//~ case ASGN_pow : 		// '**='
		//~ case ASGN_add : 		// '+='
		//~ case ASGN_sub : 		// '-='
		//~ case ASGN_mul : 		// '*='
		//~ case ASGN_div : 		// '/='
		//~ case ASGN_mod : 		// '%='

		//~ case ASGN_shl : 		// '>>='
		//~ case ASGN_shr : 		// '<<='
		//~ case ASGN_and : 		// '&='
		//~ case ASGN_ior : 		// '|='
		//~ case ASGN_xor : 		// '^='

		case ASGN_set : {		// '='
			fputmsg(fout, "%I<%s id='%d' type='%?T' oper='%?k' expr='%?+k'>\n", lev, text, ast->kind, ast->type, ast, ast);
			dumpxml(fout, ast->lhso, lev + 1, "lval");
			dumpxml(fout, ast->rhso, lev + 1, "rval");
			fputmsg(fout, "%I</%s>\n", lev, text, ast->kind, ast->type, ast);
		} break;
		//}*/

		//{ TVAL
		case CNST_int : 
		case CNST_flt : 
		case CNST_str : {
			fputmsg(fout, "%I<cnst id='%d' type='%?T'>%k</cnst>\n", lev, ast->kind, ast->type, ast);
		} break;
		case TYPE_ref : {
			fputmsg(fout, "%I<%s id='%d' type='%?T'>%s</%s>\n", lev, text, ast->kind, ast->type, ast->name, text);
		} break;
		//}

		default: fatal(s, "Node(%k)", ast);
	}
	else ;
}

node scan(state s, int mode) {
	node tok, lst = 0, ast = 0;

	#if 0		// read ahead all tokens
	while (read_tok(s, tok = newnode(s, 0))) {
		if (!ast) s->_tok = tok;
		else ast->next = tok;
		ast = tok;
	}
	//~ if (s->errc) return 0;
	ast = 0;
	#endif

	trace(+32, "enter:scan(%?k)", peek(s));
	//~ enter(s, FILE_new, s->file);
	
	//~ enter(s, newNode(FILE_new));
	enter(s, 0);
	while (peek(s)) {
		if ((tok = stmt(s, 0))) {
			if (!ast) ast = lst = tok;
			else lst = lst->next = tok;
			if (mode && tok->kind != OPER_nop)
				error(s, s->file, ast->line, "declaration expected");
		}
	}
	leave(s, 3, &s->glob);

	if (s->nest)
		error(s, s->file, s->line, "premature end of file");
	trace(-32, "leave:scan('%k')", peek(s));

	if (ast) {
		node tmp = newnode(s, OPER_beg);
		//~ tmp->name = filename;
		tmp->stmt = ast;
		ast = tmp;
	}

	if (1) {
		FILE *f = fopen("out.xml", "wt");
		if (f != NULL) {
			dumpxml(f, ast, 0, "node");
			fclose(f);
		}
	}

	return ast;
}

node stmt(state s, int mode) {
	node ast, tmp = peek(s);
	int qual = 0;// qual(s, mode);
	trace(+16, "enter:stmt('%k')", peek(s));
	if (skip(s, OPER_nop)) ast = 0;			// ;
	else if ((ast = next(s, OPER_beg))) {		// {...}
		node beg = 0;
		ast->kind = OPER_beg;
		enter(s, 0);
		while (peek(s) && !skip(s, OPER_end)) {
			if (!peek(s)) break;
			if ((tmp = stmt(s, 0))) {
				if (!beg) ast->stmt = tmp;
				else beg->next = tmp;
				beg = tmp;
			}
		}
		if (!ast->stmt) {			// eat sort of {{{...}}}
			delnode(s, ast);
			ast = 0;
		}// */
		leave(s, 7, &ast->type);
	}
	else if ((ast = next(s, OPER_jmp))) {		// if (...)
		enter(s, 0);
		if (skip(s, PNCT_lp)) {
			ast->test = expr(s, 1);
			if (skip(s, PNCT_rp)) {
				ast->stmt = stmt(s, 1);
				if (skip(s, OPER_els)) {
					ast->step = stmt(s, 1);
				}
				else ast->step = NULL;
			}
			else error(s, s->file, s->line, " ')' expected");
		}
		else error(s, s->file, s->line, " '(' expected");
		leave(s, 7, &ast->type);
	}
	else if ((ast = next(s, OPER_for))) {		// for (...)
		enter(s, 0);
		if (!skip(s, PNCT_lp)) {
			error(s, s->file, s->line, " '(' expected");
		}
		if (!skip(s, OPER_nop)) {		// init
			int qual = 0;
			defn typ = spec(s, &qual);
			ast->init = typ ? dvar(s, typ, qual) : expr(s, 0);
			if (!skip(s, OPER_nop))
				error(s, s->file, s->line, " ';' expected");
		}
		else ast->init = 0;
		if (!skip(s, OPER_nop)) {		// test
			ast->test = expr(s, 1);
			if (!skip(s, OPER_nop)) {
				error(s, s->file, s->line, " ';' expected");
			}
		}
		else ast->test = 0;
		if (!skip(s, PNCT_rp)) {		// incr
			ast->step = expr(s, 1);
			if (!skip(s, PNCT_rp)) {
				error(s, s->file, s->line, " ')' expected");
			}
		}
		else ast->step = 0;
		ast->stmt = stmt(s, 1);
		leave(s, 7, &ast->type);
	}
	else if ((ast = peek(s))) {			// decl|expr
		/*if (lookup(s, ast, NULL)) {
			if (istype(ast)) {	// decl
			}
			else {			// expr
			}
		}
		else {
			next(s, 0);
			if (skip(s, PNCT_cln))	// :label
				install(s, ast);
		}// */
		ast = newnode(s, OPER_nop);
		ast->line = s->line;
		ast->type = spec(s, &qual);
		ast->stmt = dvar(s, ast->type, qual);
		if (!skip(s, OPER_nop)) {
			error(s, s->file, s->line, " ';' expected, not `%k`", peek(s));
			while (peek(s) && !skip(s, OPER_nop)) {
				if (skip(s, OPER_end)) return 0;
				skip(s, 0);
			}
		}
		if (ast->type && mode)
			error(s, s->file, s->line, "unexpected declaration");
#if 0		// print asm code for expr
		tmp = newnode(s, CODE_inf);
		tmp->cinf.stmt = ast;
		ast = tmp;
#endif
	}
	trace(-16, "leave:stmt('%+k')", ast);
	return ast;
}

node decl(state s, int mode) {
	//~ switch (mode)
		//~ case 0 : enable or not <expr>.
		//~ case 7 : native decl.
	return NULL;
}

defn type(state s, int mode) {
	node tok;
	defn def = 0;
	/*while ((tok = peek(s))) {		// qual : const/packed/volatile
		if (tok->kind == QUAL_con) {	// constant
			if (qual & QUAL_con) error(s, s->file, tok->line, "duplicate qualifier: %k", tok);
			qual |= QUAL_con;
			skip(s, 0);
			continue;
		}
		if (tok->kind == QUAL_vol) {	// volatile
			if (qual & QUAL_vol) error(s, s->file, tok->line, "duplicate qualifier: %k", tok);
			qual |= QUAL_vol;
			skip(s, 0);
			continue;
		}
		break;
	}*/
	while ((tok = peek(s))) {		// type(.type)*
		defn tmp = lookup(s, tok, 0);
		if (!tmp || !istype(tok)) break;
		next(s, 0);
		def = tmp;
		if (!skip(s, OPER_dot)) break;
	}
	if (def) {
		while (skip(s, PNCT_lc)) {			// array	: int [20][30] | int[100](int)
			defn tmp = newdefn(s, TYPE_arr);
			if (!skip(s, PNCT_rc)) {
				tmp->init = expr(s, CNST_int);
				if (!skip(s, PNCT_rc)) {
					error(s, s->file, s->line, "missing ']'");
					return tmp;
				}
			}
			tmp->type = def;
			def = tmp;
		}// */
		if (skip(s, PNCT_lp)) {				// function
			defn tmp = newdefn(s, TYPE_fnc);
			enter(s, 0);
			args(s, fl_type+fl_name);
			leave(s, 1, &tmp->args);
			tmp->type = def;
			def = tmp;
			//declare(s, TYPE_def, newnode(s, TYPE_ref), def, argv);
		}
		//~ else break;
	}
	//~ debug("typename = %T", def);
	return def;
}

node expr(state s, int mode) {
	node	buff[TOKS], *base = buff + TOKS;
	node	*post = buff, *prec = base, tok;
	char	sym[TOKS];							// symbol stack {, [, (, ?
	int	level = 0, unary = 1;						// precedence, top of sym , start in unary mode
	sym[level] = 0;

	trace(+2, "enter:expr('%k')", peek(s));
	while ((tok = peek(s))) {						// parse
		int pri = level << 4;
		switch (tok->kind) {
			default : {
				if (tok_tbl[tok->kind].argc) {			// operator
					if (unary) {
						*post++ = 0;
						error(s, s->file, tok->line, "syntax error before token '%k'", tok);
					}
					unary = 1;
					goto tok_op;
				} else {
					if (!unary)
						error(s, s->file, tok->line, "syntax error before token '%k'", tok);
					unary = 0;
					goto tok_ty;
				}
			} break;
			case PNCT_lp  : {			// '('
				if (unary)			// a + (3*b)
					*post++ = 0;
				else if (skip(s, PNCT_rp)) {	// a()
					*post++ = 0;
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
					tok = 0;
					break;
				}
				else {
					error(s, s->file, tok->line, "syntax error before ')' token");
					break;
				}
				unary = 0;
			} goto tok_op;
			case PNCT_lc  : {			// '['
				if (!unary)
					tok->kind = OPER_idx;
				else error(s, s->file, tok->line, "syntax error before '[' token");
				sym[++level] = '[';
				unary = 1;
			} goto tok_op;
			case PNCT_rc  : {			// ']'
				if (!unary && sym[level] == '[') {
					tok->kind = OPER_nop;
					level -= 1;
				}
				else if (!level) {
					tok = 0;
					break;
				}
				else {
					error(s, s->file, tok->line, "syntax error before ']' token");
					break;
				}
				unary = 0;
			} goto tok_op;
			case PNCT_qst : {			// '?'
				if (unary) {
					*post++ = 0;		// ???
					error(s, s->file, tok->line, "syntax error before '?' token");
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
					tok = 0;
					break;
				}
				else {
					error(s, s->file, tok->line, "syntax error before ':' token");
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
					error(s, s->file, tok->line, "syntax error before '!' token");
				unary = 1;
			} goto tok_op;
			case OPER_cmt : {			// '~'
				if (!unary)
					error(s, s->file, tok->line, "syntax error before '~' token");
				unary = 1;
			} goto tok_op;

			case OPER_com : 			// ','
				if (level || mode != TYPE_ref) {
					if (unary)
						error(s, s->file, tok->line, "syntax error before ',' token");
					unary = 1;
					goto tok_op;
				}
				// else fall through
			case OPER_jmp : 			// 'if'
			case OPER_for : 			// 'for'
			case OPER_els : 			// 'else'
			case OPER_beg : 			// '{'
			case OPER_end : 			// '}'
			case OPER_nop : 			// ';'
				tok = 0;			// done
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
			tok_ty: {
				*post++ = tok;
			} break;
		}
		if (!tok) break;
		if (post >= prec) {
			error(s, s->file, s->line, "Expression too complex");
			return 0;
		}
		next(s, 0);
	}
	if (unary || level) {							// error
		char c = level ? sym[level] : 0;
		error(s, s->file, s->line, "missing %s", c == '(' ? "')'" : c == '[' ? "']'" : c == '?' ? "':'" : "expression");
		//~ return 0;
	}
	else if (prec > buff) {							// build
		//~ defn typ = 0;
		node *ptr, *lhs;//, *dup;
		while (prec < buff + TOKS)					// flush
			*post++ = *prec++;
		for (lhs = ptr = buff; ptr < post; ptr += 1) {
			//~ fputmsg(stdout, "%k ", *ptr);
			if ((tok = *ptr) == NULL) ;				// skip
			else if (tok_tbl[tok->kind].argc) {			// oper
				int argc = tok_tbl[tok->kind].argc;
				if ((lhs -= argc) < buff) {
					// fatal
					debug("expr(<overflow>) %k", tok);
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
			}
			/*/ find duplicate sub-trees				// ????
			for (dup = buff; dup < lhs; dup += 1) {
				if (samenode(*dup, tok)) {
					print(0, INFO, s->file, tok->line, "+dupp: %+k", *dup);
					break;
				}
				//~ if (tok) print(0, 0, s->file, tok->line, "+dupp: %+k", *dup);
			} // */
			*lhs++ = tok;
		}
		//~ fputmsg(stdout, "\n");
	}
	lookup(s, tok, 0);
	trace(-2, "leave:expr('%+k')", tok);
	return tok;
}

int args(state s, int mode) {
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
			//~ error(s, s->file, tag->line, "unexpected %T", typ);
		//~ if (tag && !(mode & fl_name))
			//~ error(s, s->file, tag->line, "unexpected %k", tag);
		//~ if (val && !(mode & fl_init))
			//~ error(s, s->file, tag->line, "unexpected %+k", val);

		if (!skip(s, OPER_com)) {
			if (test(s, PNCT_rp)) continue;
			//~ if (!tmp) error(s, "missing declarator", s->file, s->line);
			error(s, s->file, s->line, "',' expected");
		}
		if (!peek(s)) break;
		argc++;
	}
	return argc;
}

defn spec(state s, int *kind) {
	node tok, tag = 0;
	defn tmp, def = 0;
	int qual = 0;			// Access Modifier / Qualifyer
	int offs = 0;
	int size = 0;
	int pack = 4;			// allignment

	trace(+8, "enter:spec('%k')", peek(s));
	//~ Storage Class	: auto register static extern typedef

	//~ if (skipToken(s, 'static'|'inline')) qual |= 'static';
	//~ if (skipToken(s, 'packed')) qual |= 'packed';
	//~ if (skipToken(s, 'public')) qual |= 'public';
	//~ if (skipToken(s, 'extern')) qual |= 'extern';

	if (skip(s, TYPE_def)) {			// define
		//~ define name type			// typedef
		//~ define name = expr			// macro

		if ((tag = next(s, TYPE_ref))) {
			if (skip(s, ASGN_set)) {	// cnstdef: define PI = 3.14;
				node val = expr(s, TYPE_def);
				//~ int constnode = iscons(val);
				int constnode = eval(NULL, val, TYPE_any);
				if (constnode) {
					//~ defn typ = lookup(s, 0, tag);
					def = declare(s, constnode, tag, 0, 0);
					def->type = val->type;
					def->init = val;
				}
				else {
					error(s, s->file, val->line, "Constant expression expected", peek(s));
					def = declare(s, TYPE_def, tag, 0, 0);
					def->type = NULL;
					def->init = val;
				}
				trace(1, "define('%T' as '%k')", def, val);
			}
			else {				// typedef: define u4 unsigned int;
				if ((def = type(s, qual)))
					def = declare(s, TYPE_def, tag, def, 0);
				else
					error(s, s->file, tag->line, "typename excepted");
				trace(1, "define('%T' as '%T')", def, def);
			}
			/*if (!skip(s, OPER_nop)) {
				print(s, -1, s->file, tok->line, "unexcepted token '%k'", peek(s));
				if (!peek(s)) return 0;
				while (!skip(s, OPER_nop)) {
					if (skip(s, OPER_end)) return 0;
					skip(s, 0);
				}
			}*/
		}
		else error(s, s->file, s->line, "Identifyer expected");
		*kind = TYPE_def;
	}
	else if (skip(s, TYPE_rec)) {			// struct
		if (!(tag = next(s, TYPE_ref))) {	// name?
			tag = newnode(s, TYPE_ref);
			tag->line = s->line;
		}
		if (skip(s, PNCT_cln)) {		// pack?
			if ((tok = next(s, CNST_int))) {
				switch ((int)tok->cint) {
					default : 
						error(s, s->file, s->line, "alignment must be a small power of two, not %d", tok->cint);
						break;
					case  0 : pack =  0; break;
					case  1 : pack =  1; break;
					case  2 : pack =  2; break;
					case  4 : pack =  4; break;
					case  8 : pack =  8; break;
					case 16 : pack = 16; break;
				}
			}
		}
		if (tag && skip(s, OPER_beg)) {		// body
			def = declare(s, TYPE_rec, tag, 0, 0);
			enter(s, def);
			//~ while (skipToken(s, 'inherit'));
			while (!skip(s, OPER_end)) {
				defn typ = spec(s, &qual);
				if (typ && (tok = next(s, TYPE_ref))) {
					offs = align(offs, pack, typ->size);
					tmp = declare(s, TYPE_ref, tok, typ, 0);
					tmp->offs = offs;
					offs += typ->size;
					if (size < offs) size = offs;
				}
				if (!skip(s, OPER_nop)) {
					// TODO error
					if (!(tok = peek(s))) return 0;
					error(s, s->file, tok->line, "unexcepted token '%k'", tok);
					while (!skip(s, OPER_nop)) {
						if (skip(s, OPER_end)) break;//return 0;
						skip(s, 0);
					}
				}
			}
			def->size = size;
			leave(s, 3, &def->args);		// ???
			if (!def->args) warn(s, 2, s->file, s->line, "empty declaration");
		}
		if (kind) *kind = TYPE_def;
	}
	//~ else if (skip(s, TYPE_cls)) ;			// class
	else if (skip(s, TYPE_enu)) {			// enum
		if (!(tag = next(s, TYPE_ref))) {	// name?
			tag = newnode(s, TYPE_ref);
			tag->line = s->line;
		}
		/*if (skip(s, PNCT_lp)) {		// args?
		}*/
		if (tag && skip(s, OPER_beg)) {		// body
			if (tag->name) def = declare(s, TYPE_enu, tag, 0, 0);
			if (tag->name) enter(s, def);
			while (!skip(s, OPER_end)) {
				if ((tok = next(s, TYPE_ref))) {
					tmp = declare(s, CNST_int, tok, type_i32, 0);
					if (skip(s, ASGN_set)) {
						tmp->init = expr(s, CNST_int);
						if (eval(NULL, tmp->init, TYPE_i32) != CNST_int)
							error(s, s->file, s->line, "integer constant expected");
						//~ if (iscons(tmp->init) != CNST_int)
							//~ error(s, s->file, s->line, "integer constant expected");
					}
					else {
						tmp->init = newnode(s, CNST_int);
						tmp->init->type = type_i32;
						tmp->init->cint = offs;
						offs += 1;
					}
				}
				/*else {
					error(s, s->file, s->line, "identifyer expected");
					return 0;
				}*/
				if (!skip(s, OPER_nop)) {
					// TODO error
					if (!(tok = peek(s))) return 0;
					error(s, s->file, tok->line, "unexcepted token '%k'", tok);
					while (!skip(s, OPER_nop)) {
						if (skip(s, OPER_end)) break;//return 0;
						skip(s, 0);
					}
				}
			}
			if (tag->name) leave(s, 3, tag->name ? &def->args : 0);
		}
		if (kind) *kind = TYPE_def;
	}
	else def = type(s, 0);
	trace(-8, "leave:spec('%+T')", def);
	return def;
}

node dvar(state s, defn typ, int qual) {
	node prev = 0, root = 0;
	node body = 0, tag;
	defn ref;

	trace(+4, "enter:dclr('%k')", peek(s));

	if (typ == 0) {
		return expr(s, 0);
	}
	else while ((tag = next(s, TYPE_ref))) {
		defn def = typ, argv = 0;
		if (skip(s, PNCT_lp)) {				// function
			//~ trace(0, "function");
			enter(s, 0);
			args(s, fl_name);
			if (skip(s, OPER_beg)) {			// int foo(...){
				node tmp, list = 0;
				while (peek(s) && !skip(s, OPER_end)) {
					if ((tmp = stmt(s, 0))) {
						if (!list) body = tmp;
						else list->next = tmp;
						list = tmp;
					}
				}
			}
			leave(s, 3, &argv);
		}
		else while (skip(s, PNCT_lc)) {			// array	: int [20][30] | int(int)[100]
			defn tmp = newdefn(s, TYPE_arr);
			tmp->init = expr(s, CNST_int);
			if (!skip(s, PNCT_rc)) {
				error(s, s->file, s->line, "missing ']'");
				break;
			}
			tmp->type = def;
			def = tmp;
		}// */
		if (body && argv) {
			ref = declare(s, TYPE_fnc, tag, def, argv);
			ref->init = body;
		}
		else {
			ref = declare(s, TYPE_ref, tag, def, argv);
			if (skip(s, ASGN_set))
				ref->init = expr(s, TYPE_ref);
		}
		if (!prev) root = tag;
		else prev->next = tag;
		prev = tag;

		if (!skip(s, OPER_com)) break;
		if (!test(s, TYPE_ref))
			error(s, s->file, s->line, "unexpected ','");
		/*
		if (ast && ast->kind == OPER_com)
			ast->rhso = tok;
		else ast = tok;
		*/
		//~ trace(4, "declare('%T' as '%+T')", ref, typ);
	}

	trace(-4, "leave:dclr('%-k')", root);
	return root;
}

int argc(defn sym) {
	int argc = 0;
	defn arg = sym->args;
	while (arg) {
		argc += 1;
		arg = arg->next;
	}
	return argc;
}

void enter(state s, defn def) {
	s->nest += 1;
	s->scope[s->nest].csym = def;
	//~ s->scope[s->nest].stmt = NULL;
	//~ return &s->scope[s->nest];
}

void leave(state s, int mode, defn *d) {
	defn ref = 0, def = 0;
	int i;
	s->nest -= 1;
	for (i = 0; i < TBLS; i++) {
		defn def = s->deft[i];
		while (def && def->nest > s->nest) {
			defn tmp = def;
			def = def->deft;
			tmp->deft = 0;
		}
		s->deft[i] = def;
	}
	while (s->defs) {
		defn sym = s->defs;
		if (sym->nest <= s->nest) break;

		s->defs = sym->next;

		if (sym->kind == TYPE_ref) {		// ref
			if (mode & 4 && !sym->used)
				warn(s, 8, s->file, sym->link ? sym->link->line : 0, "unused variable '%T'", sym);
			sym->next = ref;
			ref = sym;
		} else {
			sym->next = def;
			def = sym;
		}

		if (d) {
			//~ if (mode & 2) // type
			sym->next = *d;
			*d = sym;
		}
	}
	//~ return &s->scope[s->nest + 1];
}

/** COMP.C ---------------------------------------------------------------------
 * scan a source
 * @param mode : no script mode
**/
/** STMT.C ---------------------------------------------------------------------
 * scan a statement (mode ? enable decl : do not)
 * @param mode : enable or not <decl>.
 *	ex : "for( ; ; ) int a;", if mode == 0 => error
 * stmt := ';'
!*	 | (<decl> | <expr>) ';'
 *	 | '{' <stmt>* '}'
 *	 | (static)? 'if' (<expr>) <stmt> ('else' <stmt>)?
 *	 | (static | parallel)? 'for' '(' <decl>?';' <expr>?';' <expr>? ')' <stmt>
+*	 | 'return' <expr>? ';'
+*	 | 'continue' ';'
+*	 | 'break' <IDTF>?;
+*	 | 'goto' <IDTF>;
+*	 | <IDTF> ':'
+*	 | 'sync' ('('<vars>')')? '{'<stmt>'}'		// synchtonized
+*	 | 'task' ('('<vars>')')? '{'<stmt>'}'		// parallel task
?*	 | 'fork'					// ? : task with code length = 0
x*sync {
x*	task {
x*		statements;
x*	}
x*	task {
x*		statements;
x*	}
x*	task {
x*		statements;
x*	}
x*} // sync : wait for tasks
x* static if(...)		// compile time test
x* static for(...)		// loop unroll
x* parallel for(...)		// parallel loop
**/
/** DECL.C ---------------------------------------------------------------------
 * scan a declaration, add to ast tree
 * @param mode : enable or not <expr>.
 *<decl>:

 * variable
 * 	typename <idtf> (= <expr>)?, (<idtf> (= <expr>)?)? \;
 + array
 * 	typename <idtf>[]... (= <expr>)?, (<idtf> (= <expr>)?)? \;
 + function
 * 	typename <idtf> \( <args> \) ({<stmt>})? \;
 + operator
 * 	typename <oper> \( <args> \) ({<stmt>})? \;
 * template

 *	define ...
 - 	define '{'(name'('constant)')'*'}'		// enum
 * 	define <idtf> = <cons>;				// typed, named const
 * 	define <idtf> <type>;				// typedef
 ? 	define <idtf>(args...) {stmt}			// inline
 *	struct (\:<pack>)? (tag) '{'(<decl>)*'}'	// record

 *	enumerate ...
 *	enum <idtf>? {(<idtf> (= cintexpr)?;)+}		// enum
 !	enum <idtf>\(...\) {(<idtf> \(... \);)+}	// enum

 *	packed ???

 *	typename
 *	template

 * var	(static)? <type> name = expr [, name = expr]*
 * arr	 $ (static)? <type> name'['expr']' (= '('(expr[,expr]*)')'|expr)?
X* fnc	 $ (static)? <type> name'('type name (= expr)? [,type name (= expr)?]*')'
X* ???	 $ (packed)? <type> name'('type name (= expr)? [,type name (= expr)?]*')'
 * 	 $ static float[100][100] a, b, c;
 *

//~ defn spec(state s, int *qual);		// type specifyer
//~ node dclr(state s, defn typ, int qual);	// declarator | expr
//~ node args(state s) ;			// scan function arguments

//~ packed(2) struct

//~ template class
//~ template function
//~ template operator

//~ template(typename t1, typename t2) pair;
//~ pair.t1

//~ template(typename ElementT, int size) class stack {
//~ 	ElementT bp[size], *sp = bp;
//~ 
//~ }

//~ enum token(int prec, int argc, str name) {
//~ 	TYPE_i32(0x1, 0, "integer");
//~ 	STMT_for(0x0, 0, "for");
//~ 	OPER_add(0xc, 2, "+");
//~ 	ASGN_mul(0x2, 2, "*=");
//~ }
//~ TYPE_i32.name === "integer"
**/
/** EXPR.C ---------------------------------------------------------------------
 * scan and or type check an expression
 * if mode == TYPE_ref then exit when ','
 * <expr> := 
 * 	 | CNST
 * 	 | IDTF
 * 	 | <expr> '[' <expr> ']'		\arr	array
+* 	 | '[' <expr> ']'			\ptr	pointer
 * 	 | <expr> '(' <expr>? ')'		\fnc	function
 * 	 | '(' <expr> ')'			\pri	precedence
 * 	 | OPER <expr>				\uop	unary operator
 * 	 | <expr> OPER <expr>			\bop	binary operator
 * 	 | <expr> '?' <expr> ':' <expr>		\top	ternary operator
 * 	returns the root node of the expression tree
**/
//}
