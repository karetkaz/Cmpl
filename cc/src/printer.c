/*******************************************************************************
 *   File: clog.c
 *   Date: 2011/06/23
 *   Desc: logging & dumping
 *******************************************************************************
 logging:
	ast formatted printing.
	sym formatted printing.
	...
*******************************************************************************/
#include <stdarg.h>
#include <string.h>
#include "internal.h"

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
			*--ptr = (char) plc;
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

/// print qualified name
static void fputqal(FILE* fout, const char *esc[], symn sym) {
	if (sym == NULL) {
		return;
	}
	if (sym->decl != NULL && sym->decl != sym) {
		fputqal(fout, esc, sym->decl);
		fputchr(fout, '.');
	}
	if (sym->name != NULL) {
		fputstr(fout, esc, sym->name);
	}
	else {
		fputstr(fout, esc, "?");
	}
}

/// print array type
static void fputarr(FILE* fout, const char *esc[], symn sym, int mode) {
	if (sym != NULL && sym->kind == CAST_arr) {
		fputarr(fout, esc, sym->type, mode);
		fputfmt(fout, "[%?+t]", sym->init);
		return;
	}
	fputSym(fout, esc, sym, mode, 0);
}

void fputAst(FILE *out, const char *esc[], astn ast, int mode, int indent) {
	static const int exprLevel = -15;
	int pr_attr = mode & prAttr;

	int nlBody = mode & nlAstBody;
	int nlElif = mode & nlAstElIf;
	int oneLine = mode & prOneLine;

	int kind;

	if (ast == NULL) {
		fputstr(out, esc, "(null)");
		return;
	}

	if (indent > 0) {
		if (!oneLine) {
			fputfmt(out, "%I", indent);
		}
	}
	else {
		indent = -indent;
	}

	if (pr_attr && ast->cast) {
		if ((ast->cast & ATTR_stat) != 0) {
			fputstr(out, esc, "static ");
		}
		if ((ast->cast & ATTR_const) != 0) {
			fputstr(out, esc, "const ");
		}
		if ((ast->cast & ATTR_paral) != 0) {
			fputstr(out, esc, "parallel ");
		}
	}

	switch (kind = ast->kind) {
		//#{ STMT
		case STMT_end: {
			if (mode == prName) {
				fputstr(out, esc, ";");
				break;
			}

			fputAst(out, esc, ast->stmt.stmt, mode, exprLevel);
			fputstr(out, esc, ";");
			if (oneLine) {
				break;
			}
			fputstr(out, esc, "\n");
		} break;
		case STMT_beg: {
			astn list;
			if (mode == prName) {
				fputstr(out, esc, "{}");
				break;
			}

			if (oneLine) {
				fputstr(out, esc, "{...}");
				break;
			}

			fputstr(out, esc, "{\n");
			for (list = ast->stmt.stmt; list; list = list->next) {
				fputAst(out, esc, list, mode, indent + 1);
				if (list->kind < STMT_beg || list->kind > STMT_end) {
					// TODO: use declaration statement ?
					if (list->kind == TYPE_def) {
						fputstr(out, esc, ";\n");
					}
					else {
						fputFmt(out, esc, "; /* { expression: %K } */\n", list->kind);
					}
				}
			}
			fputFmt(out, esc, "%I%s", indent, "}\n");
		} break;

		case STMT_if: {
			fputstr(out, esc, "if");
			if (mode == prName) {
				break;
			}

			fputstr(out, esc, " (");
			fputAst(out, esc, ast->stmt.test, mode, exprLevel);
			fputstr(out, esc, ")");

			if (oneLine) {
				break;
			}

			// if then part
			if (ast->stmt.stmt != NULL) {
				int kind = ast->stmt.stmt->kind;
				if (kind == STMT_beg && !nlBody) {
					fputstr(out, esc, " ");
					fputAst(out, esc, ast->stmt.stmt, mode, -indent);
				}
				else {
					fputstr(out, esc, "\n");
					fputAst(out, esc, ast->stmt.stmt, mode, indent + (kind != STMT_beg));
				}
			}
			else {
				fputstr(out, esc, " ;\n");
			}

			// if else part
			if (ast->stmt.step != NULL) {
				int kind = ast->stmt.step->kind;
				if (kind == STMT_if && !nlElif) {
					fputFmt(out, esc, "%Ielse ", indent);
					fputAst(out, esc, ast->stmt.step, mode, -indent);
				}
				else if (kind == STMT_beg && !nlBody) {
					fputFmt(out, esc, "%Ielse ", indent);
					fputAst(out, esc, ast->stmt.step, mode, -indent);
				}
				else {
					fputFmt(out, esc, "%Ielse\n", indent);
					fputAst(out, esc, ast->stmt.step, mode, indent + (kind != STMT_beg));
				}
			}
			break;
		}
		case STMT_for: {
			fputstr(out, esc, "for");
			if (mode == prName) {
				break;
			}

			fputstr(out, esc, " (");
			if (ast->stmt.init) {
				fputAst(out, esc, ast->stmt.init, mode | oneLine, exprLevel);
			}
			else {
				fputstr(out, esc, " ");
			}
			fputstr(out, esc, "; ");
			if (ast->stmt.test) {
				fputAst(out, esc, ast->stmt.test, mode, exprLevel);
			}
			fputstr(out, esc, "; ");
			if (ast->stmt.step) {
				fputAst(out, esc, ast->stmt.step, mode, exprLevel);
			}
			fputstr(out, esc, ")");

			if (oneLine) {
				break;
			}

			if (ast->stmt.stmt != NULL) {
				int kind = ast->stmt.stmt->kind;
				if (kind == STMT_beg && nlBody) {
					fputstr(out, esc, "\n");
					fputAst(out, esc, ast->stmt.stmt, mode, indent + (kind != STMT_beg));
				}
				else {
					fputstr(out, esc, " ");
					fputAst(out, esc, ast->stmt.stmt, mode, -indent);
				}
			}
			else {
				fputstr(out, esc, " ;\n");
			}
			break;
		}
		case STMT_con:
		case STMT_brk: {
			switch (kind) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					return;

				case STMT_con:
					fputstr(out, esc, "continue");
					break;

				case STMT_brk:
					fputstr(out, esc, "break");
					break;
			}

			if (mode == prName) {
				break;
			}
			fputstr(out, esc, ";");
			if (oneLine) {
				break;
			}
			fputstr(out, esc, "\n");
			break;
		}
		case STMT_ret: {
			fputstr(out, esc, "return");
			if (mode == prName) {
				break;
			}
			if (oneLine) {
				fputstr(out, esc, ";");
				break;
			}
			if (ast->stmt.stmt != NULL) {
				astn ret = ast->stmt.stmt;
				fputstr(out, esc, " ");
				// `return 3;` is modified to `return (result = 3);`
				logif(ret->kind != ASGN_set && ret->kind != OPER_fnc, ERR_INTERNAL_ERROR);
				logif(ret->kind == OPER_fnc && ret->type && ret->type->cast != CAST_vid, ERR_INTERNAL_ERROR);
				fputAst(out, esc, ret->op.rhso, mode, exprLevel);
			}
			fputstr(out, esc, ";\n");
			break;
		}
		//#}
		//#{ OPER
		case OPER_fnc: {	// '()'
			if (mode == prName) {
				fputstr(out, esc, "()");
				break;
			}

			if (ast->op.lhso != NULL) {
				fputAst(out, esc, ast->op.lhso, mode, exprLevel);
			}
			fputchr(out, '(');
			if (ast->op.rhso != NULL) {
				fputAst(out, esc, ast->op.rhso, mode, exprLevel);
			}
			fputchr(out, ')');
			break;
		}

		case OPER_idx: {	// '[]'
			if (mode == prName) {
				fputstr(out, esc, "[]");
				break;
			}

			if (ast->op.lhso != NULL) {
				fputAst(out, esc, ast->op.lhso, mode, exprLevel);
			}
			fputchr(out, '[');
			if (ast->op.rhso != NULL) {
				fputAst(out, esc, ast->op.rhso, mode, exprLevel);
			}
			fputchr(out, ']');
			break;
		}

		case OPER_dot: {	// '.'
			if (mode == prName) {
				fputchr(out, '.');
				break;
			}
			fputAst(out, esc, ast->op.lhso, mode, 0);
			fputchr(out, '.');
			fputAst(out, esc, ast->op.rhso, mode, 0);
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

		case OPER_all:		// '&&'
		case OPER_any:		// '||'

		case OPER_com:		// ','

		case ASGN_set: {	// '='
			if (mode == prName) {
				fputstr(out, esc, tok_tbl[kind].name);
				break;
			}
			int precedence = tok_tbl[kind].type & 0x0f;
			int putParen = indent < precedence;

			if (mode & prAstCast && ast->type) {
				fputSym(out, esc, ast->type, prSymQual, 0);
				putParen = 1;
			}

			if (putParen) {
				fputchr(out, '(');
			}

			if (ast->op.lhso != NULL) {
				fputAst(out, esc, ast->op.lhso, mode, -precedence);
				if (kind != OPER_com) {
					fputchr(out, ' ');
				}
			}

			fputstr(out, esc, tok_tbl[kind].name);

			// in case of unary operators: '-a' not '- a'
			if (ast->op.lhso != NULL) {
				fputchr(out, ' ');
			}

			fputAst(out, esc, ast->op.rhso, mode, -precedence);

			if (putParen) {
				fputchr(out, ')');
			}
			break;
		}

		case OPER_sel: {	// '?:'
			if (mode == prName) {
				fputstr(out, esc, tok_tbl[kind].name);
				break;
			}
			fputAst(out, esc, ast->op.test, mode, -OPER_sel);
			fputstr(out, esc, " ? ");
			fputAst(out, esc, ast->op.lhso, mode, -OPER_sel);
			fputstr(out, esc, " : ");
			fputAst(out, esc, ast->op.rhso, mode, -OPER_sel);
			break;
		}
		//#}
		//#{ TVAL
		case CAST_bit:
			fputFmt(out, esc, "%U", ast->cint);
			break;

		case TYPE_int:
			fputFmt(out, esc, "%D", ast->cint);
			break;

		case TYPE_flt:
			fputFmt(out, esc, "%F", ast->cflt);
			break;

		case TYPE_str:
			fputstr(out, esc, "\'");
			fputstr(out, escapeStr(), ast->ref.name);
			fputstr(out, esc, "\'");
			break;

		case TYPE_def:
		case CAST_ref:
			if (ast->ref.link != NULL) {
				if (kind == TYPE_def) {
					fputSym(out, esc, ast->ref.link, mode & ~prSymQual, -indent);
				}
				else {
					fputSym(out, esc, ast->ref.link, prName, -indent);
				}
			}
			else {
				fputstr(out, esc, ast->ref.name);
			}
			break;
		//#}

		default:
			fputstr(out, esc, tok_tbl[kind].name);
			break;
	}
}

void fputSym(FILE *out, const char *esc[], symn sym, int mode, int indent) {
	int oneLine = mode & prOneLine;
	int pr_attr = mode & prAttr;

	int pr_qual = mode & prSymQual;
	int pr_args = mode & prSymArgs;
	int pr_type = mode & prSymType;
	int pr_init = mode & prSymInit;
	int pr_body = !oneLine && pr_init;

	if (sym == NULL) {
		fputstr(out, esc, "(null)");
		return;
	}

	if (indent > 0) {
		fputfmt(out, "%I", indent);
	}
	else {
		indent = -indent;
	}

	if (pr_attr && sym->cast) {
		if ((sym->cast & ATTR_stat) != 0) {
			fputstr(out, esc, "static ");
		}
		if ((sym->cast & ATTR_const) != 0) {
			fputstr(out, esc, "const ");
		}
		if ((sym->cast & ATTR_paral) != 0) {
			fputstr(out, esc, "parallel ");
			fatal(ERR_INTERNAL_ERROR); // symbols can not be parallel
		}
	}

	if (sym->name && *sym->name) {
		if (pr_qual) {
			fputqal(out, esc, sym);
		}
		else {
			fputstr(out, esc, sym->name);
		}
	}

	switch (sym->kind) {

		case EMIT_kwd: {
			if (sym->init) {
				fputfmt(out, "(%t)", sym->init);
			}
			break;
		}

		case CAST_arr: {
			if (sym->name == NULL) {
				fputarr(out, esc, sym, mode);
				return;
			}
			// fall TYPE_rec
		}
		case TYPE_rec: {
			if (pr_body) {
				symn arg;
				fputFmt(out, esc, ": struct %s", "{\n");

				for (arg = sym->flds; arg; arg = arg->next) {
					fputSym(out, esc, arg, mode & ~prSymQual, indent + 1);
					if (arg->kind != TYPE_rec) {		// nested struct
						fputstr(out, esc, ";\n");
					}
				}
				fputFmt(out, esc, "%I%s", indent, "}");
			}
			break;
		}

		case TYPE_def:
		case CAST_ref: {
			if (pr_args && sym->call) {
				symn arg = sym->prms;
				fputchr(out, '(');
				// TODO: check cc->void_tag?
				if (arg && arg->name && *arg->name) {
					for ( ; arg; arg = arg->next) {
						if (arg != sym->prms) {
							fputstr(out, esc, ", ");
						}
						fputSym(out, esc, arg, (mode | prSymType) & ~prSymQual, 0);
					}
				}
				fputchr(out, ')');
			}
			if (pr_type && sym->type) {
				fputstr(out, esc, ": ");
				fputSym(out, esc, sym->type, prName, -indent);
			}
			if (pr_init && sym->init) {
				fputstr(out, esc, " := ");
				fputAst(out, esc, sym->init, prShort, -indent);
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
			char nil = 0;    // [?]? skip on null || zero value (may print pad char once)
			char sgn = 0;    // [+-]? sign flag / alignment.
			char pad = 0;    // [0 ]? padding character.
			int len = 0;     // [0-9]* length / mode
			int prc = 0;     // (\.\*|[0-9]*)? precision / indent
			int noPrc = 1;   // format string has no precision

			const char* fmt = msg - 1;
			char* str = NULL;

			if (*msg == '?') {
				nil = *msg++;
			}
			if (*msg == '+' || *msg == '-') {
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
				noPrc = 0;
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

				case 'T': {		// type name
					symn sym = va_arg(ap, symn);

					if (sym == NULL && nil) {
						if (pad != 0) {
							fputchr(fout, pad);
						}
						continue;
					}

					switch (sgn) {
						default:
							if (noPrc) {
								prc = prShort;
							}
							break;
						case '-':
							len = -len;
							// fall
						case '+':
							if (noPrc) {
								prc = prFull;
							}
							break;
					}
					fputSym(fout, esc, sym, prc, len);
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
							if (noPrc) {
								prc = prShort;
							}
							break;
						case '-':
							len = -len;
							// fall
						case '+':
							if (noPrc) {
								prc = prFull;
							}
							break;
					}
					fputAst(fout, esc, ast, prc, len);
					continue;
				}

				case 'K': {		// symbol kind
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
						arg &= KIND_mask;
						if (arg < tok_last) {
							sprintf(str, "%s%s%s%s", paral, stat, con, tok_tbl[arg].name);
						}
						else {
							sprintf(str, "%s%s%s.ERR_%02x", paral, stat, con, arg);
						}
					}
					break;
				}

				case 'A': {		// instruction
					void* opc = va_arg(ap, void*);

					if (opc == NULL && nil) {
						str = "";
						len = 1;
						break;
					}

					fputAsm(fout, esc, NULL, opc, prc);
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
						buff[len] = 0;
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
		else {
			fputchr(fout, chr);
		}
	}
	fflush(fout);
}

void logFILE(rtContext rt, FILE* file) {
	// close previous opened file
	if (rt->logClose != 0) {
		fclose(rt->logFile);
		rt->logClose = 0;
	}

	rt->logFile = file;
}

int logfile(rtContext rt, char* file, int append) {

	logFILE(rt, NULL);

	if (file != NULL) {
		rt->logFile = fopen(file, append ? "ab" : "wb");
		if (rt->logFile == NULL) {
			return -1;
		}
		rt->logClose = 1;
	}

	return 0;
}

void fputFmt(FILE *out, const char *esc[], const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	FPUTFMT(out, esc, fmt, ap);
	va_end(ap);
}

void fputfmt(FILE*out, const char*fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	FPUTFMT(out, NULL, fmt, ap);
	va_end(ap);
}

void ccError(rtContext rt, int level, const char *file, int line, const char *msg, ...) {
	int warnLevel = rt && rt->cc ? rt->cc->warn : 0;
	FILE *logFile = rt ? rt->logFile : stdout;
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
		rt->errCount += 1;
	}

	if (logFile == NULL) {
		return;
	}

	if (rt && rt->cc && file == NULL) {
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

// dump
void dumpApi(rtContext rt, userContext ctx, void customPrinter(userContext, symn)) {
	symn sym, bp[TOKS], *sp = bp;

	// compilation errors or compiler was not initialized.
	if (rt->cc == NULL || rt->main == NULL) {
		// fatal(ERR_INTERNAL_ERROR);
		return;
	}
	dieif(rt->cc->defs != rt->main->flds, ERR_INTERNAL_ERROR);
	for (*sp = rt->main->flds; sp >= bp;) {
		if (!(sym = *sp)) {
			--sp;
			continue;
		}
		*sp = sym->next;

		switch (sym->kind) {
			// inline/constant
			case TYPE_def:
				/* print params of inline expressions
				if (sym->call && sym->prms) {
					*++sp = sym->prms;
				}// */
				break;

			// array/typename
			case CAST_arr:
			case TYPE_rec:
				*++sp = sym->flds;
				break;

			// variable/function
			case CAST_ref:
				/* print function private members ?
				if (sym->call && sym->flds) {
					*++sp = sym->flds;
				}
				// */
				break;

			case EMIT_kwd:
				*++sp = sym->flds;
				break;

			default:
				fatal(ERR_INTERNAL_ERROR);
				break;
		}

		if (customPrinter != NULL) {
			customPrinter(ctx, sym);
		}
		else {
			FILE *out = rt->logFile;
			fputfmt(out, "%-T: %T\n", sym, sym->type);
			fflush(out);
		}
	}
}
