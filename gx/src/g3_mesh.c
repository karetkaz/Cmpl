#define debug(msg, ...) fprintf(stderr, "%s:%d:debug:"msg"\n", __FILE__, __LINE__, ##__VA_ARGS__)

static vertex* VtxCnt(mesh *msh, int cnt) {
	if (cnt >= msh->maxvtx) {
		while (cnt >= msh->maxvtx) msh->maxvtx <<= 1;
		msh->vtxptr = realloc(msh->vtxptr, sizeof(vertex) * msh->maxvtx);
		if (msh->vtxptr) memset(msh->vtxptr + cnt, 0, sizeof(vertex));
	}
	if (msh->vtxcnt <= cnt)
		msh->vtxcnt = cnt + 1;
	return msh->vtxptr;
}

int setvtx(mesh *msh, int idx, scalar x, scalar y, scalar z, char what) {
	vertex *vtx = VtxCnt(msh, idx);
	if (vtx) switch (what) {
		case 'p': vecldf(&vtx[idx].pos, x, y, z, 1); break;
		case 'n': vecldf(&vtx[idx].nrm, x, y, z, 0); break;
		case 't': {
			if (x < 0) x = -x; if (x > 1) x = 1;
			if (y < 0) y = -y; if (y > 1) y = 1;
			vtx[idx].tex.s = x * 65535;
			vtx[idx].tex.t = y * 65535;
		} break;
	}
	return idx;
}

int setvtxDV(mesh *msh, int idx, double val[3], char what) {
	return setvtx(msh, idx, val[0], val[1], val[2], what);
}

int addvtxDV(mesh *msh, double pos[3], double nrm[3], double tex[2]) {
	int idx = msh->vtxcnt;
	if (pos) setvtxDV(msh, idx, pos, 'p');
	if (nrm) setvtxDV(msh, idx, nrm, 'n');
	if (tex) setvtxDV(msh, idx, tex, 't');
	return idx;
}

int setvtxFV(mesh *msh, int idx, float val[3], char what) {
	return setvtx(msh, idx, val[0], val[1], val[2], what);
}

int addvtx(mesh *msh, vecptr pos, vecptr nrm, long tex) {
	int idx = msh->vtxcnt;
	vertex *vtx = VtxCnt(msh, idx);
	if (vtx) {
		vtx[idx].pos = *pos;
		if (nrm) vecnrm(&vtx[idx].nrm, nrm);
		else vecldf(&vtx[idx].nrm, 0, 0, 0, 0);
		vtx[idx].tex.val = tex;
	}
	return vtx ? idx : -1;
}// */

int addtri(mesh *msh, int p1, int p2, int p3) {
	if (p1 >= msh->vtxcnt || p2 >= msh->vtxcnt || p3 >= msh->vtxcnt) return -1;
	if (msh->tricnt >= msh->maxtri) {
		msh->triptr = (struct tri*)realloc(msh->triptr, sizeof(struct tri) * (msh->maxtri <<= 1));
		if (!msh->triptr) {
			freeMesh(msh);
			return -2;
		}
	}
	msh->triptr[msh->tricnt].i1 = p1;
	msh->triptr[msh->tricnt].i2 = p2;
	msh->triptr[msh->tricnt].i3 = p3;
	return msh->tricnt++;
}

int vtxcmp(mesh *msh, int i, int j, scalar tol) {
	vertex lhs = msh->vtxptr[i];
	vertex rhs = msh->vtxptr[j];
	scalar dif;
	dif = lhs.pos.x - rhs.pos.x;
	if (tol < (dif < 0 ? -dif : dif)) return 2;
	dif = lhs.pos.y - rhs.pos.y;
	if (tol < (dif < 0 ? -dif : dif)) return 2;
	dif = lhs.pos.z - rhs.pos.z;
	if (tol < (dif < 0 ? -dif : dif)) return 2;
	return lhs.tex.val != rhs.tex.val;
}

//{ param surf
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

static const double H = 1e-15;
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
	const double s_min = 0.0, s_max = 1 * MPI;
	const double t_min = 0.0, t_max = 2 * MPI;

	double S = s * (s_max - s_min) + s_min;
	double T = t * (t_max - t_min) + t_min;

	dst[0] = (cos(T) * sin(S));
	dst[1] = (sin(T) * sin(S));
	dst[2] = (cos(S));

	if (nrm != NULL) dv3nrm(nrm, dst);		// simple
		//evalN(nrm, s, t, evalP_sphere);

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

int evalMesh(mesh *msh, double* evalP(double [3], double [3], double s, double t), unsigned sdiv, unsigned tdiv, unsigned closed) {
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
			double tex[2]; tex[0] = t; tex[1] = s;
			evalP(pos, nrm, s, t);
			mp[ti][si] = addvtxDV(msh, pos, nrm, tex);
		}
	}
	for (ti = 0; ti < tdiv - 1; ti += 1) {
		for (si = 0; si < sdiv - 1; si += 1) {
			int v0 = mp[si+0][ti+0];		//	v0--v3
			int v1 = mp[si+0][ti+1];		//	| \  |
			int v2 = mp[si+1][ti+1];		//	|  \ |
			int v3 = mp[si+1][ti+0];		//	v1--v2
			addtri(msh, v0, v1, v2);
			addtri(msh, v0, v2, v3);
		}
		if (closed & 1) {			// closed s
			int v0 = mp[sdiv-1][ti+0];		//	v0--v3
			int v1 = mp[sdiv-1][ti+1];		//	| \  |
			int v2 = mp[0][ti + 1];			//	|  \ |
			int v3 = mp[0][ti + 0];			//	v1--v2
			addtri(msh, v0, v1, v2);
			addtri(msh, v0, v2, v3);
		}
	}
	if (closed & 2) {				// closed t
		for (si = 0; si < sdiv - 1; si += 1) {
			int v0 = mp[si + 0][tdiv - 1];		//	v0--v3
			int v1 = mp[si + 0][0];			//	| \  |
			int v3 = mp[si + 1][tdiv - 1];		//	|  \ |
			int v2 = mp[si + 1][0];			//	v1--v2
			addtri(msh, v0, v1, v2);
			addtri(msh, v0, v2, v3);
		}
		if (closed & 1) {			// closed s
			int v0 = mp[sdiv-1][tdiv - 1];		//	v0--v3
			int v1 = mp[sdiv-1][0];			//	| \  |
			int v3 = mp[0][tdiv - 1];		//	|  \ |
			int v2 = mp[0][0];			//	v1--v2
			addtri(msh, v0, v1, v2);
			addtri(msh, v0, v2, v3);
		}
	}// */
	return 0;
}
//}

void normMesh(mesh *msh, scalar tol);

int prgDefCB(float prec) {
	return 0;
}

float precent(int i, int n) {
	return 100. * i / n;
}

static char* readcmd(char *ptr, char *cmd, char *sep) {
	while (*cmd && *cmd == *ptr) cmd++, ptr++;
	while (strchr(sep, *ptr)) ptr++;
	return *cmd ? 0 : ptr;
}

static char* readCmd(char *ptr, char *name) {
	int len = strlen(name);
	if (strncmp(ptr, name, len) != 0) return 0;
	ptr += len;
	if (*ptr && !strchr(" \t\n\r", *ptr)) return 0;
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

static int read_obj_XXX(mesh* msh, const char* file) {
	FILE *f;
	char buff[65536], *ptr;
	//~ double pos[3], nrm[3], tex[2];
	//~ static double gid[65536][3];
	static double pos[65536][3];
	static double nrm[65536][3];
	static double tex[65536][2];
	int line = 0, posi = 0, nrmi = 0, texi = 0;//, idxi = 0;

	if (!(f = fopen(file, "r"))) return -1;
	msh->tricnt = msh->vtxcnt = 0;
	for ( ; ; ) {
		line++;
		fgets(buff, sizeof(buff), f);
		if (feof(f)) break;
		if (*buff == '#') continue;				// Comment
		if (*buff == '\n') continue;			// Empty line

		// Grouping:
		if (readCmd(buff, "g")) continue;		// Group name
		if (readCmd(buff, "s")) continue;		// Smoothing group
		if (readCmd(buff, "o")) continue;		// Object name
		//~ if (readCmd(buff, "mg")) continue;		// Merging group

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
			sscanf(ptr, "%lf%lf%lf", &pos[posi][0], &pos[posi][1], &pos[posi][2]);
			posi += 1;
			continue;
		}
		if ((ptr = readCmd(buff, "vt"))) {		// Texture vertices
			sscanf(ptr, "%lf%lf", &tex[texi][0], &tex[texi][1]);
			if (tex[texi][0] < 0) tex[texi][0] = -tex[texi][0];
			if (tex[texi][1] < 0) tex[texi][1] = -tex[texi][1];
			if (tex[texi][0] > 1) tex[texi][0] = 0;
			if (tex[texi][1] > 1) tex[texi][1] = 0;
			if (tex[texi][0] < 0 || tex[texi][1] < 0) {
				fprintf(stderr, "%s:%d:%d: %s", file, line, ptr - buff, buff);
				break;
			}
			if (tex[texi][0] > 1 || tex[texi][1] > 1) {
				fprintf(stderr, "%s:%d:%d: %s", file, line, ptr - buff, buff);
				break;
			}
			//~ if (tex[texi][0] < 0) tex[texi][0] = -tex[texi][0];
			//~ if (tex[texi][1] < 0) tex[texi][1] = -tex[texi][1];
			texi += 1;
			continue;
		}
		if ((ptr = readCmd(buff, "vn"))) {		// Vertex normals
			sscanf(ptr, "%lf%lf%lf", &nrm[nrmi][0], &nrm[nrmi][1], &nrm[nrmi][2]);
			nrmi += 1;
			continue;
		}
		//~ if (readCmd(buff, "vp")) continue;		// Parameter space vertices

		// Elements:
		if ((ptr = readCmd(buff, "f"))) {			// Face
			int i, vp[4], vn[4], vt[4], v[4];
			for (i = 0; *ptr != 0; i++) {
				while (strchr(" \t", *ptr)) ptr++;		// skip white spaces
				if (*ptr == '\n') break;				// end of face
				if (i > 4) break;						// error

				vp[i] = vn[i] = vt[i] = 0;
				if (!strchr("+-0123456789", *ptr)) {	// error
					break;
				}

				ptr = readNum(ptr, &vp[i]);				// position index
				if (*ptr == '/') {						// texture index
					ptr += 1;
					if (strchr("+-0123456789", *ptr)) {
						ptr = readNum(ptr, &vt[i]);
					}
				}
				if (*ptr == '/') {						// normal index
					ptr += 1;
					if (!strchr("+-0123456789", *ptr)) {
						break;
					}
					ptr = readNum(ptr, &vn[i]);
				}

				if (vp[i]) vp[i] += vp[i] < 0 ? posi : -1;
				if (vn[i]) vn[i] += vn[i] < 0 ? nrmi : -1;
				if (vt[i]) vt[i] += vt[i] < 0 ? texi : -1;

				if (vp[i] > posi || vn[i] > nrmi || vt[i] > texi) break;
				if (vp[i] < 0 || vn[i] < 0 || vt[i] < 0) break;
			}
			if (*ptr == '\n' && i == 3) {
				v[0] = addvtxDV(msh, pos[vp[0]], nrm[vn[0]], tex[vt[0]]);
				v[1] = addvtxDV(msh, pos[vp[1]], nrm[vn[1]], tex[vt[1]]);
				v[2] = addvtxDV(msh, pos[vp[2]], nrm[vn[2]], tex[vt[2]]);
				printf("addtri(%d, %d, %d)\n", v[0], v[1], v[2]);
				addtri(msh, v[0], v[1], v[2]);
				continue;
			}
			if (*ptr == '\n' && i == 4) {
				v[0] = addvtxDV(msh, pos[vp[0]], nrm[vn[0]], tex[vt[0]]);
				v[1] = addvtxDV(msh, pos[vp[1]], nrm[vn[1]], tex[vt[1]]);
				v[2] = addvtxDV(msh, pos[vp[2]], nrm[vn[2]], tex[vt[2]]);
				v[3] = addvtxDV(msh, pos[vp[3]], nrm[vn[3]], tex[vt[3]]);
				addtri(msh, v[0], v[1], v[2]);
				addtri(msh, v[0], v[2], v[3]);
				continue;
			}
		}
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

		fprintf(stderr, "%s:%d:%d: %s", file, line, ptr - buff, buff);
		fclose(f);
		return -4;
	}
	msh->hasNrm = 1;			// always true
	if (nrmi == 0) {			// no normals
		int i;
		for (i = 0; i < msh->tricnt; i++) {
			vertex *v1 = &msh->vtxptr[msh->triptr[i].i1];
			vertex *v2 = &msh->vtxptr[msh->triptr[i].i2];
			vertex *v3 = &msh->vtxptr[msh->triptr[i].i3];
			backface(&v1->nrm, &v1->pos, &v2->pos, &v3->pos);
			v3->nrm = v2->nrm = v1->nrm;
		}
	}
	msh->hasTex = 1;
	if (texi == 0) {			// no texture -> normals
		int i;
		msh->hasTex = 0;
		for (i = 0; i < msh->tricnt; i++) {
			vertex *v1 = &msh->vtxptr[msh->triptr[i].i1];
			vertex *v2 = &msh->vtxptr[msh->triptr[i].i2];
			vertex *v3 = &msh->vtxptr[msh->triptr[i].i3];
			v1->tex.rgb = nrmrgb(&v1->nrm);
			v2->tex.rgb = nrmrgb(&v2->nrm);
			v3->tex.rgb = nrmrgb(&v3->nrm);
		}
	}
	fclose(f);
	return 0;
}

struct growBuffer {
	char *ptr;
	int max;
	//~ int cnt;
	int esz;
};

static void* growBuff(struct growBuffer* buff, int cnt) {
	int sss = cnt * buff->esz;
	if (sss >= buff->max) {
		while (sss >= buff->max)
			buff->max <<= 1;
		buff->ptr = realloc(buff->ptr, buff->max);
	}
	return buff->ptr ? buff->ptr + sss : NULL;
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

static int read_obj(mesh* msh, const char* file) {
	FILE *fin;

	char buff[65536], *ptr;
	int posi = 0, texi = 0;
	int nrmi = 0, line = 0;
	struct growBuffer texb;
	struct growBuffer nrmb;

	if (!(fin = fopen(file, "rt"))) return -1;

	initBuff(&nrmb, 64, 3 * sizeof(double));
	initBuff(&texb, 64, 2 * sizeof(double));

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
		//~ if (readCmd(buff, "mg")) continue;		// Merging group

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
			double vtx[3];
			sscanf(ptr, "%lf%lf%lf", vtx + 0, vtx + 1, vtx + 2);
			//~ debug("%d/%d", posi, texi);
			setvtxDV(msh, posi, vtx, 'p');
			posi += 1;
			continue;
		}
		if ((ptr = readCmd(buff, "vt"))) {		// Texture vertices
			double *vtx = growBuff(&texb, texi);
			if (!vtx) {debug("memory");break;}
			sscanf(ptr, "%lf%lf", vtx + 0, vtx + 1);
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
		if ((ptr = readCmd(buff, "vn"))) {		// Vertex normals
			double *vtx = growBuff(&texb, nrmi);
			if (!vtx) {debug("memory");break;}
			sscanf(ptr, "%lf%lf%lf", vtx + 0, vtx + 1, vtx + 2);
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
					if (!strchr("+-0123456789", *ptr)) {
						break;
					}
					ptr = readNum(ptr, &vn[i]);
				}

				if (v[i]) v[i] += v[i] < 0 ? posi : -1;
				if (vn[i]) vn[i] += vn[i] < 0 ? nrmi : -1;
				if (vt[i]) vt[i] += vt[i] < 0 ? texi : -1;

				if (v[i] > posi || v[i] < 0) break;
				if (vn[i] > nrmi || vn[i] < 0) break;
				if (vt[i] > texi || vt[i] < 0) break;

				if (texi) setvtxDV(msh, v[i], growBuff(&texb, vt[i]), 't');
				//~ if (nrmi) setvtxDV(msh, v[i], growBuff(&texb, vn[i]), 'n');
				/*if (nrmi && v[i] != vn[i]) {
					if (nrmi > v[i]) {
						fprintf(stderr, "%s:%d: normal:%s", file, line, buff);
						nrmi = 0;
						//~ break;
					}
					if (v[i] < msh->vtxcnt && vn[i] < msh->vtxcnt)
						msh->vtxptr[v[i]].nrm = msh->vtxptr[vn[i]].nrm;
				}
				if (texi && v[i] != vt[i]) {
					if (texi > v[i]) {
						fprintf(stderr, "%s:%d: texture:%s", file, line, buff);
						texi = 0;
						//~ break;
					}
					if (v[i] < msh->vtxcnt && vt[i] < msh->vtxcnt)
						msh->vtxptr[v[i]].tex = msh->vtxptr[vt[i]].tex;
				}*/
			}
			if (*ptr != '\n') break;
			if (addtri(msh, v[0], v[1], v[2]) < 0) break;
			if (i == 4) if (addtri(msh, v[0], v[2], v[3]) < 0) break;
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

float freadf32(FILE *fin) {
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

static int read_3ds(mesh* msh, const char* filename) {
	FILE *fin;
	char buff[65536];

	unsigned short chunk_id;
	unsigned int chunk_len;
	unsigned short qty;

	int i, vtxi = 0;
	int posi = 0, texi = 0;
	//~ int mtli = 0, obji = 0;				// material, mesh index

	if (!(fin = fopen (filename, "rb"))) return -1;

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
							if (setvtxFV(msh, vtxi + i, dat, 'p') < 0) return 1;
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
							if (setvtxFV(msh, vtxi + i, dat, 't') < 0) return 2;
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

int save_obj(mesh* msh, const char* file) {
	unsigned i;
	FILE* out = fopen(file, "wt");

	for (i = 0; i < msh->vtxcnt; i += 1) {
		vecptr pos = &msh->vtxptr[i].pos;
		vecptr nrm = &msh->vtxptr[i].nrm;
		scalar s = msh->vtxptr[i].tex.s / 65535.;
		scalar t = msh->vtxptr[i].tex.t / 65535.;
		fprintf(out, "v  %f %f %f\n", pos->x, pos->y, pos->z);
		if (msh->hasTex) fprintf(out, "vt %f %f\n", s, -t);
		if (msh->hasNrm) fprintf(out, "vn %f %f %f\n", nrm->x, nrm->y, nrm->z);
	}

	for (i = 0; i < msh->tricnt; i += 1) {
		int i1 = msh->triptr[i].i1 + 1;
		int i2 = msh->triptr[i].i2 + 1;
		int i3 = msh->triptr[i].i3 + 1;
		if (msh->hasTex && msh->hasNrm) fprintf(out, "f  %d/%d/%d %d/%d/%d %d/%d/%d\n", i1, i1, i1, i2, i2, i2, i3, i3, i3);
		else if (msh->hasNrm) fprintf(out, "f  %d//%d %d//%d %d//%d\n", i1, i1, i2, i2, i3, i3);
		else if (msh->hasTex) fprintf(out, "f  %d/%d %d/%d %d/%d\n", i1, i1, i2, i2, i3, i3);
		else fprintf(out, "f  %d %d %d\n", i1, i2, i3);
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

int readMesh(mesh* msh, const char* file) {
	char *ext;
	//~ printf("readMesh(%s:'%s', %s:'%s')\n", fext(file), file, fext(texf), texf);
	msh->hasTex = msh->hasNrm = 0;
	msh->tricnt = msh->vtxcnt = 0;
	if (file && (ext = fext(file))) {
		if (stricmp(ext, "obj") == 0)
			return read_obj(msh, file);
		if (stricmp(ext, "3ds") == 0)
			return read_3ds(msh, file);
	}
	return -9;
}

void bboxMesh(mesh *msh, vecptr min, vecptr max) {
	unsigned i;
	if (msh->vtxcnt == 0) {
		vecldf(min, 0, 0, 0, 0);
		vecldf(max, 0, 0, 0, 0);
		return;
	}

	*min = *max = msh->vtxptr[0].pos;

	for (i = 1; i < msh->vtxcnt; i += 1) {
		vecmax(max, max, &msh->vtxptr[i].pos);
		vecmin(min, min, &msh->vtxptr[i].pos);
	}
}

/*void tranMesh(mesh *msh, matptr mat) {
	unsigned i;
	for (i = 0; i < msh->vtxcnt; i += 1)
		matvp4(&msh->vtxptr[i].pos, mat, &msh->vtxptr[i].pos);
}*/

void centMesh(mesh *msh, scalar size) {
	unsigned i;
	vector min, max, use;

	bboxMesh(msh, &min, &max);

	vecsca(&use, vecadd(&use, &max, &min), 1./2);		// scale
	for (i = 0; i < msh->vtxcnt; i += 1)
		vecsub(&msh->vtxptr[i].pos, &msh->vtxptr[i].pos, &use);

	if (size == 0) return;

	vecsub(&use, &max, &min);		// resize
	if (use.x < use.y) use.x = use.y;
	if (use.x < use.z) use.x = use.z;
	size = 2 * size / use.x;

	for (i = 0; i < msh->vtxcnt; i += 1) {
		vecsca(&msh->vtxptr[i].pos, &msh->vtxptr[i].pos, size);
		msh->vtxptr[i].pos.w = 1;
	}
}

void normMesh(mesh *msh, scalar tol) {
	vector tmp;
	unsigned i, j;
	for (i = 0; i < msh->vtxcnt; i += 1)
		vecldf(&msh->vtxptr[i].nrm, 0, 0, 0, 1);

	for (i = 0; i < msh->tricnt; i += 1) {
		vector nrm;
		vertex *v1 = &msh->vtxptr[msh->triptr[i].i1];
		vertex *v2 = &msh->vtxptr[msh->triptr[i].i2];
		vertex *v3 = &msh->vtxptr[msh->triptr[i].i3];
		backface(&nrm, &v1->pos, &v2->pos, &v3->pos);
		vecadd(&v1->nrm, &v1->nrm, &nrm);
		vecadd(&v2->nrm, &v2->nrm, &nrm);
		vecadd(&v3->nrm, &v3->nrm, &nrm);
	}

	if (tol) for (i = 1; i < msh->vtxcnt; i += 1) {
		for (j = 0; j < i; j += 1) {
			if (vtxcmp(msh, i, j, tol) < 2) {
			//~ if (vtxcmp(msh, i, j, tol) == 0) {
				vecadd(&tmp, &msh->vtxptr[j].nrm, &msh->vtxptr[i].nrm);
				msh->vtxptr[i].nrm = tmp;
				msh->vtxptr[j].nrm = tmp;
				break;
			}
		}
	}// */

	for (i = 0; i < msh->vtxcnt; i += 1) {
		//~ vecsca(&msh->vtxptr[i].nrm, &msh->vtxptr[i].nrm, 1./msh->vtxptr[i].nrm.w);
		msh->vtxptr[i].nrm.w = 0;
		vecnrm(&msh->vtxptr[i].nrm, &msh->vtxptr[i].nrm);
	}
}// */

/**	subdivide mesh
 *	TODO
**/
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
void sdivMesh(mesh *msh) {
	unsigned i4, i5, i6, i, n = msh->tricnt;
	for (i = 0; i < n; i++) {
		vertex nv1, nv2, nv3;
		unsigned i1 = msh->triptr[i].i1;
		unsigned i2 = msh->triptr[i].i2;
		unsigned i3 = msh->triptr[i].i3;
		sdivvtx(&nv1, &msh->vtxptr[i1], &msh->vtxptr[i2]);
		sdivvtx(&nv2, &msh->vtxptr[i2], &msh->vtxptr[i3]);
		sdivvtx(&nv3, &msh->vtxptr[i3], &msh->vtxptr[i1]);

		i4 = addvtx(msh, &nv1.pos, &nv1.nrm, nv1.tex.val);
		i5 = addvtx(msh, &nv2.pos, &nv2.nrm, nv2.tex.val);
		i6 = addvtx(msh, &nv3.pos, &nv3.nrm, nv3.tex.val);

		msh->triptr[i].i2 = i4;
		msh->triptr[i].i3 = i6;
		addtri(msh, i4, i2, i5);
		addtri(msh, i5, i3, i6);
		addtri(msh, i4, i5, i6);
	}
}

void optiMesh(mesh *msh, scalar tol, int prgCB(float prec)) {
	unsigned i, j, k, n;
	if (prgCB == NULL) prgCB = prgDefCB;
	prgCB(0);
	for (n = i = 1; i < msh->vtxcnt; i += 1) {
		if (prgCB(precent(i, msh->vtxcnt)) == -1) break;
		for (j = 0; j < n && vtxcmp(msh, i, j, tol) != 0; j += 1);

		if (j == n) msh->vtxptr[n++] = msh->vtxptr[i];
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

int readf(mesh *msh, char* name) {
	FILE *f = fopen(name, "r");
	int cnt, n;
	if (f == NULL) return -1;

	fscanf(f, "%d", &cnt);
	if (fgetc(f) != ';') return 1;
	n = cnt;
	for (n = 0; n < cnt; n += 1) {
		double pos[3];
		fscanf(f, "%lf", &pos[0]);
		if (fgetc(f) != ';') return 1;
		fscanf(f, "%lf", &pos[1]);
		if (fgetc(f) != ';') return 1;
		fscanf(f, "%lf", &pos[2]);
		if (fgetc(f) != ';') return 1;
		switch (fgetc(f)) {
			default  : return -4;
			case ';' : if (n != cnt-1) {printf("pos: n != cnt : %d != %d\n", n, cnt); return -2;}
			case ',' : break;
		}
		if (n != addvtxdv(msh, pos, NULL, NULL, 0)) return -99;
	}
	for (n = 0; n < cnt; n += 1) {
		double nrm[3];
		fscanf(f, "%lf", &nrm[0]);
		if (fgetc(f) != ';') return 1;
		fscanf(f, "%lf", &nrm[1]);
		if (fgetc(f) != ';') return 1;
		fscanf(f, "%lf", &nrm[2]);
		if (fgetc(f) != ';') return 1;
		switch (fgetc(f)) {
			default  : return -4;
			case ';' : if (n != cnt-1) {printf("pos: n != cnt : %d != %d\n", n, cnt); return -2;}
			case ',' : break;
		}
		vecldf(&msh->vtxptr[n].nrm, nrm[0], nrm[1], nrm[2], 0);
		//setvtx(n, NULL, nrm);
	}
	fscanf(f, "%d", &cnt);
	if (fgetc(f) != ';') return 1;
	while (cnt--) {
		unsigned n, tri[3];
		fscanf(f, "%d", &n);
		if (fgetc(f) != ';') return 1;
		if (n == 3) {
			fscanf(f, "%d", &tri[0]);
			if (fgetc(f) != ',') return 2;
			fscanf(f, "%d", &tri[1]);
			if (fgetc(f) != ',') return 2;
			fscanf(f, "%d", &tri[2]);
			if (fgetc(f) != ';') return 2;
			switch (fgetc(f)) {
				default  : return -5;
				case ',' : break;
				case ';' : if (cnt) return -3;
			}
			addtri(msh, tri[0], tri[1], tri[2]);
		}
		else 
			fprintf(stdout, "tri : cnt:%d n:%d\n", cnt, n);
	}
	return 0;
}

int readf1(mesh *msh, char* name) {
	FILE *f = fopen(name, "r");
	int cnt, n;
	if (f == NULL) return -1;

	fscanf(f, "%d", &cnt);
	//~ if (fgetc(f) != '\n') return 1;

	for (n = 0; n < cnt; n += 1) {
		double pos[3], nrm[3];//, tex[2];
		fscanf(f, "%lf%lf%lf", &pos[0], &pos[1], &pos[2]);
		fscanf(f, "%lf%lf%lf", &nrm[0], &nrm[1], &nrm[2]);
		if (n != addvtxdv(msh, pos, nrm, NULL, 0)) return -99;
	}
	for ( ; ; ) {
		unsigned tri[3];
		fscanf(f, "%d%d%d", &tri[0], &tri[1], &tri[2]);
		if (feof(f)) break;
		addtri(msh, tri[0], tri[1], tri[2]);
	}
	fclose(f);
	return 0;
}


*/

