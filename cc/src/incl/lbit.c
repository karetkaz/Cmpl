int btc(unsigned x) {		// count bits
	x -= ((x >> 1) & 0x55555555);
	x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
	x = (((x >> 4) + x) & 0x0f0f0f0f);
	x += (x >> 8);
	x += (x >> 16);
	return x & 0x3f;
	//~ return (((x >> 8) + (x >> 24)) & 0x3f);
}

// scan bit reverse (first bit = 31 (msb))
int bsr(unsigned x) {
	unsigned ans = 0;
	if (!x) return -1;
	if ((x & 0xffff0000) != 0) { ans += 16; x >>= 16; }
	if ((x & 0x0000ff00) != 0) { ans +=  8; x >>=  8; }
	if ((x & 0x000000f0) != 0) { ans +=  4; x >>=  4; }
	if ((x & 0x0000000c) != 0) { ans +=  2; x >>=  2; }
	if ((x & 0x00000002) != 0) { ans +=  1; }
	return ans;
}

// scan bit forward (first bit = 0 (lsb))
int bsf(unsigned x) {
	int ans = 0;
	if (!x) return -1;
	if ((x & 0x0000ffff) == 0) { ans += 16; x >>= 16; }
	if ((x & 0x000000ff) == 0) { ans +=  8; x >>=  8; }
	if ((x & 0x0000000f) == 0) { ans +=  4; x >>=  4; }
	if ((x & 0x00000003) == 0) { ans +=  2; x >>=  2; }
	if ((x & 0x00000001) == 0) { ans +=  1; }
	return ans;
}

unsigned rot(unsigned val, signed cnt) {
	//~ cnt = cnt & 31;
	return (val << cnt) | (val >> (32 - cnt));
}

unsigned shl(unsigned val, signed cnt) {
	//~ cnt = cnt & 31;
	return (unsigned)val << (signed)cnt;
}

unsigned shr(unsigned val, signed cnt) {
	//~ cnt = cnt & 31;
	return (unsigned)val >> (signed)cnt;
}

unsigned sar(unsigned val, signed cnt) {
	//~ cnt = cnt & 31;
	return (signed)val >> (signed)cnt;
}

unsigned zxt(unsigned val, unsigned offs, unsigned size) {
	return (unsigned)(val << (32 - (offs + size))) >> (32 - size);
}

unsigned sxt(signed val, unsigned offs, unsigned size) {
	return (unsigned)(val << (32 - (offs + size))) >> (32 - size);
}

/*unsigned zxtlh(unsigned val, unsigned hi, unsigned lo) {
	return (unsigned)(val << (32 - hi)) >> ((32 - hi) + lo);
}

unsigned sxt2(signed val, unsigned offs, unsigned size) {
	return (val << (-(offs + size))) >> (-size);
}

int bits(unsigned x) {		// count bits
	unsigned ans = 0;
	while (x) {
		ans += x & 1;
		x >>= 1;
	}
	return ans;
}
*/
