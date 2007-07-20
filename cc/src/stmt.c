#include <stdlib.h>
#include <stdio.h>

#include "node.h"
#include "decl.c"

node addcode(state s, node n) {
	llist list;
	if (!n) return 0;
	list = (struct llist_t*)(s->buffp);
	s->buffp += sizeof(struct llist_t);
	list->data = n;
	list->flgs = s->data;
	list->next = 0;
	if (s->cend) {
		s->cend->next = list;
	}
	else {
		s->cbeg = list;
	}
	s->cend = list;
	return n;
}

tree stmt(state s) {
	int get_next = -1;
	node tok = next_token(s);
	if (!tok) return 0;
	switch (tok->kind & tok_msk) {						// syntax check
		default : {
			raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
			get_next |= got_eror;
		} break;
		case tok_err : break;
		case tok_sym : switch (tok->kind) {				// symbol, operator ???
			default : {
				//~ raiseerror(s, 999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
				get_next = get_expr;
			} break;
			case sym_und : {			// error invalid character
				raiseerror(s, -1, tok->line, "stray '%c' in program", *s->buffp);
				return 0;
			} break;
			case sym_sem : {			// ';'		stop
				eat_token(s, tok);
				return 0;
			} break;
			case sym_lbb : {			// '{'		stop
				eat_token(s, tok);
				enterblock(s, NULL, NULL, NULL, NULL);
				return 0;
			} break;
			case sym_rbb : {			// '}'		stop
				if (s->level > 0) {
					symbol link = (symbol)s->levelb[s->level].link;
					node incr = s->levelb[s->level].incr;		// continue;
					node test = s->levelb[s->level].test;		// break;
					node fork = s->levelb[s->level].fork;		// fork;
					if (s->levelb[s->level].wait) {				// wait
						node jmp = new_token(s);
						jmp->kind = opr_jmp;
						jmp->type = stmt_fork;
						jmp->olhs = 0;
						jmp->orhs = 0;
						addcode(s, jmp);
					}
					leaveblock(s);
					if (fork) {
						node nop = new_token(s);
						nop->kind = opr_nop;
						nop->line = ++s->jmps;
						fork->orhs = nop;
						addcode(s,nop);
					}
					if (link == stmt_for) {
						if (incr && test && test->kind == opr_jmp) {
							node jmp = new_token(s);
							jmp->kind = opr_jmp;
							jmp->olhs = 0;
							jmp->orhs = incr;
							addcode(s, jmp);
							tok->kind = opr_nop;
							tok->line = ++s->jmps;
							test->orhs = tok;
							addcode(s, tok);
							if (fork) {
								node jmp = new_token(s);
								jmp->kind = opr_jmp;
								jmp->type = stmt_fork;
								jmp->olhs = 0;
								jmp->orhs = 0;
								addcode(s, jmp);
							}
						}
						else raiseerror(s, -999, 0, "unhandled exception %s:%d(endfor)", __FILE__, __LINE__);
					}
					else if (link == stmt_if) {
						if (test) {
							tok->kind = opr_nop;
							tok->line = ++s->jmps;
							test->orhs = tok;
							addcode(s, tok);
						}
						else raiseerror(s, -999, 0, "unhandled exception %s:%d(endiff)", __FILE__, __LINE__);
					}
					else {
						tok->kind = opr_nop;
						tok->line = 0;
						addcode(s, tok);
					}
				}
				else raiseerror(s, -1, tok->line, "syntax error before '}' token");
			} return 0;
		} break;
		case tok_idf : {								// stmt, decl, expr ???
			if (tok->type) {
				switch (tok->type->kind & idf_msk) {
					default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__); break;
					/*case tok_def : {
						//~ ... TODO
					} break;*/
					case idf_kwd : {
						get_next = get_stmt;
					} break;
					case idf_dcl : {
						get_next = get_decl;
					} break;
					case idf_ref : {
						get_next = get_expr;
					} break;
				}
			}
			else {
				//~ raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__); break;
				get_next = get_expr;
			}
		} break;
		case tok_opr : {								// operator : expr
			get_next = get_expr;
		} break;
		case tok_val : {								// constant : expr
			get_next = get_expr;
		} break;
	}
	if (get_next == -1 || get_next == get_expr) {
		node tmp;
		back_token(s, tok);
		tok = expr(s);
		tmp = next_token(s);
		if (tmp && tmp->kind == sym_sem) {	//;
			eat_token(s, tmp);
			//~ if ((tok->kind & (tok_msk | 0xff)) != (opr_ass)) {
				//~ raiseerror(s, 2, tmp->line, "statement has no effect");
			//~ }
			return addcode(s, tok);
		}
		else raiseerror(s, -1, s->line, "missing ';'");
	}
	else if (get_next == get_decl) {
		node tmp;
		back_token(s, tok);
		tok = decl(s);
		tmp = next_token(s);
		if (tmp && tmp->kind == sym_sem) {	//;
			eat_token(s, tmp);
			return addcode(s, tok);
		}
		else raiseerror(s, -1, s->line, "missing ';'");
	}
	else if (get_next == get_stmt) {
		node init = 0, test = 0, incr = 0;
		node tmp;
		if (tok->type == stmt_fork) {
			tmp = next_token(s);				// read '{'
			if (tmp && tmp->kind == sym_lbb) {
				node jmp = tmp;
				addcode(s, jmp);
				jmp->kind = opr_jmp;
				jmp->type = stmt_fork;
				jmp->olhs = 0;
				jmp->orhs = 0;
				enterblock(s, 0, jmp, 0, 0);
			}
			else raiseerror(s, -1, tok->line, "fork{<statement>};");
		}
		else if (tok->type == stmt_for) {
			symbol fork = 0;
			tmp = next_token(s);				// read '('
			if (!tmp || tmp->kind != sym_lbs) {
				tmp = 0;
			}
			else eat_token(s, tmp);
			if (tmp && (tmp = next_token(s))) {			// read ';' or init;
				if (tmp->kind != sym_sem) {				// init
					back_token(s, tmp);
					init = expr(s);						// expr | decl
					if ((tmp = next_token(s))) {		// ;
						if (tmp->kind != sym_sem) {
							tmp = 0;
						}
					}
				}
				eat_token(s, tmp);
			}
			if (tmp && (tmp = next_token(s))) {			// read ';' or test;
				if (tmp->kind != sym_sem) {				// test
					back_token(s, tmp);
					test = expr(s);
					if ((tmp = next_token(s))) {		// ;
						if (tmp->kind != sym_sem) {
							tmp = 0;
						}
					}
				}
				eat_token(s, tmp);
			}
			if (tmp && (tmp = next_token(s))) {			// read ')' or incr;
				if (tmp->kind != sym_sem) {				// incr
					back_token(s, tmp);
					incr = expr(s);
					if ((tmp = next_token(s))) {		// )
						if (tmp->kind != sym_rbs) {
							tmp = 0;
						}
					}
				}
				eat_token(s, tmp);
			}
			if (tmp && (tmp = next_token(s))) {			// read '{' or fork{
				if (tmp->kind != sym_lbb) {				// fork?
					if(tmp->type == stmt_fork) {
						fork = stmt_fork;
					}
					if ((tmp = next_token(s))) {		// {
						if (tmp->kind != sym_lbb) {
							tmp = 0;
						}
					}
				}
				eat_token(s, tmp);
			}
			if (tmp) {
				node nop = new_token(s);
				node nop2 = new_token(s);
				node jmp = new_token(s);
				node jnz = new_token(s);
				addcode(s, init);
				addcode(s, jmp);
				addcode(s, nop);
				addcode(s, incr);
				addcode(s, nop2);
				addcode(s, jnz);
				nop->kind = opr_nop;
				nop->line = ++s->jmps;
				nop->olhs = 0;
				nop->orhs = 0;
				nop2->kind = opr_nop;
				nop2->line = ++s->jmps;
				nop2->olhs = 0;
				nop2->orhs = 0;
				jmp->kind = opr_jmp;
				jmp->olhs = 0;
				jmp->orhs = nop2;
				jnz->kind = opr_jmp;
				jnz->olhs = test;
				jnz->orhs = 0;
				if (fork) {
					jmp = new_token(s);
					addcode(s, jmp);
					jmp->kind = opr_jmp;
					jmp->type = stmt_fork;
					jmp->olhs = 0;
					jmp->orhs = 0;
				}
				else jmp = 0;
				enterblock(s, stmt_for, jmp, jnz, nop);
			}
			else raiseerror(s, -1, tok->line, "for(<expression>;<expression>;<expression>){<statement>};");
			return (node)stmt_for;
		}
		else if (tok->type == stmt_if) {
			tmp = next_token(s);							// read '('
			if (tmp && tmp->kind == sym_lbs) {
				eat_token(s, tmp);
				test = expr(s);								// read test
				tmp = next_token(s);
				if (tmp && tmp->kind == sym_rbs) {			// read ')'
					eat_token(s, tmp);
					tmp = next_token(s);
					if (tmp && tmp->kind == sym_lbb) {		// read '{'
						eat_token(s,tmp);
					}
					else tmp = 0;
				}
				else tmp = 0;
			}
			if (tmp) {
				node jnz = new_token(s);
				addcode(s, jnz);
				jnz->kind = opr_jmp;
				jnz->olhs = test;
				jnz->orhs = 0;

				enterblock(s, stmt_if, 0, jnz, NULL);
			}
			else {
				raiseerror(s, -1, tok->line, "if(<expression>){<statement>};");
				return 0;
			}
			return (node)stmt_if;
		}
		/*else if (tok->type == stmt_ret) {
			tok->kind = opr_def;
			tok->opr.lhs = 0;
			tok->opr.rhs = expr(s);
			//~ printf("[stmt]"); walk(tok,0); printf("\n");
		}*/
	}
	return 0;
}

/*--8<------------------------------------------------------------------------\-
void print_data(state s);
void print_code(state s);

struct state_t s;
node t;

int main() {
	cc_open(&s, "test.c");
	//~ print_defn(&s);
	while ((t = next_token(&s))) {
		back_token(&s, t);
		stmt(&s);
		//~ if (s.errc) break;
	}
	//~ print_defn(&s);
	//~ if (exp[0]) ecp[0]=0;
	//~ walk(s.curtok); printf("\n");
	//~ printf("\ncode size : %d\n", s.buffp - s.buffer);
	//~ printf("%d\n", sizeof(table));
	//~ printf("%d\n", sizeof(struct state_t));
	//~ printf("%d\n", sizeof(struct token_t));
	//~ printf("%d\n", sizeof(struct llist_t));
	//~ printf("%d\n", sizeof(struct defnt_t));

	if (!s.errc) {
		llist list;
		s.data = 256;
		for (list = s.tbeg; list; list = list->next) {		// data : initialized
			symbol defn = (symbol)list->data;
			if (!defn || (defn->kind & tok_msk) != tok_val) {
				raiseerror(&s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
			}
			else if (defn->type == type_str) {
				defn->offs = s.data;
				//~ s.data += defn->size;
				s.data += (strlen(defn->name)+1+3) & ~3;
			}
			else raiseerror(&s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
		}
		for (list = s.dbeg; list; list = list->next) {		// data : uninitialized
			symbol defn = (symbol)list->data;
			if (!defn || (defn->kind & tok_msk) != tok_idf) {
				raiseerror(&s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
			}
			else switch (defn->kind & idf_msk) {
				default : raiseerror(&s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__); break;
				case idf_ref : {
					defn->offs = s.data;
					s.data += ((typesize(defn)+3) & ~3);
				} break;
			}
		}
		printf("buffs : %d\n", s.buffp - s.buffer);
		printf("jumps : %d\n", s.jmps);
		printf("datas : %d\n", s.data-256);
		//~ print_strt(&s);
		//~ print_defn(&s);
		//~ print_data(&s);
		print_code(&s);
	}
	return 0;
}

//--8<------------------------------------------------------------------------*/

void print_data(state s) {
	llist list;
	printf("text :\n");
	for (list = s->tbeg; list; list = list->next) {
		symbol defn = (symbol)list->data;
		if ((defn->kind & tok_msk) != tok_val) {
			raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
		}
		else if (defn->type == type_str) {
			printf("cstr \"%s\" size:%d", defn->name, (int) strlen(defn->name)+1);
		}
		else raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
		printf("\n");
	}
	printf("data :\n");
	for (list = s->dbeg; list; list = list->next) {
		symbol defn = (symbol)list->data;
		if ((defn->kind & tok_msk) != tok_idf) {
			raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__);
		}
		else switch (defn->kind & idf_msk) {
			default : raiseerror(s, -999, 0, "%s:%d: <unhandled exception>", __FILE__, __LINE__); break;
			case idf_ref : {
				printf("name(offs %d, size %d, mods %d) ", defn->size, typesize(defn), defn->kind & 0xff);
				walk(defn->arrs ? defn->arrs : (node)defn, 0, 3);
			} break;
		}
		printf("\n");
	}
}

void print_code(state s) {
	llist list;
	printf("code :\n");
	for (list = s->cbeg; list; list = list->next) {
		node n = (node)list->data;
		//~ printf("sp + %d : ", list->flgs);
		//~ if (n && n->type == stmt_fork) {
			//~ printf("fork");
		//~ }
		walk (n, 0, 2);
		printf("\n");
	}
}
