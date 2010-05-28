#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "ccvm.h"

// default values
static const int wl = 9;		// warning level
static const int ol = 2;		// optimize level

static const int cc = 1;			// execution cores
#define ss (4 << 20)		// execution stack(32K)
#define hs (128 << 20)	// execution heap(128M)

extern int vmTest();

char *parsei32(const char *str, int32_t *res, int radix) {
	int64_t result = 0;
	int sign = 1;

	//~ ('+' | '-')?
	switch (*str) {
		case '-':
			sign = -1;
		case '+':
			str += 1;
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
	while (*str) {
		int value = *str;
		if (value >= '0' && value <= '9')
			value -= '0';
		else if (value >= 'a' && value <= 'z')
			value -= 'a' - 10;
		else if (value >= 'A' && value <= 'Z')
			value -= 'A' - 10;
		else break;

		if (value > radix)
			break;

		result *= radix;
		result += value;
		str += 1;
	}

	*res = sign * result;

	return (char *)str;
}
char *matchstr(const char *t, const char *p, int ic) {
	int i;//, ic = flgs & 1;

	for (i = 0; *t && p[i]; ++t, ++i) {
		if (p[i] == '*') {
			if (matchstr(t, p + i + 1, ic))
				return (char *)(t - i);
			return 0;
		}

		if (p[i] == '?' || p[i] == *t)		// skip | match
			continue;

		if (ic && p[i] == tolower(*t))		// ignore case
			continue;

		t -= i;
		i = -1;
	}

	while (p[i] == '*')			// "a***" is "a"
		++p;					// keep i for return

	return (char *)(p[i] ? 0 : t - i);
}
char* parsecmd(char *ptr, char *cmd, char *sws) {
	while (*cmd && *cmd == *ptr)
		cmd++, ptr++;

	if (*cmd != 0)
		return 0;

	if (*ptr == 0)
		return ptr;

	while (strchr(sws, *ptr))
		++ptr;

	return ptr;
}

void usage(char* prog, char* cmd) {
	if (cmd == NULL) {
		fputfmt(stdout, "Usage: %s <command> [options] ...\n", prog);
		fputfmt(stdout, "<Commands>\n");
		fputfmt(stdout, "\t-c: compile\n");
		fputfmt(stdout, "\t-e: execute\n");
		fputfmt(stdout, "\t-d: diassemble\n");
		fputfmt(stdout, "\t-h: help\n");
		//~ fputfmt(stdout, "\t=<expression>: eval\n");
		//~ fputfmt(stdout, "\t-d: dump\n");
	}
	else if (strcmp(cmd, "-c") == 0) {
		fputfmt(stdout, "compile: %s -c [options] files...\n", prog);
		fputfmt(stdout, "Options:\n");

		fputfmt(stdout, "\t[Output]\n");
		fputfmt(stdout, "\t-t Output is tags\n");
		fputfmt(stdout, "\t-c Output is ast code\n");
		fputfmt(stdout, "\t-s Output is asm code\n");
		fputfmt(stdout, "\telse output is binary code\n");
		fputfmt(stdout, "\t-o <file> Put output into <file>. [default=stdout]\n");

		fputfmt(stdout, "\t[Loging]\n");
		fputfmt(stdout, "\t-wa all warnings\n");
		fputfmt(stdout, "\t-wx treat warnings as errors\n");
		fputfmt(stdout, "\t-w<num> set warning level to <num> [default=%d]\n", wl);
		fputfmt(stdout, "\t-l <file> Put errors into <file>. [default=stderr]\n");

		fputfmt(stdout, "\t[Execute and Debug]\n");
		fputfmt(stdout, "\t-x<n> execute on <n> procs [default=%d]\n", cc);
		fputfmt(stdout, "\t-xd<n> debug on <n> procs [default=%d]\n", cc);

		//~ fputfmt(stdout, "\t[Debug & Optimization]\n");
	}
	else if (strcmp(cmd, "-e") == 0) {
		fputfmt(stdout, "command: '-e': execute\n");
	}
	else if (strcmp(cmd, "-d") == 0) {
		fputfmt(stdout, "command: '-d': disassemble\n");
	}
	else if (strcmp(cmd, "-m") == 0) {
		fputfmt(stdout, "command: '-m': make\n");
	}
	else if (strcmp(cmd, "-h") == 0) {
		fputfmt(stdout, "command: '-h': help\n");
	}
	else {
		fputfmt(stdout, "invalid help for: '%s'\n", cmd);
	}
}

int vmHelp(char *cmd) {
	FILE *out = stdout;
	int i, k = 0, n = 0;
	for (i = 0; i < opc_last; ++i) {
		char *opc = (char*)opc_tbl[i].name;
		if (opc && matchstr(opc, cmd, 1)) {
			fputfmt(out, "Instruction: %s\n", opc);
			n += 1;
			k = i;
		}
	}
	if (n == 1 && strcmp(cmd, opc_tbl[k].name) == 0) {
		fputfmt(out, "Opcode: 0x%02x\n", opc_tbl[k].code);
		fputfmt(out, "Length: %d\n", opc_tbl[k].size);
		fputfmt(out, "Stack check: %d\n", opc_tbl[k].chck);
		fputfmt(out, "Stack diff: %+d\n", opc_tbl[k].diff);
		//~ fputfmt(out, "0x%02x	%d		\n", opc_tbl[k].code, opc_tbl[k].size-1);
		//~ fputfmt(out, "\nDescription\n");
		//~ fputfmt(out, "The '%s' instruction %s\n", opc_tbl[k].name, "#");
		//~ fputfmt(out, "\nOperation\n");
		//~ fputfmt(out, "#\n");
		//~ fputfmt(out, "\nExceptions\n");
		//~ fputfmt(out, "None#\n");
		//~ fputfmt(out, "\n");		// end of text
	}
	else if (n == 0) {
		fputfmt(out, "No Entry for: '%s'\n", cmd);
	}
	return n;
}

int evalexp(ccEnv s, char* text/* , int opts */) {
	struct astn res;
	astn ast, tmp;
	symn typ;
	int tid;

	source(s, 0, text);

	ast = expr(s, 0);
	typ = typecheck(s, 0, ast);
	tid = eval(&res, ast);

	if (peek(s))
		error(s->s, 0, "unexpected: `%k`", peek(s));

	for (tmp = ast; tmp; tmp = tmp->next) {
		if (tmp != ast)
			fputc(' ', stdout);
		fputfmt(stdout, "%k", tmp);
	}
	fputc('\n', stdout);

	fputfmt(stdout, "eval(`%+k`) = ", ast);

	if (ast && typ && tid) {
		fputfmt(stdout, "%T(%k)\n", typ, &res);
		return 0;
	}

	fputfmt(stdout, "ERROR(typ:`%T`, tid:%d)\n", typ, tid);
	//~ dumpast(stdout, ast, 15);

	return -1;
}

int program(int argc, char *argv[]) {
	static char mem[hs];
	state s = gsInit(mem, sizeof(mem));

	char *prg, *cmd;
	dbgf dbg = NULL;//Info;

	prg = argv[0];
	cmd = argv[1];
	if (argc <= 2) {
		if (argc < 2) {
			usage(prg, NULL);
		}
		else if (*cmd == '=') {
			return evalexp(ccInit(s), cmd + 1);
		}
		else if (strcmp(cmd, "-api") == 0) {
			dumpsym(stdout, leave(ccInit(s), NULL), 1);
			ccDone(s);
		}
		else if (strcmp(cmd, "-syms") == 0) {
			symn sym = leave(ccInit(s), NULL);
			while (sym) {
				dumpsym(stdout, sym, 0);
				sym = sym->defs;
			}
			ccDone(s);
			//~ dumpsym(stdout, leave(s), 1);
		}
		else if (strcmp(cmd, "-emit") == 0) {
			ccInit(s);
			if (emit_opc)
				dumpsym(stdout, emit_opc->args, 1);
			ccDone(s);
		}
		else usage(prg, cmd);
	}
	else if (strcmp(cmd, "-c") == 0) {	// compile
		int level = -1, argi;
		int warn = wl;
		int opti = ol;
		int outc = 0;			// output
		char *srcf = 0;			// source
		char *logf = 0;			// logger
		char *outf = 0;			// output

		enum {
			//~ gen_code = 0x0010,
			out_tags = 0x0001,	// tags	// ?offs?
			out_tree = 0x0002,	// walk
			out_dasm = 0x0003,	// dasm
			run_code = 0x0004,	// exec
		};

		// options
		for (argi = 2; argi < argc; ++argi) {
			char *arg = argv[argi];

			// source file
			if (*arg != '-') {
				if (srcf) {
					fputfmt(stderr, "multiple source files are not suported\n");
					return -1;
				}
				srcf = argv[argi];
			}

			// output file
			else if (strcmp(arg, "-l") == 0) {		// log
				if (++argi >= argc || logf) {
					fputfmt(stderr, "logger error\n");
					return -1;
				}
				logf = argv[argi];
			}
			else if (strcmp(arg, "-o") == 0) {		// out
				if (++argi >= argc || outf) {
					fputfmt(stderr, "output error\n");
					return -1;
				}
				outf = argv[argi];
			}

			// output what
			else if (strncmp(arg, "-t", 2) == 0) {		// tags
				char *str = arg + 2;
				if (*parsei32(str, &level, 0)) {
					fputfmt(stderr, "invalid level '%s'\n", str);
					debug("invalid level '%s'", str);
					return 0;
				}
				outc = out_tags;
			}

			else if (strncmp(arg, "-c", 2) == 0) {		// ast
				if (*parsei32(arg + 2, &level, 16)) {
					fputfmt(stderr, "invalid argument '%s'\n", arg);
					return 0;
				}
				outc = out_tree;
			}
			else if (strncmp(arg, "-s", 2) == 0) {		// asm
				if (*parsei32(arg + 2, &level, 16)) {
					fputfmt(stderr, "invalid argument '%s'\n", arg);
					return 0;
				}
				outc = out_dasm;
			}

			else if (strncmp(arg, "-x", 2) == 0) {		// exec(&| debug)
				char *str = arg + 2;
				outc = run_code;

				if (*str == 'd' || *str == 'D') {
					dbg = *str == 'd' ? dbgInfo : dbgCon;
					str += 1;
				}

				level = 1;

				if (*str && *parsei32(str, &level, 16)) {
					fputfmt(stderr, "invalid level '%s'\n", str);
					debug("invalid level '%s'", str);
					return 0;
				}
			}

			// Override settings
			else if (strncmp(arg, "-w", 2) == 0) {		// warning level
				if (strcmp(arg, "-wx") == 0)
					warn = -1;
				else if (strcmp(arg, "-wa") == 0)
					warn = 9;
				else if (!parsei32(arg + 2, &warn, 10)) {
					fputfmt(stderr, "invalid level '%s'\n", arg + 2);
					debug("invalid level '%s'\n", arg + 2);
					return 0;
				}
			}
			else if (strncmp(arg, "-O", 2) == 0) {		// optimize level
				if (strcmp(arg, "-O") == 0)
					opti = 1;
				else if (strcmp(arg, "-Oa") == 0)
					opti = 3;
				else if (!parsei32(arg + 2, &opti, 10)) {
					fputfmt(stderr, "invalid level '%s'\n", arg + 2);
					debug("invalid level '%s'\n", arg + 2);
					return 0;
				}
			}

			else {
				fputfmt(stderr, "invalid option '%s' for -compile\n", arg);
				return -1;
			}

			//~ debug("level :0x%02x: arg[%d]: '%s'", level, argi - 2, arg);
		}

		if (logf && logfile(s, logf) != 0) {
			fputfmt(stderr, "can not open file `%s`\n", srcf);
			return -1;
		}

		if (srcfile(s, srcf) != 0) {
			fputfmt(stderr, "can not open file `%?s`\n", srcf);
			logfile(s, NULL);
			return -1;
		}
		//~ debug("compiler.init:%.2F KBytes", (s->cc->_ptr - s->_mem) / 1024.);

		if (compile(s, warn) != 0) {
			logfile(s, NULL);
			return s->errc;
		}
		//~ debug("compiler.scan:%.2F KBytes", (s->cc->_ptr - s->_mem) / 1024.);

		if (outc == out_tags || outc == out_tree) {
		}
		else if (gencode(s, opti) != 0) {
			logfile(s, NULL);
			return s->errc;
		}
		//~ debug("code:%d Bytes", s->vm->cs);
		//~ debug("data:%d Bytes", s->vm->ds);

		//~ logfile(s, outf);
		if (outc) switch (outc) {
			default: fatal("FixMe");
			case out_tags: dump2file(s, outf, dump_sym | (1), NULL); break;
			case out_dasm: dump2file(s, outf, dump_asm | (level & 0xff), NULL); break;
			case out_tree: dump2file(s, outf, dump_ast | (level & 0xff), NULL); break;
			case run_code: exec(s->vm, ss, dbg); break;
		}

		logfile(s, NULL);
		return 0;
	}
	else if (strcmp(cmd, "-e") == 0) {	// execute
		fatal("unimplemented");
		//~ objfile(s, ...);
		//~ return execute(s, cc, ss, dbgl);
	}
	else if (strcmp(cmd, "-d") == 0) {	// disasm
		fatal("unimplemented");
		//~ objfile(s, ...);
		//~ return dumpasm(s, cc, ss, dbgl);
	}
	else if (strcmp(cmd, "-m") == 0) {	// make
		fatal("unimplemented");
	}
	else if (strcmp(cmd, "-h") == 0) {	// help
		char *t = argv[2];
		if (argc < 3) {
			usage(argv[0], argc < 4 ? argv[3] : NULL);
		}
		else if (strcmp(t, "-s") == 0) {
			int i = 3;
			if (argc == 3) {
				vmHelp("*");
			}
			else while (i < argc) {
				vmHelp(argv[i++]);
			}
		}
		//~ else if (strcmp(t, "-x") == 0) ;
	}
	else fputfmt(stderr, "invalid option '%s'\n", cmd);
	return 0;
}

int mk_test(char *file, int size) {
	//~ char test[] = "signed _000000000000000001 = 8;\n";
	char test[] = "int _0000001=6;\n";
	FILE* f = fopen(file, "wb");
	int n, sp = sizeof(test) - 6;

	if (f == NULL) {
		debug("cann not open file");
		return -1;
	}

	if (size <= (128 << 20)) {
		for (n = 0; n < size; n += sizeof(test)-1) {
			fputs(test, f);
			while (++test[sp] > '9') {
				test[sp--] = '0';
				if (test[sp] == '_') {
					fclose(f);
					return -2;
				}
			}
			sp = sizeof(test) - 6;
		}
	}
	else debug("file is too large (128M max)");

	fclose(f);
	return 0;
}

int main(int argc, char *argv[]) {
	if (1 && argc == 1) {
		char *args[] = {
			"psvm",		// program name
			//~ "-h", "-s", "dup.x1", "set.x1",
			//~ "-api",
			"-c",		// compile command
			"-xd",		// execute & show symbols command
			"test.cvx",
		};
		argc = sizeof(args) / sizeof(*args);
		argv = args;
	}

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	//~ return mk_test("xxxx.cvx", 8 << 20);	// 8M
	//~ fatal("Fix!Me!");
	//~ return vmTest();
	return program(argc, argv);
}

int dbgInfo(state s, int pu, void *ip, long* sptr, int slen) {
	vmEnv vm = s->vm;
	if (ip == NULL) {
		if (s->cc)
			vmTags(s->cc, (char*)sptr, slen, 0);
		else {
			debug("!s->cc");
		}
		vmInfo(stdout, vm);
		return 0;
	}
	return 0;
}
int dbgCon(state s, int pu, void *ip, long* sptr, int slen) {
	static char buff[1024];
	static char cmd = 'N';
	vmEnv vm = s->vm;
	int IP, SP;
	char *arg;

	if (ip == NULL) {
		return dbgInfo(s, pu, ip, sptr, slen);
	}

	if (cmd == 'r') {	// && !breakpoint(vm, ip)
		return 0;
	}

	IP = ((char*)ip) - ((char*)vm->_mem);
	SP = ((char*)vm->_end) - ((char*)sptr);

	fputfmt(stdout, ">exec:pu%02d@.%04x:%04x[ss:%03d]x[0x%016X]: %A\n", pu, IP, SP, slen, *(int64_t*)sptr, ip);

	if (cmd != 'N') for ( ; ; ) {
		if (fgets(buff, 1024, stdin) == NULL) {
			//~ chr = 'r'; // dont try next time to read
			return 0;
		}

		if ((arg = strchr(buff, '\n'))) {
			*arg = 0;		// remove new line char
		}

		if (*buff == 0);		// no command, use last
		else if ((arg = parsecmd(buff, "print", " "))) {
			cmd = 'p';
		}
		else if ((arg = parsecmd(buff, "step", " "))) {
			if (*arg == 0)
				cmd = 'n';
			else if (strcmp(arg, "over") == 0) {
				cmd = 'n';
			}
			else if (strcmp(arg, "into") == 0) {
				cmd = 'n';
			}
		}
		else if ((arg = parsecmd(buff, "sp", " "))) {
			cmd = 's';
		}
		else if (buff[1] == 0) {
			arg = buff + 1;
			cmd = buff[0];
		}
		else {
			debug("invalid command %s", buff);
			arg = buff + 1;
			cmd = buff[0];
			buff[1] = 0;
			continue;
		}
		if (!arg) {
			debug("invalid command %s", buff);
			arg = NULL;
			//cmd = 0;
			continue;
		}

		switch (cmd) {
			default:
				debug("invalid command '%c'", cmd);
			case 0 :
				break;

			case 'r' :		// resume
			//~ case 'c' :		// step in
			//~ case 'C' :		// step out
			case 'n' :		// step over
				return 0;
			case 'p' : if (vm->s && vm->s->cc) {		// print
				if (!*arg) {
					vmTags(vm->s->cc, (void*)sptr, slen, 0);
				}
				else {
					symn sym = findsym(vm->s->cc, NULL, arg);
					debug("arg:%T", sym);
					if (sym && sym->kind == TYPE_ref && sym->offs < 0) {
						stkval* sp = (stkval*)(sptr + slen + sym->offs);
						void vm_fputval(FILE *fout, symn var, stkval* ref, int flgs);
						vm_fputval(stdout, sym, sp, 0);
					}
				}
			} break;
			case 's' : {
				int i;
				stkval* sp = (stkval*)sptr;
				if (*arg == 0) for (i = 0; i < slen; i++) {
					fputfmt(stdout, "\tsp(%d): {i32(%d), f32(%g), i64(%D), f64(%G)}\n", i, sp[i].i4, sp[i].f4, sp[i].i8, sp[i].f8);
				}
				else if (*parsei32(arg, &i, 10) == '\0') {
					if (i < slen)
						fputfmt(stdout, "\tsp(%d): {i32(%d), f32(%g), i64(%D), f64(%G)}\n", i, sp[i].i4, sp[i].f4, sp[i].i8, sp[i].f8);
					//~ fputfmt(stdout, "\tsp: {i32(%d), i64(%D), f32(%g), f64(%G), p4f(%g, %g, %g, %g), p2d(%G, %G)}\n", sp->i4, sp->i8, sp->f4, sp->f8, sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w, sp->pd.x, sp->pd.y);
				}
				else if (strcmp(arg, "i32") == 0)
					fputfmt(stdout, "\tsp: i32(%d)\n", sp->i4);
				else if (strcmp(arg, "f32") == 0)
					fputfmt(stdout, "\tsp: f32(%d)\n", sp->i8);
				else if (strcmp(arg, "i64") == 0)
					fputfmt(stdout, "\tsp: i64(%d)\n", sp->f4);
				else if (strcmp(arg, "f64") == 0)
					fputfmt(stdout, "\tsp: f64(%d)\n", sp->f8);
			} break;
		}
	}
	return 0;
}
