#include "api.h"
#include <math.h>
#include <stdlib.h>

//~ #define debug(msg, ...) fprintf(stdout, "%s:%d:debug:%s: "msg"\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
//~ #define debug(msg, ...) fprintf(stdout, "%s:%d:debug: "msg"\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define debug(msg, ...) fprintf(stdout, msg"\n", ##__VA_ARGS__)

typedef struct userData {
	double s, t, S, T;
	int pos:1;
	double px, py, pz;
	int nrm:1;
	double nx, ny, nz;
	int col:1;
	double cr, cg, cb;

} *userData;
void setPos(state args) {
	userData d = args->data;
	d->px = popf32(args);
	d->py = popf32(args);
	d->pz = popf32(args);
	d->pos = 1;
	//~ debug("setNrm(%f, %f, %f)", x, y, z);
}
void setNrm(state args) {
	userData d = args->data;
	d->nx = popf32(args);
	d->ny = popf32(args);
	d->nz = popf32(args);
	d->nrm = 1;
	int len = d->nx*d->nx + d->ny*d->ny + d->nz*d->nz;
	if (len) {
		len = 1. / sqrt(len);
		d->nx *= len;
		d->ny *= len;
		d->nz *= len;
	}
	//~ debug("setNrm(%f, %f, %f)", x, y, z);
}
void setCol(state args) {
	userData d = args->data;
	d->cr = popf32(args);
	d->cg = popf32(args);
	d->cb = popf32(args);
	d->col = 1;
	//~ debug("setNrm(%f, %f, %f)", x, y, z);
}
void getArg(state args) {
	userData d = args->data;
	int32t c = popi32(args);
	flt64t min = popf64(args);
	flt64t max = popf64(args);
	switch (c) {
		case 's': retf32(args, d->S = d->s * (max - min) + min); break;
		case 't': retf32(args, d->T = d->t * (max - min) + min); break;
		default : debug("getArg: invalid argument"); break;
	}
	//~ debug("getArg('%c', %f, %f)", c, min, max);
}

extern int lookupflt(ccEnv s, char *name, double* res);
extern int lookupint(ccEnv s, char *name, int* res);
extern int lookup_nz(ccEnv s, char *name);

/*int mainq(int argc, char *argv[]) {
	char *srcf = "main.cvx";
	char *logf = 0;//"main.log";
	char *outf = "main.out";
	static struct state st[1];
	struct userData ud;
	int cs = 0, ct = 0;
	int i, j;

	double smin = 0, tmin = 0;
	double smax = 0, tmax = 0;
	int sdiv = 0, tdiv = 0;
	double s, t, ds, dt;

	int wtf = 0;
	FILE* out = stdout;

	st->_cnt = sizeof(st->_mem);
	st->_ptr = st->_mem;
	st->cc = 0;
	st->vm = 0;

	if (argc == 2) {
		srcf = argv[1];
		logf = NULL;
		outf = NULL;
	}

	if (!ccInit(st))
		return 1;

	install(s, TYPE_var, "const flt32 s", &ud.s);
	install(s, TYPE_var, "const flt32 t", &ud.t);

	install(s, TYPE_var, "flt32 x", &ud.x);
	install(s, TYPE_var, "flt32 y", &ud.y);
	install(s, TYPE_var, "flt32 z", &ud.z);

	if (logfile(st, logf) != 0) {
		debug("can not open file `%s`\n", srcf);
		return -1;
	}
	if (srcfile(st, srcf) != 0) {
		debug("can not open file `%s`\n", srcf);
		return -1;
	}
	if (compile(st, 0) != 0) {
		debug("compile failed");
		return st->errc;
	}
	if (gencode(st, 0) != 0) {
		debug("gencode failed");
		return st->errc;
	}

	st->data = &ud;

	if (lookup_nz(st->cc, "hasTex"))
		wtf |= 1;
	if (lookup_nz(st->cc, "hasNrm"))
		wtf |= 2;

	if (!(out = fopen("obj.obj", "w")))
		return -1;

	cs = lookup_nz(st->cc, "closeS");
	ct = lookup_nz(st->cc, "closeT");

	lookupflt(st->cc, "smin", &smin);
	lookupflt(st->cc, "smax", &smax);
	lookupint(st->cc, "sdiv", &sdiv);

	lookupflt(st->cc, "tmin", &tmin);
	lookupflt(st->cc, "tmax", &tmax);
	lookupint(st->cc, "tdiv", &tdiv);

	//~ exec(st->vm, 1, 4096, dbgInfo);
	vmInfo(st->vm);

	ds = (smax - smin) / (sdiv - cs);
	dt = (tmax - tmin) / (tdiv - ct);
	for (t = tmin, j = 0; j < tdiv; t += dt, ++j) {
		for (s = smin, i = 0; i < sdiv; s += ds, ++i) {
			//~ ud.x = ud.y = ud.z = 0. / 0;
			//~ ud.s = s; ud.t = t;

			if (exec(st->vm, 1, 4096, NULL) != 0) {
				debug("error");
				return -1;
			}

			if (!ud.pos) {
				debug("position not updated");
				return -1;
			}
			if (!ud.nrm && (wtf & 2)) {
				debug("normal not updated");
				return -1;
			}

			fprintf(out, "v %f %f %f\n", ud.px, ud.py, ud.pz);
			if (wtf & 1) fprintf(out, "vt %f %f\n", ud.s, ud.t);
			if (wtf & 2) fprintf(out, "vn %f %f %f\n", ud.nx, ud.ny, ud.nz);
		}
	}

	for (j = 0; j < tdiv - 1; ++j) {
		int l1 = j * tdiv + 1;
		int l2 = l1 + 1;
		for (i = 0; i < sdiv - 1; ++i) {
			int v1 = l1 + i;
			int v2 = v1 + 1;
			int v4 = l2 + 1;
			int v3 = v4 + 1;
			//~ int v1 = l1 + i;
			//~ int v2 = v1 + N;
			//~ int v3 = v2 + 1;
			//~ int v4 = v1 + 1;
			switch (wtf) {
				default: debug("invalid %d", wtf);
				case 0: fprintf(out, "f %d %d %d %d\n", v1, v2, v3, v4); break;
				case 1: fprintf(out, "f %d/%d %d/%d %d/%d %d/%d\n", v1, v1, v2, v2, v3, v3, v4, v4); break;
				case 2: fprintf(out, "f %d//%d %d//%d %d//%d %d//%d\n", v1, v1, v2, v2, v3, v3, v4, v4); break;
				case 3: fprintf(out, "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d\n", v1, v1, v1, v2, v2, v2, v3, v3, v3, v4, v4, v4);
			}
		}
	}

	if (out != stdout)
		fclose(out);

	return 0;
}// */
// */
int main(int argc, char *argv[]) {
	char *srcf = "main.cvx";
	char *logf = 0;//"obj3d.log";
	char *outf = 0;//"obj3d.out";
	static struct state s[1];
	struct userData ud;
	double delta;
	int i, j, n, N;
	int cs = 0, ct = 0;
	int wtf = 0;
	FILE* out = stdout;

	s->_cnt = sizeof(s->_mem);
	s->_ptr = s->_mem;
	s->cc = 0;
	s->vm = 0;

	if (argc == 2) {
		srcf = argv[1];
		logf = NULL;
		outf = NULL;
	}

	if (!ccInit(s))
		return 1;

	installlibc(s, getArg, "flt32 getArg(int32 arg, flt64 min, flt64 max)");
	installlibc(s, setPos, "void setPos(flt32 x, flt32 y, flt32 z)");
	installlibc(s, setNrm, "void setNrm(flt32 x, flt32 y, flt32 z)");

	//~ install(s, TYPE_var, "const flt32 s", &ud.s);
	//~ install(s, TYPE_var, "const flt32 t", &ud.t);

	//~ install(s, TYPE_var, "flt32 x", &ud.x);
	//~ install(s, TYPE_var, "flt32 y", &ud.y);
	//~ install(s, TYPE_var, "flt32 z", &ud.z);

	if (logfile(s, logf) != 0) {
		debug("can not open file `%s`\n", srcf);
		return -1;
	}
	if (srcfile(s, srcf) != 0) {
		srcf = "../obj3d.cvx";
		if (srcfile(s, srcf) != 0) {
			debug("can not open file `%s`\n", srcf);
			return -1;
		}
	}
	if (compile(s, 0) != 0) {
		debug("compile failed");
		return s->errc;
	}
	if (gencode(s, 0) != 0) {
		debug("gencode failed");
		return s->errc;
	}

	//~ dump(s, outf, dump_new | dump_sym | 0x01, "tags:\n");
	//~ dump(s, outf, dump_ast | 0x00, "\ncode (ast):\n");
	//~ dump(s, outf, dump_ast | 0x0f, "\ncode (xml):\n");
	//~ dump(s, outf, dump_asm | 0x39, "\ndasm:\n");

	s->data = &ud;
	if (!lookupint(s->cc, "division", &N)) {
		n = 32;
		debug("division: %d", n);
	}

	cs = lookup_nz(s->cc, "closeS");
	ct = lookup_nz(s->cc, "closeT");

	if (lookup_nz(s->cc, "hasTex"))
		wtf |= 1;

	if (lookup_nz(s->cc, "smaximum"))
		debug("alma");

	if (lookup_nz(s->cc, "hasNrm"))
		wtf |= 2;

	if (!(out = fopen("obj.obj", "w")))
		return -1;

	//~ exec(s->vm, 1, 4096, dbgInfo);
	vmInfo(s->vm);

	delta = 1. / (n = (N - 1));
	for (ud.t = j = 0; j <= n; ud.t += delta, ++j) {
		if (j == n && ct) ud.t = 0;
		for (ud.s = i = 0; i <= n; ud.s += delta, ++i) {
			if (i == n && cs) ud.s = 0;
			ud.pos = ud.nrm = 0;
			if (exec(s->vm, 1, 4096, NULL) != 0) {
				debug("error");
				return -1;
			}

			if (!ud.pos) {
				debug("position not updated");
				return -1;
			}
			if (!ud.nrm && (wtf & 2)) {
				debug("normal not updated");
				return -1;
			}

			fprintf(out, "v %f %f %f\n", ud.px, ud.py, ud.pz);
			if (wtf & 1) fprintf(out, "vt %f %f\n", ud.s, ud.t);
			if (wtf & 2) fprintf(out, "vn %f %f %f\n", ud.nx, ud.ny, ud.nz);
		}
	}

	for (j = 0; j < n; ++j) {
		int l1 = j * N + 1;
		for (i = 0; i < n; ++i) {
			int v1 = l1 + i;
			int v2 = v1 + 1;
			int v4 = v1 + N;
			int v3 = v4 + 1;
			//~ int v1 = l1 + i;
			//~ int v2 = v1 + N;
			//~ int v3 = v2 + 1;
			//~ int v4 = v1 + 1;
			switch (wtf) {
				default: debug("invalid %d", wtf);
				case 0: fprintf(out, "f %d %d %d %d\n", v1, v2, v3, v4); break;
				case 1: fprintf(out, "f %d/%d %d/%d %d/%d %d/%d\n", v1, v1, v2, v2, v3, v3, v4, v4); break;
				case 2: fprintf(out, "f %d//%d %d//%d %d//%d %d//%d\n", v1, v1, v2, v2, v3, v3, v4, v4); break;
				case 3: fprintf(out, "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d\n", v1, v1, v1, v2, v2, v2, v3, v3, v3, v4, v4, v4);
			}
		}
	}

	if (out != stdout)
		fclose(out);

	return 0;
}

int main2(int argc, char *argv[]) {
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
	}// */

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	return program(argc, argv);
}
