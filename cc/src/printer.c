/*******************************************************************************
 *   File: clog.c
 *   Date: 2011/06/23
 *   Desc: logging & dumping
 *******************************************************************************
 logging:
	ast formated printig,
	sym formated printig,
	...
*******************************************************************************/
#include <stdarg.h>
#include <string.h>
#include "core.h"

static void fputsym(FILE*, const char *[], symn, int, int);
static inline int fputchr(FILE* stream, int chr) { return fputc(chr, stream); }

static const char** escapeStr() {
	static const char *escape[256];
	static char initialized = 0;
	if (!initialized) {
		memset(escape, 0, sizeof(escape));
		//~ escape['a'] = "`$!>a<!$`";
		escape['\n'] = "\\n";
		escape['\r'] = "\\r";
		escape['\t'] = "\\t";
		escape['\''] = "\\'";
		escape['\"'] = "\\\"";
		initialized = 1;
	}
	return escape;
}

static char* fmtuns(char* dst, int max, int prc, int radix, uint64_t num) {
	char* ptr = dst + max;
	int p = 0, plc = ',';
	*--ptr = 0;
	do {
		if (prc > 0 && ++p > prc) {
			*--ptr = plc;
			p = 1;
		}
		*--ptr = "0123456789abcdef"[num % radix];
	} while (num /= radix);
	return ptr;
}

static void fputstr(FILE* fout, const char *esc[], char* str) {
	if (esc == NULL) {
		fputs(str, fout);
	}
	else {
		int c;
		while ((c = *str++)) {
			if (esc != NULL && esc[c & 0xff] != NULL) {
				fputs(esc[c & 0xff], fout);
			}
			else {
				fputc(c & 0xff, fout);
			}
		}
	}
}

static void fputqal(FILE* fout, const char *esc[], symn sym, int mode) {
	if (sym != NULL && sym->decl != NULL) {
		fputqal(fout, esc, sym->decl, 1);
	}

	fputsym(fout, NULL, sym, 0, 0);
	if (mode != 0) {
		fputchr(fout, '.');
	}
}

static void fputast(FILE* fout, const char *esc[], astn ast, int mode, int level) {
	int nlElse = 1;                 // (' ' | '\n%I') 'else' ...
	int nlBody = mode & nlAstBody;  // (' ' | '\n%I')'{' ...
	int nlElif = mode & nlAstElIf;  // ... 'else' (' ' | '\n%I') 'if' ...
	int rlev = mode & 0xf;

	if (level > 0) {
		fputfmt(fout, "%I", level);
	}
	else {
		level = -level;
	}

	if (ast == NULL) {
		fputstr(fout, esc, "(null)");
		return;
	}

	if (rlev >= 2) {
		if ((ast->cst2 & ATTR_stat) != 0) {
			fputstr(fout, esc, "static ");
		}
		if ((ast->cst2 & ATTR_const) != 0) {
			fputstr(fout, esc, "const ");
		}
		if ((ast->cst2 & ATTR_paral) != 0) {
			fputstr(fout, esc, "parallel ");
		}
	}

	switch (ast->kind) {
		//#{ STMT
		case STMT_do: {
			fputast(fout, esc, ast->stmt.stmt, mode, -level);
		} break;
		case STMT_beg: {
			astn list;
			if (rlev < 2) {
				if (rlev < 1) {
					fputstr(fout, esc, "{");
					break;
				}
				fputstr(fout, esc, "{...}");
				break;
			}

			fputstr(fout, esc, "{\n");
			for (list = ast->stmt.stmt; list; list = list->next) {
				fputast(fout, esc, list, mode, level + 1);
				switch(list->kind) {
					case STMT_if:
					case STMT_for:
					case STMT_beg:
						break;

					default:
						fputstr(fout, esc, ";\n");
						break;
				}
			}
			fputesc(fout, esc, "%I%s", level, "}\n");
		} break;
		case STMT_if: {
			if (rlev == 0) {
				fputstr(fout, esc, "if");
				break;
			}

			fputstr(fout, esc, "if (");
			fputast(fout, esc, ast->stmt.test, mode, -0xf);
			fputstr(fout, esc, ")");

			if (rlev < 2) {
				break;
			}
			// if then part
			if (ast->stmt.stmt != NULL) {
				int kind = ast->stmt.stmt->kind;
				if (kind == STMT_beg && nlBody) {
					fputstr(fout, esc, "\n");
					fputast(fout, esc, ast->stmt.stmt, mode, level + (kind != STMT_beg));
				}
				else {
					fputstr(fout, esc, " ");
					fputast(fout, esc, ast->stmt.stmt, mode, -level);
				}
			}
			else {
				fputstr(fout, esc, ";\n");
			}

			// if else part
			if (ast->stmt.step != NULL) {
				int kind = ast->stmt.step->kind;
				if (nlElse) {
					// we already have this new line
					//~ fputesc(fout, esc, "\n");
				}
				if (kind == STMT_if && nlElif) {
					fputesc(fout, esc, "%I%s\n", level, "else");
					fputast(fout, esc, ast->stmt.step, mode, level + (kind != STMT_beg));
				}
				else if (kind == STMT_beg && nlBody) {
					fputesc(fout, esc, "%I%s\n", level, "else");
					fputast(fout, esc, ast->stmt.step, mode, level + (kind != STMT_beg));
				}
				else {
					fputesc(fout, esc, "%I%s ", level, "else");
					fputast(fout, esc, ast->stmt.step, mode, -level);
				}
			}
			break;
		}
		case STMT_for: {
			if (rlev < 2) {
				fputstr(fout, esc, "for");
				if (rlev > 0) {
					fputesc(fout, esc, " (%?+ t; %?+t; %?+t)", ast->stmt.init, ast->stmt.test, ast->stmt.step);
				}
				break;
			}

			fputstr(fout, esc, "for (");
			if (ast->stmt.init) {
				fputast(fout, esc, ast->stmt.init, 1, -0xf);
			}

			fputstr(fout, esc, "; ");
			if (ast->stmt.test) {
				fputast(fout, esc, ast->stmt.test, 1, -0xf);
			}

			fputstr(fout, esc, "; ");
			if (ast->stmt.step) {
				fputast(fout, esc, ast->stmt.step, 1, -0xf);
			}

			fputstr(fout, esc, ")");

			if (ast->stmt.stmt != NULL) {
				int kind = ast->stmt.stmt->kind;
				if (kind == STMT_beg && nlBody) {
					fputstr(fout, esc, "\n");
					fputast(fout, esc, ast->stmt.stmt, mode, level + (kind != STMT_beg));
				}
				else {
					fputstr(fout, esc, " ");
					fputast(fout, esc, ast->stmt.stmt, mode, -level);
				}
			}
			else {
				fputstr(fout, esc, ";\n");
			}
			break;
		}
		case STMT_con:
		case STMT_brk: {
			if (rlev < 2) {
				switch (ast->kind) {
					default:
						fatal(ERR_INTERNAL_ERROR);
						return;

					case STMT_con:
						fputstr(fout, esc, "continue");
						break;

					case STMT_brk:
						fputstr(fout, esc, "break");
						break;
				}
				break;
			}

			switch (ast->cst2) {
				default:
					traceAst(ast);
				case TYPE_any:
					break;
			}
			switch (ast->kind) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					return;

				case STMT_con:
					fputstr(fout, esc, "continue;\n");
					break;

				case STMT_brk:
					fputstr(fout, esc, "break;\n");
					break;
			}
			break;
		}
		case STMT_ret: {
			fputstr(fout, esc, "return");
			if (rlev > 0 && ast->stmt.stmt) {
				astn ret = ast->stmt.stmt;
				fputstr(fout, esc, " ");
				// `return 3;` is modified to `return (result = 3);`
				logif(ret->kind != ASGN_set && ret->kind != OPER_fnc, ERR_INTERNAL_ERROR);
				logif(ret->kind == OPER_fnc && ret->type && ret->type->cast != TYPE_vid, ERR_INTERNAL_ERROR);
				fputast(fout, esc, ret->op.rhso, mode, -0xf);
			}
			break;
		}
		case STMT_end:
			fputstr(fout, esc, "}");
			if (rlev > 1) {
				// TODO: print list of variables to be freed.
				fputstr(fout, esc, "// delete [...]");
			}
			break;
		//#}
		//#{ OPER
		case OPER_fnc: {	// '()'
			if (rlev > 0) {
				if (ast->op.lhso != NULL) {
					fputast(fout, esc, ast->op.lhso, mode, 0);
				}
				fputchr(fout, '(');
				if (rlev && ast->op.rhso != NULL) {
					fputast(fout, esc, ast->op.rhso, mode, -0xf);
				}
				fputchr(fout, ')');
			}
			else {
				fputstr(fout, esc, "()");
			}
			break;
		}

		case OPER_idx: {	// '[]'
			if (rlev > 0) {
				fputast(fout, esc, ast->op.lhso, mode, 0);
				fputchr(fout, '[');
				if (rlev && ast->op.rhso!= NULL) {
					fputast(fout, esc, ast->op.rhso, mode, -0xf);
				}
				fputchr(fout, ']');
			}
			else {
				fputstr(fout, esc, "[]");
			}
			break;
		}

		case OPER_dot: {	// '.'
			if (rlev > 0) {
				fputast(fout, esc, ast->op.lhso, mode, 0);
				fputchr(fout, '.');
				fputast(fout, esc, ast->op.rhso, mode, 0);
			}
			else {
				fputchr(fout, '.');
			}
			break;
		}

		case OPER_adr:		// '&'
		case OPER_pls:		// '+'
		case OPER_mns:		// '-'
		case OPER_cmt:		// '~'
		case OPER_not:		// '!'

		case OPER_add:		// '+'
		case OPER_sub:		// '-'
		case OPER_mul:		// '*'
		case OPER_div:		// '/'
		case OPER_mod:		// '%'

		case OPER_shl:		// '>>'
		case OPER_shr:		// '<<'
		case OPER_and:		// '&'
		case OPER_ior:		// '|'
		case OPER_xor:		// '^'

		case OPER_equ:		// '=='
		case OPER_neq:		// '!='
		case OPER_lte:		// '<'
		case OPER_leq:		// '<='
		case OPER_gte:		// '>'
		case OPER_geq:		// '>='

		case OPER_lnd:		// '&&'
		case OPER_lor:		// '||'

		case ASGN_set: {	// '='
			if (rlev > 0) {
				int pre = tok_tbl[ast->kind].type & 0x0f;
				int putparen = level < pre;

				if (mode & prAstCast) {
					fputsym(fout, esc, ast->type, prSymQual, 0);
					putparen = 1;
				}

				if (putparen) {
					fputchr(fout, '(');
				}

				if (ast->op.lhso != NULL) {
					fputast(fout, esc, ast->op.lhso, mode, -pre);
					fputchr(fout, ' ');
				}

				fputast(fout, esc, ast, 0, 0);

				// in case of unary operators: '-a' not '- a'
				if (ast->op.lhso != NULL) {
					fputchr(fout, ' ');
				}

				fputast(fout, esc, ast->op.rhso, mode, -pre);

				if (putparen) {
					fputchr(fout, ')');
				}
			}
			else switch (ast->kind) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					return;
				case OPER_dot: fputstr(fout, esc, "."); break;		// '.'
				case OPER_adr: fputstr(fout, esc, "&"); break;		// '&'
				case OPER_pls: fputstr(fout, esc, "+"); break;		// '+'
				case OPER_mns: fputstr(fout, esc, "-"); break;		// '-'
				case OPER_cmt: fputstr(fout, esc, "~"); break;		// '~'
				case OPER_not: fputstr(fout, esc, "!"); break;		// '!'

				case OPER_shr: fputstr(fout, esc, ">>"); break;		// '>>'
				case OPER_shl: fputstr(fout, esc, "<<"); break;		// '<<'
				case OPER_and: fputstr(fout, esc, "&"); break;		// '&'
				case OPER_ior: fputstr(fout, esc, "|"); break;		// '|'
				case OPER_xor: fputstr(fout, esc, "^"); break;		// '^'

				case OPER_equ: fputstr(fout, esc, "=="); break;		// '=='
				case OPER_neq: fputstr(fout, esc, "!="); break;		// '!='
				case OPER_lte: fputstr(fout, esc, "<"); break;		// '<'
				case OPER_leq: fputstr(fout, esc, "<="); break;		// '<='
				case OPER_gte: fputstr(fout, esc, ">"); break;		// '>'
				case OPER_geq: fputstr(fout, esc, ">="); break;		// '>='

				case OPER_add: fputstr(fout, esc, "+"); break;		// '+'
				case OPER_sub: fputstr(fout, esc, "-"); break;		// '-'
				case OPER_mul: fputstr(fout, esc, "*"); break;		// '*'
				case OPER_div: fputstr(fout, esc, "/"); break;		// '/'
				case OPER_mod: fputstr(fout, esc, "%"); break;		// '%'

				case OPER_lnd: fputstr(fout, esc, "&&"); break;		// '&'
				case OPER_lor: fputstr(fout, esc, "||"); break;		// '|'

				case ASGN_set: fputstr(fout, esc, "="); break;		// ':='
			}
			break;
		}

		case OPER_com: {
			if (rlev > 0) {
				fputast(fout, esc, ast->op.lhso, mode, -OPER_com);
				fputstr(fout, esc, ", ");
				fputast(fout, esc, ast->op.rhso, mode, -OPER_com);
			}
			else {
				fputchr(fout, ',');
			}
			break;
		}

		case OPER_sel: {	// '?:'
			if (rlev > 0) {
				fputast(fout, esc, ast->op.test, mode, -OPER_sel);
				fputstr(fout, esc, " ? ");
				fputast(fout, esc, ast->op.lhso, mode, -OPER_sel);
				fputstr(fout, esc, " : ");
				fputast(fout, esc, ast->op.rhso, mode, -OPER_sel);
			}
			else {
				fputstr(fout, esc, "?:");
			}
			break;
		}
		//#}
		//#{ TVAL
		case EMIT_opc:
			fputstr(fout, esc, "emit");
			break;

		case TYPE_bit:
			fputesc(fout, esc, "%U", ast->cint);
			break;

		case TYPE_int:
			fputesc(fout, esc, "%D", ast->cint);
			break;

		case TYPE_flt:
			fputesc(fout, esc, "%F", ast->cflt);
			break;

		case TYPE_str:
			fputstr(fout, esc, "\'");
			fputstr(fout, escapeStr(), ast->ref.name);
			fputstr(fout, esc, "\'");
			break;

		case TYPE_def: {
			if (ast->ref.link) {
				fputsym(fout, esc, ast->ref.link, mode| prSymInit |prType, -level);
			}
			else {
				fputstr(fout, esc, ast->ref.name);
			}
			break;
		}
		case TYPE_ref: {
			if (ast->ref.link) {
				fputsym(fout, esc, ast->ref.link, 0, 0);
			}
			else {
				fputstr(fout, esc, ast->ref.name);
			}
			break;
		}
		//#}

		case PNCT_lc:
			fputchr(fout, '[');
			break;

		case PNCT_rc:
			fputchr(fout, ']');
			break;

		case PNCT_lp:
			fputchr(fout, '(');
			break;

		case PNCT_rp:
			fputchr(fout, ')');
			break;

		default:
			fputesc(fout, esc, "%K(0x%02x)", ast->kind, ast->kind);
			break;
	}
}

static void fputsym(FILE* fout, const char *esc[], symn sym, int mode, int level) {
	int pr_type = mode & prType;
	int pr_init = mode & prSymInit;
	int pr_qual = mode & prSymQual;
	int rlev = mode & 0xf;

	if (level > 0) {
		fputfmt(fout, "%I", level);
	}
	else {
		level = -level;
	}

	if (sym == NULL) {
		fputstr(fout, esc, "(null)");
		return;
	}

	switch (sym->kind) {

		case EMIT_opc: {
			if (sym->name) {
				if (pr_qual && sym->decl) {
					fputsym(fout, esc , sym->decl, mode, 0);
					fputchr(fout, '.');
				}
				fputstr(fout, esc, sym->name);
			}

			if (sym->init) {
				fputfmt(fout, "(%+t)", sym->init);
			}

			break;
		}

		case TYPE_arr: {
			if (sym->name == NULL) {
				symn bp[TBLS], *sp = bp, *p;

				symn typ = sym;
				while (typ->kind == TYPE_arr) {
					*sp++ = typ;
					typ = typ->type;
				}
				fputsym(fout, esc, typ, mode, -level);

				for (p = bp; p < sp; ++p) {
					typ = *p;
					fputfmt(fout, "[%?+t]", typ->init);
				}
				return;
			}
			// fall TYPE_rec
		}
		case TYPE_rec: {
			symn arg;
			if (rlev < 2) {
				if (pr_qual && sym->decl) {
					fputqal(fout, esc, sym->decl, 1);
				}
				fputstr(fout, esc, sym->name);
				break;
			}

			fputesc(fout, esc, "struct %?s {\n", sym->name);

			for (arg = sym->flds; arg; arg = arg->next) {
				fputsym(fout, esc, arg, mode & ~prSymQual, level + 1);
				if (arg->kind != TYPE_rec) {		// nested struct
					fputstr(fout, esc, ";\n");
				}
			}

			fputesc(fout, esc, "%I}\n", level);

			break;
		}

		case TYPE_def:
		case TYPE_ref: {
			if (rlev < 1) {
				fputstr(fout, esc, sym->name);
				break;
			}

			if (pr_type) switch (sym->kind) {
				case TYPE_rec:
					fputstr(fout, esc, "struct ");
					break;

				default:
					if (sym->type) {
						fputsym(fout, esc, sym->type, pr_qual, -1);
						if (sym->name && *sym->name) {
							fputchr(fout, ' ');
						}
						switch (sym->cast) {
							default:
								break;

							// value type by reference
							case TYPE_ref:
								if (!sym->call && sym->type->cast != TYPE_ref)
									fputchr(fout, '&');
								break;

							//~ case TYPE_def:
								//~ fputchr(fout, '&&');
								//~ break;
						}
					}
					break;
			}

			if (sym->name && *sym->name) {
				if (pr_qual && sym->decl) {
					fputsym(fout, esc, sym->decl, prSymQual, 0);
					fputchr(fout, '.');
				}
				fputstr(fout, esc, sym->name);
			}

			if (rlev > 0 && sym->call) {
				symn arg = sym->prms;
				fputchr(fout, '(');
				while (arg) {
					fputsym(fout, esc, arg, prType | 1, 0);
					if ((arg = arg->next))
						fputstr(fout, esc, ", ");
				}
				fputchr(fout, ')');
			}

			if (pr_init && sym->init) {
				fputstr(fout, esc, " = ");
				fputast(fout, esc, sym->init, 1, 0);
			}

			break;
		}

		default:
			fatal(ERR_INTERNAL_ERROR);
			return;
	}
}

static void FPUTFMT(FILE* fout, const char *esc[], const char* msg, va_list ap) {
	char buff[1024], chr;

	while ((chr = *msg++)) {
		if (chr == '%') {
			char  nil = 0;    // [?]? skip on null || zero value
			char  sgn = 0;    // [+-]?
			char  pad = 0;    // 0?
			long  len = 0;    // ([0-9]*)? length / indent
			long  prc = -1;   // (.('*')|([0-9])*)? precision / mode
			char* str = NULL; // the string to be printed

			const char*	fmt = msg - 1;		// start of format string

			if (*msg == '?') {
				nil = *msg++;
			}
			if (*msg == '-' || *msg == '+') {
				sgn = *msg++;
			}
			if (*msg == '0' || *msg == ' ') {
				pad = *msg++;
			}

			while (*msg >= '0' && *msg <= '9') {
				len = (len * 10) + (*msg - '0');
				msg++;
			}

			if (*msg == '.') {
				msg++;
				if (*msg == '*') {
					prc = va_arg(ap, int32_t);
					msg++;
				}
				else {
					prc = 0;
					while (*msg >= '0' && *msg <= '9') {
						prc = (prc * 10) + (*msg - '0');
						msg++;
					}
				}
			}

			switch (chr = *msg++) {
				case 0:
					msg -= 1;
					continue;

				default:
					fputchr(fout, chr);
					continue;

				case 'T': {		// type node
					symn sym = va_arg(ap, symn);

					if (sym == NULL && nil) {
						if (pad != 0) {
							fputchr(fout, pad);
						}
						continue;
					}

					if (len < 0) {
						len = 0;
					}

					switch (sgn) {
						default:
							break;
						case '-':
							len = -len;
							/*if (prc < 0) {
								prc = prType | prSymQual | 1;
							}
							break;*/
						case '+':
							if (prc < 0) {
								prc = prSymQual | 1;
							}
							break;
					}
					if (prc < 0) {
						prc = 1;
					}
					fputsym(fout, esc, sym, prc, len);
					continue;
				}

				case 't': {		// tree node
					astn ast = va_arg(ap, astn);

					if (ast == NULL && nil) {
						if (pad != 0) {
							fputchr(fout, pad);
						}
						continue;
					}

					switch (sgn) {
						default:
							break;
						case '-':
							len = -len;
							// fall
						case '+':
							if (prc < 0) {
								prc = 2;
							}
							break;
					}
					if (prc < 0) {
						prc = 1;
					}
					fputast(fout, esc, ast, prc, len);
					continue;
				}

				case 'K': {		// token kind
					unsigned arg = va_arg(ap, unsigned);

					if (arg == 0 && nil) {
						if (pad != 0) {
							fputchr(fout, pad);
						}
						continue;
					}

					if (arg < tok_last) {
						str = tok_tbl[arg].name;
					}
					else {
						char *con = arg & ATTR_const ? "const " : "";
						char *stat = arg & ATTR_stat ? "static " : "";
						char *paral = arg & ATTR_paral ? "parallel " : "";
						str = buff;
						arg &= 0xff;
						if (arg < tok_last) {
							sprintf(str, "%s%s%s%s", paral, stat, con, tok_tbl[arg].name);
						}
						else {
							sprintf(str, "%s%s%s.ERR_%02x", paral, stat, con, arg);
						}
					}
					break;
				}

				case 'A': {		// opcode
					void* opc = va_arg(ap, void*);

					if (opc == NULL && nil) {
						str = "";
						len = 1;
						break;
					}

					fputasm(fout, opc, len, prc, NULL);
					continue;
				}

				case 'I': {		// indent
					unsigned arg = va_arg(ap, unsigned);
					len = len ? len * arg : arg;
					if (pad == 0) {
						pad = '\t';
					}
					str = (char*)"";
					break;
				}

				case 'b': {		// bin32
					uint32_t num = va_arg(ap, int32_t);
					if (num == 0 && nil) {
						len = pad != 0;
						str = "";
						break;
					}
					str = fmtuns(buff, sizeof(buff), prc, 2, num);
					break;
				}

				case 'B': {		// bin64
					uint64_t num = va_arg(ap, int64_t);
					if (num == 0 && nil) {
						len = pad != 0;
						str = "";
						break;
					}
					str = fmtuns(buff, sizeof(buff), prc, 2, num);
					break;
				}

				case 'o': {		// oct32
					uint32_t num = va_arg(ap, int32_t);
					if (num == 0 && nil) {
						len = pad != 0;
						str = "";
						break;
					}
					str = fmtuns(buff, sizeof(buff), prc, 8, num);
					break;
				}

				case 'O': {		// oct64
					uint64_t num = va_arg(ap, int64_t);
					if (num == 0 && nil) {
						len = pad != 0;
						str = "";
						break;
					}
					str = fmtuns(buff, sizeof(buff), prc, 8, num);
					break;
				}

				case 'x': {		// hex32
					uint32_t num = va_arg(ap, int32_t);
					if (num == 0 && nil) {
						len = pad != 0;
						str = "";
						break;
					}
					str = fmtuns(buff, sizeof(buff), prc, 16, num);
					break;
				}

				case 'X': {		// hex64
					uint64_t num = va_arg(ap, int64_t);
					if (num == 0 && nil) {
						len = pad != 0;
						str = "";
						break;
					}
					str = fmtuns(buff, sizeof(buff), prc, 16, num);
					break;
				}

				case 'u':		// uint32
				case 'd': {		// dec32
					int neg = 0;
					uint32_t num = va_arg(ap, int32_t);

					if (num == 0 && nil) {
						len = pad != 0;
						str = "";
						break;
					}

					if (chr == 'd' && (int32_t)num < 0) {
						num = (uint32_t) 0 -num;
						neg = -1;
					}

					str = fmtuns(buff, sizeof(buff), prc, 10, num);
					if (neg) {
						*--str = '-';
					}
					else if (sgn == '+') {
						*--str = '+';
					}
					break;
				}

				case 'U':		// uint64
				case 'D': {		// dec64
					int neg = 0;
					uint64_t num = va_arg(ap, int64_t);

					if (num == 0 && nil) {
						len = pad != 0;
						str = "";
						break;
					}

					if (chr == 'D' && (int64_t)num < 0) {
						num = (uint64_t) 0 -num;
						neg = -1;
					}
					str = fmtuns(buff, sizeof(buff), prc, 10, num);
					if (neg) {
						*--str = '-';
					}
					else if (sgn == '+') {
						*--str = '+';
					}
					break;
				}

				case 'E':		// float64
				case 'F':		// float64
					chr -= 'A' - 'a';
					// no break
				case 'e':		// float32
				case 'f': {		// float32
					float64_t num = va_arg(ap, float64_t);
					if (num == 0 && nil) {
						len = pad != 0;
						str = "";
						break;
					}

					if ((len = msg - fmt - 1) < 1020) {
						memcpy(buff, fmt, (size_t) len);
						if (buff[1] == '?') {
							len -= 1;
							memcpy(buff+1, buff+2, (size_t) len - 1);
						}
						if (buff[1] == '-' || buff[1] == '+') {
							len -= 1;
							memcpy(buff+1, buff+2, (size_t) len - 1);
							if (num >= 0) {
								fprintf(fout, "+");
							}
						}
						buff[len++] = chr;
						buff[len++] = 0;
						//~ TODO: replace fprintf
						fprintf(fout, buff, num);
					}
					continue;
				}

				case 's':		// string
					str = va_arg(ap, char*);
					if (str == NULL && nil) {
						len = pad != 0;
						str = "";
						break;
					}
					break;

				case 'c':		// char
					buff[0] = va_arg(ap, int);
					buff[1] = 0;
					str = buff;
					break;
			}

			if (str != NULL) {
				len -= strlen(str);

				if (pad == 0) {
					pad = ' ';
				}

				while (len > 0) {
					fputchr(fout, pad);
					len -= 1;
				}

				fputstr(fout, esc, str);
			}
		}
		/*else if (esc && esc[chr & 255]) {
			fputstr(fout, NULL, esc[chr & 255]);
		}*/
		else {
			fputchr(fout, chr);
		}
	}
	fflush(fout);
}

/*static void dumpxml(FILE* fout, astn ast, int mode, int level, const char* text) {
	static char** escapeXml() {
		static char *escape[256];
		static char initialized = 0;
		if (!initialized) {
			memset(escape, 0, sizeof(escape));
			escape['"'] = "&quot;";
			escape['\''] = "&apos;";
			escape['<'] = "&lt;";
			escape['>'] = "&gt;";
			escape['&'] = "&amp;";
			initialized = 1;
		}
		return escape;
	}
	char **escape = escapeXml();
	if (ast == NULL) {
		return;
	}

	fputesc(fout, escape, "%I<%s op=\"%K\"", level, text, ast->kind);

	if ((mode & prType) != 0) {
		fputesc(fout, escape, " type=\"%?T\"", ast->type);
	}

	if ((mode & prAstCast) != 0) {
		fputesc(fout, escape, " cast=\"%?K\"", ast->cst2);
	}

	if ((mode & prXMLLine) != 0) {
		if (ast->file != NULL && ast->line > 0) {
			fputesc(fout, escape, " file=\"%s:%d\"", ast->file, ast->line);
		}
	}

	switch (ast->kind) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return;

		//#{ STMT
		case STMT_do:
			fputesc(fout, escape, " stmt=\"%?+t\">\n", ast);
			dumpxml(fout, ast->stmt.stmt, mode, level + 1, "expr");
			fputesc(fout, escape, "%I</%s>\n", level, text);
			break;

		case STMT_beg: {
			astn list;
			fputesc(fout, escape, ">\n");
			for (list = ast->stmt.stmt; list; list = list->next) {
				dumpxml(fout, list, mode, level + 1, "stmt");
			}
			fputesc(fout, escape, "%I</%s>\n", level, text);
			break;
		}

		case STMT_if:
			fputesc(fout, escape, " stmt=\"%?+t\">\n", ast);
			dumpxml(fout, ast->stmt.test, mode, level + 1, "test");
			dumpxml(fout, ast->stmt.stmt, mode, level + 1, "then");
			dumpxml(fout, ast->stmt.step, mode, level + 1, "else");
			fputesc(fout, escape, "%I</%s>\n", level, text);
			break;

		case STMT_for:
			fputesc(fout, escape, " stmt=\"%?+t\">\n", ast);
			dumpxml(fout, ast->stmt.init, mode, level + 1, "init");
			dumpxml(fout, ast->stmt.test, mode, level + 1, "test");
			dumpxml(fout, ast->stmt.step, mode, level + 1, "step");
			dumpxml(fout, ast->stmt.stmt, mode, level + 1, "stmt");
			fputesc(fout, escape, "%I</%s>\n", level, text);
			break;

		case STMT_con:
		case STMT_brk:
			fputesc(fout, escape, " />\n");
			break;

		case STMT_ret:
			fputesc(fout, escape, " stmt=\"%?+t\">\n", ast);
			dumpxml(fout, ast->stmt.stmt, mode, level + 1, "expr");
			fputesc(fout, escape, "%I</%s>\n", level, text);
			break;

		//#}
		//#{ OPER
		case OPER_fnc: {	// '()'
			astn args = ast->op.rhso;
			fputesc(fout, escape, ">\n");
			if (args != NULL) {
				while (args && args->kind == OPER_com) {
					dumpxml(fout, args->op.rhso, mode, level + 1, "push");
					args = args->op.lhso;
				}
				dumpxml(fout, args, mode, level + 1, "push");
			}
			dumpxml(fout, ast->op.lhso, mode, level + 1, "call");
			fputesc(fout, escape, "%I</%s>\n", level, text);
			break;
		}

		case OPER_dot:		// '.'
		case OPER_idx:		// '[]'

		case OPER_adr:		// '&'
		case OPER_pls:		// '+'
		case OPER_mns:		// '-'
		case OPER_cmt:		// '~'
		case OPER_not:		// '!'

		case OPER_add:		// '+'
		case OPER_sub:		// '-'
		case OPER_mul:		// '*'
		case OPER_div:		// '/'
		case OPER_mod:		// '%'

		case OPER_shl:		// '>>'
		case OPER_shr:		// '<<'
		case OPER_and:		// '&'
		case OPER_ior:		// '|'
		case OPER_xor:		// '^'

		case OPER_equ:		// '=='
		case OPER_neq:		// '!='
		case OPER_lte:		// '<'
		case OPER_leq:		// '<='
		case OPER_gte:		// '>'
		case OPER_geq:		// '>='

		case OPER_lnd:		// '&&'
		case OPER_lor:		// '||'
		case OPER_sel:		// '?:'

		case OPER_com:		// ','

		case ASGN_set:		// '='
			fputesc(fout, escape, " oper=\"%?+t\">\n", ast);
			dumpxml(fout, ast->op.test, mode, level + 1, "test");
			dumpxml(fout, ast->op.lhso, mode, level + 1, "lval");
			dumpxml(fout, ast->op.rhso, mode, level + 1, "rval");
			fputesc(fout, escape, "%I</%s>\n", level, text);
			break;

		//#}
		//#{ TVAL
		case EMIT_opc:
			fputesc(fout, escape, " />\n", text);
			break;

		case TYPE_int:
		case TYPE_flt:
		case TYPE_str:
			fputesc(fout, escape, " value=\"%+t\" />\n", ast);
			break;

		case TYPE_ref:
			fputesc(fout, escape, ">%T</%s>\n", ast->ref.link, text);
			break;

		case TYPE_def: {
			symn var = ast->ref.link;
			astn init = var ? var->init : NULL;
			symn fields = var ? var->flds : NULL;

			fputesc(fout, escape, " tyc=\"%?K\" name=\"%+T\"", var ? var->cast : TYPE_any, var);
			if (fields || init) {
				fputesc(fout, escape, ">\n");
			}
			else {
				fputesc(fout, escape, " />\n");
			}

			if (var && fields) {
				symn def;
				char *argn = "def";

				switch (var->type->kind) {
					case TYPE_arr:
					case TYPE_rec:
						argn = "base";
						break;
					default:
						break;
				}

				if (var->call || var->type->call) {
					argn = "argn";
				}

				for (def = fields; def; def = def->next) {
					fputesc(fout, escape, "%I<%s op=\"%K\"", level + 1, argn, def->kind, def);
					if (mode & prType) {
						fputesc(fout, escape, " type=\"%?T\"", def->type);
					}
					if (mode & prAstCast) {
						fputesc(fout, escape, " cast=\"%?K\"", def->cast);
					}
					if (mode & prXMLLine) {
						// && def->file && def->line) {
						fputesc(fout, escape, " line=\"%s:%d\"", def->file, def->line);
					}
					fputesc(fout, escape, " name=\"%+T\"", def);

					if (def->init) {
						fputesc(fout, escape, " value=\"%+t\"", def->init);
					}
					fputesc(fout, escape, " />\n");
				}
			}

			if (init) {
				dumpxml(fout, var->init, mode, level + 1, "init");
			}

			if (fields || init) {
				fputesc(fout, escape, "%I</%s>\n", level, text);
			}
			break;
		}
		//#}
	}
}// */


void logFILE(state rt, FILE* file) {

	if (rt->closelog != 0) {
		fclose(rt->logf);
		rt->closelog = 0;
	}

	rt->logf = file;
}

int logfile(state rt, char* file, int append) {

	logFILE(rt, NULL);

	if (file != NULL) {
		rt->logf = fopen(file, append ? "ab" : "wb");
		if (rt->logf == NULL) {
			return -1;
		}
		rt->closelog = 1;
	}

	return 0;
}

void fputfmt(FILE* fout, const char* msg, ...) {
	va_list ap;
	va_start(ap, msg);
	FPUTFMT(fout, NULL, msg, ap);
	va_end(ap);
}

void fputesc(FILE* fout, const char *esc[], const char* msg, ...) {
	va_list ap;
	va_start(ap, msg);
	FPUTFMT(fout, esc, msg, ap);
	va_end(ap);
}

void perr(state rt, int level, const char* file, int line, const char* msg, ...) {
	int warnLevel = rt && rt->cc ? rt->cc->warn : 0;
	FILE *logFile = rt ? rt->logf : stdout;
	const char **esc = NULL;

	va_list argp;

	if (level >= 0) {
		if (warnLevel < 0) {
			level = warnLevel;
		}
		if (level > warnLevel) {
			return;
		}
	}
	else if (rt != NULL) {
		rt->errc += 1;
	}

	if (logFile == NULL) {
		return;
	}

	if (rt && rt->cc && !file) {
		file = rt->cc->file;
	}

	if (file != NULL && line > 0) {
		fputfmt(logFile, "%s:%u: ", file, line);
	}

	if (level > 0) {
		fputfmt(logFile, "warning[%d]: ", level);
	}
	else if (level) {
		fputstr(logFile, esc, "error: ");
	}

	va_start(argp, msg);
	FPUTFMT(logFile, NULL, msg, argp);
	fputchr(logFile, '\n');
	fflush(logFile);
	va_end(argp);
}
