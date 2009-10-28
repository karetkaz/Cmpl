#include "pvmc.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// default values
static const int wl = 9;		// warninig level
static const int ol = 0;		// optimize level

static const int cc = 1;			// execution cores
static const int ss = 1 << 10;		// execution stack(256K)
//~ static const int hs = 128 << 20;	// execution heap(128M)

void usage(state s, char* prog, char* cmd) {
	if (cmd == NULL) {
		fputfmt(stdout, "Usage: %s <command> [options] ...\n", prog);
		fputfmt(stdout, "<Commands>\n");
		fputfmt(stdout, "\t-c: compile\n");
		fputfmt(stdout, "\t-e: execute\n");
		fputfmt(stdout, "\t-d: diassemble\n");
		fputfmt(stdout, "\t-h: help\n");
		fputfmt(stdout, "\t=<expression>: eval\n");
		//~ fputfmt(stdout, "\t-d: dump\n");
	}
	else if (strcmp(cmd, "-c") == 0) {
		fputfmt(stdout, "compile: %s -c [options] files...\n", prog);
		fputfmt(stdout, "Options:\n");

		fputfmt(stdout, "\t[Output]\n");
		fputfmt(stdout, "\t-o <file> set file for output. [default=stdout]\n");
		fputfmt(stdout, "\t-t tags\n");
		fputfmt(stdout, "\t-s assembled code\n");

		fputfmt(stdout, "\t[Loging]\n");
		fputfmt(stdout, "\t-l <file> set file for errors. [default=stderr]\n");
		fputfmt(stdout, "\t-w<num> set warning level to <num> [default=%d]\n", wl);
		fputfmt(stdout, "\t-wa all warnings\n");
		fputfmt(stdout, "\t-wx treat warnings as errors\n");

		fputfmt(stdout, "\t[Debuging]\n");
		fputfmt(stdout, "\t-(ast|xml) output format\n");
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
	int i, k, n = 0;
	for (i = 0; i < opc_last; ++i) {
		char *opc = (char*)opc_tbl[i].name;
		if (opc && strfindstr(opc, cmd, 1)) {
			fputfmt(out, "Instruction: %s\n", opc);
			n += 1;
			k = i;
		}
	}
	if (n == 1 && strcmp(cmd, opc_tbl[k].name) == 0) {
		fputfmt(out, "Opcode: 0x%02x\n", opc_tbl[k].code);
		fputfmt(out, "Length: %d\n", opc_tbl[k].size);
		fputfmt(out, "Stack min: %d\n", opc_tbl[k].chck);
		fputfmt(out, "Stack diff: %d\n", opc_tbl[k].diff);
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

int evalexp(ccEnv s, char* text) {
	struct astn res;
	astn ast;
	symn typ;
	int tid;

	source(s, 0, text);
	ast = expr(s, 0);
	typ = lookup(s, 0, ast);
	tid = eval(&res, ast, TYPE_flt);

	if (peek(s))
		fputfmt(stdout, "unexpected: `%k`\n", peek(s));

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
	static struct state s[1];
	char *prg, *cmd, hexc = '#';
	dbgf dbg = nodbg;//Info;

	s->_cnt = sizeof(s->_mem);
	s->_ptr = s->_mem;
	s->cc = 0;
	s->vm = 0;

	prg = argv[0];
	cmd = argv[1];
	if (argc <= 2) {
		if (argc < 2) {
			usage(s, prg, NULL);
		}
		else if (*cmd != '-') {
			return evalexp(ccInit(s), cmd);
		}
		else if (strcmp(cmd, "-api") == 0) {
			dumpsym(stdout, leave(ccInit(s)), 1);
		}
		else if (strcmp(cmd, "-syms") == 0) {
			symn sym = leave(ccInit(s));
			while (sym) {
				dumpsym(stdout, sym, 0);
				sym = sym->defs;
			}
			//~ dumpsym(stdout, leave(s), 1);
		}
		else if (strcmp(cmd, "-emit") == 0) {
			ccInit(s);
			dumpsym(stdout, emit_opc->args, 1);
		}
		else usage(s, prg, cmd);
	}
	else if (strcmp(cmd, "-c") == 0) {	// compile
		int level = -1, argi = 2;
		int warn = wl;
		int outc = 0;			// output
		char *srcf = 0;			// source
		char *logf = 0;			// logger
		char *outf = 0;			// output
		enum {
			gen_code = 0x0010,
			out_tags = 0x0011,	// tags	// ?offs?
			out_tree = 0x0002,	// walk

			out_dasm = 0x0013,	// dasm
			run_code = 0x0014,	// exec
		};

		// options
		while (argi < argc) {
			char *arg = argv[argi];

			// source file
			if (*arg != '-') {
				if (srcf) {
					fputfmt(stderr, "multiple sources not suported\n");
					return -1;
				}
				srcf = arg;
			}

			// output file
			else if (strcmp(arg, "-l") == 0) {		// log
				if (++argi >= argc || logf) {
					fputfmt(stderr, "logger error\n");
					return -1;
				}
				logf = arg;
			}
			else if (strcmp(arg, "-o") == 0) {		// out
				if (++argi >= argc || outf) {
					fputfmt(stderr, "output error\n");
					return -1;
				}
				outf = arg;
			}

			// output text
			else if (strncmp(arg, "-x", 2) == 0) {		// exec
				char *str = arg + 2;
				outc = run_code;
				if (*str == 'd') {
					dbg = dbgInfo;
					str += 1;
				}
				else if (*str == 'D') {
					dbg = dbgCon;
					str += 1;
				}
				if (!parseInt(str, &level, 0)) {
					fputfmt(stderr, "invalid level '%s'\n", arg + 2);
					debug("invalid level '%s'\n", arg + 2);
					return 0;
				}
			}
			else if (strncmp(arg, "-t", 2) == 0) {		// tags
				if (!parseInt(arg + 2, &level, hexc)) {
					fputfmt(stderr, "invalid level '%s'\n", arg + 2);
					debug("invalid level '%s'\n", arg + 2);
					return 0;
				}
				outc = out_tags;
			}
			else if (strncmp(arg, "-s", 2) == 0) {		// dasm
				if (!parseInt(arg + 2, &level, hexc)) {
					fputfmt(stderr, "invalid level '%s'\n", arg + 2);
					debug("invalid level '%s'\n", arg + 2);
					return 0;
				}
				outc = out_dasm;
			}
			else if (strncmp(arg, "-c", 2) == 0) {		// tree
				if (!parseInt(arg + 2, &level, hexc)) {
					fputfmt(stderr, "invalid level '%s'\n", arg + 2);
					debug("invalid level '%s'\n", arg + 2);
					return 0;
				}
				outc = out_tree;
			}

			// Override settings
			else if (strncmp(arg, "-w", 2) == 0) {		// warning level
				if (strcmp(arg, "-wx"))
					warn = -1;
				else if (strcmp(arg, "-wa"))
					warn = 9;
				else if (!parseInt(arg + 2, &warn, 0)) {
					fputfmt(stderr, "invalid level '%s'\n", arg + 2);
					debug("invalid level '%s'\n", arg + 2);
					return 0;
				}
			}
			/*else if (strncmp(arg, "-d", 2) == 0) {		// optimize/debug level
				return -1;
			}*/

			else {
				fputfmt(stderr, "invalid option '%s' for -compile\n", arg);
				return -1;
			}
			++argi;
			//~ debug("level :0x%02x: arg[%d]: '%s'", level, argi - 2, arg);
		}
		if (logfile(s, logf) != 0) {
			fputfmt(stderr, "can not open file `%s`\n", srcf);
			return -1;
		}
		if (srcfile(s, srcf) != 0) {
			fputfmt(stderr, "can not open file `%s`\n", srcf);
			return -1;
		}
		if (compile(s, !srcUnit) != 0) {
			//~ fputfmt(stderr, "can not open file `%s`\n", srcf);
			return s->errc;
		}
		if (gencode(s, 0) != 0) {
			//~ fputfmt(stderr, "can not open file `%s`\n", srcf);
			return s->errc;
		}

		switch (outc) {
			case out_tree: dump(s, outf, dump_ast | (level & 0xff), NULL); break;
			case out_dasm: dump(s, outf, dump_asm | (level & 0xff), NULL); break;
			case out_tags: dump(s, outf, dump_sym | (1), NULL); break;
			case run_code: exec(s->vm, cc, ss, dbg); break;
		}

		return 0;
	}
	else if (strcmp(cmd, "-e") == 0) {	// execute
		fatal("unimplemented option '%s' \n", cmd);
		//~ objfile(s, ...);
		//~ return execute(s, cc, ss, dbgl);
	}
	else if (strcmp(cmd, "-d") == 0) {	// assemble
		fatal("unimplemented option '%s' \n", cmd);
		//~ objfile(s, ...);
		//~ return dumpasm(s, cc, ss, dbgl);
	}
	else if (strcmp(cmd, "-m") == 0) {	// make
		fatal("unimplemented option '%s' \n", cmd);
	}
	else if (strcmp(cmd, "-h") == 0) {	// help
		char *t = argv[2];
		if (argc < 3) {
			usage(s, argv[0], argc < 4 ? argv[3] : NULL);
		}
		else if (strcmp(t, "-s") == 0) {
			int i = 3;
			if (argc == 3) {
				vmHelp("?");
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
	else fputfmt(stderr, "invalid option '%s'", cmd);
	return 0;
}

int main(int argc, char *argv[]) {
	if (1 && argc == 1) {
		char *args[] = {
			"psvm",		// program name
			//~ "-emit",
			//~ "-api",
			"-c",		// compile command
			//~ "-c#0",		// execute command
			"-xd",		// execute command
			"../main.cvx",
		};
		argc = sizeof(args) / sizeof(*args);
		argv = args;
	}

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	return program(argc, argv);
}
