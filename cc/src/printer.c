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

//~ #define fputstr(__OUT, __STR) fputs(__STR, __OUT)
#define fputchr(__OUT, __CHR) fputc(__CHR, __OUT)

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

static void fputstr(FILE* fout, char *esc[], char* str) {
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

static char** escapeStr() {
	static char *escape[256];
	static char initialized = 0;
	if (!initialized) {
		memset(escape, 0, sizeof(escape));
		//~ escape['a'] = "`$!>a<!$`";
		escape['\n'] = "\\n";
		escape['\r'] = "\\r";
		escape['\''] = "\\'";
		escape['\"'] = "\\\"";
		initialized = 1;
	}
	return escape;
}

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

static void fputsym(FILE* fout, char *esc[], symn sym, int mode, int level);
static void typeQual(FILE* fout, symn sym) {
	if (sym->decl != NULL) {
		typeQual(fout, sym->decl);
	}
	fputsym(fout, NULL, sym, 0, 0);
	fputchr(fout, '.');
}
static void fputsym(FILE* fout, char *esc[], symn sym, int mode, int level) {
	int pr_type = mode & prType;
	int pr_init = mode & prInit;
	int pr_qual = mode & prQual;
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
				fputfmt(fout, "(%+k)", sym->init);
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
					fputfmt(fout, "[%?+k]", typ->init);
				}
				return;
			}
			// fall TYPE_rec
		}
		case TYPE_rec: {
			symn arg;
			if (rlev < 2) {
				if (pr_qual && sym->decl) {
					typeQual(fout, sym->decl);
				}
				fputstr(fout, esc, sym->name);
				break;
			}

			fputfmt(fout, "struct %?s {\n", sym->name);

			for (arg = sym->flds; arg; arg = arg->next) {
				fputsym(fout, esc, arg, mode & ~prQual, level + 1);
				if (arg->kind != TYPE_rec) {		// nested struct
					fputstr(fout, esc, ";\n");
				}
			}

			fputfmt(fout, "%I}\n", level);

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
					fputsym(fout, esc, sym->decl, prQual, 0);
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
				fputfmt(fout, " = %+k", sym->init);
			}

			break;
		}

		default:
			fatal(ERR_INTERNAL_ERROR);
			return;
	}
}

static void fputast(FILE* fout, char *esc[], astn ast, int mode, int level) {
	int nl_body = mode & nlBody;	// '\n'? { ...
	int nl_elif = mode & nlElIf;	// ...}'\n'else '\n'? if ...
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
				fputstr(fout, esc, "{ ... }");
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
			fputfmt(fout, "%I}\n", level);
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
				if (!nl_body && kind == STMT_beg) {
					fputstr(fout, esc, " ");
					fputast(fout, esc, ast->stmt.stmt, mode, -level);
				}
				else {
					fputstr(fout, esc, "\n");
					fputast(fout, esc, ast->stmt.stmt, mode, level + (kind != STMT_beg));
				}
			}
			else {
				fputstr(fout, esc, ";\n");
			}

			// if else part
			if (ast->stmt.step != NULL) {
				int kind = ast->stmt.step->kind;
				if (!nl_body && (kind == STMT_beg)) {
					fputfmt(fout, "%Ielse ", level);
					fputast(fout, esc, ast->stmt.step, mode, -level);
				}
				else if (!nl_elif && (kind == STMT_if)) {
					fputfmt(fout, "%Ielse ", level);
					fputast(fout, esc, ast->stmt.step, mode, -level);
				}
				else {
					fputfmt(fout, "%Ielse\n", level);
					fputast(fout, esc, ast->stmt.step, mode, level + (kind != STMT_beg));
				}
			}

			break;
		}
		case STMT_for: {
			if (rlev < 2) {
				fputstr(fout, esc, "for");
				if (rlev > 0) {
					fputfmt(fout, " (%?+ k; %?+k; %?+k)", ast->stmt.init, ast->stmt.test, ast->stmt.step);
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
				if (!nl_body && kind == STMT_beg) {
					fputstr(fout, esc, " ");
					fputast(fout, esc, ast->stmt.stmt, mode, -level);
				}
				else {
					fputstr(fout, esc, "\n");
					fputast(fout, esc, ast->stmt.stmt, mode, level + (kind != STMT_beg));
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
					trace("%+k", ast);
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

				if (mode & prCast) {
					fputsym(fout, esc, ast->type, prQual, 0);
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
			fputfmt(fout, "%U", ast->cint);
			break;

		case TYPE_int:
			fputfmt(fout, "%D", ast->cint);
			break;

		case TYPE_flt:
			fputfmt(fout, "%F", ast->cflt);
			break;

		case TYPE_str:
			fputchr(fout, '\'');
			fputstr(fout, escapeStr(), ast->ref.name);
			fputchr(fout, '\'');
			break;

		case TYPE_def: {
			if (ast->ref.link) {
				fputsym(fout, esc, ast->ref.link, mode|prInit|prType, -level);
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
			fputfmt(fout, "%t(0x%02x)", ast->kind, ast->kind);
			break;
	}
}

static void FPUTFMT(FILE* fout, char *esc[], const char* msg, va_list ap) {
	char buff[1024], chr;

	while ((chr = *msg++)) {
		if (chr == '%') {
			char	nil = 0;	// [?]? skip on null || zero value
			char	sgn = 0;	// [+-]?
			char	pad = 0;	// 0?
			long	len = 0;	// ([0-9]*)? length
			long	prc = -1;	// (.('*')|([0-9])*)? precision / ident
			char*	str = NULL;	// the string to be printed
			//~ %(\?)?[+-]?[0 ]?([1-9][0-9]*)?(.[1-9][0-9]*)?[tTkKAIbBoOxXuUdDfFeEsScC]
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

				// TODO: remove token kind. (grep '%[^t"]*t' *)
				// TODO: %t: tree; %T: type.
				case 'T': {		// type
					int mode = 0;
					symn sym = va_arg(ap, symn);

					if (sym == NULL && nil) {
						if (pad != 0) {
							fputchr(fout, pad);
						}
						continue;
					}

					switch (sgn) {
						default:
							break;
						case '-':
							mode = prType | prQual | 1;
							break;
						case '+':
							mode = prQual | 1;
							break;
					}
					fputsym(fout, esc, sym, mode, prc < 0 ? prc : -prc);
					continue;
				}

				case 'k': {		// node
					int mode = 0;
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

						case 0:
							mode = len;
							break;

						case '-':
							mode = 2;
							break;

						case '+':
							mode = 1;
							prc = 0xf;
							break;
					}

					fputast(fout, esc, ast, mode, prc < 0 ? prc : -prc);
					continue;
				}

				case 't': {		// token kind
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
						str = buff;
						arg &= 0xff;
						if (arg < tok_last) {
							sprintf(str, "%s%s%s", con, stat, tok_tbl[arg].name);
						}
						else {
							sprintf(str, "%s%s.ERR_%02x", con, stat, arg);
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

					// fputfmt(fout, ".%06x: ", prc);
					fputopc(fout, opc, len, prc, NULL);
					continue;
				}

				case 'I': {		// ident
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

			if (str) {
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

static void fputesc(FILE* fout, char *esc[], const char* msg, ...) {
	va_list ap;
	va_start(ap, msg);
	FPUTFMT(fout, esc, msg, ap);
	va_end(ap);
}


/** print symbols
 * output looks like:
 * file:line:kind:?prot:name:type
 * file: is the file name of decl
 * line: is the line number of decl
 * kind:
 *	'^': typename(dcl)
 *	'#': constant(def)
 *	'$': variable(ref)
 * @param mode
 * :	mode & 0x80: print file:line
 * :	mode & 0x40: print init
 * :	mode & 0x20: print qualified symbol type names: Math.sin(double x): Math.double
 * :	mode & 0x10: print type
 * :	mode & 0x0f: print level:
 * :		0: this symbol only;
 * :		1: this symbol only;
**/
static void dumpsym(FILE* fout, symn sym, int mode) {
	symn ptr, bp[TOKS], *sp = bp;

	int print_line = mode & prLine;		// print file and location.
	int print_type = mode & prType;		// print type of symbol.
	int print_init = mode & prInit;		// print initializer of symbol.
	//~ int print_qual = mode & prQual;		// print qualified name of symbol.
	int print_info = 0;
	int print_used = 0;
	mode &= 0x0f;

	for (*sp = sym; sp >= bp;) {
		char* tch = "";
		if (!(ptr = *sp)) {
			--sp;
			continue;
		}
		*sp = ptr->next;

		switch (ptr->kind) {

			// constant
			case TYPE_def:
				if (mode > 4) {
					*++sp = ptr->flds;
				}
				tch = "alias";
				break;

			// typename/array
			case TYPE_arr:
			case TYPE_rec:
				if (mode > 1) {
					*++sp = ptr->flds;
				}
				tch = "typename";
				break;

			// variable/function
			case TYPE_ref:
				tch = "variable";
				if (mode > 3 && ptr->call && ptr->prms) {
					*++sp = ptr->prms;
				}
				break;

			case EMIT_opc:
				tch = "opcode";
				if (mode > 2) {
					*++sp = ptr->flds;
				}
				break;

			default:
				trace("psym:%d:%T['%t']", ptr->kind, ptr, ptr->kind);
				tch = "err";
				break;
		}

		if (print_line && ptr->file && ptr->line) {
			fputfmt(fout, "%s:%u: ", ptr->file, ptr->line);
		}

		// qualified name with arguments
		fputsym(fout, NULL, ptr, prQual | 1, 0);

		// qualified base type(s)
		if (print_type && ptr->type) {
			fputfmt(fout, ": %-T", ptr->type);
		}

		// initializer
		if (print_init && ptr->init && ptr->kind != TYPE_ref) {
			fputfmt(fout, " = %+k", ptr->init);
		}

		if (print_info) {
			fputfmt(fout, ": [%c%06x", ptr->stat ? '@' : '+', ptr->offs);
			fputfmt(fout, ", size: %d", ptr->size);
			fputfmt(fout, ", kind:");

			if (ptr->stat) {
				fputfmt(fout, " %s", "static");
			}
			if (ptr->cnst) {
				fputfmt(fout, " %s", "const");
			}
			fputfmt(fout, " %s", tch);

			if (ptr->cast != 0) {
				fputfmt(fout, ", cast: %t", ptr->cast);
			}

			fputstr(fout, NULL, "]");
		}

		fputchr(fout, '\n');
		fflush(fout);

		if (print_used) {
			astn used;
			for (used = ptr->used; used; used = used->ref.used) {
				fputfmt(fout, "%s:%d: referenced(%t) as `%+k`\n", used->file, used->line, used->cst2, used);
			}
		}

		if (mode == 0) {
			break;
		}
	}
}

static void dumpapi(FILE* fout, symn sym) {
	symn ptr, bp[TOKS], *sp = bp;

	for (*sp = sym; sp >= bp;) {
		if (!(ptr = *sp)) {
			--sp;
			continue;
		}
		*sp = ptr->next;

		switch (ptr->kind) {

			// constant
			case TYPE_def:
				/* if (ptr->call && ptr->prms) {
					*++sp = ptr->prms;
				} */
				*++sp = ptr->flds;
				break;

			// array/typename
			case TYPE_arr:
			case TYPE_rec:
				*++sp = ptr->flds;
				break;

			// variable/function
			case TYPE_ref:
				if (ptr->call) {
					*++sp = ptr->flds;
				}
				/* if (ptr->call && ptr->prms) {
					*++sp = ptr->prms;
				} */
				break;

			case EMIT_opc:
				*++sp = ptr->flds;
				break;

			default:
				trace("psym:%d:%T['%t']", ptr->kind, ptr, ptr->kind);
				break;
		}

		// qualified name with arguments
		fputsym(fout, NULL, ptr, prQual | 1, 0);

		if (ptr->call && ptr->type) {
			fputstr(fout, NULL, ": ");
			fputsym(fout, NULL, ptr->type, prQual | 1, 0);
		}

		fputchr(fout, '\n');
		fflush(fout);
	}
}

static void dumpxml(FILE* fout, astn ast, int mode, int level, const char* text) {
	char **escape = escapeXml();
	if (ast == NULL) {
		return;
	}

	fputesc(fout, escape, "%I<%s op=\"%t\"", level, text, ast->kind);

	if ((mode & prType) != 0) {
		fputesc(fout, escape, " type=\"%?T\"", ast->type);
	}

	if ((mode & prCast) != 0) {
		fputesc(fout, escape, " cast=\"%?t\"", ast->cst2);
	}

	if ((mode & prLine) != 0) {
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
			fputesc(fout, escape, " stmt=\"%?+k\">\n", ast);
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
			fputesc(fout, escape, " stmt=\"%?+k\">\n", ast);
			dumpxml(fout, ast->stmt.test, mode, level + 1, "test");
			dumpxml(fout, ast->stmt.stmt, mode, level + 1, "then");
			dumpxml(fout, ast->stmt.step, mode, level + 1, "else");
			fputesc(fout, escape, "%I</%s>\n", level, text);
			break;

		case STMT_for:
			fputesc(fout, escape, " stmt=\"%?+k\">\n", ast);
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
			fputesc(fout, escape, " stmt=\"%?+k\">\n", ast);
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
			fputesc(fout, escape, " oper=\"%?+k\">\n", ast);
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
			fputesc(fout, escape, " value=\"%+k\" />\n", ast);
			break;

		case TYPE_ref:
			fputesc(fout, escape, ">%T</%s>\n", ast->ref.link, text);
			break;

		case TYPE_def: {
			symn var = ast->ref.link;
			astn init = var ? var->init : NULL;
			symn fields = var ? var->flds : NULL;

			fputesc(fout, escape, " tyc=\"%?t\" name=\"%+T\"", var ? var->cast : TYPE_any, var);
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
					fputesc(fout, escape, "%I<%s op=\"%t\"", level + 1, argn, def->kind, def);
					if (mode & prType) {
						fputesc(fout, escape, " type=\"%?T\"", def->type);
					}
					if (mode & prCast) {
						fputesc(fout, escape, " cast=\"%?t\"", def->cast);
					}
					if (mode & prLine) {
						// && def->file && def->line) {
						fputesc(fout, escape, " line=\"%s:%d\"", def->file, def->line);
					}
					fputesc(fout, escape, " name=\"%+T\"", def);

					if (def->init) {
						fputesc(fout, escape, " value=\"%+k\"", def->init);
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
}


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

void dump(state rt, int mode, symn sym) {
	int level = mode & 0xff;
	FILE* logf = rt->logf;

	if (logf == NULL) {
		return;
	}

	if (mode & dump_sym && rt->defs != NULL) {
		if (sym != NULL) {
			dumpsym(logf, sym, level);
		}
		else {
			if (level == 0) {
				dumpapi(logf, rt->defs);
			}
			else if (level == 1) {
				for (sym = rt->defs; sym; sym = sym->defs) {
					fputfmt(logf, "%-T: %-T\n", sym, sym->type);
				}
			}
			else if (level == 2) {
				for (sym = rt->defs; sym; sym = sym->gdef) {
					fputfmt(logf, "%-T: %-T\n", sym, sym->type);
				}
			}
			else if (level == 3) {
				for (sym = rt->defs; sym; sym = sym->next) {
					fputfmt(logf, "%-T: %-T\n", sym, sym->type);
				}
			}
			else if (level == -1) {
				for (sym = rt->defs; sym; sym = sym->next) {
					// qualified name with arguments
					fputsym(logf, NULL, sym, prQual | 1, 0);

					if (sym->call && sym->type) {
						fputstr(logf, NULL, ": ");
						fputsym(logf, NULL, sym->type, prQual | 1, 0);
					}

					fputchr(logf, '\n');
					fflush(logf);
				}
			}
			else {
				dumpsym(logf, rt->defs, level);
			}
		}
	}

	// ast can be printed only if compiler context was not destroyed
	if (mode & dump_ast && rt->cc != NULL) {
		if (sym != NULL) {
			if (sym->kind == TYPE_ref && sym->call) {
				dumpxml(logf, sym->init, level, 0, "code");
			}
		}
		else {
			dumpxml(logf, rt->cc->root, level, 0, "code");
		}
	}

	if (mode & dump_asm) {
		if (sym != NULL) {
			fputfmt(logf, "%-T [@%06x: %d] {\n", sym, sym->offs, sym->size);
			if (sym->kind == TYPE_ref && sym->call) {
				symn param;
				for (param = sym->flds; param; param = param->next) {
					fputfmt(logf, "\t.local %-T [@%06x, size:%d, cast:%t]\n", param, param->offs, param->size, param->cast);
				}
				fputasm(rt, logf, sym->offs, sym->offs + sym->size, 0x100 | (mode & 0xff));
			}
			fputfmt(logf, "}\n");
		}
		else {
			for (sym = rt->defs; sym; sym = sym->gdef) {
				if (sym->kind == TYPE_ref && sym->call) {
					symn param;
					fputfmt(logf, "%-T [@%06x: %d] {\n", sym, sym->offs, sym->size);
					for (param = sym->flds; param; param = param->next) {
						fputfmt(logf, "\t.local %-T [@%06x, size:%d, cast:%t]\n", param, param->offs, param->size, param->cast);
					}
					//~ fputast(logf, NULL, sym->init, 2, 1);
					fputfmt(logf, "\t.ast %15.1k\n", sym->init);
					fputasm(rt, logf, sym->offs, sym->offs + sym->size, 0x100 | (mode & 0xff));
					fputfmt(logf, "}\n");
				}
			}
		}
	}

	if (mode & dump_bin) {
		unsigned int i, brk = level & 0xff;
		unsigned int max = rt->_beg - rt->_mem;

		if (brk == 0)
			brk = 16;

		for (i = 0; i < max; i += 1) {
			int val = rt->_mem[i];
			if (((i % brk) == 0) && i != 0) {
				unsigned int j = i - brk;
				fputchr(logf, ' ');
				for ( ; j < i; j += 1) {
					int chr = rt->_mem[j];

					if (chr < 32 || chr > 127)
						chr = '.';

					fputchr(logf, chr);
				}
				fputchr(logf, '\n');
			}
			else if (i != 0) {
				fputchr(logf, ' ');
			}
			fputchr(logf, "0123456789abcdef"[val >> 16 & 0x0f]);
			fputchr(logf, "0123456789abcdef"[val >>  0 & 0x0f]);
		}
	}
}

void perr(state rt, int level, const char* file, int line, const char* msg, ...) {
	int warnLevel = rt && rt->cc ? rt->cc->warn : 0;
	FILE *logFile = rt ? rt->logf : stdout;
	char **esc = NULL;

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
		fputfmt(logFile, "%s:%d: ", file, line);
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
