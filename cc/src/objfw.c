#include "pvm.h"

typedef struct userData {
	double s, t;
	int pos:1;
	double px, py, pz;
	int nrm:1;
	double nx, ny, nz;
	//~ int col:1;
	//~ double cr, cg, cb;

} *userData;
void setPos(libcarg args) {
	userData d = usrval(args);
	d->px = popf32(args);
	d->py = popf32(args);
	d->pz = popf32(args);
	d->pos = 1;
	//~ debug("setNrm(%f, %f, %f)", x, y, z);
}
void setNrm(libcarg args) {
	userData d = usrval(args);
	d->nx = popf32(args);
	d->ny = popf32(args);
	d->nz = popf32(args);
	d->nrm = 1;
	//~ debug("setNrm(%f, %f, %f)", x, y, z);
}
void getArg(libcarg args) {
	int32t c = popi32(args);
	flt32t min = popf64(args);
	flt32t max = popf64(args);
	userData d = usrval(args);
	switch (c) {
		case 's': retf32(args, d->s * (max - min) + min); break;
		case 't': retf32(args, d->t * (max - min) + min); break;
		//~ default : debug("getArg: invalid argument"); break;
	}
	//~ debug("getArg('%c', %f, %f)", c, min, max);
}

int main(int argc, char*argv[]) {
	//~ static struct state_t s[1];
	state cc = ccInit(8 << 20);
	vmEnv vm = vmInit(200);
	struct userData ud;
	double delta;
	char *file;

	installlibc(cc, getArg, "flt32 getArg(int32 arg, flt64 min, flt64 max)");
	installlibc(cc, setPos, "void setPos(flt32 x, flt32 y, flt32 z)");
	installlibc(cc, setNrm, "void setNrm(flt32 x, flt32 y, flt32 z)");

	file = argc == 2 ? argv[1] : "../main.cvx";

	if (srcfile(cc, file) != 0)
		return ccDone(cc);
	if (compile(cc, 0) != 0)
		return ccDone(cc);
	if (gencode(vm, cc) != 0)
		return ccDone(cc);

	//~ dumpsym(stdout, leave(s, NULL), 1);
	//~ dump(s, stdout, dump_sym | 0x01);
	//~ dump(s, stdout, dump_ast | 0x00);
	//~ dump(s, stdout, dump_asm | 0x39);

	if (!lookupflt(s, "delta", &delta)) {
		delta = 1. / 32;
	}

	for (ud.s = 0; ud.s < 1; ud.s += delta) {
		for (ud.t = 0; ud.t < 1; ud.t += delta) {
			ud.pos = ud.nrm = 0;
			if (exec(vm, 1, 4096, 0, &ud) == 0) {
				//~ debug("tex(%f, %f)", ud.s, ud.t);
				if (ud.pos) debug("pos(%f, %f, %f)", ud.px, ud.py, ud.pz);
				if (ud.nrm) debug("nrm(%f, %f, %f)", ud.nx, ud.ny, ud.nz);
				//~ debug("col(%f, %f, %f)", ud.cr, ud.cg, ud.cb);
			}
		}
	}

	return 0;
}


