/*******************************************************************************
 *   File: parse.c
 *   Date: 2011/06/23
 *   Desc: lexer and parser
 *******************************************************************************

Lexical elements

	Comments:
		line comments: //
		block comments: / * ... * / and nestable /+ ... +/

	Tokens:
		Identifiers: variable or type names.

			identifier = (letter)+

		Keywords:
			break,
			const,
			continue,
			define,
			else,
			emit,
			enum,
			for,
			if,
			module,
			operator,
			parallel,
			return,
			static,
			struct

		Operators and Delimiters:
			+ - * / % . ,
			~ & | ^ >> <<
			&& ||
			! == != < <= > >=
			= := += -= *= /= %= &= |= ^= >>= <<=
			( ) [ ] { } ? : ;

		Integer and Floating-point literals:
			bin_lit = '0'[bB][01]+
			oct_lit = '0'[oO][0-7]+
			hex_lit = '0'[xX][0-9a-fA-F]+
			decimal_lit = [1-9][0-9]*
			floating_lit = decimal_lit (('.'[0-9]*) | )([eE]([+-]?)[0-9]+)

		Character and String literals:
			char_lit = \'[^\'\n]*
			string_lit = \"[^\"\n]*
*/

#if !(defined _MSC_VER)
#include <unistd.h>
#else
#include <io.h>
#endif

#include <string.h>
#include <fcntl.h>
#include <math.h>
#include "core.h"

//{~~~~~~~~~ Input ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

unsigned rehash(const char* str, unsigned len) {
	static unsigned const crc_tab[256] = {
		0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
		0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
		0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
		0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
		0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
		0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
		0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
		0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
		0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
		0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
		0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
		0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
		0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
		0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
		0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
		0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
		0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
		0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
		0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
		0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
		0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
		0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
		0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
		0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
		0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
		0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
		0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
		0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
		0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
		0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
		0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
		0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
		0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
		0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
		0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
		0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
		0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
		0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
		0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
		0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
		0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
		0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
		0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
		0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
		0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
		0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
		0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
		0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
		0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
		0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
		0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
		0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
		0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
		0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
		0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
		0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
		0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
		0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
		0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
		0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
		0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
		0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
		0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
		0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
	};

	register unsigned hs = 0xffffffff;

	if (str) {
		if (len == -1U)
			len = strlen(str) + 1;
		while (len-- > 0)
			hs = (hs >> 8) ^ crc_tab[(hs ^ (*str++)) & 0xff];
	}

	return hs ^ 0xffffffff;
}

static int fillBuf(ccState s) {
	if (s->fin._fin >= 0) {
		unsigned char* base = s->fin._buf + s->fin._cnt;
		int l, size = sizeof(s->fin._buf) - s->fin._cnt;
		memcpy(s->fin._buf, s->fin._ptr, s->fin._cnt);
		s->fin._cnt += l = read(s->fin._fin, base, size);
		if (l == 0) {
			s->fin._buf[s->fin._cnt] = 0;
			close(s->fin._fin);
			s->fin._fin = -1;
		}
		s->fin._ptr = (char*)s->fin._buf;
	}
	return s->fin._cnt;
}

static int readChr(ccState s) {
	if (s->_chr != -1) {
		int chr = s->_chr;
		s->_chr = -1;
		return chr;
	}
	if (s->fin._cnt <= 4 && fillBuf(s) <= 0) {
		return -1;
	}
	/* removed: backslash newline: ('\' 'newline')
	while (*s->fin._ptr == '\\' && (s->fin._ptr[1] == '\n' || s->fin._ptr[1] == '\r')) {
		if (s->fin._ptr[1] == '\r' && s->fin._ptr[2] == '\n') {
			s->fin._cnt -= 3;
			s->fin._ptr += 3;
		} else {
			s->fin._ptr += 2;
			s->fin._cnt -= 2;
		}
		if (s->fin._cnt <= 4 && !fillBuf(s)) {
			warn(s->s, 9, s->file, s->line, "backslash-newline at end of file");
			return -1;
		}
		s->line += s->line != 0;
	}*/
	// threat 'cr', 'lf' and 'cr + lf' as lf
	if (*s->fin._ptr == '\r' || *s->fin._ptr == '\n') {
		if (s->fin._ptr[0] == '\r' && s->fin._ptr[1] == '\n')
			--s->fin._cnt, s->fin._ptr++;
		--s->fin._cnt, s->fin._ptr++;
		s->line += s->line != 0;
		return '\n';
	}
	return *(--s->fin._cnt, s->fin._ptr++);
}

static int peekChr(ccState s) {
	if (s->_chr == -1)
		s->_chr = readChr(s);
	return s->_chr;
}

static int skipChr(ccState s, int chr) {
	if (!chr || chr == peekChr(s))
		return readChr(s);
	return 0;
}

static int backChr(ccState s, int chr) {
	dieif(s->_chr != -1, "can not put back more than one character");
	return s->_chr = chr;
}

int source(ccState s, srcType mode, char* file) {
	if (s->fin._fin > 3)
		close(s->fin._fin);
	s->fin._fin = -1;
	s->fin._ptr = 0;
	s->fin._cnt = 0;
	s->_chr = -1;
	s->_tok = 0;
	//~ s->file = 0;
	s->line = 0;
	//~ s->nest = 0;

	if (mode & srcFile) {
		if ((s->fin._fin = open(file, O_RDONLY)) <= 0)
			return -1;
		s->file = mapstr(s, file, -1, -1);
		s->line = 1;
		if (fillBuf(s) > 2) {
			if (s->fin._ptr[0] == '#' && s->fin._ptr[1] == '!') {
				int chr = readChr(s);
				while (chr != -1) {
					if (chr == '\n')
						break;
					chr = readChr(s);
				}
			}
		}
	}
	else if (file) {
		s->fin._ptr = file;
		s->fin._cnt = strlen(file);
	}
	return 0;
}

//}

//{~~~~~~~~~ Lexer ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

char *mapstr(ccState s, char *name, unsigned size/* = -1U*/, unsigned hash/* = -1U*/) {

	list newn, next, prev = 0;

	if (size == -1U) {
		//~ debug("strlen '%s'", name);
		size = strlen(name) + 1;
	}

	if (hash == -1U) {
		//~ debug("strcrc '%s'", name);
		hash = rehash(name, size) % TBLS;
	}

	dieif (name[size-1], "FixMe: %s[%d]", name, size);
	for (next = s->strt[hash]; next; next = next->next) {
		//~ int slen = next->data - (unsigned char*)next;
		register int c = memcmp(next->data, name, size);
		if (c == 0) return (char*)next->data;
		if (c > 0) break;
		prev = next;
	}

	dieif(s->_end - s->_beg <= (int)(sizeof(struct list) + size + 1), "memory overrun");

	if (name != s->_beg) {
		debug("lookupstr(%s)", name);
		memcpy(s->_beg, name, size);
		name = s->_beg;
	}

	s->_beg += size;

	newn = (list)s->_beg;
	s->_beg += sizeof(struct list);

	//~ s->_end -= sizeof(struct list);
	//~ newn = (list)s->_end;

	if (!prev) s->strt[hash] = newn;
	else prev->next = newn;

	newn->next = next;
	newn->data = (unsigned char *)name;

	return name;
}

static int readTok(ccState s, astn tok) {
	enum {
		OTHER = 0x00000000,

		SPACE = 0x00000100,
		UNDER = 0x00000200,
		PUNCT = 0x00000400,
		CNTRL = 0x00000800,

		DIGIT = 0x00001000,
		LOWER = 0x00002000,
		UPPER = 0x00004000,
		SCTRL = CNTRL|SPACE,
		ALPHA = UPPER|LOWER,
		ALNUM = DIGIT|ALPHA,
		CWORD = UNDER|ALNUM,
		//~ GRAPH = (PUNCT|ALNUM),
		//~ ASCII = 0x0000FF00,
	};
	static unsigned const chr_map[256] = {
		/* 000 nul */	CNTRL,
		/* 001 soh */	CNTRL,
		/* 002 stx */	CNTRL,
		/* 003 etx */	CNTRL,
		/* 004 eot */	CNTRL,
		/* 005 enq */	CNTRL,
		/* 006 ack */	CNTRL,
		/* 007 bel */	CNTRL,
		/* 010 bs  */	CNTRL,
		/* 011 ht  */	SCTRL,	// horz tab
		/* 012 nl  */	SCTRL,	// \n
		/* 013 vt  */	SCTRL,	// Vert Tab,
		/* 014 ff  */	SCTRL,	// Form Feed,
		/* 015 cr  */	SCTRL,	// \r
		/* 016 so  */	CNTRL,
		/* 017 si  */	CNTRL,
		/* 020 dle */	CNTRL,
		/* 021 dc1 */	CNTRL,
		/* 022 dc2 */	CNTRL,
		/* 023 dc3 */	CNTRL,
		/* 024 dc4 */	CNTRL,
		/* 025 nak */	CNTRL,
		/* 026 syn */	CNTRL,
		/* 027 etb */	CNTRL,
		/* 030 can */	CNTRL,
		/* 031 em  */	CNTRL,
		/* 032 sub */	CNTRL,
		/* 033 esc */	CNTRL,
		/* 034 fs  */	CNTRL,
		/* 035 gs  */	CNTRL,
		/* 036 rs  */	CNTRL,
		/* 037 us  */	CNTRL,
		/* 040 sp  */	SPACE,	// space
		/* 041 !   */	PUNCT,
		/* 042 "   */	PUNCT,
		/* 043 #   */	PUNCT,
		/* 044 $   */	PUNCT,
		/* 045 %   */	PUNCT,
		/* 046 &   */	PUNCT,
		/* 047 '   */	PUNCT,
		/* 050 (   */	PUNCT,
		/* 051 )   */	PUNCT,
		/* 052 *   */	PUNCT,
		/* 053 +   */	PUNCT,
		/* 054 ,   */	PUNCT,
		/* 055 -   */	PUNCT,
		/* 056 .   */	PUNCT,
		/* 057 /   */	PUNCT,
		/* 060 0   */	DIGIT,
		/* 061 1   */	DIGIT,
		/* 062 2   */	DIGIT,
		/* 063 3   */	DIGIT,
		/* 064 4   */	DIGIT,
		/* 065 5   */	DIGIT,
		/* 066 6   */	DIGIT,
		/* 067 7   */	DIGIT,
		/* 070 8   */	DIGIT,
		/* 071 9   */	DIGIT,
		/* 072 :   */	PUNCT,
		/* 073 ;   */	PUNCT,
		/* 074 <   */	PUNCT,
		/* 075 =   */	PUNCT,
		/* 076 >   */	PUNCT,
		/* 077 ?   */	PUNCT,
		/* 100 @   */	PUNCT,
		/* 101 A   */	UPPER,
		/* 102 B   */	UPPER,
		/* 103 C   */	UPPER,
		/* 104 D   */	UPPER,
		/* 105 E   */	UPPER,
		/* 106 F   */	UPPER,
		/* 107 G   */	UPPER,
		/* 110 H   */	UPPER,
		/* 111 I   */	UPPER,
		/* 112 J   */	UPPER,
		/* 113 K   */	UPPER,
		/* 114 L   */	UPPER,
		/* 115 M   */	UPPER,
		/* 116 N   */	UPPER,
		/* 117 O   */	UPPER,
		/* 120 P   */	UPPER,
		/* 121 Q   */	UPPER,
		/* 122 R   */	UPPER,
		/* 123 S   */	UPPER,
		/* 124 T   */	UPPER,
		/* 125 U   */	UPPER,
		/* 126 V   */	UPPER,
		/* 127 W   */	UPPER,
		/* 130 X   */	UPPER,
		/* 131 Y   */	UPPER,
		/* 132 Z   */	UPPER,
		/* 133 [   */	PUNCT,
		/* 134 \   */	PUNCT,
		/* 135 ]   */	PUNCT,
		/* 136 ^   */	PUNCT,
		/* 137 _   */	UNDER,
		/* 140 `   */	PUNCT,
		/* 141 a   */	LOWER,
		/* 142 b   */	LOWER,
		/* 143 c   */	LOWER,
		/* 144 d   */	LOWER,
		/* 145 e   */	LOWER,
		/* 146 f   */	LOWER,
		/* 147 g   */	LOWER,
		/* 150 h   */	LOWER,
		/* 151 i   */	LOWER,
		/* 152 j   */	LOWER,
		/* 153 k   */	LOWER,
		/* 154 l   */	LOWER,
		/* 155 m   */	LOWER,
		/* 156 n   */	LOWER,
		/* 157 o   */	LOWER,
		/* 160 p   */	LOWER,
		/* 161 q   */	LOWER,
		/* 162 r   */	LOWER,
		/* 163 s   */	LOWER,
		/* 164 t   */	LOWER,
		/* 165 u   */	LOWER,
		/* 166 v   */	LOWER,
		/* 167 w   */	LOWER,
		/* 170 x   */	LOWER,
		/* 171 y   */	LOWER,
		/* 172 z   */	LOWER,
		/* 173 {   */	PUNCT,
		/* 174 |   */	PUNCT,
		/* 175 }   */	PUNCT,
		/* 176 ~   */	PUNCT,
		/* 177    */	CNTRL,		// del
		/* 200     */	OTHER,
	};
	int chr = readChr(s);

	char *end = s->_end;
	char *beg = 0, *ptr = 0;

	// skip white spaces and comments
	while (chr != -1) {
		if (chr == '/') {
			int line = s->line;
			int next = peekChr(s);
			if (skipChr(s, '/')) {
				int fmt = skipChr(s, '%');

				chr = readChr(s);
				ptr = beg = s->_beg;
				while (ptr < end && chr != -1) {
					if (chr == '\n')
						break;

					*ptr++ = chr;
					chr = readChr(s);
				}
				if (chr == -1)
					warn(s->s, 9, s->file, line, "no newline at end of file");
				else if (s->line != line + 1)
					warn(s->s, 9, s->file, line, "multi-line comment");
				if (fmt && s->pfmt && s->pfmt->line == line) {
					*ptr++ = 0;
					s->pfmt->pfmt = mapstr(s, beg, ptr - beg, -1);
				}
			}
			else if (next == '*') {
				int level = 0;

				//~ ptr = beg = s->_beg;
				while (chr != -1) {

					int prev_chr = chr;
					chr = readChr(s);
					//~ *ptr++ = chr;

					if (prev_chr == '/' && chr == '*') {
						level += 1;
						chr = 0;	// disable /+/
						if (level > 1)
							warn(s->s, 9, s->file, s->line, "\"/*\" within comment");
					}

					if (prev_chr == '*' && chr == '/') {
						level -= 1;
						//~ if (!level)
						break;
					}

				}

				if (chr == -1)
					error(s->s, s->file, line, "unterminated comment");

				/*if (s->pfmt && strncmp(beg, "**\n", 3) == 0) {
					ptr[-2] = 0;
					info(s->s, s->pfmt->file, s->pfmt->line, "%-T\n%s", s->pfmt, beg + 3);
				}*/

				chr = readChr(s);
			}
			else if (next == '+') {
				int level = 0;

				while (chr != -1) {

					int prev_chr = chr;
					chr = readChr(s);

					if (prev_chr == '/' && chr == '+') {
						level += 1;
						chr = 0;	// disable /+/
					}

					if (prev_chr == '+' && chr == '/') {
						level -= 1;
						if (!level)
							break;
					}

				}

				if (chr == -1)
					error(s->s, s->file, line, "unterminated comment1");

				chr = readChr(s);
			}
			s->pfmt = NULL;
		}
		if (chr == 0) {
			warn(s->s, 5, s->file, s->line, "null character(s) ignored");
			while (!chr) chr = readChr(s);
		}

		if (!(chr_map[chr & 0xff] & SPACE)) break;
		chr = readChr(s);
	}

	// here begins our token
	memset(tok, 0, sizeof(*tok));
	tok->file = s->file;
	tok->line = s->line;
	ptr = beg = s->_beg;
	s->pfmt = NULL;

	// scan
	if (chr != -1) switch (chr) {
		default: {
			if (chr_map[chr & 0xff] & (ALPHA|UNDER))
				goto read_idf;

			if (chr_map[chr & 0xff] & DIGIT)
				goto read_num;

			// control(<32) or non ascii character(>127) ?
			error(s->s, s->file, s->line, "Invalid character: '%c'", chr);
			tok->kind = TOKN_err;
		} break;
		case '.': {
			if (chr_map[peekChr(s) & 0xff] == DIGIT) goto read_num;
			tok->kind = OPER_dot;
		} break;
		case ';': {
			tok->kind = STMT_do;
		} break;
		case ',': {
			tok->kind = OPER_com;
		} break;
		case '{': {		// beg?
			tok->kind = STMT_beg;
		} break;
		case '[': {		// idx?
			tok->kind = PNCT_lc;
		} break;
		case '(': {		// fnc?
			tok->kind = PNCT_lp;
		} break;
		case '}': {		// end?
			tok->kind = STMT_end;
		} break;
		case ']': {
			tok->kind = PNCT_rc;
		} break;
		case ')': {
			tok->kind = PNCT_rp;
		} break;
		case '?': {		// sel?+
			tok->kind = PNCT_qst;
		} break;
		case ':': {
			if (peekChr(s) == '=') {
				readChr(s);
				tok->kind = ASGN_set;
			}
			else tok->kind = PNCT_cln;
		} break;
		case '~': {
			tok->kind = OPER_cmt;
		} break;
		case '=': {
			if (peekChr(s) == '=') {
				readChr(s);
				tok->kind = OPER_equ;
			}
			else tok->kind = ASGN_set;
		} break;
		case '!': {
			if (peekChr(s) == '=') {
				readChr(s);
				tok->kind = OPER_neq;
			}
			else tok->kind = OPER_not;
		} break;
		case '>': {
			chr = peekChr(s);
			if (chr == '>') {
				readChr(s);
				if (peekChr(s) == '=') {
					readChr(s);
					tok->kind = ASGN_shr;
				}
				else tok->kind = OPER_shr;
			}
			else if (chr == '=') {
				readChr(s);
				tok->kind = OPER_geq;
			}
			else tok->kind = OPER_gte;
		} break;
		case '<': {
			chr = peekChr(s);
			if (chr == '<') {
				readChr(s);
				if (peekChr(s) == '=') {
					readChr(s);
					tok->kind = ASGN_shl;
				}
				else tok->kind = OPER_shl;
			}
			else if (chr == '=') {
				readChr(s);
				tok->kind = OPER_leq;
			}
			else tok->kind = OPER_lte;
		} break;
		case '&': {
			chr = peekChr(s);
			if (chr == '=') {
				readChr(s);
				tok->kind = ASGN_and;
			}
			else if (chr == '&') {
				readChr(s);
				tok->kind = OPER_lnd;
			}
			else tok->kind = OPER_and;
		} break;
		case '|': {
			chr = peekChr(s);
			if (chr == '=') {
				readChr(s);
				tok->kind = ASGN_ior;
			}
			else if (chr == '|') {
				readChr(s);
				tok->kind = OPER_lor;
			}
			else tok->kind = OPER_ior;
		} break;
		case '^': {
			if (peekChr(s) == '=') {
				readChr(s);
				tok->kind = ASGN_xor;
			}
			else tok->kind = OPER_xor;
		} break;
		case '+': {
			int chr = peekChr(s);
			if (chr == '=') {
				readChr(s);
				tok->kind = ASGN_add;
			}
			/*else if (chr == '+') {
				readChr(s);
				tok->kind = OPER_inc;
			}*/
			else tok->kind = OPER_add;
		} break;
		case '-': {
			chr = peekChr(s);
			if (chr == '=') {
				readChr(s);
				tok->kind = ASGN_sub;
			}
			/*else if (chr == '-') {
				readChr(s);
				tok->kind = OPER_dec;
			}*/
			else tok->kind = OPER_sub;
		} break;
		case '*': {
			chr = peekChr(s);
			if (chr == '=') {
				readChr(s);
				tok->kind = ASGN_mul;
			}
			/* else if (chr == '*') {
				readChr(s);
				if (peekChr(s) == '=') {
					readChr(s);
					tok->kind = ASGN_pow;
				}
				else tok->kind = OPER_pow;
			} */
			else tok->kind = OPER_mul;
		} break;
		case '/': {
			if (peekChr(s) == '=') {
				readChr(s);
				tok->kind = ASGN_div;
			}
			else tok->kind = OPER_div;
		} break;
		case '%': {
			if (peekChr(s) == '=') {
				readChr(s);
				tok->kind = ASGN_mod;
			}
			else tok->kind = OPER_mod;
		} break;
		case '\'':			// \'[^\'\n]*
		case '"': {			// \"[^\"\n]*
			int lit = chr;
			while (ptr < end && (chr = readChr(s)) != -1) {

				if (chr == lit)
					break;

				if (chr == '\n')
					break;

				// escape
				if (chr == '\\') switch (chr = readChr(s)) {
					case  'a': chr = '\a'; break;
					case  'b': chr = '\b'; break;
					case  'f': chr = '\f'; break;
					case  'n': chr = '\n'; break;
					case  'r': chr = '\r'; break;
					case  't': chr = '\t'; break;
					case  'v': chr = '\v'; break;
					case '\'': chr = '\''; break;
					case '\"': chr = '\"'; break;
					case '\\': chr = '\\'; break;
					case '\?': chr = '\?'; break;

					// oct sequence (max len = 3)
					case '0': case '1':
					case '2': case '3':
					case '4': case '5':
					case '6': case '7': {
						int val = chr - '0';
						if ((chr = peekChr(s)) >= '0' && chr <= '7') {
							val = (val << 3) | (chr - '0');
							readChr(s);
							if ((chr = peekChr(s)) >= '0' && chr <= '7') {
								val = (val << 3) | (chr - '0');
								readChr(s);
							}
						}
						if (val & 0xffffff00)
							warn(s->s, 4, s->file, s->line, "octal escape sequence overflow");
						chr = val & 0xff;
					} break;
					

					// hex sequence (len = 2)
					case 'x': {
						int h1 = readChr(s);
						int h2 = readChr(s);
						if (h1 >= '0' && h1 <= '9') h1 -= '0';
						else if (h1 >= 'a' && h1 <= 'f') h1 -= 'a' - 10;
						else if (h1 >= 'A' && h1 <= 'F') h1 -= 'A' - 10;
						else {
							error(s->s, s->file, s->line, "hex escape sequence invalid");
							break;
						}

						if (h2 >= '0' && h2 <= '9') h2 -= '0';
						else if (h2 >= 'a' && h2 <= 'f') h2 -= 'a' - 10;
						else if (h2 >= 'A' && h2 <= 'F') h2 -= 'A' - 10;
						else {
							error(s->s, s->file, s->line, "hex escape sequence invalid");
							break;
						}
						chr = h1 << 4 | h2;
					} break;

					default:
						error(s->s, s->file, s->line, "invalid escape sequence '\\%c'", chr);
						chr = 0;
						break;
				}

				*ptr++ = chr;
			}
			*ptr++ = 0;

			if (chr != lit) {
				error(s->s, s->file, s->line, "unclosed %s literal", lit == '"' ? "string" : "character");
				return tok->kind = TOKN_err;
			}

			if (lit == '"') {
				tok->kind = TYPE_str;
				tok->ref.hash = rehash(beg, ptr - beg) % TBLS;
				tok->ref.name = mapstr(s, beg, ptr - beg, tok->ref.hash);
			}
			else {
				int val = 0;

				if (ptr == beg) {
					error(s->s, s->file, s->line, "empty character constant");
					return tok->kind = TOKN_err;
				}

				TODO("size of char is type_chr->size, not sizeof(char)")

				if (ptr > beg + sizeof(val) + 1)
					warn(s->s, 1, s->file, s->line, "multi character constant truncated");

				else if (ptr > beg + sizeof(char) + 1)
					warn(s->s, 5, s->file, s->line, "multi character constant");

				for (ptr = beg; *ptr; ++ptr) {
					val = val << 8 | *ptr;
				}

				tok->kind = TYPE_int;
				tok->con.cint = val;
			}
		} break;
		read_idf: {			// [a-zA-Z_][a-zA-Z0-9_]*
			static struct {
				char *name;
				int type;
			}
			keywords[] = {
				// Warning: sort keywords (SciTE can do it)
				//~ {"module", UNIT_kwd},
				//~ {"operator", OPER_def},
				{"break", STMT_brk},
				{"const", QUAL_con},
				{"continue", STMT_con},
				{"define", TYPE_def},
				{"else", STMT_els},
				{"emit", EMIT_opc},
				{"enum", ENUM_kwd},
				{"for", STMT_for},
				{"if", STMT_if},
				{"parallel", QUAL_par},
				{"return", STMT_ret},
				{"static", QUAL_sta},
				{"struct", TYPE_rec}
			};

			int lo = 0;
			int hi = sizeof(keywords) / sizeof(*keywords);
			int cmp = -1;

			while (ptr < end && chr != -1) {
				if (!(chr_map[chr & 0xff] & CWORD))
					break;
				*ptr++ = chr;
				chr = readChr(s);
			}
			backChr(s, chr);
			*ptr++ = 0;

			// binary search the keyword
			while (cmp != 0 && lo < hi) {
				int mid = lo + ((hi - lo) / 2);
				cmp = strcmp(keywords[mid].name, beg);
				if (cmp < 0)
					lo = mid + 1;
				else
					hi = mid;
			}
			if (cmp == 0) {
				tok->kind = keywords[hi].type;
				//~ debug("kwd: %k, %s", tok, beg);
			}
			else {
				tok->kind = TYPE_ref;
				tok->type = tok->ref.link = 0;
				tok->ref.hash = rehash(beg, ptr - beg) % TBLS;
				tok->ref.name = mapstr(s, beg, ptr - beg, tok->ref.hash);
				//~ debug("idf: %k", tok);
			}
		} break;
		read_num: {			// int | ([0-9]+'.'[0-9]* | '.'[0-9]+)([eE][+-]?[0-9]+)?
			int flt = 0;			// floating
			int ovrf = 0;			// overflow
			int radix = 10;
			int64_t i64v = 0;
			float64_t f64v = 0;
			char *suffix = 0;

			//~ 0[.oObBxX]?
			if (chr == '0') {
				*ptr++ = chr;
				chr = readChr(s);

				switch (chr) {

					default:
						radix = 8;
						break;

					case '.':		// do not skip
						break;

					case 'b':
					case 'B':
						radix = 2;
						*ptr++ = chr;
						chr = readChr(s);
						break;

					case 'o':
					case 'O':
						radix = 8;
						*ptr++ = chr;
						chr = readChr(s);
						break;

					case 'x':
					case 'X':
						radix = 16;
						*ptr++ = chr;
						chr = readChr(s);
						break;
				}
			}

			//~ ([0-9a-fA-F])*
			while (chr != -1) {
				int value = chr;
				if (value >= '0' && value <= '9')
					value -= '0';
				else if (value >= 'a' && value <= 'f')
					value -= 'a' - 10;
				else if (value >= 'A' && value <= 'F')
					value -= 'A' - 10;
				else break;

				if (value > radix)
					break;

				if (i64v > (int64_t)(0x7fffffffffffffffULL / 10)) {
					ovrf = radix == 10;
				}

				i64v *= radix;
				i64v += value;

				if (i64v < 0 && radix == 10) {
					ovrf = 1;
				}

				*ptr++ = chr;
				chr = readChr(s);
			}

			if (ovrf)
				warn(s->s, 4, s->file, s->line, "value overflow");

			//~ ('.'[0-9]*)? ([eE]([+-]?)[0-9]+)?
			if (radix == 10) {

				f64v = (float64_t)i64v;

				if (chr == '.') {
					long double val = 0;
					long double exp = 1;

					*ptr++ = chr;
					chr = readChr(s);

					while (chr >= '0' && chr <= '9') {
						val = val * 10 + (chr - '0');
						exp *= 10;

						*ptr++ = chr;
						chr = readChr(s);
					}

					f64v += val / exp;
					flt = 1;
				}

				if (chr == 'e' || chr == 'E') {
					long double p = 1, m = 10;
					unsigned int val = 0;
					int sgn = 1;

					*ptr++ = chr;
					chr = readChr(s);

					switch (chr) {
						case '-':
							sgn = -1;
						case '+':
							*ptr++ = chr;
							chr = readChr(s);
					}

					ovrf = 0;
					suffix = ptr;
					while (chr >= '0' && chr <= '9') {
						if (val > 0x7fffffff / 10) {
							ovrf = 1;
						}
						val = val * 10 + (chr - '0');
						if ((signed)val < 0) {
							ovrf = 1;
						}

						*ptr++ = chr;
						chr = readChr(s);
					}

					if (suffix == ptr) {
						error(s->s, tok->file, tok->line, "no exponent in numeric constant");
					}
					else if (ovrf || val > 1024) {
						warn(s->s, 4, s->file, s->line, "exponent overflow");
					}

					while (val) {		// pow(10, val);
						if (val & 1)
							p *= m;
						val >>= 1;
						m *= m;
					}

					if (sgn < 0)
						f64v /= p;
					else
						f64v *= p;
					flt = 1;
				}// */
			}

			suffix = ptr;
			while (ptr < end && chr != -1) {
				if (!(chr_map[chr & 0xff] & CWORD))
					break;
				*ptr++ = chr;
				chr = readChr(s);
			}
			backChr(s, chr);
			*ptr++ = 0;

			/*while (chr != -1) {
				if (chr == '.' || chr == '_') {}
				else if (chr >= '0' && chr <= '9') {}
				else if (chr >= 'a' && chr <= 'z') {}
				else if (chr >= 'A' && chr <= 'Z') {}
				else {*ptr = 0; break;}
				*ptr++ = chr;
				chr = readChr(s);
			}*/
			//~ backChr(s, chr);

			if (*suffix) {	// error
				if (suffix[0] == 'f' && suffix[1] == 0) {
					tok->cst2 = TYPE_f32;
					flt = 1;
				}
				else {
					error(s->s, tok->file, tok->line, "invalid suffix in numeric constant '%s'", suffix);
					tok->kind = TYPE_any;
				}
			}
			if (flt) {		// float
				tok->kind = TYPE_flt;
				tok->con.cflt = f64v;
			}
			else {			// integer
				tok->kind = TYPE_int;
				tok->con.cint = i64v;
			}
		} break;
	}
	dieif(ptr >= end, "mem overrun %t", tok->kind);
	return tok->kind;
}

astn peekTok(ccState s, int kind) {
	//~ dieif(!s->_cnt, "FixMe: invalid ccState");
	if (s->_tok == 0) {
		s->_tok = newnode(s, 0);
		if (!readTok(s, s->_tok)) {
			eatnode(s, s->_tok);
			s->_tok = 0;
		}
	}

	if (!kind || !s->_tok)
		return s->_tok;

	if (s->_tok->kind == kind)
		return s->_tok;

	return 0;
}

TODO(this should be removed)
astn peek(ccState s) {return peekTok(s, 0);}

// static
astn next(ccState s, int kind) {
	if (peekTok(s, kind)) {
		astn ast = s->_tok;
		s->_tok = ast->next;
		ast->next = 0;
		return ast;
	}
	return 0;
}

static void backTok(ccState s, astn ast) {
	//~ dieif(!s->_cnt, "FixMe: invalid ccState");
	ast->next = s->_tok;
	s->_tok = ast;
}

static int test(ccState s, int kind) {
	astn ast = peek(s);
	if (!ast || (kind && ast->kind != kind))
		return 0;
	return ast->kind;
}// */
int skip(ccState s, int kind) {
	astn ast = peekTok(s, 0);
	if (!ast || (kind && ast->kind != kind))
		return 0;
	s->_tok = ast->next;
	eatnode(s, ast);
	return 1;
}

static int skiptok(ccState s, int kind, int raise) {
	if (!skip(s, kind)) {
		if (raise) {
			error(s->s, s->file, s->line, "`%t` excepted, got `%k`", kind, peek(s));
			//~ error(s->s, s->file, s->line, "`%t` excepted", kind);
		}

		switch (kind) {
			case STMT_do:
			case STMT_end:
			case PNCT_rp:
			case PNCT_rc:
				break;

			default:
				return 0;//!peek(s);
		}
		while (!skip(s, kind)) {
			//~ if (!peek(s)) return 0;
			//~ debug("skipping('%k')", peek(s));
			if (skip(s, STMT_do)) return 0;
			if (skip(s, STMT_end)) return 0;
			if (skip(s, PNCT_rp)) return 0;
			if (skip(s, PNCT_rc)) return 0;
			if (!skip(s, 0)) return 0;		// eof?
		}
		return 0;
	}
	return kind;
}

//}

//{~~~~~~~~~ Parser ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// if lhs is null return (rhs) else return (lhs, rhs)
static inline astn argnode(ccState s, astn lhs, astn rhs) {
	return lhs ? opnode(s, OPER_com, lhs, rhs) : rhs;
}

TODO("it does not work as expected, these should go to type.c")
static void redefine(ccState s, symn sym) {
	symn ptr;

	if (!sym)
		return;

	for (ptr = sym->next; ptr; ptr = ptr->next) {
		symn arg1 = ptr->args;
		symn arg2 = sym->args;

		if (!ptr->name)
			continue;

		if (sym->call != ptr->call)
			continue;

		if (strcmp(sym->name, ptr->name) != 0)
			continue;

		/*/ if not redefineable
		if (ptr->cnst)
			break;
		// */

		while (arg1 && arg2) {
			if (arg1->type != arg2->type)
				break;
			if (arg1->call != arg2->call)
				break;
			arg1 = arg1->next;
			arg2 = arg2->next;
		}

		if (arg1 || arg2)
			continue;

		break;
	}

	if (ptr && (ptr->nest >= sym->nest/*  || ptr->read */)) {
		error(s->s, sym->file, sym->line, "redefinition of `%-T`", sym);
		if (ptr->file && ptr->line)
			info(s->s, ptr->file, ptr->line, "first defined here as `%-T`", ptr);
	}
}

/*static symn copysym(ccState s, symn sym) {
	symn result = newdefn(s, TYPE_ref);
	if (result != NULL) {
		result->type = sym->type;
		result->args = sym->args;
		result->cast = sym->cast;
		result->call = sym->call;
		result->size = sym->size;
		result->offs = sym->offs;
		result->next = NULL;
	}
	return result;
}*/
static symn ctorArg(ccState s, symn rec) {
	symn ctor = install(s, rec->name, TYPE_def, 0, 0, rec, NULL);
	if (ctor != NULL) {
		astn root = NULL;
		symn arg, newarg;

		enter(s, NULL);

		for (arg = rec->args; arg; arg = arg->next) {

			if (arg->stat)
				break;

			if (arg->kind != TYPE_ref)
				continue;

			newarg = install(s, arg->name, TYPE_ref, 0, 0, arg->type, NULL);

			if (newarg) {
				astn tag;
				//~ newarg->type = arg->type;
				newarg->call = arg->call;
				newarg->args = arg->args;
				newarg->cast = TYPE_def;

				tag = lnknode(s, newarg);
				tag->cst2 = arg->cast;
				root = argnode(s, root, tag);
			}
		}

		ctor->args = leave(s, ctor, 0);
		ctor->kind = TYPE_def;
		//~ ctor->type = rec;
		ctor->call = 1;
		ctor->cnst = 1;	// returns constant (params must be constants too)

		ctor->init = opnode(s, OPER_fnc, s->emit_tag, root);
		ctor->init->type = rec;

		//~ debug("constructor created: %-T ", ctor);
	}
	return ctor;
}
static symn ctorPtr(ccState s, symn rec) {
	symn ctor = NULL;//install(s, rec->name, TYPE_def, TYPE_ref, 0, rec);
	if (ctor != NULL) {
		astn argtag = NULL;
		symn rettyp = rec;//copysym(s, rec);
		symn argptr = newdefn(s, TYPE_ref);

		if (rettyp && argptr) {
			//~ rettyp->cast = TYPE_ref;

			argptr->name = ".ptr";
			argptr->next = NULL;
			argptr->type = type_ptr;
			argptr->cast = TYPE_def;

			argtag = lnknode(s, argptr);
			argtag->cst2 = TYPE_ref;
		}

		ctor->args = argptr;
		ctor->kind = TYPE_def;
		ctor->type = rec;
		ctor->cast = TYPE_ref;
		ctor->call = 1;

		ctor->init = opnode(s, OPER_fnc, s->emit_tag, argtag);
		ctor->init->type = ctor->type = rettyp;

		//~ debug("constructor created: %-T ", ctor);
	}
	return ctor;
}

static int mkConst(astn ast, int cast) {
	struct astn tmp;

	dieif(!ast, "FixMe");

	if (!eval(&tmp, ast))
		return 0;

	ast->kind = tmp.kind;
	ast->con = tmp.con;

	switch (ast->cst2 = cast) {
		default: fatal("FixMe");

		case TYPE_vid:
		case TYPE_any:
			break;

		case TYPE_bit:
			ast->con.cint = constbol(ast);
			ast->kind = TYPE_int;
			break;

		case TYPE_u32:
		case TYPE_i32:
		case TYPE_i64:
		//~ case TYPE_int:
			ast->con.cint = constint(ast);
			ast->kind = TYPE_int;
			break;

		case TYPE_f32:
		case TYPE_f64:
		//~ case TYPE_flt:
			ast->con.cflt = constflt(ast);
			ast->kind = TYPE_flt;
			break;
	}

	return ast->kind;
}

//~ astn unit(ccState, int mode);		// parse unit			(mode: script/unit)
static astn stmt(ccState, int mode);	// parse statement		(mode: enable decl)
//~ astn decl(ccState, int mode);		// parse declaration	(mode: enable spec)
//~ astn spec(ccState s/* , int qual */);
//~ astn type(ccState s/* , int qual */);

static astn args(ccState s, int mode);

TODO("eliminate this function, use instead expr, then check if it is a type")
static astn type(ccState s/* , int mode */) {	// type(.type)* ('&' | '[]')mode?
	symn def = NULL;
	astn tok, list = NULL;
	while ((tok = next(s, TYPE_ref))) {

		symn loc = def ? def->args : s->deft[tok->ref.hash];
		symn sym = lookup(s, loc, tok, NULL, 0);

		tok->next = list;
		list = tok;

		def = sym;

		if (!isType(sym)) {
			def = NULL;
			break;
		}

		if ((tok = next(s, OPER_dot))) {
			tok->next = list;
			list = tok;
		}
		else
			break;

	}
	if (!def && list) {
		while (list) {
			astn back = list;
			list = list->next;
			backTok(s, back);
			//~ trace("not a type `%k`", back);
		}
		list = NULL;
	}
	else if (list) {
		list->type = def;
	}
	//info(s->s, root->file, root->line, "`%+k` is a type", root);
	/*if (list && mode && skip(s, OPER_and)) {
		//~ list = newsy
	}*/
	return list;
}

static astn decl_init(ccState s, symn var) {
	if (skip(s, ASGN_set)) {
		symn typ = var->type;
		int cast = var->cast;
		int mkcon = var->cnst;

		var->init = expr(s, TYPE_def);

		if (var->init->type == emit_opc) {		// just in case of  = emit(val, ...)
			var->init->type = var->type;
			var->init->cst2 = cast;
			if (mkcon) {
				error(s->s, var->file, var->line, "invalid constant initialization `%+k`", var->init);
			}
		}
		if (canAssign(var, var->init, cast == TYPE_ref)) {
			//~ trace("canAssign(%-T, %+k)", typ, var->init);
			if (!castTo(var->init, cast)) {
				trace("%t", cast);
				return NULL;
			}
			if (mkcon && !mkConst(var->init, cast)) {
				if (!isConst(var->init))
					warn(s->s, 1, var->file, var->line, "probably non constant initialization of `%-T`", var);
				//~ trace("%t", cast);
				//~ return NULL;
			}
		}
		// try initialize an array with elements
		else if (typ->kind == TYPE_arr) {
			astn init = var->init;
			symn base = typ;
			int nelem = 1;

			// TODO: base should not be stored in var->args !!!
			base = var->args;
			cast = base ? base->cast : 0;

			while (init->kind == OPER_com) {
				astn val = init->op.rhso;
				if (!canAssign(base, val, 0)) {
					trace("canAssignCast(%-T, %+k, %t)", base, val, cast);
					return NULL;
				}
				if (!castTo(val, cast)) {
					trace("canAssignCast(%-T, %+k, %t)", base, val, cast);
					return NULL;
				}
				if (mkcon && !mkConst(val, cast)) {
					trace("canAssignCast(%-T, %+k, %t)", base, val, cast);
					return NULL;
				}
				init = init->op.lhso;
				nelem += 1;
			}

			if (!canAssign(base, init, 0)) {
				trace("canAssignCast(%-T, %+k, %t)", base, init, cast);
				return NULL;
			}
			if (!castTo(init, cast)) {
				trace("canAssignCast(%-T, %+k, %t)", base, init, cast);
				return NULL;
			}
			if (mkcon && !mkConst(init, cast)) {
				trace("%t", cast);
				return NULL;
			}

			// int a[] = 1 / int a[] = 1, 2, 3, 4, 5, 6
			if (base == typ->type && typ->init == NULL) {
				typ->init = intnode(s, nelem);
				//~ typ->size = nelem;
			}
			else if (typ->size < 0) {
				error(s->s, var->file, var->line, "invalid initialization `%+k`", var->init);
			}
		}
		else {
			error(s->s, var->file, var->line, "invalid initialization `%-T` := `%+k`", var, var->init);
		}
	}
	return var->init;
}

astn decl_var(ccState s, astn *argv, int mode) {
	astn tag = NULL;
	if ((tag = type(s))) {
		symn ref = NULL;
		symn typ = tag->type;

		int argeval = 0;

		int byref = skip(s, OPER_and);

		if (!byref && mode == TYPE_def) {
			byref = argeval = skip(s, OPER_xor);
		}

		if (!(tag = next(s, TYPE_ref))) {
			debug("id expected, not %k", peek(s));
			return 0;
		}

		if (byref && typ->cast == TYPE_ref) {
			warn(s->s, 3, tag->file, tag->line, "the type `%-T` is a reference type", typ);
		}

		ref = declare(s, TYPE_ref, tag, typ);
		ref->size = byref ? vm_size : sizeOf(typ);

		if (argv) {
			*argv = NULL;
		}

		if (skip(s, PNCT_lp)) {				// int a(...)
			astn argroot;
			enter(s, tag);
			argroot = args(s, TYPE_ref);
			skiptok(s, PNCT_rp, 1);


			ref->args = leave(s, ref, 0);
			//~ ref->cast = TYPE_ref;
			ref->call = 1;

			if (ref->args == NULL) {
				argroot = s->void_tag;
				ref->args = s->void_tag->ref.link;
			}

			if (byref) {
				error(s->s, tag->file, tag->line, "declaration of `%+T` as returning a reference [TODO]", ref);
			}

			if (argv) {
				*argv = argroot;
			}

			byref = 0;
		}

		/* TODO: arrays of functions: without else
		 * int rgbCopy(int a, int b)[8] = rgbCpy, rgbAnd, ...
		 */
		else if (skip(s, PNCT_lc)) {		// int a[...]
			symn tmp = newdefn(s, TYPE_arr);
			symn base = typ;
			int dynarr = 1;		// dynamic sized array: slice

			tmp->type = typ;
			tmp->size = -1;
			typ = tmp;

			if (byref) {					// int& a[200] a contains 200 references to integers
				error(s->s, tag->file, tag->line, "declaration of `%+T` as array of references [TODO]", ref);
			}

			ref->type = typ;
			tag->type = typ;

			if (!test(s, PNCT_rc)) {
				struct astn val;
				astn init = expr(s, TYPE_def);
				typ->init = init;

				if (init != NULL) {
					astn dims = init;
					/*while (dims->kind == OPER_com) {
						//? multi dimensional array declaration: int a[1, 2, 3, 4];
						symn tmp = newdefn(s, TYPE_arr);
						tmp->type = typ->type;
						typ->type = tmp;

						if (eval(&val, dims->op.rhso) == TYPE_int) {
							addarg(s, tmp, "length", TYPE_def, type_i32, dims->op.rhso);
							tmp->size = val.con.cint;
							tmp->init = dims->op.rhso;
						}
						else {
							error(s->s, dims->file, dims->line, "integer constant expected");
						}
						dims = dims->op.lhso;
					}*/
					if (eval(&val, dims) == TYPE_int) {
						addarg(s, typ, "length", TYPE_def, type_i32, dims);
						typ->size = val.con.cint;
						typ->init = init;
						ref->cast = 0;

						dynarr = 0;
						if (typ->size < 0) {
							error(s->s, dims->file, dims->line, "positive integer constant expected, got `%+k`", dims);
						}
					}
					else/*  if (init->kind == OPER_com)  */{
						error(s->s, init->file, init->line, "integer constant expected declaring array `%T`", ref);
					}
				}
			}
			if (dynarr) {
				symn length = addarg(s, typ, "length", TYPE_ref, type_i32, NULL);
				dieif(length == NULL, "FixMe");
				length->offs = 4;

				ref->cast = TYPE_arr;
				ref->size = 8;
			}

			skiptok(s, PNCT_rc, 1);

			// Multi dimensional arrays / c style
			while (skip(s, PNCT_lc)) {
				symn tmp = newdefn(s, TYPE_arr);
				trloop("%k", peek(s));
				tmp->type = typ->type;
				typ->type = tmp;
				tmp->size = -1;
				typ = tmp;

				if (!test(s, PNCT_rc)) {
					struct astn val;
					astn init = expr(s, TYPE_def);
					if (eval(&val, init) == TYPE_int) {
						addarg(s, typ, "length", TYPE_def, type_i32, init);
						typ->size = val.con.cint;
						typ->init = init;
					}
					else {
						error(s->s, init->file, init->line, "integer constant expected");
					}
				}
				if (typ->init == NULL) {
					symn length = addarg(s, typ, "length", TYPE_ref, type_i32, NULL);
					dieif(length == NULL, "FixMe");
					length->offs = 4;

					ref->cast = TYPE_arr;
					ref->size = 8;
				}

				if (dynarr != (typ->init == NULL)) {
					error(s->s, ref->file, ref->line, "invalid mixing of dynamic sized arrays");
				}

				skiptok(s, PNCT_rc, 1);

			}

			ref->args = base;	// fixme (temporarly used)
			byref = 0;
		}

		if (byref) {
			//~ debug("variable %-T is by ref", ref);
			ref->cast = TYPE_ref;
		}

		if (ref->call) {
			tag->cst2 = TYPE_ref;
		}
		else {
			//~ addarg(s, typ, "Class", ATTR_const | TYPE_def, s->type_rec, lnknode(s, typ));
			tag->cst2 = ref->cast;
		}
	}

	return tag;
}

static int qual(ccState s, int mode) {
	astn ast;
	int result = 0;
	while ((ast = peek(s))) {
		trloop("%k", peek(s));
		switch (ast->kind) {
			case QUAL_con:
				if (!(mode & ATTR_const))
					return result;
				if (result & ATTR_const) {
					error(s->s, ast->file, ast->line, "qualifier `%t` declared more than once", ast);
				}
				result |= ATTR_const;
				skip(s, 0);
				break;
			case QUAL_sta:
				if (!(mode & ATTR_stat))
					return result;
				if (result & ATTR_stat) {
					error(s->s, ast->file, ast->line, "qualifier `%t` declared more than once", ast);
				}
				result |= ATTR_stat;
				skip(s, 0);
				break;
			default:
				//~ trace("next %k", peek(s));
				return result;
		}
	}
	//~ trace("next %t", peek(s));
	return result;
}

static astn args(ccState s, int mode) {
	astn root = NULL;

	if (test(s, PNCT_rp))
		return s->void_tag;

	while (peek(s)) {
		int attr;
		astn atag;
		trloop("%k", peek(s));
		attr = qual(s, ATTR_const);
		if ((atag = decl_var(s, NULL, mode))) {
			root = argnode(s, root, atag);
			if (attr & ATTR_const) {
				atag->ref.link->cnst = 1;
			}
		}

		if (!skip(s, OPER_com))
			break;
	}
	return root;
}

/** read a list of statements.
 * @param mode: raise error if (decl / stmt) found
 */
static astn block(ccState s, int mode) {
	astn head = NULL;
	astn tail = NULL;
	astn tmp;

	while (peek(s)) {
		int stop = 0;
		trloop("%k", peek(s));

		switch (test(s, 0)) {
			case 0 :	// end of file
			case PNCT_rp :
			case PNCT_rc :
			case STMT_end :
				stop = 1;
				break;
		}
		if (stop) {
			break;
		}

		if ((tmp = stmt(s, 0))) {
			if (tail == NULL) {
				head = tmp;
			}
			else {
				tail->next = tmp;
			}

			tail = tmp;
		}
	}

	return head;
}

static astn stmt(ccState s, int mode) {
	astn ast = NULL;
	int qual = 0;			// static | parallel

	//~ invalid start of statement
	switch (test(s, 0)) {
		case STMT_end:
		case PNCT_rp :
		case PNCT_rc :
		case 0 :	// end of file
			//~ error(s->s, s->file, s->line, "unexpected token '%k'", peek(s));
			//~ skip(s, 0);0
			return NULL;
	}

	// check statement construct
	if ((ast = next(s, QUAL_par))) {		// 'parallel' ('for' | '{')
		switch (test(s, 0)) {
			case STMT_beg:		// parallel task
			case STMT_for:		// parallel loop
				qual = QUAL_par;
				break;
			default:
				backTok(s, ast);
				break;
		}
		ast = 0;
	}
	else if ((ast = next(s, QUAL_sta))) {	// 'static' ('if' | 'for')
		switch (test(s, 0)) {
			case STMT_for:		// loop unroll
			case STMT_if:		// compile time if
				qual = QUAL_sta;
				break;
			default:
				backTok(s, ast);
				break;
		}
		ast = 0;
	}

	// scan the statement
	if (skip(s, STMT_do)) {}				// ;
	else if ((ast = next(s, STMT_beg))) {	// { ... }
		astn blk;
		int newscope = !mode;

		if (newscope)
			enter(s, ast);

		if ((blk = block(s, 0))) {
			ast->stmt.stmt = blk;
			ast->type = type_vid;
		}
		else {
			// eat sort of {{;{;};{}{}}}
			eatnode(s, ast);
			ast = 0;
		}

		skiptok(s, STMT_end, 1);

		if (newscope) {
			leave(s, NULL, 0);
		}
	}
	else if ((ast = next(s,  STMT_if))) {	// if (...)
		int siffalse = s->siff;
		int newscope = 1;

		skiptok(s, PNCT_lp, 1);
		ast->stmt.test = expr(s, TYPE_bit);
		skiptok(s, PNCT_rp, 1);

		if (qual == QUAL_sta) {
			struct astn ift;
			if (!eval(&ift, ast->stmt.test) || !test(s, STMT_beg)) {
				error(s->s, ast->file, ast->line, "invalid static if construct");
				return 0;
			}
			if (constbol(&ift))
				newscope = 0;

			s->siff |= newscope;
			/* skip syntax checking ?
			else if (skip(s, STMT_beg)) {
				int lb = 1;
				while (lb > 0) {
					if (skip(s, STMT_beg))
						lb += 1;
					else if (skip(s, STMT_end))
						lb -= 1;
					else if (!skip(s, 0))
						break;
				}
				//~ if (lb)
				s->nest += lb;
				return 0;
			} // */
		}

		if (newscope)
			enter(s, ast);

		ast->stmt.stmt = stmt(s, 1);	// no new scope / no decl

		if (skip(s, STMT_els)) {
			if (newscope) {
				leave(s, NULL, 0);
				enter(s, ast);
			}
			ast->stmt.step = stmt(s, 1);
		}

		ast->type = type_vid;
		ast->cst2 = qual;

		if (newscope) {
			leave(s, NULL, 0);
		}

		s->siff = siffalse;
	}
	else if ((ast = next(s, STMT_for))) {	// for (...)
		enter(s, ast);
		skiptok(s, PNCT_lp, 1);
		if (!skip(s, STMT_do)) {		// init or iterate
			ast->stmt.init = decl(s, decl_NoDefs|decl_Colon);

			if (!ast->stmt.init) {
				ast->stmt.init = expr(s, TYPE_vid);
				skiptok(s, STMT_do, 1);
			}
			//~ /* for (int a : range(0, 60)) {...}
			else if (skip(s, PNCT_cln)) {		// iterate
				astn itin = expr(s, TYPE_vid);

				if (itin && itin->type) {
					// auto .it = iterator(range);
					// next(.it, a);
					astn fun, arg;

					astn tok = newIden(s, "iterator");
					symn loc = s->deft[tok->ref.hash];
					symn sym = lookup(s, loc, tok, itin, 0);
					astn dcl1 = ast->stmt.init;
					astn dcl2 = newIden(s, ".it");

					if (sym != NULL && dcl2 != NULL) {
						symn x = declare(s, TYPE_ref, dcl2, sym->type);
						x->init = opnode(s, OPER_fnc, tok, itin);
						x->init->type = sym->type;
						x->size = sym->type->size;
						x->cast = TYPE_rec;

						dcl2->kind = TYPE_def;
						dcl2->file = dcl1->file;
						dcl2->line = dcl1->line;
						dcl2->next = NULL;
						dcl1->next = dcl2;
						sym = x;
					}

					ast->stmt.init = newnode(s, STMT_beg);
					ast->stmt.init->stmt.stmt = dcl1;
					ast->stmt.init->type = type_vid;
					ast->stmt.init->cst2 = TYPE_rec;

					// make the `next(it, a)` test
					arg = opnode(s, OPER_com, newIden(s, dcl2->ref.name), newIden(s, dcl1->ref.name));
					fun = opnode(s, OPER_fnc, newIden(s, "next"), arg);

					ast->stmt.test = fun;
					if (!typecheck(s, NULL, fun) || !typecheck(s, NULL, sym->init)) {
						error(s->s, ast->file, ast->line, "invalid iterator for `%+k`", itin);
						debug("%7K", ast);
						if (fun->type && fun->type != type_bol) {
							error(s->s, ast->file, ast->line, "invalid iterator, next should return boolean `%+k`", fun);
						}
					}
					fun->cst2 = TYPE_bit;
					sym->init->cst2 = TYPE_rec;
				}
				backTok(s, newnode(s, STMT_do));
			}// */
		}
		if (!skip(s, STMT_do)) {		// test
			ast->stmt.test = expr(s, TYPE_bit);
			skiptok(s, STMT_do, 1);
		}
		if (!skip(s, PNCT_rp)) {		// incr
			ast->stmt.step = expr(s, TYPE_vid);
			skiptok(s, PNCT_rp, 1);
		}

		ast->stmt.stmt = stmt(s, 1);	// no new scope & disable decl
		ast->type = type_vid;
		ast->cst2 = qual;

		leave(s, NULL, 0);
	}
	else if ((ast = next(s, STMT_brk))) {	// break;
		ast->type = type_vid;
	}
	else if ((ast = next(s, STMT_con))) {	// continue;
		ast->type = type_vid;
	}
	else if ((ast = next(s, STMT_ret))) {	// return;
		ast->type = type_vid;
	}

	else if ((ast = decl(s, TYPE_any))) {	// declaration
		astn tmp = newnode(s, STMT_do);
		symn ref = ast->ref.link;
		tmp->file = ast->file;
		tmp->line = ast->line;
		tmp->type = ast->type;
		tmp->stmt.stmt = ast;

		// constant variables are static
		if (ref && ref->cnst && ref->kind == TYPE_ref) {
			if (!ref->init) {
				error(s->s, ref->file, ref->line, "uninitialized constant `%-T`", ref);
			}
			ref->stat = 1;
		}

		if (mode)
			error(s->s, ast->file, ast->line, "unexpected declaration %+k", ast);

		ast = tmp;
	}

	else if ((ast = expr(s, TYPE_vid))) {	// expression
		astn tmp = newnode(s, STMT_do);
		tmp->file = ast->file;
		tmp->line = ast->line;
		tmp->type = type_vid;
		tmp->stmt.stmt = ast;
		skiptok(s, STMT_do, 1);
		switch (ast->kind) {
			default:
				warn(s->s, 1, ast->file, ast->line, "statement expression expected");
				break;

			case OPER_fnc:
			case ASGN_set:
				break;
		}
		ast = tmp;
	}

	else if ((ast = peek(s))) {
		error(s->s, ast->file, ast->line, "unexpected token: %k", ast);
		skiptok(s, STMT_do, 1);
	}
	else {
		error(s->s, s->file, s->line, "unexpected end of file");
	}

	//~ dieif(qual, "FixMe");
	return ast;
}

astn expr(ccState s, int mode) {
	astn buff[TOKS], *base = buff + TOKS;
	astn *post = buff, *prec = base, tok;
	char sym[TOKS];							// symbol stack {, [, (, ?
	int level = 0, unary = 1;				// precedence, top of sym , start in unary mode
	sym[level] = 0;

	while ((tok = next(s, 0))) {					// parse
		int pri = level << 4;
		trloop("%k", peek(s));
		// statements are not allowed in expressions !!!!
		if (tok->kind >= STMT_beg && tok->kind <= STMT_end) {
			backTok(s, tok);
			tok = 0;
			break;
		}// */
		switch (tok->kind) {
			tok_id: {
				*post++ = tok;
			} break;
			tok_op: {
				int oppri = tok_tbl[tok->kind].type;
				tok->op.prec = pri | (oppri & 0x0f);
				if (oppri & 0x10) while (prec < buff + TOKS) {
					if ((*prec)->op.prec <= tok->op.prec)
						break;
					*post++ = *prec++;
				}
				else while (prec < buff + TOKS) {
					if ((*prec)->op.prec < tok->op.prec)
						break;
					*post++ = *prec++;
				}
				if (tok->kind != STMT_do) {
					*--prec = tok;
				}
			} break;

			default: {
				if (tok_tbl[tok->kind].argc) {			// operator
					if (unary) {
						*post++ = 0;
						error(s->s, tok->file, tok->line, "syntax error before '%k'", tok);
					}
					unary = 1;
					goto tok_op;
				}
				else {
					if (!unary)
						error(s->s, tok->file, tok->line, "syntax error before '%k'", tok);
					unary = 0;
					goto tok_id;
				}
			} break;

			case PNCT_lp: {			// '('
				if (unary)			// a + (3*b)
					*post++ = 0;
				else if (test(s, PNCT_rp)) {	// a()
					unary = 2;
				}
				else {
					unary = 1;
				}

				tok->kind = OPER_fnc;
				sym[++level] = '(';

			} goto tok_op;
			case PNCT_rp: {			// ')'
				if (unary == 2 && sym[level] == '(') {
					tok->kind = STMT_do;
					*post++ = NULL;
					level -= 1;
				}
				else if (!unary && sym[level] == '(') {
					tok->kind = STMT_do;
					level -= 1;
				}
				else if (level == 0) {
					backTok(s, tok);
					tok = 0;
					break;
				}
				else {
					error(s->s, tok->file, tok->line, "syntax error before '%k'", tok);
					break;
				}
				unary = 0;
			} goto tok_op;
			case PNCT_lc: {			// '['
				if (!unary)
					tok->kind = OPER_idx;
				else {
					error(s->s, tok->file, tok->line, "syntax error before '%k'", tok);
					//~ break?;
				}
				sym[++level] = '[';
				unary = 1;
			} goto tok_op;
			case PNCT_rc: {			// ']'
				if (!unary && sym[level] == '[') {
					tok->kind = STMT_do;
					level -= 1;
				}
				else if (!level) {
					backTok(s, tok);
					tok = 0;
					break;
				}
				else {
					error(s->s, tok->file, tok->line, "syntax error before '%k'", tok);
					break;
				}
				unary = 0;
			} goto tok_op;
			case PNCT_qst: {		// '?'
				if (unary) {
					*post++ = 0;		// ???
					error(s->s, tok->file, tok->line, "syntax error before '%k'", tok);
				}
				tok->kind = OPER_sel;
				sym[++level] = '?';
				unary = 1;
			} goto tok_op;
			case PNCT_cln: {		// ':'
				if (!unary && sym[level] == '?') {
					tok->kind = STMT_do;
					level -= 1;
				}
				else if (level == 0) {
					backTok(s, tok);
					tok = 0;
					break;
				}
				else {
					error(s->s, tok->file, tok->line, "syntax error before '%k'", tok);
					break;
				}
				unary = 1;
			} goto tok_op;
			case OPER_add: {		// '+'
				if (unary)
					tok->kind = OPER_pls;
				unary = 1;
			} goto tok_op;
			case OPER_sub: {		// '-'
				if (unary)
					tok->kind = OPER_mns;
				unary = 1;
			} goto tok_op;
			case OPER_and: {		// '&'
				if (unary)
					tok->kind = OPER_adr;
				unary = 1;
			} goto tok_op;
			case OPER_not: {		// '!'
				if (!unary)
					error(s->s, tok->file, tok->line, "syntax error before '%k'", tok);
				unary = 1;
			} goto tok_op;
			case OPER_cmt: {		// '~'
				if (!unary)
					error(s->s, tok->file, tok->line, "syntax error before '%k'", tok);
				unary = 1;
			} goto tok_op;
			case OPER_com: {		// ','
				if (unary)
					error(s->s, tok->file, tok->line, "syntax error before '%k'", tok);
				unary = 1;
			} goto tok_op;
		}
		if (post >= prec) {
			error(s->s, s->file, s->line, "Expression too complex");
			return 0;
		}
		if (!tok)
			break;
	}
	if (unary || level) {							// error
		char *missing = "expression";
		if (level) switch (sym[level]) {
			default:
				fatal("FixMe");
				break;
			case '(': missing = "')'"; break;
			case '[': missing = "']'"; break;
			case '?': missing = "':'"; break;
		}
		error(s->s, s->file, s->line, "missing %s, %k", missing, peek(s));
	}
	else if (prec > buff) {							// build
		astn *ptr, *lhs;

		while (prec < buff + TOKS)					// flush
			*post++ = *prec++;
		*post = NULL;

		// we have the postfix form, build the tree
		for (lhs = ptr = buff; ptr < post; ptr += 1) {
			if ((tok = *ptr) == NULL) {}			// skip
			else if (tok_tbl[tok->kind].argc) {		// oper
				int argc = tok_tbl[tok->kind].argc;
				if ((lhs -= argc) < buff) {
					fatal("FixMe");
					return 0;
				}
				switch (argc) {
					default:
						fatal("FixMe");
						break;

					case 1:
						//~ tok->op.test = NULL;
						//~ tok->op.lhso = NULL;
						tok->op.rhso = lhs[0];
						break;

					case 2:
						//~ tok->op.test = NULL;
						tok->op.lhso = lhs[0];
						tok->op.rhso = lhs[1];
						break;
					case 3:
						tok->op.test = lhs[0];
						tok->op.lhso = lhs[1];
						tok->op.rhso = lhs[2];
						break;
				}
				#ifdef DEBUGGING
				switch (tok->kind) {
					default:
						break;

					case ASGN_add:
					case ASGN_sub:
					case ASGN_mul:
					case ASGN_div:
					case ASGN_mod:
					case ASGN_shl:
					case ASGN_shr:
					case ASGN_and:
					case ASGN_ior:
					case ASGN_xor: {
						astn tmp = newnode(s, 0);
						switch (tok->kind) {
							case ASGN_add: tmp->kind = OPER_add; break;
							case ASGN_sub: tmp->kind = OPER_sub; break;
							case ASGN_mul: tmp->kind = OPER_mul; break;
							case ASGN_div: tmp->kind = OPER_div; break;
							case ASGN_mod: tmp->kind = OPER_mod; break;
							case ASGN_shl: tmp->kind = OPER_shl; break;
							case ASGN_shr: tmp->kind = OPER_shr; break;
							case ASGN_and: tmp->kind = OPER_and; break;
							case ASGN_ior: tmp->kind = OPER_ior; break;
							case ASGN_xor: tmp->kind = OPER_xor; break;
							default: fatal("FixMe");
						}
						tmp->op.rhso = tok->op.rhso;
						tmp->op.lhso = tok->op.lhso;
						tok->kind = ASGN_set;
						tok->op.rhso = tmp;
					} break;
				}
				#endif
			}
			*lhs++ = tok;
		}
	}
	if (mode && tok) {								// check

		s->root = NULL;
		if (!typecheck(s, NULL, tok)) {
			error(s->s, tok->file, tok->line, "invalid expression: `%+k` in `%+k`", s->root, tok);
			debug("%7K", tok);
		}

		/*/ usually one of [TYPE_bit, TYPE_int, TYPE_vid]
		if (mode != TYPE_def && !castTo(tok, mode))
			error(s->s, tok->file, tok->line, "invalid expression: %+k", tok);
		// */
	}
	return tok;
}

astn decl(ccState s, int Rmode) {
	int Attr = qual(s, ATTR_const | ATTR_stat);
	int line = s->line;
	char *file = s->file;
	astn tok, tag = NULL;
	symn def = NULL;

	if (skip(s, ENUM_kwd)) {			// enum
		symn base = type_i32;

		tag = next(s, TYPE_ref);

		if (skip(s, PNCT_cln)) {		// base type
			base = NULL;
			if ((tok = expr(s, TYPE_def))) {
				if (istype(tok)) {
					base = linkOf(tok);
				}
				else {
					error(s->s, s->file, s->line, "typename expected, got `%+15K`", tok);
				}
			}
			else {
				error(s->s, s->file, s->line, "typename expected, got `%k`", peek(s));
			}
		}

		if (skip(s, STMT_beg)) {		// constant list
			int enuminc = base && (base->cast == TYPE_i32 || base->cast == TYPE_i64);
			int64_t lastval = -1;

			if (tag) {
				def = declare(s, TYPE_rec, tag, base);
				tag->kind = TYPE_def;
				enter(s, tag);
			}
			else {
				tag = newnode(s, TYPE_def);
				tag->type = base;
				tag->ref.link = base;
				def = NULL;
			}

			while (!skip(s, STMT_end)) {
				trloop("%k", peek(s));
				if (!peek(s)) {
					error(s->s, s->file, s->line, "invalid end of file:%+k", peek(s));
					return NULL;
				}

				if ((tok = next(s, TYPE_ref))) {

					// HACK: declare as a variable to be assignable, then revert to def.
					symn tmp = declare(s, TYPE_ref, tok, base);
					tmp->stat = tmp->cnst = 1;
					redefine(s, tmp);

					if (decl_init(s, tmp)) {
						if (enuminc) {
							lastval = constint(tmp->init);
						}
					}
					else if (enuminc) {
						tmp->init = intnode(s, lastval += 1);
						tmp->init->type = base;
					}
					else {
						error(s->s, tmp->file, tmp->line, "constant expected");
					}
					tmp->kind = TYPE_def;
				}
				skiptok(s, STMT_do, 1);
			}

			if (def) {
				def->args = leave(s, def, 1);
			}
		}
		/*else if (tag && test(s, ASGN_set)) {	// simple constant: enum x:int = 9;
			//~ HACK: declare as a variable to be assignable, then revert to def.
			symn tmp = declare(s, TYPE_ref, tag, base);
			tmp->stat = tmp->cnst = 1;
			redefine(s, tmp);

			if (!decl_init(s, tmp)) {
				error(s->s, tmp->file, tmp->line, "constant expected");
			}
			skiptok(s, STMT_do, 1);
			tmp->kind = TYPE_def;
		}*/
		else {
			skiptok(s, STMT_do, 1);
		}
		if (def) {
			redefine(s, def);
		}
		//~ Attr &= ~(0);		// disable all qualifiers
	}
	else if (skip(s, TYPE_rec)) {		// struct
		int byref = skip(s, OPER_and);
		int pack = vm_size;

		if (!(tag = next(s, TYPE_ref))) {	// name?
			tag = newnode(s, TYPE_ref);
			//~ tag->type = type_vid;
			tag->file = s->file;
			tag->line = s->line;
		}

		if (skip(s, PNCT_cln)) {			// basetype or packing
			if ((tok = expr(s, TYPE_def))) {
				if (tok->kind == TYPE_int) {

					if (Attr == ATTR_stat) {
						error(s->s, s->file, s->line, "alignment can not be applied to static struct");
					}
					else switch ((int)constint(tok)) {
						default:
							error(s->s, tok->file, tok->line, "alignment must be a small power of two, not %k", tok);
							break;


						case  0:		// union.
							pack =  0;
							break;

						case  1:
							pack =  1;
							break;

						case  2:
							pack =  2;
							break;

						case  4:
							pack =  4;
							break;

						case  8:
							pack =  8;
							break;

						case 16:
							pack = 16;
							break;
					}
				}
				/*else if (istype(tok)) {
					base = linkOf(tok);
					pack = base->pack;
				}// */
				else {
					error(s->s, s->file, s->line, "alignment must be an integer constant");
				}
			}
			else {
				error(s->s, s->file, s->line, "alignment must be an integer constant, got `%k`", peek(s));
			}
		}

		if (skip(s, STMT_beg)) {			// body

			def = declare(s, TYPE_rec, tag, s->type_rec);
			redefine(s, def);

			enter(s, tag);

			//~ install(s, "class", ATTR_stat | ATTR_const | TYPE_def, 0, type_ptr, lnknode(s, def));
			while (!skip(s, STMT_end)) {
				trloop("%k", peek(s));
				if (!peek(s)) {
					error(s->s, s->file, s->line, "invalid end of file:%+k", peek(s));
					return NULL;
				}

				if ((tok = decl(s, 0))) {
					symn ref = tok->ref.link;
					if (Attr == ATTR_stat) {
						ref->stat = 1;
					}
					if (ref->kind != TYPE_ref) {
						ref->stat = 1;
					}

					if (!ref->stat) {
						if (ref->init) {
							error(s->s, ref->file, ref->line, "non static member `%-T` can not be initialized", ref);
						}
					}
					else {
						if (ref->cnst && !ref->init) {
							error(s->s, ref->file, ref->line, "uninitialized constant `%-T`", ref);
						}
					}
				}
				else {
					error(s->s, s->file, s->line, "declaration expected, got: `%+k`", peek(s));
					skiptok(s, STMT_do, 0);
				}
			}

			def->args = leave(s, def, (Attr & ATTR_stat) != 0);
			def->size = fixargs(def, pack, 0);
			def->cast = byref ? TYPE_ref : TYPE_rec;

			if (Attr != ATTR_stat) {
				ctorPtr(s, def);
				if (def->args && pack == vm_size) {
					ctorArg(s, def);
				}

				if (!def->args && !def->type) {
					warn(s->s, 2, s->file, s->line, "empty declaration");
				}
			}
			else {
				Attr &= ~ATTR_stat;
			}
		}
		else {
			skiptok(s, STMT_beg, 1);
		}

		// error handling
		if (byref && Attr == ATTR_stat) {	// namespace: static struct
			error(s->s, tag->file, tag->line, "alignment can not be applied to this struct");
		}

		Attr &= ~(ATTR_stat);		// enable static structures
		tag->type = def;
	}

	else if (skip(s, TYPE_def)) {		// define
		symn typ = 0;

		if (!(tag = next(s, TYPE_ref))) {	// name?
			error(s->s, s->file, s->line, "Identifyer expected");
			skiptok(s, STMT_do, 1);
			return NULL;
		}

		if (skip(s, ASGN_set)) {				// const:   define PI = 3.14...;
			// can not call decl_init
			// TODO: check if locals are used.
			astn val = expr(s, TYPE_def);
			if (val != NULL) {
				def = declare(s, TYPE_def, tag, val->type);
				if (isConst(val)) {
					def->stat = 1;
					def->cnst = 1;
				}
				else if (isStatic(s, val)) {
					def->stat = 1;
				}
				def->init = val;
				typ = def->type;
			}
			else {
				error(s->s, s->file, s->line, "expression expected");
				//~ return NULL;
			}
		}
		else if (skip(s, PNCT_lp)) {			// inline:  define isNan(float64 x) = (x != x);
			symn arg;
			int warnfun = 0;

			// TODO: check if locals are used.
			def = declare(s, TYPE_def, tag, NULL);
			def->name = NULL;

			enter(s, NULL);
			args(s, TYPE_def);
			skiptok(s, PNCT_rp, 1);

			if (skip(s, ASGN_set)) {		// define sqr(int a) = (a * a);
				// disable recursive inlineing: define x(int a) = (a * x(a/2));
				//~ char* name = def->name;
				//~ def->name = 0;
				if ((def->init = expr(s, TYPE_def))) {
					if (def->init->kind != OPER_fnc) {
						astn tmp = newnode(s, OPER_fnc);
						warnfun = 1;
						tmp->type = def->init->type;
						tmp->cst2 = def->init->cst2;
						tmp->file = def->init->file;
						tmp->line = def->init->line;
						tmp->op.rhso = def->init;
						def->init = tmp;

						/*TODO("find a better way to do this")
						if (!typecheck(s, 0, tmp)) {
							error(s->s, tmp->file, tmp->line, "invalid expression: %+k", tmp);
						}*/
					}
					typ = def->init->type;
				}
				//~ def->name = name;
			}

			def->args = leave(s, NULL, 0);
			def->stat = isStatic(s, def->init);
			def->cnst = isConst(def->init);
			def->name = tag->ref.name;
			def->type = typ;
			def->call = 1;

			if (def->args == NULL)
				def->args = s->void_tag->ref.link;

			for (arg = def->args; arg; arg = arg->next) {
				if (arg->cast == TYPE_ref)
					arg->cast = arg->type->cast;
				else
					arg->cast = TYPE_def;
			}

			if (warnfun) {
				warn(s->s, 8, tag->file, tag->line, "%+k should be a function", tag);
				info(s->s, tag->file, tag->line, "defined as: %-T", def);
			}
		}
		else if ((tok = type(s))) {				// typedef: define hex16 int16;	???
			typ = tok->type;
			//~ def = declare(s, TYPE_def, tag, typ);
			def = declare(s, TYPE_rec, tag, typ);
			def->kind = typ->kind;
			def->cast = typ->cast;
			def->type = typ->type;
			s->pfmt = def;
		}
		else
			error(s->s, tag->file, tag->line, "typename excepted");
		//~ if (op) error(s->s, tag->file, tag->line, "invalid operator");
		skiptok(s, STMT_do, 1);

		tag->kind = TYPE_def;
		tag->type = typ;
		tag->ref.link = def;

		Attr &= ~(ATTR_stat|ATTR_const);		// enable static and const qualifiers
		redefine(s, def);
	}
	/*if (skip(s, OPER_kwd)) {			// operator
		skiptok(s, STMT_do, 1);
	}// */

	else if ((tag = decl_var(s, NULL, 0))) {	// variable, function, ...
		symn typ = tag->type;
		symn ref = tag->ref.link;

		//~ debug("%-T: %-T / %+k", typ, ref, tag);

		if ((Rmode & decl_NoInit) == 0) {
			if (ref->call && test(s, STMT_beg)) {		// int sqr(int a) {return a * a;}
				symn tmp, result = NULL;
				int maxlevel = s->maxlevel;

				// create a def for each argument, and a result

				enter(s, tag);
				ref->gdef = s->func;
				s->func = ref;
				s->maxlevel = s->nest;
				result = install(s, "result", TYPE_ref, 0, ref->type->size, typ, NULL);

				if (result) {

					//~ if (typ->cast == TYPE_ref)
						//~ result->cast = TYPE_ref;

					result->decl = ref;

					// result is the first argument
					result->offs = sizeOf(result);

					ref->offs = result->offs + fixargs(ref, vm_size, -result->offs);

					//~ ref->nest = s->nest + 1;

				}
				// reinstall all args and TODO:(closure variables)
				for (tmp = ref->args; tmp; tmp = tmp->next) {
					symn arg = install(s, tmp->name, TYPE_ref, 0, 0, NULL, NULL);
					if (arg != NULL) {
						arg->type = tmp->type;
						arg->args = tmp->args;
						arg->cast = tmp->cast;
						arg->size = tmp->size;
						arg->offs = tmp->offs;
						arg->Attr = tmp->Attr;
					}
				}

				ref->stat = 1;
				ref->init = stmt(s, 1);

				if (ref->init == NULL) {
					ref->init = newnode(s, STMT_beg);
					ref->init->type = type_vid;
				}

				s->func = s->func->gdef;
				s->maxlevel = maxlevel;
				leave(s, ref, 0);
				backTok(s, newnode(s, STMT_do));

				Attr |= ATTR_stat;

			}
			else if (test(s, ASGN_set)) {
				if (ref->call) {
					ref->cast = TYPE_ref;
				}
				if (!decl_init(s, ref)) {
					trace("FixMe");
					return NULL;
				}
			}
		}

		if (Rmode & decl_Colon)
			if (test(s, PNCT_cln))
				backTok(s, newnode(s, STMT_do));

		skiptok(s, STMT_do, 1);

		if (Attr & ATTR_const) {
			// constant variables are also static
			ref->cnst = 1;
		}

		if (Attr & ATTR_stat) {
			ref->stat = 1;
		}

		Attr &= ~(ATTR_stat|ATTR_const);		// enable static and const qualifiers
		tag->kind = TYPE_def;
		redefine(s, ref);
	}

	if (Attr & ATTR_const) {
		error(s->s, file, line, "qualifier `const` can not be applied to `%+k`", tag);
	}
	if (Attr & ATTR_stat) {
		error(s->s, file, line, "qualifier `static` can not be applied to `%+k`", tag);
	}

	//~ dieif(Attr, "FixMe: %+k", tag);
	return tag;
}

astn unit(ccState s, int mode) {
	astn tmp, root = NULL;
	int level = s->nest;

	/*if (skip(s, UNIT_kwd)) {
		astn tag = next(s, TYPE_ref);

		if (tag == NULL) {
			error(s->s, s->file, s->line, "Identifyer expected");
			skiptok(s, STMT_do, 1);
			return NULL;
		}

		def = declare(s, TYPE_def, tag, type_vid);
	}// */

	root = newnode(s, STMT_beg);

	if ((tmp = block(s, 0))) {
		root->stmt.stmt = tmp;
		root->type = type_vid;
	}
	else {
		eatnode(s, root);
		root = NULL;
	}

	if (peekTok(s, 0)) {
		error(s->s, s->file, s->line, "unexpected token: %k", peekTok(s, 0));
		return NULL;
	}

	if (s->nest != level) {
		error(s->s, s->file, s->line, "premature end of file: %d", s->nest);
		return NULL;
	}

	(void)mode;
	return root;
}

//}

/** parse the current buffer and add to root.
 */
int parse(ccState s, srcType mode, int wl) {
	astn newRoot, oldRoot = s->root;

	s->warn = wl;

	dieif(mode != 0, "FixMe");

	/*if (mode & SCAN_pre) {
		astn root = NULL;
		astn ast, prev = NULL;
		while ((ast = next(s, 0))) {
			if (prev == NULL) {
				root = ast;
				prev = ast;
			}
			else
				prev->next = ast;
			prev = ast;
			//~ info(s->s, ast->file, ast->line, "%k", ast);
		}
		backTok(root);
	}
	switch (mode) {
		case srcUnit: root = unit(s, 0); break;
		case srcDecl: root = decl(s, 0); break;
		case srcExpr: root = expr(s, 0); break;
	}//*/

	if ((newRoot = unit(s, 0))) {
		if (oldRoot) {

			astn end = oldRoot->stmt.stmt;

			dieif(oldRoot->kind != STMT_beg || !oldRoot->stmt.stmt, "FixMe");

			if (newRoot->kind == STMT_beg && newRoot->stmt.stmt) {
				astn beg = newRoot;
				dieif(newRoot->next, "FixMe");
				newRoot = beg->stmt.stmt;
				eatnode(s, beg);
			}

			while (end->next)
				end = end->next;

			end->next = newRoot;

			newRoot = oldRoot;
		}
		s->root = newRoot;
	}
	return ccDone(s->s);
}

/** unit -----------------------------------------------------------------------
scan a source file
	@param mode: script mode (enable non declaration statements)

unit:
	| ('module' <name> ';') <stmt>*
	| <stmt>*
	;
**/
/** stmt -----------------------------------------------------------------------
scan a statement
	@param mode: enable or not <decl>, enetr new scope

stmt: ';'
	| '{' <stmt>* '}'
	| ('static')? 'if' '(' <decl> | <expr> ')' <stmt> ('else' <stmt>)?
	| ('static' | 'parallel')? 'for' '(' (<decl> | <expr>)?; <expr>?; <expr>? ')' <stmt>
	| 'continue' ';'
	| 'break' ';'
	| 'return' ';'
	| mode && <decl>
	| <expr> ';'
	;

	+ 'return' <expr>? ';'
**/
/** decl -----------------------------------------------------------------------
declarations
	@param mode: enable or not type decl.

decl:
	# enum
	| 'enum' <name> (':' <type>)? ':=' <expr> ';'
	| 'enum' <name>? (':' <type>)? '{' (<name> (':=' <expr>)? ';')* '}'

	# struct
	| ('static')? 'struct' <name>? (':' (<pack>|<type>))? '{' <decl>+ '}'

	# define
	| 'define' <name> <type>;						// typedef
	# inline expression: static and const should force the lhs to be const or to not use local vars
	| ('static' | 'const')+ 'define' <name> ('(' <args> ')')? ':=' <expr>;

	//~ # operator: TODO
	//~ | 'operator' <OP> ('(' <type> rhs ')') ':=' <expr> ';'
	//~ | 'operator' <type> ('(' <type> rhs ')') ':=' <expr> ';'
	//~ | 'operator' ('(' <type> lhs ')') <OP> ('(' <type> rhs ')') ':=' <expr> ';'

	# variable
	| <type> <name> ('['<expr>?']')? (':=' <expr>)? ';'
	# function
	| <type> <name> '(' <args> ')' (('{' <stmt>* '}') | (':=' <name> ';') | (';'))
	;

type:
	| typename ('.' typename)*
	;

tref:
	| <type> ('^' | '&')? <name>
	;

//~ init: ':=' <expr>;

args:
	| <tref> (',' <tref>)*
	;

/+ Operator: operators are explicit, defines are implicit
	@ operator Complex(float64 re) = Complex(re, 0);
	@ operator - (Complex ^x) = Complex(-x.re, -x.im);
	@ operator (Complex ^x) + (Complex ^y) = Complex(x.re + y.re, x.im + y.im);
	@ Complex x1 = 3.1;
	@ Complex y1 = -x1;
	@ Complex z1 = x1 + y1;
	@ define Complex(float64 re) = Complex(re, 0);
	@ define neg(Complex ^x) = Complex(-x.re, -x.im);
	@ define add(Complex ^x, Complex ^y) = Complex(x.re + y.re, x.im + y.im);
	@ Complex x2 = Complex(3.1);
	@ Complex y2 = neg(x2);
	@ Complex z2 = add(x2, y2);

	# define
	@ define string : char[];
	@ define pi = 3.1415;
		@ define sqr(int ^a) = int(a * a);
	passing arguments to `neg(Complex ^x)`: is by reference, but if x is not a local reference it vill be evaluated first.
// +/

**/
/** expr -----------------------------------------------------------------------
expression parser (non recursive, )
expr := <name>
	| <expr> '[' <expr> ']'
	| <expr> '.' <expr>
	| <expr>? '(' <expr>? ')'
	| ['+', '-', '~', '!', '&'] <expr>
	| <expr> ['+', '*', ...] <expr>
	| <expr> '?' <expr> ':' <expr>
	;
**/
