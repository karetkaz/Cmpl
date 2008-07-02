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

double* diffS_gen(double dst[3], double s, double t, double* (*evalP)(double [3], double s, double t)) {
	double d[3], p[3];
	evalP(d, s + H, t);
	evalP(p, s, t);
	//~ dst[0] = (d[0] - p[0]) / H;
	//~ dst[1] = (d[1] - p[1]) / H;
	//~ dst[2] = (d[2] - p[2]) / H;
	dst[0] = (p[0] - d[0]) / H;
	dst[1] = (p[1] - d[1]) / H;
	dst[2] = (p[2] - d[2]) / H;
	return dst;
}

double* diffT_gen(double dst[3], double s, double t, double* (*evalP)(double [3], double s, double t)) {
	double d[3], p[3];
	evalP(d, s, t + H);
	evalP(p, s, t);
	//~ dst[0] = (d[0] - p[0]) / H;
	//~ dst[1] = (d[1] - p[1]) / H;
	//~ dst[2] = (d[2] - p[2]) / H;
	dst[0] = (p[0] - d[0]) / H;
	dst[1] = (p[1] - d[1]) / H;
	dst[2] = (p[2] - d[2]) / H;
	return dst;
}

double* evalN(double dst[3], double s, double t, double* (*evalP)(double [3], double s, double t)) {
	double pt[3], ds[3], dt[3];
	//~ if (!s && !t) s = t = H;
	if (!s) s = H;
	if (!t) t = H;
	evalP(pt, s, t);
	evalP(ds, s + H, t);
	evalP(dt, s, t + H);
	ds[0] = (pt[0] - ds[0]) / H;
	ds[1] = (pt[1] - ds[1]) / H;
	ds[2] = (pt[2] - ds[2]) / H;
	dt[0] = (pt[0] - dt[0]) / H;
	dt[1] = (pt[1] - dt[1]) / H;
	dt[2] = (pt[2] - dt[2]) / H;
	return dv3nrm(dst, dv3crs(dst, dt, ds));
}

double* evalP_sphere(double dst[3]/* , double rnm[3], double dft[3], double dfs[3] */, double s, double t) {
	const double s_min = 0, s_max = 1*3.14159265358979323846;
	const double t_min = 0, t_max = 2*3.14159265358979323846;

	const double Tx = 0, Sx =  1;
	const double Ty = 0, Sy =  1;
	const double Tz = 0, Sz =  1;
	s = s * (s_max - s_min) + s_min;
	t = t * (t_max - t_min) + t_min;
	dst[0] = Tx + Sx * (cos(t) * sin(s));
	dst[2] = Ty + Sy * (sin(t) * sin(s));
	dst[1] = Tz + Sz * (cos(s));
	return dst;
}

double* evalP_apple(double dst[3], double s, double t) {
	const double s_min =  0.0, s_max =  2*3.14159265358979323846;
	const double t_min = -3.14159265358979323846, t_max =  3.14159265358979323846;
	s = s * (s_max - s_min) + s_min;
	t = t * (t_max - t_min) + t_min;
	dst[0] = cos(s) * (4 + 3.8 * cos(t));
	dst[1] = sin(s) * (4 + 3.8 * cos(t));
	dst[2] = (cos(t) + sin(t) - 1) * (1 + sin(t)) * log(1 - 3.14159265358979323846 * t / 10) + 7.5 * sin(t);
	return dst;
}

double* evalP_tours(double dst[3], double s, double t) {
	const double R = 20;
	const double r = 10;
	const double s_min =  0.0, s_max =  2*3.14159265358979323846;
	const double t_min =  0.0, t_max =  2*3.14159265358979323846;
	s = s * (s_max - s_min) + s_min;
	t = t * (t_max - t_min) + t_min;
	dst[0] = (R + r * sin(s)) * cos(t);
	dst[1] = (R + r * sin(s)) * sin(t);
	dst[2] = r * cos(s);
	return dst;
}

double* evalP_cone(double dst[3], double s, double t) {
	const double l = 2;
	const double s_min = -1.5, s_max =  1.5;
	//~ const double s_min =  0.0, s_max =  1.5;
	const double t_min =  0.0, t_max =  2*3.14159265358979323846;
	s = s * (s_max - s_min) + s_min;
	t = t * (t_max - t_min) + t_min;
	dst[0] = s * sin(t);
	dst[1] = s * cos(t);
	dst[2] = l * s;
	return dst;
}

double* evalP_001(double dst[3], double s, double t) {
	const double Sx =  20.0;
	const double Sy =  20.0;
	const double Sz =  20.0;
	double sst, H = 10 / 20.;
	const double s_min = -20.0, s_max = +20.0;
	const double t_min = -20.0, t_max = +20.0;
	s = s * (s_max - s_min) + s_min;
	t = t * (t_max - t_min) + t_min;
	if ((sst = sqrt(s*s+t*t)) == 0) sst = 1.;
	dst[0] = Sx * s / 20;
	dst[1] = Sy * t / 20;
	dst[2] = Sz * H * sin(sst) / sst;
	return dst;
}

double* evalP_002(double dst[3], double s, double t) {
	const double Rx =  1.0;
	const double Ry =  1.0;
	const double Rz =  1.0;
	const double s_min =  0.0, s_max =  2*3.14159265358979323846;
	const double t_min =  0.0, t_max =  2*3.14159265358979323846;
	s = s * (s_max - s_min) + s_min;
	t = t * (t_max - t_min) + t_min;
	dst[0] = Rx * cos(s) / (sqrt(2) + sin(t));
	dst[1] = Ry * sin(s) / (sqrt(2) + sin(t));
	dst[2] = -Rz / (sqrt(2) + cos(t));
	return dst;
}

double* evalP_003(double dst[3], double s, double t) {
	dst[0] = s;
	dst[1] = t;
	dst[2] = (s-t)*(s-t);
	return dst;
}
