#include <stdio.h>

#include "node.h"
#include "expr.c"

tree decl(state s) {
	typedef struct {
		node	ref;	// token reference
		long	pri;	// token priority
	} prec;
	char	sym_sb[MAX_TOKS], *sym_sp = sym_sb;				// symbol stack (priority)
	prec	opr_sb[MAX_TOKS], *opr_sp = opr_sb;				// operator precedence stack
	node	tok_sb[MAX_TOKS], *tok_sp = tok_sb;				// token stack
	int		level = 0, get_next = get_type, mod = 0;//, action=0;
	node	next, tok;
	symbol	def;
	while ((tok = next_token(s))) {
		//~ #define get_next(_get_type) {get_next &= 0xffffff00; get_next |= (_get_type);}
		int toktype = tok->kind;
		switch (tok->kind & tok_msk) {						// syntax check
			default : {
				raiseerror(s, -999, 0, "%s:%d: <unhandled exception", __FILE__, __LINE__);
				get_next |= got_eror;
			} break;
			case tok_err : get_next |= got_eror; break;
			case tok_sym : switch (tok->kind) {				// symbol, operator ???
				default : {									// error invalid character ???
					//~ raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
					get_next |= got_eror;
				} break;
				case sym_und : {
					raiseerror(s, -1, tok->line, "stray '%c' in program", *s->buffp);
					get_next |= got_eror;
				} break;
				case sym_sem : {			// ';' -> install
					get_next |= got_stop;
					back_token(s, tok);
				} break;
				case sym_com : {			// ',' -> Comma operator, install
					if (!(get_next & ack_stop))
						get_next |= got_stxe;
					else {
						tok->kind = opr_com;
						get_next(get_name);
					}
				} break;
				/*case sym_dot : {			// '.' -> Member ref, advance?
					if (!(get_next & get_dots))
						get_next |= got_stxe;
					else {
						tok->kind = opr_dot;
						get_next(get_name);
					}
				} break;*/
				case sym_lbi : {			// '[' -> Element ref
					if (!(get_next & get_arrs))
						get_next |= got_stxe;
					else {
						tok->kind = opr_idx;
						get_next(get_cval);
					}
				} break;
				case sym_rbi : {			// ']'
					if (sym_sp > sym_sb) {
						if (sym_sp[-1] != '[')
							get_next |= got_stxe;
						else if (get_next & ack_null)
							*tok_sp++ = NULL;
						//~ else if (get_next & get_expr)
							//~ get_next |= got_stxe;
						get_next(get_arrs | ack_stop);
					}
					else {
						get_next |= got_stop;
						back_token(s, tok);
					}
				} break;
				/*case sym_lbs : {			// '('
					if (!(get_next & get_expr))
						get_next |= got_stxe;
					//~ get_next(get_expr);
				} break;*/
				/*case sym_rbs : {			// ')'
					if (sym_sp > sym_sb) {
						if (sym_sp[-1] != '(')
							get_next |= got_stxe;
						else if (get_next & get_null)
							*tok_sp++ = NULL;
						else if (get_next & get_expr)
							get_next |= got_stxe;
						get_next(0);
					}
					else {
						get_next |= got_stop;
						back_token(s, tok);
					}
				} break;*/
				/*case sym_lbb : {			// '{'		???
					//~ get_next |= got_stop;
					//~ back_token(s, tok);
				} break; //*/
				case sym_rbb : {			// '}'		???
					//~ if (sym_sp > sym_sb) {
						//~ if (sym_sp[-1] != '{')
							//~ get_next |= got_stxe;
						//~ else if (get_next & get_null)
							//~ *tok_sp++ = NULL;
						//~ else if (get_next & get_expr)
							//~ get_next |= got_stxe;
						//~ get_next(0);
					//~ }
					//~ else {
						get_next |= got_stop;
						back_token(s, tok);
					//~ }
				} break;
				/*case sym_qtm : {			// '?'
					if (get_next & get_expr)
						get_next |= got_stxe;
					else {
						tok->kind = opr_ite;
						get_next(get_expr);
					}
				} break;*/
				/*case sym_col : {			// ':'
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
				} break;*/
			} break;
			case tok_val : {								// constant
				get_next |= got_eror;
			} break;
			case tok_idf : {								// typename
				if (!(get_next & get_decl))
					raiseerror(s, -1, tok->line, "syntax error before `%s`", tok->name);
				else {
					if ((get_next & get_type)) {
						if (!tok->type) {
							raiseerror(s, -1, tok->line, "undeclared symbol \"%s\"" , tok->name);
						}
						else {
							int kind = tok->type->kind;
							if ((kind & idf_msk) == idf_dcl) {
								if (kind & 0xff) {
									mod |= kind & 0xff;
									get_next(get_type);
								}
								else {
									def = tok->type;
									get_next(get_name);
								}
							}
							else raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
						}
					}
					else if ((next = next_token(s))) {
						symbol ref;
						int offset = 0;
						switch (next->kind) {
							//~ case sym_lbs : {				// function decl
								//~ tok->kind = opr_fnc;
								//~ get_next(get_expr | get_null);
							//~ } break;
							case sym_lbi : {				// array
								get_next(get_arrs);
							} break;
							//~ case sym_dot : {			// member
								//~ get_next(get_dots);
							//~ } break;
							default : {						// variable
								get_next(ack_stop);
							} break;
						}
						back_token(s, next);
						if (tok->type && ((tok->type->kind & (tok_msk|idf_msk)) != idf_ref)) {
							raiseerror(s, -1, tok->line, "invalid use of symbol \"%s\"", tok->name);
						}
						else {
							//~ if (tok->type) raiseerror(s, 1, tok->line, "redeclaration of symbol \"%s\" ", tok->name);
							ref = install(s, def, tok->name, offset, idf_ref | mod | ((get_next & get_arrs) ? idm_sta : 0));
							if (!tok->type) tok->type = ref;
						}
					}
				}
			} break;
			case tok_opr : {								// operator
				switch (toktype) {
					default : {
						raiseerror(s, -1, tok->line, "syntax error " , s->buffp);
						//~ get_next |= got_eror;
					} break;
					/*
					case ass_equ : {				// '='
						if (!(get_next & ack_stop))
							get_next |= got_eror;
						else {
							get_next(get_cval);
						}
					} break;//*/
				}
			} break;
		}
		if (get_next & got_stxe) {							// syntax error
			raiseerror(s, -1, tok->line, "syntax error before \"%s\" token", s->buffp);
			get_next ^= got_eror;
		}
		if (get_next & got_stop) {
			while (opr_sp > opr_sb) {						// copy remaining operators
				*tok_sp++ = (--opr_sp)->ref;
			}
			break;
		}
		if ((tok->kind & tok_msk) == tok_opr) {				// operator found ?
			int pri = (level << 8) + (tok->kind & 0xf3);
			while (opr_sp > opr_sb) {
				if (opr_sp[-1].pri < pri) break;
				*tok_sp++ = (--opr_sp)->ref;
			}
			opr_sp->pri = pri & 0xfffffff0;
			opr_sp->ref = tok;
			opr_sp += 1;
		}
		else if ((tok->kind & tok_msk) != tok_sym) {		// is a const or value
			*tok_sp++ = tok;
		}
		else eat_token(s, tok);
		if ((toktype & tok_msk) == tok_sym) {
			switch (toktype) {
				//~ default : raiseerror(s, 0, 0, "%s:%d: <unhandled exception>(%x)", __FILE__, __LINE__, tok->kind); break;
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
					//~ level -= 0;
				} break;
			}
		}
		if (get_next & get_cval) {
			*tok_sp++ = expr(s);
			get_next(ack_stop);
		}
		//~ #undef get_next
	}
	if (sym_sp > sym_sb) {
		raiseerror(s, -1, s->line, "unclosed \"%c\"", sym_sp[-1]);
		return 0;
	}
	else if (!(get_next & ack_stop)) {
		raiseerror(s, -1, s->line, "missing expression");
		return 0;
	}
	if (tok_sp > tok_sb) {
		node *ptr;
		symbol ref = NULL;
		for (ptr = tok_sb; ptr < tok_sp; ptr += 1) {
			node tmp = *ptr;
			//~ print_token(tmp,7); //printf(" ");
			if (tmp && (tmp->kind & tok_msk) == tok_opr) {
				if ((tmp->kind & otf_unf) == 0) {					// binary
					if (tmp->orhs || tmp->olhs);
					else if ((opr_sp -= 2) >= opr_sb) {
						tmp->orhs = opr_sp[1].ref;
						tmp->olhs = opr_sp[0].ref;
					}
				}
				else {
					if (tmp->orhs);
					else if ((opr_sp -= 1) >= opr_sb) {
						tmp->orhs = opr_sp[0].ref;
					}
				}
				if (opr_sp < opr_sb) {
					raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
					return 0;
				}
				if (tmp->kind == opr_idx) {
					if (ref) ref->arrs = tmp;
					if (tmp->orhs->kind != tok_val || tmp->orhs->type != type_int) {
						raiseerror(s, -1, tmp->line, "integer constant expected");
					}
					else if(tmp->orhs->cint <= 0) {
						raiseerror(s, -1, tmp->line, "integer constant must be greater than zero");
					}
				}
				else ref = 0;
				/*else if (tmp->kind == opr_fnc) {
					if (ref) ref->args = tmp;
					if (tmp->orhs->kind != tok_val || tmp->orhs->type != type_int) {
						raiseerror(s, -1, tmp->line, "integer constant expected");
					}
					else if(tmp->orhs->cint <= 0) {
						raiseerror(s, -1, tmp->line, "integer constant must be greater than zero");
					}
				}*/
			}
			if (tmp && (tmp->kind & tok_msk) == tok_idf) {
				if (tmp->type && tmp->type->type) {					// reference
					ref = tmp->type;
				}
			}
			opr_sp->ref = tmp;
			opr_sp += 1;
		}
		/*for (ptr = tok_sb; ptr < tok_sp; ptr += 1) {
			node tmp = *ptr;
			if (tmp && (tmp->kind & tok_msk) == tok_idf) {
				if (tmp->type && tmp->type->type) {			// declared reference
					if(!tmp->type->arrs) walk(tmp->type, 0,2);
					walk(tmp->type->arrs, 0,2);
					printf("\n");
				}
			}
		}*/
		//~ return tok_sp[-1];
	}
	return 0;
}

/*--8<--------------------------------------------------------------------------
struct state_t s;
node t;

int main() {
	//~ int b += c;
	//~ int 4 a, b;
	//~ int a, b, c, d;
	//~ char exp[]  = "int asin(int a, int b, int c, float d);\n";
	//~ char exp[]  = "int abs(double d[10], int e){\n";
	//~ char exp[]  = "int a[2], b[100], c[3];\n";
	//~ char exp[]  = "int abs(int a, int b, int c, int b[]);\n";
	//~ char exp[]  = "float a,a[12][22], c[10][20*77];\n";
	char exp[]  = "static float i, ind[10][20], inc[1][2];\n";
	//~ extern static int a;
	//~ char exp[]  = "int a(int) = 0, f;\n";
	puts(exp); fflush(stdout);
	cc_init(&s, exp);
	s.name = __FILE__;
	s.line = __LINE__;
	t = decl(&s);
	walk(t,0,2);
	//~ printf("%08x\n", t->type->kind);
	//~ printf("%s", nodetype(t)->name);
	//~ printf("\n");
	//~ print_strt(&s);
	//~ print_defn(&s);
	printf("scan : %d Bytes\n", s.buffp - s.buffer);
	printf("errors : %d\n", s.errc);
	//~ printf("memory : %d\n", s.memu);
	//~ printf("%d\n", sizeof(struct state_t));
	//~ printf("%d\n", sizeof(struct token_t));
	//~ printf("%d\n", sizeof(struct llist_t));
	//~ printf("%d\n", sizeof(struct defnt_t));

	//~ print_strt(&s);
	//~ print_defn(&s);
	return 0;
}

//--8<------------------------------------------------------------------------*/

void print_defn(state s) {
	int i;
	printf("definitions\n");
	for (i = 0; i < MAX_TBLS; i+=1) {
		struct llist_t *list = s->deft[i];
		if (list) {
			printf("%3d\t",i);
			while (list) {
				symbol defn = (symbol)list->data;
				if ((defn->kind & tok_msk) == tok_val) {
					printf("const string \"%s\"", defn->name); 
				}
				else switch (defn->kind & idf_msk) {
					case idf_kwd : printf("keyword %s", defn->name); break;
					case idf_ref : {
						printf("name(offs %d, size %d, mods %d) ", defn->size, typesize(defn), defn->kind & 0xff);
						walk(defn->arrs ? defn->arrs : (node)defn, 0, 3);
					} break;
					case idf_dcl : printf("type(size %d) %s", defn->size, defn->name); break;
					default   : printf("keyword %s", defn->name); break;
				}
				//~ printf("%s ", defn->name);
				//~ print_token(defn,1|2|4);
				if ((list = list->next)) printf(", ");
			}
			printf("\n");
		}
	}
}
