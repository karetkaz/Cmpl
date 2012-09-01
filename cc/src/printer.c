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

#define fputstr(__OUT, __STR) fputs(__STR, __OUT)
#define fputchr(__OUT, __CHR) fputc(__CHR, __OUT)

enum Format {
	noIden = 0x1000,
	//~ noName = 0x2000,

	alPars = 0x0100,
	nlBody = 0x0400,
	nlElIf = 0x0800,

	// sym & ast
	prType = 0x0010,

	prQual = 0x0020,
	prCast = 0x0020,

	prInit = 0x0040,

	//~ prArgs = 0x0080,
	prLine = 0x0080,
};

static void fputesc(FILE* fout, char* str) {
	int c;
	fputchr(fout, '\'');
	while ((c = *str++)) {
		switch (c) {
			default : fputchr(fout, c); break;
			case '\n': fputstr(fout, "\\n"); break;
			case '\t': fputstr(fout, "\\t"); break;
		}
	}
	fputchr(fout, '\'');
}

static void fputsym(FILE* fout, symn sym, int mode, int level) {
	int noiden = mode & noIden;	// internal use
	int prtype = mode & prType;
	int prinit = mode & prInit;
	int prqual = mode & prQual;
	int rlev = mode & 0xf;
	symn arg;

	if (sym) switch (sym->kind) {

		case TYPE_arr: {
			if (sym->name == NULL) {
				symn bp[512], *sp = bp, *p;// + sizeof(bp) / sizeof(*bp);

				symn typ = sym;
				while (typ->kind == TYPE_arr) {
					*sp++ = typ;
					typ = typ->type;
				}
				fputsym(fout, typ, mode, level);

				for (p = bp; p < sp; ++p) {
					typ = *p;
					fputfmt(fout, "[%?+k]", typ->init);
				}
				return;
			}
			else
				goto print_record;
		} break;

		case EMIT_opc: {
			if (prqual && sym->decl) {
				fputsym(fout, sym->decl, mode & prQual, 0);
				fputchr(fout, '.');
			}

			if (sym->name)
				fputstr(fout, sym->name);

			if (sym->init)
				fputfmt(fout, "(%+k)", sym->init);

		} break;

		print_record:
		case TYPE_rec: {
			if (rlev < 2) {
				fputstr(fout, sym->name);
				break;
			}

			fputfmt(fout, "%Istruct %?s {\n", noiden ? 0 : level, sym->name);

			for (arg = sym->args; arg; arg = arg->next) {
				fputsym(fout, arg, mode & ~prQual, level + 1);
				if (arg->kind != TYPE_rec)		// nested struct
					fputstr(fout, ";\n");
			}

			fputfmt(fout, "%I}\n", noiden ? 0 : level);

		} break;

		case TYPE_def:
		case TYPE_ref: {
			if (rlev < 1) {
				fputstr(fout, sym->name);
				break;
			}

			fputfmt(fout, "%I", noiden ? 0 : level);
			if (prtype) switch (sym->kind) {

				case TYPE_def:
					fputstr(fout, "define ");
					break;

				case TYPE_rec:
					fputstr(fout, "struct ");
					break;

				default:
					if (sym->type) {
						fputsym(fout, sym->type, noIden | prqual, 0);
						if (sym->name && *sym->name) {
							fputchr(fout, ' ');
						}
						switch (sym->cast) {
							default:
								break;
							case TYPE_ref:
								if (!sym->call && sym->type->cast != TYPE_ref)
									fputchr(fout, '&');
								break;
							//~ case TYPE_def:
								//~ fputchr(fout, '$');
								//~ break;
						}
					}
					break;
			}

			if (sym->name && *sym->name) {
				if (prqual && sym->decl) {
					fputsym(fout, sym->decl, prQual, 0);
					fputchr(fout, '.');
				}
				fputstr(fout, sym->name);
			}
			if (rlev > 0 && sym->call) {
				symn arg = sym->args;
				fputchr(fout, '(');
				while (arg) {
					fputsym(fout, arg, prType | 1, 0);
					if ((arg = arg->next))
						fputstr(fout, ", ");
				}
				fputchr(fout, ')');
			}
			/*if (rlev > 2 && sym->kind == TYPE_ref && sym->init) {
				fputast(fout, sym->init, mode | noIden, level);
			}
			else */
			if (prinit && sym->init) {
				fputfmt(fout, " = %+k", sym->init);
			}
		} break;

		default:
			fatal("FixMe(%s)", sym->name);
			break;
	}
	else fputstr(fout, "(null)");
}
static void fputast(FILE* fout, astn ast, int mode, int level) {
	int noiden = mode & noIden;	// internal use
	int nlelif = mode & nlElIf;	// ...}'\n'else '\n'? if ...
	int nlbody = mode & nlBody;	// '\n'? { ...
	int rlev = mode & 0xf;

	if (ast) switch (ast->kind) {
		//#{ STMT
		case STMT_do: {
			if (rlev < 2) {
				if (rlev > 0) {
					fputast(fout, ast->stmt.stmt, mode, 0xf);
				}
				fputchr(fout, ';');
				break;
			}
			if (ast->stmt.stmt->kind == TYPE_def) {
				fputast(fout, ast->stmt.stmt, mode, level);
				fputstr(fout, "\n");
			}
			else {
				fputfmt(fout, "%I", noiden ? 0 : level);
				fputast(fout, ast->stmt.stmt, mode, 0xf);
				fputstr(fout, ";\n");
			}
		} break;
		case STMT_beg: {
			astn lst;
			if (rlev < 2) {
				fputstr(fout, "{ ... }");
				break;
			}
			fputfmt(fout, "%I%{\n", noiden ? 0 : level);
			for (lst = ast->stmt.stmt; lst; lst = lst->next)
				fputast(fout, lst, mode & ~noIden, level + 1);
			fputfmt(fout, "%I}\n", level, ast->type);
		} break;
		case STMT_if:  {
			if (rlev < 2) {
				fputstr(fout, "if");
				if (rlev > 0) {
					fputfmt(fout, " (%+k)", ast->stmt.test);
				}
				break;
			}

			if (!noiden && level)
				fputfmt(fout, "%I", level);

			switch (ast->cst2) {
				case 0: break;
				case QUAL_sta:
					fputstr(fout, "static ");
					break;
				default:
					debug("error");
					break;
			}

			fputstr(fout, "if (");
			fputast(fout, ast->stmt.test, mode | noIden, 0xf);
			fputstr(fout, ")");

			if (ast->stmt.stmt) {
				int kind = ast->stmt.stmt->kind;
				if (!nlbody && kind == STMT_beg) {
					fputstr(fout, " ");
					fputast(fout, ast->stmt.stmt, mode | noIden, level);
				}
				else {
					fputstr(fout, "\n");
					fputast(fout, ast->stmt.stmt, mode, level + (kind != STMT_beg));
				}
			}
			else
				fputstr(fout, ";\n");
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
				fputstr(fout, "for");
				if (rlev > 0)
					fputfmt(fout, " (%+k; %+k; %+k)", ast->stmt.init, ast->stmt.test, ast->stmt.step);
				break;
			}
			fputfmt(fout, "%I", noiden ? 0 : level);
			switch (ast->cst2) {
				case 0: break;
				case QUAL_sta:
					fputstr(fout, "static ");
					break;
				case QUAL_par:
					fputstr(fout, "parallel ");
					break;
				default:
					debug("error");
					break;
			}

			fputstr(fout, "for (");
			if (ast->stmt.init)
				fputast(fout, ast->stmt.init, 1 | noIden, 0x0);

			fputstr(fout, "; ");
			if (ast->stmt.test)
				fputast(fout, ast->stmt.test, 1 | noIden, 0xf);

			fputstr(fout, "; ");
			if (ast->stmt.step)
				fputast(fout, ast->stmt.step, 1 | noIden, 0xf);

			fputstr(fout, ")");

			if (ast->stmt.stmt) {
				int kind = ast->stmt.stmt->kind;
				if (!nlbody && kind == STMT_beg) {
					fputstr(fout, " ");
					fputast(fout, ast->stmt.stmt, mode | noIden, level);
				}
				else {
					fputstr(fout, "\n");
					fputast(fout, ast->stmt.stmt, mode, level + (kind != STMT_beg));
				}
			}
			else
				fputstr(fout, ";\n");
		} break;
		case STMT_con:
		case STMT_brk: {
			if (rlev < 2) {
				switch (ast->kind) {
					default: fatal("FixMe");
					case STMT_con: fputstr(fout, "continue"); break;
					case STMT_brk: fputstr(fout, "break"); break;
				}
				break;
			}
			fputfmt(fout, "%I", noiden ? 0 : level);
			switch (ast->cst2) {
				case 0: break;
				default:
					debug("error");
					break;
			}
			switch (ast->kind) {
				default: fatal("FixMe");
				case STMT_con: fputstr(fout, "continue;\n"); break;
				case STMT_brk: fputstr(fout, "break;\n"); break;
			}
		} break;
		case STMT_ret: {
			if (rlev < 2) {
				fputstr(fout, "return");
				if (rlev > 0) {
					if (ast->stmt.stmt) {
						fputstr(fout, " (");
						fputast(fout, ast->stmt.stmt, mode | noIden, 0xf);
						fputchr(fout, ')');
					}
				}
				break;
			}
			fputfmt(fout, "%I", noiden ? 0 : level);
			switch (ast->cst2) {
				case 0: break;
				default:
					debug("error");
					break;
			}
			fputstr(fout, "return");
			if (ast->stmt.stmt) {
				fputstr(fout, " (");
				fputast(fout, ast->stmt.stmt, mode | noIden, 0xf);
				fputchr(fout, ')');
			}
		} break;
		//#}
		//#{ OPER
		case OPER_fnc: {	// '()'
			if (rlev > 0) {
				if (ast->op.lhso)
					fputast(fout, ast->op.lhso, mode, 0);
				fputchr(fout, '(');
				if (rlev && ast->op.rhso)
					fputast(fout, ast->op.rhso, mode, 0x0f);
				fputchr(fout, ')');
			}
			else
				fputstr(fout, "()");
		} break;
		case OPER_idx: {	// '[]'
			if (rlev > 0) {
				fputast(fout, ast->op.lhso, mode, 0);
				fputchr(fout, '[');
				if (rlev && ast->op.rhso)
					fputast(fout, ast->op.rhso, mode, 0);
				fputchr(fout, ']');
			}
			else
				fputstr(fout, "[]");
		} break;
		case OPER_dot: {	// '.'
			if (rlev > 0) {
				fputast(fout, ast->op.lhso, mode, 0);
				fputchr(fout, '.');
				fputast(fout, ast->op.rhso, mode, 0);
			}
			else
				fputchr(fout, '.');
		} break;

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
				int pre = ast->kind & 0xf;
				int putparen = level < pre;

				if (mode & prCast) {
					fputsym(fout, ast->type, prQual, 0);
					putparen = 1;
				}

				if (putparen)
					fputchr(fout, '(');

				if (ast->op.lhso) {
					fputast(fout, ast->op.lhso, mode, pre);
					fputchr(fout, ' ');
				}

				fputast(fout, ast, 0, 0);

				// in case of unary operators: '-a' not '- a'
				if (ast->op.lhso)
					fputchr(fout, ' ');

				fputast(fout, ast->op.rhso, mode, pre);

				if (putparen)
					fputchr(fout, ')');
			}
			else switch (ast->kind) {
				default: fatal("FixMe");
				case OPER_dot: fputstr(fout, "."); break;		// '.'
				case OPER_adr: fputstr(fout, "&"); break;		// '&'
				case OPER_pls: fputstr(fout, "+"); break;		// '+'
				case OPER_mns: fputstr(fout, "-"); break;		// '-'
				case OPER_cmt: fputstr(fout, "~"); break;		// '~'
				case OPER_not: fputstr(fout, "!"); break;		// '!'

				case OPER_shr: fputstr(fout, ">>"); break;		// '>>'
				case OPER_shl: fputstr(fout, "<<"); break;		// '<<'
				case OPER_and: fputstr(fout, "&"); break;		// '&'
				case OPER_ior: fputstr(fout, "|"); break;		// '|'
				case OPER_xor: fputstr(fout, "^"); break;		// '^'

				case OPER_equ: fputstr(fout, "=="); break;		// '=='
				case OPER_neq: fputstr(fout, "!="); break;		// '!='
				case OPER_lte: fputstr(fout, "<"); break;		// '<'
				case OPER_leq: fputstr(fout, "<="); break;		// '<='
				case OPER_gte: fputstr(fout, ">"); break;		// '>'
				case OPER_geq: fputstr(fout, ">="); break;		// '>='

				case OPER_add: fputstr(fout, "+"); break;		// '+'
				case OPER_sub: fputstr(fout, "-"); break;		// '-'
				case OPER_mul: fputstr(fout, "*"); break;		// '*'
				case OPER_div: fputstr(fout, "/"); break;		// '/'
				case OPER_mod: fputstr(fout, "%"); break;		// '%'

				case OPER_lnd: fputstr(fout, "&&"); break;		// '&'
				case OPER_lor: fputstr(fout, "||"); break;		// '|'

				case ASGN_set: fputstr(fout, "="); break;		// ':='
			}
		} break;

		case OPER_com: {
			if (rlev > 0) {
				fputast(fout, ast->op.lhso, mode, OPER_com);
				fputstr(fout, ", ");
				fputast(fout, ast->op.rhso, mode, OPER_com);
			}
			else
				fputchr(fout, ',');
		} break;

		case OPER_sel: {	// '?:'
			if (rlev > 0) {
				fputast(fout, ast->op.test, mode, OPER_sel);
				fputstr(fout, " ? ");
				fputast(fout, ast->op.lhso, mode, OPER_sel);
				fputstr(fout, " : ");
				fputast(fout, ast->op.rhso, mode, OPER_sel);
			}
			else
				fputstr(fout, "?:");
		}break;
		//#}
		//#{ TVAL
		case EMIT_opc: fputstr(fout, "emit"); break;
		case TYPE_bit: fputfmt(fout, "%U", ast->con.cint); break;
		case TYPE_int: fputfmt(fout, "%D", ast->con.cint); break;
		case TYPE_flt: fputfmt(fout, "%F", ast->con.cflt); break;
		case TYPE_str: fputesc(fout, ast->ref.name); break;
		case TYPE_def: {
			if (ast->ref.link) {
				fputsym(fout, ast->ref.link, mode|prInit|prType, level);
			}
			else {
				fputstr(fout, ast->ref.name);
			}
		} break;
		case TYPE_ref: {
			if (ast->ref.link) {
				fputsym(fout, ast->ref.link, 0, 0);
			}
			else {
				fputstr(fout, ast->ref.name);
			}
		} break;
		//#}

		case PNCT_lc: fputchr(fout, '['); break;
		case PNCT_rc: fputchr(fout, ']'); break;
		case PNCT_lp: fputchr(fout, '('); break;
		case PNCT_rp: fputchr(fout, ')'); break;
		default:
			fputfmt(fout, "%t(0x%02x)", ast->kind, ast->kind);
			break;
	}
	else fputstr(fout, "(null)");
}
static void dumpxml(FILE* fout, astn ast, int mode, int level, const char* text) {
	if (!ast) {
		return;
	}
	fputfmt(fout, "%I<%s op=\"%t\"", level, text, ast->kind);
	if (mode & prType) {
		fputfmt(fout, " type=\"%?T\"", ast->type);
	}
	if (mode & prCast) {
		fputfmt(fout, " cast=\"%?t\"", ast->cst2);
	}
	if (mode & prLine) {
		// && ast->file && ast->line) {
		fputfmt(fout, " line=\"%s:%d\"", ast->file, ast->line);
	}
	switch (ast->kind) {
		default:
			fatal("FixMe %t", ast->kind);
			break;
		//#{ STMT
		case STMT_do: {
			fputfmt(fout, " stmt=\"%?+k\">\n", ast);
			dumpxml(fout, ast->stmt.stmt, mode, level + 1, "expr");
			fputfmt(fout, "%I</%s>\n", level, text);
		} break;
		case STMT_beg: {
			astn l = ast->stmt.stmt;
			fputfmt(fout, ">\n");
			for (l = ast->stmt.stmt; l; l = l->next)
				dumpxml(fout, l, mode, level + 1, "stmt");
			fputfmt(fout, "%I</%s>\n", level, text);
		} break;
		case STMT_if: {
			fputfmt(fout, " stmt=\"%?+k\">\n", ast);
			dumpxml(fout, ast->stmt.test, mode, level + 1, "test");
			dumpxml(fout, ast->stmt.stmt, mode, level + 1, "then");
			dumpxml(fout, ast->stmt.step, mode, level + 1, "else");
			fputfmt(fout, "%I</%s>\n", level, text);
		} break;
		case STMT_for: {
			fputfmt(fout, " stmt=\"%?+k\">\n", ast);
			dumpxml(fout, ast->stmt.init, mode, level + 1, "init");
			dumpxml(fout, ast->stmt.test, mode, level + 1, "test");
			dumpxml(fout, ast->stmt.step, mode, level + 1, "step");
			dumpxml(fout, ast->stmt.stmt, mode, level + 1, "stmt");
			fputfmt(fout, "%I</%s>\n", level, text);
		} break;
		case STMT_brk: {
			fputfmt(fout, " />\n");
		} break;
		case STMT_ret: {
			fputfmt(fout, " stmt=\"%?+k\">\n", ast);
			dumpxml(fout, ast->stmt.stmt, mode, level + 1, "expr");
			fputfmt(fout, "%I</%s>\n", level, text);
		} break;
		//#}
		//#{ OPER
		case OPER_fnc: {	// '()'
			astn arg = ast->op.rhso;
			fputfmt(fout, ">\n");
			if (arg) {
				while (arg && arg->kind == OPER_com) {
					dumpxml(fout, arg->op.rhso, mode, level + 1, "push");
					arg = arg->op.lhso;
				}
				dumpxml(fout, arg, mode, level + 1, "push");
			}
			dumpxml(fout, ast->op.lhso, mode, level + 1, "call");
			fputfmt(fout, "%I</%s>\n", level, text);
		} break;
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

		case ASGN_set: {	// '='
			fputfmt(fout, " oper=\"%?+k\">\n", ast);
			dumpxml(fout, ast->op.test, mode, level + 1, "test");
			dumpxml(fout, ast->op.lhso, mode, level + 1, "lval");
			dumpxml(fout, ast->op.rhso, mode, level + 1, "rval");
			fputfmt(fout, "%I</%s>\n", level, text);
		} break;
		//#}
		//#{ TVAL
		case EMIT_opc:
			fputfmt(fout, " />\n", text);
			break;

		case TYPE_int:
		case TYPE_flt:
		case TYPE_str:
			fputfmt(fout, " value=\"%+k\" />\n", ast);
			break;

		case TYPE_ref:
			fputfmt(fout, ">%T</%s>\n", ast->ref.link, text);
			break;

		case TYPE_def: {
			symn var = ast->ref.link;

			fputfmt(fout, " name=\"%+T\"", var);
			if (var && (var->args || var->init)) {
				fputfmt(fout, ">\n");
			}
			else {
				fputfmt(fout, " />\n");
			}

			if (var && var->args) {
				symn def = var->args;
				for (def = var->args; def; def = def->next) {
					fputfmt(fout, "%I<def op=\"%t\"", level + 1, def->kind, def);
					if (mode & prType) {
						fputfmt(fout, " type=\"%?T\"", def->type);
					}
					if (mode & prCast) {
						fputfmt(fout, " cast=\"%?t\"", def->cast);
					}
					if (mode & prLine) {
						// && def->file && def->line) {
						fputfmt(fout, " line=\"%s:%d\"", def->file, def->line);
					}
					fputfmt(fout, " name=\"%+T\"", def);

					if (def->init) {
						fputfmt(fout, " value=\"%+k\"", def->init);
					}
					fputfmt(fout, " />\n");
				}
			}

			if (var && var->init)
				dumpxml(fout, var->init, mode, level + 1, "init");

			if (var && (var->args || var->init))
				fputfmt(fout, "%I</%s>\n", level, text);
		} break;
		//#}
	}
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

static void FPUTFMT(FILE* fout, const char* msg, va_list ap) {
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
					return;

				default:
					fputchr(fout, chr);
					continue;

				case 'T': {		// type
					symn sym = va_arg(ap, symn);
					if (!sym && nil)
						continue;
					switch (sgn) {
						case '-': fputsym(fout, sym, prType|prQual | 1, 0); break;
						case '+': fputsym(fout, sym, prQual | 1, 0); break;
						default:  fputsym(fout, sym, 0, 0); break;
					}
				} continue;
				case 'k': {		// node
					astn ast = va_arg(ap, astn);
					if (!ast && nil)
						continue;
					switch (sgn) {
						case '-': fputast(fout, ast, noIden | 2, prc); break;	// walk
						case '+': fputast(fout, ast, noIden | 1, 0x0f); break;	// tree
						default:  fputast(fout, ast, noIden | 0, 0); break;		// astn
					}
				} continue;
				case 'K': {		// node
					astn ast = va_arg(ap, astn);
					if (!ast && nil)
						continue;
					dumpxml(fout, ast, len << 4, 0, "node");
				} continue;
				case 'A': {		// opcode
					void* opc = va_arg(ap, void*);
					if (nil && !opc)
						continue;
					fputopc(fout, opc, len, prc, NULL);
				} continue;
				case 't': {		// token
					unsigned arg = va_arg(ap, unsigned);
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
				case 'G':
					chr -= 'A' - 'a';
					// no break

				case 'e':		// float32
				case 'f':		// float32
				case 'g': {		// float32
					float64_t num = va_arg(ap, float64_t);
					if ((len = msg - fmt - 1) < 1020) {
						memcpy(buff, fmt, len);
						buff[len++] = chr;
						buff[len++] = 0;
						//~ TODO: replace fprintf
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
					fputchr(fout, pad);
					len -= 1;
				}

				fputstr(fout, str);
			}
		}
		else
			fputchr(fout, chr);
	}
}

void fputfmt(FILE* fout, const char* msg, ...) {
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
void dumpsym(FILE* fout, symn sym, int mode) {
	symn ptr, bp[TOKS], *sp = bp;

	int print_line = mode & prLine;		// print file and location
	int print_type = mode & prType;
	int print_init = mode & prInit;
	int print_info = 1;
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

			// constant/enum
			case TYPE_def:
				tch = "def";
				break;

			// typename/array
			case TYPE_arr:
			case TYPE_rec:
				tch = "typ";
				break;

			// variable/function
			case TYPE_ref:
				tch = "var";
				break;

			case EMIT_opc:
				tch = "opc";
				break;

			default:
				debug("psym:%d:%T['%t']", ptr->kind, ptr, ptr->kind);
				tch = "err";
				break;
		}

		switch (ptr->kind) {

			case TYPE_def:
			case TYPE_ref:
				if (mode > 3)
					*++sp = ptr->args;
				break;

			case EMIT_opc:
				if (mode > 2)
					*++sp = ptr->args;
				break;

			default:
				if (mode > 1)
					*++sp = ptr->args;
				break;
		}

		if (print_line && ptr->file && ptr->line)
			fputfmt(fout, "%s:%d:", ptr->file, ptr->line);

		// qualified name with arguments
		fputsym(fout, ptr, prQual | 1, 0);

		// qualified base type(s)
		if (print_type && ptr->type) {
			fputstr(fout, ": ");
			fputsym(fout, ptr->type, prQual, 0);
		}

		// initializer
		if (print_init && ptr->init && ptr->kind != TYPE_ref) {
			fputstr(fout, " = ");
			fputast(fout, ptr->init, noIden | 1, 0xf0);
		}

		if (print_info) {
			fputfmt(fout, ": [%c%06x", ptr->stat ? '@' : '+', ptr->offs);
			fputfmt(fout, ", size: %d", ptr->size);
			fputfmt(fout, ", kind: %s", tch);
			if (ptr->cnst)
				fputfmt(fout, ", const");
			if (ptr->stat)
				fputfmt(fout, ", static");
			if (ptr->cast != 0)
				fputfmt(fout, ", cast: %t", ptr->cast);
			fputstr(fout, "]");
		}

		fputchr(fout, '\n');
		if (print_used && ptr->used) {
			astn use = ptr->used;
			while (use) {
				fputfmt(fout, "%s:%d: referenced as `%+k`\n", use->file, use->line, use);
				use = use->ref.used;
			}
		}

		fflush(fout);
		if (!mode)
			break;
	}
}

int logFILE(state rt, FILE* file) {
	if (rt->closelog)
		fclose(rt->logf);
	rt->logf = file;
	rt->closelog = 0;
	return 0;
}
int logfile(state rt, char* file) {

	logFILE(rt, NULL);

	if (file) {
		rt->logf = fopen(file, "wb");
		if (!rt->logf) return -1;
		rt->closelog = 1;
	}
	return 0;
}

void dump(state rt, int mode, symn sym, char* text, ...) {
	FILE* logf = rt ? rt->logf : stdout;
	int level = mode & 0xff;

	if (!logf)
		return;

	if (text != NULL) {
		va_list ap;
		va_start(ap, text);
		FPUTFMT(logf, text, ap);
		va_end(ap);
		fflush(logf);
	}

	if (sym) {
		if (mode & dump_sym) {
			dumpsym(logf, sym, level);
		}
		if (mode & dump_ast) {
			if (sym->kind == TYPE_ref && sym->call) {
				if ((level & 0x0f) == 0x0f)
					dumpxml(logf, sym->init, level, 0, "code");
				else
					fputast(logf, sym->init, level | 2, 0);
			}
		}
		if (mode & dump_asm) {
			if (sym->kind == TYPE_ref && sym->call) {
				fputasm(logf, rt, sym->offs, sym->offs + sym->size, 0x119);
			}
		}
	}
	else {
		if (mode & dump_sym) {
			symn glob = rt->defs;

			if ((level & 0x0f) == 0x0f) {
				while (glob) {
					dumpsym(logf, glob, 0);
					glob = glob->defs;
				}
			}
			else {
				dumpsym(logf, glob, level);
			}
		}
		if (mode & dump_ast) {
			if ((level & 0x0f) == 0x0f)
				dumpxml(logf, rt->cc->root, level, 0, "root");
			else
				fputast(logf, rt->cc->root, level | 2, 0);
		}

		if (mode & dump_asm) {
			symn var;
			for (var = rt->defs; var; var = var->next) {
				if (var->kind == TYPE_ref && var->call) {
					symn arg = var->args;
					fputfmt(logf, "%-T [@%06x: %d] {\n", var, var->offs, var->size);
					for (; arg; arg = arg->next) {
						fputfmt(logf, "\targ %-T [@%06x, size:%d, cast:%t]\n", arg, arg->offs, arg->size, arg->cast);
					}
					fputasm(logf, rt, var->offs, var->offs + var->size, mode);
					fputfmt(logf, "}\n");
				}
			}
			fputfmt(logf, "init(ro: %d"
				", ss: %d"
				", sm: %d"
				", pc: %d"
				", px: %d"
				", size.meta: %d"
				", size.code: %d"
				", size.data: %d"
			") {\n", rt->vm.ro, rt->vm.ss, rt->vm.sm, rt->vm.pc, rt->vm.px, rt->vm.size.meta, rt->vm.size.code, rt->vm.size.data);

			fputasm(logf, rt, rt->vm.pc, rt->vm.px, mode);
			fputfmt(logf, "}\n");
		}

		if (mode & dump_bin) {
			unsigned int i, brk = level & 0xff;

			if (brk == 0)
				brk = 16;

			for (i = 0; i < rt->vm.pos; i += 1) {
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
}

void perr(state rt, int level, const char* file, int line, const char* msg, ...) {
	FILE* logf = rt ? rt->logf : stderr;
	int warnl = rt && rt->cc ? rt->cc->warn : 0;
	va_list argp;

	if (level >= 0) {
		if (warnl < 0)
			level = warnl;
		if (level > warnl)
			return;
	}
	else if (rt) {
		rt->errc += 1;
	}

	if (!logf)
		return;

	if (rt && rt->cc && !file)
		file = rt->cc->file;

	if (file && line)
		fputfmt(logf, "%s:%u: ", file, line);

	if (level > 0)
		fputstr(logf, "warning: ");
	else if (level)
		fputstr(logf, "error: ");

	va_start(argp, msg);
	FPUTFMT(logf, msg, argp);
	fputchr(logf, '\n');
	fflush(logf);
	va_end(argp);
}
