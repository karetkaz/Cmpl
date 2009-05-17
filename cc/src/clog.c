//{ #include "clog.c"
//~ TODO remove fprintf
//~ TODO rename fputmsg : fputfmt

#include "main.h"
//~~~~~~~~~~ Output ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include <stdarg.h>
#include <string.h>

void fputast(FILE *fout, node ast, int mode, int level/*=0xf*/);
void fputsym(FILE *fout, defn sym, int mode, int level) {
	if (sym) switch (sym->kind) {
		// define
		case CNST_int : 
		case CNST_flt : 
		case CNST_str : {
			if (mode > 0 && sym->init)
				fputmsg(fout, "define %s = %?+k", sym->name, sym->init);
			else fputmsg(fout, "%s", sym->name);
		} break;

		/*case TYPE_def : {
			if (mode && sym->type) {
				fputmsg(fout, "define %s ", sym->name);
				fputsym(fout, sym->type, mode, level);
			}
			else fputmsg(fout, "?%?s!", sym->name);
		} break;*/

		// typename
		case TYPE_vid :
		case TYPE_bit :
		case TYPE_uns :
		case TYPE_int :
		case TYPE_f32 : 
		case TYPE_f64 : 
		case TYPE_flt : {
			// TODO recurese
			fputmsg(fout, "%?s", sym->name);
		} break;
		case TYPE_enu :
		case TYPE_rec : {
			fputmsg(fout, "%I%?s", level, sym->name);
			if (mode > 2) {
				defn arg = sym->args;
				fputmsg(fout, "{\n");
				for (arg = sym->args; arg; arg = arg->next)
					fputsym(fout, arg, mode, level + 1);
				fputmsg(fout, "%I}\n", level);
			}
		} break;

		// variable
		/*case TYPE_fnc : {
			fputsym(fout, sym->type, mode);
			if (sym->name) {
				fputc(' ', fout);
				fputs(sym->name, fout);
			}
			fputc('(', fout);
			if (sym->args) {
				defn args = sym->args;
				while (args) {
					fputsym(fout, args->type, mode);
					if ((args = args->next))
						fputs(", ", fout);
				}
			}
			fputc(')', fout);
		} break;
		case TYPE_arr : {
			fputsym(fout, sym->type, mode);
			fputmsg(fout, "[%?+k]", sym->init);
		} break;// */

		case TYPE_ref : {
			if (mode > 0)
				fputsym(fout, sym->type, mode, 0);
			if (sym->name) {
				if (mode > 0)
					fputc(' ', fout);
				fputs(sym->name, fout);
			}
			if (mode > 1 && sym->init)
				fputmsg(fout, " = %+k", sym->init);
		} break;
		default : 
			if (sym->kind < TYPE_ref)
				fputmsg(fout, "%s", sym->name);
			else
				fputmsg(fout, "[%d]?%s?", sym->kind, tok_tbl[sym->kind].name);
			break;
	}
	else fputs("(null)", fout);
}

void fputast(FILE *fout, node ast, int mode, int level) {
	int pre = ast ? ast->kind & 0x0f : 0;
	int nlbody = 0;	// \n{ ...
	int noiden = mode & 0x0100;	// \n{ ...
	//~ int nlelse = mode & 0x0100;	// ...}'\n'else ...

	mode &= 0xf;
	if (ast) switch (ast->kind) {
		//{ STMT
		case OPER_nop : {
			if (mode < 2) {
				goto ferrast;
				fputs(";", fout);
				return;
			}
			fputmsg(fout, "%I%+k;\n", level, ast->rhso);
		} break;
		case OPER_beg : {
			node lst;
			if (mode < 2) {
				goto ferrast;
				fputs("{}", fout);
				return;
			}
			fputmsg(fout, "%I{\n", noiden ? 0 : level);
			for (lst = ast->stmt; lst; lst = lst->next)
				fputast(fout, lst, mode, level + 1);
			fputmsg(fout, "%I}\n", level, ast->type);
		} break;
		case OPER_jmp : {
			if (mode < 2) {
				goto ferrast;
				if (mode > 0) {
					fputmsg(fout, "if (%+k)", ast->test);
				}
				else {
					fputmsg(fout, "if");
				}
				return;
			}

			fputmsg(fout, "%Iif (%+k)", noiden ? 0 : level, ast->test);
			if (ast->stmt) {
				if (ast->stmt->kind == OPER_beg) {
					if (nlbody) {
						fputc('\n', fout);
						fputast(fout, ast->stmt, mode, level);
					}
					else {
						fputc(' ', fout);
						fputast(fout, ast->stmt, mode | 0x0100, level);
					}
				}
				else {
					fputc('\n', fout);
					fputast(fout, ast->stmt, mode, level + 1);
				}
			}
			else fputs(" ;\n", fout);
			if (ast->step) {
				if (ast->step->kind == OPER_jmp) {	// else if
					fputmsg(fout, "%Ielse ", level);
					fputast(fout, ast->step, mode | 0x0100, level);
				}
				else if (ast->step->kind == OPER_beg) {	// else {

					fputmsg(fout, "%Ielse%c", level, nlbody ? '\n' : ' ');
					if (nlbody) {
						fputast(fout, ast->step, mode, level);
					}
					else {
						fputast(fout, ast->step, mode | 0x0100, level);
					}
				}
				else {
					fputmsg(fout, "%Ielse\n", level);
					fputast(fout, ast->step, mode, level + 1);
				}
			}
		} break;
		case OPER_for : {
			if (mode < 2) {
				if (mode > 0) {
					fputmsg(fout, "for (%+k; %+k; %+k)", ast->init, ast->test, ast->step);
				}
				else {
					fputmsg(fout, "for");
				}
				return;
			}


			if (ast->stmt) {
				fputmsg(fout, "%Ifor (%+k; %+k; %+k)\n", level, ast->init, ast->test, ast->step);
				fputast(fout, ast->stmt, mode, level + (ast->stmt->kind != OPER_beg));
			}
			else fputmsg(fout, "%Ifor (%+k; %+k; %+k);\n", level, ast->init, ast->test, ast->step);
		} break;
		//} */ // STMT

		//{ OPER
		case OPER_dot : {		// '.'
			if (mode > 0) {
				fputast(fout, ast->lhso, mode, pre);
				fputc('.', fout);
				fputast(fout, ast->rhso, mode, pre);
			}
			else
				fputc('.', fout);
		} break;
		case OPER_fnc : {		// '()'
			if (mode > 0 && ast->lhso)
				fputast(fout, ast->lhso, mode, pre);
			fputc('(', fout);
			if (mode > 0 && ast->rhso)
				fputast(fout, ast->rhso, mode, pre);
			fputc(')', fout);
		} break;
		case OPER_idx : {		// '[]'
			if (mode > 0)
				fputast(fout, ast->lhso, mode, pre);
			fputc('[', fout);
			if (mode > 0 && ast->rhso)
				fputast(fout, ast->rhso, mode, pre);
			fputc(']', fout);
		} break;

		case OPER_pls : 		// '+'
		case OPER_mns : 		// '-'
		case OPER_cmt : 		// '~'
		case OPER_not : 		// '!'

		case OPER_add : 		// '+'
		case OPER_sub : 		// '-'
		case OPER_mul : 		// '*'
		case OPER_div : 		// '/'
		case OPER_mod : 		// '%'

		case OPER_shl : 		// '>>'
		case OPER_shr : 		// '<<'
		case OPER_and : 		// '&'
		case OPER_ior : 		// '|'
		case OPER_xor : 		// '^'

		case OPER_equ : 		// '=='
		case OPER_neq : 		// '!='
		case OPER_lte : 		// '<'
		case OPER_leq : 		// '<='
		case OPER_gte : 		// '>'
		case OPER_geq : 		// '>='

		case ASGN_set : {		// '='
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
				case OPER_pls : fputs("+", fout); break;		// '+'
				case OPER_mns : fputs("-", fout); break;		// '-'
				case OPER_cmt : fputs("~", fout); break;		// '~'
				case OPER_not : fputs("!", fout); break;		// '!'

				case OPER_shl : fputs(">>", fout); break;		// '>>'
				case OPER_shr : fputs("<<", fout); break;		// '<<'
				case OPER_and : fputs("&", fout); break;		// '&'
				case OPER_ior : fputs("|", fout); break;		// '|'
				case OPER_xor : fputs("^", fout); break;		// '^'

				case OPER_equ : fputs("==", fout); break;		// '=='
				case OPER_neq : fputs("!=", fout); break;		// '!='
				case OPER_lte : fputs("<", fout); break;		// '<'
				case OPER_leq : fputs("<=", fout); break;		// '<='
				case OPER_gte : fputs(">", fout); break;		// '>'
				case OPER_geq : fputs(">=", fout); break;		// '>='

				case OPER_add : fputs("+", fout); break;		// '+'
				case OPER_sub : fputs("-", fout); break;		// '-'
				case OPER_mul : fputs("*", fout); break;		// '*'
				case OPER_div : fputs("/", fout); break;		// '/'
				case OPER_mod : fputs("%", fout); break;		// '%'
				
				case ASGN_set : fputs("=", fout); break;		// '%'
			}
		} break;

		case OPER_com : {
			if (mode > 0) {
				fputast(fout, ast->lhso, mode, pre);
				fputs(", ", fout);
				fputast(fout, ast->rhso, mode, pre);
			}
			else
				fputc(',', fout);
		} break;
		//}

		//{ TVAL
		case CNST_int : fputmsg(fout, "%D", ast->cint); break;
		case CNST_flt : fputmsg(fout, "%F", ast->cflt); break;
		case CNST_str : fputmsg(fout, "'%s'", ast->name); break;
		case TYPE_ref : fputmsg(fout, "%s", ast->name); break;
		//~ case TYPE_def : fputmsg(fout, "%s", ast->name); break;
		case EMIT_opc : fputmsg(fout, "emit"); break;
		//}

		default :	// ptint here a realy ugly thing
		ferrast :
			fputmsg(fout, "(?%?s[0x%02x])", tok_tbl[ast->kind].name, ast->kind);
			break;
	}
	else fputs("(null)", fout);
}

void fputfmt(FILE *fout, const char *msg, va_list ap) {
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
				case 0   : --msg; break;
				default  : fputc(chr, fout); continue;
				case 'T' : {		// sym type, list, ????
					defn sym = va_arg(ap, defn);
					if (nil && !sym)
						continue;
					if (sgn == '-') while(sym) {
						fputsym(fout, sym, 0, 0);
						if ((sym = sym->next))
							fputs(", ", fout);
					}
					else fputsym(fout, sym, sgn == '+', 0);
				} continue;
				case 'k' : {		// ast: node, expr, unit
					node ast = va_arg(ap, node);

					if (nil && !ast)
						continue;

					switch (sgn) {
						case '-' :
							fputast(fout, ast, 2, 0x0);	// walk
							break;
						case '+' :
							fputast(fout, ast, 1, 0xf);	// tree
							break;
						case  0  :
							fputast(fout, ast, 0, 0x0);	// node
							break;
					}
				} continue;
				case 'A' : {		// dasm (code)
					bcde opc = va_arg(ap, bcde);
					if (nil && !opc) continue;
					fputasm(fout, opc, len, -1);
				} continue;

				case 'I' : {		// ident
					unsigned arg = va_arg(ap, unsigned);
					len = len ? len*arg : arg;
					if (!pad) pad = '\t';
					str = (char*)"";
				} break;

				case 'b' : {		// bin32
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
				case 'B' : {		// bin64
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
				case 'o' : {		// oct32
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
				case 'O' : {		// oct64
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
				case 'x' : {		// hex32
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
				case 'X' : {		// hex64
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

				case 'u' : 		// uns32
				case 'd' : {		// dec32
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

				case 'U' : 		// uns64
				case 'D' : {		// dec64
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

				case 'e' :		// flt32
				case 'f' :		// flt32
				case 'g' : {		// flt32:TODO
					flt32t num = (flt32t)va_arg(ap, flt64t);
					if ((len =  msg - fmt - 1) < 1020) {
						memcpy(buff, fmt, len);
						buff[len++] = chr;
						buff[len++] = 0;
						fprintf(fout, buff, num);
					}
				} continue;

				case 'E' :		// flt64
				case 'F' :		// flt64
				case 'G' : {		// flt64:TODO
					flt64t num = va_arg(ap, flt64t);
					if ((len = msg - fmt - 1) < 1020) {
						memcpy(buff, fmt, len);
						buff[len++] = 'l';
						buff[len++] = chr - 'A' + 'a';
						buff[len++] = 0;
						fprintf(fout, buff, num);
					}
				} continue;

				//~ case 'S' : 		// wstr
				case 's' : {		// cstr
					str = va_arg(ap, char*);
					//~ if (!str) continue;
					//~ if (str) len -= strlen(str);
				} break;

				//~ case 'C' : 		// wchr
				case 'c' : {		// cchr
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
		//~ else if (chr == '\\')
		else fputc(chr, fout);
	}
}

void fputmsg(FILE *fout, const char *msg, ...) {
	va_list ap;
	if (!fout) fout = stdout;

	va_start(ap, msg);
	fputfmt(fout, (char *)msg, ap);
	//~ fputc('\n', fout);
	fflush(fout);
	va_end(ap);
}

//~ int cc_done(state);

void perr(state s, int level, const char *file, int line, const char *msg, ...) {
	FILE *fout =  (s && s->logf) ? s->logf : stderr;
	va_list argp;

	if (level) {
		if (level < 0 || s->warn < 0) {
			if (file && line)
				fprintf(fout, "%s:%u:", file, line);
			fprintf(fout, "error:");
			s->errc += 1;
		}
		else if (level <= s->warn) {
			if (file && line)
				fprintf(fout, "%s:%u:", file, line);
			fprintf(fout, "warn%d:", level);
		}
		else return;
	}
	else if (file && line)
		fprintf(fout, "%s:%u:", file, line);
	va_start(argp, msg);
	fputfmt(fout, msg, argp);
	fputc('\n', fout);
	va_end(argp);

	fflush(fout);
	//~ if (s->errc > 25)
		//~ cc_done(s);
}
//}
