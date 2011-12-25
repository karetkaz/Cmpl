#include "api.h"

// bit scan forward
static int b32bsf(state s) {
	uint32_t x = popi32(s);
	int ans = 0;
	//~ if ((x & 0x00000000ffffffff) == 0) { ans += 32; x >>= 32; }
	if ((x & 0x000000000000ffff) == 0) { ans += 16; x >>= 16; }
	if ((x & 0x00000000000000ff) == 0) { ans +=  8; x >>=  8; }
	if ((x & 0x000000000000000f) == 0) { ans +=  4; x >>=  4; }
	if ((x & 0x0000000000000003) == 0) { ans +=  2; x >>=  2; }
	if ((x & 0x0000000000000001) == 0) { ans +=  1; }
	setret(s, uint32_t, x ? ans : -1);
	return 0;
}
// bit scan reverse
static int b32bsr(state s) {
	uint32_t x = popi32(s);
	unsigned ans = 0;
	//~ if ((x & 0xffffffff00000000) != 0) { ans += 32; x >>= 32; }
	if ((x & 0x00000000ffff0000) != 0) { ans += 16; x >>= 16; }
	if ((x & 0x000000000000ff00) != 0) { ans +=  8; x >>=  8; }
	if ((x & 0x00000000000000f0) != 0) { ans +=  4; x >>=  4; }
	if ((x & 0x000000000000000c) != 0) { ans +=  2; x >>=  2; }
	if ((x & 0x0000000000000002) != 0) { ans +=  1; }
	setret(s, uint32_t, x ? ans : -1);
	return 0;
}
// extracts the highest set bit
static int b32hib(state s) {
	uint32_t x = popi32(s);
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	setret(s, uint32_t, x - (x >> 1));
	return 0;
}
// extracts the lowest set bit
static int b32lob(state s) {
	uint32_t x = popi32(s);
	setret(s, uint32_t, x & -x);
	return 0;
}
// count bits
static int b32btc(state s) {
	uint32_t x = popi32(s);
	x -= ((x >> 1) & 0x55555555);
	x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
	x = (((x >> 4) + x) & 0x0f0f0f0f);
	x += (x >> 8) + (x >> 16);
	setret(s, uint32_t, x & 0x3f);
	return 0;
}
// swap bits
static int b32swp(state s) {
	uint32_t x = popi32(s);
	x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
	x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
	x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
	x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
	setret(s, uint32_t, (x >> 16) | (x << 16));
	return 0;
}

static int b64shl(state s) {
	uint64_t x = popi64(s);
	int32_t y = popi32(s);
	setret(s, uint64_t, x << y);
	return 0;
}
static int b64shr(state s) {
	uint64_t x = popi64(s);
	int32_t y = popi32(s);
	setret(s, uint64_t, x >> y);
	return 0;
}
static int b64sar(state s) {
	int64_t x = popi64(s);
	int32_t y = popi32(s);
	setret(s, uint64_t, x >> y);
	return 0;
}
static int b64and(state s) {
	uint64_t x = popi64(s);
	uint64_t y = popi64(s);
	setret(s, uint64_t, x & y);
	return 0;
}
static int b64ior(state s) {
	uint64_t x = popi64(s);
	uint64_t y = popi64(s);
	setret(s, uint64_t, x | y);
	return 0;
}
static int b64xor(state s) {
	uint64_t x = popi64(s);
	uint64_t y = popi64(s);
	setret(s, uint64_t, x ^ y);
	return 0;
}

static int b64bsf(state s) {
	uint64_t x = popi64(s);
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
	setret(s, int32_t, ans);
	return 0;
}
static int b64bsr(state s) {
	uint64_t x = popi64(s);
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
	setret(s, int32_t, ans);
	return 0;
}
static int b64hib(state s) {
	uint64_t x = popi64(s);
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x |= x >> 32;
	setret(s, uint64_t, x - (x >> 1));
	return 0;
}
static int b64lob(state s) {
	uint64_t x = popi64(s);
	setret(s, uint64_t, x & -x);
	return 0;
}

// zero extend(value, offset, bits)
static int b64zxt(state s) {
	uint64_t val = popi64(s);
	int32_t ofs = popi32(s);
	int32_t cnt = popi32(s);
	val <<= 64 - (ofs + cnt);
	setret(s, int64_t, val >> (64 - cnt));
	return 0;
}
// sign extend(value, offset, bits)
static int b64sxt(state s) {
	int64_t val = popi64(s);
	int32_t ofs = popi32(s);
	int32_t cnt = popi32(s);
	val <<= 64 - (ofs + cnt);
	setret(s, int64_t, val >> (64 - cnt));
	return 0;
}

int apiMain(stateApi api) {
	symn nsp;
	struct {
		int (*fun)(state);
		int n;
		char *def;
	}
	defs[] = {
		{b32btc, 0, "int32 btc(int32 x);"},		// bitcount
		{b32bsf, 0, "int32 bsf(int32 x);"},
		{b32bsr, 0, "int32 bsr(int32 x);"},
		{b32swp, 0, "int32 swp(int32 x);"},
		{b32lob, 0, "int32 lobit(int32 x);"},
		{b32hib, 0, "int32 hibit(int32 x);"},

		{b64bsf, 0, "int32 bsf(int64 x);"},
		{b64bsr, 0, "int32 bsr(int64 x);"},
		{b64lob, 0, "int64 lobit(int64 x);"},
		{b64hib, 0, "int64 hibit(int64 x);"},

		{b64shl, 0, "int64 shl(int64 x, int32 y);"},
		{b64shr, 0, "int64 shr(int64 x, int32 y);"},
		{b64sar, 0, "int64 sar(int64 x, int32 y);"},
		{b64and, 0, "int64 and(int64 x, int64 y);"},
		{b64ior, 0, "int64  or(int64 x, int64 y);"},
		{b64xor, 0, "int64 xor(int64 x, int64 y);"},

		{b64zxt, 0, "int64 zxt(int64 val, int offs, int bits);"},
		{b64sxt, 0, "int64 sxt(int64 val, int offs, int bits);"},
	};
	if ((nsp = api->ccBegin(api->rt, "bits"))) {
		int i;
		for (i = 0; i < sizeof(defs) / sizeof(*defs); i += 1) {
			if (!api->libcall(api->rt, defs[i].fun, defs[i].n, defs[i].def)) {
				return -99;
			}
		}
		api->ccEnd(api->rt, nsp);
	}
	return 0;
}
