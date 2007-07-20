/****************************************************************************
*   File: Scan.c
*   Date: 2007/04/20
*   Desc: Lexical analyzer
*============================================================================
*
*
****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "node.h"
#include "type.c"

static const unsigned tok_map[256] = {
	/* 000 nul */	0,//tok_eot,
	/* 001 soh */	sym_und,
	/* 002 stx */	sym_und,
	/* 003 etx */	sym_und,
	/* 004 eot */	sym_und,
	/* 005 enq */	sym_und,
	/* 006 ack */	sym_und,
	/* 007 bel */	sym_und,
	/* 010 bs  */	sym_und,
	/* 011 ht  */	sym_spc,	// horz tab
	/* 012 nl  */	sym_spc,	// new line
	/* 013 vt  */	sym_und,//sym_spc,
	/* 014 ff  */	sym_und,//sym_spc,
	/* 015 cr  */	sym_und,
	/* 016 so  */	sym_und,
	/* 017 si  */	sym_und,
	/* 020 dle */	sym_und,
	/* 021 dc1 */	sym_und,
	/* 022 dc2 */	sym_und,
	/* 023 dc3 */	sym_und,
	/* 024 dc4 */	sym_und,
	/* 025 nak */	sym_und,
	/* 026 syn */	sym_und,
	/* 027 etb */	sym_und,
	/* 030 can */	sym_und,
	/* 031 em  */	sym_und,
	/* 032 sub */	sym_und,
	/* 033 esc */	sym_und,
	/* 034 fs  */	sym_und,
	/* 035 gs  */	sym_und,
	/* 036 rs  */	sym_und,
	/* 037 us  */	sym_und,
	/* 040 sp  */	sym_spc,
	/* 041 !   */	tok_opr,
	/* 042 "   */	tok_val,
	/* 043 #   */	sym_und,
	/* 044 $   */	sym_und,
	/* 045 %   */	tok_opr,
	/* 046 &   */	tok_opr,
	/* 047 '   */	tok_val,
	/* 050 (   */	sym_lbs,
	/* 051 )   */	sym_rbs,
	/* 052 *   */	tok_opr,
	/* 053 +   */	tok_opr,
	/* 054 ,   */	sym_com,
	/* 055 -   */	tok_opr,
	/* 056 .   */	sym_dot,
	/* 057 /   */	tok_opr,
	/* 060 0   */	tok_val,
	/* 061 1   */	tok_val,
	/* 062 2   */	tok_val,
	/* 063 3   */	tok_val,
	/* 064 4   */	tok_val,
	/* 065 5   */	tok_val,
	/* 066 6   */	tok_val,
	/* 067 7   */	tok_val,
	/* 070 8   */	tok_val,
	/* 071 9   */	tok_val,
	/* 072 :   */	sym_col,
	/* 073 ;   */	sym_sem,
	/* 074 <   */	tok_opr,
	/* 075 =   */	tok_opr,
	/* 076 >   */	tok_opr,
	/* 077 ?   */	sym_qtm,
	/* 100 @   */	sym_und,
	/* 101 A   */	tok_idf,
	/* 102 B   */	tok_idf,
	/* 103 C   */	tok_idf,
	/* 104 D   */	tok_idf,
	/* 105 E   */	tok_idf,
	/* 106 F   */	tok_idf,
	/* 107 G   */	tok_idf,
	/* 110 H   */	tok_idf,
	/* 111 I   */	tok_idf,
	/* 112 J   */	tok_idf,
	/* 113 K   */	tok_idf,
	/* 114 L   */	tok_idf,
	/* 115 M   */	tok_idf,
	/* 116 N   */	tok_idf,
	/* 117 O   */	tok_idf,
	/* 120 P   */	tok_idf,
	/* 121 Q   */	tok_idf,
	/* 122 R   */	tok_idf,
	/* 123 S   */	tok_idf,
	/* 124 T   */	tok_idf,
	/* 125 U   */	tok_idf,
	/* 126 V   */	tok_idf,
	/* 127 W   */	tok_idf,
	/* 130 X   */	tok_idf,
	/* 131 Y   */	tok_idf,
	/* 132 Z   */	tok_idf,
	/* 133 [   */	sym_lbi,
	/* 134 \   */	sym_und,
	/* 135 ]   */	sym_rbi,
	/* 136 ^   */	tok_opr,
	/* 137 _   */	tok_idf,
	/* 140 `   */	sym_und,
	/* 141 a   */	tok_idf,
	/* 142 b   */	tok_idf,
	/* 143 c   */	tok_idf,
	/* 144 d   */	tok_idf,
	/* 145 e   */	tok_idf,
	/* 146 f   */	tok_idf,
	/* 147 g   */	tok_idf,
	/* 150 h   */	tok_idf,
	/* 151 i   */	tok_idf,
	/* 152 j   */	tok_idf,
	/* 153 k   */	tok_idf,
	/* 154 l   */	tok_idf,
	/* 155 m   */	tok_idf,
	/* 156 n   */	tok_idf,
	/* 157 o   */	tok_idf,
	/* 160 p   */	tok_idf,
	/* 161 q   */	tok_idf,
	/* 162 r   */	tok_idf,
	/* 163 s   */	tok_idf,
	/* 164 t   */	tok_idf,
	/* 165 u   */	tok_idf,
	/* 166 v   */	tok_idf,
	/* 167 w   */	tok_idf,
	/* 170 x   */	tok_idf,
	/* 171 y   */	tok_idf,
	/* 172 z   */	tok_idf,
	/* 173 {   */	sym_lbb,
	/* 174 |   */	tok_opr,
	/* 175 }   */	sym_rbb,
	/* 176 ~   */	tok_opr,
	/* 177    */	sym_und,
	/* 200 �   */	sym_und,
	/* 201 �   */	sym_und,
	/* 202 �   */	sym_und,
	/* 203 �   */	sym_und,
	/* 204 �   */	sym_und,
	/* 205 �   */	sym_und,
	/* 206 �   */	sym_und,
	/* 207 �   */	sym_und,
	/* 210 �   */	sym_und,
	/* 211 �   */	sym_und,
	/* 212 �   */	sym_und,
	/* 213 �   */	sym_und,
	/* 214 �   */	sym_und,
	/* 215 �   */	sym_und,
	/* 216 �   */	sym_und,
	/* 217 �   */	sym_und,
	/* 220 �   */	sym_und,
	/* 221 �   */	sym_und,
	/* 222 �   */	sym_und,
	/* 223 �   */	sym_und,
	/* 224 �   */	sym_und,
	/* 225 �   */	sym_und,
	/* 226 �   */	sym_und,
	/* 227 �   */	sym_und,
	/* 230 �   */	sym_und,
	/* 231 �   */	sym_und,
	/* 232 �   */	sym_und,
	/* 233 �   */	sym_und,
	/* 234 �   */	sym_und,
	/* 235 �   */	sym_und,
	/* 236 �   */	sym_und,
	/* 237 �   */	sym_und,
	/* 240 �   */	sym_und,
	/* 241 �   */	sym_und,
	/* 242 �   */	sym_und,
	/* 243 �   */	sym_und,
	/* 244 �   */	sym_und,
	/* 245 �   */	sym_und,
	/* 246 �   */	sym_und,
	/* 247 �   */	sym_und,
	/* 250 �   */	sym_und,
	/* 251 �   */	sym_und,
	/* 252 �   */	sym_und,
	/* 253 �   */	sym_und,
	/* 254 �   */	sym_und,
	/* 255 �   */	sym_und,
	/* 256 �   */	sym_und,
	/* 257 �   */	sym_und,
	/* 260 �   */	sym_und,
	/* 261 �   */	sym_und,
	/* 262 �   */	sym_und,
	/* 263 �   */	sym_und,
	/* 264 �   */	sym_und,
	/* 265 �   */	sym_und,
	/* 266 �   */	sym_und,
	/* 267 �   */	sym_und,
	/* 270 �   */	sym_und,
	/* 271 �   */	sym_und,
	/* 272 �   */	sym_und,
	/* 273 �   */	sym_und,
	/* 274 �   */	sym_und,
	/* 275 �   */	sym_und,
	/* 276 �   */	sym_und,
	/* 277 �   */	sym_und,
	/* 300 �   */	sym_und,
	/* 301 �   */	sym_und,
	/* 302 �   */	sym_und,
	/* 303 �   */	sym_und,
	/* 304 �   */	sym_und,
	/* 305 �   */	sym_und,
	/* 306 �   */	sym_und,
	/* 307 �   */	sym_und,
	/* 310 �   */	sym_und,
	/* 311 �   */	sym_und,
	/* 312 �   */	sym_und,
	/* 313 �   */	sym_und,
	/* 314 �   */	sym_und,
	/* 315 �   */	sym_und,
	/* 316 �   */	sym_und,
	/* 317 �   */	sym_und,
	/* 320 �   */	sym_und,
	/* 321 �   */	sym_und,
	/* 322 �   */	sym_und,
	/* 323 �   */	sym_und,
	/* 324 �   */	sym_und,
	/* 325 �   */	sym_und,
	/* 326 �   */	sym_und,
	/* 327 �   */	sym_und,
	/* 330 �   */	sym_und,
	/* 331 �   */	sym_und,
	/* 332 �   */	sym_und,
	/* 333 �   */	sym_und,
	/* 334 �   */	sym_und,
	/* 335 �   */	sym_und,
	/* 336 �   */	sym_und,
	/* 337 �   */	sym_und,
	/* 340 �   */	sym_und,
	/* 341 �   */	sym_und,
	/* 342 �   */	sym_und,
	/* 343 �   */	sym_und,
	/* 344 �   */	sym_und,
	/* 345 �   */	sym_und,
	/* 346 �   */	sym_und,
	/* 347 �   */	sym_und,
	/* 350 �   */	sym_und,
	/* 351 �   */	sym_und,
	/* 352 �   */	sym_und,
	/* 353 �   */	sym_und,
	/* 354 �   */	sym_und,
	/* 355 �   */	sym_und,
	/* 356 �   */	sym_und,
	/* 357 �   */	sym_und,
	/* 360 �   */	sym_und,
	/* 361 �   */	sym_und,
	/* 362 �   */	sym_und,
	/* 363 �   */	sym_und,
	/* 364 �   */	sym_und,
	/* 365 �   */	sym_und,
	/* 366 �   */	sym_und,
	/* 367 �   */	sym_und,
	/* 370 �   */	sym_und,
	/* 371 �   */	sym_und,
	/* 372 �   */	sym_und,
	/* 373 �   */	sym_und,
	/* 374 �   */	sym_und,
	/* 375 �   */	sym_und,
	/* 376 �   */	sym_und,
	/* 377 �   */	sym_und,
};

static inline int readch(state s) {
	int nextc, currc = 0;
	if (s->nextc) {
		currc = s->nextc;
		s->nextc = 0;
		return currc;
	}
	if (s->file) {
		if ((currc = fgetc(s->file)) == -1) {
			return 0;
		}
		while (currc == '\\') {
			switch (nextc = fgetc(s->file)) {
				default   : {
					s->nextc = nextc;
					return currc;
				} break;
				case   -1 : {
					//~ raiseerror(s, 5, s->line, "no-newline at end of file");
					return currc;
				} break;
				case '\n' : {
					if ((currc = fgetc(s->file)) == -1) {
						raiseerror(s, 5, s->line, "backslash-newline at end of file");
						return 0;
					}
					s->line += 1;
				} break;
			}
		}
	}
	else if (s->buff) {
		if ((currc = *s->buff) == 0) {
			return 0;
		}
		else s->buff += 1;
		while (currc == '\\') {
			switch (nextc = *s->buff) {
				default   : {
					s->buff += 1;
					s->nextc = nextc;
					return currc;
				} break;
				case    0 : {
					raiseerror(s, 5, s->line, "backslash-newline at end of file");
					return currc;
				} break;
				case '\n' : {
					s->buff += 1;
					if ((currc = *s->buff) == 0) {
						raiseerror(s, 5, s->line, "backslash-newline at end of file");
						return 0;
					}
					else s->buff += 1;
					s->line += 1;
				} break;
			}
		}//*/
	}
	//~ else {
	//~ }
	if (currc == '\n') {
		s->line += 1;
	}
	//~ printf(s->buff);
	return currc;
}

static int getchr(state s) {
	return s->curchr = readch(s);
}

static int nextch(state s) {
	return s->nextc = readch(s);
}

//~ ----------------------------------------------------------------------------
static int gethex(char chr) {
	if (chr >= '0' && chr <= '9') return chr - '0';
	else if (chr >= 'A' && chr <= 'F') return chr - 'A' + 10;
	else if (chr >= 'a' && chr <= 'f') return chr - 'a' + 10;
	return -1;
}

static int getoct(char chr) {
	if (chr >= '0' && chr <= '7') return chr - '0';
	return -1;
}

static int bslash(state s) {
	if (s->curchr != '\\') return -1;
	switch (getchr(s)) {
		case  'a' : getchr(s); return 7;
		case  'b' : getchr(s); return '\b';
		case  'f' : getchr(s); return '\f';
		case  'n' : getchr(s); return '\n';
		case  'r' : getchr(s); return '\r';
		case  't' : getchr(s); return '\t';
		case  'v' : getchr(s); return '\v';
		case '\'' : getchr(s); return '\'';
		case '\"' : getchr(s); return '\"';
		case '\\' : getchr(s); return '\\';
		case '\?' : getchr(s); return '\?';
		// oct sequence (max len = 3)
		case '0' :
		case '1' :
		case '2' :
		case '3' :
		case '4' :
		case '5' :
		case '6' :
		case '7' : {
			int tmp, val = getoct(s->curchr);
			if ((tmp = getoct(getchr(s))) >= 0) {
				val = (val << 3) | tmp;
				if ((tmp = getoct(getchr(s))) >= 0) {
					val = (val << 3) | tmp;
					getchr(s);
				}
			}
			if (val & 0xffffff00)
				raiseerror(s, 4, s->line, "oct escape sequence overflow");
			return val & 0xff;
		}
		// hex sequence (max len = 2)
		case 'x' : {
			int tmp, val = 0;
			if ((tmp = gethex(getchr(s))) >= 0) {
				val = (val << 4) | tmp;
				if ((tmp = gethex(getchr(s))) >= 0) {
					val = (val << 4) | tmp;
					getchr(s);
				}
			} else raiseerror(s, -1, s->line, "hex escape sequence invalid");
			return val & 0xff;
		}
		default : raiseerror(s, -1, s->line, "unknown escape sequence");
	}
	return 0;
}

//~ ----------------------------------------------------------------------------
static int getval(state s, node tok) {
	register char *ptr = s->buffp;
	if (s->curchr == '\'') {			// chr constant
		int val = 0;
		while (getchr(s)) {
			if (s->curchr == '\\') {
				*ptr = bslash(s);
				if (ptr - s->buffer <= bufmaxlen) {
					val = (val << 8) | *ptr;
					ptr += 1;
				}
			}
			if (s->curchr == '\'') {
				break;
			}
			else if (s->curchr == '\n') {
				break;
			}
			else {
				*ptr = s->curchr;
				if (ptr - s->buffer <= bufmaxlen) {
					val = val << 8 | *ptr++;
				}
			}
		}
		s->buffer[bufmaxlen] = *ptr = 0;
		if (s->curchr == '\'') {
			if (ptr - s->buffer > bufmaxlen)
				raiseerror(s, -1, s->line, "buffer overrun");
			else if (ptr == s->buffp)
				raiseerror(s, -1, s->line, "empty character constant");
			else if (ptr > s->buffp + sizeof(int))
				raiseerror(s, 1, s->line, "character constant truncated");
			else if (ptr > s->buffp + sizeof(char))
				raiseerror(s, 5, s->line, "multi character constant");
			getchr(s);
			tok->cint = val;
			tok->type = type_int;
			return tok->kind = tok_val;
		}
		if (s->curchr == 0 || s->curchr == '\n')
			raiseerror(s, -1, s->line, "unclosed character constant ('\'' missing)");
		return tok->kind = tok_err;
	}
	else if (s->curchr == '\"') {		// str constant
		//~ char* cstr = s->buffp;
		register unsigned hs = 0xffffffff;
		while (getchr(s)) {
			if (s->curchr == '\\') {
				*ptr = bslash(s);
				if (ptr - s->buffer <= bufmaxlen) {
					hs = (hs >> 8) ^ crc_tab[(hs ^ *ptr++) & 0xff];
					//~ hs ^= *ptr++;
				}
			}
			if (s->curchr == '\"') {
				break;
			}
			else if (s->curchr == '\n') {
				break;
			}
			else if (ptr - s->buffer <= bufmaxlen) {
				*ptr = s->curchr;
				hs = (hs >> 8) ^ crc_tab[(hs ^ *ptr++) & 0xff];
				//~ hs ^= *ptr++ = s->curchr;
			}
		}
		if (s->curchr != '\"') {		// error
			raiseerror(s, -1, s->line, "unclosed string constant ('\"' missing)");
			return tok->kind = tok_err;
		}
		if (ptr - s->buffer == bufmaxlen) {
			raiseerror(s, 1, s->line, "string constant truncated");
			s->buffer[bufmaxlen] = 0;
		}
		getchr(s);
		*ptr = 0;
		hs ^= 0xffffffff;
		//~ tok->cstr = lookupstr(s, s->buffp, ptr - s->buffp, hs);
		//~ if (tok->cstr == cstr)
		tok->type = install(s, type_str, lookupstr(s, s->buffp, ptr - s->buffp, hs), ptr - s->buffp, tok_val);
		//~ tok->type = type_str;
		return tok->kind = tok_val;
	}
	else {								// num constant
		double flt = 0;
		int val = 0, exp = 0;
		char *suf;
		enum {
			get_any = 0x0000,		// val_int
			get_oct = 0x0001,		// val_int
			get_bin = 0x0002,		// val_int
			get_hex = 0x0004,		// val_int
			get_val = 0x0008,		// read [0..9]

			got_dot = 0x0010,		// '.' taken
			got_exp = 0x0020,		// 'E' taken
			neg_exp = 0x0040,		// +/- taken
			not_oct = 0x0080,		// 8/9 taken

			oct_err = 0x0081,		// get_oct | not_oct
			got_err = 0x0200,		// error
			val_ovf = 0x0400,		// overflov
		} ;
		int get_type = get_any;
		if (s->curchr == '0') {
			get_type = get_oct;
			*ptr++ = s->curchr; getchr(s);
			if (s->curchr == 'b' || s->curchr == 'B') {
				get_type = get_bin | get_val;
				*ptr++ = s->curchr; getchr(s);
			}
			else if (s->curchr == 'x' || s->curchr == 'X') {
				get_type = get_hex | get_val;
				*ptr++ = s->curchr; getchr(s);
			}
		}
		while (s->curchr) {
			if (get_type & (get_bin | get_hex)) {
				if (get_type & get_bin) {
					if (s->curchr >= '0' && s->curchr <= '1') {
						if (val & 0x80000000) get_type |= val_ovf;
						val = (val << 1) | (s->curchr - '0');
						get_type &= ~get_val;
					}
					else break;
				}
				else if (s->curchr >= '0' && s->curchr <= '9') {
					if (val & 0xF0000000) get_type |= val_ovf;
					val = (val << 4) | (s->curchr - '0');
					get_type &= ~get_val;
				}
				else if (s->curchr >= 'a' && s->curchr <= 'f') {
					if (val & 0xF0000000) get_type |= val_ovf;
					val = (val << 4) | (s->curchr - 'a' + 10);
					get_type &= ~get_val;
				}
				else if (s->curchr >= 'A' && s->curchr <= 'F') {
					if (val & 0xF0000000) get_type |= val_ovf;
					val = (val << 4) | (s->curchr - 'A' + 10);
					get_type &= ~get_val;
				}
				else break;
			}
			else if (s->curchr == '.') {
				if (get_type & (get_val | got_dot | got_exp)) {
					get_type |= got_err;
					break;
				}
				val = 0;
				get_type |= got_dot;
			}
			else if (s->curchr == 'e' || s->curchr == 'E') {
				if ((get_type & (get_val | got_exp))) {
					get_type |= got_err;
					break;
				}
				if (!(get_type & got_dot)) val = 0;
				get_type |= got_exp;
				get_type |= get_val;
				switch (nextch(s)) {
					case '-' : get_type |= neg_exp;
					case '+' : *ptr++ = s->curchr; getchr(s);
				}
			}
			else if (s->curchr >= '0' && s->curchr <= '9') {
				if (get_type & got_exp) {
					exp = exp * 10 + s->curchr - '0';
				}
				else {
					if (get_type & got_dot) val -= 1;
					else {
						if (get_type & get_oct) {
							if (s->curchr >= '0' && s->curchr <= '7') {
								if (val & 0xE0000000) get_type |= val_ovf;
								val = (val << 3) + (s->curchr - '0');
							}
							else get_type |= not_oct;
						}
						else {
							if (val & 0xF0000000) get_type |= val_ovf;		//???
							val = val * 10 + s->curchr - '0';
						}
					}
					flt = flt * 10 + s->curchr - '0';
				}
				get_type &= ~get_val;
			}
			else break;
			*ptr++ = s->curchr;
			getchr(s);
		}
		suf = ptr;		// sufix on number ?
		//~ *ptr = 0; printf("[%c]\t%08x\t%s\n", s->curchr, get_type, s->tokstr);
		while (s->curchr) {
			if (s->curchr == '.' || s->curchr == '_') ;
			else if (s->curchr >= '0' && s->curchr <= '9') ;
			else if (s->curchr >= 'a' && s->curchr <= 'z') ;
			else if (s->curchr >= 'A' && s->curchr <= 'Z') ;
			else break;
			get_type |= got_err;
			*ptr++ = s->curchr;
			getchr(s);
		}
		*ptr = 0;
		if (*suf) {			// error
			raiseerror(s, -1, s->line, "invalid suffix in numeric constant '%s'", suf);
			return tok->kind = tok_err;
		}
		if (get_type & (get_val | got_err)) {			// error
			raiseerror(s, -1, s->line, "parse error in numeric constant \"%s\"", s->buffp);
			return tok->kind = tok_err;
		}
		if (get_type & (got_dot | got_exp)) {			// double
			tok->cflt = flt * pow(10, (get_type & neg_exp ? -exp : exp) + val);
			tok->type = type_flt;
			return tok->kind = tok_val;
		}
		else {											// integer
			if ((get_type & oct_err) == oct_err) {
				raiseerror(s, -1, s->line, "parse error in octal constant \"%s\"", s->buffp);
				return tok->kind = tok_err;
			}
			else if (!(get_type & (get_hex|get_oct|get_bin)) && (val = (long)flt) < 0) get_type |= val_ovf;

			if (get_type & val_ovf) {
				raiseerror(s, 1, s->line, "integer constant truncated \"%s\"", s->buffp);
			}
			else if (val < 0) {
				raiseerror(s, 2, s->line, "integer constant overflow \"%s\"", s->buffp);
			}
			tok->cint = val;
			tok->type = type_int;
			return tok->kind = tok_val;
		}
	}
	return tok->kind = tok_err;
}

static int getidf(state s, node tok) {
	register unsigned hs = 0xffffffff;
	register char *ptr = s->buffp;
	for( ; ; ) {
		if (s->curchr >= '0' && s->curchr <= '9') {
		} else if (tok_map[s->curchr] == tok_idf) {
		} else break;
		if (ptr - s->buffer <= bufmaxlen) {
			*ptr = s->curchr;
			hs = (hs >> 8) ^ crc_tab[(hs ^ *ptr++) & 0xff];
		}
		getchr(s);
	}
	*ptr = 0;
	hs ^= 0xffffffff;
	tok->kind = tok_idf;
	tok->name = lookupstr(s, s->buffp, ptr - s->buffp, hs);
	tok->type = lookupdef(s, tok->name, s->level, hs);
	return tok_idf;
}

static int getopr(state s, node tok) {
	char *ptr = s->buffp;
	switch (s->curchr) {
		case ',' : {
			*ptr++ = ',';
			tok->kind = opr_com;
			getchr(s);
		} break;
		case '~' : {
			*ptr++ = '~';
			tok->kind = opr_not;
			getchr(s);
		} break;
		case '=' : {
			*ptr++ = '=';
			getchr(s);
			if (s->curchr == '=') {
				*ptr++ = '=';
				tok->kind = opr_equ;
				getchr(s);
			}
			else tok->kind = ass_equ;
		} break;
		case '!' : {
			*ptr++ = '!';
			getchr(s);
			if (s->curchr == '=') {
				*ptr++ = '=';
				tok->kind = opr_neq;
				getchr(s);
			}
			else tok->kind = opr_neg;
		} break;
		case '>' : {
			*ptr++ = '>';
			getchr(s);
			if (s->curchr == '>') {
				*ptr++ = '>';
				getchr(s);
				if (s->curchr == '=') {
					*ptr++ = '=';
					tok->kind = ass_shr;
					getchr(s);
				}
				else tok->kind = opr_shr;
			}
			else if (s->curchr == '=') {
				*ptr++ = '=';
				tok->kind = opr_geq;
				getchr(s);
			}
			else tok->kind = opr_gte;
		} break;
		case '<' : {
			*ptr++ = '<';
			getchr(s);
			if (s->curchr == '<') {
				*ptr++ = '<';
				getchr(s);
				if (s->curchr == '=') {
					*ptr++ = '=';
					tok->kind = ass_shl;
					getchr(s);
				}
				else tok->kind = opr_shl;
			}
			else if (s->curchr == '=') {
				*ptr++ = '=';
				tok->kind = opr_leq;
				getchr(s);
			}
			else tok->kind = opr_lte;
		} break;
		case '&' : {
			*ptr++ = '&';
			getchr(s);
			if(s->curchr == '=') {
				*ptr++ = '=';
				tok->kind = ass_and;
				getchr(s);
			}
			/*else if (s->curchr == '&') {
				*ptr++ = '&';
				tok->kind ^= opr_lnd;
				getchr(s);
			}*/
			else tok->kind = opr_and;
		} break;
		case '|' : {
			*ptr++ = '|';
			getchr(s);
			if(s->curchr == '=') {
				*ptr++ = '=';
				tok->kind = ass_or;
				getchr(s);
			}
			/*else if (s->curchr == '|') {
				*ptr++ = '|';
				tok->kind ^= opr_lor;
				getchr(s);
			}*/
			else tok->kind = opr_or;
		} break;
		case '^' : {
			*ptr++ = '^';
			getchr(s);
			if (s->curchr == '=') {
				*ptr++ = '=';
				tok->kind = ass_xor;
				getchr(s);
			}
			else tok->kind = opr_xor;
		} break;
		case '+' : {
			*ptr++ = '+';
			getchr(s);
			if (s->curchr == '=') {
				*ptr++ = '=';
				tok->kind = ass_add;
				getchr(s);
			}
			/*else if (s->curchr == '+') {
				*ptr++ = '+';
				tok->kind = opr_inc;
				getchr(s);
			}*/
			else tok->kind = opr_add;
		} break;
		case '-' : {
			*ptr++ = '-';
			getchr(s);
			if (s->curchr == '=') {
				*ptr++ = '=';
				tok->kind = ass_sub;
				getchr(s);
			}
			/*else if (s->curchr == '-') {
				*ptr++ = '-';
				tok->kind = opr_dec;
				getchr(s);
			}*/
			else tok->kind = opr_sub;
		} break;
		case '*' : {
			*ptr++ = '*';
			getchr(s);
			if (s->curchr == '=') {
				*ptr++ = '=';
				tok->kind = ass_mul;
				getchr(s);
			}
			else tok->kind = opr_mul;
		} break;
		case '/' : {
			*ptr++ = '/';
			getchr(s);
			if (s->curchr == '=') {
				*ptr++ = '=';
				tok->kind = ass_div;
				getchr(s);
			}
			else tok->kind = opr_div;
		} break;
		case '%' : {
			*ptr++ = '%';
			getchr(s);
			if (s->curchr == '=') {
				*ptr++ = '=';
				tok->kind = ass_mod;
				getchr(s);
			}
			else tok->kind = opr_mod;
		} break;
	}
	*ptr = 0;
	return tok->kind;
}

node next_token(state s) {
	node tmp;
	if (s->nextt) {
		tmp = s->nextt;
		s->nextt = 0;
		return tmp;
	}
	if ((s->buffp + 65536) > (s->buffer + bufmaxlen)) {
		raiseerror(s, -15, 0, "innternal error Memory overrun");
		return 0;
	}
	while (s->curchr) {			// skip white spaces AND comments
		if (tok_map[s->curchr] == sym_spc) {			// skip white spaces
			getchr(s);
			continue;
		}
		if (s->curchr == '/' && nextch(s) == '/') {		// skip line comments
			while (getchr(s)) {
				if (s->curchr == '\n') break;
			}
			getchr(s);
			continue;
		}
		if (s->curchr == '/' && nextch(s) == '*') {		// skip block comments
			int curl = s->line;
			while (getchr(s)) {
				//~ if (s->curchr == '/' && nextch(s) == '*') {
					//~ raiseerror(s, 5, s->line, "\"/*\" within comment");
				//~ }
				if (s->curchr == '*' && nextch(s) == '/') break;
			}
			if (s->curchr == 0) {
				raiseerror(s, -5, curl, "unterminated comment");
				return 0;
			}
			getchr(s);		// read '/'
			getchr(s);		// get next char
			continue;
		}
		else break;
	}
	if (!s->curchr) return 0;
	tmp = (node)s->buffp;
	memset(tmp,0,sizeof(struct token_t));
	s->buffp += sizeof(struct token_t);
	tmp->line = s->line;
	//~ tmp->kind = tok_err;
	switch (tok_map[s->curchr]) {
		case tok_val : getval(s, tmp); return tmp;
		case tok_opr : getopr(s, tmp); return tmp;
		case tok_idf : getidf(s, tmp); return tmp;
		case sym_dot : {	// tok_val OR sym_dot ?
			if (tok_map[nextch(s)] == tok_val) {
				getval(s, tmp);
				return tmp;
			}
		}
		default : {			// symbol
			tmp->kind = tok_map[s->curchr];
			s->buffp[0] = s->curchr;
			s->buffp[1] = 0;
			getchr(s);
			return tmp;
		}
	}
	return 0;
}

node back_token(state s, node ref) {
	if (s->nextt) return NULL;
	s->nextt = ref;
	return ref;
}

node eat_token(state s, node ref) {
	node tok = (node)(s->buffp - sizeof(struct token_t));
	if (!s->nextt && ref == tok) {
		s->buffp = (char*)tok;
		return tok;
	}
	return NULL;
}

node new_token(state s) {
	node tmp = (node)s->buffp;
	memset(tmp,0,sizeof(struct token_t));
	s->buffp += sizeof(struct token_t);
	tmp->line = s->line;
	tmp->kind = 0;//tok_und;
	return tmp;
}
//~ ----------------------------------------------------------------------------

int enterblock(state s, symbol t, node f, node test, node cont) {
	if (!t && f) s->levelb[s->level].wait += 1;
	s->level += 1;
	s->levelb[s->level].wait = 0;
	s->levelb[s->level].link = t;
	s->levelb[s->level].fork = f;
	s->levelb[s->level].test = test;
	s->levelb[s->level].incr = cont;
	return s->level;
}

int leaveblock(state s) {					// !!!
	//~ s->levelb[s->level].link = 0;
	//~ s->levelb[s->level].fork = 0;
	//~ s->levelb[s->level].test = 0;
	//~ s->levelb[s->level].incr = 0;
	uninst(s, s->level);
	return s->level -= 1;
}

int allign(int offs, int pack) {
	switch (pack) {
		default : return offs;
		case  1 : return offs;
		case  2 : return (offs +  1) & (~1);
		case  4 : return (offs +  3) & (~3);
		case  8 : return (offs +  7) & (~7);
		case 16 : return (offs + 15) & (~15);
		case 32 : return (offs + 31) & (~31);
	}
}

//~ ----------------------------------------------------------------------------
void cc_init(state s, const char * str) {
	s->buffp = s->buffer;
	s->buff = (char*)str;
	s->nextc = 0;
	s->nextt = 0;
	s->level = 0;
	s->line = 1;

	memset(s->strt, 0, sizeof(s->strt));
	memset(s->deft, 0, sizeof(s->deft));
	stmt_if 	= install(s, 0, "if",		0, idf_kwd);
	stmt_for	= install(s, 0, "for",		0, idf_kwd);
	stmt_fork	= install(s, 0, "fork",		0, idf_kwd);
				  //~ install(s, 0, "break",	0, idf_kwd);
				  //~ install(s, 0, "continue",0, idf_kwd);
	//~ stmt_ret	= install(s, 0, "return",	0, idf_kwd);

				  //~ install(s, 0, "define",	0, 0);

				  install(s, 0, "const",	0, idf_dcl | idm_con);
				  install(s, 0, "static",	0, idf_dcl | idm_sta);
				  install(s, 0, "volatile",	0, idf_dcl | idm_vol);
				  //~ install(s, 0, "inline",	0, idf_dcl | 0x10);
				  //~ install(s, 0, "extern",	0, idf_dcl | 0x20);

	oper_sof	= install(s, 0, "sizeof",	0, 0);
				  //~ install(s, 0, "struct",	0, idf_dcl);
				  //~ install(s, 0, "union",	0, idf_dcl);

	type_str	= install(s, 0, "cstr",			0, idf_dcl | idm_con);
	type_void	= install(s, 0, "void",		0, idf_dcl);
	oper_out	= install(s, type_void, "print",	0, idf_ref);
				  //~ install(s, 0, "char",		1, idf_dcl);
				  //~ install(s, 0, "short",	2, idf_dcl);
				  //~ install(s, 0, "long",		8, idf_dcl);

	type_int	= install(s, 0, "int",		4, idf_dcl);
	type_flt	= install(s, 0, "float",	4, idf_dcl);
				  //~ install(s, 0, "double",	8, tok_dcl);
	getchr(s);
}

int cc_open(state s, const char * str) {
	s->file = fopen(s->name = str, "r");
	if (!s->file) return -1;
	cc_init(s, NULL);
	return 0;
}

int cc_done(state s) {
	if (!s->file) return -1;
	fclose(s->file);
	return s->errc;
}

int raiseerror(state s, int level, int line, char* msg, ...) {
	/**
	 * level < 0 : error
	 * level > 0 : warning
	 * line > 0  : error in file/line
	 * error ==0 : ???
	 * line == 0 : ???
	**/
	void **argp = (void **)&msg;
	//~ char *name = "scan.c";
	char *name = (char*)(s->name ? s->name : "temp.c");
	FILE *file = stderr;
	if (level < 0) s->errc += 1;
	if (line && level) {
		fprintf(file, "%s:%d: %s: ", name, line, level < 0 ? "error" : "warning");
	}
	else if (level) {
		//~ fprintf(file, "%s:%s: ", name, level < 0 ? "error" : "warning");
	}
	else if (line) {
		//~ fprintf(file, "%s:%d: ", name);
	}
	else {
	}
	while (*msg) {
		if (*msg == '%') {
			switch (*(msg+=1)) {
				case '%' : msg += 1; argp += 0; fputc('%', file); break;
				case 'x' : msg += 1; argp += 1; fprintf(file, "%08x", *(int*)argp); break;
				case 'd' : msg += 1; argp += 1; fprintf(file, "%d", *(int*)argp); break;
				case 'c' : msg += 1; argp += 1; fprintf(file, "%c", *(char*)argp); break;
				case 's' : msg += 1; argp += 1; fprintf(file, "%s", *(char**)argp); break;
			}
		}
		else {
			fputc(*msg, file);
			msg += 1;
		}
	}
	fputc('\n', file);
	return 0;
}

//{--8<-------------------------------------------------------------------------\-
void print_token(node ref, int fmt) {
	if (!ref) {
		printf("null");
		if (fmt) printf("\n");
		return;
	}
	//~ if ((ref->kind == opr_com) && fmt) return;
	if (fmt & 2) {
		switch (ref->kind & tok_msk) {
			default		 : printf("(?)"); break;
			case tok_val : printf("val"); break;
			case tok_idf : printf("idf"); break;
			case tok_opr : printf("opr"); break;
			case tok_sym : printf("sym"); break;
		}
		printf(" [%08x] ", ref->kind);
	}
	//~ if (fmt & 4 && ref->type) printf("(typename %s) ", ref->type->name);
	switch (ref->kind & tok_msk) {
		case tok_val : {
			if (ref->type == type_int) printf("%d", ref->cint);
			else if (ref->type == type_flt) printf("%f", ref->cflt);
			else if (ref->type && ref->type->type == type_str) printf("\"%s\"", ref->type->name);
		} break;
		case tok_idf : {
			printf("%s", ref->name);
			//~ switch (ref->kind & 0xff00) {
				//~ case tok_def : printf("def %s", ref->name); break;
				//~ case tok_kwd : printf("kwd %s", ref->name); break;
				//~ case tok_dcl : printf("dcl %s", ref->name); break;
				//~ case tok_var : printf("var %s", ref->name); break;
			//~ }
		} break;
		case tok_opr : {
			switch (ref->kind) {
				default : printf("'?'"); break;
				case opr_nop : {
					printf("l%d:", ref->line);
				} break;
				case opr_jmp : {
					if (ref->type == stmt_fork) {
							printf("fork");
					}
					else {
						if (ref->olhs)
							printf("goto if false");
						else
							printf("goto");
					}
				} break;
				//~ case opr_out : printf("cout << "); break;
				case opr_tpc : printf("(%s)", ref->type->name); break;
				case opr_mul : printf("*"); break;
				case opr_div : printf("/"); break;
				case opr_mod : printf("%%"); break;

				case opr_add : printf("+"); break;
				case opr_sub : printf("-"); break;

				case opr_shr : printf(">>"); break;
				case opr_shl : printf("<<"); break;

				case opr_gte : printf(">"); break;
				case opr_geq : printf(">="); break;
				case opr_lte : printf("<"); break;
				case opr_leq : printf("<="); break;

				case opr_neq : printf("!="); break;
				case opr_equ : printf("=="); break;

				case opr_and : printf("&"); break;
				case opr_xor : printf("^"); break;
				case opr_or  : printf("|"); break;

				case opr_lnd : printf("&&"); break;
				case opr_lor : printf("||"); break;

				case ass_equ : printf("="); break;

				case ass_mul : printf("*="); break;
				case ass_div : printf("/="); break;
				case ass_mod : printf("%%="); break;
				case ass_add : printf("+="); break;
				case ass_sub : printf("-="); break;
				case ass_shr : printf(">>="); break;
				case ass_shl : printf("<<="); break;
				case ass_and : printf("&="); break;
				case ass_xor : printf("^="); break;
				case ass_or  : printf("|="); break;

				case opr_ite : printf("?"); break;
				case opr_els : printf(":"); break;

				case opr_unm : printf("-"); break;
				case opr_unp : printf("+"); break;
				case opr_neg : printf("!"); break;
				case opr_not : printf("~"); break;
				case opr_idx : printf("[]"); break;
				case opr_dot : printf("."); break;
				//~ case opr_ptr : printf("*"); break;
				case opr_fnc : printf("%s()", ref->name); break;
				//~ case opr_def : printf("%s<>", ref->name); break;

				case opr_com : printf(","); break;
			}
		} break;
		default : break;
	}
	if (fmt) printf("\n");
	fflush(stdout);
}

void print_strt(state s) {
	int i;
	printf("strt : table\n");
	for (i = 0; i < MAX_TBLS; i+=1) {
		struct llist_t *list = s->strt[i];
		if (list) {
			printf("%3d\t",i);
			while (list) {
				char *name = (char*)list->data;
				printf("%s ", name);
				list = list->next;
			}
			printf("\n");
		}
	}
}

//}--8<------------------------------------------------------------------------*/

/*--8<--------------------------------------------------------------------------

struct state_t s;
node t;

//~ #include "type.c"

int main() {
	char exp[]  = "'' di 0378 0xff 0b11111111c 255 1243879878 0xffffffff 0. .0 0.0 1. .1 0.33 0e33 1e33 1e-33 3.14e-100 \"hello vilag\" '\\\"' 'Q' '\\x51' '\\121' 'WBMP' 'xx\\x4B'\n";
	//~ char exp[]  = "\"3d\" \"4f\" \"ks\" \"ks\" \"kw45ys\" \"k3ys\" \"k34uys\" \"kerghs\" \"kerths\" \"ke6h53s\" \"k35hs\" \"k356h3s\" \"krebyts\" \"kertherts\" \"kertherts\" \"kserth\" \n";
	//~ char exp[]  = "int a[10], b[100] = (1,2,3,4,5);, c[20*sin(.5)]=88, d(int a,b,c);\n";
	//~ char exp[]  = "int a(int) = 0, f;\n";
	//~ puts(exp); fflush(stdout);
	cc_init(&s, exp); while ((t = next_token(&s))) print_token(t,4+2+1);
	//~ initstate(&s, exp); printf("\n%d\n", s.buffp - s.buffer);
	//~ printf("%d\n", sizeof(struct state_t));
	printf("%d\n", sizeof(struct token_t));
	//~ printf("%d\n", sizeof(struct llist_t));
	//~ print_strt(&s);
	//~ printf("\n%d\n", s.buffp - s.buffer);
	print_defn(&s);
	//~ int i; for (i=0; i<256; i+=1) printf("\t/%c %03o %c   %c/\tsym_und,\n", '*',i ,i , '*');
	return 0;
}

//--8<------------------------------------------------------------------------*/
