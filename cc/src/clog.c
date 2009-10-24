//~ #include "clog.c"

#include "pvmc.h"
//~~~~~~~~~~ Output ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include <stdarg.h>
#include <string.h>

enum Format {
	noIden = 0x0100,
	nlBody = 0x0010,
	nlElIf = 0x0020,
};
static void fputsym(FILE *fout, symn sym, int mode, int level) {
	if (sym) switch (sym->kind) {

		// typename
		case TYPE_vid:
		case TYPE_bit:
		case TYPE_int:
		//~ case TYPE_i32:
		//~ case TYPE_i64:
		case TYPE_flt:
		//~ case TYPE_f32:
		//~ case TYPE_f64:
		/* TODO: types*/ {
			// TODO recurese
			fputfmt(fout, "%?s", sym->name);
		} break;
		case TYPE_enu:
		case TYPE_rec: {
			fputfmt(fout, "%I%?s", level, sym->name);
			if (mode > 2) {
				symn arg = sym->args;
				fputfmt(fout, "{\n");
				for (arg = sym->args; arg; arg = arg->next)
					fputsym(fout, arg, mode, level + 1);
				fputfmt(fout, "%I}\n", level);
			}
		} break;

		// variable
		case TYPE_arr: {
			//~ fputfmt(fout, "(");
			fputfmt(fout, "[%?+k]", sym->init);
			fputsym(fout, sym->type, mode, level);
			//~ fputfmt(fout, "):(%d)", sym->size);
		} break;

		case TYPE_ref: {
			if (mode > 0)
				fputsym(fout, sym->type, mode, 0);
			if (sym->name) {
				if (mode > 0)
					fputc(' ', fout);
				fputs(sym->name, fout);
			}
			if (sym->libc || sym->call) {
				fputc('(', fout);
				if (sym->args) {
					symn args = sym->args;
					while (args) {
						fputsym(fout, args->type, mode, 0);
						if ((args = args->next))
							fputs(", ", fout);
					}
				}
				fputc(')', fout);
			}
			if (mode > 1 && sym->init)
				fputfmt(fout, " = %+k", sym->init);
		} break;
		//~ TODO case TYPE_def: fputfmt("define %s", sym->name);
		default:
			//~ if (sym->kind <= TYPE_ref)
				//~ fputfmt(fout, "%s", sym->name);
			//~ else
			debug("%t('%s')", sym->kind, sym->name);
			break;
	}
	else fputs("(null)", fout);
}
static void fputast(FILE *fout, astn ast, int mode, int level) {
	int pre = ast ? ast->kind & 0x0f : 0;
	int noiden = mode & noIden;	// \n{ ...
	int nlelif = mode & nlElIf;	// ...}'\n'else ...
	int nlbody = mode & nlBody;	// \n{ ...
	//~ int nlelif = 1;	// else\n if (...) ...
	//~ int nlbody = 1;	// \n{ ...

	mode &= 0xf;
	/*if (ast && mode &&!noiden) switch (ast->kind) {
		case OPER_nop:
		case OPER_beg:
		case OPER_jmp:
		case OPER_for:
		case OPER_els:
		case OPER_end: fputfmt(fout, "%4d|", ast->line); break;
	}// */

	if (ast) switch (ast->kind) {
		//{ STMT
		case OPER_nop: {
			if (mode < 2) {
				//~ goto ferrast;
				if (mode > 0)
					fputfmt(fout, "%+k", ast->stmt);
				fputs(";", fout);
				break;
			}
			fputfmt(fout, "%I%+k;\n", noiden ? 0 : level, ast->rhso);
			//~ fputfmt(fout, "%8d|%I%+k;\n", ast->line, noiden ? 0 : level, ast->rhso);
		} break;
		case OPER_beg: {
			astn lst;
			if (mode < 2) {
				//~ goto ferrast;
				fputs("{}", fout);
				break;
			}
			//~ mode | nlbody | nlelif;
			fputfmt(fout, "%I%{\n", noiden ? 0 : level);
			for (lst = ast->stmt; lst; lst = lst->next)
				fputast(fout, lst, mode | nlbody | nlelif, level + 1);
			//~ fputfmt(fout, "% 4I|", 1);
			fputfmt(fout, "%I}\n", level, ast->type);
		} break;
		case OPER_jmp: {
			if (mode < 2) {
				fputs("if", fout);
				if (mode > 0)
					fputfmt(fout, " (%+k)", ast->test);
				break;
			}
			fputfmt(fout, "%Iif (%+k)", noiden ? 0 : level, ast->test);
			if (ast->stmt) {
				int kind = ast->stmt->kind;
				if (!nlbody && kind == OPER_beg) {
					fputfmt(fout, " ");
					fputast(fout, ast->stmt, mode | nlbody | nlelif | noIden, level);
				}
				else {
					fputfmt(fout, "\n");
					fputast(fout, ast->stmt, mode | nlbody | nlelif, level + (kind != OPER_beg));
				}
			}
			else fputfmt(fout, ";\n");
			if (ast->step) {
				int kind = ast->step->kind;
				if (!nlbody && (kind == OPER_beg)) {
					fputfmt(fout, "%Ielse ", level);
					fputast(fout, ast->step, mode | nlbody | nlelif | noIden, level);
				}
				else if (!nlelif && (kind == OPER_jmp)) {
					fputfmt(fout, "%Ielse ", level);
					fputast(fout, ast->step, mode | nlbody | nlelif | noIden, level);
				}
				else {
					fputfmt(fout, "%Ielse\n", level);
					fputast(fout, ast->step, mode | nlbody | nlelif, level + (kind != OPER_beg));
				}
			}
		} break;
		case OPER_for: {
			if (mode < 2) {
				fputs("for", fout);
				if (mode > 0)
					fputfmt(fout, " (%+k; %+k; %+k)", ast->init, ast->test, ast->step);
				break;
			}
			fputfmt(fout, "%Ifor (%+k; %+k; %+k)", noiden ? 0 : level, ast->init, ast->test, ast->step);
			if (ast->stmt) {
				int kind = ast->stmt->kind;
				if (!nlbody && kind == OPER_beg) {
					fputfmt(fout, " ");
					fputast(fout, ast->stmt, mode | nlbody | nlelif | noIden, level);
				}
				else {
					fputfmt(fout, "\n");
					fputast(fout, ast->stmt, mode | nlbody | nlelif, level + (kind != OPER_beg));
				}
				/*if (blst && !nlbody) {
					fputfmt(fout, "%Ifor (%+k; %+k; %+k) ", level, ast->init, ast->test, ast->step);
					fputast(fout, ast->stmt, mode | noIden, level);
				}
				else {
					fputfmt(fout, "%Ifor (%+k; %+k; %+k)\n", level, ast->init, ast->test, ast->step);
					fputast(fout, ast->stmt, mode, level + !blst);
				}*/
			}
			else fputfmt(fout, ";\n");
		} break;
		//} */ // STMT
		//{ OPER
		case OPER_dot: {	// '.'
			if (mode > 0) {
				fputast(fout, ast->lhso, mode, pre);
				fputc('.', fout);
				fputast(fout, ast->rhso, mode, pre);
			}
			else
				fputc('.', fout);
		} break;
		case OPER_fnc: {	// '()'
			if (mode > 0 && ast->lhso)
				fputast(fout, ast->lhso, mode, pre);
			fputc('(', fout);
			if (mode > 0 && ast->rhso)
				fputast(fout, ast->rhso, mode, pre);
			fputc(')', fout);
		} break;
		case OPER_idx: {	// '[]'
			if (mode > 0)
				fputast(fout, ast->lhso, mode, pre);
			fputc('[', fout);
			if (mode > 0 && ast->rhso)
				fputast(fout, ast->rhso, mode, pre);
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
			if (mode > 0) {
				if (level < pre) fputc('(', fout);
				if (ast->lhso) {
					fputast(fout, ast->lhso, mode, pre);
					fputc(' ', fout);
				}
				fputast(fout, ast, 0, 0);
				if (ast->lhso) fputc(' ', fout);
				fputast(fout, ast->rhso, mode, pre);
				if (level < pre) fputc(')', fout);
			}
			else switch (ast->kind) {
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
			if (mode > 0) {
				fputast(fout, ast->lhso, mode, pre);
				fputs(", ", fout);
				fputast(fout, ast->rhso, mode, pre);
			}
			else
				fputc(',', fout);
		} break;

		case OPER_sel: {	// '?:'
			if (mode > 0) {
				fputast(fout, ast->test, mode, pre);
				fputs(" ? ", fout);
				fputast(fout, ast->lhso, mode, pre);
				fputs(" : ", fout);
				fputast(fout, ast->rhso, mode, pre);
			}
			else
				fputs("?:", fout);
		}break;
		//}
		//{ TVAL
		case EMIT_opc: fputfmt(fout, "emit"); break;
		case CNST_int: fputfmt(fout, "%D", ast->cint); break;
		case CNST_flt: fputfmt(fout, "%F", ast->cflt); break;
		case CNST_str: fputfmt(fout, "'%s'", ast->name); break;
		case TYPE_ref: fputfmt(fout, "%s", ast->name); break;
		case TYPE_def: {
			symn sym = ast->link;
			symn typ = ast->type;
			if (mode < 1) {
				fputfmt(fout, "%s", ast->name);
			}
			else {
				switch (sym->kind) {
					case TYPE_def: fputfmt(fout, "define %s", ast->name); break;
					case TYPE_ref: fputfmt(fout, "%T %s", typ, ast->name); break;
				}
				if (!sym || !typ) {
					fputfmt(fout, ": ?err?");
				}
				else {
					if (sym->init) {
						//~ fputfmt(fout, " = %T(%+k)", sym->type, sym->init);
						fputfmt(fout, " = %+k", sym->init);
					}
					else {
						fputfmt(fout, " %+T", typ);
					}
				}
			}
		} break;
		//}

		default:
			//~ fputfmt(fout, "([0x%02x]:'%k')", ast->kind, ast->kind);
			fputfmt(fout, "([0x%02x]:'ERROR')", ast->kind);
			break;
	}
	else fputs("(null)", fout);
}

//~ void fputsym(FILE *fout, symn sym, int mode, int level);
//~ void fputast(FILE *fout, astn ast, int mode, int level);
void fputopc(FILE *fout, void *ip, int len, int offset);

static void FPUTFMT(FILE *fout, const char *msg, va_list ap) {
	char buff[1024], chr;
	int p = 0, plc = ',';
	while ((chr = *msg++)) {
		if (chr == '%') {
			char	nil = 0;	// [?]? skip on null || zero value
			char	sgn = 0;	// [+-]?
			char	pad = 0;	// [0 ]?
			long	len = 0;	// ([1-9][0-9]*)?
			long	prc = 0;	// (.[1-9][0-9]*)? precision
			char*	str = 0;	// the string to be printed
			//~ %(\?)?[+-]?[0 ]?([1-9][0-9]*)?(.[1-9][0-9]*)?[TkAIbBoOxXuUdDfFeEsScC]
			const char*	fmt = msg - 1;		// start of format string
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
				while (*msg >= '0' && *msg <= '9') {
					prc = (prc * 10) + (*msg - '0');
					msg++;
				}
			}
			switch (chr = *msg++) {
				case 0: return;//--msg; break;
				default: fputc(chr, fout); continue;
				case 't': {		// token
					unsigned arg = va_arg(ap, unsigned);
					dieif (arg > tok_last, "");
					str = (char*)tok_tbl[arg].name;
				} break;
				case 'T': {		// type
					symn sym = va_arg(ap, symn);
					if (!sym && nil)
						continue;
					switch (sgn) {
						case '-': while(sym) {
							fputsym(fout, sym, 0, 0);
							if ((sym = sym->next))
								fputs(", ", fout);
						} break;
						default:
							fputsym(fout, sym, sgn == '+', 0);
							break;
					}
				} continue;
				case 'k': {		// node
					astn ast = va_arg(ap, astn);
					if (!ast && nil)
						continue;
					switch (sgn) {
						case '-':
							fputast(fout, ast, 2, 0x0);	// walk
							break;
						case '+':
							fputast(fout, ast, noIden | 1, 0xf);	// tree
							break;
						case   0:
							fputast(fout, ast, noIden | 0, 0x0);	// astn
							break;
					}
				} continue;
				case 'A': {		// opcode
					void* opc = va_arg(ap, void*);
					if (nil && !opc) continue;
					fputopc(fout, opc, len, -1);
				} continue;

				case 'I': {		// ident
					unsigned arg = va_arg(ap, unsigned);
					len = len ? len * arg : arg;
					if (!pad) pad = '\t';
					str = (char*)"";
				} break;

				case 'b': {		// bin32
					uns32t num = va_arg(ap, uns32t);
					char *ptr = buff + sizeof(buff);
					*--ptr = 0;
					do {
						if (prc && ++p > prc) {
							*--ptr = plc;
							p = 1;
						}
						*--ptr = "01"[num%2];
						len -= 1;
					} while (num /= 2);
					str = ptr;
				} break;
				case 'B': {		// bin64
					uns64t num = va_arg(ap, uns64t);
					char *ptr = buff + sizeof(buff);
					*--ptr = 0;
					do {
						if (prc && ++p > prc) {
							*--ptr = plc;
							p = 1;
						}
						*--ptr = "01"[num%2];
						len -= 1;
					} while (num /= 2);
					str = ptr;
				} break;
				case 'o': {		// oct32
					uns32t num = va_arg(ap, uns32t);
					char *ptr = buff + sizeof(buff);
					*--ptr = 0;
					do {
						if (prc && ++p > prc) {
							*--ptr = plc;
							p = 1;
						}
						*--ptr = "01234567"[num%8];
						len -= 1;
					} while (num /= 8);
					str = ptr;
				} break;
				case 'O': {		// oct64
					uns64t num = va_arg(ap, uns64t);
					char *ptr = buff + sizeof(buff);
					*--ptr = 0;
					do {
						if (prc && ++p > prc) {
							*--ptr = plc;
							p = 1;
						}
						*--ptr = "01234567"[num%8];
						len -= 1;
					} while (num /= 8);
					str = ptr;
				} break;
				case 'x': {		// hex32
					uns32t num = va_arg(ap, uns32t);
					char *ptr = buff + sizeof(buff);
					*--ptr = 0;
					do {
						if (prc && ++p > prc) {
							*--ptr = plc;
							p = 1;
						}
						*--ptr = "0123456789abcdef"[num%16];
						len -= 1;
					} while (num /= 16);
					str = ptr;
				} break;
				case 'X': {		// hex64
					uns64t num = va_arg(ap, uns64t);
					char *ptr = buff + sizeof(buff);
					*--ptr = 0;
					do {
						if (prc && ++p > prc) {
							*--ptr = plc;
							p = 1;
						}
						*--ptr = "0123456789abcdef"[num%16];
						len -= 1;
					} while (num /= 16);
					str = ptr;
				} break;

				case 'u': 		// uns32
				case 'd': {		// dec32
					int neg = 0;
					uns32t num = va_arg(ap, uns32t);
					char *ptr = buff + sizeof(buff);
					if (chr == 'd' && (int32t)num < 0) {
						num = -(int32t)num;
						neg = -1;
					}
					*--ptr = 0;
					do {
						if (prc && ++p > prc) {
							*--ptr = plc;
							p = 1;
						}
						*--ptr = "0123456789"[num%10];
						len -= 1;
					} while (num /= 10);
					if (neg) *--ptr = '-';
					str = ptr;
				} break;

				case 'U': 		// uns64
				case 'D': {		// dec64
					int neg = 0;
					uns64t num = va_arg(ap, uns64t);
					char *ptr = buff + sizeof(buff);
					if (chr == 'D' && (int64t)num < 0) {
						num = -(int64t)num;
						neg = -1;
					}
					*--ptr = 0;
					do {
						if (prc && ++p > prc) {
							*--ptr = plc;
							p = 1;
						}
						*--ptr = "0123456789"[num % 10];
						len -= 1;
					} while (num /= 10);
					if (neg) *--ptr = '-';
					str = ptr;
				} break;

				case 'e':		// flt32
				case 'f':		// flt32
				case 'g': {		// flt32:TODO
					flt32t num = (flt32t)va_arg(ap, flt64t);
					if ((len =  msg - fmt - 1) < 1020) {
						memcpy(buff, fmt, len);
						buff[len++] = chr;
						buff[len++] = 0;
						fprintf(fout, buff, num);
					}
				} continue;

				case 'E':		// flt64
				case 'F':		// flt64
				case 'G': {		// flt64:TODO
					flt64t num = va_arg(ap, flt64t);
					if ((len = msg - fmt - 1) < 1020) {
						memcpy(buff, fmt, len);
						buff[len++] = 'l';
						buff[len++] = chr - 'A' + 'a';
						buff[len++] = 0;
						fprintf(fout, buff, num);
					}
				} continue;

				//~ case 'S': 		// wstr
				case 's': {		// cstr
					str = va_arg(ap, char*);
					//~ if (!str) continue;
					//~ if (str) len -= strlen(str);
				} break;

				//~ case 'C': 		// wchr
				case 'c': {		// cchr
					str = buff;
					str[0] = va_arg(ap, int);
					str[1] = 0;
					len -= 1;
				} break;
			}

			if (str && len > 0) {
				if (!pad) pad = ' ';
				while (len > 0) {
					fputc(pad, fout);
					len -= 1;
				}
			}
			if (str) fputs(str, fout);
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

void perr(state s, int level, const char *file, int line, const char *msg, ...) {
	FILE *fout =  (s && s && s->logf) ? s->logf : stderr;
	int warnl = s->cc ? s->cc->warn : 0;
	va_list argp;

	if (level) {
		if (level < 0 || warnl < 0) {
			if (file && line)
				fprintf(fout, "%s:%u:", file, line);
			fprintf(fout, "error:");
			s->errc += 1;
		}
		else if (level <= warnl) {
			if (file && line)
				fprintf(fout, "%s:%u:", file, line);
			fprintf(fout, "warn%d:", level);
		}
		else return;
	}
	else if (file && line)
		fprintf(fout, "%s:%u:", file, line);
	va_start(argp, msg);
	FPUTFMT(fout, msg, argp);
	fputc('\n', fout);
	va_end(argp);

	fflush(fout);
	//~ if (s->errc > 25)
		//~ cc_done(s);
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
 *	'$': function?
 *+	'*': operator?
 *+prot: protection level
 *	'+': public
 *	'-': private
 *	'=': protected?
 * 
**/
void dumpsym(FILE *fout, symn sym, int alma) {
	symn ptr, bs[TOKS], *ss = bs;
	symn *tp, bp[TOKS], *sp = bp;
	const int chrtyp = 0;
	for (*sp = sym; sp >= bp;) {
		char* tch = chrtyp ? "?" : "???: ";
		if (!(ptr = *sp)) {
			--sp, --ss;
			continue;
		}
		*sp = ptr->next;

		switch (ptr->kind) {

			// constant
			//~ case CNST_int:
			//~ case CNST_flt:
			//~ case CNST_str: tch = '#'; break;
			//~ case TYPE_def: tch = '#'; break;

			// variable/function
			case TYPE_ref: tch = chrtyp ? "$" : "var: "; break;

			// typename
			case TYPE_def:	// TODO
				//~ if (ptr->init) {
					tch = chrtyp ? "#" : "def: ";
					break;
				//~ }

			case TYPE_vid:

			case TYPE_bit:
			case TYPE_u32:

			case TYPE_int:
			case TYPE_i32:
			case TYPE_i64:

			case TYPE_flt:
			case TYPE_f32:
			case TYPE_f64:
			case TYPE_p4x:

			//~ case TYPE_arr:
			case TYPE_enu:
			case TYPE_rec: 
				*++sp = ptr->args;
				*ss++ = ptr;
				tch = chrtyp ? "^" : "typ: ";
				break;

			case EMIT_opc:
				tch = chrtyp ? "@" : "opc: ";
				break;

			default:
				debug("psym:%d:%T['%t']", ptr->kind, ptr, ptr->kind);
				tch = chrtyp ? "?" : "err: ";
				break;
		}

		if (ptr->file && ptr->line)
			fputfmt(fout, "%s:%d:", ptr->file, ptr->line);

		fputs(tch, fout);

		for (tp = bs; tp < ss; tp++) {
			symn typ = *tp;
			if (typ != ptr && typ && typ->name)
				fputfmt(fout, "%s.", typ->name);
		}// */

		fputfmt(fout, "%?s", ptr->name);

		if (ptr->kind == TYPE_ref && ptr->call) {
			symn arg = ptr->args;
			fputc('(', fout);
			while (arg) {
				fputfmt(fout, "%?s", arg->type->name);
				if (arg->name) fputfmt(fout, " %s", arg->name);
				if (arg->init) fputfmt(fout, "= %+k", arg->init);
				if ((arg = arg->next)) fputs(", ", fout);
			}
			fputc(')', fout);
		}

		if (ptr->kind == TYPE_ref) {
			if (ptr->offs < 0)
				fputfmt(fout, "@sp(%d)", ptr->offs);
			else
				fputfmt(fout, "@%xh", ptr->offs);
		}
		else if (ptr->kind == EMIT_opc) {
			fputfmt(fout, "@%xh", ptr->size);
		}
		else if (ptr->kind != TYPE_def) {
			fputfmt(fout, ":%d", ptr->size);
		}

		if (ptr->type)
			fputfmt(fout, ": %T", ptr->type);

		if (ptr->init)
			//~ fputfmt(fout, " = %?T(%k)", ptr->type, ptr->init);
			fputfmt(fout, " = %?T(%+k)", ptr->type, ptr->init);

		if (!alma) {
			fputc('\n', fout);
			fflush(fout);
			break;
		}

		fputc('\n', fout);
		fflush(fout);
	}
}
void dumpxml(FILE *fout, astn ast, int lev, const char* text, int level) {
	if (!ast) {
		return;
	}

	fputfmt(fout, "%I<%s id='%d' oper='%?k'", lev, text, ast->kind, ast);
	if (level & 1) fputfmt(fout, " type='%?T'", ast->type);
	if (level & 2) fputfmt(fout, " cast='%t'", ast->cast);
	if (level & 8) fputfmt(fout, " line='%d'", ast->line);
	if (level & 4) fputfmt(fout, " stmt='%+k'", ast);
	switch (ast->kind) {
		//{ STMT
		case OPER_nop: {
			fputfmt(fout, ">\n");
			dumpxml(fout, ast->stmt, lev+1, "expr", level);
			fputfmt(fout, "%I</%s>\n", lev, text);
		} break;
		case OPER_beg: {
			astn l = ast->stmt;
			fputfmt(fout, ">\n");
			for (l = ast->stmt; l; l = l->next)
				dumpxml(fout, l, lev + 1, "stmt", level);
			fputfmt(fout, "%I</%s>\n", lev, text);
		} break;
		case OPER_jmp: {
			fputfmt(fout, ">\n");
			dumpxml(fout, ast->test, lev + 1, "test", level);
			dumpxml(fout, ast->stmt, lev + 1, "then", level);
			dumpxml(fout, ast->step, lev + 1, "else", level);
			fputfmt(fout, "%I</%s>\n", lev, text);
		} break;
		case OPER_for: {
			fputfmt(fout, ">\n");
			dumpxml(fout, ast->init, lev + 1, "init", level);
			dumpxml(fout, ast->test, lev + 1, "test", level);
			dumpxml(fout, ast->step, lev + 1, "step", level);
			dumpxml(fout, ast->stmt, lev + 1, "stmt", level);
			fputfmt(fout, "%I</stmt>\n", lev);
		} break;
		//}

		//{ OPER
		case OPER_fnc: {		// '()'
			astn arg = ast->rhso;
			fputfmt(fout, ">\n");
			while (arg && arg->kind == OPER_com) {
				dumpxml(fout, arg->rhso, lev + 1, "push", level);
				arg = arg->lhso;
			}
			dumpxml(fout, arg, lev + 1, "push", level);
			dumpxml(fout, ast->lhso, lev + 1, "call", level);
			fputfmt(fout, "%I</%s>\n", lev, text);
		} break;
		case OPER_dot:		// '.'
		case OPER_idx:		// '[]'

		case OPER_pls:		// '+'
		case OPER_mns:		// '-'
		case OPER_cmt:		// '~'
		case OPER_not:		// '!'

		//~ case OPER_pow:		// '**'
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

		//~ case OPER_min:		// '?<'
		//~ case OPER_max:		// '?>'

		//~ case ASGN_pow:		// '**='
		//~ case ASGN_add:		// '+='
		//~ case ASGN_sub:		// '-='
		//~ case ASGN_mul:		// '*='
		//~ case ASGN_div:		// '/='
		//~ case ASGN_mod:		// '%='

		//~ case ASGN_shl:		// '>>='
		//~ case ASGN_shr:		// '<<='
		//~ case ASGN_and:		// '&='
		//~ case ASGN_ior:		// '|='
		//~ case ASGN_xor:		// '^='

		case ASGN_set: {		// '='
			fputfmt(fout, ">\n");
			//~ if (ast->kind == OPER_sel)
			dumpxml(fout, ast->test, lev + 1, "test", level);
			dumpxml(fout, ast->lhso, lev + 1, "lval", level);
			dumpxml(fout, ast->rhso, lev + 1, "rval", level);
			fputfmt(fout, "%I</%s>\n", lev, text, ast->kind, ast->type, ast);
		} break;
		//}*/

		//{ TVAL
		case EMIT_opc:
		case TYPE_def:
		case TYPE_enu:
		case TYPE_ref:
		case CNST_int:
		case CNST_flt:
		case CNST_str: {
			fputfmt(fout, "/>\n");
		} break;
		//}

		default: fatal("%t / %k", ast->kind, ast); break;
	}
}
void dumpast(FILE *fout, astn ast, int level) {
	if (level & 0x0f)
		dumpxml(fout, ast, 0, "root", level);
	else
		fputast(fout, ast, level | 2, 0);
}

void dump(state s, char *file, dumpMode mode, char *text) {
	FILE *fout = stdout;
	int level = mode & 0xff;

	if (file)
		fout = fopen(file, (mode & dump_new) ? "w" : "a");

	dieif(!fout, "cannot open file '%s'", file);

	if (text != NULL)
		fputs(text, fout);

	switch (mode & dumpMask) {
		case dump_sym: dumpsym(fout, s->cc->defs, level); break;
		case dump_ast: dumpast(fout, s->cc->root, level); break;
		case dump_asm: dumpasm(fout, s->vm, level); break;
		default: debug("invalid mode");
	}
	if (file)
		fclose(fout);
}
