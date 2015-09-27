/* command line options
application [global options] [local options]...

	<global options>:
		-g[d][-]<int>			generate
			d:					generate debug info
			-:					generate globals on stack
			int:				optimization level, max 255

		-x[v][d|D[:<type>]]		execute
			v:					print global variable values
			d|D:				debug application
			<type>				print top of stack as it would be a variable of type
				ex: -xd:int32[20]

		-l <file>				logger
		-o <file>				output

		-api[<hex>][.<sym>]		output symbols
		-asm[<hex>][.<fun>]		output assembly
		-ast[<hex>]				output syntax tree

		--[s|c]*				TEMP: skip: stdlib | code geration

	<local options>:
		-w[a|x|<hex>]			set warning level
		-L<file>				load library (.so|.dll)
		-C<file>				compile source
		-b<int>					breakpoint on line
		<file>					if file extension is (.so|.dll) load as libraray else compile
*/

//~ (wcl386 -cc -q -ei -6s -d0  -fe=../main *.c) && (rm -f *.o *.obj *.err)
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "core.h"

// enable dynamic dll/so lib loading
#define USEPLUGINS

// default values
static const int wl = 19;			// warning level
static const int ol = 2;			// optimize level
static char mem[8 << 20];			// runtime memory

const char* STDLIB = "stdlib.cvx";		// standard library

#ifdef __linux__
#define stricmp(__STR1, __STR2) strcasecmp(__STR1, __STR2)
#endif

static inline int streq(const char *a, const char *b) {
	return strcmp(a, b) == 0;
}

static char* parsei32(const char* str, int32_t* res, int radix) {
	int64_t result = 0;
	int sign = 1;

	//~ ('+' | '-')?
	switch (*str) {
		case '-':
			sign = -1;
		case '+':
			str += 1;
		default:
			break;
	}

	if (radix == 0) {		// detect
		radix = 10;
		if (*str == '0') {
			str += 1;

			switch (*str) {

				default:
					radix = 8;
					break;

				case 'b':
				case 'B':
					radix = 2;
					str += 1;
					break;

				case 'o':
				case 'O':
					radix = 8;
					str += 1;
					break;

				case 'x':
				case 'X':
					radix = 16;
					str += 1;
					break;

			}
		}
	}

	//~ ([0-9])*
	while (*str != 0) {
		int value = *str;
		if (value >= '0' && value <= '9') {
			value -= '0';
		}
		else if (value >= 'a' && value <= 'z') {
			value -= 'a' - 10;
		}
		else if (value >= 'A' && value <= 'Z') {
			value -= 'A' - 10;
		}
		else {
			break;
		}

		if (value > radix) {
			break;
		}

		result *= radix;
		result += value;
		str += 1;
	}

	*res = (int32_t)(sign * result);

	return (char*)str;
}
static char* parsecmd(char* ptr, const char* cmd, const char* sws) {
	while (*cmd && *cmd == *ptr) {
		cmd++, ptr++;
	}

	if (*cmd != 0) {
		return 0;
	}

	if (*ptr == 0) {
		return ptr;
	}

	while (strchr(sws, *ptr)) {
		++ptr;
	}

	return ptr;
}

static void usage(char* prog) {
	FILE *out = stdout;
	fputfmt(out, "Usage: %s =<eval expression>\n", prog);
	fputfmt(out, "Usage: %s <global options> <local options>*\n", prog);

	fputfmt(out, "	<global options>:\n");
	fputfmt(out, "		-O<int>		optimize\n");
	fputfmt(out,"\n");
	fputfmt(out, "		-l <file>	logger\n");
	fputfmt(out, "		-o <file>	output\n");
	fputfmt(out,"\n");
	fputfmt(out, "		-x[v][d<hex>]	execute\n");
	fputfmt(out, "			v: print global variables\n");
	fputfmt(out, "			d<hex>: debug using level <hex>\n");
	fputfmt(out, "		--[s|c]*		skip stdlib / code generation\n");
	fputfmt(out,"\n");
	fputfmt(out,"		-api[<hex>][.<sym>]		output symbols\n");
	fputfmt(out,"		-asm[<hex>][.<fun>]		output assembly\n");
	fputfmt(out,"		-ast[<hex>]	output syntax tree\n");
	fputfmt(out,"\n");
	fputfmt(out,"	<local options>:\n");
	fputfmt(out,"		-w[a|x|<hex>]		set warning level\n");
	fputfmt(out,"		-L<file>	import plugin (.so|.dll)\n");
	fputfmt(out,"		-C<file>	compile source\n");
	fputfmt(out,"		<file>		if file extension is (.so|.dll) import else compile\n");
}

static int evalexp(state rt, char* text) {
	struct astNode res;
	astn ast = NULL;
	symn typ = NULL;
	int tid = 0;

	ccState cc = ccInit(rt, creg_def, NULL);
	ccOpen(cc->s, NULL, 0, text);

	//ast = expr(cc, TYPE_def);
	typ = typecheck(cc, NULL, ast);
	tid = eval(&res, ast);

	ccDone(cc);

	fputfmt(cc->s->logf, "eval(`%+t`) = ", ast);

	if (ast && typ && tid) {
		fputfmt(cc->s->logf, "%T(%t)\n", typ, &res);
		return 0;
	}

	fputfmt(cc->s->logf, "ERROR(typ:`%T`, tid:%d)\n", typ, tid);

	return -1;
}

#if defined(USEPLUGINS)
static const char* pluginLibInstall = "ccvmInit";
static const char* pluginLibDestroy = "ccvmDone";

static int installDll(state rt, int ccApiMain(state)) {
	return ccApiMain(rt);
}

#if (defined(WIN32) || defined(_WIN32))

#include <windows.h>
static struct pluginLib {
	struct pluginLib *next;		// next plugin
	void(*onClose)();
	HANDLE lib;					//
} *pluginLibs = NULL;
static void closeLibs() {
	while (pluginLibs != NULL) {
		struct pluginLib *lib = pluginLibs;
		pluginLibs = lib->next;

		if (lib->onClose) {
			lib->onClose();
		}
		if (lib->lib) {
			FreeLibrary((HINSTANCE)lib->lib);
		}
		free(lib);
	}
}
static int importLib(state rt, const char* path) {
	int result = -1;
	HANDLE library = LoadLibraryA(path);
	if (library != NULL) {
		void *install = (void*)GetProcAddress(library, pluginLibInstall);
		void *destroy = (void*)GetProcAddress(library, pluginLibDestroy);
		if (install != NULL) {
			struct pluginLib *lib = malloc(sizeof(struct pluginLib));
			if (lib != NULL) {
				result = installDll(rt, install);
				lib->lib = library;
				lib->onClose = destroy;
				lib->next = pluginLibs;
				pluginLibs = lib;
			}
		}
		else {
			result = -2;
		}
	}
	return result;
}

#elif (defined(__linux__))
#include <dlfcn.h>
static struct pluginLib {
	struct pluginLib *next;		// next plugin
	void (*onClose)();
	void *lib;					//
} *pluginLibs = NULL;
static void closeLibs() {
	while (pluginLibs != NULL) {
		struct pluginLib *lib = pluginLibs;
		pluginLibs = lib->next;

		if (lib->onClose) {
			lib->onClose();
		}
		if (lib->lib) {
			dlclose(lib->lib);
		}
		free(lib);
	}
}
static int importLib(state rt, const char* path) {
	int result = -1;
	void* library = dlopen(path, RTLD_NOW);
	if (library != NULL) {
		void* install = dlsym(library, pluginLibInstall);
		void* destroy = dlsym(library, pluginLibDestroy);
		if (install != NULL) {
			struct pluginLib *lib = malloc(sizeof(struct pluginLib));
			result = installDll(rt, install);
			lib->onClose = destroy;
			lib->next = pluginLibs;
			lib->lib = library;
			pluginLibs = lib;
		}
		else {
			result = -2;
		}
	}
	return result;
}
#else

static void closeLibs() {}
static int importLib(state rt, const char* path) {
	(void)rt;
	(void)path;
	(void)pluginLibInstall;
	(void)pluginLibDestroy;
	error(rt, path, 1, "dynamic linking is not available in this build.");
	return -1;
}

#endif

#else
static void closeLibs() {}
static int importLib(state rt, const char* path) {
	(void)rt;
	(void)path;
	error(rt, path, 1, "dynamic linking is not available in this build.");
	return -1;
}
#endif

static symn printvars = NULL;
static void printGlobals(FILE* out, state rt, int all);
static int dbgConsole(state, int pu, void* ip, void* sp, size_t ss, vmError err, size_t fp);

static void dumpJsAst(FILE* fout, astn ast, const char *kind, int indent) {
	static const char **esc = NULL;
	static const char* KEY_KIND = "kind";
	static const char* KEY_CAST = "cast";
	static const char* KEY_FILE = "file";
	static const char* KEY_LINE = "line";
	static const char* KEY_TYPE = "type";
	static const char* KEY_STMT = "stmt";
	static const char* KEY_INIT = "init";
	static const char* KEY_TEST = "test";
	static const char* KEY_THEN = "then";
	static const char* KEY_STEP = "step";
	static const char* KEY_ELSE = "else";
	static const char* KEY_ARGS = "args";
	static const char* KEY_LHSO = "lval";
	static const char* KEY_RHSO = "rval";
	static const char* KEY_VALUE = "value";

	if (esc == NULL) {
		// layzy initialize on first function call
		static const char *esc_js[256];
		memset(esc_js, 0, sizeof(esc_js));
		esc_js['\n'] = "\\n";
		esc_js['\r'] = "\\r";
		esc_js['\t'] = "\\t";
		esc_js['\''] = "\\'";
		esc_js['\"'] = "\\\"";
		esc = esc_js;
	}
	if (ast == NULL) {
		return;
	}
	if (kind != NULL) {
		fputesc(fout, esc, "%I%s: { // %t\n", indent - 1, kind, ast);
	}

	fputesc(fout, esc, "%I%s: '%K',\n", indent, KEY_KIND, ast->kind);
	if (ast->type != NULL) {
		fputesc(fout, esc, "%I%s: '%T',\n", indent, KEY_TYPE, ast->type);
	}
	if (ast->cst2 != TYPE_any) {
		fputesc(fout, esc, "%I%s: '%K',\n", indent, KEY_CAST, ast->cst2);
	}
	if (ast->file != NULL) {
		fputesc(fout, esc, "%I%s: '%s',\n", indent, KEY_FILE, ast->file);
	}
	if (ast->line != 0) {
		fputesc(fout, esc, "%I%s: '%u',\n", indent, KEY_LINE, ast->line);
	}
	switch (ast->kind) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			fatal("%K", ast->kind);
			return;
		//#{ STMT
		case STMT_do:
			dumpJsAst(fout, ast->stmt.stmt, KEY_STMT, indent + 1);
			break;

		case STMT_beg: {
			astn list;
			fputesc(fout, esc, "%I%s: [{", indent, KEY_STMT);
			for (list = ast->stmt.stmt; list; list = list->next) {
				if (list != ast->stmt.stmt) {
					fputesc(fout, esc, "%I}, {", indent, list);
				}
				fputesc(fout, esc, " // %t\n", list);
				dumpJsAst(fout, list, NULL, indent + 1);
			}
			fputesc(fout, esc, "%I}],\n", indent);
			break;
		}

		case STMT_if:
			dumpJsAst(fout, ast->stmt.test, KEY_TEST, indent + 1);
			dumpJsAst(fout, ast->stmt.stmt, KEY_THEN, indent + 1);
			dumpJsAst(fout, ast->stmt.step, KEY_ELSE, indent + 1);
			break;

		case STMT_for:
			dumpJsAst(fout, ast->stmt.init, KEY_INIT, indent + 1);
			dumpJsAst(fout, ast->stmt.test, KEY_TEST, indent + 1);
			dumpJsAst(fout, ast->stmt.step, KEY_STEP, indent + 1);
			dumpJsAst(fout, ast->stmt.stmt, KEY_STMT, indent + 1);
			break;

		case STMT_con:
		case STMT_brk:
			break;

		case STMT_ret:
			dumpJsAst(fout, ast->stmt.stmt, KEY_STMT, indent + 1);
			break;

		//#}
		//#{ OPER
		case OPER_fnc: {	// '()'
			astn args = ast->op.rhso;
			fputesc(fout, esc, "%I%s: [{", indent, KEY_ARGS);
			if (args != NULL) {
				while (args && args->kind == OPER_com) {
					if (args != ast->stmt.stmt) {
						fputesc(fout, esc, "%I}, {", indent);
					}
					fputesc(fout, esc, " // %t\n", args->op.rhso);
					dumpJsAst(fout, args->op.rhso, NULL, indent + 1);
					args = args->op.lhso;
				}
				if (args != ast->stmt.stmt) {
					fputesc(fout, esc, "%I}, {", indent, args);
				}
				fputesc(fout, esc, " // %t\n", args);
				dumpJsAst(fout, args, NULL, indent + 1);
			}
			fputesc(fout, esc, "%I}],\n", indent);
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
			dumpJsAst(fout, ast->op.test, KEY_TEST, indent + 1);
			dumpJsAst(fout, ast->op.lhso, KEY_LHSO, indent + 1);
			dumpJsAst(fout, ast->op.rhso, KEY_RHSO, indent + 1);
			break;

		//#}
		//#{ TVAL
		case EMIT_opc:
			//~ fputesc(fout, escape, " />\n", text);
			//~ break;

		case TYPE_int:
		case TYPE_flt:
		case TYPE_str:

		case TYPE_ref:
		case TYPE_def:	// TODO: see dumpxml
			fputesc(fout, esc, "%I%s: \"%t\"\n", indent, KEY_VALUE, ast);
			break;

		//#}
	}

	if (kind != NULL) {
		fputesc(fout, esc, "%I},\n", indent - 1);
		//~ fputesc(fout, esc, "%I%s: {", indent - 1, kind);
	}
}
static void dumpJsSym(FILE* fout, symn ptr, const char *kind, int indent) {
	static const char **esc = NULL;

	static const char* KEY_KIND = "kind";
	static const char* KEY_FILE = "file";
	static const char* KEY_LINE = "line";
	static const char* KEY_NAME = "name";
	static const char* KEY_PROTO = "proto";
	static const char* KEY_DECL = "declaredIn";
	static const char* KEY_TYPE = "type";
	static const char* KEY_INIT = "init";
	static const char* KEY_ARGS = "args";
	static const char* KEY_CONST = "const";
	static const char* KEY_STAT = "static";
	//~ static const char* KEY_PARLEL = "parallel";
	static const char* KEY_CAST = "cast";
	static const char* KEY_SIZE = "size";
	static const char* KEY_OFFS = "offs";

	static const char* VAL_TRUE = "true";
	static const char* VAL_FALSE = "false";

	static const char* KIND_PARAM = "param";

	if (esc == NULL) {
		// layzy initialize on first function call
		static const char *esc_js[256];
		memset(esc_js, 0, sizeof(esc_js));
		esc_js['\n'] = "\\n";
		esc_js['\r'] = "\\r";
		esc_js['\t'] = "\\t";
		esc_js['\''] = "\\'";
		esc_js['\"'] = "\\\"";
		esc = esc_js;
	}

	fputesc(fout, esc, "%I%s: '%s',\n", indent, KEY_KIND, kind);
	fputesc(fout, esc, "%I%s: '%.0T',\n", indent, KEY_NAME, ptr);
	fputesc(fout, esc, "%I%s: '%+T',\n", indent, KEY_PROTO, ptr);

	if (ptr->decl != NULL) {
		fputesc(fout, esc, "%I%s: '%+T',\n", indent, KEY_DECL, ptr->decl);
	}

	if (ptr->type != NULL) {
		fputesc(fout, esc, "%I%s: '%T',\n", indent, KEY_TYPE, ptr->type);
	}
	if (ptr->file != NULL) {
		fputesc(fout, esc, "%I%s: '%s',\n", indent, KEY_FILE, ptr->file);
	}
	if (ptr->line != 0) {
		fputesc(fout, esc, "%I%s: '%u',\n", indent, KEY_LINE, ptr->line);
	}
	if (ptr->init != NULL) {
		//~ fputesc(fout, esc, "%I%s: '%+t'\n", indent, KEY_INIT, ptr->init);
		dumpJsAst(fout, ptr->init, KEY_INIT, indent + 1);
	}

	if (ptr->call && ptr->prms) {
		symn arg;
		fputesc(fout, NULL, "%I%s: [{", indent, KEY_ARGS);
		for (arg = ptr->prms; arg; arg = arg->next) {
			if (arg != ptr->prms) {
				fputesc(fout, NULL, "%I}, {", indent);
			}
			fputesc(fout, NULL, " // %+T: %+T\n", arg, arg->type);
			dumpJsSym(fout, arg, KIND_PARAM, indent + 1);
		}
		fputesc(fout, NULL, "%I}],\n", indent);
	}
	if (ptr->cast != 0) {
		fputesc(fout, NULL, "%I%s: '%K',\n", indent, KEY_CAST, ptr->cast);
	}
	fputesc(fout, NULL, "%I%s: %u,\n", indent, KEY_SIZE, ptr->size);
	fputesc(fout, NULL, "%I%s: %u,\n", indent, KEY_OFFS, ptr->offs);
	fputesc(fout, NULL, "%I%s: %s,\n", indent, KEY_CONST, ptr->cnst ? VAL_TRUE : VAL_FALSE);
	fputesc(fout, NULL, "%I%s: %s\n", indent, KEY_STAT, ptr->stat ? VAL_TRUE : VAL_FALSE);
	// no parallel symbols!!! fputesc(fout, NULL, "%I%s: %s,\n", indent, KEY_PARLEL, ptr->stat ? VAL_TRUE : VAL_FALSE);
}

int program(int argc, char* argv[]) {
	char* stdlib = (char*)STDLIB;

	// compile, run, debug, ...
	int level = -1, argi;
	int gen_code = ol;	// optimize_level, debug_info, ...
	int run_code = 0;	// true/false: exec

	char* stk_dump = NULL;	// dump top of stack
	int val_dump = -1;	// dump variable values after execution.
	int sym_dump = -1;	// dump variables and functions.
	int ast_dump = -1;	// dump abstract syntax tree.
	int asm_dump = -1;	// dump disassembly.
	int bin_dump = -1;	// dump binary.
	int api_dump = 0;	// dump api for scite / json.

	enum {
		sym_attr = 0x01,   // size, kind, attributes, initializer, ...
		sym_refs = 0x02,   // dump usage references of variable
		sym_prms = 0x04,   // print params

		asm_lofs = 0x10,   // local offsets
		asm_gofs = 0x20,   // global offsets
		asm_data = 0x0f,   // print 0-15 bytes as hex code
	};

	int log_append = 0;				// start with a new file
	char* logf = NULL;			// logger filename
	FILE* dmpf = stdout;		// dump file

	int warn = wl;
	int result = 0;

	char* amem = paddptr(mem, rt_size);
	state rt = rtInit(amem, sizeof(mem) - (amem - mem));
	int (*dbg)(state, int pu, void* ip, void* sp, size_t ss, vmError err, size_t fp) = NULL;

	// max 32 break points
	char* bp_file[32];
	int bp_line[32];
	int bp_size = 0;

	if (rt == NULL) {
		fatal("initializing runtime context");
		return -1;
	}

	// evaluate constant expression.
	if (argc == 2 && *argv[1] == '=') {
		return evalexp(rt, argv[1] + 1);
	}

	// porcess global options
	for (argi = 1; argi < argc; ++argi) {
		char* arg = argv[argi];

		// generate code
		if (strncmp(arg, "-g", 2) == 0) {			// generate code
			char* str = arg + 2;

			if (*str == 'd') {
				// generate debug info
				gen_code |= cgen_info;
				str += 1;
			}
			if (*str == '-') {
				// generate globals on stack
				gen_code |= cgen_glob;
				str += 1;
			}
			if (*str) {
				if (!parsei32(str, &level, 10)) {
					error(rt, NULL, 0, "invalid level '%s'", str);
					return 0;
				}
				gen_code &= ~cgen_opti;
				gen_code |= level & cgen_opti;
			}
		}

		// execute code
		else if (strncmp(arg, "-x", 2) == 0) {		// execute(&| debug)
			char* str = arg + 2;

			if (*str == 'v') {
				val_dump = 0;
				str += 1;
			}
			else if (*str == 'V') {
				val_dump = 1;
				str += 1;
			}

			if (*str == 'd') {

				gen_code |= cgen_info;
				dbg = dbgConsole;

				if (str[1] == ':') {
					stk_dump = str + 2;
					str += 1;
				}
			}
			run_code = 1;
		}

		// output file
		else if (strncmp(arg, "-l", 2) == 0) {		// log
			// append to existing log
			if (arg[2] == 'a') {
				log_append = 1;
			}

			if (++argi >= argc || logf) {
				error(rt, NULL, 0, "log file not specified");
				return -1;
			}
			logf = argv[argi];
		}
		/*else if (strcmp(arg, "-o") == 0) {		// out
			if (++argi >= argc || outf) {
				error(rt, NULL, 0, "output file not specified");
				return -1;
			}
			outf = argv[argi];
		}*/

		// output what
		else if (strncmp(arg, "-api", 4) == 0) {	// tags
			level = 0x32;
			if (arg[4]) {
				char* ptr = parsei32(arg + 4, &level, 16);
				if (*ptr) {
					error(rt, NULL, 0, "invalid argument '%s'\n", arg);
					return 0;
				}
			}
			else {
				api_dump = 2;
			}
			sym_dump = level;
		}
		else if (strncmp(arg, "-ast", 4) == 0) {	// tree
			level = 0x7f;
			if (arg[4]) {
				char* ptr = parsei32(arg + 4, &level, 16);
				if (*ptr) {
					error(rt, NULL, 0, "invalid argument '%s'\n", arg);
					return 0;
				}
			}
			ast_dump = level;
		}
		else if (strncmp(arg, "-asm", 4) == 0) {	// dasm
			level = 0x29;	// 2 use global offset, 9 characters for bytecode hexview
			if (arg[4]) {
				char* ptr = parsei32(arg + 4, &level, 16);
				if (*ptr) {
					error(rt, NULL, 0, "invalid argument '%s'\n", arg);
					return 0;
				}
			}
			asm_dump = level;
		}
		else if (strncmp(arg, "-bin", 4) == 0) {	// dasm
			level = 16;	// default value: characters per line
			if (arg[4]) {
				char* ptr = parsei32(arg + 4, &level, 16);
				if (*ptr) {
					error(rt, NULL, 0, "invalid argument '%s'\n", arg);
					return 0;
				}
			}
			bin_dump = level;
		}

		// temp
		else if (strncmp(arg, "-std", 4) == 0) {		// override stdlib file
			stdlib = arg + 4;
		}
		else if (strncmp(arg, "--", 2) == 0) {		// exclude stdlib, do not gen code, ...
			if (strchr(arg, 'c'))
				gen_code = 0;
			if (strchr(arg, 's'))
				stdlib = NULL;
		}

		else break;
	}

	// open log file (global option)
	if (logf && logfile(rt, logf, log_append) != 0) {
		error(rt, NULL, 0, "can not open log file: `%s`", logf);
		return -1;
	}

	// intstall base type system.
	if (!ccInit(rt, creg_def, NULL)) {
		error(rt, NULL, 0, "error registering base types");
		logfile(rt, NULL, 0);
		return -6;
	}

	// intstall standard libraries.
	if (!ccAddUnit(rt, install_stdc, 3, stdlib)) {
		error(rt, NULL, 0, "error registering standard libs");
		logfile(rt, NULL, 0);
		return -6;
	}

	// intstall file libraries.
	if (!ccAddUnit(rt, install_file, 0, NULL)) {
		error(rt, NULL, 0, "error registering file libs");
		logfile(rt, NULL, 0);
		return -6;
	}

	// compile and import files / modules
	for (; argi < argc; ++argi) {
		char* arg = argv[argi];
		if (*arg != '-') {
			int i;
			char* ext = strrchr(arg, '.');
			if (ext && (streq(ext, ".so") || streq(ext, ".dll"))) {
				int resultCode = importLib(rt, arg);
				if (resultCode != 0) {
					error(rt, NULL, 0, "error(%d) importing library `%s`", resultCode, arg);
				}
			}
			else if (!ccAddCode(rt, warn, arg, 1, NULL)) {
				error(rt, NULL, 0, "error compiling source `%s`", arg);
			}
			if (bp_size > 0) {
				for (i = bp_size - 1; i >= 0; i -= 1) {
					if (bp_file[i] == NULL) {
						bp_file[i] = arg;
					}
				}
			}
		}
		else if (strncmp(arg, "-b", 2) == 0) {		// warning level for file
			int line = 0;
			if (bp_size > lengthOf(bp_file)) {
				error(rt, NULL, 0, "maximum 32 breakponts are available.");
				return 1;
			}
			if (*parsei32(arg + 2, &line, 10)) {
				error(rt, NULL, 0, "invalid warning level '%s'", arg + 2);
				return 1;
			}
			bp_file[bp_size] = NULL;
			bp_line[bp_size] = line;
			bp_size += 1;
		}
		else if (strncmp(arg, "-w", 2) == 0) {		// warning level for file
			if (strcmp(arg, "-wx") == 0)
				warn = -1;
			else if (strcmp(arg, "-wa") == 0)
				warn = 32;
			else if (*parsei32(arg + 2, &warn, 10)) {
				error(rt, NULL, 0, "invalid warning level '%s'", arg + 2);
				return 1;
			}
		}
		else if (strncmp(arg, "-L", 2) == 0) {		// import library
			char* str = arg + 2;
			int resultCode = importLib(rt, str);
			if (resultCode != 0) {
				error(rt, NULL, 0, "error(%d) importing library `%s`", resultCode, arg);
			}
		}
		else if (strncmp(arg, "-C", 2) == 0) {		// compile source
			char* str = arg + 2;
			if (!ccAddCode(rt, warn, str, 1, NULL)) {
				error(rt, NULL, 0, "error compiling source `%s`", arg);
			}
		}
		else {
			error(rt, NULL, 0, "invalid option: `%s`", arg);
		}
	}

	// compile the given type for the debuging
	if (stk_dump != NULL) {
		ccState cc = ccOpen(rt, NULL, 0, stk_dump);
		if (cc != NULL) {
			printvars = linkOf(decl_var(cc, NULL, TYPE_def));
			if (printvars != NULL) {
				printvars->name = "sp";	// stack pointer
			}
			else {
				// TODO: should this rise error or just warn?
				error(rt, NULL, 0, "error in debug print format `%s`", stk_dump);
			}
			ccDone(cc);
		}
	}

	// generate code only if needed and there are no compilation errors
	if ((gen_code || run_code) && rt->errc == 0) {
		int i;
		// generate variables and vm code.
		if (!gencode(rt, gen_code)) {
			// show dump, but do not execute broken code.
			//error(rt, NULL, 0, "error generating code");
			run_code = 0;
		}
		// set breakpoints
		for (i = 0; i < bp_size; ++i) {
			char *file = bp_file[i];
			int line = bp_line[i];
			dbgInfo dbg = getDbgStatement(rt, file, line);
			if (dbg != NULL) {
				dbg->bp = 1;
			}
		}
	}

	if (rt->logf != NULL) {
		dmpf = rt->logf;
	}

	if (sym_dump >= 0 || ast_dump >= 0 || asm_dump >= 0) {
		static const int identExt = 1;
		symn sym, bp[TOKS], *sp = bp;
		FILE* fout = rt->logf;
		char *typ = "";

		if (api_dump && (ast_dump >= 0 || asm_dump >= 0)) {
			// invalidate api dump if assembly or syntax tree dump is requested.
			api_dump = 0;
		}

		if (api_dump == 2) {
			fputfmt(fout, "var api = [\n");
		}

		for (*sp = rt->defs; sp >= bp;) {
			int dumpAst = 0;
			int dumpAsm = 0;
			if (!(sym = *sp)) {
				--sp;
				continue;
			}
			*sp = sym->next;

			switch (sym->kind) {
				// inline/constant
				case TYPE_def:
					typ = "alias";
					/* print params of inline expressions
					if (sym->call && sym->prms) {
						*++sp = sym->prms;
					}// */
					break;

				// array/typename
				case TYPE_arr:
				case TYPE_rec:
					typ = "typename";
					*++sp = sym->flds;
					break;

				// variable/function
				case TYPE_ref:
					typ = "variable";
					/* print params of functions
					if (sym->call && sym->prms) {
						*++sp = sym->prms;
					}
					// */
					break;

				case EMIT_opc:
					typ = "opcode";
					*++sp = sym->flds;
					break;

				default:
					typ = "!!!";
					trace("psym:%d:%T['%K']", sym->kind, sym, sym->kind);
					break;
			}

			if (api_dump) {
				if (api_dump == 2) {
					if (sym != rt->defs) {
						//~ fputfmt(fout, "}, {\n");
					}
					fputfmt(fout, "{ // %+T: %+T\n", sym, sym->type);
					dumpJsSym(fout, sym, typ, 1);
					fputfmt(fout, "},\n", sym);
				}
				else {
					// api dump for SciTE.
					fputfmt(fout, "%+T", sym);
					if (sym->call && sym->kind != TYPE_ref) {
						fputfmt(fout, ": %+T", sym->type);
					}
					fputfmt(fout, "\n");
				}
				fflush(fout);
				continue;
			}

			if (asm_dump >= 0 && sym->call && sym->kind == TYPE_ref) {
				if (sym->cast != TYPE_ref) {
					// function reference may point to null or another function.
					dumpAsm = asm_dump;
				}
			}
			if (ast_dump >= 0 && sym->init != NULL) {
				dumpAst = ast_dump;
			}

			// filter symbols by request.
			if (sym_dump < 0) {
				// skip builtin symbols if not requested (with '-api').
				if (sym->file == NULL && sym->line == 0) {
					continue;
				}
				// skip symbol if there is nothing to dump.
				if (!dumpAsm && !dumpAst) {
					continue;
				}
			}

			// print symbol definition location
			if (sym->file != NULL && sym->line > 0) {
				fputfmt(fout, "%s:%u: ", sym->file, sym->line);
			}

			// print qualified name with arguments
			fputfmt(fout, "%+T: %+T {\n", sym, sym->type);

			// print symbol info (kind, size, offset, ...)
			if (sym_dump >= 0 && sym_dump & sym_attr) {
				fputfmt(fout, "%I.kind:%?s%?s %s %?K\n", identExt
					, sym->stat ? " static" : ""
					, sym->cnst ? " const" : ""
					, typ, sym->cast
				);
				fputfmt(fout, "%I.offset: %06x\n", identExt, sym->offs);
				fputfmt(fout, "%I.size: %d\n", identExt, sym->size);
			}

			// explain params of the function
			if (sym_dump >= 0 && sym_dump & sym_prms) {
				symn param;
				for (param = sym->prms; param; param = param->next) {
					fputfmt(fout, "%I.param %T: %?+T (@%06x+%d->%K)\n", identExt, param, param->type, param->offs, param->size, param->cast);
				}
			}

			if (dumpAst != 0) {
				fputfmt(fout, "%I.syntaxTree: %?-1.*t", identExt, dumpAst, sym->init);
				if (sym->init->kind != STMT_beg) {
					fputfmt(fout, "\n");
				}
			}

			// print disassembly of the function
			if (dumpAsm != 0) {
				fputfmt(fout, "%I.assembly [@%06x: %d] {\n", identExt, sym->offs, sym->size);
				dumpasm(rt, fout, sym, ((identExt + 1) << 8) | (asm_dump & 0xff));
				fputfmt(fout, "%I}\n", identExt);
			}

			// print usages of symbol
			if (sym_dump >= 0 && sym_dump & sym_refs) {
				astn usage;
				int usages = 0;
				int extUsages = 0;
				for (usage = sym->used; usage; usage = usage->ref.used) {
					if (usage->file && usage->line) {
						int referenced = !(usage->file == sym->file && usage->line == sym->line);
						fputfmt(fout, "%I%s:%u: %s as `%+t`\n", identExt, usage->file, usage->line, referenced ? "referenced" : "defined", usage);
						#ifdef LOG_MAX_ITEMS
						if ((usages += 1) > LOG_MAX_ITEMS) {
							break;
						}
						#endif
					}
					else {
						extUsages += 1;
					}
				}
				if (extUsages > 0) {
					fputfmt(fout, "%Iexternal references: %d\n", identExt, extUsages);
				}
				(void)usages;
			}

			fputfmt(fout, "}\n");
			fflush(fout);
		}

		if (api_dump == 2) {
			fputfmt(fout, "];\n", sym);
		}
	}

	if (bin_dump > 0) {
		size_t i, max = rt->_beg - rt->_mem;

		FILE* fout = rt->logf;

		for (i = 0; i < max; i += 1) {
			int val = rt->_mem[i];
			if (((i % bin_dump) == 0) && i != 0) {
				unsigned int j = i - bin_dump;
				fputfmt(fout, " ");
				for ( ; j < i; j += 1) {
					int chr = rt->_mem[j];

					if (chr < 32 || chr > 127)
						chr = '.';

					fputfmt(fout, "%c", chr);
				}
				fputfmt(fout, "\n");
			}
			else if (i != 0) {
				fputfmt(fout, " ");
			}
			fputfmt(fout, "%c", "0123456789abcdef"[val >> 16 & 0x0f]);
			fputfmt(fout, "%c", "0123456789abcdef"[val >>  0 & 0x0f]);
		}
	}

	// run code if there is no error.
	if (run_code != 0 && rt->errc == 0) {
		if (dbg != NULL && rt->dbg != NULL) {
			rt->dbg->dbug = dbg;
			rt->dbg->breakLt = rt->vm.ro;
			rt->dbg->breakGt = rt->vm.px + px_size;
		}
		fputfmt(dmpf, "\n>/*-- exec:\n");
		result = execute(rt, NULL, rt->_size / 4);
		fputfmt(dmpf, "\n// */\n");
		if (val_dump >= 0) {

			fputfmt(dmpf, "\n>/*-- vars:\n");
			printGlobals(dmpf, rt, val_dump);
			fputfmt(dmpf, "// */\n");

			//~ fputfmt(dmpf, "\n>/*-- trace:\n");
			//~ logTrace(rt, NULL, 1, 0, 20);
			//~ fputfmt(dmpf, "// */\n");

			fputfmt(dmpf, "\n>/*-- trace:\n");
			if (rt->dbg) {
				int n = rt->dbg->functions.cnt;
				dbgInfo dbg = (dbgInfo)rt->dbg->functions.ptr;
				while (n > 0) {
					if (dbg->hits) {
						symn sym = dbg->decl;
						if (sym == NULL) {
							sym = mapsym(rt, dbg->start, 1);
						}
						fputfmt(dmpf, "%s:%u:[.%06x, .%06x): <%?.0T> hits(%D), time(%D%?+D / %.3F%?+.3F ms)\n"
							, dbg->file, dbg->line, dbg->start, dbg->end, sym
							, (int64_t)dbg->hits, (int64_t)dbg->funcTime, (int64_t)dbg->diffTime
							, dbg->funcTime / (double)CLOCKS_PER_SEC, dbg->diffTime / (double)CLOCKS_PER_SEC
						);
					}
					dbg++;
					n--;
				}
				fputfmt(dmpf, ">//-- trace.statements:\n");
				n = rt->dbg->statements.cnt;
				dbg = (dbgInfo)rt->dbg->statements.ptr;
				while (n > 0) {
					int symOffs = 0;
					symn sym = dbg->decl;
					if (sym == NULL) {
						sym = mapsym(rt, dbg->start, 1);
					}
					if (sym != NULL) {
						symOffs = dbg->start - sym->offs;
					}
						fputfmt(dmpf, "%s:%u:[.%06x, .%06x): <%?.0T+%d> hits(%D), time(%D%?+D / %.3F%?+.3F ms)\n"
							, dbg->file, dbg->line, dbg->start, dbg->end, sym, symOffs
							, (int64_t)dbg->hits, (int64_t)dbg->funcTime, (int64_t)dbg->diffTime
							, dbg->funcTime / (double)CLOCKS_PER_SEC, dbg->diffTime / (double)CLOCKS_PER_SEC
						);
					dbg++;
					n--;
				}
			}
			fputfmt(dmpf, "// */\n");

			// show allocated memory chunks.
			fputfmt(dmpf, "\n>/*-- heap:\n");
			rtAlloc(rt, NULL, 0);
			fputfmt(dmpf, "// */\n");
		}
	}

	// close log file
	logfile(rt, NULL, 0);
	closeLibs();
	return result;
}

extern int vmTest();
extern int vmHelp();

int main(int argc, char* argv[]) {
	if (argc < 2) {
		usage(*argv);
		return 0;
	}
	if (argc == 2) {
		#if DEBUGGING > 0
		if (strcmp(argv[1], "-vmTest") == 0) {
			return vmTest();
		}
		if (strcmp(argv[1], "-vmHelp") == 0) {
			return vmHelp();
		}
		#endif
		if (strcmp(argv[1], "--help") == 0) {
			usage(*argv);
			return 0;
		}
	}
	//setbuf(stdout, NULL);
	//setbuf(stderr, NULL);
	return program(argc, argv);
}

static void printGlobals(FILE* out, state rt, int all) {
	symn var;
	for (var = rt->defs; var; var = var->next) {
		char* ofs = NULL;

		// exclude types
		if (var->kind != TYPE_ref)
			continue;

		// exclude functions
		if (var->call && !all)
			continue;

		// exclude null
		if (var->offs == 0)
			continue;

		if (var->file && var->line) {
			fputfmt(out, "%s:%u:%u: ", var->file, var->line, var->colp);
		}
		else {
			//~ fputfmt(out, "var: ");
		}

		if (var->stat) {
			// static variable.
			ofs = (char*)rt->_mem + var->offs;
		}
		else {
			// argument or local variable.
			// in case if: var != rt->defs.
			//~ ofs = (char*)rt->retv + rt->fun->prms->offs - var->offs;
			continue;
		}

		fputval(rt, out, var, (stkval*)ofs, 0, prSymQual |prType);
		fputc('\n', out);
	}
}

static int dbgConsole(state rt, int pu, void* ip, void* sp, size_t ss, vmError err, size_t fp) {
	/* commands
	 *   \0: break and read command
	 *   r: break on next breakpoint (continue)
	 *   n: break on next line (step over line)
	 *   i: break on next call (step into function)
	 *   o: break on next return (step out function)
	 *   a: break on next opcode (step over opcode)

	 *   p: print (print top of stack)
	 *   t: trace (print callstack)
	 */
	static char lastCommand = 'r';
	char buff[1024];

	int brk = 0;
	size_t i = vmOffset(rt, ip);
	symn fun = mapsym(rt, i, 0);
	dbgInfo dbg = mapDbgStatement(rt, i);

	if (err != noError) {
		// error executing opcode or libcall: abort execution
		brk = 1;
	}
	else if (i < rt->dbg->breakLt) {
		// scheduled break
		brk = 2;
	}
	else if (i > rt->dbg->breakGt) {
		// scheduled break
		brk = 3;
	}
	else if (dbg != NULL) {
		// check breakpoint hit
		if (dbg->bp && i == dbg->start) {
			brk = 4;
		}
	}

	// no need to break, continue execution.
	if (brk == 0) {
		return 0;
	}

	// print the error message in case of unhandled errors
	if (!rt->dbg->checked) {
		char *errorType = "Unknown error";
		switch (err) {
			case noError:
				errorType = NULL;
				break;

			case invalidIP:
				errorType = "Invalid instruction pointer";
				break;
			case invalidSP:
				errorType = "Invalid stack pointer";
				break;

			case invalidOpcode:	// illegalInstruction
				errorType = "Invalid instruction";
				break;

			case traceOverflow:
			case stackOverflow:
				errorType = "Stack Overflow";
				break;

			case divisionByZero:
				errorType = "Division by Zero";
				break;

			case libCallError:
				errorType = "External call error";
				//fputfmt(stdout, "External call returned an error [@%06x]: @%+T\n", fp, mapsym(rt, fp, 1));
				//errorType = NULL;
				break;

			case segmentationFault:
				errorType = "Illegal memory access";
				break;

			default:
				//Unhandled exception at 0x0140AA67 in cc.exe : 0xC00000FD : Stack overflow(parameters : 0x00000000, 0x00F52000).
				//Unhandled exception at 0x00F3AA67 in cc.exe : 0xC0000005 : Access violation reading location 0x00C90000.
				//Unhandled exception at 0x010D409C in cc.exe : 0xC0000005 : Access violation writing location 0x00000000.
				errorType = "Unknown error";
				break;
		}

		// print error type
		if (errorType != NULL && fun != NULL) {
			fputfmt(stdout, "%s in function: %+T <+%06x>\n", errorType, fun, i - fun->offs);
		}

		// print current opcode
		if (dbg != NULL && dbg->file != NULL && dbg->line > 0) {
			fputfmt(stdout, "%s:%u: ", dbg->file, dbg->line);
		}
		// print current opcode
		fputfmt(stdout, "exec: .%06x: %9.*A\n", i, i, ip);

		logTrace(rt, stdout, 1, -1, 256);
	}
	for ( ; ; ) {
		char* arg = NULL;
		char cmd = lastCommand;

		fputfmt(stdout, "(dbg[%?c])", cmd);
		if (fgets(buff, sizeof(buff), stdin) == NULL) {
			// Can not read from stdin
			fputfmt(stdout, "can not read from standard input, continue\n");
			cmd = 'r';
			return 0;
		}
		if ((arg = strchr(buff, '\n'))) {
			*arg = 0;		// remove new line char
		}

		if (*buff == 0) {}		// no command, use last one
		else if (buff[1] == 0) {// one char command
			arg = buff + 1;
			cmd = buff[0];
		}
		else if ((arg = parsecmd(buff, "step", " "))) {
			if (*arg == 0) {	// step == step over
				cmd = 'n';
			}
			else if (strcmp(arg, "over") == 0) {
				cmd = 'n';
			}
			else if (strcmp(arg, "out") == 0) {
				cmd = 'n';
			}
			else if (strcmp(arg, "in") == 0) {
				cmd = 'n';
			}
			else {
				cmd = -1;	// error
			}
		}
		else if ((arg = parsecmd(buff, "continue", " "))) {
			if (*arg != 0) {
				cmd = -1;	// error
			}
			else {
				cmd = 'N';
			}
		}

		else if ((arg = parsecmd(buff, "trace", " "))) {
			if (*arg != 0) {
				cmd = -1;	// error
			}
			else {
				cmd = 't';
			}
		}
		else if ((arg = parsecmd(buff, "print", " "))) {
			if (*arg != 0) {
				cmd = -1;	// error
			}
			else {
				cmd = 'p';
			}
		}
		else {
			cmd = -1;	// error
		}
		if (arg == NULL) {
			cmd = -1;
		}

		switch (cmd) {
			default:
				fputfmt(stdout, "invalid command '%s'\n", buff);
				break;

			case 0:
				break;

			case 'q' :		// abort
				return -1;
			case 'r' :		// resume
				if (rt->dbg && dbg) {
					rt->dbg->breakLt = rt->vm.ro;
					rt->dbg->breakGt = rt->vm.px + px_size;
				}
				lastCommand = 'r';
				return 0;
			case 'a' :		// step over opcode
				if (rt->dbg && dbg) {
					rt->dbg->breakLt = dbg->start;
					rt->dbg->breakGt = dbg->start;
				}
				lastCommand = 'a';
				return 0;
			case 'n' :		// step over line
				if (rt->dbg && dbg) {
					rt->dbg->breakLt = dbg->start;
					rt->dbg->breakGt = dbg->end;
				}
				lastCommand = 'n';
				return 0;

			// print
			case 'p' :
				if (*arg == 0) {
					// print top of stack
					if (ss > 0) {
						if (printvars != NULL) {
							fputfmt(stdout, "\tsp(%d/%d): ", 0, ss);
							fputval(rt, stdout, printvars, (stkval*)sp, 0, prType);
							fputfmt(stdout, "\n");
						}
					}
					else {
						for (i = 0; i < ss; i++) {
							stkval* v = (stkval*)&((long*)sp)[i];
							fputfmt(stdout, "\tsp(%d): {i32(%d), f32(%g), i64(%D), f64(%G)}\n", i, v->i4, v->f4, v->i8, v->f8);
						}
					}
					// vmTags(rt, (void*)sptr, slen, 0);
				}
				else {
					symn sym = ccFindSym(rt->cc, NULL, arg);
					fputfmt(stdout, "arg:%T", sym);
					if (sym && sym->kind == TYPE_ref && !sym->stat) {
						fputval(rt, stdout, sym, (stkval*)sp, 0, prType);
					}
				}
				break;

			// trace
			case 't' :
				logTrace(rt, stdout, 1, 0, 20);
				break;
		}
	}
	return 0;
	(void)pu;
	(void)fp;
}
