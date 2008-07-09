//{ param surf
//{ dv3opr

inline double* dv3set(double dst[3], double x, double y, double z) {
	dst[0] = x;
	dst[1] = y;
	dst[2] = z;
	return dst;
}

inline double* dv3cpy(double dst[3], double src[3]) {
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	return dst;
}

inline double* dv3add(double dst[3], double lhs[3], double rhs[3]) {
	dst[0] = lhs[0] + rhs[0];
	dst[1] = lhs[1] + rhs[1];
	dst[2] = lhs[2] + rhs[2];
	return dst;
}

inline double* dv3sub(double dst[3], double lhs[3], double rhs[3]) {
	dst[0] = lhs[0] - rhs[0];
	dst[1] = lhs[1] - rhs[1];
	dst[2] = lhs[2] - rhs[2];
	return dst;
}

inline double* dv3mul(double dst[3], double lhs[3], double rhs[3]) {
	dst[0] = lhs[0] * rhs[0];
	dst[1] = lhs[1] * rhs[1];
	dst[2] = lhs[2] * rhs[2];
	return dst;
}

inline double* dv3sca(double dst[3], double lhs[3], double rhs) {
	dst[0] = lhs[0] * rhs;
	dst[1] = lhs[1] * rhs;
	dst[2] = lhs[2] * rhs;
	return dst;
}

inline double  dv3dot(double lhs[3], double rhs[3]) {
	return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
}

inline double* dv3crs(double dst[3], double lhs[3], double rhs[3]) {
	dst[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
	dst[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
	dst[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
	return dst;
}

inline double* dv3mad(double dst[3]/* , double src[3] */, double mul[3], double add[3]) {
	double* src = dst;
	dst[0] = src[0] * mul[0] + add[0];
	dst[1] = src[1] * mul[1] + add[1];
	dst[2] = src[2] * mul[2] + add[2];
	return dst;
}

inline double* dv3nrm(double dst[3], double src[3]) {
	double len = sqrt(dv3dot(src, src));
	if (len > 0) {
		dst[0] = src[0] / len;
		dst[1] = src[1] / len;
		dst[2] = src[2] / len;
	}
	else {
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
	}
	return dst;
}

//}

static const double H = 1e-5;
static const double MPI = 3.14159265358979323846;

double* evalN(double dst[3], double s, double t, double* (*evalP)(double [3], double [3], double s, double t)) {
	double pt[3], ds[3], dt[3];
	//~ if (!s && !t) s = t = H;
	evalP(pt, 0, s, t);
	//~ if (!s) s = H;
	//~ if (!t) t = H;
	evalP(ds, 0, s + H, t/*  ? t : H */);
	evalP(dt, 0, s/*  ? s : H */, t + H);
	ds[0] = (pt[0] - ds[0]) / H;
	ds[1] = (pt[1] - ds[1]) / H;
	ds[2] = (pt[2] - ds[2]) / H;
	dt[0] = (pt[0] - dt[0]) / H;
	dt[1] = (pt[1] - dt[1]) / H;
	dt[2] = (pt[2] - dt[2]) / H;
	return dv3nrm(dst, dv3crs(dst, ds, dt));
}

double* evalP_sphere(double dst[3], double nrm[3]/* , double dft[3], double dfs[3] */, double s, double t) {
	const double s_min = 0.0, s_max = 1 * MPI;
	const double t_min = 0.0, t_max = 2 * MPI;

	double S = s * (s_max - s_min) + s_min;
	double T = t * (t_max - t_min) + t_min;

	dst[0] = (cos(T) * sin(S));
	dst[1] = (sin(T) * sin(S));
	dst[2] = (cos(S));

	if (nrm != NULL) //dv3nrm(nrm, dst);		// simple
		evalN(nrm, s, t, evalP_sphere);

	return dst;
}

double* evalP_apple(double dst[3], double nrm[3]/* , double dft[3], double dfs[3] */, double s, double t) {
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

double* evalP_tours(double dst[3], double nrm[3]/* , double dft[3], double dfs[3] */, double s, double t) {
	const double s_min =  0.0, s_max =  2 * MPI;
	const double t_min =  0.0, t_max =  2 * MPI;

	const double R = 2;//20;
	const double r = 1;//10;

	double S = s * (s_max - s_min) + s_min;
	double T = t * (t_max - t_min) + t_min;
	dst[0] = (R + r * sin(S)) * cos(T);
	dst[1] = (R + r * sin(S)) * sin(T);
	dst[2] = r * cos(S);

	if (nrm != NULL)
		evalN(nrm, s, t, evalP_tours);
	return dst;
}

double* evalP_cone(double dst[3], double nrm[3]/* , double dft[3], double dfs[3] */, double s, double t) {
	const double s_min = -1.5, s_max =  1.5;
	const double t_min =  0.0, t_max =  2 * MPI;

	const double l = 2;

	double S = s * (s_max - s_min) + s_min;
	double T = t * (t_max - t_min) + t_min;

	dst[0] = S * sin(T);
	dst[1] = S * cos(T);
	dst[2] = l * S;

	if (nrm != NULL)
		evalN(nrm, s, t, evalP_cone);
	return dst;
}

double* evalP_001(double dst[3], double nrm[3]/* , double dft[3], double dfs[3] */, double s, double t) {

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

//~ double* evalP_002(double dst[3], double nrm[3]/* , double dft[3], double dfs[3] */, double s, double t) {
	//~ const double Rx =  1.0;
	//~ const double Ry =  1.0;
	//~ const double Rz =  1.0;
	//~ const double s_min =  0.0, s_max =  2*3.14159265358979323846;
	//~ const double t_min =  0.0, t_max =  2*3.14159265358979323846;
	//~ s = s * (s_max - s_min) + s_min;
	//~ t = t * (t_max - t_min) + t_min;
	//~ dst[0] = Rx * cos(s) / (sqrt(2) + sin(t));
	//~ dst[1] = Ry * sin(s) / (sqrt(2) + sin(t));
	//~ dst[2] = -Rz / (sqrt(2) + cos(t));
	//~ if (nrm != NULL)
		//~ evalN(nrm, s, t, evalP_002);
	//~ return dst;
//~ }

//~ double* evalP_003(double dst[3], double nrm[3]/* , double dft[3], double dfs[3] */, double s, double t) {
	//~ dst[0] = s;
	//~ dst[1] = t;
	//~ dst[2] = (s-t)*(s-t);
	//~ if (nrm != NULL)
		//~ evalN(nrm, s, t, evalP_003);
	//~ return dst;
//~ }
