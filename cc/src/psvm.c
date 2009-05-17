#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include "main.h"

// default values
static const int wl = 9;		// warninig level
static const int ol = 1;		// optimize level

static const int dl = 2;		// execution level
static const int cc = 1;		// execution cores
static const int ss = 4 << 20;		// execution stack(4M)
//~ static const int hs = 32 << 20;		// execution heap(32M)

//~ #include "clog.c" // no printf
//~ #include "clex.c" // no printf
//~ #include "scan.c" // no printf
//~ #include "tree.c" // no printf
//~ #include "type.c" // no printf
//~ #include "code.c" // 

void usage(state s, char* name, char* cmd) {
	int n = 0;
	//~ char *pn = strrchr(name, '/');
	//~ if (pn) name = pn+1;
	if (strcmp(cmd, "-api") == 0) {
		defn api = 0;
		cc_init(s);
		leave(s,3, &api);
		dumpsym(stdout, api); 
		return ;
	}
	if (cmd == NULL) {
		puts("Usage : program command [options] ...\n");
		puts("where command can be one of:\n");
		puts("\t-c: compile\n");
		puts("\t-e: execute\n");
		puts("\t-m: make\n");
		puts("\t-h: help\n");
		//~ puts("\t-d: dump\n");
		n += 1;
	}
	if (!cmd || strcmp(cmd, "-c") == 0) {
		//~ puts("compile : %s -c [options] files...\n", name);
		puts("compile : psvm -c [options] files...\n");
		puts("Options :\n");

		puts("\t[Output to]\n");
		puts("\t-o <file> set file for output.\n");
		puts("\t-l <file> set file for errors.\n");

		puts("\t[Output is]\n");
		puts("\t-t tags\n");
		puts("\t-d code\n");
		puts("\t-a asm\n");

		puts("\t[Errors & Warnings]\n");
		//~ puts("\t-w<num> set warning level to <num> [default=%d]\n", wl);
		puts("\t-w<num> set warning level to <num>\n");
		puts("\t-wa all warnings\n");
		puts("\t-wx treat warnings as errors\n");

		//~ puts("\t[Debug & Optimization]\n");
		//~ puts("\t-d0 generate no debug information\n");
		//~ puts("\t-d1 generate debug {line numbers}\n");
		//~ puts("\t-d2 generate debug {-d1 + Symbols}\n");
		//~ puts("\t-d3 generate debug {-d2 + Symbols (all)}\n");
		//~ puts("\t-O<num> set optimization level. [default=%d]\n", dol);
		n += 1;
	}
	if (!cmd || strcmp(cmd, "-e") == 0) {
		puts("command : '-e' : execute\n");
		n += 1;
	}
	if (!cmd || strcmp(cmd, "-d") == 0) {
		puts("command : '-d' : disassemble\n");
		n += 1;
	}
	if (!cmd || strcmp(cmd, "-m") == 0) {
		puts("command : '-m' : make\n");
		n += 1;
	}
	if (!cmd || strcmp(cmd, "-h") == 0) {
		puts("command : '-h' : help\n");
		n += 1;
	}
	if (!n)
		puts("invalid command\n");
}

struct state_t env;

int main(int argc, char *argv[]) {
	state s = &env;
	if (argc < 3) usage(s, argv[0], argv[1]);
	else if (strcmp(argv[1], "=") == 0) {	// evaluate

		if (source(s, 1, argv[2])) {		// open source as text
			return -1;						// this never happens ???
		}

		ast = expr(s, 0);
		typ = cast(s, 0, ast);
		tid = eval(&res, ast, typ->kind);

		//~ debug("eval(`%+k`) = %T(%k)", ast, typ, &res, tok_tbl[tid].name);

		if (ast && typ && tid) {
			fputmsg(stdout, "`%+k` = %k\n", exp, &res);
			//~ fputmsg(stdout, "ERROR\n", exp, &res);
			//~ return -2;
		}
		return 0;

	}
	else if (strcmp(argv[1], "-c") == 0) {	// compile
		enum {
			out_bin = 0,	// vm_dump
			out_tag = 1,	// cc_tags
			out_pch = 2,	// cc_dump
			out_ast = 4,	// cc_walk
			out_asm = 5,	// vm_dasm
			run_bin = 8,	// vm_exec
		};
		int dbgl = 0, arg = 2;
		int warn = wl;
		//~ int optl = ol;
		int outc = 0;			// output type
		char* outf = 0;			// output file
		char* logf = 0;			// logger file

		// options
		for (arg = 2; arg < argc; arg += 1) {
			if (!argv[arg] || *argv[arg] != '-') break;

			// Override output file
			else if (strcmp(argv[arg], "-l") == 0) {		// log
				if (++arg >= argc || logf) {
					fputmsg(stderr, "logger error\n");
					return -1;
				}
				logf = argv[arg];
			}
			else if (strcmp(argv[arg], "-o") == 0) {		// out
				if (++arg >= argc || outf) {
					fputmsg(stderr, "output error\n");
					return -1;
				}
				outf = argv[arg];
			}

			// Override output type
			else if (strcmp(argv[arg], "-t") == 0) {		// tag
				outc = out_tag;
			}
			else if (strcmp(argv[arg], "-a") == 0) {		// asm
				outc = out_asm;
			}
			else if (strcmp(argv[arg], "-c") == 0) {		// ast
				outc = out_ast;
			}
			else if (strcmp(argv[arg], "-h") == 0) {		// pch
				outc = out_ast;
			}

			// Override settings
			else if (strncmp(argv[arg], "-x", 2) == 0) {		// execute
				outc = run_bin;
			}

			else if (strncmp(argv[arg], "-d", 2) == 0) {		// execute
				outc = run_bin;
				switch (argv[arg][2]) {
					case  0  : dbgl = 7; break;
					case '1' : dbgl = 1; break;
					case '2' : dbgl = 2; break;
					default  : 
						fputmsg(stderr, "invalid debug level '%c'\n", argv[arg][2]);
						dbgl = 2;
						break;
				}
			}
			else if (strncmp(argv[arg], "-w", 2) == 0) {		// warning level
				if (!argv[arg][3]) switch (argv[arg][2]) {
					case '0' : warn = 0; break;
					case '1' : warn = 1; break;
					case '2' : warn = 2; break;
					case '3' : warn = 3; break;
					case '4' : warn = 4; break;
					case '5' : warn = 5; break;
					case '6' : warn = 6; break;
					case '7' : warn = 7; break;
					case '8' : warn = 8; break;
					case '9' : warn = 9; break;
					case 'a' : warn = 9; break;		// all
					case 'x' : warn = -1; break;	// threat as errors
					default  :
						fputmsg(stderr, "invalid warning level '%c'\n", argv[arg][2]);
						break;
				}
				else fputmsg(stderr, "invalid warning level '%s'\n", argv[arg]);
			}
			else if (strncmp(argv[arg], "-d", 2) == 0) {		// optimize/debug level
				return -1;
			}
			else {
				fputmsg(stderr, "invalid option '%s' for -compile\n", argv[arg]);
				return -1;
			}
		}

		// compile
		cc_init(s);
		s->warn = warn;

		/*if (source(s, 0, "lstd.cxx") != 0) {
			fputmsg(stderr, "can not open standard library `%s`\n", argv[arg]);
			exit(-1);
		}
		scan(s, 0);
		*/

		logfile(s, logf);
		while (arg < argc) {
			node ast = 0;
			if (source(s, 0, argv[arg])) {
				fputmsg(stderr, "can not open file `%s`\n", argv[arg]);
			}
			ast = scan(s, 0);
			if (s->root) {
				fputmsg(stderr, "only one file suported by now\n", argv[arg]);
			}
			s->root = ast;
			arg += 1;
		}

		s->code.memp = (unsigned char*)s->buffp;
		s->code.mems = bufmaxlen - (s->buffp - s->buffer);
		s->code.ds = s->code.cs = s->code.pc = 0;
		s->code.rets = s->code.mins = 0;
		s->code.ic = 0;
		cgen(s, s->root, TYPE_any);

		// output
		switch (outc) {
			//~ case out_pch : dump(s, outf); break;
			//~ case out_bin : dumpbin(s, outf); break;
			//~ case out_tag : dumpsym(stdout, s->glob); break;
			//~ case out_ast : dumpast(stdout, s->root); break;
			//~ case out_asm : dumpasm(stdout, s); break;

			/*case run_bin : {		// exec
				int i = exec(s, cc, ss, dbgl);
				fputmsg(stdout, "exec inst: %d\n", s->code.ic);
				if (i == 0) vm_tags(s);
			} break;*/
		}
		return cc_done(s);
	}
	else if (strcmp(argv[1], "-e") == 0) {	// execute
		error(0, __FILE__, __LINE__, "unimplemented option '%s' \n", argv[1]);
		//~ vm_open(&env, ...);
		//~ vm_exec(&env, ...);
		//~ return vm_done(&env, ...);
	}
	else if (strcmp(argv[1], "-d") == 0) {	// disassemble
		error(0, __FILE__, __LINE__, "unimplemented option '%s' \n", argv[1]);
	}
	else if (strcmp(argv[1], "-m") == 0) {	// make
		error(0, __FILE__, __LINE__, "unimplemented option '%s' \n", argv[1]);
	}
	else if (strcmp(argv[1], "-h") == 0) {	// help
		usage(s, argv[0], argc < 4 ? argv[3] : NULL);
	}
	else error(0, __FILE__, __LINE__, "invalid option '%s' \n", argv[1]);
	return 0;
}

int dbg_ccct(state s, char *file, int line, char *text) {
	time_t sw = clock();
	if (source(s, 1, text)) return -1;
	s->file = file;
	s->line = line;
	s->root = scan(s, 0);
	//~ source(s, 0, NULL);
	return s->errc ? -1 : clock() - sw;
}

int dbg_cccf(state s, char *file) {
	time_t sw = clock();
	if (source(s, 0, file)) {
		return -1;
	}
	s->root = scan(s, 0);
	//~ source(s, 0, NULL);
	return s->errc ? -1 : clock() - sw;
}

int dbg_cgen(state s) {
	time_t sw = clock();
	if (s->errc) return -1;

	s->code.memp = (unsigned char*)s->buffp;
	s->code.mems = bufmaxlen - (s->buffp - s->buffer);

	s->code.ds = s->code.cs = s->code.pc = 0;
	s->code.rets = s->code.mins = 0;
	s->code.ic = 0;

	/*/ TODO emit data seg
	for (data = s->glob; data; data = data->next) {
		if (data->kind != TYPE_ref) continue;
		s->code.ds += data->size;
		// TODO initialize
	}// */

	// emit code seg
	if (!s->root)
		warn(s, 0, s->file, 1, "no data");
	cgen(s, s->root, TYPE_any);

	return s->errc ? -1 : clock() - sw;
}

int dbg_exec(state s, unsigned cc, unsigned ss, int dbg) {
	time_t sw = clock();
	if (s->errc) return -1;
	if (exec(s, cc, ss, dbg)) return -1;
	return s->errc ? -1 : clock() - sw;
}
