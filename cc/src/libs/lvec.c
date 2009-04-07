#include <math.h>

typedef float scalar;

typedef union argb32_t {
	unsigned long val;
	unsigned char d[4];
	struct {
		unsigned char	b;
		unsigned char	g;
		unsigned char	r;
		unsigned char	a;
	};
} bvec, argb;

typedef union vector_t {
	scalar d[4];
	struct {
		scalar x;
		scalar y;
		scalar z;
		scalar w;
	};
	struct {
		scalar r;
		scalar g;
		scalar b;
		scalar a;
	}; // */
} vector, *vecptr;

typedef union matrix_t {
	scalar d[16];
	scalar m[4][4];
	struct {
		vector x;
		vector y;
		vector z;
		vector w;
	};
	struct {
		scalar m11, m12, m13, m14;
		scalar m21, m22, m23, m24;
		scalar m31, m32, m33, m34;
		scalar m41, m42, m43, m44;
	};
	struct {
		scalar xx, xy, xz, xt;
		scalar yx, yy, yz, yt;
		scalar zx, zy, zz, zt;
		scalar wx, wy, wz, wt;
	};
} matrix, *matptr;

typedef enum {		// swizzle
	xxxx = 0x00, yxxx = 0x01, zxxx = 0x02, wxxx = 0x03, xyxx = 0x04, yyxx = 0x05, zyxx = 0x06, wyxx = 0x07, 
	xzxx = 0x08, yzxx = 0x09, zzxx = 0x0a, wzxx = 0x0b, xwxx = 0x0c, ywxx = 0x0d, zwxx = 0x0e, wwxx = 0x0f, 
	xxyx = 0x10, yxyx = 0x11, zxyx = 0x12, wxyx = 0x13, xyyx = 0x14, yyyx = 0x15, zyyx = 0x16, wyyx = 0x17, 
	xzyx = 0x18, yzyx = 0x19, zzyx = 0x1a, wzyx = 0x1b, xwyx = 0x1c, ywyx = 0x1d, zwyx = 0x1e, wwyx = 0x1f, 
	xxzx = 0x20, yxzx = 0x21, zxzx = 0x22, wxzx = 0x23, xyzx = 0x24, yyzx = 0x25, zyzx = 0x26, wyzx = 0x27, 
	xzzx = 0x28, yzzx = 0x29, zzzx = 0x2a, wzzx = 0x2b, xwzx = 0x2c, ywzx = 0x2d, zwzx = 0x2e, wwzx = 0x2f, 
	xxwx = 0x30, yxwx = 0x31, zxwx = 0x32, wxwx = 0x33, xywx = 0x34, yywx = 0x35, zywx = 0x36, wywx = 0x37, 
	xzwx = 0x38, yzwx = 0x39, zzwx = 0x3a, wzwx = 0x3b, xwwx = 0x3c, ywwx = 0x3d, zwwx = 0x3e, wwwx = 0x3f, 
	xxxy = 0x40, yxxy = 0x41, zxxy = 0x42, wxxy = 0x43, xyxy = 0x44, yyxy = 0x45, zyxy = 0x46, wyxy = 0x47, 
	xzxy = 0x48, yzxy = 0x49, zzxy = 0x4a, wzxy = 0x4b, xwxy = 0x4c, ywxy = 0x4d, zwxy = 0x4e, wwxy = 0x4f, 
	xxyy = 0x50, yxyy = 0x51, zxyy = 0x52, wxyy = 0x53, xyyy = 0x54, yyyy = 0x55, zyyy = 0x56, wyyy = 0x57, 
	xzyy = 0x58, yzyy = 0x59, zzyy = 0x5a, wzyy = 0x5b, xwyy = 0x5c, ywyy = 0x5d, zwyy = 0x5e, wwyy = 0x5f, 
	xxzy = 0x60, yxzy = 0x61, zxzy = 0x62, wxzy = 0x63, xyzy = 0x64, yyzy = 0x65, zyzy = 0x66, wyzy = 0x67, 
	xzzy = 0x68, yzzy = 0x69, zzzy = 0x6a, wzzy = 0x6b, xwzy = 0x6c, ywzy = 0x6d, zwzy = 0x6e, wwzy = 0x6f, 
	xxwy = 0x70, yxwy = 0x71, zxwy = 0x72, wxwy = 0x73, xywy = 0x74, yywy = 0x75, zywy = 0x76, wywy = 0x77, 
	xzwy = 0x78, yzwy = 0x79, zzwy = 0x7a, wzwy = 0x7b, xwwy = 0x7c, ywwy = 0x7d, zwwy = 0x7e, wwwy = 0x7f, 
	xxxz = 0x80, yxxz = 0x81, zxxz = 0x82, wxxz = 0x83, xyxz = 0x84, yyxz = 0x85, zyxz = 0x86, wyxz = 0x87, 
	xzxz = 0x88, yzxz = 0x89, zzxz = 0x8a, wzxz = 0x8b, xwxz = 0x8c, ywxz = 0x8d, zwxz = 0x8e, wwxz = 0x8f, 
	xxyz = 0x90, yxyz = 0x91, zxyz = 0x92, wxyz = 0x93, xyyz = 0x94, yyyz = 0x95, zyyz = 0x96, wyyz = 0x97, 
	xzyz = 0x98, yzyz = 0x99, zzyz = 0x9a, wzyz = 0x9b, xwyz = 0x9c, ywyz = 0x9d, zwyz = 0x9e, wwyz = 0x9f, 
	xxzz = 0xa0, yxzz = 0xa1, zxzz = 0xa2, wxzz = 0xa3, xyzz = 0xa4, yyzz = 0xa5, zyzz = 0xa6, wyzz = 0xa7, 
	xzzz = 0xa8, yzzz = 0xa9, zzzz = 0xaa, wzzz = 0xab, xwzz = 0xac, ywzz = 0xad, zwzz = 0xae, wwzz = 0xaf, 
	xxwz = 0xb0, yxwz = 0xb1, zxwz = 0xb2, wxwz = 0xb3, xywz = 0xb4, yywz = 0xb5, zywz = 0xb6, wywz = 0xb7, 
	xzwz = 0xb8, yzwz = 0xb9, zzwz = 0xba, wzwz = 0xbb, xwwz = 0xbc, ywwz = 0xbd, zwwz = 0xbe, wwwz = 0xbf, 
	xxxw = 0xc0, yxxw = 0xc1, zxxw = 0xc2, wxxw = 0xc3, xyxw = 0xc4, yyxw = 0xc5, zyxw = 0xc6, wyxw = 0xc7, 
	xzxw = 0xc8, yzxw = 0xc9, zzxw = 0xca, wzxw = 0xcb, xwxw = 0xcc, ywxw = 0xcd, zwxw = 0xce, wwxw = 0xcf, 
	xxyw = 0xd0, yxyw = 0xd1, zxyw = 0xd2, wxyw = 0xd3, xyyw = 0xd4, yyyw = 0xd5, zyyw = 0xd6, wyyw = 0xd7, 
	xzyw = 0xd8, yzyw = 0xd9, zzyw = 0xda, wzyw = 0xdb, xwyw = 0xdc, ywyw = 0xdd, zwyw = 0xde, wwyw = 0xdf, 
	xxzw = 0xe0, yxzw = 0xe1, zxzw = 0xe2, wxzw = 0xe3, xyzw = 0xe4, yyzw = 0xe5, zyzw = 0xe6, wyzw = 0xe7, 
	xzzw = 0xe8, yzzw = 0xe9, zzzw = 0xea, wzzw = 0xeb, xwzw = 0xec, ywzw = 0xed, zwzw = 0xee, wwzw = 0xef, 
	xxww = 0xf0, yxww = 0xf1, zxww = 0xf2, wxww = 0xf3, xyww = 0xf4, yyww = 0xf5, zyww = 0xf6, wyww = 0xf7, 
	xzww = 0xf8, yzww = 0xf9, zzww = 0xfa, wzww = 0xfb, xwww = 0xfc, ywww = 0xfd, zwww = 0xfe, wwww = 0xff
} vec_swz;

/*{#################################  COLOR8  ##################################

argb rgbneg(argb rhs) {
	argb res;
	res.b = -rhs.b;
	res.g = -rhs.g;
	res.r = -rhs.r;
	res.a = -rhs.a;
	return res;
}

argb rgbnot(argb rhs) {
	argb res;
	res.val = ~rhs.val;
	return res;
}

argb rgband(argb lhs, argb rhs) {
	argb res;
	res.val = lhs.val & rhs.val;
	return res;
}

argb rgbior(argb lhs, argb rhs) {
	argb res;
	res.val = lhs.val | rhs.val;
	return res;
}

argb rgbxor(argb lhs, argb rhs) {
	argb res;
	res.val = lhs.val ^ rhs.val;
	return res;
}

argb rgbadd(argb lhs, argb rhs) {
	argb res;
	register long tmp;
	res.b = (tmp = lhs.b + rhs.b) > 255 ? 255 : tmp;
	res.g = (tmp = lhs.g + rhs.g) > 255 ? 255 : tmp;
	res.r = (tmp = lhs.r + rhs.r) > 255 ? 255 : tmp;
	res.a = (tmp = lhs.a + rhs.a) > 255 ? 255 : tmp;
	return res;
}

argb rgbsub(argb lhs, argb rhs) {
	argb res;
	register long tmp;
	res.b = (tmp = lhs.b - rhs.b) < 0 ? 0 : tmp;
	res.g = (tmp = lhs.g - rhs.g) < 0 ? 0 : tmp;
	res.r = (tmp = lhs.r - rhs.r) < 0 ? 0 : tmp;
	res.a = (tmp = lhs.a - rhs.a) < 0 ? 0 : tmp;
	return res;
}

argb rgbmul(argb lhs, argb rhs) {
	argb res;
	register long tmp = 0;
	res.b = ((tmp = lhs.b * rhs.b) + (tmp >> 8) + 1) >> 8;
	res.g = ((tmp = lhs.g * rhs.g) + (tmp >> 8) + 1) >> 8;
	res.r = ((tmp = lhs.r * rhs.r) + (tmp >> 8) + 1) >> 8;
	res.a = ((tmp = lhs.a * rhs.a) + (tmp >> 8) + 1) >> 8;
	return res;
}

argb rgbdiv(argb lhs, argb rhs) {
	argb res;
	res.b = (lhs.b << 8) / (rhs.b + 1);
	res.g = (lhs.g << 8) / (rhs.g + 1);
	res.r = (lhs.r << 8) / (rhs.r + 1);
	res.a = (lhs.a << 8) / (rhs.a + 1);
	return res;
}

argb rgbmin(argb lhs, argb rhs) {
	argb res;
	res.b = lhs.b < rhs.b ? lhs.b : rhs.b;
	res.g = lhs.g < rhs.g ? lhs.g : rhs.g;
	res.r = lhs.r < rhs.r ? lhs.r : rhs.r;
	res.a = lhs.a < rhs.a ? lhs.a : rhs.a;
	return res;
}

argb rgbmax(argb lhs, argb rhs) {
	argb res;
	res.b = lhs.b > rhs.b ? lhs.b : rhs.b;
	res.g = lhs.g > rhs.g ? lhs.g : rhs.g;
	res.r = lhs.r > rhs.r ? lhs.r : rhs.r;
	res.a = lhs.a > rhs.a ? lhs.a : rhs.a;
	return res;
}

argb rgbmix(argb lhs, argb rhs, unsigned char cnt) {
	argb res;
	register long tmp;
	res.b = rhs.b + (((tmp = (lhs.b - rhs.b) * cnt) + (tmp >> 8) + 1) >> 8);
	res.g = rhs.g + (((tmp = (lhs.g - rhs.g) * cnt) + (tmp >> 8) + 1) >> 8);
	res.r = rhs.r + (((tmp = (lhs.r - rhs.r) * cnt) + (tmp >> 8) + 1) >> 8);
	res.a = rhs.a + (((tmp = (lhs.a - rhs.a) * cnt) + (tmp >> 8) + 1) >> 8);
	return res;
}

argb rgbscr(argb lhs, argb rhs) {
	//~ argb res;
	//~ res.b = 255 - ((255 - lhs.b) * (255 - rhs.b + 1) >> 8);
	//~ res.g = 255 - ((255 - lhs.g) * (255 - rhs.g + 1) >> 8);
	//~ res.r = 255 - ((255 - lhs.r) * (255 - rhs.r + 1) >> 8);
	//~ res.a = 255 - ((255 - lhs.a) * (255 - rhs.a + 1) >> 8);
	//~ return res;
	return rgbneg(rgbmul(rgbneg(lhs), rgbnot(rhs)));
}

argb rgbovr(argb lhs, argb rhs) {
	argb res;
	res.b = lhs.b <= 127 ? (lhs.b * (1 + rhs.b)) >> 7 : 255 - (((255 - lhs.b) * (256 - rhs.b)) >> 7);
	res.g = lhs.g <= 127 ? (lhs.g * (1 + rhs.g)) >> 7 : 255 - (((255 - lhs.g) * (256 - rhs.g)) >> 7);
	res.r = lhs.r <= 127 ? (lhs.r * (1 + rhs.r)) >> 7 : 255 - (((255 - lhs.r) * (256 - rhs.r)) >> 7);
	res.a = lhs.a <= 127 ? (lhs.a * (1 + rhs.a)) >> 7 : 255 - (((255 - lhs.a) * (256 - rhs.a)) >> 7);
	return res;
}
//} */
//{#################################  VECTOR  ##################################

inline vecptr vecldc(vecptr dst, argb col) {
	dst->b = col.b / 255.;
	dst->g = col.g / 255.;
	dst->r = col.r / 255.;
	dst->a = col.a / 255.;
	return dst;
}

inline vecptr veclds(vecptr dst, scalar s) {
	dst->x = dst->y = dst->z = dst->w = s;
	return dst;
}

inline vecptr vecldf(vecptr dst, scalar x, scalar y, scalar z, scalar w) {
	dst->x = x;
	dst->y = y;
	dst->z = z;
	dst->w = w;
	return dst;
}

inline vecptr veccpy(vecptr dst, vecptr src) {
	*dst = *src;
	return dst;
}

inline vecptr vecneg(vecptr dst, vecptr src) {
	dst->x = -src->x;
	dst->y = -src->y;
	dst->z = -src->z;
	dst->w = -src->w;
	return dst;
}

inline vecptr vecabs(vecptr dst, vecptr src) {
	dst->x = src->x >= 0 ? src->x : -src->x;
	dst->y = src->y >= 0 ? src->y : -src->y;
	dst->z = src->z >= 0 ? src->z : -src->z;
	dst->w = src->w >= 0 ? src->w : -src->w;
	return dst;
}

inline vecptr vecrcp(vecptr dst, vecptr src) {
	dst->x = src->x ? 1. / src->x : 0;
	dst->y = src->y ? 1. / src->y : 0;
	dst->z = src->z ? 1. / src->z : 0;
	dst->w = src->w ? 1. / src->w : 0;
	return dst;
}

inline vecptr vecsat(vecptr dst, vecptr src) {
	if (src->x > 1) dst->x = 1; else if (src->x < 0) dst->x = 0; else dst->x = src->x;
	if (src->y > 1) dst->y = 1; else if (src->y < 0) dst->y = 0; else dst->y = src->y;
	if (src->z > 1) dst->z = 1; else if (src->z < 0) dst->z = 0; else dst->z = src->z;
	if (src->w > 1) dst->w = 1; else if (src->w < 0) dst->w = 0; else dst->w = src->w;
	return dst;
}

inline scalar vecdp3(vecptr lhs, vecptr rhs) {
	return lhs->x * rhs->x + lhs->y * rhs->y + lhs->z * rhs->z;
}

inline scalar vecdph(vecptr lhs, vecptr rhs) {
	return lhs->x * rhs->x + lhs->y * rhs->y + lhs->z * rhs->z + lhs->w;
}

inline scalar vecdp4(vecptr lhs, vecptr rhs) {
	return lhs->x * rhs->x + lhs->y * rhs->y + lhs->z * rhs->z + lhs->w * rhs->w;
}

inline vecptr vecmin(vecptr dst, vecptr lhs, vecptr rhs) {
	dst->x = lhs->x < rhs->x ? lhs->x : rhs->x;
	dst->y = lhs->y < rhs->y ? lhs->y : rhs->y;
	dst->z = lhs->z < rhs->z ? lhs->z : rhs->z;
	dst->w = lhs->w < rhs->w ? lhs->w : rhs->w;
	return dst;
}

inline vecptr vecmax(vecptr dst, vecptr lhs, vecptr rhs) {
	dst->x = lhs->x > rhs->x ? lhs->x : rhs->x;
	dst->y = lhs->y > rhs->y ? lhs->y : rhs->y;
	dst->z = lhs->z > rhs->z ? lhs->z : rhs->z;
	dst->w = lhs->w > rhs->w ? lhs->w : rhs->w;
	return dst;
}

inline vecptr vecadd(vecptr dst, vecptr lhs, vecptr rhs) {
	dst->x = lhs->x + rhs->x;
	dst->y = lhs->y + rhs->y;
	dst->z = lhs->z + rhs->z;
	dst->w = lhs->w + rhs->w;
	return dst;
}

inline vecptr vecsub(vecptr dst, vecptr lhs, vecptr rhs) {
	dst->x = lhs->x - rhs->x;
	dst->y = lhs->y - rhs->y;
	dst->z = lhs->z - rhs->z;
	dst->w = lhs->w - rhs->w;
	return dst;
}

inline vecptr vecmul(vecptr dst, vecptr lhs, vecptr rhs) {
	dst->x = lhs->x * rhs->x;
	dst->y = lhs->y * rhs->y;
	dst->z = lhs->z * rhs->z;
	dst->w = lhs->w * rhs->w;
	return dst;
}

inline vecptr veccrs(vecptr dst, vecptr lhs, vecptr rhs) {
	vector tmp;
	tmp.x = lhs->y * rhs->z - lhs->z * rhs->y;
	tmp.y = lhs->z * rhs->x - lhs->x * rhs->z;
	tmp.z = lhs->x * rhs->y - lhs->y * rhs->x;
	return veccpy(dst, &tmp);
}

inline vecptr vecsca(vecptr dst, vecptr lhs, scalar rhs) {
	dst->x = lhs->x * rhs;
	dst->y = lhs->y * rhs;
	dst->z = lhs->z * rhs;
	dst->w = lhs->w * rhs;
	return dst;
}

inline vecptr vecswz(vecptr dst, vecptr src, vec_swz swz) {
	vector tmp;
	if (src == dst)
		src = veccpy(&tmp, src);
	dst->x = src->d[(swz >> 0) & 3]; 
	dst->y = src->d[(swz >> 2) & 3]; 
	dst->z = src->d[(swz >> 4) & 3]; 
	dst->w = src->d[(swz >> 6) & 3]; 
	return dst;
}

inline vecptr vecnrm(vecptr dst, vecptr src) {
	scalar len = vecdp3(src, src);
	if (len)
		len = 1. / sqrt(len);
	dst->x = src->x * len;
	dst->y = src->y * len;
	dst->z = src->z * len;
	dst->w = src->w * len;
	return dst;
}

inline vecptr vecHCS(vecptr dst, vecptr src) {
	scalar len = src->w;
	if (len) vecsca(dst, src, 1. / len);
	else dst->x = dst->y = dst->z = dst->w = 0;
	return dst;
}

//~ inline vecptr vecMOV(vecptr dst, vecptr src, int how);

//? inline vecptr vecABS(vecptr dst, vecptr src);	| a |						// max(-src, src)
//+ inline vecptr vecRCP(vecptr dst, vecptr src);	1 / a
//~ inline vecptr vecRSQ(vecptr dst, vecptr src);								// <=> vecPOW(dst, src, <-1/2, -1/2, -1/2, -1/2>)

//? inline vecptr vecFLR(vecptr dst, vecptr src);
//? inline vecptr vecFRC(vecptr dst, vecptr src);

//+ inline vecptr vecSIN(vecptr dst, vecptr src);
//+ inline vecptr vecCOS(vecptr dst, vecptr src);
//+ inline vecptr vecLG2(vecptr dst, vecptr src);
//+ inline vecptr vecEX2(vecptr dst, vecptr src);

//? inline vecptr vecPOW(vecptr dst, vecptr lhs, vecptr rhs);					// Exp2(rhs * Log2(lhs))
//+ inline vecptr vecLRP(vecptr dst, vecptr lhs, vecptr rhs, vecptr cnt);		// lhs*(cnt) + rhs*(1-cnt)

//~ inline vecptr vecLIT(vecptr dst, ???);

//? inline vecptr vecCMP(vecptr dst, vecptr lhs, vecptr rhs, int how);			// min/max/sge/slt
//~ inline vecptr vecMAX(vecptr dst, vecptr lhs, vecptr rhs);	// set \lt
//~ inline vecptr vecMIN(vecptr dst, vecptr lhs, vecptr rhs);	// set \ge
//~ inline vecptr vecSGE(vecptr dst, vecptr lhs, vecptr rhs);	// set \ge
//~ inline vecptr vecSLT(vecptr dst, vecptr lhs, vecptr rhs);	// set \lt

//~ inline vecptr vecMAD(vecptr dst, vecptr src, vecptr mul, vecptr add);

/*

inline long veccmp(vecptr lhs, vecptr rhs, vec_cmp cmp) {
	union {
		long val;
		struct {
			char x;
			char y;
			char z;
			char w;
		};
	} res;
	if (cmp) return -1;
	res.x = (char)(lhs->x < rhs->x ? -1 : lhs->x == rhs->x ? 0 : 1);
	res.y = (char)(lhs->y < rhs->y ? -1 : lhs->y == rhs->y ? 0 : 1);
	res.z = (char)(lhs->z < rhs->z ? -1 : lhs->z == rhs->z ? 0 : 1);
	res.w = (char)(lhs->w < rhs->w ? -1 : lhs->w == rhs->w ? 0 : 1);
	return res.val;
}

long vecequ(vecptr lhs, vecptr rhs, scalar eps) {
	if ((lhs->x > rhs->x) ? (lhs->x - rhs->x) : (rhs->x - lhs->x) > eps) return 0;
	if ((lhs->y > rhs->y) ? (lhs->y - rhs->y) : (rhs->y - lhs->y) > eps) return 0;
	if ((lhs->z > rhs->z) ? (lhs->z - rhs->z) : (rhs->z - lhs->z) > eps) return 0;
	//~ if ((lhs->w > rhs->w) ? (lhs->w - rhs->w) : (rhs->w - lhs->w) > eps) return 0;
	return 1;
}

#pragma aux (parm [edi] [eax] [edx] value [edi] modify exact []) vecmul =\
	".686"\
	"movps	xmm0, [eax]"\
	"mulps	xmm0, [edx]"\
	"movups	[edi], xmm0"

*/

inline scalar vecdp2(vecptr lhs, vecptr rhs) {
	return lhs->x * rhs->x + lhs->y * rhs->y;
}

argb vectoc(vecptr src) {
	argb res;
	res.b = src->b < 0 ? 0 : src->b > 1 ? 255 : src->b * 255;
	res.g = src->g < 0 ? 0 : src->g > 1 ? 255 : src->g * 255;
	res.r = src->r < 0 ? 0 : src->r > 1 ? 255 : src->r * 255;
	res.a = src->a < 0 ? 0 : src->a > 1 ? 255 : src->a * 255;
	return res;
}

//} */
//{#################################  MATRIX  ##################################
matptr matidn(matptr dst) {
	dst->m11 = 1; dst->m12 = 0; dst->m13 = 0; dst->m14 = 0;
	dst->m21 = 0; dst->m22 = 1; dst->m23 = 0; dst->m24 = 0;
	dst->m31 = 0; dst->m32 = 0; dst->m33 = 1; dst->m34 = 0;
	dst->m41 = 0; dst->m42 = 0; dst->m43 = 0; dst->m44 = 1;
	return dst;
}

inline matptr matcpy(matptr dst, matptr src) {
	*dst = *src;
	return dst;
}

matptr matadd(matptr dst, matptr lhs, matptr rhs) {
	vecadd(&dst->x, &lhs->x, &rhs->x);
	vecadd(&dst->y, &lhs->y, &rhs->y);
	vecadd(&dst->z, &lhs->z, &rhs->z);
	vecadd(&dst->w, &lhs->w, &rhs->w);
	return dst;
}

matptr matsub(matptr dst, matptr lhs, matptr rhs) {
	vecsub(&dst->x, &lhs->x, &rhs->x);
	vecsub(&dst->y, &lhs->y, &rhs->y);
	vecsub(&dst->z, &lhs->z, &rhs->z);
	vecsub(&dst->w, &lhs->w, &rhs->w);
	return dst;
}

matptr matsca(matptr dst, matptr lhs, scalar rhs) {
	vecsca(&dst->x, &lhs->x, rhs);
	vecsca(&dst->y, &lhs->y, rhs);
	vecsca(&dst->z, &lhs->z, rhs);
	vecsca(&dst->w, &lhs->w, rhs);
	return dst;
}

matptr matmul(matptr dst, matptr lhs, matptr rhs) {
	matrix tmp;
	int row, col;
	for(row = 0; row < 4; ++row) {
		for(col = 0; col < 4; ++col) {
			tmp.m[row][col] = \
				lhs->m[row][0] * rhs->m[0][col]+
				lhs->m[row][1] * rhs->m[1][col]+
				lhs->m[row][2] * rhs->m[2][col]+
				lhs->m[row][3] * rhs->m[3][col];
		}
	}
	*dst = tmp;
	return dst;//matcpy(dst, &tmp);
}

vecptr matvp3(vecptr dst, matptr mat, vecptr src) {
	vector tmp;
	if (src == dst)
		src = veccpy(&tmp, src);
	dst->x = vecdp3(&mat->x, src);
	dst->y = vecdp3(&mat->y, src);
	dst->z = vecdp3(&mat->z, src);
	dst->w = 1;//vecdp3(&mat->w, src);
	return dst;
}

vecptr matvp4(vecptr dst, matptr mat, vecptr src) {
	vector tmp;
	if (src == dst)
		src = veccpy(&tmp, src);
	dst->x = vecdp4(&mat->x, src);
	dst->y = vecdp4(&mat->y, src);
	dst->z = vecdp4(&mat->z, src);
	dst->w = vecdp4(&mat->w, src);
	return dst;
}

vecptr matvph(vecptr dst, matptr mat, vecptr src) {
	vector tmp;
	if (src == dst)
		src = veccpy(&tmp, src);
	dst->x = vecdph(&mat->x, src);
	dst->y = vecdph(&mat->y, src);
	dst->z = vecdph(&mat->z, src);
	dst->w = vecdph(&mat->w, src);
	return dst;
}

matptr matldf(matptr dst, float _11, float _12, float _13, float _14\
			, float _21, float _22, float _23, float _24\
			, float _31, float _32, float _33, float _34\
			, float _41, float _42, float _43, float _44) {
	dst->m11 = _11; dst->m12 = _12; dst->m13 = _13; dst->m14 = _14;
	dst->m21 = _21; dst->m22 = _22; dst->m23 = _23; dst->m24 = _24;
	dst->m31 = _31; dst->m32 = _32; dst->m33 = _33; dst->m34 = _34;
	dst->m41 = _41; dst->m42 = _42; dst->m43 = _43; dst->m44 = _44;
	return dst;
}

// rotate
matptr matRot(matptr dst, vecptr dir, scalar ang) {
	vector tmp;
	scalar xx, yy, zz, xy, yz, xz;
	scalar sin_t = sin(ang);
	scalar cos_t = cos(ang);
	scalar one_c = 1 - cos_t;
	vecsca(&tmp, dir, sin_t);

	xx = dir->x * dir->x;
	yy = dir->y * dir->y;
	zz = dir->z * dir->z;
	xy = dir->x * dir->y;
	xz = dir->x * dir->z;
	yz = dir->y * dir->z;

	dst->xx = one_c * xx + cos_t;
	dst->xy = one_c * xy - tmp.z;
	dst->xz = one_c * xz + tmp.y;
	dst->xt = 0;

	dst->yx = one_c * xy + tmp.z;
	dst->yy = one_c * yy + cos_t;
	dst->yz = one_c * yz - tmp.x;
	dst->yt = 0;

	dst->zx = one_c * xz - tmp.y;
	dst->zy = one_c * yz + tmp.x;
	dst->zz = one_c * zz + cos_t;
	dst->zt = 0;

	dst->wx = 0;
	dst->wy = 0;
	dst->wz = 0;
	dst->wt = 1;
	return dst;
}

// scale
matptr matSca(matptr dst, vecptr dir, scalar cnt) {
	vector tmp;
	matidn(dst);
	vecsca(&tmp, dir, cnt);
	dst->xx = tmp.x;
	dst->yy = tmp.y;
	dst->zz = tmp.z;
	//~ dst->ww = tmp.w;
	return dst;
}

// translate
matptr matTrn(matptr dst, vecptr dir, scalar cnt) {
	vector tmp;
	matidn(dst);
	vecsca(&tmp, dir, cnt);
	dst->xt = tmp.x;
	dst->yt = tmp.y;
	dst->zt = tmp.z;
	//~ dst->wt = tmp.w;
	return dst;
}

void ortho_mat(matptr dst, float l, float r, float b, float t, float n, float f) {
	matrix tmp;
	float	rl = r - l, tb = t - b, nf = n - f;
	if (rl == 0. || tb  == 0. || nf  == 0. ) return;
	matldf(dst,				// scale
		2/(r-l),	0.,		0.,		0.,
		0.,		2/(t-b),	0.,		0.,
		0.,		0.,		2/(n-f),	0.,
		0.,		0.,		0.,		1.);
	matmul(dst, dst, matldf(&tmp,		// translate
		1.,		0.,		0.,		-(l+r)/2,
		0.,		1.,		0.,		-(t+b)/2,
		0.,		0.,		1.,		-(n+f)/2,
		0.,		0.,		0.,		1.));

	/*matldf(dst,			// Projection matrix - orthographic
		2 / rl,			0.,				0.,				-(r+l) / rl,
		0.,				2 / tb,			0.,				-(t+b) / tb,
		0.,				0.,				2 / nf,			-(f+n) / nf,
		0.,				0.,				0.,				1.);
	// */
}

void persp_mat(matptr dst, float l, float r, float b, float t, float n, float f) {
	matrix tmp;
	float	rl = r - l, tb = t - b, nf = n - f;
	if (rl == 0. || tb  == 0. || nf  == 0. ) return;
	//~ /*
	matldf(dst,				// scale
		2/(r-l),	0.,		0.,		0.,
		0.,		2/(t-b),	0.,		0.,
		0.,		0.,		2/(n-f),	0.,
		0.,		0.,		0.,		1.);
	matmul(dst, dst, matldf(&tmp,		// translate
		1.,		0.,		0.,		-(l+r)/2,
		0.,		1.,		0.,		-(t+b)/2,
		0.,		0.,		1.,		-(n+f)/2,
		0.,		0.,		0.,		1.));
	//~ --- Ortographic ---
	matmul(dst, dst, matldf(&tmp,		// project
		n,		0.,		0.,		0,
		0.,		n,		0.,		0,
		0.,		0.,		n+f,		-n*f,
		0.,		0.,		1.,		0.));
	//~ */
}

void projv_mat(matptr dst, float fovy, float asp, float n, float f) {
	float bot = 1, nf = n - f;
	matrix tmp;
	//~ fovy *= (3.14159265358979323846 / 180) / 2;
	//~ if (!(bot = cos( fovy ))) bot = 10000.;
	//~ else bot = sin( fovy ) / bot;
	if (fovy) bot = tan( fovy*(3.14159265358979323846/180)/2);
	asp *= bot;
	matldf(dst,		// Projection matrix	// orthographic
		1 / asp,	0.,		0.,		0,
		0.,		1 / bot,	0.,		0,
		0.,		0.,		2 / nf,		-(f+n) / nf,
		0.,		0.,		0.,		1.);
	if (fovy) matmul(dst, dst, matldf(&tmp,		// perspective
		n,		0.,		0.,		0,
		0.,		n,		0.,		0,
		0.,		0.,		n+f,		-n*f,
		0.,		0.,		1.,		0.));
}

matptr matran(matptr dst, matptr src) {
	matrix tmp;
	int row, col;
	if (src == dst)
		src = matcpy(&tmp, src);
	for(row = 0; row < 4; ++row)
		for(col = 0; col < 4; ++col)
			tmp.m[col][row] = src->m[row][col];
	return matcpy(dst, &tmp);
}

scalar det3x3(  scalar x1, scalar x2, scalar x3,
		scalar y1, scalar y2, scalar y3,
		scalar z1, scalar z2, scalar z3) {
	return	x1 * (y2 * z3 - z2 * y3)-
		x2 * (y1 * z3 - z1 * y3)+
		x3 * (y1 * z2 - z1 * y2);
}

scalar matdet(matptr src) {
	scalar* x = src->x.d;
	scalar* y = src->y.d;
	scalar* z = src->z.d;
	scalar* w = src->w.d;
	return	x[0] * det3x3(y[1], y[2], y[3], z[1], z[2], z[3], w[1], w[2], w[3])-
		x[1] * det3x3(y[0], y[2], y[3], z[0], z[2], z[3], w[0], w[2], w[3])+
		x[2] * det3x3(y[0], y[1], y[3], z[0], z[1], z[3], w[0], w[1], w[3])-
		x[3] * det3x3(y[0], y[1], y[2], z[0], z[1], z[2], w[0], w[1], w[2]);
}

matptr matadj(matptr dst, matptr src) {
	matrix tmp;
	scalar* x = src->x.d;
	scalar* y = src->y.d;
	scalar* z = src->z.d;
	scalar* w = src->w.d;
	matran(&tmp, src);
	dst->m11 = +det3x3(y[1], y[2], y[3], z[1], z[2], z[3], w[1], w[2], w[3]);
	dst->m12 = -det3x3(y[0], y[2], y[3], z[0], z[2], z[3], w[0], w[2], w[3]);
	dst->m13 = +det3x3(y[0], y[1], y[3], z[0], z[1], z[3], w[0], w[1], w[3]);
	dst->m14 = -det3x3(y[0], y[1], y[2], z[0], z[1], z[2], w[0], w[1], w[2]);
	dst->m21 = -det3x3(x[1], x[2], x[3], z[1], z[2], z[3], w[1], w[2], w[3]);
	dst->m22 = +det3x3(x[0], x[2], x[3], z[0], z[2], z[3], w[0], w[2], w[3]);
	dst->m23 = -det3x3(x[0], x[1], x[3], z[0], z[1], z[3], w[0], w[1], w[3]);
	dst->m24 = +det3x3(x[0], x[1], x[2], z[0], z[1], z[2], w[0], w[1], w[2]);
	dst->m31 = +det3x3(x[1], x[2], x[3], y[1], y[2], y[3], w[1], w[2], w[3]);
	dst->m32 = -det3x3(x[0], x[2], x[3], y[0], y[2], y[3], w[0], w[2], w[3]);
	dst->m33 = +det3x3(x[0], x[1], x[3], y[0], y[1], y[3], w[0], w[1], w[3]);
	dst->m34 = -det3x3(x[0], x[1], x[2], y[0], y[1], y[2], w[0], w[1], w[2]);
	dst->m41 = -det3x3(x[1], x[2], x[3], y[1], y[2], y[3], z[1], z[2], z[3]);
	dst->m42 = +det3x3(x[0], x[2], x[3], y[0], y[2], y[3], z[0], z[2], z[3]);
	dst->m43 = -det3x3(x[0], x[1], x[3], y[0], y[1], y[3], z[0], z[1], z[3]);
	dst->m44 = +det3x3(x[0], x[1], x[2], y[0], y[1], y[2], z[0], z[1], z[2]);
	return dst;
}

scalar matinv(matptr dst, matptr src) {
	scalar det = matdet(src);
	if (det) {
		matadj(dst, src);
		matsca(dst, dst, 1./det);
	}
	return det;
}

argb matrgb(matptr mat, argb rgb) {
	vector tmp;
	matvp3(&tmp, mat, vecldc(&tmp, rgb));
	return vectoc(&tmp);
}

//}*/
