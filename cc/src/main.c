
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../util.c"
#include "ccvm.h"

// default values
static const int wl = 9;		// warning level
static const int ol = 2;		// optimize level

static const int cc = 1;			// execution cores
static const int ss = 32 << 10;		// execution stack(32K)
//~ static const int hs = 128 << 20;	// execution heap(128M)

extern int vmTest();

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
	static char mem[128 << 10];	// 128K
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
		else if (strcmp(cmd, "-help") == 0) {
			usage(prg, "-c");
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

		if (logf == NULL) {
			s->logf = stderr;
		}
		else if (logfile(s, logf) != 0) {
			fputfmt(stderr, "can not open file `%s`\n", srcf);
			return -1;
		}

		if (srcf == NULL) {
			fputfmt(stderr, "no input file\n");
			return -1;
		}
		else if (srcfile(s, srcf) != 0) {
			fputfmt(stderr, "can not open file `%s`\n", srcf);
			logfile(s, NULL);
			return -1;
		}
		debug("compiler.init:%.2F KBytes", (s->cc->_ptr - s->_mem) / 1024.);

		if (compile(s, warn) != 0) {
			logfile(s, NULL);
			return s->errc;
		}
		debug("compiler.scan:%.2F KBytes", (s->cc->_ptr - s->_mem) / 1024.);

		if (gencode(s, opti) != 0) {
			logfile(s, NULL);
			return s->errc;
		}
		debug("code:%d Bytes", s->vm->cs);
		debug("data:%d Bytes", s->vm->ds);

		if (outc) switch (outc) {
			default: fatal("FixMe!");
			case out_tree: dump(s, outf, dump_new | dump_ast | (level & 0xff), NULL); break;
			case out_dasm: dump(s, outf, dump_new | dump_asm | (level & 0xff), NULL); break;
			case out_tags: dump(s, outf, dump_new | dump_sym | (1), NULL); break;
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
	else if (argc == 2 && *cmd != '-') {	// try to eval
		return evalexp(ccInit(s), cmd);
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
			"../test.cvx",
		};
		argc = sizeof(args) / sizeof(*args);
		argv = args;
	}

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	return program(argc, argv);
	//~ return mk_test("xxxx.cvx", 8 << 20);	// 512 M
	//~ fatal("FixMe!");
	//~ return vmTest();
}
