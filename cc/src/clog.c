#include "ccvm.h"
//~~~~~~~~~~ Output ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include <stdarg.h>
#include <string.h>

enum Format {
	// ast
	noIden = 0x0100,

	alPars = 0x0010,
	nlBody = 0x0020,
	nlElIf = 0x0040,

	// sym
	prType = 0x0010,
	prQual = 0x0020,
	prArgs = 0x0040,

	// xml
	prCast = 0x0020,
	prLine = 0x0080,
};

static void fputsym(FILE *fout, symn sym, int mode, int level) {
	int rlev = mode & 0xf;

	if (!sym) {
		fputs("(null)", fout);
		return;
	}

	if (sym->kind == TYPE_arr) {
		fputsym(fout, sym->type, mode, level);
		fputfmt(fout, "[%?d]", sym->size);
		return;
	}

	if (sym->kind == EMIT_opc) {
		if ((mode & prQual) && sym->decl) {
			fputsym(fout, sym->decl, mode & prQual, 0);
			fputc('.', fout);
		}
		if (sym->name)
			fputs(sym->name, fout);
		if (sym->init)
			fputfmt(fout, "(%d)", constint(sym->init));
		return;
	}

	if ((mode & prType) && sym->type) {
		switch (sym->kind) {
			case TYPE_def:
				//~ fputs("define ", fout);
				break;
			case TYPE_rec:
				//~ fputs("struct ", fout);
				break;
			case TYPE_enu:
				//~ fputs("enum ", fout);
				break;
			default:
				fputsym(fout, sym->type, mode, level);
				fputc(' ', fout);
		}
	}

	if ((mode & prQual) && sym->decl) {
		fputsym(fout, sym->decl, mode & prQual, 0);
		fputc('.', fout);
	}

	if (sym->name)
		fputs(sym->name, fout);
	else
		fputs("_", fout);

	if ((mode & prArgs) && sym->call) {
		symn arg = sym->args;
		fputc('(', fout);
		//~ if (arg != void_arg) 
		while (arg) {
			fputsym(fout, arg->type, mode, 0);
			if (mode && arg->name && *arg->name && rlev > 0) {
				fputc(' ', fout);
				fputs(arg->name, fout);
			}

			if ((arg = arg->next))
				fputs(", ", fout);
		}
		fputc(')', fout);
	}
}
static void fputast(FILE *fout, astn ast, int mode, int level) {
	int noiden = mode & noIden;	// internal use
	int nlelif = mode & nlElIf;	// ...}'\n'else '\n'? if ...
	int nlbody = mode & nlBody;	// '\n'? { ...
	int rlev = mode & 0xf;

	if (ast) switch (ast->kind) {
		//{ STMT
		case STMT_do: {
			if (rlev < 2) {
				if (rlev > 0) {
					fputast(fout, ast->stmt.stmt, mode, 0xf);
				}
				fputs(";", fout);
				break;
			}
			fputfmt(fout, "%I", noiden ? 0 : level);
			fputast(fout, ast->stmt.stmt, mode, 0xf);
			fputfmt(fout, ";\n");
		} break;
		case STMT_beg: {
			astn lst;
			if (rlev < 2) {
				fputs("{}", fout);
				break;
			}
			fputfmt(fout, "%I%{\n", noiden ? 0 : level);
			for (lst = ast->stmt.stmt; lst; lst = lst->next)
				fputast(fout, lst, mode & ~noIden, level + 1);
			fputfmt(fout, "%I}\n", level, ast->type);
		} break;
		case STMT_if:  {
			if (rlev < 2) {
				fputs("if", fout);
				if (rlev > 0)
					fputfmt(fout, " (%+k)", ast->stmt.test);
				break;
			}
			fputfmt(fout, "%I", noiden ? 0 : level);
			switch (ast->csts) {
				case 0: break;
				case QUAL_sta:
					fputfmt(fout, "static ");
					break;
				default:
					debug("error");
					break;
			}
			fputfmt(fout, "if (%+k)", ast->stmt.test);
			if (ast->stmt.stmt) {
				int kind = ast->stmt.stmt->kind;
				if (!nlbody && kind == STMT_beg) {
					fputfmt(fout, " ");
					fputast(fout, ast->stmt.stmt, mode | noIden, level);
				}
				else {
					fputfmt(fout, "\n");
					fputast(fout, ast->stmt.stmt, mode, level + (kind != STMT_beg));
				}
			}
			else fputfmt(fout, ";\n");
			if (ast->stmt.step) {
				int kind = ast->stmt.step->kind;
				if (!nlbody && (kind == STMT_beg)) {
					fputfmt(fout, "%Ielse ", level);
					fputast(fout, ast->stmt.step, mode | noIden, level);
				}
				else if (!nlelif && (kind == STMT_if)) {
					fputfmt(fout, "%Ielse ", level);
					fputast(fout, ast->stmt.step, mode | noIden, level);
				}
				else {
					fputfmt(fout, "%Ielse\n", level);
					fputast(fout, ast->stmt.step, mode, level + (kind != STMT_beg));
				}
			}
		} break;
		case STMT_for: {
			if (rlev < 2) {
				fputs("for", fout);
				if (rlev > 0)
					fputfmt(fout, " (%+k; %+k; %+k)", ast->stmt.init, ast->stmt.test, ast->stmt.step);
				break;
			}
			fputfmt(fout, "%I", noiden ? 0 : level);
			switch (ast->csts) {
				case 0: break;
				case QUAL_sta:
					fputfmt(fout, "static ");
					break;
				case QUAL_par:
					fputfmt(fout, "parralel ");
					break;
				default:
					debug("error");
					break;
			}
			fputfmt(fout, "for (%?+k; %?+k; %?+k)", ast->stmt.init, ast->stmt.test, ast->stmt.step);
			if (ast->stmt.stmt) {
				int kind = ast->stmt.stmt->kind;
				if (!nlbody && kind == STMT_beg) {
					fputfmt(fout, " ");
					fputast(fout, ast->stmt.stmt, mode | noIden, level);
				}
				else {
					fputfmt(fout, "\n");
					fputast(fout, ast->stmt.stmt, mode, level + (kind != STMT_beg));
				}
			}
			else fputfmt(fout, ";\n");
		} break;
		//} */ // STMT
		//{ OPER
		case OPER_dot: {	// '.'
			if (rlev) {
				fputast(fout, ast->op.lhso, mode, 0);
				fputc('.', fout);
				fputast(fout, ast->op.rhso, mode, 0);
			}
			else
				fputc('.', fout);
		} break;
		case OPER_fnc: {	// '()'
			if (rlev && ast->op.lhso) {
				fputast(fout, ast->op.lhso, mode, 0);
				fputc('(', fout);
				if (rlev && ast->op.rhso)
					fputast(fout, ast->op.rhso, mode, 0);
				fputc(')', fout);
			}
			else if (rlev) {
				if (rlev && ast->op.rhso)
					fputast(fout, ast->op.rhso, mode, 0);
			}
			else {
				fputc('(', fout);
				if (rlev && ast->op.rhso)
					fputast(fout, ast->op.rhso, mode, 0);
				fputc(')', fout);
			}
		} break;
		case OPER_idx: {	// '[]'
			if (rlev)
				fputast(fout, ast->op.lhso, mode, 0);
			fputc('[', fout);
			if (rlev && ast->op.rhso)
				fputast(fout, ast->op.rhso, mode, 0);
			fputc(']', fout);
		} break;

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

		case ASGN_set: {	// '='
			int pre = ast->kind & 0xf;
			//~ int putlrp = mode & alPars;
			if (rlev) {
				int putlrp = (level < pre) || (mode & alPars);
				if (putlrp) fputc('(', fout);
				if (ast->op.lhso) {
					fputast(fout, ast->op.lhso, mode, pre);
					fputc(' ', fout);
				}
				fputast(fout, ast, 0, 0);
				if (ast->op.lhso) fputc(' ', fout);
				fputast(fout, ast->op.rhso, mode, pre);
				if (putlrp) fputc(')', fout);
			}
			else switch (ast->kind) {
				default: fatal("FixMe");
				case OPER_pls: fputs("+", fout); break;		// '+'
				case OPER_mns: fputs("-", fout); break;		// '-'
				case OPER_cmt: fputs("~", fout); break;		// '~'
				case OPER_not: fputs("!", fout); break;		// '!'

				case OPER_shr: fputs(">>", fout); break;		// '>>'
				case OPER_shl: fputs("<<", fout); break;		// '<<'
				case OPER_and: fputs("&", fout); break;		// '&'
				case OPER_ior: fputs("|", fout); break;		// '|'
				case OPER_xor: fputs("^", fout); break;		// '^'

				case OPER_equ: fputs("==", fout); break;		// '=='
				case OPER_neq: fputs("!=", fout); break;		// '!='
				case OPER_lte: fputs("<", fout); break;		// '<'
				case OPER_leq: fputs("<=", fout); break;		// '<='
				case OPER_gte: fputs(">", fout); break;		// '>'
				case OPER_geq: fputs(">=", fout); break;		// '>='

				case OPER_add: fputs("+", fout); break;		// '+'
				case OPER_sub: fputs("-", fout); break;		// '-'
				case OPER_mul: fputs("*", fout); break;		// '*'
				case OPER_div: fputs("/", fout); break;		// '/'
				case OPER_mod: fputs("%", fout); break;		// '%'

				case ASGN_set: fputs("=", fout); break;		// '%'
			}
		} break;

		case OPER_com: {
			if (rlev) {
				fputast(fout, ast->op.lhso, mode, OPER_com);
				fputs(", ", fout);
				fputast(fout, ast->op.rhso, mode, OPER_com);
			}
			else
				fputc(',', fout);
		} break;

		case OPER_sel: {	// '?:'
			if (rlev) {
				fputast(fout, ast->op.test, mode, OPER_sel);
				fputs(" ? ", fout);
				fputast(fout, ast->op.lhso, mode, OPER_sel);
				fputs(" : ", fout);
				fputast(fout, ast->op.rhso, mode, OPER_sel);
			}
			else
				fputs("?:", fout);
		}break;
		//}
		//{ TVAL
		case EMIT_opc: fputfmt(fout, "emit"); break;
		case TYPE_bit: fputfmt(fout, "%U", ast->con.cint); break;
		case TYPE_int: fputfmt(fout, "%D", ast->con.cint); break;
		case TYPE_flt: fputfmt(fout, "%F", ast->con.cflt); break;
		//~ case TYPE_str: fputfmt(fout, "'%s'", ast->con.cstr); break;
		case TYPE_str: fputfmt(fout, "\"%s\"", ast->id.name); break;
		case TYPE_ref: {
			if (ast->id.link)
				fputsym(fout, ast->id.link, 0, level);
			else
				fputfmt(fout, "%s", ast->id.name);
		} break;

		case TYPE_def: {
			symn val = ast->id.link;
			if (val) {
				switch (val->kind) {
					case TYPE_enu: fputfmt(fout, "enum "); break;
					case TYPE_rec: fputfmt(fout, "struct "); break;
					case TYPE_def: fputfmt(fout, "define "); break;
				}
				fputsym(fout, val, rlev ? -1 : 0, level);
				if (rlev && val && val->init) {
					fputfmt(fout, " = %+k", val->init);
				}
			}
			else
				fputfmt(fout, "define ");

		} break;
		//}

		case PNCT_lc: fputfmt(fout, "%c", '['); break;
		case PNCT_rc: fputfmt(fout, "%c", ']'); break;
		case PNCT_lp: fputfmt(fout, "%c", '('); break;
		case PNCT_rp: fputfmt(fout, "%c", ')'); break;
		default:
			fputfmt(fout, "%t(0x%02x)", ast->kind, ast->kind);
			break;
	}
	else fputs("(null)", fout);
}

static char* fmtuns(char *dst, int max, int prc, int radix, uint64_t num) {
	char *ptr = dst + max;
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

void fputopc(FILE *fout, unsigned char* ptr, int len, int offset);

static void FPUTFMT(FILE *fout, const char *msg, va_list ap) {
	char buff[1024], chr;
	while ((chr = *msg++)) {
		if (chr == '%') {
			char	nil = 0;	// [?]? skip on null || zero value
			char	sgn = 0;	// [+-]?
			long	len = 0;	// ([0-9]*)?
			char	pad = 0;	// 0?
			long	prc = -1;	// (.('*')|([0-9])*)? precision
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
					//--msg; break;
					return;

				default:
					fputc(chr, fout);
					continue;

				case 'T': {		// type
					symn sym = va_arg(ap, symn);
					if (!sym && nil)
						continue;
					switch (sgn) {
						case '+': fputsym(fout, sym, prQual, 0); break;
						case '-': fputsym(fout, sym, prType|prQual|prArgs, 0); break;
						default:  fputsym(fout, sym, 0, 0); break;
					}
				} continue;
				case 'k': {		// node
					astn ast = va_arg(ap, astn);
					if (!ast && nil)
						continue;
					switch (sgn) {
						case '-': fputast(fout, ast, noIden | 2, prc); break;	// walk
						case '+': fputast(fout, ast, noIden | 1, 0); break;		// tree
						default:  fputast(fout, ast, noIden | 0, 0); break;		// astn
					}
				} continue;
				case 'K': {		// node
					astn ast = va_arg(ap, astn);
					if (!ast && nil)
						continue;
					/*switch (sgn) {
						case '-': fputast(fout, ast, noIden | 2, 0xf0); break;	// walk
						case '+': fputast(fout, ast, noIden | 1, 0xff); break;	// tree
						default:  fputast(fout, ast, noIden | 0, 0xf0); break;	// astn
					}*/
					dumpxml(fout, ast, 0, "node", len<<4);
					//~ fputxml(fout, ast, 2, len);
				} continue;
				case 'A': {		// opcode
					void* opc = va_arg(ap, void*);
					if (nil && !opc)
						continue;
					fputopc(fout, opc, len, prc);
				} continue;
				case 't': {		// token
					unsigned arg = va_arg(ap, unsigned);
					dieif (arg > tok_last, "%d", arg);
					if (!arg && nil)
						continue;
					if (arg < tok_last)
						str = (char*)tok_tbl[arg].name;
					else
						str = ".ERROR.";
				} break;

				case 'I': {		// ident
					unsigned arg = va_arg(ap, unsigned);
					len = len ? len * arg : arg;
					if (!pad)
						pad = '\t';
					str = (char*)"";
				} break;

				case 'b': {		// bin32
					uint32_t num = va_arg(ap, int32_t);
					str = fmtuns(buff, sizeof(buff), prc, 2, num);
				} break;
				case 'B': {		// bin64
					uint64_t num = va_arg(ap, int64_t);
					str = fmtuns(buff, sizeof(buff), prc, 2, num);
				} break;
				case 'o': {		// oct32
					uint32_t num = va_arg(ap, int32_t);
					str = fmtuns(buff, sizeof(buff), prc, 8, num);
				} break;
				case 'O': {		// oct64
					uint64_t num = va_arg(ap, int64_t);
					str = fmtuns(buff, sizeof(buff), prc, 8, num);
				} break;
				case 'x': {		// hex32
					uint32_t num = va_arg(ap, int32_t);
					str = fmtuns(buff, sizeof(buff), prc, 16, num);
				} break;
				case 'X': {		// hex64
					uint64_t num = va_arg(ap, int64_t);
					str = fmtuns(buff, sizeof(buff), prc, 16, num);
				} break;

				case 'u':		// uint32
				case 'd': {		// dec32
					int neg = 0;
					uint32_t num = va_arg(ap, int32_t);
					if (chr == 'd' && (int32_t)num < 0) {
						num = -(int32_t)num;
						neg = -1;
					}
					str = fmtuns(buff, sizeof(buff), prc, 10, num);
					if (neg)
						*--str = '-';
					else if (sgn == '+')
						*--str = '+';
				} break;

				case 'U':		// uns64
				case 'D': {		// dec64
					int neg = 0;
					uint64_t num = va_arg(ap, int64_t);
					if (chr == 'D' && (int64_t)num < 0) {
						num = -(int64_t)num;
						neg = -1;
					}
					str = fmtuns(buff, sizeof(buff), prc, 10, num);
					if (neg)
						*--str = '-';
					else if (sgn == '+')
						*--str = '+';
				} break;

				case 'E':		// float64
				case 'F':		// float64
				case 'G':		// float64: TODO: replace fprintf
					chr -= 'A' - 'a';

				case 'e':		// float32
				case 'f':		// float32
				case 'g': {		// float32
					float64_t num = va_arg(ap, float64_t);
					if ((len = msg - fmt - 1) < 1020) {
						memcpy(buff, fmt, len);
						buff[len++] = chr;
						buff[len++] = 0;
						fprintf(fout, buff, num);
					}
				} continue;

				//~ case 'S':		// wstr
				case 's': {		// cstr
					str = va_arg(ap, char*);
					//~ if (!str) continue;
					//~ if (str) len -= strlen(str);
				} break;

				//~ case 'C':		// wchr  // passed as int
				case 'c': {		// cchr
					str = buff;
					str[0] = va_arg(ap, int);
					str[1] = 0;
					len -= 1;
				} break;
			}

			if (str) {
				len -= strlen(str);

				if (!pad)
					pad = ' ';

				while (len > 0) {
					fputc(pad, fout);
					len -= 1;
				}

				fputs(str, fout);
			}
		}
		else fputc(chr, fout);
	}
}

void fputfmt(FILE *fout, const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	FPUTFMT(fout, msg, ap);
	va_end(ap);
	fflush(fout);
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
**/
void dumpsym(FILE *fout, symn sym, int alma) {
	symn ptr, bp[TOKS], *sp = bp;
	const int chrtyp = 0;
	for (*sp = sym; sp >= bp;) {
		char* tch = NULL;
		if (!(ptr = *sp)) {
			--sp;
			continue;
		}
		*sp = ptr->next;

		switch (ptr->kind) {

			// constant/enum
			case TYPE_enu:
				tch = chrtyp ? "#" : "enu: ";
				break;
			case TYPE_def:
				tch = chrtyp ? "#" : "def: ";
				break;

			// typename/array
			case TYPE_arr:
			case TYPE_rec:
				tch = chrtyp ? "^" : "typ: ";
				break;

			// variable/function
			case TYPE_ref:
				tch = chrtyp ? "$" : "var: ";
				break;
			case EMIT_opc:
				tch = chrtyp ? "@" : "opc: ";
				break;

			default:
				debug("psym:%d:%T['%t']", ptr->kind, ptr, ptr->kind);
				tch = chrtyp ? "?" : "err: ";
				break;
		}

		switch (ptr->kind) {

			case EMIT_opc:
			case TYPE_def:
			case TYPE_ref:
				break;

			default:
				if (alma > 1)
					*++sp = ptr->args;
		}

		if (ptr->file && ptr->line)
			fputfmt(fout, "%s:%d:", ptr->file, ptr->line);

		fputs(tch, fout);

		// qualified name with arguments
		fputsym(fout, ptr, prQual|prArgs | 1, 20);

		// qualified type
		if (ptr->type) {
			fputs(": ", fout);
			fputsym(fout, ptr->type, prQual, 0);
		}

		if (ptr->cast)
			fputfmt(fout, ":[%t]", ptr->cast);

		// initializer
		if (ptr->init && ptr->kind != TYPE_ref) {
			fputs(" = ", fout);
			fputast(fout, ptr->init, noIden | 1, 0xf0);
		}

		/*/ size or offset
		if (ptr->kind == TYPE_ref) {
			if (ptr->offs < 0) {
				fputfmt(fout, "[@st(%d)]: ", -ptr->offs);
			}
			else
				fputfmt(fout, "[@%04xh]: ", ptr->offs);
		}
		else if (ptr->kind == EMIT_opc) {
			fputfmt(fout, "[@%02xh]: ", ptr->size);
		}
		else if (ptr->kind != TYPE_def) {
			fputfmt(fout, "[size: %d:%d]: ", ptr->size, 0);
		}
		// */

		fputc('\n', fout);

		/* Debug: where is used
		if (alma & 0x30 && ptr->used) {
			int used = 0;
			astn ast = ptr->used;
			if (alma & 0x20) while (ast != NULL) {
				fputfmt(fout, "%s:%d:using `%T` here\n", ptr->file, ast->line, ptr);
				ast = ast->id.nuse;
				used += 1;
			}
			if (alma & 0x10)
				fputfmt(fout, "%s:%d: `%-T` used %d times\n", ptr->file, ptr->line, ptr, used);
		}// */

		fflush(fout);
		if (!alma)
			break;
	}
}

void perr(state s, int level, const char *file, int line, const char *msg, ...) {
	FILE *logf = s ? s->logf : stderr;
	int warnl = s && s->cc ? s->cc->warn : 0;
	va_list argp;

	if (level > 0) {
		if (warnl < 0)
			level = warnl;
		if (level > warnl)
			return;
	}
	else if (s) {
		s->errc += 1;
	}

	if (!logf)
		return;

	if (s && s->cc && !file)
		file = s->cc->file;

	if (file && line)
		fprintf(logf, "%s:%u:", file, line);

	if (level > 0)
		fputs("warning:", logf);
	else if (level)
		fputs("error:", logf);

	va_start(argp, msg);
	FPUTFMT(logf, msg, argp);
	fputc('\n', logf);
	fflush(logf);
	va_end(argp);
}

//{ temp & debug ---------------------------------------------------------------
void dumpast(FILE *fout, astn ast, int level) {
	if ((level & 0x0f) == 0x0f)
		dumpxml(fout, ast, 0, "root", level);
	else
		//~ fputast(fout, ast, 2, -1);
		fputast(fout, ast, level | 2, 0);
}
void dumpasm(FILE *fout, vmState s, int mode) {
	fputasm(fout, s, s->pc, s->cs, mode);
}

void dump2file(state s, char *file, dumpMode mode, char *text) {
	FILE *fout = stdout;
	int level = mode & 0xff;

	if (file)
		fout = fopen(file, "w");

	dieif(!fout, "cannot open file '%s'", file);

	if (text != NULL)
		fputs(text, fout);

	switch (mode & dumpMask) {
		default: fatal("FixMe");
		case dump_ast: dumpast(fout, s->cc->root, level); break;
		case dump_sym: dumpsym(fout, s->defs, level); break;
		case dump_asm: dumpasm(fout, s->vm, level); break;
	}
	if (file)
		fclose(fout);
}
void dump(state s, dumpMode mode, char *text) {
	FILE *logf = s ? s->logf : stdout;
	int level = mode & 0xff;

	if (!logf)
		return;

	if (text != NULL)
		fputs(text, logf);

	switch (mode & dumpMask) {
		default: fatal("FixMe");
		case dump_ast: dumpast(logf, s->cc->root, level); break;
		case dump_sym: dumpsym(logf, s->defs, level); break;
		case dump_asm: dumpasm(logf, s->vm, level); break;
	}
}

void dumpxml(FILE *fout, astn ast, int lev, const char* text, int level) {
	if (!ast) {
		return;
	}

	//~ fputfmt(fout, "%I<%s id='%d' oper='%?k'", lev, text, ast->kind, ast);
	fputfmt(fout, "%I<%s id='%t' oper='%?k'", lev, text, ast->kind, ast);
	if (level & prType) fputfmt(fout, " type='%?T'", ast->type);
	if (level & prCast) fputfmt(fout, " cast='%?t'", ast->csts);
	if (level & prLine) fputfmt(fout, " line='%d'", ast->line);
	if (level & prArgs) fputfmt(fout, " stmt='%?+k'", ast);
	switch (ast->kind) {
		default: fatal("FixMe");
		//{ STMT
		case STMT_do: {
			fputfmt(fout, ">\n");
			dumpxml(fout, ast->stmt.stmt, lev+1, "expr", level);
			fputfmt(fout, "%I</%s>\n", lev, text);
		} break;
		case STMT_beg: {
			astn l = ast->stmt.stmt;
			fputfmt(fout, ">\n");
			for (l = ast->stmt.stmt; l; l = l->next)
				dumpxml(fout, l, lev + 1, "stmt", level);
			fputfmt(fout, "%I</%s>\n", lev, text);
		} break;
		case STMT_if: {
			fputfmt(fout, ">\n");
			dumpxml(fout, ast->stmt.test, lev + 1, "test", level);
			dumpxml(fout, ast->stmt.stmt, lev + 1, "then", level);
			dumpxml(fout, ast->stmt.step, lev + 1, "else", level);
			fputfmt(fout, "%I</%s>\n", lev, text);
		} break;
		case STMT_for: {
			fputfmt(fout, ">\n");
			dumpxml(fout, ast->stmt.init, lev + 1, "init", level);
			dumpxml(fout, ast->stmt.test, lev + 1, "test", level);
			dumpxml(fout, ast->stmt.step, lev + 1, "step", level);
			dumpxml(fout, ast->stmt.stmt, lev + 1, "stmt", level);
			fputfmt(fout, "%I</stmt>\n", lev);
		} break;
		//}
		//{ OPER
		case OPER_fnc: {		// '()'
			astn arg = ast->op.rhso;
			fputfmt(fout, ">\n");
			if (arg) {
				while (arg && arg->kind == OPER_com) {
					dumpxml(fout, arg->op.rhso, lev + 1, "push", level);
					arg = arg->op.lhso;
				}
				dumpxml(fout, arg, lev + 1, "push", level);
			}
			dumpxml(fout, ast->op.lhso, lev + 1, "call", level);
			fputfmt(fout, "%I</%s>\n", lev, text);
		} break;
		case OPER_dot:		// '.'
		case OPER_idx:		// '[]'

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

		case ASGN_set: {		// '='
			fputfmt(fout, ">\n");
			//~ if (ast->kind == OPER_sel)
			dumpxml(fout, ast->op.test, lev + 1, "test", level);
			dumpxml(fout, ast->op.lhso, lev + 1, "lval", level);
			dumpxml(fout, ast->op.rhso, lev + 1, "rval", level);
			fputfmt(fout, "%I</%s>\n", lev, text, ast->kind, ast->type, ast);
		} break;
		//}*/
		//{ TVAL
		case EMIT_opc:
			fputfmt(fout, "/>\n", text);
			break;
		case TYPE_rec:
		case TYPE_enu:
			fputfmt(fout, ">ERROR</%s>\n", text);
			break;

		case TYPE_int:
		case TYPE_flt:
		case TYPE_str:
			fputfmt(fout, ">%k</%s>\n", ast, text);
			break;

		//~ case TYPE_ref: fputfmt(fout, ">%t[%T]</%s>\n", ast->id.link ? ast->id.link->kind : 0, ast->id.link, text); break;
		case TYPE_ref: {
			symn ref = ast->id.link;
			fputfmt(fout, ">%-T</%s>\n", ast->id.link, text);
			if (ref && ref->kind == TYPE_def && ref->init) {
				dumpxml(fout, ref->init, lev + 1, "init", level);
			}
		} break;
		/*case TYPE_def: {
			symn var = ast->id.link;
			if (var && var->init) {
				fputfmt(fout, ">\n");
				dumpxml(fout, var->init, lev + 1, "init", level);
				fputfmt(fout, "%I</%s>\n", lev, text);
			}
			else
				fputfmt(fout, "/>\n");
		} break;*/
		case TYPE_def: {
			symn var = ast->id.link;
			if (var && (var->args || var->init))
				fputfmt(fout, ">\n");
			else
				fputfmt(fout, "/>\n");

			if (var && var->args) {
				symn def = var->args;
				for (def = var->args; def; def = def->next) {
					fputfmt(fout, "%I<def id='%t' name='%+T'", lev + 1, def->kind, def);
					if (level & prType) fputfmt(fout, " type='%?T'", def->type);
					//~ if (level & prCast) fputfmt(fout, " cast='%?t'", ast->cst2);
					if (level & prLine) fputfmt(fout, " line='%d'", def->line);
					if (level & prArgs && def->init) fputfmt(fout, " stmt='%T = %?+k'", def, def->init);
					if (def->init) {
						fputfmt(fout, ">\n");
						dumpxml(fout, def->init, lev + 2, "init", level);
						fputfmt(fout, "%I</def>\n", lev + 1);
					}
					else
						fputfmt(fout, "/>\n");
				}
			}// */

			if (var && var->init) {
				//~ fputfmt(fout, ">\n");
				dumpxml(fout, var->init, lev + 1, "init", level);
			}

			if (var && (var->args || var->init))
				fputfmt(fout, "%I</%s>\n", lev, text);

		} break;
		//}
	}
}
//}
