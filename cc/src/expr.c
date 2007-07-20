#include <stdio.h>

#include "node.h"
#include "scan.c"

tree expr(state s) {
	typedef struct {
		node	ref;	// token reference
		long	pri;	// token priority
	} prec;
	char	sym_sb[MAX_TOKS], *sym_sp = sym_sb;				// symbol stack (priority)
	prec	opr_sb[MAX_TOKS], *opr_sp = opr_sb;				// operator precedence stack
	node	tok_bp[MAX_TOKS], *tok_lp = tok_bp;				// token array in postfix form
	int		level = 0, get_next = get_expr;
	node	next, tok;
	while ((tok = next_token(s))) {
		int toktype = tok->kind;
		switch (tok->kind & tok_msk) {						// syntax check
			default : {
				raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
				get_next |= got_eror;
			} break;
			case tok_err : get_next |= got_eror; break;
			case tok_sym : switch (tok->kind) {				// symbol, operator ???
				default : {									// error invalid character ???
					raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
					get_next |= got_eror;
				} break;
				case sym_und : {
					raiseerror(s, -1, tok->line, "stray '%c' in program", *s->buffp);
					get_next |= got_eror;
				} break;
				case sym_sem : {			// ';'
					get_next |= got_stop;
					back_token(s, tok);
				} break;
				case sym_com : {			// ',' -> Comma operator
					//~ if (sym_sp == sym_sb) {
						//~ get_next |= got_stop;
						//~ back_token(s, tok);
					//~ }
					//~ else {
						if ((get_next & get_expr))
							get_next |= got_stxe;
						else {
							tok->kind = opr_com;
							get_next(get_expr);
						}
					//~ }
				} break;
				case sym_dot : {			// '.' -> Member ref
					if (!(get_next & get_dots))
						get_next |= got_stxe;
					else {
						tok->kind = opr_dot;
						get_next(get_name);
					}
				} break;
				case sym_lbi : {			// '[' -> Element ref
					if (!(get_next & get_arrs))
						get_next |= got_stxe;
					else {
						tok->kind = opr_idx;
						get_next(get_expr);
					}
				} break;
				case sym_rbi : {			// ']'
					if (sym_sp > sym_sb) {
						if (sym_sp[-1] != '[')
							get_next |= got_stxe;
						else if (get_next & ack_null)
							*tok_lp++ = NULL;
						else if (get_next & get_expr)
							get_next |= got_stxe;
						get_next(get_arrs);
					}
					else {
						get_next |= got_stop;
						back_token(s, tok);
					}
				} break;
				case sym_lbs : {			// '('
					if (!(get_next & get_expr))
						get_next |= got_stxe;
					//~ get_next(get_expr);
				} break;
				case sym_rbs : {			// ')'
					if (sym_sp > sym_sb) {
						if (sym_sp[-1] != '(')
							get_next |= got_stxe;
						else if (get_next & ack_null)
							*tok_lp++ = NULL;
						else if (get_next & get_expr)
							get_next |= got_stxe;
						get_next(0);
					}
					else {
						get_next |= got_stop;
						back_token(s, tok);
					}
				} break;
				case sym_lbb : {			// '{'		???
					//~ get_next |= got_stop;
					//~ back_token(s, tok);
				} break;
				case sym_rbb : {			// '}'		???
					//~ if (sym_sp > sym_sb) {
						//~ if (sym_sp[-1] != '{')
							//~ get_next |= got_stxe;
						//~ else if (get_next & ack_null)
							//~ *tok_sp++ = NULL;
						//~ else if (get_next & get_expr)
							//~ get_next |= got_stxe;
						//~ get_next(0);
					//~ }
					//~ else {
						//~ get_next |= got_stop;
						//~ back(s, tok);
					//~ }
				} break;
				case sym_qtm : {			// '?'
					if (get_next & get_expr)
						get_next |= got_stxe;
					else {
						tok->kind = opr_ite;
						get_next(get_expr);
					}
				} break;
				case sym_col : {			// ':'
					if (sym_sp > sym_sb) {
						if (sym_sp[-1] != '?')
							get_next |= got_stxe;
						else if ((get_next & get_expr))
							get_next |= got_stxe;
						else {
							tok->kind = opr_els;
							get_next(get_expr);
						}
					}
					else {
						get_next |= got_stxe;
					}
				} break;
			} break;
			case tok_val : {								// constant
				if (!(get_next & get_cval))
					get_next |= got_stxe;
				else {
					get_next(0);
				}
			} break;
			case tok_idf : {								// typename, function ???
				if(!tok->type) {
					raiseerror(s, -1, tok->line, "undeclared symbol \"%s\"" , tok->name);
					//~ install(s, tok->name, 0, -1);
				}
				if (!(get_next & get_name))
					raiseerror(s, -1, tok->line, "syntax error before `%s`", tok->name);
				if ((next = next_token(s))) {
					switch (next->kind) {
						case sym_lbs : {				// function call
							tok->kind = opr_fnc;
							get_next(get_expr | ack_null);
						} break;
						case sym_lbi : {				// array
							get_next(get_arrs);
						} break;
						case sym_dot : {				// member
							get_next(get_dots);
						} break;
						default : {						// variable
							get_next(0);
						} break;
					}
					back_token(s, next);
				}
			} break;
			case tok_opr : {								// operator
				if ((get_next & get_expr)) {
					switch (tok->kind) {
						default : get_next |= got_eror; break;
						case opr_neg : break;
						case opr_not : break;
						//~ case opr_neg : break;
						//~ case opr_ptr : break;
						case opr_add : tok->kind = opr_unp; break;
						case opr_sub : tok->kind = opr_unm; break;
					}
				}
				get_next(get_expr);
			} break;
		}
		if (get_next & got_stxe) {							// syntax error
			raiseerror(s, -1, tok->line, "syntax error before \"%s\" token", s->buffp);
			//~ get_next ^= got_eror;
		}
		if (get_next & got_stop) break;
		if ((tok->kind & tok_msk) == tok_opr) {				// operator found ?
			int pri = (level << 8) + (tok->kind & (opm_pri|opf_rla));
			while (opr_sp > opr_sb) {
				if (opr_sp[-1].pri < pri) break;
				*tok_lp++ = (--opr_sp)->ref;
			}
			opr_sp->pri = pri & 0xfffffff0;
			opr_sp->ref = tok;
			opr_sp += 1;
		}
		else if ((tok->kind & tok_msk) != tok_sym) {		// is a const or value
			*tok_lp++ = tok;
		}
		else eat_token(s, tok);
		if ((toktype & tok_msk) == tok_sym) {
			switch (toktype) {
				//~ default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__); break;
				case sym_lbs : {			// '('
					*sym_sp++ = '(';
					level += 1;
				} break;
				case sym_lbi : {			// '['
					*sym_sp++ = '[';
					level += 1;
				} break;
				case sym_lbb : {			// '{'
					*sym_sp++ = '{';
					//~ level += 1;
				} break;
				case sym_qtm : {			// '?'
					*sym_sp++ = '?';
					//~ level -= 0;
				} break;
				case sym_rbs : {			// ')'
					sym_sp -= 1;
					level -= 1;
				} break;
				case sym_rbi : {			// ']'
					sym_sp -= 1;
					level -= 1;
				} break;
				case sym_rbb : {			// '}'
					sym_sp -= 1;
					//~ level -= 1;
				} break;
				case sym_col : {			// ':'
					sym_sp -= 1;
					//~ level += 0;
				} break;
			}
		}
	}
	while (opr_sp > opr_sb) {								// copy remaining operators
		*tok_lp++ = (--opr_sp)->ref;
	}
	if (sym_sp > sym_sb) {
		raiseerror(s, -1, s->line, "unclosed \"%c\"", sym_sp[-1]);
		return 0;
	}
	else if (get_next & get_expr) {
		raiseerror(s, -1, s->line, "missing expression");
		return 0;
	}
	else if (get_next & get_cval) {
		raiseerror(s, -1, s->line, "missing constant");
		return 0;
	}
	else if (get_next & get_name) {
		raiseerror(s, -1, s->line, "missing typename");
		return 0;
	}
	if (tok_lp > tok_bp) {									// build tree and do semantic analyzis
		node *ptr;
		sym_sp = sym_sb-1;
		for (ptr = tok_bp; ptr < tok_lp; ptr += 1) {
			node tmp = *ptr;
			if (tmp && (tmp->kind & tok_msk) == tok_opr) {
				if ((tmp->kind & otf_unf) == 0) {					// binary
					if ((opr_sp -= 2) >= opr_sb) {
						tmp->orhs = opr_sp[1].ref;
						tmp->olhs = opr_sp[0].ref;
					}
				}
				else if ((opr_sp -= 1) >= opr_sb) {
					tmp->orhs = opr_sp[0].ref;
					tmp->olhs = 0;
				}
				if (opr_sp < opr_sb) {
					raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
					return 0;
				}
				if (tmp->kind == opr_ite) {
					node opr = opr->orhs;
					if (!opr || opr->kind != opr_els) {
						raiseerror(s, opr->line, -1, "xxx");
					}
				}
				if (tmp->kind == opr_fnc) {
					node argl = tmp->orhs;
					int argc = 0;
					while (argl) {
						argc += 1;
						argl = argl->kind == opr_com ? argl->olhs : NULL;
					}
					if (tmp->type == oper_sof) {		// sizeof(int);
						//~ if (argc != 1)
							//~ raiseerror(s, -1, tmp->line, "sizeof operator must take one argument");
						if ((tmp->orhs->kind & tok_msk) == tok_opr) {
							raiseerror(s, -1, tmp->line, "invalid argument for sizeof operator");
						}
						tmp->kind = tok_val;
						tmp->type = type_int;
						tmp->cint = nodesize(tmp->orhs);
					}
					if (tmp->type == oper_out) {		// ptint(...);
						//~ if (argc != 1)
							//~ raiseerror(s, -1, tmp->line, "sizeof operator must take one argument");
						//~ tmp->kind = opr_out;
						//~ tmp->type = type_void;
						//~ walk(tmp->orhs,0,0);
					}
					else if (tmp->type == type_int) {	// int(a);
						if (argc != 1)
							raiseerror(s, -1, tmp->line, "typecast operator must take one argument");
						else {
							tmp->kind = opr_tpc;
							tmp->type = type_int;
						}
					}
					else if (tmp->type == type_flt) {	// float(3);
						if (argc != 1)
							raiseerror(s, -1, tmp->line, "typecast operator must take one argument");
						else {
							tmp->kind = opr_tpc;
							tmp->type = type_flt;
						}
					}
					else {
						raiseerror(s, -3, tmp->line, "undefined first in use function \"%s\"/%d",tmp->name, argc);
					}
				}
				else if (!tmp->type) {								// set return type
					if ((tmp->type = cast(s, tmp, tmp->olhs, tmp->orhs))) {
						fold(s, tmp, tmp->olhs, tmp->orhs);
					}
				}
			}
			opr_sp->ref = tmp;
			opr_sp += 1;
		}
		return tok_lp[-1];					// root
	}
	return 0;
}

/*--8<--------------------------------------------------------------------------
//~ #include "code.c"
struct state_t s;
node t;

int main() {
	//~ int a,b,c = 6,fix = 2, A[1][2];
	//~ char exp[]  = "mat[4][8][2] = mat[4][8][2] + 5 * + - - cross(2+3, 67, 7);\n";
	char exp[]  = "* /";
	//~ char exp[]  = "a += div(1,2,3,4);\n";
	//~ char exp[]  = "b = c;\n";
	//~ char exp[]  = "b = 2 * (a += 2);\n";
	//~ char exp[]  = "a = b = c = sizeof(void);\n";
	//~ char exp[]  = "a[6];\n";
	//~ char exp[]  = "(a[1][2][3] = 3)*2;\n";
	//~ char exp[]  = "2 - Math.add(1, 2, 3) - sizeof(1) + int(b);\n";
	//~ char exp[]  = "dstPixel[-x][y] = \nsrcPixel[x + ((componentX(x, y) - 128) * scaleX) / 256][ y + ((componentY(x, y) - 128) *scaleY) / 256];\n";
	//~ char exp[]  = "a1() + 34 - a2(1,2+9,3,4,5) % a3(1) - 3 * a4(2,3,4);\n";
	//~ char exp[]  = "(a <= 0 ? 1-3 : b < 0 ? 0-9 : 2-1)*2;\n";
	//~ char exp[]  = "math.sin(3.14);\n";
	//~ char exp[]  = "a = b = 1+b*3 + m[2][3];\n";
	//~ char exp[]  = "2 + 3 * -4;\n";
	//~ char exp[]  = "a['\xff'][a+3]\\\n - 5;\n";
	//~ char exp[]  = "a = xxx.c[rr];\n";
	//~ char exp[]  = "2 += 3 -= float(123 & 0xf0);\n";
	//~ char exp[]  = "1+0-8;2 += -3 -= float(123 & 0xf0);\n";
	//~ char exp[]  = "a+b+c+sin();\n";
	//~ char exp[]  = "((a <= 0) ? (1) : (a ? (-1) : (0)))*2;\n";
	//~ char exp[]  = "f(\"alma\", 2);\n";
	//~ printf("(%d):\t%s", fix, exp);
	cc_init(&s, exp);
	s.name = __FILE__;
	s.line = __LINE__;
	walk(t = expr(&s), 0, 3);
	printf("\n");
	//~ cgen(t, 1); printf("\n");
	//~ printf("state_t : %d\n", sizeof(struct state_t));
	//~ printf("token_t : %d\n", sizeof(struct token_t));
	//~ printf("llist_t : %d\n", sizeof(struct llist_t));
	printf("code size : %d\n", s.buffp - s.buffer);
	return 0;
}

//--8<------------------------------------------------------------------------*/

void walk(tree ptr, int fmt, int fix) {
	if (!ptr) return;
	if ((ptr->kind & tok_msk) == tok_opr) {
		switch (fix) {
			case 0 : 
			case 2 : {						// infix
				//~ printf("(%s)", nodename(nodetype(ptr)));
				//~ printf("(%s,%s)", nodename(nodetype(ptr)), nodename(ptr));
				if (!fmt) printf("(");
				if (!(ptr->kind & otf_unf) && ptr->olhs) {
					walk(ptr->olhs, fmt, fix);
					if (!fmt) printf(" ");
				}
				print_token(ptr, fmt);
				if (ptr->orhs) {
					if (!fmt) printf(" ");
					walk(ptr->orhs, fmt, fix);
				}
				if (!fmt) printf(")");
			} break;
			case 1 : {						// prefix
				if (!fmt) printf("(");
				print_token(ptr, fmt);
				if (!fmt) printf(" ");
				walk(ptr->olhs, fmt, fix);
				if (!fmt) printf(" ");
				walk(ptr->orhs, fmt, fix);
				if (!fmt) printf(")");
			} break;
			case 3 : {						// postfix
				//~ printf("(%s)", nodename(nodetype(ptr)));
				//~ printf("(%s,%s)", nodename(nodetype(ptr)), nodename(ptr));
				//~ if (!fmt) printf("(");
				walk(ptr->olhs, fmt, fix);
				if (!fmt) printf(" ");
				walk(ptr->orhs, fmt, fix);
				if (!fmt) printf(" ");
				print_token(ptr, fmt);
				//~ if (!fmt) printf(")");
			} break;
		}
	}
	else {					// leaf
		//~ symbol defn = ptr->type;
		//~ printf("(%s,%s)", nodename(nodetype(ptr)), nodename(ptr));
		/*
		if (defn && (defn->kind & (tok_msk|idf_msk)) == idf_ref) {
			if ((defn->kind & 0xfe) == 0) {
				printf("(%d)", defn->offs);
			}
		}//*/
		print_token(ptr, fmt);
		//~ if (fmt == 0) printf(" ");
	}
}
