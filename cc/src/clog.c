//{ #include "clog.c"
//~ TODO remove fprintf
//~ TODO rename fputmsg : fputfmt

#include "ccvm.h"
//~~~~~~~~~~ Output ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include <stdarg.h>
#include <string.h>
enum {
	//~ put_node = 0,

	//~ put_recn = 1,			// +
	put_tree = 1,
	put_next = 2,				// - enable next on expr/stmt
	//~ put_tree = 1,			// + recurse expr/stmt/decl
	put_type = 16,

	put_para = 4,
	put_cast = 8,

	//~ put_expr = 1,		// recurse expressions
	//~ put_stmt = 2,		// recurse statements
	//~ put_decl = 4,		// recurse declarations

	//~ walk_para = 2,		// ()
	//~ walk_qual = 3,		// qualify expressions int(int(a) + int(b))
	//~ walk_init = 3,		// print initialization

	//~ para_tree = 2,
	//~ type_expr = 2,		// ptint ex: int(int(a) + int(b))

	//~ walk_type = 4,
};

void fputast(FILE *fout, node ast, int mode, int pri);

void fputsym(FILE *fout, defn sym, int mode) {
	if (sym) switch (sym->kind) {
		// define
		case CNST_int : {
			if (mode && sym->init && sym->init->kind == CNST_int)
				fputmsg(fout, "define %s = %D", sym->name, sym->init->cint);
			else fputmsg(fout, "%s", sym->name);
		} break;
		case CNST_flt : {
			if (mode && sym->init && sym->init->kind == CNST_flt)
				fputmsg(fout, "define %s = %G", sym->name, sym->init->cflt);
			else fputmsg(fout, "%s", sym->name);
		} break;
		case CNST_str : {
			if (mode && sym->init && sym->init->kind == CNST_str)
				fputmsg(fout, "define %s = '%s'", sym->name, sym->init->name);
			else fputmsg(fout, "%s", sym->name);
		} break;
		case TYPE_def : {
			if (mode && sym->type) {
				fputmsg(fout, "define %s ", sym->name);
				fputsym(fout, sym->type, mode);
			}
			else fputmsg(fout, "%s", sym->name);
		} break;

		// typename
		case TYPE_vid :
		case TYPE_u08 :
		case TYPE_u16 :
		case TYPE_u32 :
		case TYPE_i08 :
		case TYPE_i16 :
		case TYPE_i32 :
		case TYPE_i64 :
		case TYPE_f32 :
		case TYPE_f64 :
		case TYPE_enu :
		case TYPE_rec : {
			// TODO recurese
			fputmsg(fout, "%?s", sym->name);
		} break;

		// variable
		case TYPE_fnc : {
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
		case TYPE_ref : {
			if (mode & put_type)
				fputsym(fout, sym->type, 1);
			if (sym->name) {
				if (mode & put_type)
					fputc(' ', fout);
				fputs(sym->name, fout);
			}
			if ((mode & put_tree) && sym->init)
				fputmsg(fout, " = %+k", sym->init);
		} break;
		case TYPE_arr : {
			fputsym(fout, sym->type, mode);
			fputmsg(fout, "[%?+k]", sym->init);
		} break;// */

		default : 
			fputmsg(fout, "[%d]?%s?", sym->kind, tok_tbl[sym->kind].name);
			break;
	}
	else fputs("(null)", fout);
}

void fputast(FILE *fout, node ast, int mode, int pri) {
	if (ast) switch (ast->kind) {
		/*/{ STMT
		case CODE_inf : {
			fputast(fout, ast->stmt, mode, pri);
			if ((mode & put_next) && (ast = ast->next))
				goto next_node;
		} break;
		case OPER_nop : {
			if (ast->type && ast->stmt) {
				node vars = ast->stmt;
				while (vars) {
					//~ defn var = vars->link;
					fputsym(fout, vars->link, mode);
					
					if ((vars = vars->next))
						fputs(", ", fout);
				}
				fputs(";\n", fout);
				//~ fputmsg(fout, "%IDECL: %-k;\n", pri, ast->stmt);
			}
			else if (ast->type) {
				fputmsg(fout, "%IDEFN: %+T;\n", pri, ast->type);
			}
			else if (ast->stmt) {
				//~ fputmsg(fout, "%IEXPR: %+k;\n", pri, ast->stmt);
				fputast(fout, ast->stmt, mode, 0xff);
				fputs(";\n", fout);
			}
			else
				fputmsg(fout, "%IEXPR: ???;\n", pri);
			if ((mode & put_next) && (ast = ast->next))
				goto next_node;
		} break;
		case OPER_beg : {
			defn ref = ast->type;
			fputmsg(fout, "%I{\n", pri);
			fputast(fout, ast->stmt, mode, pri+1);
			fputmsg(fout, "%I}\n", pri, ref);
			if ((ast = ast->next))
				goto next_node;
		} break;
		case OPER_jmp : {
			if (ast->test) {				// if()...
				fputmsg(fout, "%Iif(...)...;", pri);
			}
			else if (ast->lhso && ast->rhso) {		// goto
				fputmsg(fout, "%Igoto;", pri);
			}
			else if (ast->rhso) {				// break
				fputmsg(fout, "%Ibreak;", pri);
			}
			else if (ast->lhso) {				// continue
				fputmsg(fout, "%Icontinue;", pri);
			}
			if ((ast = ast->next))
				goto next_node;
		} break;
		case OPER_for : {
			fputmsg(fout, "%Ifor (%+k; %+k; %+k)\n", pri, ast->init, ast->test, ast->step);
			if (mode & put_tree && ast->stmt)
				fputast(fout, ast->stmt, mode, pri + (ast->stmt->kind != OPER_beg));
			if ((ast = ast->next))
				goto next_node;
		} break;
		//} */ // STMT

		case CNST_int : fputmsg(fout, "%D", ast->cint); break;
		case CNST_flt : fputmsg(fout, "%F", ast->cflt); break;
		case CNST_str : fputmsg(fout, "'%s'", ast->name); break;
		case TYPE_ref : fputmsg(fout, "%s", ast->name); break;

		default : if (mode && tok_tbl[ast->kind].argc) {
			int pre = tok_tbl[ast->kind].type;
			int par = (mode & put_para) || (pri < pre);
			//~ mode |= put_para|put_cast;
			//~ par = 1;
			if ((mode & put_cast) && par && ast->type)
				fputsym(fout, ast->type, 1);
			if (par) fputc('(', fout);
			if (ast->kind == OPER_sel) {
				fputast(fout, ast->test, mode, pre);
				fputs(" ? ", fout);
				fputast(fout, ast->lhso, mode, pre);
				fputs(" : ", fout);
				fputast(fout, ast->rhso, mode, pre);
			}
			else if (ast->kind == OPER_idx) {
				fputast(fout, ast->lhso, mode, pre);
				fputc('[', fout);
				fputast(fout, ast->rhso, mode, pre);
				fputc(']', fout);
			}
			else if (ast->kind == OPER_dot) {
				fputast(fout, ast->lhso, put_tree, pre);
				fputc('.', fout);
				fputast(fout, ast->rhso, put_tree, pre);
			}
			else if (ast->kind == OPER_fnc) {
				if (ast->lhso)
					fputast(fout, ast->lhso, mode, pre);
				fputc('(', fout);
				fputast(fout, ast->rhso, mode, pre);
				fputc(')', fout);
			}
			else {
				if (ast->lhso) {
					fputast(fout, ast->lhso, mode, pre);
					if (ast->kind != OPER_com) fputc(' ', fout);
				}
				fputast(fout, ast, 0, 0);
				if (ast->rhso) {
					if (ast->lhso) fputc(' ', fout);
					fputast(fout, ast->rhso, mode, pre);
				}
			}
			if (par) fputc(')', fout);
		} else if (tok_tbl[ast->kind].argc) {
			fputmsg(fout, "%?s", tok_tbl[ast->kind].name);
		} else {
			fputmsg(fout, "?%?s?", tok_tbl[ast->kind].name);
		} break;
	}
	else fputs("(null)", fout);
}

void fputasm(FILE *fout, bcde ip, int len, int offs) {
	int i;
	unsigned char* ptr = (unsigned char*)ip;
	if (offs >= 0) fprintf(fout, ".%04x ", offs);
	if (len > 1 && len < opc_tbl[ip->opc].size) {
		for (i = 0; i < len - 2; i++) {
			if (i < opc_tbl[ip->opc].size) fprintf(fout, "%02x ", ptr[i]);
			else fprintf(fout, "   ");
		}
		if (i < opc_tbl[ip->opc].size)
			fprintf(fout, "%02x... ", ptr[i]);
	}
	else for (i = 0; i < len; i++) {
		if (i < opc_tbl[ip->opc].size) fprintf(fout, "%02x ", ptr[i]);
		else fprintf(fout, "   ");
	}
	//~ if (!opc) return;
	if (opc_tbl[ip->opc].name)
		fputs(opc_tbl[ip->opc].name, fout);
	else fputmsg(fout, "opc%02x", ip->opc);
	switch (ip->opc) {
		case opc_jmp : 
		case opc_jnz : 
		case opc_jz : {
			if (offs < 0) fprintf(fout, " %+d", ip->jmp);
			else fprintf(fout, " .%04x", offs + ip->jmp);
		} break;
		/*case opc_bit : switch (ip->arg.u1 >> 5) {
			default : fprintf(fout, "bit.???"); break;
			case  1 : fprintf(fout, "shl %d", ip->arg.u1 & 0x1F); break;
			case  2 : fprintf(fout, "shr %d", ip->arg.u1 & 0x1F); break;
			case  3 : fprintf(fout, "sar %d", ip->arg.u1 & 0x1F); break;
			case  4 : fprintf(fout, "rot %d", ip->arg.u1 & 0x1F); break;
			//~ case  5 : fprintf(fout, "get %d", ip->arg.u1 & 0x1F); break;
			//~ case  6 : fprintf(fout, "set %d", ip->arg.u1 & 0x1F); break;
			//~ case  7 : fprintf(fout, "cmt %d", ip->arg.u1 & 0x1F); break;
			case  0 : switch (ip->arg.u1) {
				default : fprintf(fout, "bit.???"); break;
				case  1 : fprintf(fout, "bit.any"); break;
				case  2 : fprintf(fout, "bit.all"); break;
				case  3 : fprintf(fout, "bit.sgn"); break;
				case  4 : fprintf(fout, "bit.par"); break;
				case  5 : fprintf(fout, "bit.cnt"); break;
				case  6 : fprintf(fout, "bit.bsf"); break;		// most significant bit index
				case  7 : fprintf(fout, "bit.bsr"); break;
				case  8 : fprintf(fout, "bit.msb"); break;		// use most significant bit only
				case  9 : fprintf(fout, "bit.lsb"); break;
				case 10 : fprintf(fout, "bit.rev"); break;		// reverse bits
				// swp, neg, cmt, 
			} break;
		} break;*/
		case opc_pop : fprintf(fout, " %d", ip->idx); break;
		case opc_dup1 : case opc_dup2 :
		case opc_dup4 : fprintf(fout, " sp(%d)", ip->idx); break;
		case opc_set1 : case opc_set2 :
		case opc_set4 : fprintf(fout, " sp(%d)", ip->idx); break;
		case opc_ldc1 : fprintf(fout, " %d", ip->arg.i1); break;
		case opc_ldc2 : fprintf(fout, " %d", ip->arg.i2); break;
		case opc_ldc4 : fprintf(fout, " %d", (int)ip->arg.i4); break;
		case opc_ldc8 : fprintf(fout, " %Ld", (long long int)ip->arg.i8); break;
		case opc_ldcf : fprintf(fout, " %f", ip->arg.f4); break;
		case opc_ldcF : fprintf(fout, " %lf", ip->arg.f8); break;
		case opc_ldcr : fprintf(fout, " %x", (int)ip->arg.u4); break;
		//~ case opc_libc : fputmsg(fout, " %s %T", libcfnc[ip->idx].name, libcfnc[ip->idx].sym->type); break;
		//~ case opc_libc : fputmsg(fout, " %+T", libcfnc[ip->idx].sym); break;
		//~ case opc_libc : fprintf(fout, " %s", libcfnc[ip->idx].name); break;
		case opc_sysc : switch (ip->arg.u1) {
			case  0 : fprintf(fout, ".halt"); break;
			default : fprintf(fout, ".unknown"); break;
		} break;
		case p4d_swz  : {
			char c1 = "xyzw"[ip->arg.u1 >> 0 & 3];
			char c2 = "xyzw"[ip->arg.u1 >> 2 & 3];
			char c3 = "xyzw"[ip->arg.u1 >> 4 & 3];
			char c4 = "xyzw"[ip->arg.u1 >> 6 & 3];
			fprintf(fout, " %c%c%c%c(%02x)", c1, c2, c3, c4, ip->arg.u1);
		} break;
		//~ case i32_cmp  :
		//~ case f32_cmp  :
		//~ case i64_cmp  :
		case opc_cmp  : {
			//~ switch (ip->arg.u1 & 7) 
			fprintf(fout, "(%s)", "??\0eq\0lt\0le\0nz\0ne\0ge\0gt\0" + (3 * (ip->arg.u1 & 7)));
		} break;
	}
}

void fputfmt(FILE *fout, char *msg, va_list ap) {
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
			char*	fmt = msg - 1;		// start of format string
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
				case 'T' : {		// defn (type only)
					defn sym = va_arg(ap, defn);
					if (nil && !sym) continue;
					if (sgn == '-') while(sym) {
						fputsym(fout, sym, 0);
						if ((sym = sym->next))
							fputs(", ", fout);
					}
					else fputsym(fout, sym, sgn == '+');
				} continue;
				case 'k' : {		// tree (expr only)
					node ast = va_arg(ap, node);
					if (nil && !ast) continue;
					if (sgn == '-') while(ast) {
						fputast(fout, ast, 0, 0);
						if ((ast = ast->next))
							fputs(" ", fout);
					}
					else fputast(fout, ast, sgn == '+', 0xff);
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
					str = "";
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

void fputmsg(FILE *fout, char *msg, ...) {
	va_list ap;
	if (!fout) fout = stdout;

	va_start(ap, msg);
	fputfmt(fout, msg, ap);
	//~ fputc('\n', fout);
	fflush(fout);
	va_end(ap);
}

//~ int cc_done(state);

void perr(state s, int level, char *file, int line, char *msg, ...) {
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
