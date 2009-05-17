/* Standard library

	The contents of this file will be automatically included in a special global
scope before every input file
//~ */

//~ int32 operator add(int32 a, uns32 b) {return emit(i32.add, i32(b), i32(a));}
//~ int32 operator add(int32 a, int32 b) {return emit(i32.add, i32(b), i32(a));}
//~ int64 operator add(int32 a, int64 b) {return emit(i64.add, i64(b), i64(a));}
//~ flt32 operator add(int32 a, flt32 b) {return emit(f32.add, f32(b), f32(a));}
//~ flt64 operator add(int32 a, flt64 b) {return emit(f64.add, f64(b), f64(a));}

//~ int64 operator add(int64 a, uns32 b) {return emit(i64.add, i64(b), i64(a));}
//~ int64 operator add(int64 a, int32 b) {return emit(i64.add, i64(b), i64(a));}
//~ int64 operator add(int64 a, int64 b) {return emit(i64.add, i64(b), i64(a));}
//~ flt32 operator add(int64 a, flt32 b) {return emit(f32.add, f32(b), f32(a));}
//~ flt64 operator add(int64 a, flt64 b) {return emit(f64.add, f64(b), f64(a));}

//~ flt32 operator add(flt32 a, uns32 b) {return emit(f32.add, f32(b), f32(a));}
//~ flt32 operator add(flt32 a, int32 b) {return emit(f32.add, f32(b), f32(a));}
//~ flt32 operator add(flt32 a, int64 b) {return emit(f32.add, f32(b), f32(a));}
//~ flt32 operator add(flt32 a, flt32 b) {return emit(f32.add, f32(b), f32(a));}
//~ flt64 operator add(flt32 a, flt64 b) {return emit(f64.add, f64(b), f64(a));}

//~ flt64 operator add(flt64 a, uns32 b) {return emit(f64.add, f64(b), f64(a));}
//~ flt64 operator add(flt64 a, int32 b) {return emit(f64.add, f64(b), f64(a));}
//~ flt64 operator add(flt64 a, int64 b) {return emit(f64.add, f64(b), f64(a));}
//~ flt64 operator add(flt64 a, flt32 b) {return emit(f64.add, f64(b), f64(a));}
//~ flt64 operator add(flt64 a, flt64 b) {return emit(f64.add, f64(b), f64(a));}

class uns32 {
	u32 value;
	int32 add(int32 &a, int32 b) {return a = add(a, b);}
	int32 operator add(int32 &a, int32 b) {return a = add(a, b);}
	int32 operator add(int32 a, int32 b) {return emit(i32.add, i32(a), i32(b));}
}
class int32 {
	i32 value;
	int32 operator add(int32 &a, int32 b) {return a = add(a, b);}
	int32 operator add(int32 a, int32 b) {return emit(i32.add, i32(a), i32(b));}
}
class int64 {
	i64 value;
	int64 operator add(int64 a, int64 b) {return emit(i64.add, i64(a), i64(b));}
	int64 operator add(int64 &a, int64 b) {return a = add(a, b);}
}
class flt32 {
	f32 value;
}
class flt64 {
	f64 value;
}// +/

class cpl64 {
	flt64 re;
	flt64 im;
	this(flt64 re) {
		this.re = re;
		this.im = 0;
	}
	this(flt64 re, flt64 im) {
		this.re = re;
		this.im = im;
	}

	cpl64 operator mul(cpl64 a, cpl64 b) const {
		emit(pf2.mul, b.im, b.re, a.im, a.re);		// a.re * b.re, a.im * b.im
		emit(pf2.mul, b.re, b.im, a.im, a.re);		// a.re * b.im, a.im * b.re
		a.im = emit(f64.add);						//>im = a.re * b.im + a.im * b.re
		a.re = emit(f64.sub);						//>re = a.re * b.re - a.im * b.im
		return a;
	}
	cpl64 operator mul(cpl64 a, cpl64 b) { return a += b;}

	cpl64 operator mul(cpl64 a, flt64 b) asm {
		return emit(pf2.mul, b, b, a.im, a.re);
	}
	cpl64 class operator mul(flt64 a, cpl64 b) asm {
		return emit(pf2.mul, b, b, a.im, a.re);
	}
	cpl64 class operator mul(cpl64 &a, flt64 b) {return a = mul(a, b);}

	cpl64 operator add(cpl64 a, cpl64 b) asm {
		return emit(pf2.add, b.im, b.re, a.im, a.re);
	}
	cpl64 operator add(cpl64 &a, flt64 b) {return a = add(a, b);}
	cpl64 operator sub(cpl64 a, cpl64 b) asm {
		return emit(v4f.sub, b.im, b.re, a.im, a.re);
	}
	cpl64 operator neg(cpl64 a) asm {
		return emit(pf2.neg, a.im, a.re);
	}
	cpl64 operator rcp(cpl64 a) asm {
		//~ return new Complex(
			//~ +a.re / (a.re * a.re + a.im * a.im),
			//~ -a.im / (a.re * a.re + a.im * a.im));
		emit(neg.f64, a);	// a.re, -a.im
		emit(mul.pd2, a, a);	// a.re ** 2, a.im ** 2
		emit(dupp, 0);		// a.re ** 2, a.im ** 2
		emit(add.pd2);
		emit(div.pd2);
	}
}
class vec4f {
	flt32 x;
	flt32 y;
	flt32 z;
	flt32 w;
	//~ static this() {}
	this() {x = y = z = w = 0;}
	this(flt32 x) {this.x = this.y = this.z = x; this.w = 1;}
	this(flt32 x, flt32 y, flt32 z) {this.x = x; this.y = y; this.z = z; this.w = 0;}
	this(flt32 x, flt32 y, flt32 z, flt32 w) {this.x = x; this.y = y; this.z = z; this.w = w;}

	vec4f operator +(vec4f lhs, vec4f rhs) {return add(lhs, rhs);}
	vec4f operator +(vec4f lhs, flt32 rhs) {return add(lhs, vec4f(rhs));}
	vec4f operator +(flt32 lhs, vec4f rhs) {return add(vec4f(lhs), rhs);}
	vec4f operator +=(out vec4f lhs, vec4f rhs) {return lhs = add(lhs, rhs);}
	vec4f operator +=(out vec4f lhs, flt32 rhs) {return lhs = add(lhs, vec4f(rhs));}

	static vec4f neg(vec4f rhs) {emit(pf4.neg, p4f(rhs));}
	static vec4f add(vec4f lhs, vec4f rhs) {return emit(add.pf4, v4f(lhs), v4f(rhs));}
	static vec4f sub(vec4f lhs, vec4f rhs) {return emit(sub.pf4, v4f(lhs), v4f(rhs));}
	static vec4f mul(vec4f lhs, vec4f rhs) {return emit(mul.pf4, v4f(lhs), v4f(rhs));}
	static vec4f div(vec4f lhs, vec4f rhs) {return emit(div.pf4, v4f(lhs), v4f(rhs));}

	static flt32 dp3(vec4f lhs, vec4f rhs) {return flt32(emit(dp3.pf4, vec4f(lhs), vec4f(rhs)));}
	static flt32 dp4(vec4f lhs, vec4f rhs) {return flt32(emit(dp4.pf4, vec4f(lhs), vec4f(rhs)));}
	static flt32 dph(vec4f lhs, vec4f rhs) {return flt32(emit(dph.pf4, vec4f(lhs), vec4f(rhs)));}
	static flt32 len(vec4f lhs) {return sqrt(flt32(emit(dph.pf4, vec4f(lhs), vec4f(lhs))));}

	static flt32 eval(float val) {return (((w * val + z) * val + y) * val) + x;}

	flt32 dp3(vec4f rhs) {return dp3(this, rhs);}
	flt32 dph(vec4f rhs) {return dph(this, rhs);}
	flt32 dp4(vec4f rhs) {return dp4(this, rhs);}
	flt32 len() {return len(this);}

	static vec4f mul(out vec4f res, ref vec4f lhs, ref vec4f rhs) {
		res.x = lhs.x * rhs.x;
		res.y = lhs.y * rhs.y;
		res.z = lhs.z * rhs.z;
		res.w = lhs.w * rhs.w;
		return res;
	}

	operator vec *= (vec rhs) {return mul(this, this, rhs);}
	static operator vec * (vec lhs, vec rhs) {return mul(this, lhs, rhs);}

	flt32 operator () (flt32 val) {return this.eval(val);}
	flt32 operator [] (uns32 idx) {return idx == 0 ? x : idx == 1 ? y : idx == 2 ? z : w;}
};
class mat4f {
	union {
		scalar d[16];
		scalar m[4][4];
		struct {
			vector x;
			vector y;
			vector z;
			vector w;
		}
		struct {
			scalar m11, m12, m13, m14;
			scalar m21, m22, m23, m24;
			scalar m31, m32, m33, m34;
			scalar m41, m42, m43, m44;
		}
		struct {
			scalar xx, xy, xz, xt;
			scalar yx, yy, yz, yt;
			scalar zx, zy, zz, zt;
			scalar wx, wy, wz, wt;
		}
	}
	//~ static this() {}
	this() {}
	this(flt32 x) {
		this.x = vec4f(x, 0, 0, 0);
		this.y = vec4f(0, x, 0, 0);
		this.z = vec4f(0, 0, x, 0);
		this.w = vec4f(0, 0, 0, x);
	}

	mat4f operator +(ref mat4f lhs, ref mat4f rhs) {return add(new, lhs, rhs);}
	mat4f operator +=(out mat4f lhs, ref mat4f rhs) {return add(lhs, lhs, rhs);}

	static vec4f dp3(ref mat4f mat, vec4f vec) {
		return vec4f(
			vec4f.dp3(mat.x, vec),
			vec4f.dp3(mat.y, vec),
			vec4f.dp3(mat.z, vec),
			1);
	}
	static vec4f dp4(ref mat4f mat, vec4f vec) {
		return vec4f(
			vec4f.dp4(mat.x, src),
			vec4f.dp4(mat.y, src),
			vec4f.dp4(mat.z, src),
			vec4f.dp4(mat.w, src));
	}
	static vec4f dph(ref mat4f mat, vec4f vec) {
		return vec4f(
			vec4f.dph(mat.x, src),
			vec4f.dph(mat.y, src),
			vec4f.dph(mat.z, src),
			vec4f.dph(mat.w, src));
	}

	vec4f dp3(vec4f rhs) {return dp3(this, rhs);}
	vec4f dph(vec4f rhs) {return dph(this, rhs);}
	vec4f dp4(vec4f rhs) {return dp4(this, rhs);}

	vec4f operator mul(ref mat4f mat, vec4f vec) {return dp4(mat, rhs);}

	static mat4f add(out mat4f res, ref mat4f lhs, ref mat4f rhs) {
		res.x = lhs.x + rhs.x;
		res.y = lhs.y + rhs.y;
		res.z = lhs.z + rhs.z;
		res.w = lhs.w + rhs.w;
		return dst;
	}
	static mat4f sub(out mat4f res, ref mat4f lhs, ref mat4f rhs) {
		res.x = lhs.x - rhs.x;
		res.y = lhs.y - rhs.y;
		res.z = lhs.z - rhs.z;
		res.w = lhs.w - rhs.w;
		return dst;
	}
}
class intAP ;
class fltAP ;

/+ class intAP {
	uns32 size;
	uns32 data[];

	this() {bits = null;}
	this(uns32 size) {
		this.size = size;
		this.data = new uns32[size];
	}
	uns32 add(uns32[] res, uns32[] lhs, uns32[] rhs, uns32 llen, uns32 rlen) {
		int c = 0, i = 0;
		while (i < llen && i < rlen) {
			res[i] = lhs[i] + rhs[i] + c;
			c = res[i] < rhs[i];
			i += 1;
		}
		while (i < llen) {
			res[i] = lhs[i] + c;
			i += 1;
			c = 0;
		}
		while (i < rlen) {
			res[i] = lhs[i] + c;
			i += 1;
			c = 0;
		}
	}

	intAP operator add(intAP a, intAP b) {
		uns32 i = 0, c = 0, n = min(a.size, b.size);
		intAP result = intAP(n);
		add(result.data, a.data, a.size, b.data, b.size)
		return result;
	}
	intAP operator add(intAP &a, intAP b) {
		if (a.size < b.size) {
			uns32 data[] = new uns32[b.size];
			for (int i = 0; i < a.size; i += 1)
				a.data[i] = b.data[i];
			a.size = b.size;
		}
		add(a.data, a.data, a.size, b.data, b.size);
		for (int i = 0; i < b.size; i += 1)
			a[i] += b[i];
		return a;
	}
}

class poly {
	uns32 size;
	flt32 data[];

} // +/
// */
