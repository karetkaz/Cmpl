char* fext(const char* name) {
	char *ext = "";
	char *ptr = (char*)name;
	if (ptr) while (*ptr) {
		if (*ptr == '.')
			ext = ptr + 1;
		ptr += 1;
	}
	return ext;
}

char* readInt(char *ptr, int *outVal) {
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
char* readFlt(char *str, double *outVal) {
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
char* readCmd(char *ptr, char *cmd) {
	//~ return readKVP(ptr, cmd, " ", "");
	while (*cmd && *cmd == *ptr) cmd++, ptr++;
	if (!strchr(" \t\n\r", *ptr)) return 0;
	while (strchr(" \t", *ptr)) ptr += 1;
	return ptr;
}

char* readKVP(char *ptr, char *cmd, char *skp, char *sep) {	// key = value pair
	if (skp == NULL) skp = "";
	if (sep == NULL) sep = " \t\n\r";

	// match cmd and skip white spaces
	while (*cmd && *cmd == *ptr) ++cmd, ++ptr;
	while (*ptr && strchr(sep, *ptr)) ++ptr;

	// match skp and skip white spaces
	while (*skp && *skp == *ptr) ++skp, ++ptr;
	while (*ptr && strchr(sep, *ptr)) ++ptr;

	return (*cmd || *skp) ? 0 : ptr;
}

char* readF32(char *str, float *outVal) {
	double f64;
	str = readFlt(str, &f64);
	if (outVal) *outVal = f64;
	return str;
}
char* readVec(char *str, vector dst, scalar defw) {
	char *sep = ",";

	char *ptr;

	dst->w = defw;

	str = readF32(str, &dst->x);

	if (!*str) {
		dst->y = dst->x;
		dst->z = dst->x;
		return str;
	}

	if (!(ptr = readCmd(str, sep)))
		return str;

	str = readF32(ptr, &dst->y);

	if (!(ptr = readCmd(str, sep)))
		return str;

	str = readF32(ptr, &dst->z);

	if (!*str) return str;

	if (!(ptr = readCmd(str, sep)))
		return str;

	str = readF32(ptr, &dst->w);

	return str;
}

//~ char*
