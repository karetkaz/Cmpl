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

/// print qualified name
static void fputqal(FILE* fout, const char *esc[], symn sym) {
	if (sym == NULL) {
		return;
	}
	if (sym->decl != NULL) {
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
	if (sym != NULL && sym->kind == TYPE_arr) {
		fputarr(fout, esc, sym->type, mode);
		fputfmt(fout, "[%?+t]", sym->init);
		return;
	}
	fputSym(fout, esc, sym, mode, 0);
}

void fputAst(FILE *out, const char *esc[], astn ast, int mode, int indent) {
	int nlElse = 1;                 // (' ' | '\n%I') 'else' ...
	int nlBody = mode & nlAstBody;  // (' ' | '\n%I')'{' ...
	int nlElif = mode & nlAstElIf;  // ... 'else' (' ' | '\n%I') 'if' ...
	int rlev = mode & 0xf;

	if (ast == NULL) {
		fputstr(out, esc, "(null)");
		return;
	}

	if (indent > 0) {
		fputfmt(out, "%I", indent);
	}
	else {
		indent = -indent;
	}

	if (rlev >= 2) {
		if ((ast->cst2 & ATTR_stat) != 0) {
			fputstr(out, esc, "static ");
		}
		if ((ast->cst2 & ATTR_const) != 0) {
			fputstr(out, esc, "const ");
		}
		if ((ast->cst2 & ATTR_paral) != 0) {
			fputstr(out, esc, "parallel ");
		}
	}

	switch (ast->kind) {
		//#{ STMT
		case STMT_end: {
			fputAst(out, esc, ast->stmt.stmt, mode, -indent);
		} break;
		case STMT_beg: {
			astn list;
			if (rlev < 2) {
				if (rlev < 1) {
					fputstr(out, esc, "{");
					break;
				}
				fputstr(out, esc, "{...}");
				break;
			}

			fputstr(out, esc, "{\n");
			for (list = ast->stmt.stmt; list; list = list->next) {
				fputAst(out, esc, list, mode, indent + 1);
				switch(list->kind) {
					case STMT_if:
					case STMT_for:
					case STMT_beg:
						break;

					default:
						fputstr(out, esc, ";\n");
						break;
				}
			}
			fputFmt(out, esc, "%I%s", indent, "}\n");
		} break;
		case STMT_if: {
			if (rlev == 0) {
				fputstr(out, esc, "if");
				break;
			}

			fputstr(out, esc, "if (");
			fputAst(out, esc, ast->stmt.test, mode, -0xf);
			fputstr(out, esc, ")");

			if (rlev < 2) {
				break;
			}
			// if then part
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
				fputstr(out, esc, ";\n");
			}

			// if else part
			if (ast->stmt.step != NULL) {
				int kind = ast->stmt.step->kind;
				if (nlElse) {
					// we already have this new line
					//~ fputFmt(out, esc, "\n");
				}
				if (kind == STMT_if && nlElif) {
					fputFmt(out, esc, "%I%s\n", indent, "else");
					fputAst(out, esc, ast->stmt.step, mode, indent + (kind != STMT_beg));
				}
				else if (kind == STMT_beg && nlBody) {
					fputFmt(out, esc, "%I%s\n", indent, "else");
					fputAst(out, esc, ast->stmt.step, mode, indent + (kind != STMT_beg));
				}
				else {
					fputFmt(out, esc, "%I%s ", indent, "else");
					fputAst(out, esc, ast->stmt.step, mode, -indent);
				}
			}
			break;
		}
		case STMT_for: {
			if (rlev < 2) {
				fputstr(out, esc, "for");
				if (rlev > 0) {
					fputFmt(out, esc, " (%?+ t; %?+t; %?+t)", ast->stmt.init, ast->stmt.test, ast->stmt.step);
				}
				break;
			}

			fputstr(out, esc, "for (");
			if (ast->stmt.init) {
				fputAst(out, esc, ast->stmt.init, 1, -0xf);
			}

			fputstr(out, esc, "; ");
			if (ast->stmt.test) {
				fputAst(out, esc, ast->stmt.test, 1, -0xf);
			}

			fputstr(out, esc, "; ");
			if (ast->stmt.step) {
				fputAst(out, esc, ast->stmt.step, 1, -0xf);
			}

			fputstr(out, esc, ")");

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
				fputstr(out, esc, ";\n");
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
						fputstr(out, esc, "continue");
						break;

					case STMT_brk:
						fputstr(out, esc, "break");
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
					fputstr(out, esc, "continue;\n");
					break;

				case STMT_brk:
					fputstr(out, esc, "break;\n");
					break;
			}
			break;
		}
		case STMT_ret: {
			fputstr(out, esc, "return");
			if (rlev > 0 && ast->stmt.stmt) {
				astn ret = ast->stmt.stmt;
				fputstr(out, esc, " ");
				// `return 3;` is modified to `return (result = 3);`
				logif(ret->kind != ASGN_set && ret->kind != OPER_fnc, ERR_INTERNAL_ERROR);
				logif(ret->kind == OPER_fnc && ret->type && ret->type->cast != TYPE_vid, ERR_INTERNAL_ERROR);
				fputAst(out, esc, ret->op.rhso, mode, -0xf);
			}
			break;
		}
		//#}
		//#{ OPER
		case OPER_fnc: {	// '()'
			if (rlev > 0) {
				if (ast->op.lhso != NULL) {
					fputAst(out, esc, ast->op.lhso, mode, 0);
				}
				fputchr(out, '(');
				if (rlev && ast->op.rhso != NULL) {
					fputAst(out, esc, ast->op.rhso, mode, -0xf);
				}
				fputchr(out, ')');
			}
			else {
				fputstr(out, esc, "()");
			}
			break;
		}

		case OPER_idx: {	// '[]'
			if (rlev > 0) {
				fputAst(out, esc, ast->op.lhso, mode, 0);
				fputchr(out, '[');
				if (rlev && ast->op.rhso!= NULL) {
					fputAst(out, esc, ast->op.rhso, mode, -0xf);
				}
				fputchr(out, ']');
			}
			else {
				fputstr(out, esc, "[]");
			}
			break;
		}

		case OPER_dot: {	// '.'
			if (rlev > 0) {
				fputAst(out, esc, ast->op.lhso, mode, 0);
				fputchr(out, '.');
				fputAst(out, esc, ast->op.rhso, mode, 0);
			}
			else {
				fputchr(out, '.');
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
				int putparen = indent < pre;

				if (mode & prAstCast) {
					fputSym(out, esc, ast->type, prSymQual, 0);
					putparen = 1;
				}

				if (putparen) {
					fputchr(out, '(');
				}

				if (ast->op.lhso != NULL) {
					fputAst(out, esc, ast->op.lhso, mode, -pre);
					fputchr(out, ' ');
				}

				fputAst(out, esc, ast, 0, 0);

				// in case of unary operators: '-a' not '- a'
				if (ast->op.lhso != NULL) {
					fputchr(out, ' ');
				}

				fputAst(out, esc, ast->op.rhso, mode, -pre);

				if (putparen) {
					fputchr(out, ')');
				}
			}
			else switch (ast->kind) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					return;
				case OPER_dot: fputstr(out, esc, "."); break;		// '.'
				case OPER_adr: fputstr(out, esc, "&"); break;		// '&'
				case OPER_pls: fputstr(out, esc, "+"); break;		// '+'
				case OPER_mns: fputstr(out, esc, "-"); break;		// '-'
				case OPER_cmt: fputstr(out, esc, "~"); break;		// '~'
				case OPER_not: fputstr(out, esc, "!"); break;		// '!'

				case OPER_shr: fputstr(out, esc, ">>"); break;		// '>>'
				case OPER_shl: fputstr(out, esc, "<<"); break;		// '<<'
				case OPER_and: fputstr(out, esc, "&"); break;		// '&'
				case OPER_ior: fputstr(out, esc, "|"); break;		// '|'
				case OPER_xor: fputstr(out, esc, "^"); break;		// '^'

				case OPER_equ: fputstr(out, esc, "=="); break;		// '=='
				case OPER_neq: fputstr(out, esc, "!="); break;		// '!='
				case OPER_lte: fputstr(out, esc, "<"); break;		// '<'
				case OPER_leq: fputstr(out, esc, "<="); break;		// '<='
				case OPER_gte: fputstr(out, esc, ">"); break;		// '>'
				case OPER_geq: fputstr(out, esc, ">="); break;		// '>='

				case OPER_add: fputstr(out, esc, "+"); break;		// '+'
				case OPER_sub: fputstr(out, esc, "-"); break;		// '-'
				case OPER_mul: fputstr(out, esc, "*"); break;		// '*'
				case OPER_div: fputstr(out, esc, "/"); break;		// '/'
				case OPER_mod: fputstr(out, esc, "%"); break;		// '%'

				case OPER_lnd: fputstr(out, esc, "&&"); break;		// '&'
				case OPER_lor: fputstr(out, esc, "||"); break;		// '|'

				case ASGN_set: fputstr(out, esc, "="); break;		// ':='
			}
			break;
		}

		case OPER_com: {
			if (rlev > 0) {
				fputAst(out, esc, ast->op.lhso, mode, -OPER_com);
				fputstr(out, esc, ", ");
				fputAst(out, esc, ast->op.rhso, mode, -OPER_com);
			}
			else {
				fputchr(out, ',');
			}
			break;
		}

		case OPER_sel: {	// '?:'
			if (rlev > 0) {
				fputAst(out, esc, ast->op.test, mode, -OPER_sel);
				fputstr(out, esc, " ? ");
				fputAst(out, esc, ast->op.lhso, mode, -OPER_sel);
				fputstr(out, esc, " : ");
				fputAst(out, esc, ast->op.rhso, mode, -OPER_sel);
			}
			else {
				fputstr(out, esc, "?:");
			}
			break;
		}
		//#}
		//#{ TVAL
		case EMIT_opc:
			fputstr(out, esc, "emit");
			break;

		case TYPE_bit:
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

		case TYPE_def: {
			if (ast->ref.link) {
				fputSym(out, esc, ast->ref.link, mode | prSymInit | prType, -indent);
			}
			else {
				fputstr(out, esc, ast->ref.name);
			}
			break;
		}
		case TYPE_ref: {
			if (ast->ref.link) {
				fputSym(out, esc, ast->ref.link, 0, 0);
			}
			else {
				fputstr(out, esc, ast->ref.name);
			}
			break;
		}
		//#}

		case PNCT_lc:
			fputchr(out, '[');
			break;

		case PNCT_rc:
			fputchr(out, ']');
			break;

		case PNCT_lp:
			fputchr(out, '(');
			break;

		case PNCT_rp:
			fputchr(out, ')');
			break;

		default:
			fputFmt(out, esc, "%K(0x%02x)", ast->kind, ast->kind);
			break;
	}
}

void fputSym(FILE *out, const char *esc[], symn sym, int mode, int indent) {
	int pr_type = mode & prType;
	int pr_init = mode & prSymInit;
	int pr_qual = mode & prSymQual;
	int pr_attr = 1;//mode & prSymAttr;
	int pr_args = (mode & 0xf) > 0;//mode & prSymArgs;
	int pr_body = (mode & 0xf) > 6;//mode & prSymArgs;

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

		case EMIT_opc: {
			if (sym->init) {
				fputfmt(out, "(%+t)", sym->init);
			}
			break;
		}

		case TYPE_arr: {
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
		case TYPE_ref: {
			if (pr_args && sym->call) {
				symn arg = sym->prms;
				fputchr(out, '(');
				// TODO: check cc->void_tag?
				if (arg && arg->name && *arg->name) {
					for ( ; arg; arg = arg->next) {
						if (arg != sym->prms) {
							fputstr(out, esc, ", ");
						}
						fputSym(out, esc, arg, mode & ~prSymQual, 0);
					}
				}
				fputchr(out, ')');
			}
			if (pr_type && sym->type) {
				fputstr(out, esc, ": ");
				fputSym(out, esc, sym->type, 0, 0);
			}
			if (pr_init && sym->init) {
				fputstr(out, esc, " = ");
				fputAst(out, esc, sym->init, 1, 0);
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
							if (prc < 0) {
								prc = prType | prSymQual | 1;
							}
							break;
						case '+':
							if (prc < 0) {
								prc = prSymQual | 1;
							}
							break;
					}
					if (prc < 0) {
						prc = prSymQual;
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
					fputAst(fout, esc, ast, prc, len);
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
		case STMT_end:
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
			fputFmt(fout, escape, "%I</%s>\n", level, text);
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
	for (*sp = rt->vars; sp >= bp;) {
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
			case TYPE_arr:
			case TYPE_rec:
				*++sp = sym->flds;
				break;

			// variable/function
			case TYPE_ref:
				/* print function private members ?
				if (sym->call && sym->flds) {
					*++sp = sym->flds;
				}
				// */
				break;

			case EMIT_opc:
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
	if (customPrinter != NULL) {
		customPrinter(ctx, NULL);
	}
}
