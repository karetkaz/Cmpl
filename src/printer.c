#include "printer.h"
#include "compiler.h"
#include "tree.h"
#include "type.h"
#include "code.h"

#include <stdarg.h>

/// format an integer number
static char *formatInt(char *dst, int max, int prc, char sgn, int radix, uint64_t value) {
	char *ptr = dst + max;
	int p = 0, plc = ',';
	*--ptr = 0;
	do {
		if (prc > 0 && ++p > prc) {
			*--ptr = (char) plc;
			p = 1;
		}
		*--ptr = "0123456789abcdef"[value % radix];
	} while (value /= radix);
	if (sgn != 0) {
		*--ptr = sgn;
	}
	return ptr;
}

/// format a floating number
static char *formatFlt(char *dst, int max, int prc, char sgn, char mode, float64_t value) {
	char format[32] = {0};
	int i = 0;
	format[i++] = '%';
	if (sgn != 0) {
		format[i++] = sgn;
	}

	if (prc == 0) {
		format[i++] = mode;
		snprintf(dst, max, format, value);
		return dst;
	}

	format[i++] = '.';
	format[i++] = '*';
	format[i++] = mode;
	snprintf(dst, max, format, prc, value);
	return dst;
}

static void print_fmt(FILE *out, const char **esc, const char *fmt, va_list ap) {
	char buff[1024], chr;

	while ((chr = *fmt++)) {
		if (chr != '%') {
			putc(chr, out);
			continue;
		}

		char opt = 0;    // [?]? optional flag: skip null / zero value (may print pad char once)
		if (*fmt == '?') {
			opt = *fmt++;
		}

		char sgn = 0;    // [+-]? sign flag / alignment.
		if (*fmt == '+' || *fmt == '-') {
			sgn = *fmt++;
		}

		char pad = 0;    // [0 ]? padding character.
		if (*fmt == '0' || *fmt == ' ') {
			pad = *fmt++;
		}

		int len = 0;     // [0-9]* length / mode
		while (*fmt >= '0' && *fmt <= '9') {
			len = (len * 10) + (*fmt - '0');
			fmt++;
		}

		int prc = 0;     // (\.\*|[0-9]*)? precision / indent
		int noPrc = 1;   // format string has no precision
		if (*fmt == '.') {
			fmt++;
			noPrc = 0;
			if (*fmt == '*') {
				prc = va_arg(ap, int32_t);
				fmt++;
			}
			else {
				prc = 0;
				while (*fmt >= '0' && *fmt <= '9') {
					prc = (prc * 10) + (*fmt - '0');
					fmt++;
				}
			}
		}

		const char *str = NULL;
		switch (chr = *fmt++) {
			case 0:
				fmt -= 1;
				continue;

			default:
				fputc(chr, out);
				continue;

			case 'T': {		// type name
				symn sym = va_arg(ap, symn);

				if (sym == NULL && opt) {
					len = pad != 0;
					str = "";
					break;
				}

				switch (sgn) {
					default:
						if (noPrc) {
							prc = prShort;
						}
						break;

					case '-':
						len = -len;
						// fall through
					case '+':
						if (noPrc) {
							prc = prDetail;
						}
						break;
				}
				printSym(out, esc, sym, (dmpMode) prc, len);
				continue;
			}

			case 't': {		// tree node
				astn ast = va_arg(ap, astn);

				if (ast == NULL && opt) {
					len = pad != 0;
					str = "";
					break;
				}

				switch (sgn) {
					default:
						if (noPrc) {
							prc = prShort;
						}
						break;

					case '-':
						len = -len;
						// fall through
					case '+':
						if (noPrc) {
							prc = prDetail;
						}
						break;
				}
				printAst(out, esc, ast, (dmpMode) prc, len);
				continue;
			}

			case 'K': {		// symbol kind
				ccKind arg = va_arg(ap, ccKind);
				char *_cast = "ERR";
				char *_kind = "";
				char *_stat = "";
				char *_mut = "";
				char *_opt = "";

				if (arg == 0 && opt) {
					len = pad != 0;
					str = "";
					break;
				}

				switch (arg & MASK_cast) {
					default:
						break;

					case CAST_any:
						_cast = NULL;
						break;

					case CAST_vid:
						_cast = "void";
						break;

					case CAST_bit:
						_cast = "bool";
						break;

					case CAST_i32:
						_cast = "i32";
						break;

					case CAST_u32:
						_cast = "u32";
						break;

					case CAST_i64:
						_cast = "i64";
						break;

					case CAST_u64:
						_cast = "u64";
						break;

					case CAST_f32:
						_cast = "f32";
						break;

					case CAST_f64:
						_cast = "f64";
						break;

					case CAST_val:
						_cast = "val";
						break;

					case CAST_ptr:
					case CAST_obj:
						_cast = "ref";
						break;

					case CAST_arr:
						_cast = "arr";
						break;

					case CAST_var:
						_cast = "var";
						break;
				}

				switch (arg & MASK_kind) {
					default:
						break;

					case KIND_def:
						if (_cast != NULL) {
							_kind = _cast;
							_cast = NULL;
						}
						else if (!opt) {
							_kind = "inline";
						}
						break;

					case KIND_typ:
						_kind = "typename";
						break;

					case KIND_fun:
						_kind = "function";
						break;

					case KIND_var:
						_kind = "variable";
						break;
				}

				if (arg & ATTR_stat) {
					_stat = "static ";
				}
				if (arg & ATTR_mut) {
					_mut = "mutable ";
				}
				if (arg & ATTR_opt) {
					_opt = "optional ";
				}
				if (_cast != NULL) {
					snprintf(buff, sizeof(buff), "%s%s%s%s(%s)", _stat, _mut, _opt, _kind, _cast);
				} else {
					snprintf(buff, sizeof(buff), "%s%s%s%s", _stat, _mut, _opt, _kind);
				}
				str = buff;
				break;
			}

			case 'k': {		// token kind
				ccToken arg = va_arg(ap, ccToken);

				if (arg == 0 && opt) {
					len = pad != 0;
					str = "";
					break;
				}

				if (arg < tok_last) {
					str = token_tbl[arg].name;
				} else {
					snprintf(buff, sizeof(buff), "TOKEN_%02x", arg);
					str = buff;
				}
				break;
			}

			case 'A': {		// instruction
				void *opc = va_arg(ap, void*);

				if (opc == NULL && opt) {
					len = pad != 0;
					str = "";
					break;
				}

				switch (sgn) {
					default:
						if (noPrc) {
							prc = prAbsOffs;
						}
						break;

					case '-':
					case '+':
						if (noPrc) {
							// print using relative offset
							prc = prRelOffs;
						}
						break;
				}

				// length is the max number of bytes to be printed
				prc |= (prc & prAsmCode) | (len & prAsmCode);

				printAsm(out, esc, NULL, opc, (dmpMode) prc);
				continue;
			}

			case 'a': {		// offset
				size_t offs = va_arg(ap, size_t);

				if (offs == 0 && opt) {
					len = pad != 0;
					str = "";
					break;
				}

				switch (sgn) {
					default:
						if (noPrc) {
							prc = prAbsOffs;
						}
						break;

					case '-':
					case '+':
						if (noPrc) {
							// print using relative offset
							prc = prRelOffs;
						}
						break;
				}
				printOfs(out, esc, NULL, NULL, offs, (dmpMode) prc);
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
				uint32_t num = va_arg(ap, uint32_t);
				if (num == 0 && opt) {
					len = pad != 0;
					str = "";
					break;
				}
				str = formatInt(buff, sizeof(buff), prc, sgn, 2, num);
				break;
			}

			case 'B': {		// bin64
				uint64_t num = va_arg(ap, uint64_t);
				if (num == 0 && opt) {
					len = pad != 0;
					str = "";
					break;
				}
				str = formatInt(buff, sizeof(buff), prc, sgn, 2, num);
				break;
			}

			case 'o': {		// oct32
				uint32_t num = va_arg(ap, uint32_t);
				if (num == 0 && opt) {
					len = pad != 0;
					str = "";
					break;
				}
				str = formatInt(buff, sizeof(buff), prc, sgn, 8, num);
				break;
			}

			case 'O': {		// oct64
				uint64_t num = va_arg(ap, uint64_t);
				if (num == 0 && opt) {
					len = pad != 0;
					str = "";
					break;
				}
				str = formatInt(buff, sizeof(buff), prc, sgn, 8, num);
				break;
			}

			case 'x': {		// hex32
				uint32_t num = va_arg(ap, uint32_t);
				if (num == 0 && opt) {
					len = pad != 0;
					str = "";
					break;
				}
				str = formatInt(buff, sizeof(buff), prc, sgn, 16, num);
				break;
			}

			case 'X': {		// hex64
				uint64_t num = va_arg(ap, uint64_t);
				if (num == 0 && opt) {
					len = pad != 0;
					str = "";
					break;
				}
				str = formatInt(buff, sizeof(buff), prc, sgn, 16, num);
				break;
			}

			case 'u':		// uint32
			case 'd': {		// dec32
				uint32_t num = va_arg(ap, uint32_t);

				if (num == 0 && opt) {
					len = pad != 0;
					str = "";
					break;
				}

				if (chr == 'd' && (int32_t)num < 0) {
					num = -num;
					sgn = '-';
				}

				str = formatInt(buff, sizeof(buff), prc, sgn, 10, num);
				break;
			}

			case 'U':		// uint64
			case 'D': {		// dec64
				uint64_t num = va_arg(ap, uint64_t);

				if (num == 0 && opt) {
					len = pad != 0;
					str = "";
					break;
				}

				if (chr == 'D' && (int64_t)num < 0) {
					num = -num;
					sgn = '-';
				}
				str = formatInt(buff, sizeof(buff), prc, sgn, 10, num);
				break;
			}

			case 'E':		// float64
			case 'F':		// float64
				chr -= 'A' - 'a';
				// fall through
			case 'e':		// float32
			case 'f': {		// float32
				float64_t num = va_arg(ap, float64_t);
				if (num == 0 && opt) {
					len = pad != 0;
					str = "";
					break;
				}

				str = formatFlt(buff, sizeof(buff), prc, sgn, chr, num);
				break;
			}

			case 's':		// string
				str = va_arg(ap, char*);
				if (str == NULL && opt) {
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

		if (str == NULL) {
			str = "(null)";
		}

		if (pad == 0) {
			pad = ' ';
		}

		len -= strlen(str);
		while (len > 0) {
			fputc(pad, out);
			len -= 1;
		}

		if (esc == NULL) {
			fputs(str, out);
			continue;
		}

		while ((chr = *str) != 0) {
			if (esc[chr & 0xff] != NULL) {
				fputs(esc[chr & 0xff], out);
			} else {
				fputc(chr & 0xff, out);
			}
			str += 1;
		}
	}
	fflush(out);
}

void printFmt(FILE *out, const char **esc, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	print_fmt(out, esc, fmt, ap);
	va_end(ap);
}

void print_log(rtContext ctx, raiseLevel level, const char *file, int line, struct nfcArgArr details, const char *msg, va_list vaList) {
	FILE *out = ctx == NULL ? stdout : ctx->logFile;
	if (out == NULL) {
		return;
	}

	if (ctx != NULL) {
		if (ctx->cc && ctx->cc->inStaticIfFalse && level < raiseWarn) {
			// convert errors to warnings inside static if (false) { ... }
			level = raiseVerbose;
		}
		if (level > (int) ctx->logLevel) {
			// no need to show the log on the current level
			return;
		}
		if (level <= raiseError) {
			ctx->errors += 1;
		}
	}

	const char *logType = "UNKNOWN";
	switch (level) {
		default:
			if (level > raiseWarn && level < raiseInfo) {
				logType = "warn";
				break;
			}
			break;

		case raiseFatal:
			logType = "fatal";
			break;

		case raiseError:
			logType = "error";
			break;

		case raisePrint:
			logType = NULL;
			break;

		case raiseWarn:
			logType = "warn";
			break;

		case raiseInfo:
			logType = "info";
			break;

		case raiseDebug:
			logType = "debug";
			break;

		case raiseVerbose:
			logType = "verbose";
			break;
	}

	int wasOutput = 0;
	const char **esc = NULL;
	if (file != NULL && line > 0) {
		printFmt(out, esc, "%?s:%?u", file, line);
		wasOutput = 1;
	}

	if (logType != NULL) {
		if (wasOutput) {
			printFmt(out, esc, ": ");
		}
		printFmt(out, esc, "%s", logType);
		wasOutput = 1;
	}

	if (msg != NULL) {
		if (wasOutput) {
			printFmt(out, esc, ": ");
		}
		print_fmt(out, esc, msg, vaList);
		wasOutput = 1;
	}

	if (ctx != NULL && details.ref != NULL && details.length > 0) {
		if (wasOutput) {
			printFmt(out, esc, ": ");
		}
		for (size_t i = 0; i < details.length; ++i) {
			if (i > 0) {
				printFmt(out, esc, ", ");
			}
			vmValue *var = (vmValue *) ((char *) details.ref + 2 * vm_ref_size * i);
			symn type = vmPointer(ctx, var->type);
			vmValue *ref = vmPointer(ctx, var->ref);
			if (isDynamicSizeArray(type)) {
				// arrays compressed into variant loose the length property, they became `type[*]`
				struct symNode typeArr = *type;	// make a copy of the array type
				// remove the length property
				typeArr.fields = NULL;
				printVal(ctx->logFile, NULL, ctx, &typeArr, var, prGlobal, 0);
			}
			else if (isUnknownSizeArray(type)) {
				printVal(ctx->logFile, NULL, ctx, type, var, prGlobal, 0);
			}
			else {
				printVal(ctx->logFile, NULL, ctx, type, ref, prGlobal, 0);
			}
			wasOutput = 1;
		}
	}

	if (wasOutput) {
		printFmt(out, esc, "\n");
		fflush(out);
	}

	if (level == raiseFatal) {
#ifdef DEBUGGING
		abort();
#else
		exit(1);
#endif
	}
}

void printLog(rtContext ctx, raiseLevel level, const char *file, int line, const char *msg, ...) {
	struct nfcArgArr details = {0};
	va_list vaList;
	va_start(vaList, msg);
	print_log(ctx, level, file, line, details, msg, vaList);
	va_end(vaList);
}

static void dumpApiSym(rtContext ctx, userContext extra, symn sym, void dumpSym(userContext, symn)) {
	if (sym == ctx->main) {
		return;
	}
	dumpSym(extra, sym);
	for (symn fld = sym->fields; fld != NULL; fld = fld->next) {
		if (fld->owner != sym) {
			continue;
		}
		dumpApiSym(ctx, extra, fld, dumpSym);
	}
}
void dumpApi(rtContext ctx, userContext extra, void dumpSym(userContext, symn)) {
	if (ctx == NULL || dumpSym == NULL) {
		dieif(ctx == NULL, ERR_INTERNAL_ERROR);
		dieif(dumpSym == NULL, ERR_INTERNAL_ERROR);
		return;
	}

	// compilation errors or compiler was not initialized.
	if (ctx->cc == NULL || ctx->main == NULL) {
		dieif(ctx->cc == NULL, ERR_INTERNAL_ERROR);
		dieif(ctx->main == NULL, ERR_INTERNAL_ERROR);
		return;
	}
	if (ctx->cc->scope != ctx->main->fields) {
		dieif(ctx->cc->scope != ctx->main->fields, ERR_INTERNAL_ERROR);
		return;
	}
	for (symn fld = ctx->main->fields; fld != NULL; fld = fld->next) {
		dumpApiSym(ctx, extra, fld, dumpSym);
	}
	dumpSym(extra, ctx->main);
}

void logFILE(rtContext ctx, FILE *file) {
	// close previous opened file
	if (ctx->closeLog != 0) {
		fclose(ctx->logFile);
		ctx->closeLog = 0;
	}
	ctx->logFile = file;
}

const char **escapeStr() {
	static const char *escape[256] = {0};
	if (escape[0] == NULL) {
		escape[0] = "\\0";
		escape['\n'] = "\\n";
		escape['\r'] = "\\r";
		escape['\t'] = "\\t";
		escape['\''] = "\\'";
		escape['\"'] = "\\\"";
	}
	return escape;
}

/// print-sym.c ////////////////////////////////////////////////////////////////////////////////////////////////////////

/// print qualified name
static void printQualified(FILE *out, const char **esc, symn sym) {
	if (sym == NULL) {
		return;
	}
	if (sym->owner != NULL && sym->owner != sym) {
		printQualified(out, esc, sym->owner);
		printFmt(out, esc, "%c", '.');
	}
	if (sym->name != NULL) {
		printFmt(out, esc, "%s", sym->name);
	} else {
		printFmt(out, esc, "%s", "?");
	}
}

/// print array type
static void printArray(FILE *out, const char **esc, symn sym, dmpMode mode) {
	if (sym != NULL && isArrayType(sym)) {
		symn length = sym->fields;
		printArray(out, esc, sym->type, mode);
		if (length != NULL && isStatic(length)) {
			printFmt(out, esc, "[%d]", sym->size / sym->type->size);
		}
		else if (length != NULL) {
			printFmt(out, esc, "[]");
		}
		else {
			printFmt(out, esc, "[*]");
		}
		return;
	}
	printSym(out, esc, sym, mode, 0);
}

void printSym(FILE *out, const char **esc, symn sym, dmpMode mode, int indent) {
	int oneLine = mode & prOneLine;
	int pr_attr = mode & prSymAttr;
	int pr_qual = mode & prSymQual;
	int pr_args = mode & prSymArgs;
	int pr_type = mode & prSymType;
	int pr_init = mode & prSymInit;
	int pr_body = !oneLine && pr_init;
	int pr_result = 0;

	if (sym == NULL) {
		printFmt(out, esc, "%s", NULL);
		return;
	}

	if (indent > 0) {
		printFmt(out, esc, "%I", indent);
	}
	else {
		indent = -indent;
	}

	if (pr_attr) {
		if (isStatic(sym)) {
			printFmt(out, esc, "%s", "static ");
		}
		if (isMutable(sym) != 0) {
			printFmt(out, esc, "%s", "mutable ");
		}
		if (isOptional(sym) != 0) {
			printFmt(out, esc, "%s", "optional ");
		}
	}

	if (sym->name && *sym->name) {
		if (pr_qual) {
			printQualified(out, esc, sym);
		}
		else {
			printFmt(out, esc, "%s", sym->name);
		}
	}

	switch (sym->kind & MASK_kind) {
		case KIND_typ:
			if (castOf(sym) == CAST_arr) {
				if (sym->name == NULL) {
					printArray(out, esc, sym, mode);
					return;
				}
			}
			else if (pr_body) {
				printFmt(out, esc, ": struct %s", "{\n");

				for (symn arg = sym->fields; arg; arg = arg->next) {
					printSym(out, esc, arg, mode & ~prSymQual, indent + 1);
					if (arg->kind != KIND_typ) {		// nested struct
						printFmt(out, esc, "%s", ";\n");
					}
				}
				printFmt(out, esc, "%I%s", indent, "}");
			}
			break;

		case KIND_def:
		case KIND_fun:
		case KIND_var: {
			symn type = sym->type;
			if (pr_args && sym->params != NULL) {
				symn args = sym->params;
				if (!pr_result) {
					// skip result parameter
					type = args->type;
					args = args->next;
					pr_type = 1;
				}
				printFmt(out, esc, "%c", '(');
				for (symn arg = args; arg; arg = arg->next) {
					if (arg != args) {
						printFmt(out, esc, "%s", ", ");
					}
					printSym(out, esc, arg, (mode | prSymType) & ~prSymQual, 0);
				}
				printFmt(out, esc, "%c", ')');
			}
			if (pr_type && type) {
				printFmt(out, esc, "%s", ": ");
				printSym(out, esc, type, prName, -indent);
			}
			if (pr_init && sym->init) {
				printFmt(out, esc, "%s", " := ");
				printAst(out, esc, sym->init, mode, -indent);
			}
			break;
		}

		default:
			fatal(ERR_INTERNAL_ERROR);
			return;
	}
}

/// print-ast.c ////////////////////////////////////////////////////////////////////////////////////////////////////////

void printAst(FILE *out, const char **esc, astn ast, dmpMode mode, int indent) {
	static const int exprLevel = 0;
	const int oneLine = mode & prOneLine;
	const int nlBody = mode & nlAstBody;
	const int nlElIf = mode & nlAstElIf;
	int nlElse = !(mode & prMinified);

	if (ast == NULL) {
		printFmt(out, esc, "%s", NULL);
		return;
	}

	if (indent > 0) {
		if (!oneLine) {
			printFmt(out, esc, "%I", indent);
		}
	}
	else {
		indent = -indent;
	}

	ccToken kind = ast->kind;
	switch (kind) {
		default:
			printFmt(out, esc, "%s", token_tbl[kind].name);
			break;

		//#{ STATEMENTS
		case STMT_end:
			if (mode == prName) {
				printFmt(out, esc, "%s", token_tbl[kind].name);
				break;
			}

			printAst(out, esc, ast->stmt.stmt, mode, exprLevel);
			printFmt(out, esc, "%s", ";");
			if (oneLine) {
				break;
			}
			break;

		case STMT_beg:
			if (mode == prName) {
				printFmt(out, esc, "%s", token_tbl[kind].name);
				break;
			}

			if (oneLine) {
				printFmt(out, esc, "%s", "{...}");
				break;
			}

			printFmt(out, esc, "%s", "{");
			for (astn list = ast->stmt.stmt; list; list = list->next) {
				int indent2 = indent + 1;
				int exprStatement = 0;
				printFmt(out, esc, "%s", "\n");
				if (list->kind < STMT_beg || list->kind > STMT_end) {
					switch (list->kind) {
						default:
							// not valid expression statement
							exprStatement = -1;
							indent2 = exprLevel;
							printFmt(out, esc, "%I", indent + 1);
							break;

						case OPER_fnc:
						case INIT_set:
						case ASGN_set:
							// valid expression statement
							exprStatement = 1;
							indent2 = exprLevel;
							printFmt(out, esc, "%I", indent + 1);
							break;

						case TOKEN_var:
							if (list->id.link && list->id.link->tag == list) {
								// valid declaration statement
								exprStatement = 1;
							}
							break;
					}
				}
				printAst(out, esc, list, mode, indent2);
				if (exprStatement != 0) {
					if (exprStatement < 0) {
						printFmt(out, esc, "; /* `%k` is not a valid statement */", list->kind);
					}
					else {
						printFmt(out, esc, "%s", ";");
					}
				}
			}
			printFmt(out, esc, "\n%I%s", indent, "}");
			break;

		case STMT_if:
		case STMT_sif:
			printFmt(out, esc, "%s", token_tbl[kind].name);
			if (mode == prName) {
				break;
			}

			printFmt(out, esc, "%s", " (");
			printAst(out, esc, ast->stmt.test, mode, exprLevel);
			printFmt(out, esc, "%s", ")");

			if (oneLine) {
				break;
			}

			// if then part
			if (ast->stmt.stmt != NULL) {
				astn stmt = ast->stmt.stmt;
				kind = stmt->kind;
				if (nlBody == 0 && kind == STMT_beg) {
					// keep on the same line: `if (...) {`
					printFmt(out, esc, "%s", " ");
					printAst(out, esc, stmt, mode, -indent);
				}
				else {
					printFmt(out, esc, "%s", "\n");
					printAst(out, esc, stmt, mode, indent + (kind != STMT_beg));
					// force else on a new line
					nlElse = 1;
				}
			}
			else {
				printFmt(out, esc, "%s", " ;");
				nlElse = 1;
			}

			// if else part
			if (ast->stmt.step != NULL) {
				astn stmt = ast->stmt.step;
				kind = stmt->kind;

				// else block must contain a single if statement to be an `else if`
				if (nlElIf == 0 && kind == STMT_beg && stmt->stmt.stmt != NULL) {
					ccToken kind2 = stmt->stmt.stmt->kind;
					if (kind2 == STMT_if || kind2 == STMT_sif) {
						if (stmt->stmt.stmt->next == NULL) {
							stmt = stmt->stmt.stmt;
							kind = stmt->kind;
						}
					}
				}

				if (nlElse == 0 && nlBody == 0 && kind == STMT_beg) {
					// keep on the same line: `} else {`
					printFmt(out, esc, " else ");
					printAst(out, esc, stmt, mode, -indent);
				}
				else if (nlBody == 0 && kind == STMT_beg) {
					// keep on the same line: `else {`
					printFmt(out, esc, "\n%Ielse ", indent);
					printAst(out, esc, stmt, mode, -indent);
				}
				else if (kind == STMT_if || kind == STMT_sif) {
					// keep `else if` always on a new line
					printFmt(out, esc, "\n%Ielse ", indent);
					printAst(out, esc, stmt, mode, -indent);
				}
				else {
					printFmt(out, esc, "\n%Ielse\n", indent);
					printAst(out, esc, stmt, mode, indent + (kind != STMT_beg));
				}
			}
			break;

		case STMT_for:
		case STMT_sfor:
			printFmt(out, esc, "%s", token_tbl[kind].name);
			if (mode == prName) {
				break;
			}

			printFmt(out, esc, "%s", " (");
			if (ast->stmt.init) {
				printAst(out, esc, ast->stmt.init, mode | oneLine, exprLevel);
			}
			else {
				printFmt(out, esc, "%s", " ");
			}
			printFmt(out, esc, "%s", "; ");
			if (ast->stmt.test) {
				printAst(out, esc, ast->stmt.test, mode, exprLevel);
			}
			printFmt(out, esc, "%s", "; ");
			if (ast->stmt.step) {
				printAst(out, esc, ast->stmt.step, mode, exprLevel);
			}
			printFmt(out, esc, "%s", ")");

			if (oneLine) {
				break;
			}

			if (ast->stmt.stmt != NULL) {
				astn stmt = ast->stmt.stmt;
				kind = stmt->kind;
				if (nlBody == 0 && kind == STMT_beg) {
					printFmt(out, esc, "%s", " ");
					printAst(out, esc, stmt, mode, -indent);
				}
				else {
					printFmt(out, esc, "%s", "\n");
					printAst(out, esc, stmt, mode, indent + (kind != STMT_beg));
				}
			}
			else {
				printFmt(out, esc, "%s", " ;");
			}
			break;

		case STMT_ret:
		case STMT_con:
		case STMT_brk:
			printFmt(out, esc, "%s", token_tbl[kind].name);
			if (mode == prName) {
				break;
			}
			if (ast->kind == STMT_ret && ast->ret.value != NULL) {
				printFmt(out, esc, "%s", " ");
				printAst(out, esc, ast->ret.value, mode, exprLevel);
			}
			printFmt(out, esc, "%s", ";");
			break;

		//#}
		//#{ OPERATORS
		case OPER_fnc:		// '()'
			if (mode == prName) {
				printFmt(out, esc, "%s", token_tbl[kind].name);
				break;
			}

			if (ast->op.lhso != NULL) {
				printAst(out, esc, ast->op.lhso, mode, exprLevel);
			}
			printFmt(out, esc, "%c", '(');
			if (ast->op.rhso != NULL) {
				printAst(out, esc, ast->op.rhso, mode, exprLevel);
			}
			printFmt(out, esc, "%c", ')');
			break;

		case OPER_idx:		// '[]'
			if (mode == prName) {
				printFmt(out, esc, "%s", token_tbl[kind].name);
				break;
			}

			if (ast->op.lhso != NULL) {
				printAst(out, esc, ast->op.lhso, mode, exprLevel);
			}
			printFmt(out, esc, "%c", '[');
			if (ast->op.rhso != NULL) {
				printAst(out, esc, ast->op.rhso, mode, exprLevel);
			}
			printFmt(out, esc, "%c", ']');
			break;

		case OPER_dot:		// '.'
			if (mode == prName) {
				printFmt(out, esc, "%s", token_tbl[kind].name);
				break;
			}
			printAst(out, esc, ast->op.lhso, mode, exprLevel);
			printFmt(out, esc, "%c", '.');
			printAst(out, esc, ast->op.rhso, mode, exprLevel);
			break;

		case OPER_adr:		// '&'
		case PNCT_dot3:		// '...'
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

		case OPER_ceq:		// '=='
		case OPER_cne:		// '!='
		case OPER_clt:		// '<'
		case OPER_cle:		// '<='
		case OPER_cgt:		// '>'
		case OPER_cge:		// '>='

		case OPER_all:		// '&&'
		case OPER_any:		// '||'

		case OPER_sel:		// '?'
		case OPER_cln:		// ':'

		case OPER_com:		// ','

		case INIT_set:		// '='
		case ASGN_set:		// '='
			if (mode == prName) {
				printFmt(out, esc, "%s", token_tbl[kind].name);
				break;
			}
			int precedence = token_tbl[kind].type & 0x0f;
			int putParen = indent > precedence;

			if ((mode & prAstType) && ast->type) {
				printSym(out, esc, ast->type, prSymQual, 0);
				putParen = 1;
			}

			if (putParen) {
				printFmt(out, esc, "%c", '(');
			}

			if (ast->op.lhso != NULL) {
				printAst(out, esc, ast->op.lhso, mode, -precedence);
				if (kind != OPER_com) {
					printFmt(out, esc, "%c", ' ');
				}
			}

			printFmt(out, esc, "%s", token_tbl[kind].name);

			// in case of unary operators: '-a' not '- a'
			if (ast->op.lhso != NULL) {
				printFmt(out, esc, "%c", ' ');
			}

			printAst(out, esc, ast->op.rhso, mode, -precedence);

			if (putParen) {
				printFmt(out, esc, "%c", ')');
			}
			break;

		//#{ VALUES
		case TOKEN_opc:
			printOpc(out, esc, ast->opc.code, ast->opc.args);
			break;

		case TOKEN_val:
			if (ast->type->fmt == NULL) {
				printFmt(out, esc, "{%.T @%D}", ast->type, ast->cInt);
			}
			else if (ast->type->fmt == type_fmt_string) {
				printFmt(out, esc, "%c", type_fmt_string_chr);
				printFmt(out, esc ? esc : escapeStr(), type_fmt_string, ast->id.name);
				printFmt(out, esc, "%c", type_fmt_string_chr);
			}
			else if (ast->type->fmt == type_fmt_character) {
				printFmt(out, esc, "%c", type_fmt_character_chr);
				printFmt(out, esc ? esc : escapeStr(), type_fmt_character, ast->cInt);
				printFmt(out, esc, "%c", type_fmt_character_chr);
			}
			else if (ast->type->fmt == type_fmt_signed32) {
				printFmt(out, esc, type_fmt_signed32, (int32_t) ast->cInt);
			}
			else if (ast->type->fmt == type_fmt_signed64) {
				printFmt(out, esc, type_fmt_signed64, (int64_t) ast->cInt);
			}
			else if (ast->type->fmt == type_fmt_unsigned32) {
				printFmt(out, esc, type_fmt_unsigned32, (uint32_t) ast->cInt);
			}
			else if (ast->type->fmt == type_fmt_unsigned64) {
				printFmt(out, esc, type_fmt_unsigned64, (uint64_t) ast->cInt);
			}
			else if (ast->type->fmt == type_fmt_float32) {
				printFmt(out, esc, type_fmt_float32, (float32_t) ast->cFlt);
			}
			else if (ast->type->fmt == type_fmt_float64) {
				printFmt(out, esc, type_fmt_float64, (float64_t) ast->cFlt);
			}
			else if (ast->type->fmt == type_fmt_typename) {
				printFmt(out, esc, type_fmt_typename, ast->type);
			}
			else {
				printFmt(out, esc, ast->type->fmt, ast->cInt);
			}
			break;

		case TOKEN_var:
			if (mode != prName && ast->id.link != NULL) {
				if (ast->id.link->tag == ast) {
					printSym(out, esc, ast->id.link, mode & ~prSymQual, -indent);
				}
				else {
					printSym(out, esc, ast->id.link, prName, 0);
				}
			}
			else {
				printFmt(out, esc, "%s", ast->id.name);
			}
			break;

		case TOKEN_doc:
			printFmt(out, esc, "/*%?s*/", ast->id.name);
			break;
		//#}
	}
}

/// print-ams.c ////////////////////////////////////////////////////////////////////////////////////////////////////////

void printOpc(FILE *out, const char **esc, int opc, int64_t args) {
	struct vmInstruction instruction;
	instruction.opc = opc;
	instruction.arg.i64 = args;
	printAsm(out, esc, NULL, &instruction, prName);
}
void printAsm(FILE *out, const char **esc, rtContext ctx, void *ptr, dmpMode mode) {
	size_t offs = (size_t)ptr;
	symn sym = NULL;

	if (ctx != NULL) {
		offs = vmOffset(ctx, ptr);
		sym = rtLookup(ctx, offs, 0);
	}

	dmpMode ofsMode = mode & (prSymQual|prRelOffs|prAbsOffs);
	if (mode & prAsmOffs) {
		printOfs(out, esc, ctx, sym, offs, ofsMode);

		size_t symOffs = sym == NULL ? 0 : offs - sym->offs;
		int trailLen = 1, padLen = 0;
		if (sym != NULL) {
			if (!(mode & (prRelOffs|prAbsOffs))) {
				padLen = 2 * !symOffs;
				symOffs = 0;
			}
			else if (mode & prRelOffs) {
				padLen = 5 + !symOffs;
			}
			else {
				padLen = 0;
				symOffs = 0;
			}
		}
		while (symOffs > 0) {
			symOffs /= 10;
			padLen -= 1;
		}
		if (padLen < 0) {
			padLen = 0;
		}
		printFmt(out, esc, "% I:% I", padLen, trailLen);
	}

	vmInstruction ip = ptr;
	size_t len = (size_t) mode & prAsmCode;
	if (ip == NULL) {
		printFmt(out, esc, "%s", "null");
		return;
	}

	//~ write code bytes
	if (len > 1 && len < opcode_tbl[ip->opc].size) {
		size_t i = 0;
		for (; i < len - 2; i++) {
			if (i < opcode_tbl[ip->opc].size) {
				printFmt(out, esc, "%02x ", ((unsigned char *) ptr)[i]);
			}
			else {
				printFmt(out, esc, "   ");
			}
		}
		if (i < opcode_tbl[ip->opc].size) {
			printFmt(out, esc, "%02x... ", ((unsigned char *) ptr)[i]);
		}
	}
	else {
		for (size_t i = 0; i < len; i++) {
			if (i < opcode_tbl[ip->opc].size) {
				printFmt(out, esc, "%02x ", ((unsigned char *) ptr)[i]);
			}
			else {
				printFmt(out, esc, "   ");
			}
		}
	}

	if (opcode_tbl[ip->opc].name != NULL) {
		printFmt(out, esc, "%s", opcode_tbl[ip->opc].name);
	}
	else {
		printFmt(out, esc, "opc.x%02x", ip->opc);
	}

	switch (ip->opc) {
		default:
			break;

		case opc_dup1:
		case opc_dup2:
		case opc_dup4:
		case opc_set1:
		case opc_set2:
		case opc_set4:
			printFmt(out, esc, " sp(%d)", ip->idx);
			break;

		case opc_mov1:
		case opc_mov2:
		case opc_mov4:
			printFmt(out, esc, " sp(%d, %d)", ip->mov.dst, ip->mov.src);
			break;

		case opc_lc32:
			printFmt(out, esc, " %d", ip->arg.i32);
			break;

		case opc_lc64:
			printFmt(out, esc, " %D", ip->arg.i64);
			break;

		case opc_lf32:
			printFmt(out, esc, " %f", ip->arg.f32);
			break;

		case opc_lf64:
			printFmt(out, esc, " %F", ip->arg.f64);
			break;

		case opc_inc:
		case opc_spc:
		case opc_ldsp:
			printFmt(out, esc, "(%+d)", ip->rel);
			break;

		case opc_move:
		case opc_mad:
			printFmt(out, esc, " %d", ip->rel);
			break;

		case opc_jmp:
		case opc_jnz:
		case opc_jz:
			printFmt(out, esc, " ");
			if (mode & (prRelOffs | prAbsOffs)) {
				printOfs(out, esc, ctx, sym, offs + ip->rel, ofsMode);
			}
			else {
				printFmt(out, esc, "%+d", ip->rel);
			}
			break;

		case u32_bit: switch (ip->idx & 0xc0) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					return;

				case u32_bit_and:
					printFmt(out, esc, "%s 0x%02x", "and", (1 << (ip->idx & 0x3f)) - 1);
					break;

				case u32_bit_shl:
					printFmt(out, esc, "%s 0x%02x", "shl", ip->idx & 0x3f);
					break;

				case u32_bit_shr:
					printFmt(out, esc, "%s 0x%02x", "shr", ip->idx & 0x3f);
					break;

				case u32_bit_sar:
					printFmt(out, esc, "%s 0x%02x", "sar", ip->idx & 0x3f);
					break;
			}
			break;

		case opc_lref:
		case opc_ld32:
		case opc_st32:
		case opc_ld64:
		case opc_st64:
		case opc_ld128:
		case opc_st128:
			printFmt(out, esc, " ");
			if (ip->opc == opc_lref) {
				offs = ip->arg.u32;
			}
			else {
				offs = (size_t) ip->rel;
			}

			printOfs(out, esc, ctx, NULL, offs, ofsMode);
			if (ctx != NULL) {
				sym = rtLookup(ctx, offs, 0);
				if (sym != NULL) {
					printFmt(out, esc, " ;%?T%?+d", sym, offs - sym->offs);
				}
				else if (ctx->cc != NULL) {
					char *str = vmPointer(ctx, offs);
					size_t hashTableSize = lengthOf(ctx->cc->stringTable);
					for (size_t i = 0; i < hashTableSize; i += 1) {
						for (list lst = ctx->cc->stringTable[i]; lst; lst = lst->next) {
							const char *data = lst->data;
							dieif(data == NULL, ERR_INTERNAL_ERROR);
							if (str >= data && str < data + strlen(data)) {
								printFmt(out, esc, " ;%c", type_fmt_string_chr);
								printFmt(out, esc ? esc : escapeStr(), "%s", str);
								printFmt(out, esc, "%c", type_fmt_string_chr);
								i = hashTableSize;
								break;
							}
						}
					}
				}
			}
			break;

		case opc_nfc:
			offs = (size_t) ip->rel;
			printFmt(out, esc, "(%d)", offs);

			if (ctx != NULL) {
				sym = NULL;
				if (ctx->vm.nfc != NULL) {
					sym = ((libc*)ctx->vm.nfc)[offs]->sym;
				}
				if (sym != NULL) {
					printFmt(out, esc, " ;%T", sym);
				}
			}
			break;

		case p4x_swz: {
			char c1 = "xyzw"[ip->idx >> 0 & 3];
			char c2 = "xyzw"[ip->idx >> 2 & 3];
			char c3 = "xyzw"[ip->idx >> 4 & 3];
			char c4 = "xyzw"[ip->idx >> 6 & 3];
			printFmt(out, esc, " %c%c%c%c(%02x)", c1, c2, c3, c4, ip->idx);
			break;
		}
	}
}

/// print-val.c ////////////////////////////////////////////////////////////////////////////////////////////////////////
void printOfs(FILE *out, const char **esc, rtContext ctx, symn sym, size_t offs, dmpMode mode) {
	if (ctx != NULL && offs >= ctx->_size) {
		// we should never get here, but ...
		printFmt(out, esc, "<BadRef @%06x>", offs);
		return;
	}

	if (sym != NULL) {
		size_t rel = offs - sym->offs;
		printFmt(out, esc, "%c%?.*T", '<', mode, sym);
		if (mode & prRelOffs && mode & prAbsOffs) {
			printFmt(out, esc, "%?+d @%06x%c", rel, offs, '>');
		}
		else if (mode & prRelOffs) {
			printFmt(out, esc, "%?+d%c", rel, '>');
		}
		else if (mode & prAbsOffs) {
			printFmt(out, esc, " @%06x%c", offs, '>');
		}
		else if (rel != 0) {
			printFmt(out, esc, "+?%c", '>');
		}
		else {
			printFmt(out, esc, "%c", '>');
		}
	}
	else if (mode & prAbsOffs) {
		printFmt(out, esc, "%c@%06x%c", '<', offs, '>');
	}
	else if (mode & prRelOffs) {
		printFmt(out, esc, "%c%+d%c", '<', offs, '>');
	}
	else {
		printFmt(out, esc, "%c%s%c", '<', "?", '>');
	}
}

/// Check if the pointer is inside the vm.
static int isValidOffset(rtContext ctx, void *ptr) {
	if ((unsigned char*)ptr > ctx->_mem + ctx->_size) {
		return 0;
	}
	if ((unsigned char*)ptr < ctx->_mem) {
		return 0;
	}
	return 1;
}

static int isRecursive(list prev, void *data) {
	for (list ptr = prev; ptr != NULL; ptr = ptr->next) {
		if (ptr->data == (char*) data) {
			return 1;
		}
	}
	return 0;
}
static void printValue(FILE *out, const char **esc, rtContext ctx, symn var, vmValue *val, list prev, dmpMode mode, int indent) {
	if (indent > 0) {
		printFmt(out, esc, "%I", indent);
	} else {
		indent = -indent;
	}

	const char *format = var->fmt;
	char *data = (char *)val;
	symn typ = var;
	if (!isTypename(var)) {
		typ = var->type;
		if (format == NULL) {
			format = typ->fmt;
		}
		if (isIndirect(var)) {
			data = vmPointer(ctx, val->ref);
		}
	}
	else if (val == vmPointer(ctx, var->offs)) {
		if (mode & prSymType) {
			printFmt(out, esc, "%.*T: ", mode & ~prSymType, var);
		}
		typ = var = ctx->main->fields;
		format = type_fmt_typename;
	}

	ccKind typCast = castOf(typ);
	if (var != typ) {
		printFmt(out, esc, "%.*T: ", mode & ~prSymType, var);
	}

	if (mode & prSymType) {
		printFmt(out, esc, "%.T(", typ);
	}

	struct list node;
	node.next = prev;
	node.data = (void *)data;

	// null reference.
	if (data == NULL) {
		printFmt(out, esc, "%s", "null");
	}
	// invalid offset.
	else if (!isValidOffset(ctx, data)) {
		printFmt(out, esc, "BadRef{offset}");
	}
	else if (isIndirect(typ) && isRecursive(prev, data)) {
		printFmt(out, esc, "BadRef{recursive}");
	}
	// builtin type.
	else if (format != NULL) {
		vmValue *ref = (vmValue *) data;
		if (format == type_fmt_typename) {
			printFmt(out, esc, type_fmt_typename, ref);
		}
		else if (format == type_fmt_string) {
			printFmt(out, esc, "%c", type_fmt_string_chr);
			printFmt(out, esc ? esc : escapeStr(), type_fmt_string, ref);
			printFmt(out, esc, "%c", type_fmt_string_chr);
		}
		else if (format == type_fmt_character) {
			printFmt(out, esc, "%c", type_fmt_character_chr);
			printFmt(out, esc ? esc : escapeStr(), type_fmt_character, ref->u08);
			printFmt(out, esc, "%c", type_fmt_character_chr);
		}
		else switch (typ->size) {
			default:
				// there should be no formatted(<=> builtin) type matching none of this size.
				printFmt(out, esc, "%s @%06x, size: %d", format, vmOffset(ctx, ref), var->size);
				break;

			case 1:
				if (typCast == CAST_u32) {
					printFmt(out, esc, format, ref->u08);
				}
				else {
					printFmt(out, esc, format, ref->i08);
				}
				break;

			case 2:
				if (typCast == CAST_u32) {
					printFmt(out, esc, format, ref->u16);
				}
				else {
					printFmt(out, esc, format, ref->i16);
				}
				break;

			case 4:
				if (typCast == CAST_f32) {
					printFmt(out, esc, format, ref->f32);
				}
				else if (typCast == CAST_u32) {
					// force zero extend (may sign extend to int64 ?).
					printFmt(out, esc, format, ref->u32);
				}
				else {
					printFmt(out, esc, format, ref->i32);
				}
				break;

			case 8:
				if (typCast == CAST_f64) {
					printFmt(out, esc, format, ref->f64);
				}
				else {
					printFmt(out, esc, format, ref->i64);
				}
				break;
		}
	}
	else if (indent > maxLogItems) {
		printFmt(out, esc, "...");
	}
	else if (typCast == CAST_enm) {
		printValue(out, esc, ctx, typ->type, val, &node, mode & ~(prSymQual | prSymType), -indent);
	}
	else if (typCast == CAST_var) {
		char *varData = vmPointer(ctx, val->ref);
		symn varType = vmPointer(ctx, val->type);

		if (varType == NULL || varData == NULL) {
			printFmt(out, esc, "%s", "null");
		}
		else if (mode & prOneLine) {
			printFmt(out, esc, "...");
		}
		else {
			printFmt(out, esc, "%.T: ", varType);
			printValue(out, esc, ctx, varType, (vmValue *) varData, &node, mode & ~(prSymQual | prSymType), -indent);
		}
	}
	else if (typCast == CAST_arr) {
		symn lenField = typ->fields;
		if (lenField != NULL && isStatic(lenField)) {
			// fixed size array or direct reference
			data = (char *)val;
			if (typ->type->fmt == type_fmt_character) {
				size_t length = typ->size / typ->type->size;
				if (strlen((char *) val) < length) {
					lenField = NULL;
				}
			}
		} else {
			// follow indirection
			data = vmPointer(ctx, val->ref);
		}

		if (data == NULL) {
			printFmt(out, esc, "%s", "null");
		}
		else if (!isValidOffset(ctx, data)) {
			printFmt(out, esc, "BadRef");
		}
		else if (typ->type->fmt == type_fmt_character && lenField == NULL) {
			// interpret `char[]`, `char[*]` and `char[n]` as string
			printFmt(out, esc, "%c", type_fmt_string_chr);
			printFmt(out, esc ? esc : escapeStr(), type_fmt_string, data);
			printFmt(out, esc, "%c", type_fmt_string_chr);
		}
		else if (mode & prOneLine) {
			printFmt(out, esc, "...");
		}
		else {
			size_t length = 0;
			size_t step = refSize(typ->type);
			unsigned minified = mode & prMinified;
			if (lenField && isStatic(lenField)) {
				// fixed size array
				step = typ->type->size;
				length = typ->size / step;
			}
			else if (lenField) {
				// dynamic size array
				// FIXME: slice without length information
				if (data != (char *)val) {
					length = val->length;
				}
			}

			int values = 0;
			if (isArrayType(typ->type)) {
				minified = 0;
			}
			printFmt(out, esc, "[%d] {", length);
			for (size_t idx = 0; idx < length; idx += 1) {
				if (minified == 0) {
					if (idx > 0) {
						printFmt(out, esc, ",");
					}
					printFmt(out, esc, "\n");
				}
				else if (idx > 0) {
					printFmt(out, esc, ", ");
				}
				if (idx >= maxLogItems) {
					printFmt(out, esc, "%I...", minified == 0 ? indent + 1 : 0);
					break;
				}
				vmValue *element = (vmValue *) (data + idx * step);
				if (isIndirect(typ->type)) {
					element = vmPointer(ctx, element->ref);
				}
				if (minified == 0) {
					printValue(out, esc, ctx, typ->type, element, &node, mode & ~(prSymQual | prSymType), indent + 1);
				} else {
					printValue(out, esc, ctx, typ->type, element, &node, mode & ~(prSymQual | prSymType), -indent);
				}
				values += 1;
			}
			if (values > 0 && minified == 0) {
				printFmt(out, esc, "\n%I", indent);
			}
			printFmt(out, esc, "}");
		}
	}
	else {
		// typename, function, pointer, etc.
		int fields = 0;
		if (isObjectType(typ)) {
			typ = vmPointer(ctx, ((vmValue *) data)->ref);
		}
		if (typ->fields != NULL && (mode & prOneLine) == 0) {
			for (symn sym = typ->fields; sym; sym = sym->next) {
				if (isStatic(sym)) {
					// skip static fields
					continue;
				}

				if (!isVariable(sym)) {
					// skip types, functions and aliases
					continue;
				}

				if (fields > 0) {
					if (sym->offs == typ->fields->offs) {
						// show only first union field
						continue;
					}
					printFmt(out, esc, ",");
				}
				else if (typ != typ->type) {
					printFmt(out, esc, "{");
				}

				if (typ != typ->type && sym == sym->type) {
					printValue(out, esc, ctx, sym, (vmValue *) (data + sym->offs), &node, (mode | prMember) & ~prSymQual, -indent);
				} else {
					printFmt(out, esc, "\n");
					printValue(out, esc, ctx, sym, (vmValue *) (data + sym->offs), &node, (mode | prMember) & ~prSymQual, indent + 1);
				}
				fields += 1;
			}
			if (fields > 0 && typ != typ->type) {
				printFmt(out, esc, "\n%I}", indent);
			}
		}
		if (fields == 0) {
			size_t offs = vmOffset(ctx, data);
			symn sym = rtLookup(ctx, offs, 0);
			printOfs(out, esc, ctx, sym, offs, (mode | prSymQual) & ~prSymType);
		}
	}

	if (mode & prSymType) {
		printFmt(out, esc, ")");
	}
}

void printVal(FILE *out, const char **esc, rtContext ctx, symn var, void *val, dmpMode mode, int indent) {
	printValue(out, esc, ctx, var, val, NULL, mode, indent);
}
