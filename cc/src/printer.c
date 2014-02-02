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
	noIden = 0x1000,		// TODO: to be removed: use -level instead.
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
	prLine = 0x0080
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
	int no_iden = mode & noIden;	// internal use
	int pr_type = mode & prType;
	int pr_init = mode & prInit;
	int pr_qual = mode & prQual;
	int rlev = mode & 0xf;

	if (sym == NULL) {
		fputstr(fout, "(null)");
		return;
	}

	switch (sym->kind) {

		case EMIT_opc: {
			if (sym->name) {
				if (pr_qual && sym->decl) {
					fputsym(fout, sym->decl, mode, 0);
					fputchr(fout, '.');
				}
				fputstr(fout, sym->name);
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
				fputsym(fout, typ, mode, level);

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
					fputsym(fout, sym->decl, mode, 2);
					fputchr(fout, '.');
				}
				fputstr(fout, sym->name);
				break;
			}

			fputfmt(fout, "%Istruct %?s {\n", no_iden ? 0 : level, sym->name);

			for (arg = sym->flds; arg; arg = arg->next) {
				fputsym(fout, arg, mode & ~prQual, level + 1);
				if (arg->kind != TYPE_rec)		// nested struct
					fputstr(fout, ";\n");
			}

			fputfmt(fout, "%I}\n", no_iden ? 0 : level);

			break;
		}

		case TYPE_def:
		case TYPE_ref: {
			if (rlev < 1) {
				fputstr(fout, sym->name);
				break;
			}

			fputfmt(fout, "%I", no_iden ? 0 : level);

			if (pr_type) switch (sym->kind) {
				case TYPE_def:
					fputstr(fout, "define ");
					break;

				case TYPE_rec:
					fputstr(fout, "struct ");
					break;

				default:
					if (sym->type) {
						fputsym(fout, sym->type, noIden | pr_qual, 0);
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
					fputsym(fout, sym->decl, prQual, 0);
					fputchr(fout, '.');
				}
				fputstr(fout, sym->name);
			}

			if (rlev > 0 && sym->call) {
				symn arg = sym->prms;
				fputchr(fout, '(');
				while (arg) {
					fputsym(fout, arg, prType | 1, 0);
					if ((arg = arg->next))
						fputstr(fout, ", ");
				}
				fputchr(fout, ')');
			}

			if (pr_init && sym->init) {
				fputfmt(fout, " = %+k", sym->init);
			}

			break;
		}

		default:
			fatal("FixMe(%s:%k)", sym->name, sym->kind);
			break;
	}
}

static void fputast(FILE* fout, astn ast, int mode, int level) {
	int no_iden = mode & noIden;	// internal use
	int nl_body = mode & nlBody;	// '\n'? { ...
	int nl_elif = mode & nlElIf;	// ...}'\n'else '\n'? if ...
	int rlev = mode & 0xf;

	if (ast == NULL) {
		fputstr(fout, "(null)");
		return;
	}

	switch (ast->kind) {
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
				fputfmt(fout, "%I", no_iden ? 0 : level);
				fputast(fout, ast->stmt.stmt, mode, 0xf);
				fputstr(fout, ";\n");
			}
		} break;
		case STMT_beg: {
			astn list;
			if (rlev < 2) {
				fputstr(fout, "{ ... }");
				break;
			}

			fputfmt(fout, "%I%{\n", no_iden ? 0 : level);
			for (list = ast->stmt.stmt; list; list = list->next) {
				fputast(fout, list, mode & ~noIden, level + 1);
			}
			fputfmt(fout, "%I} %-T\n", level, ast->type);
		} break;
		case STMT_if: {
			if (rlev == 0) {
				fputstr(fout, "if");
				break;
			}

			if (!no_iden && level) {
				fputfmt(fout, "%I", level);
			}

			switch (ast->cst2) {
				default:
					debug("error");
				case TYPE_any:
					break;

				case QUAL_sta:
					fputstr(fout, "static ");
					break;
			}

			fputstr(fout, "if (");
			fputast(fout, ast->stmt.test, mode | noIden, 0xf);
			fputstr(fout, ")");

			if (rlev < 2) {
				break;
			}
			// if then part
			if (ast->stmt.stmt != NULL) {
				int kind = ast->stmt.stmt->kind;
				if (!nl_body && kind == STMT_beg) {
					fputstr(fout, " ");
					fputast(fout, ast->stmt.stmt, mode | noIden, level);
				}
				else {
					fputstr(fout, "\n");
					fputast(fout, ast->stmt.stmt, mode, level + (kind != STMT_beg));
				}
			}
			else {
				fputstr(fout, ";\n");
			}

			// if else part
			if (ast->stmt.step != NULL) {
				int kind = ast->stmt.step->kind;
				if (!nl_body && (kind == STMT_beg)) {
					fputfmt(fout, "%Ielse ", level);
					fputast(fout, ast->stmt.step, mode | noIden, level);
				}
				else if (!nl_elif && (kind == STMT_if)) {
					fputfmt(fout, "%Ielse ", level);
					fputast(fout, ast->stmt.step, mode | noIden, level);
				}
				else {
					fputfmt(fout, "%Ielse\n", level);
					fputast(fout, ast->stmt.step, mode, level + (kind != STMT_beg));
				}
			}

			break;
		}
		case STMT_for: {
			if (rlev < 2) {
				fputstr(fout, "for");
				if (rlev > 0) {
					fputfmt(fout, " (%?+ k; %?+k; %?+k)", ast->stmt.init, ast->stmt.test, ast->stmt.step);
				}
				break;
			}

			fputfmt(fout, "%I", no_iden ? 0 : level);
			switch (ast->cst2) {
				default:
					debug("error");
				case TYPE_any:
					break;

				case QUAL_sta:
					fputstr(fout, "static ");
					break;

				case QUAL_par:
					fputstr(fout, "parallel ");
					break;
			}

			fputstr(fout, "for (");
			if (ast->stmt.init) {
				fputast(fout, ast->stmt.init, 1 | noIden, 0x0);
			}

			fputstr(fout, "; ");
			if (ast->stmt.test) {
				fputast(fout, ast->stmt.test, 1 | noIden, 0xf);
			}

			fputstr(fout, "; ");
			if (ast->stmt.step) {
				fputast(fout, ast->stmt.step, 1 | noIden, 0xf);
			}

			fputstr(fout, ")");

			if (ast->stmt.stmt != NULL) {
				int kind = ast->stmt.stmt->kind;
				if (!nl_body && kind == STMT_beg) {
					fputstr(fout, " ");
					fputast(fout, ast->stmt.stmt, mode | noIden, level);
				}
				else {
					fputstr(fout, "\n");
					fputast(fout, ast->stmt.stmt, mode, level + (kind != STMT_beg));
				}
			}
			else {
				fputstr(fout, ";\n");
			}
			break;
		}
		case STMT_con:
		case STMT_brk: {
			if (rlev < 2) {
				switch (ast->kind) {
					default:
						fatal("FixMe");
						break;

					case STMT_con:
						fputstr(fout, "continue");
						break;

					case STMT_brk:
						fputstr(fout, "break");
						break;
				}
				break;
			}

			fputfmt(fout, "%I", no_iden ? 0 : level);
			switch (ast->cst2) {
				case 0:
					break;
				default:
					debug("error");
					break;
			}
			switch (ast->kind) {
				default:
					fatal("FixMe");
					break;

				case STMT_con:
					fputstr(fout, "continue;\n");
					break;

				case STMT_brk:
					fputstr(fout, "break;\n");
					break;
			}
			break;
		}
		case STMT_ret: {
			if (rlev < 2) {
				fputstr(fout, "return");
				if (rlev > 0) {
					if (ast->stmt.stmt != NULL) {
						fputstr(fout, " (");
						fputast(fout, ast->stmt.stmt, mode | noIden, 0xf);
						fputchr(fout, ')');
					}
				}
				break;
			}

			fputfmt(fout, "%I", no_iden ? 0 : level);
			switch (ast->cst2) {
				default:
					debug("error");
				case TYPE_any:
					break;
			}
			fputstr(fout, "return");
			if (ast->stmt.stmt) {
				fputstr(fout, " (");
				fputast(fout, ast->stmt.stmt, mode | noIden, 0xf);
				fputchr(fout, ')');
			}
			break;
		}
		//#}
		//#{ OPER
		case OPER_fnc: {	// '()'
			if (rlev > 0) {
				if (ast->op.lhso != NULL) {
					fputast(fout, ast->op.lhso, mode, 0);
				}
				fputchr(fout, '(');
				if (rlev && ast->op.rhso != NULL) {
					fputast(fout, ast->op.rhso, mode, 0x0f);
				}
				fputchr(fout, ')');
			}
			else {
				fputstr(fout, "()");
			}
			break;
		}

		case OPER_idx: {	// '[]'
			if (rlev > 0) {
				fputast(fout, ast->op.lhso, mode, 0);
				fputchr(fout, '[');
				if (rlev && ast->op.rhso!= NULL) {
					fputast(fout, ast->op.rhso, mode, 0);
				}
				fputchr(fout, ']');
			}
			else {
				fputstr(fout, "[]");
			}
			break;
		}

		case OPER_dot: {	// '.'
			if (rlev > 0) {
				fputast(fout, ast->op.lhso, mode, 0);
				fputchr(fout, '.');
				fputast(fout, ast->op.rhso, mode, 0);
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
				int pre = ast->kind & 0xf;
				int putparen = level < pre;

				if (mode & prCast) {
					fputsym(fout, ast->type, prQual, 0);
					putparen = 1;
				}

				if (putparen) {
					fputchr(fout, '(');
				}

				if (ast->op.lhso != NULL) {
					fputast(fout, ast->op.lhso, mode, pre);
					fputchr(fout, ' ');
				}

				fputast(fout, ast, 0, 0);

				// in case of unary operators: '-a' not '- a'
				if (ast->op.lhso != NULL) {
					fputchr(fout, ' ');
				}

				fputast(fout, ast->op.rhso, mode, pre);

				if (putparen) {
					fputchr(fout, ')');
				}
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
			break;
		}

		case OPER_com: {
			if (rlev > 0) {
				fputast(fout, ast->op.lhso, mode, OPER_com);
				fputstr(fout, ", ");
				fputast(fout, ast->op.rhso, mode, OPER_com);
			}
			else {
				fputchr(fout, ',');
			}
			break;
		}

		case OPER_sel: {	// '?:'
			if (rlev > 0) {
				fputast(fout, ast->op.test, mode, OPER_sel);
				fputstr(fout, " ? ");
				fputast(fout, ast->op.lhso, mode, OPER_sel);
				fputstr(fout, " : ");
				fputast(fout, ast->op.rhso, mode, OPER_sel);
			}
			else {
				fputstr(fout, "?:");
			}
			break;
		}
		//#}
		//#{ TVAL
		case EMIT_opc:
			fputstr(fout, "emit");
			break;

		case TYPE_bit:
			fputfmt(fout, "%U", ast->con.cint);
			break;

		case TYPE_int:
			fputfmt(fout, "%D", ast->con.cint);
			break;

		case TYPE_flt:
			fputfmt(fout, "%F", ast->con.cflt);
			break;

		case TYPE_str:
			fputesc(fout, ast->ref.name);
			break;

		case TYPE_def: {
			if (ast->ref.link) {
				fputsym(fout, ast->ref.link, mode|prInit|prType, level);
			}
			else {
				fputstr(fout, ast->ref.name);
			}
			break;
		}
		case TYPE_ref: {
			if (ast->ref.link) {
				fputsym(fout, ast->ref.link, 0, 0);
			}
			else {
				fputstr(fout, ast->ref.name);
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

static void dumpxml(FILE* fout, astn ast, int mode, int level, const char* text) {
	if (ast == NULL) {
		return;
	}

	fputfmt(fout, "%I<%s op=\"%t\"", level, text, ast->kind);

	if ((mode & prType) != 0) {
		fputfmt(fout, " type=\"%?T\"", ast->type);
	}

	if ((mode & prCast) != 0) {
		fputfmt(fout, " cast=\"%?t\"", ast->cst2);
	}

	if ((mode & prLine) != 0) {
		if (ast->file != NULL && ast->line > 0) {
			fputfmt(fout, " file=\"%s:%d\"", ast->file, ast->line);
		}
	}

	switch (ast->kind) {
		default:
			fatal("FixMe %t", ast->kind);
			break;

		//#{ STMT
		case STMT_do:
			fputfmt(fout, " stmt=\"%?+k\">\n", ast);
			dumpxml(fout, ast->stmt.stmt, mode, level + 1, "expr");
			fputfmt(fout, "%I</%s>\n", level, text);
			break;

		case STMT_beg: {
			astn list;
			fputfmt(fout, ">\n");
			for (list = ast->stmt.stmt; list; list = list->next)
				dumpxml(fout, list, mode, level + 1, "stmt");
			fputfmt(fout, "%I</%s>\n", level, text);
			break;
		}

		case STMT_if:
			fputfmt(fout, " stmt=\"%?+k\">\n", ast);
			dumpxml(fout, ast->stmt.test, mode, level + 1, "test");
			dumpxml(fout, ast->stmt.stmt, mode, level + 1, "then");
			dumpxml(fout, ast->stmt.step, mode, level + 1, "else");
			fputfmt(fout, "%I</%s>\n", level, text);
			break;

		case STMT_for:
			fputfmt(fout, " stmt=\"%?+k\">\n", ast);
			dumpxml(fout, ast->stmt.init, mode, level + 1, "init");
			dumpxml(fout, ast->stmt.test, mode, level + 1, "test");
			dumpxml(fout, ast->stmt.step, mode, level + 1, "step");
			dumpxml(fout, ast->stmt.stmt, mode, level + 1, "stmt");
			fputfmt(fout, "%I</%s>\n", level, text);
			break;

		case STMT_con:
		case STMT_brk:
			fputfmt(fout, " />\n");
			break;

		case STMT_ret:
			fputfmt(fout, " stmt=\"%?+k\">\n", ast);
			dumpxml(fout, ast->stmt.stmt, mode, level + 1, "expr");
			fputfmt(fout, "%I</%s>\n", level, text);
			break;

		//#}
		//#{ OPER
		case OPER_fnc: {	// '()'
			astn args = ast->op.rhso;
			fputfmt(fout, ">\n");
			if (args != NULL) {
				while (args && args->kind == OPER_com) {
					dumpxml(fout, args->op.rhso, mode, level + 1, "push");
					args = args->op.lhso;
				}
				dumpxml(fout, args, mode, level + 1, "push");
			}
			dumpxml(fout, ast->op.lhso, mode, level + 1, "call");
			fputfmt(fout, "%I</%s>\n", level, text);
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
			fputfmt(fout, " oper=\"%?+k\">\n", ast);
			dumpxml(fout, ast->op.test, mode, level + 1, "test");
			dumpxml(fout, ast->op.lhso, mode, level + 1, "lval");
			dumpxml(fout, ast->op.rhso, mode, level + 1, "rval");
			fputfmt(fout, "%I</%s>\n", level, text);
			break;

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
			astn init = var ? var->init : NULL;
			symn fields = var ? var->flds : NULL;

			fputfmt(fout, " tyc=\"%?t\" name=\"%+T\"", var->cast, var);
			if (fields || init) {
				fputfmt(fout, ">\n");
			}
			else {
				fputfmt(fout, " />\n");
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
					fputfmt(fout, "%I<%s op=\"%t\"", level + 1, argn, def->kind, def);
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

			if (init) {
				dumpxml(fout, var->init, mode, level + 1, "init");
			}

			if (fields || init) {
				fputfmt(fout, "%I</%s>\n", level, text);
			}
			break;
		}
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
					msg -= 1;
					continue;

				default:
					fputchr(fout, chr);
					continue;

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
						case '-':
							mode = prType | prQual | 1;
							break;
						case '+':
							mode = prQual | 1;
							break;
					}
					fputsym(fout, sym, mode, 0);
					continue;
				}

				case 'k': {		// node
					int mode = noIden;
					astn ast = va_arg(ap, astn);

					if (ast == NULL && nil) {
						if (pad != 0) {
							fputchr(fout, pad);
						}
						continue;
					}

					switch (sgn) {
						case '-':
							mode = noIden | 2;
							break;

						case '+':
							mode = noIden | 1;
							prc = 0x0f;
							break;
					}

					fputast(fout, ast, mode, 0);
					continue;
				}

				case 'K': {		// node
					astn ast = va_arg(ap, astn);
					if (ast == NULL && nil) {
						if (pad != 0) {
							fputchr(fout, pad);
						}
						continue;
					}

					dumpxml(fout, ast, len << 4, 0, "node");
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
						str = (char*)tok_tbl[arg].name;
					}
					else {
						str = ".ERROR.";
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
						num = -(int32_t)num;
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

				case 'U':		// uns64
				case 'D': {		// dec64
					int neg = 0;
					uint64_t num = va_arg(ap, int64_t);

					if (num == 0 && nil) {
						len = pad != 0;
						str = "";
						break;
					}

					if (chr == 'D' && (int64_t)num < 0) {
						num = -(int64_t)num;
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
						memcpy(buff, fmt, len);
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

				fputstr(fout, str);
			}
		}
		else {
			fputchr(fout, chr);
		}
	}
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
static void dumpsym(FILE* fout, symn sym, int mode) {
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

			case TYPE_arr:
			case TYPE_ref:
				/*if (ptr->cast == TYPE_arr) {
					break;
				}
				if (ptr->cast == TYPE_ref) {
					break;
				}*/
				if (mode > 3 && ptr->call && ptr->prms) {
					*++sp = ptr->prms;
				}
				break;

			case TYPE_def:
				if (mode > 3) {
					*++sp = ptr->prms;
				}
				break;

			case EMIT_opc:
				if (mode > 2) {
					*++sp = ptr->prms;
				}
				break;

			default:
				if (mode > 1) {
					*++sp = ptr->prms;
				}
				break;
		}

		if (print_line && ptr->file && ptr->line) {
			fputfmt(fout, "%s:%d:", ptr->file, ptr->line);
		}

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

			if (ptr->cnst) {
				fputfmt(fout, ", const");
			}

			if (ptr->stat) {
				fputfmt(fout, ", static");
			}

			if (ptr->cast != 0) {
				fputfmt(fout, ", cast: %t", ptr->cast);
			}

			fputstr(fout, "]");
		}

		fputchr(fout, '\n');
		if (print_used) {
			astn used;
			for (used = ptr->used; used; used = used->ref.used) {
				fputfmt(fout, "%s:%d: referenced(%t) as `%+k`\n", used->file, used->line, used->cst2, used);
			}
		}

		fflush(fout);

		if (mode == 0) {
			break;
		}
	}
}

void logFILE(state rt, FILE* file) {

	if (rt->closelog != 0) {
		fclose(rt->logf);
		rt->closelog = 0;
	}

	rt->logf = file;
}

int logfile(state rt, char* file) {

	logFILE(rt, NULL);

	if (file != NULL) {
		rt->logf = fopen(file, "wb");
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
	FPUTFMT(fout, msg, ap);
	va_end(ap);
}

void dump(state rt, int mode, symn sym, const char* text, ...) {
	int level = mode & 0xff;
	FILE* logf = rt->logf;

	if (logf == NULL) {
		return;
	}

	if (text != NULL) {
		va_list ap;
		va_start(ap, text);
		FPUTFMT(logf, text, ap);
		va_end(ap);
	}

	if (mode & dump_sym && rt->defs != NULL) {
		if (sym != NULL) {
			dumpsym(logf, sym, level);
		}
		else {
			if ((level & 0x0f) == 0x0f) {
				// print all symbols.
				for (sym = rt->defs; sym; sym = sym->defs) {
					dumpsym(logf, sym, level & ~0x0f);
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
				if ((level & 0x0f) == 0x0f) {
					dumpxml(logf, sym->init, level, 0, "code");
				}
				else {
					fputast(logf, sym->init, level | 2, 0);
				}
			}
		}
		else {
			if ((level & 0x0f) == 0x0f) {
				dumpxml(logf, rt->cc->root, level, 0, "root");
			}
			else {
				fputast(logf, rt->cc->root, level | 2, 0);
			}
		}
	}

	if (mode & dump_asm) {
		if (sym != NULL) {
			if (sym->kind == TYPE_ref && sym->call) {
				fputasm(rt, logf, sym->offs, sym->offs + sym->size, 0x119);
			}
		}
		else {
			for (sym = rt->gdef; sym; sym = sym->gdef) {
				if (sym->kind == TYPE_ref && sym->call) {
					symn param;
					fputfmt(logf, "%-T [@%06x: %d] {\n", sym, sym->offs, sym->size);
					for (param = sym->flds; param; param = param->next) {
						fputfmt(logf, "\t.local %-T [@%06x, size:%d, cast:%t]\n", param, param->offs, param->size, param->cast);
					}
					fputasm(rt, logf, sym->offs, sym->offs + sym->size, 0x100 | (mode & 0xff));
					fputfmt(logf, "}\n");
				}
			}
			fputfmt(logf, "init(ro: %d"
				", ss: %d"
				", sm: %d"
				", pc: %d"
				", px: %d"
				//", size.meta: %d"
				//", size.code: %d"
			") {\n", rt->vm.ro, rt->vm.ss, rt->vm.sm, rt->vm.pc, rt->vm.px);

			fputasm(rt, logf, rt->vm.pc, rt->vm.px, 0x100 | (mode & 0xff));
			fputfmt(logf, "}\n");
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
	FILE* logFile = rt ? rt->logf : stdout;

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
		fputstr(logFile, "warning: ");
	}
	else if (level) {
		fputstr(logFile, "error: ");
	}

	va_start(argp, msg);
	FPUTFMT(logFile, msg, argp);
	fputchr(logFile, '\n');
	fflush(logFile);
	va_end(argp);
}
