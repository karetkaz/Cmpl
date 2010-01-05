typedef struct mesh {
	material mtl;	// back, fore;
	signed maxvtx, vtxcnt;	// vertices
	signed maxtri, tricnt;	// triangles
	signed maxseg, segcnt;	// triangles
	union vector *pos;		// (x, y, z, 1) position
	union vector *nrm;		// (x, y, z, 0) normal
	union texcol *tex;		// (s, t, 0, 0) tetxure|color
	struct tri {			// triangle list
		// signed id;	// groupId
		signed i1;
		signed i2;
		signed i3;
	} *triptr;
	struct seg {			// line list
		// signed id;	// groupId & mtl
		signed p1;
		signed p2;
	} *segptr;
	signed hlplt;			// highlight
	int hasTex:1;// := tex != null
	int hasNrm:1;// := nrm != null
} *mesh;

int freeMesh(mesh msh) {
	msh->maxtri = msh->tricnt = 0;
	msh->maxvtx = msh->vtxcnt = 0;
	free(msh->pos);
	msh->pos = 0;
	if (msh->nrm) {
		free(msh->nrm);
		msh->nrm = 0;
	}
	if (msh->tex) {
		free(msh->tex);
		msh->tex = 0;
	}
	free(msh->triptr);
	msh->triptr = 0;
	return 0;
}
int initMesh(mesh msh, int n) {
	msh->maxtri = n <= 0 ? 16 : n; msh->tricnt = 0;
	msh->maxseg = n <= 0 ? 16 : n; msh->segcnt = 0;
	msh->maxvtx = n <= 0 ? 16 : n; msh->vtxcnt = 0;
	msh->triptr = (struct tri*)malloc(sizeof(struct tri) * msh->maxtri);
	msh->segptr = (struct seg*)malloc(sizeof(struct seg) * msh->maxseg);
	msh->pos = malloc(sizeof(union vector) * msh->maxvtx);
	msh->nrm = malloc(sizeof(union vector) * msh->maxvtx);
	msh->tex = malloc(sizeof(union texcol) * msh->maxvtx);
	if (!msh->triptr || !msh->pos || !msh->nrm || !msh->tex) {
		freeMesh(msh);
		return -1;
	}
	return 0;
}

int getvtx(mesh msh, int idx) {
	if (idx >= msh->maxvtx) {
		//~ msh->maxvtx = HIBIT(idx) * 2;
		while (idx >= msh->maxvtx) msh->maxvtx <<= 1;
		msh->pos = (vector)realloc(msh->pos, sizeof(union vector) * msh->maxvtx);
		msh->nrm = (vector)realloc(msh->nrm, sizeof(union vector) * msh->maxvtx);
		msh->tex = (texcol)realloc(msh->tex, sizeof(union texcol) * msh->maxvtx);
		if (!msh->pos || !msh->nrm || !msh->tex) return -1;
	}
	if (msh->vtxcnt <= idx) {
		msh->vtxcnt = idx + 1;
	}
	return idx;
}
/*int setvtx(mesh msh, int idx, vector pos, vector nrm, long tex) {
	if (getvtx(msh, idx) < 0) 
		return -1;
	if (pos) msh->pos[idx] = *pos;
	if (nrm) msh->pos[idx] = *nrm;
	msh->tex[idx].val = tex;
	/ *switch (what) {
		case 0: break;
		default:  debug("invalid to set '%c'", what); break;
		case 'p': vecldf(msh->pos + idx, x, y, z, 1); break;
		case 'n': vecldf(msh->nrm + idx, x, y, z, 0); break;
		case 't': {
			if (x < 0) x = -x; if (x > 1) x = 1;
			if (y < 0) y = -y; if (y > 1) y = 1;
			msh->tex[idx].s = x * 65535;
			msh->tex[idx].t = y * 65535;
		} break;
	}* /
	return idx;
}
int addvtx(mesh msh, vector pos, vector nrm, long tex) {
	if (setvtx(msh, msh->vtxcnt, pos, nrm, tex) < 0)
		return -1;
	return msh->vtxcnt++;
}*/
int setvtxD(mesh msh, int idx, int atr, double x, double y, double z) {
	union vector tmp[1];
	if (getvtx(msh, idx) < 0)
		return -1;
	switch (atr) {
		default: debug("???"); break;
		case 'P': case 'p': vecldf(msh->pos + idx, x, y, z, 1); break;
		case 'N': case 'n': vecnrm(msh->nrm + idx, vecldf(tmp, x, y, z, 0)); break;
	}
	return idx;
}

int setvtxDV(mesh msh, int idx, double pos[3], double nrm[3], double tex[2]) {
	if (getvtx(msh, idx) < 0)
		return -1;
	if (pos) vecldf(msh->pos + idx, pos[0], pos[1], pos[2], 1);
	if (nrm) vecldf(msh->nrm + idx, nrm[0], nrm[1], nrm[2], 0);
	if (tex) {
		msh->tex[idx].s = tex[0] * 65535.;
		msh->tex[idx].t = tex[1] * 65535.;
	}
	return idx;
}
int addvtxDV(mesh msh, double pos[3], double nrm[3], double tex[2]) {
	int idx = msh->vtxcnt;
	if (setvtxDV(msh, idx, pos, nrm, tex) < 0)
		return -1;
	return idx;
}
int setvtxFV(mesh msh, int idx, float pos[3], float nrm[3], float tex[2]) {
	if (getvtx(msh, idx) < 0)
		return -1;
	//~ if (pos) debug("setvtxFV(%f, %f, %f)", pos[0], pos[1], pos[2]);
	if (pos) vecldf(msh->pos + idx, pos[0], pos[1], pos[2], 1);
	if (nrm) vecldf(msh->nrm + idx, nrm[0], nrm[1], nrm[2], 0);
	if (tex) {
		msh->tex[idx].s = tex[0] * 65535.;
		msh->tex[idx].t = tex[1] * 65535.;
	}
	return idx;
}
int addvtxFV(mesh msh, float pos[3], float nrm[3], float tex[2]) {
	int idx = msh->vtxcnt;
	if (setvtxFV(msh, idx, pos, nrm, tex) < 0)
		return -1;
	return idx;
}

int addtri(mesh msh, int p1, int p2, int p3) {
	if (p1 >= msh->vtxcnt || p2 >= msh->vtxcnt || p3 >= msh->vtxcnt){
		debug("addTri(%d, %d, %d)", p1, p2, p3);
		return -1;
	}
	#define H 1e-10
	if (vecdst(msh->pos + p1, msh->pos + p2) < H) return 0;
	if (vecdst(msh->pos + p1, msh->pos + p3) < H) return 0;
	if (vecdst(msh->pos + p2, msh->pos + p3) < H) return 0;
	#undef H
	if (msh->tricnt >= msh->maxtri) {
		msh->triptr = (struct tri*)realloc(msh->triptr, sizeof(struct tri) * (msh->maxtri <<= 1));
		if (!msh->triptr) return -2;
	}
	msh->triptr[msh->tricnt].i1 = p1;
	msh->triptr[msh->tricnt].i2 = p2;
	msh->triptr[msh->tricnt].i3 = p3;
	return msh->tricnt++;
}
int addquad(mesh msh, int p1, int p2, int p3, int p4) {
	//~ int e;
	//~ if (e = addtri(msh, p1, p2, p3) < 0)
		//~ return e;
	//~ return addtri(msh, p3, p4, p1);
	addtri(msh, p1, p2, p3);
	addtri(msh, p3, p4, p1);
	return 0;
}
int addseg(mesh msh, int p1, int p2) {
	if (p1 >= msh->vtxcnt || p2 >= msh->vtxcnt) return -1;
	if (msh->segcnt >= msh->maxseg) {
		if (msh->maxseg == 0) msh->maxseg = 16;
		msh->segptr = (struct seg*)realloc(msh->segptr, sizeof(struct seg) * (msh->maxseg <<= 1));
		if (!msh->segptr) return -2;
	}
	msh->segptr[msh->segcnt].p1 = p1;
	msh->segptr[msh->segcnt].p2 = p2;
	return msh->segcnt++;
}

int vtxcmp(mesh msh, int i, int j, scalar tol) {
	scalar dif;
	dif = msh->pos[i].x - msh->pos[j].x;
	if (tol < (dif < 0 ? -dif : dif)) return 2;
	dif = msh->pos[i].y - msh->pos[j].y;
	if (tol < (dif < 0 ? -dif : dif)) return 2;
	dif = msh->pos[i].z - msh->pos[j].z;
	if (tol < (dif < 0 ? -dif : dif)) return 2;
	//~ return msh->tex[i].val != msh->tex[j].val;
	return !(msh->tex && msh->tex[i].val == msh->tex[j].val);
}

/*/{ param surf	// DONE: scripted
//{ dv3opr
inline double  dv3dot(double lhs[3], double rhs[3]) {
	return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
}
inline double  dv3len(double rhs[3]) {
	double len = dv3dot(rhs, rhs);
	if (len > 0) len = sqrt(len);
	return len;
}
inline double* dv3crs(double dst[3], double lhs[3], double rhs[3]) {
	dst[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
	dst[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
	dst[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
	return dst;
}
inline double* dv3nrm(double dst[3], double src[3]) {
	double len = sqrt(dv3dot(src, src));
	if (len > 0) len = 1. / len;
	dst[0] = src[0] * len;
	dst[1] = src[1] * len;
	dst[2] = src[2] * len;
	return dst;
}
//}

static const double H = 1e-10;
static const double MPI = 3.14159265358979323846;

double* evalN(double dst[3], double s, double t, double* evalP(double [3], double [3], double, double)) {
	double pt[3], ds[3], dt[3];
	evalP(pt, 0, s, t);
	evalP(ds, 0, s + H, t);
	evalP(dt, 0, s, t + H);
	ds[0] = (pt[0] - ds[0]) / H;
	ds[1] = (pt[1] - ds[1]) / H;
	ds[2] = (pt[2] - ds[2]) / H;
	dt[0] = (pt[0] - dt[0]) / H;
	dt[1] = (pt[1] - dt[1]) / H;
	dt[2] = (pt[2] - dt[2]) / H;
	return dv3nrm(dst, dv3crs(dst, ds, dt));
}
double* evalP_plane(double dst[3], double nrm[3], double s, double t) {
	const double s_min = -1.0, s_max = 1.0;
	const double t_min = -1.0, t_max = 1.0;

	double S = s * (s_max - s_min) + s_min;
	double T = t * (t_max - t_min) + t_min;

	dst[0] = S;
	dst[1] = T;
	dst[2] = 0;

	if (nrm != NULL) {
		nrm[0] = 0;
		nrm[1] = 0;
		nrm[2] = 1;
	}
	return dst;
}
double* evalP_sphere(double dst[3], double nrm[3], double s, double t) {
	const double s_min = 0, s_max = 2 * MPI;
	const double t_min = 0, t_max = MPI;
	//~ const double t_min = -(MPI / 2), t_max = MPI / 2;

	double S = s * (s_max - s_min) + s_min;
	double T = t * (t_max - t_min) + t_min;

	//~ x = a cos(theta) cos(phi)
	//~ y = b cos(theta) sin(phi)
	//~ z = c sin(phi)
	//~ where -pi <= phi <= phi
	//~ and 0 <= theta <= 2 pi
	dst[0] = sin(T) * cos(S);
	dst[1] = sin(T) * sin(S);
	dst[2] = cos(T);

	if (nrm != NULL) dv3nrm(nrm, dst);		// simple
		//~ evalN(nrm, s, t, evalP_sphere);

	return dst;
}
double* evalP_apple(double dst[3], double nrm[3], double s, double t) {
	const double s_min =  0.0, s_max = 2 * MPI;
	const double t_min = -MPI, t_max = 1 * MPI;

	double S = s * (s_max - s_min) + s_min;
	double T = t * (t_max - t_min) + t_min;
	dst[0] = cos(S) * (4 + 3.8 * cos(T));
	dst[1] = sin(S) * (4 + 3.8 * cos(T));
	dst[2] = (cos(T) + sin(T) - 1) * (1 + sin(T)) * log(1 - 3.14159265358979323846 * T / 10) + 7.5 * sin(T);
	if (nrm != NULL)
		evalN(nrm, s, t, evalP_apple);
	return dst;
}
double* evalP_tours(double dst[3], double nrm[3], double s, double t) {
	const double s_min =  0.0, s_max =  2 * MPI;
	const double t_min =  0.0, t_max =  2 * MPI;

	const double R = 2;
	const double r = 1;

	double S = s * (s_max - s_min) + s_min;
	double T = t * (t_max - t_min) + t_min;
	dst[0] = (R + r * sin(S)) * cos(T);
	dst[1] = (R + r * sin(S)) * sin(T);
	dst[2] = r * cos(S);

	if (nrm != NULL)
		evalN(nrm, s, t, evalP_tours);
	return dst;
}
double* evalP_cone(double dst[3], double nrm[3], double s, double t) {
	const double s_min = 0.0, s_max = 1;
	const double t_min = 0.0, t_max = 2*MPI;

	const double l = 1;

	double S = s * (s_max - s_min) + s_min;
	double T = t * (t_max - t_min) + t_min;

	dst[0] = S * sin(T);
	dst[1] = S * cos(T);
	dst[2] = l * S;

	if (nrm != NULL)
		evalN(nrm, s, t, evalP_cone);
	return dst;
}
double* evalP_001(double dst[3], double nrm[3], double s, double t) {

	const double s_min = -20.0, s_max = +20.0;
	const double t_min = -20.0, t_max = +20.0;

	double sst, HH = 10 / 20.;

	const double Rx =  20.0;
	const double Ry =  20.0;
	const double Rz =  20.0;

	double S = s * (s_max - s_min) + s_min;
	double T = t * (t_max - t_min) + t_min;
	if ((sst = sqrt(S*S+T*T)) == 0) sst = 1.;
	dst[0] = Rx * S / 20;
	dst[1] = Ry * T / 20;
	dst[2] = Rz * HH * sin(sst) / sst;

	if (nrm != NULL)
		evalN(nrm, s, t, evalP_001);

	return dst;
}
double* evalP_002(double dst[3], double nrm[3], double s, double t) {
	const double Rx =  1.0;
	const double Ry =  1.0;
	const double Rz =  1.0;
	const double s_min =  0.0, s_max =  2*3.14159265358979323846;
	const double t_min =  0.0, t_max =  2*3.14159265358979323846;
	double S = s * (s_max - s_min) + s_min;
	double T = t * (t_max - t_min) + t_min;
	dst[0] = Rx * cos(S) / (sqrt(2) + sin(T));
	dst[1] = Ry * sin(S) / (sqrt(2) + sin(T));
	dst[2] = -Rz / (sqrt(2) + cos(T));
	if (nrm != NULL)
		evalN(nrm, s, t, evalP_002);
	return dst;
}
double* evalP_003(double dst[3], double nrm[3], double s, double t) {
	dst[0] = s;
	dst[1] = t;
	dst[2] = (s-t)*(s-t);
	if (nrm != NULL)
		evalN(nrm, s, t, evalP_003);
	return dst;
}
double* evalP_004(double dst[3], double nrm[3], double s, double t) {
	const double a = .5;
	const double s_min = -MPI, s_max = +MPI;
	const double t_min = -MPI, t_max = +MPI;
	double u = s * (s_max - s_min) + s_min;
	double v = t * (t_max - t_min) + t_min;
	dst[0] = cos(u)*(a + sin(v)*cos(u/2) - sin(2*v)*sin(u/2)/2);
	dst[1] = sin(u)*(a + sin(v)*cos(u/2) - sin(2*v)*sin(u/2)/2);
	dst[2] = sin(u/2)*sin(v) + cos(u/2)*sin(2*v)/2;
	if (nrm != NULL)
		evalN(nrm, s, t, evalP_004);
	return dst;
}
double* evalP_005(double dst[3], double nrm[3], double s, double t) {
	const double c = 1.5;
	const double s_min = -MPI, s_max = +MPI;
	const double t_min = -MPI, t_max = +MPI;
	double u = s * (s_max - s_min) + s_min;
	double v = t * (t_max - t_min) + t_min;
	dst[0] = (c + cos(v)) * cos(u);
	dst[1] = (c + cos(v)) * sin(u);
	dst[2] = sin(v) + cos(v);
	if (nrm != NULL)
		evalN(nrm, s, t, evalP_005);
	return dst;
}
double* evalP_006(double dst[3], double nrm[3], double s, double t) {
	const double r1 = 0.5;
	const double r2 = 0.5;
	const double periodlength = 1.5;
	const double s_min = 0, s_max = 4*MPI;
	const double t_min = 0, t_max = 2*MPI;
	double u = s * (s_max - s_min) + s_min;
	double v = t * (t_max - t_min) + t_min;
	dst[0] = (1 + r1 * cos(v)) * cos(u);
	dst[1] = (1 + r1 * cos(v)) * sin(u);
	dst[2] = r2 * (sin(v) + periodlength * u / MPI);
	//~ dst[0] = (c + cos(v)) * cos(u);
	//~ dst[1] = (c + cos(v)) * sin(u);
	//~ dst[2] = sin(v) + cos(v);
	if (nrm != NULL)
		evalN(nrm, s, t, evalP_006);
	return dst;
}
double* evalP_007(double dst[3], double nrm[3], double s, double t) {
	const double s_min = -2*MPI, s_max = +2*MPI;
	const double t_min = -2*MPI, t_max = +2*MPI;
	//~ const double s_min = -MPI / 2, s_max = MPI / 2;
	//~ const double t_min = 0, t_max = MPI;

	double S = s * (s_max - s_min) + s_min;
	double T = t * (t_max - t_min) + t_min;

	dst[0] = S;
	dst[1] = T;
	dst[2] = 0;
	if (S && T) {
		dst[2] = cos(S) * sin(T);
	}

	if (nrm != NULL) {
		//~ nrm[0] = 0;
		//~ nrm[1] = 0;
		//~ nrm[2] = 1;
		evalN(nrm, s, t, evalP_007);
	}
	return dst;
}

int evalMesh(mesh msh, double* evalP(double [3], double [3], double s, double t), unsigned sdiv, unsigned tdiv, unsigned closed) {
	unsigned si, ti;
	static unsigned mp[256][256];
	double pos[3], s, ds = 1. / (sdiv - ((closed & 2) == 0));
	double nrm[3], t, dt = 1. / (tdiv - ((closed & 1) == 0));
	if (sdiv > 256 || tdiv > 256) return -1;
	msh->tricnt = msh->vtxcnt = 0;
	msh->hasTex = 1;
	msh->hasNrm = 1;
	for (t = ti = 0; ti < tdiv; ti++, t += dt) {
		for (s = si = 0; si < sdiv; si++, s += ds) {
			double tex[2]; tex[0] = s; tex[1] = t;
			evalP(pos, nrm, s, t);
			mp[ti][si] = addvtxDV(msh, pos, nrm, tex);
		}
	}
	for (ti = 0; ti < tdiv - 1; ti += 1) {
		for (si = 0; si < sdiv - 1; si += 1) {
			int v0 = mp[si+0][ti+0];		//	v0--v1
			int v3 = mp[si+1][ti+0];		//	| \  |
			int v2 = mp[si+1][ti+1];		//	|  \ |
			int v1 = mp[si+0][ti+1];		//	v3--v2
			addtri(msh, v0, v1, v2);
			addtri(msh, v0, v2, v3);
		}
		if (closed & 1) {			// closed s
			int v0 = mp[sdiv-1][ti+0];
			int v1 = mp[0][ti + 0];
			int v2 = mp[0][ti + 1];
			int v3 = mp[sdiv-1][ti+1];
			addtri(msh, v0, v1, v2);
			addtri(msh, v0, v2, v3);
		}
	}
	if (closed & 2) {				// closed t
		for (si = 0; si < sdiv - 1; si += 1) {
			int v0 = mp[si + 0][tdiv - 1];
			int v3 = mp[si + 0][0];
			int v1 = mp[si + 1][tdiv - 1];
			int v2 = mp[si + 1][0];
			addtri(msh, v0, v1, v2);
			addtri(msh, v0, v2, v3);
		}
		if (closed & 1) {			// closed st
			int v0 = mp[sdiv - 1][tdiv - 1];
			int v1 = mp[0][tdiv - 1];
			int v2 = mp[0][0];
			int v3 = mp[sdiv-1][0];
			addtri(msh, v0, v1, v2);
			addtri(msh, v0, v2, v3);
		}
	}
	return 0;
}
//}*/

void normMesh(mesh msh, scalar tol);

int prgDefCB(float prec) {return 0;}
float precent(int i, int n) {
	return 100. * i / n;
}

static char* readKVP(char *ptr, char *cmd, char *skp, char *sep) {	// key = value pair
	if (skp == NULL) skp = "";
	if (sep == NULL) sep = " \t\n\r";

	while (*cmd && *cmd == *ptr) cmd++, ptr++;
	while (strchr(sep, *ptr)) ptr++;

	while (*skp && *skp == *ptr) skp++, ptr++;
	while (strchr(sep, *ptr)) ptr++;

	return (*cmd || *skp) ? 0 : ptr;
}
static char* readCmd(char *ptr, char *cmd) {
	//~ return readKVP(ptr, cmd, " ", "");
	while (*cmd && *cmd == *ptr) cmd++, ptr++;
	if (!strchr(" \t\n\r", *ptr)) return 0;
	while (strchr(" \t", *ptr)) ptr += 1;
	return ptr;
}
static char* readNum(char *ptr, int *outVal) {
	int sgn = 1, val = 0;
	if (!strchr("+-0123456789",*ptr))
		return ptr;
	if (*ptr == '-' || *ptr == '+') {
		sgn = *ptr == '-' ? -1 : +1;
		ptr += 1;
	}
	while (*ptr >= '0' && *ptr <= '9') {
		val = val * 10 + (*ptr - '0');
		ptr += 1;
	}
	if (outVal) *outVal = sgn * val;
	return ptr;
}

static char* readInt(char *ptr, int *outVal) {
	int sgn = 1, val = 0;
	if (!strchr("+-0123456789",*ptr))
		return ptr;
	if (*ptr == '-' || *ptr == '+') {
		sgn = *ptr == '-' ? -1 : +1;
		ptr += 1;
	}
	while (*ptr >= '0' && *ptr <= '9') {
		val = val * 10 + (*ptr - '0');
		ptr += 1;
	}
	if (outVal) *outVal = sgn * val;
	return ptr;
}

static char* readFlt(char *str, double *outVal) {
	char *ptr = str;//, *dot = 0, *exp = 0;
	double val = 0;
	int sgn = 1, exp = 0;

	//~ ('+'|'-')?
	if (*ptr == '-' || *ptr == '+') {
		sgn = *ptr == '-' ? -1 : 1;
		ptr += 1;
	}

	//~ (0 | ([1-9][0-9]+))?
	if (*ptr == '0') {
		ptr++;
	}
	else while (*ptr >= '0' && *ptr <= '9') {
		val = val * 10 + (*ptr - '0');
		ptr++;
	}

	//~ ('.'[0-9]*)?
	if (*ptr == '.') {
		//~ double tmp = 0;
		ptr++;
		while (*ptr >= '0' && *ptr <= '9') {
			val = val * 10 + (*ptr - '0');
			exp -= 1;
			ptr++;
		}
	}

	//~ ([eE]([+-]?)[0-9]+)?
	if (*ptr == 'e' || *ptr == 'E') {
		int tmp = 0;
		ptr = readInt(ptr + 1, &tmp);
		exp += tmp;
	}// */

	if (outVal) *outVal = sgn * val * pow(10, exp);
	return ptr;
}

static char* readF32(char *str, float *outVal) {
	double f64;
	str = readFlt(str, &f64);
	if (outVal) *outVal = f64;
	return str;
}

struct growBuffer {
	char *ptr;
	int max;
	//~ int cnt;
	int esz;
};
static void* growBuff(struct growBuffer* buff, int idx) {
	int pos = idx * buff->esz;
	if (pos >= buff->max) {
		buff->max = pos << 1;
		//~ while (pos >= buff->max) buff->max <<= 1;
		//~ debug("realloc(%x, %d, %d):%d", buff->ptr, buff->esz, buff->max, idx);
		buff->ptr = realloc(buff->ptr, buff->max);
	}
	return buff->ptr ? buff->ptr + pos : NULL;
}
static void freeBuff(struct growBuffer* buff) {
	free(buff->ptr);
	buff->ptr = 0;
	buff->max = 0;
}
static void initBuff(struct growBuffer* buff, int initsize, int elemsize) {
	buff->ptr = 0;
	buff->max = initsize;
	buff->esz = elemsize;
	growBuff(buff, initsize);
}

static float freadf32(FILE *fin) {
	float result;
	fread(&result, sizeof(result), 1, fin);
	return result;
}
static char* freadstr(char buff[], int maxlen, FILE *fin) {
	char *ptr = buff;
	for ( ; ; ) {
		int chr = fgetc(fin);
		if (chr == -1) break;
		if (chr == 0) break;
		*ptr = chr;
		if (maxlen > 0)
			++ptr, --maxlen;
	}
	*ptr = 0;
	return buff;
}

static int read_obj(mesh msh, const char* file) {
	FILE *fin;

	char buff[65536], *ptr;
	int posi = 0, texi = 0;
	int nrmi = 0, line = 0;
	struct growBuffer nrmb;
	struct growBuffer texb;

	if (!(fin = fopen(file, "rt"))) return -1;

	initBuff(&nrmb, 64, 3 * sizeof(float));
	initBuff(&texb, 64, 2 * sizeof(float));

	for ( ; ; ) {
		line++;
		fgets(ptr = buff, sizeof(buff), fin);

		if (feof(fin)) break;
		if (*buff == '#') continue;				// Comment
		if (*buff == '\n') continue;			// Empty line

		// Grouping:
		if (readCmd(buff, "g")) continue;		// Group name
		if (readCmd(buff, "s")) continue;		// Smoothing group
		if (readCmd(buff, "o")) continue;		// Object name
		//~ if (readCmd(buff, "mg")) continue;	// Merging group

		// Display/render attributes:
		if (readCmd(buff, "usemtl")) continue;		// Material name
		if (readCmd(buff, "mtllib")) continue;		// Material library
		//~ if (readCmd(buff, "bevel")) continue;	// Bevel interpolation
		//~ if (readCmd(buff, "c_interp")) continue;	// Color interpolation
		//~ if (readCmd(buff, "d_interp")) continue;	// Dissolve interpolation
		//~ if (readCmd(buff, "lod")) continue;		// Level of detail
		//~ if (readCmd(buff, "shadow_obj")) continue;	// Shadow casting
		//~ if (readCmd(buff, "trace_obj")) continue;	// Ray tracing
		//~ if (readCmd(buff, "ctech")) continue;	// Curve approximation technique
		//~ if (readCmd(buff, "stech")) continue;	// Surface approximation technique

		// Vertex data:
		if ((ptr = readCmd(buff, "v"))) {		// Geometric vertices
			float vtx[3];
			sscanf(ptr, "%f%f%f", vtx + 0, vtx + 1, vtx + 2);
			//~ debug("%d/%d", posi, texi);
			setvtxFV(msh, posi, vtx, NULL, NULL);
			posi += 1;
			continue;
		}
		if ((ptr = readCmd(buff, "vt"))) {	// Texture vertices
			float *vtx = growBuff(&texb, texi);
			if (!vtx) {debug("memory"); break;}
			sscanf(ptr, "%f%f", vtx + 0, vtx + 1);
			if (vtx[0] < 0) vtx[0] = -vtx[0];
			if (vtx[1] < 0) vtx[1] = -vtx[1];
			if (vtx[0] > 1) vtx[0] = 0;
			if (vtx[1] > 1) vtx[1] = 0;
			if (vtx[0] < 0 || vtx[0] > 1 || vtx[1] < 0 || vtx[1] > 1) {
				fprintf(stderr, "%s:%d:%d: %s", file, line, ptr - buff, buff);
				break;
			}
			texi += 1;
			continue;
		}// */
		if ((ptr = readCmd(buff, "vn"))) {	// Vertex normals
			float *vtx = growBuff(&nrmb, nrmi);
			if (!vtx) {debug("memory"); break;}
			sscanf(ptr, "%f%f%f", vtx + 0, vtx + 1, vtx + 2);
			nrmi += 1;
			continue;
		}// */
		//~ if (readCmd(buff, "vp")) continue;		// Parameter space vertices

		// Elements:
		if ((ptr = readCmd(buff, "f"))) {			// Face
			int i, v[4];
			for (i = 0; *ptr != 0; i++) {
				int vn[4], vt[4];
				while (strchr(" \t", *ptr)) ptr++;		// skip white spaces
				if (*ptr == '\n') break;				// end of face
				if (i > 4) break;						// error

				v[i] = vn[i] = vt[i] = 0;
				if (!strchr("+-0123456789", *ptr)) {	// error
					break;
				}

				ptr = readNum(ptr, &v[i]);				// position index
				if (*ptr == '/') {						// texture index
					ptr += 1;
					if (strchr("+-0123456789", *ptr)) {
						ptr = readNum(ptr, &vt[i]);
					}
				}
				if (*ptr == '/') {						// normal index
					ptr += 1;
					if (strchr("+-0123456789", *ptr)) {
						ptr = readNum(ptr, &vn[i]);
						//~ break;
					}
				}

				if (v[i]) v[i] += v[i] < 0 ? posi : -1;
				if (vn[i]) vn[i] += vn[i] < 0 ? nrmi : -1;
				if (vt[i]) vt[i] += vt[i] < 0 ? texi : -1;

				if (v[i] > posi || v[i] < 0) break;
				if (vn[i] > nrmi || vn[i] < 0) break;
				//~ if (vt[i] > texi || vt[i] < 0) break;
				if (vt[i] > texi || vt[i] < 0) vt[i] = 0;

				if (texi) setvtxFV(msh, v[i], NULL, NULL, growBuff(&texb, vt[i]));
				if (nrmi) setvtxFV(msh, v[i], NULL, growBuff(&nrmb, vn[i]), NULL);
			}
			if (*ptr != '\n') break;
			//~ debug("alma1(%d, %d, %d) / %d", v[0], v[1], v[2], msh->vtxcnt);
			if (i == 3) if (addtri(msh, v[0], v[1], v[2]) < 0) break;
			if (i == 4) if (addquad(msh, v[0], v[1], v[2], v[3]) < 0) break;
			continue;
		}// */
		//~ if (readCmd(buff, "p")) continue;		// Point
		//~ if (readCmd(buff, "l")) continue;		// Line
		//~ if (readCmd(buff, "curv")) continue;		// Curve
		//~ if (readCmd(buff, "curv2")) continue;		// 2D curve
		//~ if (readCmd(buff, "surf")) continue;		// Surface

		// Connectivity between free-form surfaces:
		//~ if (readCmd(buff, "con")) continue;		// Connect

		// Free-form curve/surface attributes:
		//~ if (readCmd(buff, "deg")) continue;		// Degree
		//~ if (readCmd(buff, "bmat")) continue;	// Basis matrix
		//~ if (readCmd(buff, "step")) continue;	// Step size
		//~ if (readCmd(buff, "cstype")) continue;	// Curve or surface type

		// Free-form curve/surface body statements:
		//~ if (readCmd(buff, "parm")) continue;	// Parameter values
		//~ if (readCmd(buff, "trim")) continue;	// Outer trimming loop
		//~ if (readCmd(buff, "hole")) continue;	// Inner trimming loop
		//~ if (readCmd(buff, "scrv")) continue;	// Special curve
		//~ if (readCmd(buff, "sp")) continue;		// Special point
		//~ if (readCmd(buff, "end")) continue;		// End statement
		fprintf(stderr, "unsuported line: %s", buff);
		break;
	}

	freeBuff(&texb);
	freeBuff(&nrmb);

	if (!feof(fin)) {
		//~ fprintf(stderr, "%s:%d: %s", file, line, buff);
		//~ fprintf(stderr, "%s:%d:%d: %s", file, line, ptr - buff, buff);
		fprintf(stderr, "%s:%d: pos(%d), nrm(%d), tex(%d) %s", file, line, posi, nrmi, texi, buff);
	}

	if (nrmi == 0)
		normMesh(msh, 0);
	msh->hasNrm = 1;
	msh->hasTex = texi != 0;

	fclose(fin);
	return ptr != buff;
}
static int read_3ds(mesh msh, const char* file) {
	FILE *fin;
	char buff[65536];

	unsigned short chunk_id;
	unsigned int chunk_len;
	unsigned short qty;

	int i, vtxi = 0;
	int posi = 0, texi = 0;
	//~ int mtli = 0, obji = 0;				// material, mesh index

	if (!(fin = fopen (file, "rb"))) return -1;

	for ( ; ; ) {
		fread(&chunk_id, 2, 1, fin); //Read the chunk header
		fread(&chunk_len, 4, 1, fin); //Read the lenght of the chunk
		if (feof(fin)) break;
		switch (chunk_id) {
			//{ Color chunks
				//~ 0x0010 : Rgb (float)
				//~ 0x0011 : Rgb (byte)
				//~ 0x0012 : Rgb (byte) gamma corrected
				//~ 0x0013 : Rgb (float) gamma corrected
			//}
			//{ Percent chunks
				//~ 0x0030 : percent (int)
				//~ 0x0031 : percent (float)
			//}
			//{ 0x4D4D: Main chunk
				//~ 0x0002 : 3DS-Version
				//{ 0x3D3D : 3D editor chunk
					//~ 0x0100 : One unit
					//~ 0x1100 : Background bitmap
					//~ 0x1101 : Use background bitmap
					//~ 0x1200 : Background color
					//~ 0x1201 : Use background color
					//~ 0x1300 : Gradient colors
					//~ 0x1301 : Use gradient
					//~ 0x1400 : Shadow map bias
					//~ 0x1420 : Shadow map size
					//~ 0x1450 : Shadow map sample range
					//~ 0x1460 : Raytrace bias
					//~ 0x1470 : Raytrace on
					//~ 0x2100 : Ambient color
					//~ 0x2200 : Fog
					//~ 0x2201 : Use fog
					//~ 0x2210 : Fog background
					//~ 0x2300 : Distance queue
						//~ 0x2310 : Dim background
					//~ 0x2301 : Use distance queue
					//~ 0x2302 : Layered fog options
					//~ 0x2303 : Use layered fog
					//~ 0x3D3E : Mesh version
					//{ 0x4000 : Object block
						//~ 0x4010 : Object hidden
						//~ 0x4012 : Object doesn't cast
						//~ 0x4013 : Matte object
						//~ 0x4015 : External process on
						//~ 0x4017 : Object doesn't receive shadows
						//{ 0x4100 : Triangular mesh
							//~ 0x4110 : Vertices list
							//~ 0x4120 : Faces description
								//~ 0x4130 : Faces material list
							//~ 0x4140 : Mapping coordinates list
								//~ 0x4150 : Smoothing group list
							//~ 0x4160 : Local coordinate system
							//~ 0x4165 : Object color in editor
							//~ 0x4181 : External process name
							//~ 0x4182 : External process parameters
						//}
						//{ 0x4600 : Light
							//{ 0x4610 : Spotlight
								//~ 0x4627 : Spot raytrace
								//~ 0x4630 : Light shadowed
								//~ 0x4641 : Spot shadow map
								//~ 0x4650 : Spot show cone
								//~ 0x4651 : Spot is rectangular
								//~ 0x4652 : Spot overshoot
								//~ 0x4653 : Spot map
								//~ 0x4656 : Spot roll
								//~ 0x4658 : Spot ray trace bias
							//}
							//~ 0x4620 : Light off
							//~ 0x4625 : Attenuation on
							//~ 0x4659 : Range start
							//~ 0x465A : Range end
							//~ 0x465B : Multiplier
						//}
						//~ 0x4700 : Camera
					//}
					//{ 0x7001 : Window settings
						//~ 0x7011 : Window description #2 ...
						//~ 0x7012 : Window description #1 ...
						//~ 0x7020 : Mesh windows ...
					//}
					//{ 0xAFFF : Material block
						//~ 0xA000 : Material name
						//~ 0xA010 : Ambient color
						//~ 0xA020 : Diffuse color
						//~ 0xA030 : Specular color
						//~ 0xA040 : Shininess percent
						//~ 0xA041 : Shininess strength percent
						//~ 0xA050 : Transparency percent
						//~ 0xA052 : Transparency falloff percent
						//~ 0xA053 : Reflection blur percent
						//~ 0xA081 : 2 sided
						//~ 0xA083 : Add trans
						//~ 0xA084 : Self illum
						//~ 0xA085 : Wire frame on
						//~ 0xA087 : Wire thickness
						//~ 0xA088 : Face map
						//~ 0xA08A : In tranc
						//~ 0xA08C : Soften
						//~ 0xA08E : Wire in units
						//~ 0xA100 : Render type
						//~ 0xA240 : Transparency falloff percent present
						//~ 0xA250 : Reflection blur percent present
						//~ 0xA252 : Bump map present (true percent)
						//~ 0xA200 : Texture map 1
						//~ 0xA33A : Texture map 2
						//~ 0xA210 : Opacity map
						//~ 0xA230 : Bump map
						//~ 0xA33C : Shininess map
						//~ 0xA204 : Specular map
						//~ 0xA33D : Self illum. map
						//~ 0xA220 : Reflection map
						//~ 0xA33E : Mask for texture map 1
						//~ 0xA340 : Mask for texture map 2
						//~ 0xA342 : Mask for opacity map
						//~ 0xA344 : Mask for bump map
						//~ 0xA346 : Mask for shininess map
						//~ 0xA348 : Mask for specular map
						//~ 0xA34A : Mask for self illum. map
						//~ 0xA34C : Mask for reflection map
						//{ Sub-chunks for all maps:
							//~ 0xA300 : Mapping filename
							//~ 0xA351 : Mapping parameters
							//~ 0xA353 : Blur percent
							//~ 0xA354 : V scale
							//~ 0xA356 : U scale
							//~ 0xA358 : U offset
							//~ 0xA35A : V offset
							//~ 0xA35C : Rotation angle
							//~ 0xA360 : RGB Luma/Alpha tint 1
							//~ 0xA362 : RGB Luma/Alpha tint 2
							//~ 0xA364 : RGB tint R
							//~ 0xA366 : RGB tint G
							//~ 0xA368 : RGB tint B
						//}
					//}
				//}
				//{ 0xB000 : Keyframer chunk
					//~ 0xB001 : Ambient light information block
					//~ 0xB002 : Mesh information block
					//~ 0xB003 : Camera information block
					//~ 0xB004 : Camera target information block
					//~ 0xB005 : Omni light information block
					//~ 0xB006 : Spot light target information block
					//~ 0xB007 : Spot light information block
					//{ 0xB008 : Frames (Start and End)
						//~ 0xB010 : Object name, parameters and hierarchy father
						//~ 0xB013 : Object pivot point
						//~ 0xB015 : Object morph angle
						//~ 0xB020 : Position track
						//~ 0xB021 : Rotation track
						//~ 0xB022 : Scale track
						//~ 0xB023 : FOV track
						//~ 0xB024 : Roll track
						//~ 0xB025 : Color track
						//~ 0xB026 : Morph track
						//~ 0xB027 : Hotspot track
						//~ 0xB028 : Falloff track
						//~ 0xB029 : Hide track
						//~ 0xB030 : Hierarchy position
					//}
				//}
			//} Main chunk
			case 0x4d4d: break;//debug("Main chunk"); break;
				case 0x3d3d: break;//debug("3D editor chunk"); break;
					//~ case 0x0100: // One unit
					//~ case 0x1100: // Background bitmap
					//~ case 0x1101: // Use background bitmap
					//~ case 0x1200: // Background color
					//~ case 0x1201: // Use background color
					//~ case 0x1300: // Gradient colors
					//~ case 0x1301: // Use gradient
					//~ case 0x1400: // Shadow map bias
					//~ case 0x1420: // Shadow map size
					//~ case 0x1450: // Shadow map sample range
					//~ case 0x1460: // Raytrace bias
					//~ case 0x1470: // Raytrace on
					//~ case 0x2100: // Ambient color
					//~ case 0x2200: // Fog
					//~ case 0x2201: // Use fog
					//~ case 0x2210: // Fog background
					//~ case 0x2300: // Distance queue
					//~ case 0x2310: // Dim background
					//~ case 0x2301: // Use distance queue
					//~ case 0x2302: // Layered fog options
					//~ case 0x2303: // Use layered fog
					//~ case 0x3D3E: // Mesh version
						//~ goto skipchunk;// */

					case 0x0002: goto skipchunk;		// 3DS-Version
					case 0xAFFF: goto skipchunk;		// Material block

					case 0x4000: {		// Object block
						char *objName = freadstr(buff, sizeof(buff), fin);
						vtxi = posi;
						objName = objName;
						//~ debug("Object: '%s'", objName);
					} break;
					case 0x4100: break;	// Triangular mesh
					case 0x4110: {		// Vertices list
						float dat[3];
						fread(&qty, sizeof(qty), 1, fin);
						for (i = 0; i < qty; i++) {
							fread(dat, sizeof(float), 3, fin);
							if (setvtxFV(msh, vtxi + i, dat, NULL, NULL) < 0) return 1;
							//~ debug("pos[%d](%f, %f, %f)", vtxi + i, dat[0], dat[1], dat[2]);
						}
						posi += qty;
					} break;
					case 0x4120: {		// Faces description
							unsigned short dat[4];
							fread (&qty, sizeof(qty), 1, fin);
							for (i = 0; i < qty; i++) {
								fread(&dat, sizeof(unsigned short), 4, fin);
								addtri(msh, vtxi + dat[0], vtxi + dat[1], vtxi + dat[2]);
							}
							//~ debug("faces: %d, %d, %d", vtxi, posi, texi);
					} break;
					case 0x4130:		// Faces material list
						goto skipchunk;
					case 0x4140: {		// Mapping coordinates list
						float dat[3];
						fread (&qty, sizeof(qty), 1, fin);
						for (i = 0; i < qty; i++) {
							fread(dat, sizeof(float), 2, fin);
							dat[1] = 1 - dat[1];
							if (setvtxFV(msh, vtxi + i, NULL, NULL, dat) < 0) return 2;
						}
						texi += qty;
					} break;
					case 0x4150:		// Smoothing group list
					case 0x4160:		// Local coordinate system
					case 0x4165:		// Object color in editor
					case 0x4181:		// External process name
					case 0x4182:		// External process parameters
				case 0xB000:			// Keyframer chunk
					goto skipchunk;
			default:
				debug("unknown Chunk: ID: 0x%04x; Len: %d", chunk_id, chunk_len); 
			skipchunk:
				fseek(fin, chunk_len - 6, SEEK_CUR);
		}
	}

	fclose (fin);
	msh->hasNrm = 1;
	normMesh(msh, 0);
	msh->hasTex = texi != 0;

	return 0;
}
static int save_obj(mesh msh, const char* file) {
	FILE* out;
	unsigned i;

	if (!(out = fopen(file, "wt"))) {
		debug("fopen(%s, 'wt')", file);
		return -1;
	}

	for (i = 0; i < msh->vtxcnt; ++i) {
		vector pos = msh->pos + i;
		vector nrm = msh->nrm + i;
		scalar s = msh->tex[i].s / 65535.;
		scalar t = msh->tex[i].t / 65535.;
		if (msh->hasTex && msh->hasNrm)
			fprintf(out, "#vertex: %d\n", i);
		fprintf(out, "v  %f %f %f\n", pos->x, pos->y, pos->z);
		if (msh->hasTex) fprintf(out, "vt %f %f\n", s, -t);
		if (msh->hasNrm) fprintf(out, "vn %f %f %f\n", nrm->x, nrm->y, nrm->z);
	}

	for (i = 0; i < msh->tricnt; ++i) {
		int i1 = msh->triptr[i].i1 + 1;
		int i2 = msh->triptr[i].i2 + 1;
		int i3 = msh->triptr[i].i3 + 1;
		if (msh->hasTex && msh->hasNrm) fprintf(out, "f  %d/%d/%d %d/%d/%d %d/%d/%d\n", i1, i1, i1, i2, i2, i2, i3, i3, i3);
		else if (msh->hasNrm) fprintf(out, "f  %d//%d %d//%d %d//%d\n", i1, i1, i2, i2, i3, i3);
		else if (msh->hasTex) fprintf(out, "f  %d/%d %d/%d %d/%d\n", i1, i1, i2, i2, i3, i3);
		else fprintf(out, "f  %d %d %d\n", i1, i2, i3);
	}

	for (i = 0; i < msh->segcnt; ++i) {
		int i1 = msh->segptr[i].p1 + 1;
		int i2 = msh->segptr[i].p2 + 1;
		fprintf(out, "l %d %d\n", i1, i2);
	}

	fclose(out);

	return 0;
}

static char* fext(const char* name) {
	char *ext = "";
	char *ptr = (char*)name;
	if (ptr) while (*ptr) {
		if (*ptr == '.')
			ext = ptr + 1;
		ptr += 1;
	}
	return ext;
}

int readMesh(mesh msh, const char* file) {
	char *ext = file ? fext(file) : "";
	msh->hasTex = msh->hasNrm = 0;
	msh->tricnt = msh->vtxcnt = 0;

	if (stricmp(ext, "obj") == 0)
		return read_obj(msh, file);

	if (stricmp(ext, "3ds") == 0)
		return read_3ds(msh, file);

	debug("invalid extension");
	return -9;
}
int saveMesh(mesh msh, const char* file) {
	char *ext = file ? fext(file) : "";

	if (stricmp(ext, "obj") == 0)
		return save_obj(msh, file);

	debug("invalid extension");
	return -9;
}

void bboxMesh(mesh msh, vector min, vector max) {
	unsigned i;
	if (msh->vtxcnt == 0) {
		vecldf(min, 0, 0, 0, 0);
		vecldf(max, 0, 0, 0, 0);
		return;
	}

	*min = *max = msh->pos[0];

	for (i = 1; i < msh->vtxcnt; i += 1) {
		vecmax(max, max, &msh->pos[i]);
		vecmin(min, min, &msh->pos[i]);
	}
}

void centMesh(mesh msh, scalar size) {
	unsigned i;
	union vector min, max, use;

	bboxMesh(msh, &min, &max);

	vecsca(&use, vecadd(&use, &max, &min), 1./2);		// scale
	for (i = 0; i < msh->vtxcnt; i += 1)
		vecsub(&msh->pos[i], &msh->pos[i], &use);

	if (size == 0) return;

	vecsub(&use, &max, &min);		// resize
	if (use.x < use.y) use.x = use.y;
	if (use.x < use.z) use.x = use.z;
	size = 2 * size / use.x;

	for (i = 0; i < msh->vtxcnt; i += 1) {
		vecsca(&msh->pos[i], &msh->pos[i], size);
		msh->pos[i].w = 1;
	}
}
void normMesh(mesh msh, scalar tol) {
	union vector tmp;
	unsigned i, j;
	for (i = 0; i < msh->vtxcnt; i += 1)
		vecldf(&msh->nrm[i], 0, 0, 0, 1);

	for (i = 0; i < msh->tricnt; i += 1) {
		union vector nrm;
		int i1 = msh->triptr[i].i1;
		int i2 = msh->triptr[i].i2;
		int i3 = msh->triptr[i].i3;
		backface(&nrm, &msh->pos[i1], &msh->pos[i2], &msh->pos[i3]);
		vecadd(&msh->nrm[i1], &msh->nrm[i1], &nrm);
		vecadd(&msh->nrm[i2], &msh->nrm[i2], &nrm);
		vecadd(&msh->nrm[i3], &msh->nrm[i3], &nrm);
	}

	if (tol) for (i = 1; i < msh->vtxcnt; i += 1) {
		for (j = 0; j < i; j += 1) {
			if (vtxcmp(msh, i, j, tol) < 2) {
				vecadd(&tmp, &msh->nrm[j], &msh->nrm[i]);
				msh->nrm[i] = tmp;
				msh->nrm[j] = tmp;
				break;
			}
		}
	}// */

	for (i = 0; i < msh->vtxcnt; i += 1) {
		//~ vecsca(&msh->vtxptr[i].nrm, &msh->vtxptr[i].nrm, 1./msh->vtxptr[i].nrm.w);
		msh->nrm[i].w = 0;
		vecnrm(&msh->nrm[i], &msh->nrm[i]);
	}
}// */

/**	subdivide mesh
 *	TODO: not bezier curve => patch eval
**/
static void pntri(union vector res[10], mesh msh, int i1, int i2, int i3) {
	#define P1 (msh->pos + i1)
	#define P2 (msh->pos + i2)
	#define P3 (msh->pos + i3)
	#define N1 (msh->nrm + i1)
	#define N2 (msh->nrm + i2)
	#define N3 (msh->nrm + i3)
	union vector tmp[3];
	scalar w12 = vecdp3(vecsub(tmp, P2, P1), N1);
	scalar w13 = vecdp3(vecsub(tmp, P3, P1), N1);
	scalar w21 = vecdp3(vecsub(tmp, P1, P2), N2);
	scalar w23 = vecdp3(vecsub(tmp, P3, P2), N2);
	scalar w31 = vecdp3(vecsub(tmp, P1, P3), N3);
	scalar w32 = vecdp3(vecsub(tmp, P2, P3), N3);
	//~ vector b300 = veccpy(res + 0, P1);
	//~ vector b030 = veccpy(res + 1, P2);
	//~ vector b003 = veccpy(res + 2, P3);
	vector b210 = vecsca(res + 3, vecsub(tmp, vecadd(tmp, vecsca(tmp, P1, 2), P2), vecsca(tmp + 1, N1, w12)), 1./3);
	vector b120 = vecsca(res + 4, vecsub(tmp, vecadd(tmp, vecsca(tmp, P2, 2), P1), vecsca(tmp + 1, N2, w21)), 1./3);
	vector b021 = vecsca(res + 5, vecsub(tmp, vecadd(tmp, vecsca(tmp, P2, 2), P3), vecsca(tmp + 1, N2, w23)), 1./3);
	vector b012 = vecsca(res + 6, vecsub(tmp, vecadd(tmp, vecsca(tmp, P3, 2), P2), vecsca(tmp + 1, N3, w32)), 1./3);
	vector b102 = vecsca(res + 7, vecsub(tmp, vecadd(tmp, vecsca(tmp, P3, 2), P1), vecsca(tmp + 1, N3, w31)), 1./3);
	vector b201 = vecsca(res + 8, vecsub(tmp, vecadd(tmp, vecsca(tmp, P1, 2), P3), vecsca(tmp + 1, N1, w13)), 1./3);
	//~ vector E = vecsca(tmp + 1, vecadd(tmp, vecadd(tmp, vecadd(tmp, vecadd(tmp, vecadd(tmp, b210, b120), b021), b012), b102), b201), 1./6);
	//~ vector V = vecsca(tmp + 2, vecadd(tmp, vecadd(tmp, P1, P2), P3), 1./3);
	//~ vector b111 = vecsca(res + 9, vecadd(tmp, E, vecsub(tmp, E, V)), 1./2);
	#undef P1
	#undef P2
	#undef P3
	#undef N1
	#undef N2
	#undef N3
}

static vector bezexp(union vector res[4], vector p0, vector c0, vector c1, vector p1) {
	union vector tmp[5];
	p0 = veccpy(tmp + 1, p0);
	c0 = veccpy(tmp + 2, c0);
	p1 = veccpy(tmp + 3, p1);
	c1 = veccpy(tmp + 4, c1);

	veccpy(res + 0, p0);
	vecsca(res + 1, vecsub(tmp, c0, p0), 3);
	vecsub(res + 2, vecsca(tmp, vecsub(tmp, c1, c0), 3), res+1);
	vecsub(res + 3, vecsub(tmp, vecsub(tmp, p1, res+2), res+1), res+0);
	return res;
}
static vector bezevl(union vector res[1], union vector p[4], scalar t) {
	//~ p = ((((p3) * t + p2) * t + p1) * t + p0);
	vecadd(res, vecsca(res, p+3, t), p+2);
	vecadd(res, vecsca(res, res, t), p+1);
	vecadd(res, vecsca(res, res, t), p+0);
	return res;
}

// subdivide using bezier
int sdvtxBP(mesh msh, int a, int b, vector c1, vector c2) {
	union vector tmp[6], res[3];
	int r = getvtx(msh, msh->vtxcnt);
	vecnrm(msh->nrm + r, vecadd(tmp, msh->nrm + a, msh->nrm + b));
	bezevl(msh->pos + r, bezexp(tmp, msh->pos + a, c1, c2, msh->pos + b), .5);
	return r;
}
int sdvtx(mesh msh, int a, int b) {
	union vector tmp[1];
	int r = getvtx(msh, msh->vtxcnt);
	vecnrm(msh->nrm + r, vecadd(tmp, msh->nrm + a, msh->nrm + b));
	vecsca(msh->pos + r, vecadd(tmp, msh->pos + a, msh->pos + b), .5);
	return r;
}

void sdivMesh(mesh msh, int smooth) {
	unsigned i, n = msh->tricnt;
	if (smooth) for (i = 0; i < n; i++) {
		union vector tmp[1], pn[11];
		unsigned i1 = msh->triptr[i].i1;
		unsigned i2 = msh->triptr[i].i2;
		unsigned i3 = msh->triptr[i].i3;
		unsigned i4, i5, i6;
		pntri(pn, msh, i1, i2, i3);
		i4 = sdvtxBP(msh, i1, i2, pn + 3, pn + 4);
		i5 = sdvtxBP(msh, i2, i3, pn + 5, pn + 6);
		i6 = sdvtxBP(msh, i3, i1, pn + 7, pn + 8);

		msh->triptr[i].i2 = i4;
		msh->triptr[i].i3 = i6;
		addtri(msh, i4, i2, i5);
		addtri(msh, i5, i3, i6);
		addtri(msh, i4, i5, i6);
	}
	else for (i = 0; i < n; i++) {
		signed i1 = msh->triptr[i].i1;
		signed i2 = msh->triptr[i].i2;
		signed i3 = msh->triptr[i].i3;
		signed i4 = sdvtx(msh, i1, i2);
		signed i5 = sdvtx(msh, i2, i3);
		signed i6 = sdvtx(msh, i3, i1);

		msh->triptr[i].i2 = i4;
		msh->triptr[i].i3 = i6;
		addtri(msh, i4, i2, i5);
		addtri(msh, i5, i3, i6);
		addtri(msh, i4, i5, i6);
	}
}

void optiMesh(mesh msh, scalar tol, int prgCB(float prec)) {
	unsigned i, j, k, n;
	if (prgCB == NULL)
		prgCB = prgDefCB;
	prgCB(0);
	for (n = i = 1; i < msh->vtxcnt; i += 1) {
		if (prgCB(precent(i, msh->vtxcnt)) == -1) break;
		for (j = 0; j < n && vtxcmp(msh, i, j, tol) != 0; j += 1);

		if (j == n) {
			msh->pos[n] = msh->pos[i];
			msh->nrm[n] = msh->nrm[i];
			msh->tex[n] = msh->tex[i];
			n++;
		}
		//~ else vecadd(&msh->vtxptr[j].nrm, &msh->vtxptr[j].nrm, &msh->vtxptr[i].nrm);
		for (k = 0; k < msh->tricnt; k += 1) {
			if (msh->triptr[k].i1 == i) msh->triptr[k].i1 = j;
			if (msh->triptr[k].i2 == i) msh->triptr[k].i2 = j;
			if (msh->triptr[k].i3 == i) msh->triptr[k].i3 = j;
		}
	}
	msh->vtxcnt = n;
	if (prgCB(100) == -1) return;
	//~ printf("vtx cnt %d\n", msh->vtxcnt);

	//~ for (j = 0; j < n; j += 1) vecnrm(&msh->vtxptr[j].nrm, &msh->vtxptr[j].nrm);
}// */

/* Temp
static int sdvtxX(mesh msh, int a, int b) {
	union vector tmp[6];
	//~ int r = getvtx(msh, msh->vtxcnt);
	//~ scalar len = -1/veclen(vecsub(tmp, msh->pos + b, msh->pos + a));
	//~ scalar len = -2./vecdp3(msh->pos + b, msh->pos + a);
	vector p1 = msh->pos + b;
	vector p2 = msh->pos + a;
	vector T1 = veccrs(tmp + 4, msh->nrm + a, msh->nrm + b);
	vector c1 = vecadd(tmp + 1, p1, vecsca(tmp, T1, vecdp3(T1, vecsub(tmp, p2, p1)) / 3));
	vector T2 = veccrs(tmp + 5, msh->nrm + b, msh->nrm + a);
	vector c2 = vecadd(tmp + 2, p2, vecsca(tmp, T2, vecdp3(T2, vecsub(tmp, p1, p2)) / 3));
	//~ vector c1 = vecadd(tmp + 1, p1, vecsca(tmp, msh->nrm + a, len));
	//~ vector c2 = vecadd(tmp + 2, p2, vecsca(tmp, msh->nrm + b, len));

	//~ vecnrm(msh->nrm + r, vecadd(tmp + 1, msh->nrm + a, msh->nrm + b));		// nrm = normalize(n1 + n2);

	//~ veccrs(tmp + 2, msh->nrm + a, msh->nrm + b);
	//~ vecsub(tmp + 3, msh->pos + b, msh->pos + a);
	//~ vecsca(tmp + 4, tmp + 2, vecdp3(tmp + 2, tmp + 3));
	//~ vecadd(msh->pos + r, msh->pos + a, tmp + 4);
	//~ vecadd(tmp + 4, msh->pos + a, vecdp3())
	//~ vecsca(msh->pos + r, vecadd(tmp, msh->pos + a, msh->pos + b), .5);

	//~ bezevl(msh->pos + r, bezexp(tmp, p1, c1, c2, p2), .5);
	setvtxD(msh, a, 'n', -c1->x, -c1->y, -c1->z);
	setvtxD(msh, b, 'n', -c2->x, -c2->y, -c2->z);

	//~ msh->tex[r].s = (msh->tex[a].s + msh->tex[b].s) / 2;
	//~ msh->tex[r].t = (msh->tex[a].t + msh->tex[b].t) / 2;
	return 0;//r;
}

void sdivMesh(mesh msh, int smooth) {
	unsigned i, n = msh->tricnt;
	for (i = 0; i < n; i++) {
		unsigned i1 = msh->triptr[i].i1;
		unsigned i2 = msh->triptr[i].i2;
		unsigned i3 = msh->triptr[i].i3;
		unsigned i4 = sdvtx(msh, i1, i2);
		unsigned i5 = sdvtx(msh, i2, i3);
		unsigned i6 = sdvtx(msh, i3, i1);
		//~ msh->triptr[i].i2 = i4;
		//~ msh->triptr[i].i3 = i6;
		//~ addtri(msh, i4, i2, i5);
		//~ addtri(msh, i5, i3, i6);
		//~ addtri(msh, i4, i5, i6);
	}
}// */
/* Temp
void cnrmMesh(mesh *msh) {
	unsigned i;
	for (i = 0; i < msh->tricnt; i++) {
		vertex *v1 = &msh->vtxptr[msh->triptr[i].i1];
		vertex *v2 = &msh->vtxptr[msh->triptr[i].i2];
		vertex *v3 = &msh->vtxptr[msh->triptr[i].i3];
		backface(&v1->nrm, &v1->pos, &v2->pos, &v3->pos);
		v3->nrm = v2->nrm = v1->nrm;
	}
}

void sdivMesh_2(mesh *msh) {
	unsigned i4, i5, i6, i, n = msh->tricnt;
	for (i = 0; i < n; i++) {
		unsigned i1 = msh->triptr[i].i1;
		unsigned i2 = msh->triptr[i].i2;
		unsigned i3 = msh->triptr[i].i3;
		vertex *v1 = &msh->vtxptr[i1];
		vertex *v2 = &msh->vtxptr[i2];
		vertex *v3 = &msh->vtxptr[i3];
		double nrm[3][3], pos[3][3], tex[3][2], len[3];

		nrm[0][0] = (v1->nrm.x + v2->nrm.x) * .5;
		nrm[0][1] = (v1->nrm.y + v2->nrm.y) * .5;
		nrm[0][2] = (v1->nrm.z + v2->nrm.z) * .5;
		nrm[1][0] = (v2->nrm.x + v3->nrm.x) * .5;
		nrm[1][1] = (v2->nrm.y + v3->nrm.y) * .5;
		nrm[1][2] = (v2->nrm.z + v3->nrm.z) * .5;
		nrm[2][0] = (v3->nrm.x + v1->nrm.x) * .5;
		nrm[2][1] = (v3->nrm.y + v1->nrm.y) * .5;
		nrm[2][2] = (v3->nrm.z + v1->nrm.z) * .5;

		pos[0][0] = (v1->pos.x + v2->pos.x) * .5;
		pos[0][1] = (v1->pos.y + v2->pos.y) * .5;
		pos[0][2] = (v1->pos.z + v2->pos.z) * .5;
		pos[1][0] = (v2->pos.x + v3->pos.x) * .5;
		pos[1][1] = (v2->pos.y + v3->pos.y) * .5;
		pos[1][2] = (v2->pos.z + v3->pos.z) * .5;
		pos[2][0] = (v3->pos.x + v1->pos.x) * .5;
		pos[2][1] = (v3->pos.y + v1->pos.y) * .5;
		pos[2][2] = (v3->pos.z + v1->pos.z) * .5;

		tex[0][0] = (v1->tex.s + v2->tex.s) * (.5 / 65535);
		tex[0][1] = (v1->tex.t + v2->tex.t) * (.5 / 65535);
		tex[1][0] = (v2->tex.s + v3->tex.s) * (.5 / 65535);
		tex[1][1] = (v2->tex.t + v3->tex.t) * (.5 / 65535);
		tex[2][0] = (v3->tex.s + v1->tex.s) * (.5 / 65535);
		tex[2][1] = (v3->tex.t + v1->tex.t) * (.5 / 65535);

		len[0] = dv3dot(nrm[0], nrm[0]);
		if (len[0] != 0) len[0] = 1./ sqrt(len[0]);
		len[1] = dv3dot(nrm[1], nrm[1]);
		if (len[1] != 0) len[1] = 1./ sqrt(len[1]);
		len[2] = dv3dot(nrm[2], nrm[2]);
		if (len[2] != 0) len[2] = 1./ sqrt(len[2]);

		pos[0][0] += nrm[0][0] * len[0] - nrm[0][0];
		pos[0][1] += nrm[0][1] * len[0] - nrm[0][1];
		pos[0][2] += nrm[0][2] * len[0] - nrm[0][2];
		pos[1][0] += nrm[1][0] * len[1] - nrm[1][0];
		pos[1][1] += nrm[1][1] * len[1] - nrm[1][1];
		pos[1][2] += nrm[1][2] * len[1] - nrm[1][2];
		pos[2][0] += nrm[2][0] * len[2] - nrm[2][0];
		pos[2][1] += nrm[2][1] * len[2] - nrm[2][1];
		pos[2][2] += nrm[2][2] * len[2] - nrm[2][2];

		nrm[0][0] *= len[0];
		nrm[0][1] *= len[0];
		nrm[0][2] *= len[0];
		nrm[1][0] *= len[1];
		nrm[1][1] *= len[1];
		nrm[1][2] *= len[1];
		nrm[2][0] *= len[2];
		nrm[2][1] *= len[2];
		nrm[2][2] *= len[2];

		//~ len[0] = dv3dot(pos[0], pos[0]);
		//~ if (len[0] != 0) len[0] = sqrt(len[0]);
		//~ len[1] = dv3dot(pos[1], pos[1]);
		//~ if (len[1] != 0) len[1] = sqrt(len[1]);
		//~ len[2] = dv3dot(pos[2], pos[2]);
		//~ if (len[2] != 0) len[2] = sqrt(len[2]);

		//~ pos[0][0] *= len[0];
		//~ pos[0][1] *= len[0];
		//~ pos[0][2] *= len[0];
		//~ pos[1][0] *= len[1];
		//~ pos[1][1] *= len[1];
		//~ pos[1][2] *= len[1];
		//~ pos[2][0] *= len[2];
		//~ pos[2][1] *= len[2];
		//~ pos[2][2] *= len[2];


		i4 = addvtxdv(msh, pos[0], nrm[0], tex[0], 0);
		i5 = addvtxdv(msh, pos[1], nrm[1], tex[1], 0);
		i6 = addvtxdv(msh, pos[2], nrm[2], tex[2], 0);

		msh->triptr[i].i2 = i4;
		msh->triptr[i].i3 = i6;
		addtri(msh, i4, i2, i5);
		addtri(msh, i5, i3, i6);
		addtri(msh, i4, i5, i6);
	}
}

void sdivvtx2(vecptr pos, vecptr nrm, vecptr p1, vecptr p2, vecptr n1, vecptr n2) {
	vector tmp;
	vecsca(nrm, vecadd(&tmp, n1, n2), .5);		// nrm = (n1 + n2) / 2;
	vecsca(pos, vecadd(&tmp, p1, p2), .5);		// pos = (p1 + p2) / 2;
												// tex = (t1 + t2) / 2;
	
}

void sdivvtx(vertex *res, vertex *v1, vertex *v2) {
	vector tmp;//, p1, p2, p0, pc;
	res->tex.s = (v1->tex.s + v2->tex.s) / 2;
	res->tex.t = (v1->tex.t + v2->tex.t) / 2;
	vecsca(&res->pos, vecadd(&tmp, &v1->pos, &v2->pos), .5);		// pos = (p1 + p2) / 2;
	vecsca(&res->nrm, vecadd(&tmp, &v1->nrm, &v2->nrm), .5);		// nrm = (n1 + n2) / 2;
	//~ vecnrm(&res->nrm, vecadd(&tmp, &v1->nrm, &v2->nrm));		// nrm = (n1 + n2) / 2;

	//~ vecadd(&p1, &v1->pos, &v1->nrm);
	//~ vecadd(&p2, &v2->pos, &v2->nrm);
	//~ vecadd(&p0, &res->pos, &res->nrm);
	//~ vecsca(&pc, vecadd(&tmp, &p1, &p2), .5);
	//~ vecadd(&res->pos, &res->pos, vecsub(&tmp, &p0, &pc));

	//~ vecsub(&tmp, &v1->nrm, &v2->nrm);
	//~ vecadd(&res->pos, &res->pos, vecsca(&tmp, &res->nrm, vecdp3(&tmp, &tmp)/2));
}
*/

#include "../lib/libpvmc/pvmc.h"
typedef struct userData {
	double s, smin, smax;
	double t, tmin, tmax;
	int iSpos, isNrm;
	double pos[3], nrm[3];
} *userData;

static inline double lerp(double a, double b, double t) {
	return a + t * (b - a);
}

static void getST(state args) {
	userData d = args->data;
	int32t c = popi32(args);
	switch (c) {
		case   1: retf64(args, lerp(d->smin, d->smax, d->s)); break;
		case   2: retf64(args, lerp(d->tmin, d->tmax, d->t)); break;
		case 's': retf64(args, lerp(d->smin, d->smax, d->s)); break;
		case 't': retf64(args, lerp(d->tmin, d->tmax, d->t)); break;
		case 'u': retf64(args, lerp(d->smin, d->smax, d->s)); break;
		case 'v': retf64(args, lerp(d->tmin, d->tmax, d->t)); break;
		default : debug("getArg: invalid argument"); break;
	}
	//~ debug("getArg('%c', %f, %f)", c, min, max);
}
static void setPos(state args) {
	userData d = args->data;
	d->pos[0] = popf64(args);
	d->pos[1] = popf64(args);
	d->pos[2] = popf64(args);
	//~ d->pos = 1;
	//~ debug("setNrm(%g, %g, %g)", d->pos[0], d->pos[1], d->pos[2]);
}
static void setNrm(state args) {
	userData d = args->data;
	d->nrm[0] = popf64(args);
	d->nrm[1] = popf64(args);
	d->nrm[2] = popf64(args);
	d->isNrm = 1;
	//~ debug("setNrm(%f, %f, %f)", x, y, z);
}// */

static void f64abs(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, fabs(x));
}
static void f64sin(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, sin(x));
}
static void f64cos(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, cos(x));
}
static void f64log(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, log(x));
}
static void f64exp(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, exp(x));
}
static void f64pow(state s) {
	flt64t x = poparg(s, flt64t);
	flt64t y = poparg(s, flt64t);
	setret(flt64t, s, pow(x, y));
}
static void f64sqrt(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, sqrt(x));
}

extern int findflt(ccEnv s, char *name, double* res);
extern int findint(ccEnv s, char *name, int* res);
extern int lookup_nz(ccEnv s, char *name);

#define H 1e-10

inline double  dv3dot(double lhs[3], double rhs[3]) {
	return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
}
inline double* dv3cpy(double dst[3], double src[3]) {
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	return dst;
}
inline double* dv3sca(double dst[3], double src[3], double rhs) {
	dst[0] = src[0] * rhs;
	dst[1] = src[1] * rhs;
	dst[2] = src[2] * rhs;
	return dst;
}
inline double* dv3sub(double dst[3], double lhs[3], double rhs[3]) {
	dst[0] = lhs[0] - rhs[0];
	dst[1] = lhs[1] - rhs[1];
	dst[2] = lhs[2] - rhs[2];
	return dst;
}
inline double* dv3crs(double dst[3], double lhs[3], double rhs[3]) {
	dst[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
	dst[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
	dst[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
	return dst;
}
inline double* dv3nrm(double dst[3], double src[3]) {
	double len = dv3dot(src, src);
	if (len > 0) {
		//~ len = 1. / sqrt(len);
		//~ dst[0] = src[0] * len;
		//~ dst[1] = src[1] * len;
		//~ dst[2] = src[2] * len;
		len = sqrt(len);
		dst[0] = src[0] / len;
		dst[1] = src[1] / len;
		dst[2] = src[2] / len;
	}
	return dst;
}

int evalMesh(mesh msh, char *obj, int sdiv, int tdiv) {
	static char mem[64 << 10];		// 64K memory ???
	state env = gsInit(mem, sizeof(mem));
	struct userData ud;

	char *logf = "debug.out";

	int i, j;//, cs, ct;

	double s, t, ds, dt;	// 0 .. 1
	//~ double smin = 0, smax = 1;
	//~ double tmin = 0, tmax = 1;

	if (!ccInit(env)) {
		debug("Internal error\n", env->_cnt, env->cc);
		return -9;
	}
	if (logfile(env, logf) != 0) {
		debug("can not open file `%s`\n", logf);
		return -1;
	}

	installlibc(env, f64abs, "flt64 abs(flt64 x)");
	installlibc(env, f64sin, "flt64 sin(flt64 x)");
	installlibc(env, f64cos, "flt64 cos(flt64 x)");
	installlibc(env, f64log, "flt64 log(flt64 x)");
	installlibc(env, f64exp, "flt64 exp(flt64 x)");
	installlibc(env, f64sqrt, "flt64 sqrt(flt64 x)");
	installlibc(env, f64pow, "flt64 pow(flt64 x, flt64 y)");

	installlibc(env, getST, "flt64 st(int32 arg)");
	installlibc(env, setPos, "void setPos(flt64 x, flt64 y, flt64 z)");
	//~ installlibc(env, setNrm, "void setNrm(flt64 x, flt64 y, flt64 z)");

	if (!ccOpen(env, 0, obj)) {
		debug("can not open %s", obj);
		return -8;
	}
	if (compile(env, 0) != 0) {
		debug("compile failed");
		return env->errc;
	}
	if (gencode(env, 0) != 0) {
		debug("gencode failed");
		return env->errc;
	}
	logfile(env, NULL);	// close it
	dump(env, logf, dump_sym | 0x01, "\ntags:\n");
	dump(env, logf, dump_ast | 0x00, "\ncode:\n");
	dump(env, logf, dump_asm | 0x39, "\ndasm:\n");

	env->data = &ud;
	ud.smin = ud.tmin = 0;
	ud.smax = ud.tmax = 1;

	if (findint(env->cc, "division", &tdiv)) {
		sdiv = tdiv;
	}

	findflt(env->cc, "smin", &ud.smin);
	findflt(env->cc, "smax", &ud.smax);
	findint(env->cc, "sdiv", &sdiv);

	findflt(env->cc, "tmin", &ud.tmin);
	findflt(env->cc, "tmax", &ud.tmax);
	findint(env->cc, "tdiv", &tdiv);

	//~ cs = lookup_nz(env->cc, "closedS");
	//~ ct = lookup_nz(env->cc, "closedT");

	//~ debug("s(min:%lf, max:%lf, div:%d%s)", ud.smin, ud.smax, sdiv, /* cs ? ", closed" : */ "");
	//~ debug("t(min:%lf, max:%lf, div:%d%s)", ud.tmin, ud.tmax, tdiv, /* ct ? ", closed" : */ "");
	//~ vmInfo(env->vm);

	ds = 1. / (sdiv - 1);
	dt = 1. / (tdiv - 1);
	msh->hasTex = msh->hasNrm = 1;
	msh->tricnt = msh->vtxcnt = 0;
	for (t = 0, j = 0; j < tdiv; t += dt, ++j) {
		for (s = 0, i = 0; i < sdiv; s += ds, ++i) {
			double pos[3], nrm[3], tex[2];
			double ds[3], dt[3];
			tex[0] = t;
			tex[1] = s;

			ud.s = s; ud.t = t;
			if (exec(env->vm, 4096, NULL) != 0) {
				debug("error");
				return -4;
			}
			dv3cpy(pos, ud.pos);

			ud.s = s + H; ud.t = t;
			if (exec(env->vm, 4096, NULL) != 0) {
				debug("error");
				return -3;
			}
			dv3cpy(ds, ud.pos);

			ud.s = s; ud.t = t + H;
			if (exec(env->vm, 4096, NULL) != 0) {
				debug("error");
				return -2;
			}
			dv3cpy(dt, ud.pos);

			ds[0] = (pos[0] - ds[0]) / H;
			ds[1] = (pos[1] - ds[1]) / H;
			ds[2] = (pos[2] - ds[2]) / H;
			dt[0] = (pos[0] - dt[0]) / H;
			dt[1] = (pos[1] - dt[1]) / H;
			dt[2] = (pos[2] - dt[2]) / H;
			//~ dv3sca(ds, dv3sub(ds, pos, ds), 1. / H);
			//~ dv3sca(dt, dv3sub(dt, pos, dt), 1. / H);
			dv3nrm(nrm, dv3crs(nrm, ds, dt));

			//~ debug("setNrm(%lg, %lg, %lg)", nrm[0], nrm[1], nrm[2]);

			addvtxDV(msh, pos, nrm, tex);
		}
	}
	for (j = 0; j < tdiv - 1; ++j) {
		int l1 = j * sdiv;
		int l2 = l1 + sdiv;
		for (i = 0; i < sdiv - 1; ++i) {
			int v1 = l1 + i, v2 = v1 + 1;
			int v4 = l2 + i, v3 = v4 + 1;
			addquad(msh, v1, v2, v3, v4);
		}
	}
	return 0;
}// */

static inline void sphere(double dst[3], double nrm[3], double tex[2], double s, double t) {
	const double s_min = 0.0, s_max = 1 * M_PI;
	const double t_min = 0.0, t_max = 2 * M_PI;

	double S = lerp(s_min, s_max, s);
	double T = lerp(t_min, t_max, t);

	dst[0] = cos(T) * sin(S);
	dst[1] = sin(T) * sin(S);
	dst[2] = cos(S);

	dv3nrm(nrm, dst);

	tex[0] = s;
	tex[1] = t;
}

int evalSphere(mesh msh, int sdiv, int tdiv) {
	double s, t, ds, dt;	// 0 .. 1
	int i, j;

	ds = 1. / (sdiv - 1);
	dt = 1. / (tdiv - 1);
	msh->hasTex = msh->hasNrm = 1;
	msh->tricnt = msh->vtxcnt = 0;

	for (t = 0, j = 0; j < tdiv; t += dt, ++j) {
		for (s = 0, i = 0; i < sdiv; s += ds, ++i) {
			double pos[3], nrm[3], tex[2];
			sphere(pos, nrm, tex, s, t);
			addvtxDV(msh, pos, nrm, tex);
		}
	}
	for (j = 0; j < tdiv - 1; ++j) {
		int l1 = j * sdiv;
		int l2 = l1 + sdiv;
		for (i = 0; i < sdiv - 1; ++i) {
			int v1 = l1 + i, v2 = v1 + 1;
			int v4 = l2 + i, v3 = v4 + 1;
			addquad(msh, v1, v2, v3, v4);
		}
	}
	return 0;
}
// */
