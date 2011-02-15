//~ wcl386 -zq -ei -6s -d2  -fe=../main code.c clog.c parse.c tree.c type.c ccvm.c main.c
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "ccvm.h"

// default values
static const int wl = 9;		// warning level
static const int ol = 2;		// optimize level
static const int cc = 1;		// execution cores
#define MKB 20					// size of shift
#define memsize (32 << MKB)		// runtime size(128K)
static char mem[memsize];

extern int vmTest();

char *parsei32(const char *str, int32_t *res, int radix) {
	int64_t result = 0;
	int sign = 1;

	//~ ('+' | '-')?
	switch (*str) {
		case '-':
			sign = -1;
		case '+':
			str += 1;
	}

	if (radix == 0) {		// detect
		radix = 10;
		if (*str == '0') {
			str += 1;

			switch (*str) {

				default:
					radix = 8;
					break;

				case 'b':
				case 'B':
					radix = 2;
					str += 1;
					break;

				case 'o':
				case 'O':
					radix = 8;
					str += 1;
					break;

				case 'x':
				case 'X':
					radix = 16;
					str += 1;
					break;

			}
		}
	}

	//~ ([0-9])*
	while (*str) {
		int value = *str;
		if (value >= '0' && value <= '9')
			value -= '0';
		else if (value >= 'a' && value <= 'z')
			value -= 'a' - 10;
		else if (value >= 'A' && value <= 'Z')
			value -= 'A' - 10;
		else break;

		if (value > radix)
			break;

		result *= radix;
		result += value;
		str += 1;
	}

	*res = sign * result;

	return (char *)str;
}
char *matchstr(const char *t, const char *p, int ic) {
	int i;//, ic = flgs & 1;

	for (i = 0; *t && p[i]; ++t, ++i) {
		if (p[i] == '*') {
			if (matchstr(t, p + i + 1, ic))
				return (char *)(t - i);
			return 0;
		}

		if (p[i] == '?' || p[i] == *t)		// skip | match
			continue;

		if (ic && p[i] == tolower(*t))		// ignore case
			continue;

		t -= i;
		i = -1;
	}

	while (p[i] == '*')			// "a***" is "a"
		++p;					// keep i for return

	return (char *)(p[i] ? 0 : t - i);
}
char *parsecmd(char *ptr, char *cmd, char *sws) {
	while (*cmd && *cmd == *ptr)
		cmd++, ptr++;

	if (*cmd != 0)
		return 0;

	if (*ptr == 0)
		return ptr;

	while (strchr(sws, *ptr))
		++ptr;

	return ptr;
}

//{#region std calls
static int f64abs(state s) {
	float64_t x = poparg(s, float64_t);
	setret(float64_t, s, fabs(x));
	return 0;
}
static int f64sin(state s) {
	float64_t x = poparg(s, float64_t);
	setret(float64_t, s, sin(x));
	return 0;
}
static int f64cos(state s) {
	float64_t x = poparg(s, float64_t);
	setret(float64_t, s, cos(x));
	return 0;
}
static int f64tan(state s) {
	float64_t x = poparg(s, float64_t);
	setret(float64_t, s, tan(x));
	return 0;
}

static int f64log(state s) {
	float64_t x = poparg(s, float64_t);
	setret(float64_t, s, log(x));
	return 0;
}
static int f64exp(state s) {
	float64_t x = poparg(s, float64_t);
	setret(float64_t, s, exp(x));
	return 0;
}
static int f64pow(state s) {
	float64_t x = poparg(s, float64_t);
	float64_t y = poparg(s, float64_t);
	setret(float64_t, s, pow(x, y));
	//~ debug("pow(%g, %g) := %g", x, y, pow(x, y));
	return 0;
}
static int f64sqrt(state s) {
	float64_t x = poparg(s, float64_t);
	setret(float64_t, s, sqrt(x));
	return 0;
}

static int f64atan2(state s) {
	float64_t x = poparg(s, float64_t);
	float64_t y = poparg(s, float64_t);
	setret(float64_t, s, atan2(x, y));
	return 0;
}

/*static int f64lg2(state s) {
	double log2(double);
	float64_t *sp = stk;
	*sp = log2(*sp);
	return 0;
}
static int f64_xp2(state s) {
	double exp2(double);
	float64_t *sp = stk;
	*sp = exp2(*sp);
	return 0;
}*/

#include <time.h>
#include <stdlib.h>
static int rand32(state s) {
	static int initialized = 0;
	if (!initialized) {
		srand(time(NULL));
		initialized = 1;
	}
	setret(int32_t, s, rand());
	return 0;
}

enum printEnum {
	putx64,
	puti64,
	putf64,
	putchr,
	putstr,
};

static int printCall(state s) {
	switch (s->func) {
		case putx64:
			fputfmt(stdout, "0x%X", poparg(s, uint64_t));
			break;

		case puti64:
			fputfmt(stdout, "%D", poparg(s, int64_t));
			break;

		case putf64:
			fputfmt(stdout, "%F", poparg(s, float64_t));
			break;

		case putchr:
			fputfmt(stdout, "%c", poparg(s, int32_t));
			break;

		case putstr:
			fputfmt(stdout, "%s", popstr(s));
			break;

		default:
			return -1;
	}
	return 0;
}

/*static int b32bsr(state s) {
	uint32_t x = poparg(s, int32_t);
	unsigned ans = 0;
	if ((x & 0xffff0000) != 0) { ans += 16; x >>= 16; }
	if ((x & 0x0000ff00) != 0) { ans +=  8; x >>=  8; }
	if ((x & 0x000000f0) != 0) { ans +=  4; x >>=  4; }
	if ((x & 0x0000000c) != 0) { ans +=  2; x >>=  2; }
	if ((x & 0x00000002) != 0) { ans +=  1; }
	setret(uint32_t, s, x ? ans : -1);
	return 0;
}// */

static int install_stdc(ccState cc) {
	int i, err = 0;
	struct {
		int (*fun)(state s);
		int n;
		char *def;
	}
	defs[] = {
		//{ MATH
		{f64abs, 0, "float64 abs(float64 x);"},
		{f64sin, 0, "float64 sin(float64 x);"},
		{f64cos, 0, "float64 cos(float64 x);"},
		{f64tan, 0, "float64 tan(float64 x);"},
		{f64log, 0, "float64 log(float64 x);"},
		{f64exp, 0, "float64 exp(float64 x);"},
		{f64pow, 0, "float64 pow(float64 x, float64 y);"},
		{f64sqrt, 0, "float64 sqrt(float64 x);"},
		{f64atan2, 0, "float64 atan2(float64 x, float64 y);"},

		//~ {f64lg2, "float64 log2(float64 x);"},
		//~ {f64xp2, "float64 exp2(float64 x);"},
		//}

		/*/{ BITS
		{b32btc, "int32 btc(int32 x);"},		// bitcount
		{b32bsf, "int32 bsf(int32 x);"},
		{b32bsr, "int32 bsr(int32 x);"},
		{b32swp, "int32 swp(int32 x);"},
		{b32hib, "int32 bhi(int32 x);"},

		{b64bsf, "int32 bsf(int64 x);"},
		{b64bsr, "int32 bsr(int64 x);"},
		{b64lob, "int64 lobit(int64 x);"},
		{b64hib, "int64 hibit(int64 x);"},
		//~ {b64shl, "int64 shl(int64 x, int32 y);"},
		//~ {b64shr, "int64 shr(int64 x, int32 y);"},
		//~ {b64sar, "int64 sar(int64 x, int32 y);"},
		//~ {b64and, "int64 and(int64 x, int64 y);"},
		//~ {b64ior, "int64  or(int64 x, int64 y);"},
		//~ {b64xor, "int64 xor(int64 x, int64 y);"},
		{b64zxt, "int64 zxt(int64 val, int offs, int bits);"},
		{b64sxt, "int64 sxt(int64 val, int offs, int bits);"},
		//} */
	},
	misc[] = {
		//{ IO/MEM/EXIT
		//~ {memmgr, "pointer memmgr(pointer old, int32 cnt, int allign);"},		// allocate, reallocate, free
		//~ {memset, "pointer memset(pointer dst, byte src, int32 cnt);"},
		//~ {memcpy, "pointer memcpy(pointer dst, pointer src, int32 cnt);"},
		{rand32, 0, "int random();"},
		{printCall, putchr, "void putchr(int32 val);"},
		{printCall, putstr, "void putstr(string val);"},
		{printCall, putx64, "void putx64(int64 val);"},
		{printCall, puti64, "void puti64(int64 val);"},
		{printCall, putf64, "void putf64(float64 val);"},
		//}

		// include some of the compiler functions
		// for reflection. (lookup, import, logger, exec?)

	};
	for (i = 0; i < sizeof(defs) / sizeof(*defs); i += 1) {
		if (!libcall(cc->s, defs[i].fun, defs[i].n, defs[i].def)) {
			err = 1;
		}
	}
	for (i = 0; i < sizeof(misc) / sizeof(*misc); i += 1) {
		if (!libcall(cc->s, misc[i].fun, misc[i].n, misc[i].def)) {
			err = 1;
		}
	}
	return err;
}
//}#endregion

//{#region bit operations
enum bits_funs {
	b64_cmt,
	b64_and,
	b64_or,
	b64_xor,
	b64_shl,	// 
	b64_shr,	// 
	b64_sar,	// 

	b32_btc,	// count ones
	//~ b64_btc,

	b32_bsf,	// scan forward
	b64_bsf,

	b32_bsr,	// scan reverse
	b64_bsr,

	b32_bhi,	// highes bit only
	b64_bhi,

	b32_blo,	// lowest bit only
	b64_blo,

	b32_swp,	// 
	//~ b64_swp,

	b32_zxt,	// zero extend
	b64_zxt,

	b32_sxt,	// sign extend
	b64_sxt,
};
static int bits_call(state s) {
	switch (s->libc->offs) {
		case b64_cmt: {
			uint64_t x = poparg(s, int64_t);
			setret(uint64_t, s, ~x);
		} return 0;
		case b64_and: {
			uint64_t x = poparg(s, int64_t);
			uint64_t y = poparg(s, int64_t);
			setret(uint64_t, s, x & y);
		} return 0;
		case b64_or:  {
			uint64_t x = poparg(s, int64_t);
			uint64_t y = poparg(s, int64_t);
			setret(uint64_t, s, x | y);
		} return 0;
		case b64_xor: {
			uint64_t x = poparg(s, int64_t);
			uint64_t y = poparg(s, int64_t);
			setret(uint64_t, s, x ^ y);
		} return 0;
		case b64_shl: {
			uint64_t x = poparg(s, int64_t);
			uint32_t y = poparg(s, int32_t);
			setret(uint64_t, s, x << y);
		} return 0;
		case b64_shr: {
			uint64_t x = poparg(s, int64_t);
			uint32_t y = poparg(s, int32_t);
			setret(uint64_t, s, x >> y);
			//~ debug("%D >> %d = %D", x, y, x >> y);
		} return 0;
		case b64_sar: {
			int64_t x = poparg(s, int64_t);
			uint32_t y = poparg(s, int32_t);
			setret(uint64_t, s, x >> y);
		} return 0;

		case b32_btc: {
			uint32_t x = poparg(s, int32_t);
			x -= ((x >> 1) & 0x55555555);
			x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
			x = (((x >> 4) + x) & 0x0f0f0f0f);
			x += (x >> 8) + (x >> 16);
			setret(uint32_t, s, x & 0x3f);
		} return 0;
		//case b64_btc:

		case b32_bsf: {
			uint32_t x = poparg(s, int32_t);
			int ans = 0;
			if ((x & 0x0000ffff) == 0) { ans += 16; x >>= 16; }
			if ((x & 0x000000ff) == 0) { ans +=  8; x >>=  8; }
			if ((x & 0x0000000f) == 0) { ans +=  4; x >>=  4; }
			if ((x & 0x00000003) == 0) { ans +=  2; x >>=  2; }
			if ((x & 0x00000001) == 0) { ans +=  1; }
			setret(uint32_t, s, x ? ans : -1);
		} return 0;
		case b64_bsf: {
			uint64_t x = poparg(s, int64_t);
			int ans = -1;
			if (x != 0) {
				ans = 0;
				if ((x & 0x00000000ffffffffULL) == 0) { ans += 32; x >>= 32; }
				if ((x & 0x000000000000ffffULL) == 0) { ans += 16; x >>= 16; }
				if ((x & 0x00000000000000ffULL) == 0) { ans +=  8; x >>=  8; }
				if ((x & 0x000000000000000fULL) == 0) { ans +=  4; x >>=  4; }
				if ((x & 0x0000000000000003ULL) == 0) { ans +=  2; x >>=  2; }
				if ((x & 0x0000000000000001ULL) == 0) { ans +=  1; }
			}
			setret(int32_t, s, ans);
		} return 0;

		case b32_bsr: {
			uint32_t x = poparg(s, int32_t);
			unsigned ans = 0;
			if ((x & 0xffff0000) != 0) { ans += 16; x >>= 16; }
			if ((x & 0x0000ff00) != 0) { ans +=  8; x >>=  8; }
			if ((x & 0x000000f0) != 0) { ans +=  4; x >>=  4; }
			if ((x & 0x0000000c) != 0) { ans +=  2; x >>=  2; }
			if ((x & 0x00000002) != 0) { ans +=  1; }
			setret(uint32_t, s, x ? ans : -1);
		} return 0;
		case b64_bsr: {
			uint64_t x = poparg(s, int64_t);
			int ans = -1;
			if (x != 0) {
				ans = 0;
				if ((x & 0xffffffff00000000ULL) != 0) { ans += 32; x >>= 32; }
				if ((x & 0x00000000ffff0000ULL) != 0) { ans += 16; x >>= 16; }
				if ((x & 0x000000000000ff00ULL) != 0) { ans +=  8; x >>=  8; }
				if ((x & 0x00000000000000f0ULL) != 0) { ans +=  4; x >>=  4; }
				if ((x & 0x000000000000000cULL) != 0) { ans +=  2; x >>=  2; }
				if ((x & 0x0000000000000002ULL) != 0) { ans +=  1; }
			}
			setret(int32_t, s, ans);
		} return 0;

		case b32_bhi: {
			uint32_t x = poparg(s, uint32_t);
			x |= x >> 1;
			x |= x >> 2;
			x |= x >> 4;
			x |= x >> 8;
			x |= x >> 16;
			setret(uint32_t, s, x - (x >> 1));
		} return 0;
		case b64_bhi: {
			uint64_t x = poparg(s, int64_t);
			x |= x >> 1;
			x |= x >> 2;
			x |= x >> 4;
			x |= x >> 8;
			x |= x >> 16;
			x |= x >> 32;
			setret(uint64_t, s, x - (x >> 1));
		} return 0;

		case b32_blo: {
			uint32_t x = poparg(s, uint32_t);
			setret(uint32_t, s, x & -x);
		} return 0;
		case b64_blo: {
			uint64_t x = poparg(s, uint64_t);
			setret(uint64_t, s, x & -x);
		} return 0;

		case b32_swp: {
			uint32_t x = poparg(s, int32_t);
			x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
			x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
			x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
			x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
			setret(uint32_t, s, (x >> 16) | (x << 16));
		} return 0;
		//case b64_swp:

		case b32_zxt: {
			uint32_t val = poparg(s, int32_t);
			int32_t ofs = poparg(s, int32_t);
			int32_t cnt = poparg(s, int32_t);
			val <<= 32 - (ofs + cnt);
			setret(int32_t, s, val >> (32 - cnt));
		} return 0;
		case b64_zxt: {
			uint64_t val = poparg(s, int64_t);
			int32_t ofs = poparg(s, int32_t);
			int32_t cnt = poparg(s, int32_t);
			val <<= 64 - (ofs + cnt);
			setret(int64_t, s, val >> (64 - cnt));
		} return 0;

		case b32_sxt: {
			int32_t val = poparg(s, int32_t);
			int32_t ofs = poparg(s, int32_t);
			int32_t cnt = poparg(s, int32_t);
			val <<= 32 - (ofs + cnt);
			setret(int32_t, s, val >> (32 - cnt));
		} return 0;
		case b64_sxt: {
			int64_t val = poparg(s, int64_t);
			int32_t ofs = poparg(s, int32_t);
			int32_t cnt = poparg(s, int32_t);
			val <<= 64 - (ofs + cnt);
			setret(int64_t, s, val >> (64 - cnt));
		} return 0;
	}
	return -1;
}
static int install_bits(ccState cc) {
	int err = 0;
	symn module;
	if ((module = install(cc, "bits", TYPE_rec, 0, 0))) {
		enter(cc, NULL);
		// libcall will return 0 on failure, 
		// 'err = err || !libcall...' <=> 'if (!err) err = !libcall...'
		// will skip forward libcalls if an error ocurred

		err = err || !libcall(cc->s, bits_call, b64_cmt, "int64 cmt(int64 x);");
		err = err || !libcall(cc->s, bits_call, b64_and, "int64 and(int64 x, int64 y);");
		err = err || !libcall(cc->s, bits_call, b64_or,  "int64 or(int64 x, int64 y);");
		err = err || !libcall(cc->s, bits_call, b64_xor, "int64 xor(int64 x, int64 y);");
		err = err || !libcall(cc->s, bits_call, b64_shl, "int64 shl(int64 x, int32 y);");
		err = err || !libcall(cc->s, bits_call, b64_shr, "int64 shr(int64 x, int32 y);");
		err = err || !libcall(cc->s, bits_call, b64_sar, "int64 sar(int64 x, int32 y);");

		//~ err = err || !libcall(cc->s, bits_call, b64_btc, "int32 btc(int64 x);");
		err = err || !libcall(cc->s, bits_call, b64_bsf, "int32 bsf(int64 x);");
		err = err || !libcall(cc->s, bits_call, b64_bsr, "int32 bsr(int64 x);");
		err = err || !libcall(cc->s, bits_call, b64_bhi, "int32 bhi(int64 x);");
		err = err || !libcall(cc->s, bits_call, b64_blo, "int32 blo(int64 x);");
		//~ err = err || !libcall(cc->s, bits_call, b64_swp, "int64 swp(int64 x);");
		err = err || !libcall(cc->s, bits_call, b64_zxt, "int64 zxt(int64 val, int offs, int bits);");
		err = err || !libcall(cc->s, bits_call, b64_sxt, "int64 sxt(int64 val, int offs, int bits);");

		/// 32 bits
		err = err || !libcall(cc->s, bits_call, b32_btc, "int32 btc(int32 x);");
		err = err || !libcall(cc->s, bits_call, b32_bsf, "int32 bsf(int32 x);");
		err = err || !libcall(cc->s, bits_call, b32_bsr, "int32 bsr(int32 x);");
		err = err || !libcall(cc->s, bits_call, b32_bhi, "int32 bhi(int32 x);");
		err = err || !libcall(cc->s, bits_call, b32_blo, "int32 blo(int32 x);");
		err = err || !libcall(cc->s, bits_call, b32_swp, "int32 swp(int32 x);");
		err = err || !libcall(cc->s, bits_call, b32_zxt, "int32 zxt(int32 val, int offs, int bits);");
		err = err || !libcall(cc->s, bits_call, b32_sxt, "int32 sxt(int32 val, int offs, int bits);");
		module->args = leave(cc, module);
	}
	return err;
}
//}#endregion

void usage(char* prog, char* cmd) {
	if (cmd == NULL) {
		fputfmt(stdout, "Usage: %s <command> [options] ...\n", prog);
		fputfmt(stdout, "<Commands>\n");
		fputfmt(stdout, "\t-c: compile\n");
		fputfmt(stdout, "\t-e: execute\n");
		fputfmt(stdout, "\t-d: diassemble\n");
		fputfmt(stdout, "\t-h: help\n");
		//~ fputfmt(stdout, "\t=<expression>: eval\n");
		//~ fputfmt(stdout, "\t-d: dump\n");
	}
	else if (strcmp(cmd, "-c") == 0) {
		fputfmt(stdout, "compile: %s -c [options] file ...args\n", prog);
		fputfmt(stdout, "Options:\n");

		fputfmt(stdout, "\t[Output]\n");
		fputfmt(stdout, "\t-t Output is tags\n");
		fputfmt(stdout, "\t-c Output is ast code\n");
		fputfmt(stdout, "\t-s Output is asm code\n");
		fputfmt(stdout, "\telse output is binary code\n");
		fputfmt(stdout, "\t-o <file> Put output into <file>. [default=stdout]\n");

		fputfmt(stdout, "\t[Loging]\n");
		fputfmt(stdout, "\t-wa all warnings\n");
		fputfmt(stdout, "\t-wx treat warnings as errors\n");
		fputfmt(stdout, "\t-w<num> set warning level to <num> [default=%d]\n", wl);
		fputfmt(stdout, "\t-l <file> Put errors into <file>. [default=stderr]\n");

		fputfmt(stdout, "\t[Execute and Debug]\n");
		fputfmt(stdout, "\t-x<n> execute on <n> procs [default=%d]\n", cc);
		fputfmt(stdout, "\t-xd<n> debug on <n> procs [default=%d]\n", cc);

		//~ fputfmt(stdout, "\t[Debug & Optimization]\n");
	}
	else if (strcmp(cmd, "-h") == 0) {
		fputfmt(stdout, "command: '-h': help\n");
	}
	else {
		fputfmt(stdout, "invalid help for: '%s'\n", cmd);
	}
}

int evalexp(ccState s, char* text) {
	struct astn res;
	astn ast;
	symn typ;
	int tid;

	source(s, 0, text);

	ast = expr(s, 0);
	typ = typecheck(s, 0, ast);
	tid = eval(&res, ast);

	if (peek(s))
		error(s->s, 0, "unexpected: `%k`", peek(s));

	fputfmt(stdout, "eval(`%+k`) = ", ast);

	if (ast && typ && tid) {
		fputfmt(stdout, "%T(%k)\n", typ, &res);
		return 0;
	}

	fputfmt(stdout, "ERROR(typ:`%T`, tid:%d)\n", typ, tid);
	//~ dumpast(stdout, ast, 15);

	return -1;
}

//~ enum callBackType {};
symn callBack = NULL;
static int setCallBack(state s) {
	void* ref = popref(s);
	callBack = findref(s, ref);
	trace("setCallBack(%-T)", callBack);
	return callBack != NULL;
}

int reglibc(state s) {
	int err = 0;

	libcall(s, NULL, 0, "reset");

	if (!libcall(s, libCallExitDebug, 0, "void debug1(int x, int y, int z);"))
		err = 1;

	err = install_stdc(s->cc) || err;

	err = install_bits(s->cc) || err;

	if (!libcall(s, setCallBack, 0, "void setCallBack(void callBack(int x));"))
		err = 1;

	return err;
}

int compile(state s, int wl, char *file) {
	int result;
	#if DEBUGGING > 1
	int size = 0;
	#endif

	if (!ccOpen(s, srcFile, file))
		return -1;

	#if DEBUGGING > 1
	//~ debug("after init:%d Bytes", s->cc->_beg - s->_mem);
	size = (s->cc->_beg - s->_mem);
	#endif

	s->cc->warn = wl;
	result = parse(s->cc, 0);

	#if DEBUGGING > 1
	//~ debug("%s: init(%.2F); scan(%.2F) KBytes", file, size / 1024., (s->cc->_beg - s->_mem) / 1024.);
	#endif

	return result;
}

static int dbgCon(state s, int pu, void *ip, long* bp, int ss);
int program(int argc, char *argv[]) {
	state s = rtInit(mem, sizeof(mem));

	char *prg, *cmd;
	dbgf dbg = NULL;

	prg = argv[0];
	cmd = argv[1];
	if (argc < 2) {
		usage(prg, NULL);
	}
	else if (argc == 2 && *cmd == '=') {	// eval
		if (reglibc(s))
			return -1;
		return evalexp(ccInit(s, creg_def), cmd + 1);
	}
	else if (strcmp(cmd, "-api") == 0) {
		ccState env = ccInit(s, creg_def);
		const int level = 2;
		symn glob;
		int i;

		if (!env)
			return -33;

		reglibc(s);
		glob = leave(env, NULL);
		if (argc == 2) {
			dumpsym(stdout, glob, level);
		}
		else if (argc == 3 && strcmp(argv[2], ".") == 0) {
			while (glob) {
				dumpsym(stdout, glob, 0);
				glob = glob->defs;
			}
		}
		else for (i = 2; i < argc; i += 1) {
			symn sym = findsym(env, glob, argv[i]);
			if (sym) {
				dumpsym(stdout, sym, 0);
				dumpsym(stdout, sym->args, level);
			}
			else
				fputfmt(stderr, "symbol not found `%s`\n", argv[i]);
		}
		return ccDone(s);
	}
	else if (strcmp(cmd, "-c") == 0) {		// compile
		int level = -1, argi;
		int warn = wl;
		int opti = ol;
		int outc = 0;			// output
		char *srcf = 0;			// source
		char *logf = 0;			// logger
		char *outf = 0;			// output

		enum {
			//~ gen_code = 0x0010,
			out_tags = 0x0001,	// tags	// ?offs?
			out_tree = 0x0002,	// walk
			out_dasm = 0x0003,	// dasm
			run_code = 0x0004,	// exec
		};

		// options
		for (argi = 2; argi < argc; ++argi) {
			char *arg = argv[argi];

			// source file
			if (*arg != '-') {
				if (srcf) {
					fputfmt(stderr, "multiple source files are not suported\n");
					return -1;
				}
				srcf = argv[argi];
			}

			// output file
			else if (strcmp(arg, "-l") == 0) {		// log
				if (++argi >= argc || logf) {
					fputfmt(stderr, "logger error\n");
					return -1;
				}
				logf = argv[argi];
			}
			else if (strcmp(arg, "-o") == 0) {		// out
				if (++argi >= argc || outf) {
					fputfmt(stderr, "output error\n");
					return -1;
				}
				outf = argv[argi];
			}

			// output what
			else if (strncmp(arg, "-t", 2) == 0) {		// tags
				level = 2;
				if (arg[2] && *parsei32(arg + 2, &level, 16)) {
					fputfmt(stderr, "invalid argument '%s'\n", arg);
					return 0;
				}
				outc = out_tags;
			}
			else if (strncmp(arg, "-c", 2) == 0) {		// ast
				//~ level = 0;
				if (arg[2] && *parsei32(arg + 2, &level, 16)) {
					fputfmt(stderr, "invalid argument '%s'\n", arg);
					return 0;
				}
				outc = out_tree;
			}
			else if (strncmp(arg, "-s", 2) == 0) {		// asm
				//~ level = 0;
				if (arg[2] && *parsei32(arg + 2, &level, 16)) {
					fputfmt(stderr, "invalid argument '%s'\n", arg);
					return 0;
				}
				outc = out_dasm;
			}

			else if (strncmp(arg, "-x", 2) == 0) {		// exec(&| debug)
				char *str = arg + 2;
				outc = run_code;

				if (*str == 'd' || *str == 'D') {
					dbg = dbgCon;
					str += 1;
				}

				level = 1;

				if (*str && *parsei32(str, &level, 16)) {
					fputfmt(stderr, "invalid level '%s'\n", str);
					debug("invalid level '%s'", str);
					return 0;
				}
			}

			// Override settings
			else if (strncmp(arg, "-w", 2) == 0) {		// warning level
				if (strcmp(arg, "-wx") == 0)
					warn = -1;
				else if (strcmp(arg, "-wa") == 0)
					warn = 9;
				else if (*parsei32(arg + 2, &warn, 10)) {
					fputfmt(stderr, "invalid level '%s'\n", arg + 2);
					debug("invalid level '%s'\n", arg + 2);
					return 0;
				}
			}
			else if (strncmp(arg, "-O", 2) == 0) {		// optimize level
				if (strcmp(arg, "-O") == 0)
					opti = 1;
				else if (strcmp(arg, "-Oa") == 0)
					opti = 3;
				else if (!parsei32(arg + 2, &opti, 10)) {
					fputfmt(stderr, "invalid level '%s'\n", arg + 2);
					debug("invalid level '%s'\n", arg + 2);
					return 0;
				}
			}

			else {
				fputfmt(stderr, "invalid option '%s' for -compile\n", arg);
				return -1;
			}
		}

		if (logf && logfile(s, logf) != 0) {
			fputfmt(stderr, "can not open file `%s`\n", logf);
			return -1;
		}

		if (reglibc(s) != 0) {
			error(s, 0, "error compiling `%s`", "stdlib");
			logfile(s, NULL);
			return -6;
		}

		if (compile(s, warn, "stdlib.cvx") != 0) {
			error(s, 0, "error compiling `%s`", "stdlib.cvx");
			logfile(s, NULL);
			return -9;
		}// */

		if (compile(s, warn, srcf) != 0) {
			error(s, -1, "error compiling `%s`", srcf);
			logfile(s, NULL);
			return s->errc;
		}

		if (outc == out_tags || outc == out_tree) {} else
		if (gencode(s, opti) != 0) {
			//! TODO: rename gencode to something else like assemble or something similar
			logfile(s, NULL);
			return s->errc;
		}

		if (0) {
			symn glob = s->defs;
			while (glob) {
				char* ty = ".err";
				switch (glob->kind) {
					case EMIT_opc: ty = "emit"; break;
					case TYPE_ref: ty = ".ref"; break;
					case TYPE_rec: ty = ".typ"; break;
					case TYPE_def: ty = ".def"; break;
				}
				//~ debug("%s: %-15.15T", ty, glob);
				dumpsym(stdout, glob, 0);
				glob = glob->defs;
			}
		}

		//~ logfile(s, outf);
		//~ s->defs = leave(s->cc, NULL);
		if (outc) switch (outc) {
			default: fatal("FixMe");
			case out_tags: dump2file(s, outf, dump_sym | (level & 0x0ff), NULL); break;
			case out_dasm: dump2file(s, outf, dump_asm | (level & 0xfff), NULL); break;
			case out_tree: dump2file(s, outf, dump_ast | (level & 0x0ff), NULL); break;
			case run_code: exec(s->vm, dbg); break;
		}

		if (1 && outc == run_code) {
			//~ libcSwapExit(s, NULL);
			//~ vmCall(s, findsym(s->cc, NULL, "testCallBack"), 1, 2, 3, 6);
			//~ vmCall(s, findsym(s->cc, NULL, "testCallBack"), 2, 3, 4, 6);
			//~ vmCall(s, findsym(s->cc, NULL, "testCallBack"), 3, 4, 5, 6);
			//~ vmCall(s, findsym(s->cc, NULL, "testCallBack3"));
			if (callBack) {
				vmCall(s, callBack, 4);
			}
			//~ vmCall(s, findsym(s->cc, NULL, "testCallBack"), 4, 5, 6);
		}
		logfile(s, NULL);
		//~ vmDone(s);
		return 0;
	}
	else if (strcmp(cmd, "-h") == 0) {		// help
		//~ char *t = argv[2];
		if (argc < 3) {
			usage(argv[0], argc < 4 ? argv[3] : NULL);
		}
		//~ else if (strcmp(t, "compile") == 0) {}
	}
	else fputfmt(stderr, "invalid option: '%s'\n", cmd);

	return 0;
}

int main(int argc, char *argv[]) {
	if (1 && argc == 1) {
		char *args[] = {
			"psvm",		// program name
			//~ "-h", "-s", "dup.x1", "set.x1",

			//"-api",
			
			"-c",		// compile command
			"-x",		// execute & show symbols command
			"test.cvx",
		};
		argc = sizeof(args) / sizeof(*args);
		argv = args;
	}

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	//~ return mk_test("xxxx.cvx", 8 << 20);	// 8M
	//~ return vmTest();
	//~ return test1();
	return program(argc, argv);
}

static int dbgCon(state s, int pu, void *ip, long* bp, int ss) {
	static char buff[1024];
	static char cmd = 'N';
	vmState vm = s->vm;
	int IP, SP;
	char *arg;

	if (ip == NULL) {
		return 0;
	}

	if (cmd == 'r') {	// && !breakpoint(vm, ip)
		return 0;
	}

	IP = ((char*)ip) - ((char*)vm->_mem);// - vm->pc;
	SP = ((char*)vm->_end) - ((char*)bp);

	fputfmt(stdout, ">exec:[pu%02d][sp%02d]@%9.*A\n", pu, ss, IP, ip);

	if (cmd != 'N') for ( ; ; ) {
		if (fgets(buff, 1024, stdin) == NULL) {
			// Can not read from stdin
			// continue
			cmd = 'N';
			return 0;
		}

		if ((arg = strchr(buff, '\n'))) {
			*arg = 0;		// remove new line char
		}

		if (*buff == 0);		// no command, use last
		else if ((arg = parsecmd(buff, "print", " "))) {
			cmd = 'p';
		}
		else if ((arg = parsecmd(buff, "step", " "))) {

			if (!*arg || strcmp(arg, "over") == 0) {
				cmd = 'n';
			}

			else if (strcmp(arg, "in") == 0) {
				cmd = 'n';
			}

			else if (strcmp(arg, "out") == 0) {
				cmd = 'n';
			}

		}
		else if ((arg = parsecmd(buff, "sp", " "))) {
			cmd = 's';
		}
		else if (buff[1] == 0) {
			arg = buff + 1;
			cmd = buff[0];
		}
		else {
			debug("invalid command %s", buff);
			arg = buff + 1;
			cmd = buff[0];
			buff[1] = 0;
			continue;
		}
		if (!arg) {
			debug("invalid command %s", buff);
			arg = NULL;
			//cmd = 0;
			continue;
		}

		switch (cmd) {
			default:
				debug("invalid command '%c'", cmd);
			case 0 :
				break;

			case 'r' :		// resume
			//~ case 'c' :		// step in
			//~ case 'C' :		// step out
			case 'n' :		// step over
				return 0;
			case 'p' : if (vm->s && vm->s->cc) {		// print
				if (!*arg) {
					// vmTags(vm->s, (void*)sptr, slen, 0);
				}
				else {
					symn sym = findsym(vm->s->cc, NULL, arg);
					debug("arg:%T", sym);
					if (sym && sym->kind == TYPE_ref && sym->offs >= 0) {
						stkval* sp = (stkval*)((char*)bp + ss + sym->offs);
						void vm_fputval(state s, FILE *fout, symn var, stkval* ref, int flgs);
						vm_fputval(vm->s, stdout, sym, sp, 0);
					}
				}
			} break;
			case 's' : {
				int i;
				stkval* sp = (stkval*)bp;
				if (*arg == 0) for (i = 0; i < ss; i++) {
					fputfmt(stdout, "\tsp(%d): {i32(%d), f32(%g), i64(%D), f64(%G)}\n", i, sp[i].i4, sp[i].f4, sp[i].i8, sp[i].f8);
				}
				else if (*parsei32(arg, &i, 10) == '\0') {
					if (i < ss)
						fputfmt(stdout, "\tsp(%d): {i32(%d), f32(%g), i64(%D), f64(%G)}\n", i, sp[i].i4, sp[i].f4, sp[i].i8, sp[i].f8);
					//~ fputfmt(stdout, "\tsp: {i32(%d), i64(%D), f32(%g), f64(%G), p4f(%g, %g, %g, %g), p2d(%G, %G)}\n", sp->i4, sp->i8, sp->f4, sp->f8, sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w, sp->pd.x, sp->pd.y);
				}
				else if (strcmp(arg, "i32") == 0)
					fputfmt(stdout, "\tsp: i32(%d)\n", sp->i4);
				else if (strcmp(arg, "f32") == 0)
					fputfmt(stdout, "\tsp: f32(%d)\n", sp->i8);
				else if (strcmp(arg, "i64") == 0)
					fputfmt(stdout, "\tsp: i64(%d)\n", sp->f4);
				else if (strcmp(arg, "f64") == 0)
					fputfmt(stdout, "\tsp: f64(%d)\n", sp->f8);
			} break;
		}
	}
	return 0;
}

/* temp

int vmHelp(char *cmd) {
	FILE *out = stdout;
	int i, k = 0, n = 0;
	for (i = 0; i < opc_last; ++i) {
		char *opc = (char*)opc_tbl[i].name;
		if (opc && matchstr(opc, cmd, 1)) {
			fputfmt(out, "Instruction: %s\n", opc);
			n += 1;
			k = i;
		}
	}
	if (n == 1 && strcmp(cmd, opc_tbl[k].name) == 0) {
		fputfmt(out, "Opcode: 0x%02x\n", opc_tbl[k].code);
		fputfmt(out, "Length: %d\n", opc_tbl[k].size);
		fputfmt(out, "Stack check: %d\n", opc_tbl[k].chck);
		fputfmt(out, "Stack diff: %+d\n", opc_tbl[k].diff);
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
// */
