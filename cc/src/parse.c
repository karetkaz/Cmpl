#include <unistd.h>

#include <string.h>
#include <fcntl.h>
#include <math.h>
#include "ccvm.h"

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
		if (len == -1)
			len = strlen(str) + 1;
		while (len-- > 0)
			hs = (hs >> 8) ^ crc_tab[(hs ^ (*str++)) & 0xff];
	}

	return (hs ^ 0xffffffff);// % TBLS;
}

int source(ccEnv s, srcType mode, char* file) {
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
		s->file = file;
		s->line = 1;
	}
	else if (file) {
		s->fin._ptr = file;
		s->fin._cnt = strlen(file);
	}
	return 0;
}

static int fillBuf(ccEnv s) {
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

static int readChr(ccEnv s) {
	if (s->_chr != -1) {
		int chr = s->_chr;
		s->_chr = -1;
		return chr;
	}
	if (s->fin._cnt <= 4 && fillBuf(s) <= 0) {
		return -1;
	}
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
	}
	if (*s->fin._ptr == '\r' || *s->fin._ptr == '\n') {
		if (s->fin._ptr[0] == '\r' && s->fin._ptr[1] == '\n')
			--s->fin._cnt, s->fin._ptr++;
		--s->fin._cnt, s->fin._ptr++;
		s->line += s->line != 0;
		return '\n';
	}
	return *(--s->fin._cnt, s->fin._ptr++);
}

static int peekChr(ccEnv s) {
	if (s->_chr == -1)
		s->_chr = readChr(s);
	return s->_chr;
}

static int skipChr(ccEnv s, int chr) {
	if (!chr || chr == peekChr(s))
		return readChr(s);
	return 0;
}

static int backChr(ccEnv s, int chr) {
	dieif (s->_chr != -1, "");
	return s->_chr = chr;
}

static int escchr(ccEnv s) {
	int chr;
	switch (chr = readChr(s)) {
		case  'a': return '\a';
		case  'b': return '\b';
		case  'f': return '\f';
		case  'n': return '\n';
		case  'r': return '\r';
		case  't': return '\t';
		case  'v': return '\v';
		case '\'': return '\'';
		case '\"': return '\"';
		case '\\': return '\\';
		case '\?': return '\?';

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
			return val & 0xff;
		}

		// hex sequence (len = 2)
		case 'x': {
			int h1 = readChr(s);
			int h2 = readChr(s);
			if (h1 >= '0' && h1 <= '9') h1 -= '0';
			else if (h1 >= 'a' && h1 <= 'f') h1 -= 'a' - 10;
			else if (h1 >= 'A' && h1 <= 'F') h1 -= 'A' - 10;
			else {
				error(s->s, s->line, "hex escape sequence invalid");
				return 0;
			}

			if (h2 >= '0' && h2 <= '9') h2 -= '0';
			else if (h2 >= 'a' && h2 <= 'f') h2 -= 'a' - 10;
			else if (h2 >= 'A' && h2 <= 'F') h2 -= 'A' - 10;
			else {
				error(s->s, s->line, "hex escape sequence invalid");
				return 0;
			}
			return h1 << 4 | h2;
		}

		default:
			error(s->s, s->line, "invalid escape sequence '\\%c'", chr);
			return -1;
	}
	return 0;
}

//}

//{~~~~~~~~~ Lexer ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

char *mapstr(ccEnv s, char *name, unsigned size/* = -1U*/, unsigned hash/* = -1U*/) {

	list newn, next, prev = 0;

	if (size == -1U) {
		//~ debug("strlen '%s'", name);
		size = strlen(name) + 1;
	}

	dieif (name[size-1], "FixMe: %s[%d]", name, size);

	if (hash == -1U) {
		//~ debug("strcrc '%s'", name);
		hash = rehash(name, size) % TBLS;
	}

	for (next = s->strt[hash]; next; next = next->next) {
		//~ int slen = next->data - (unsigned char*)next;
		register int c = memcmp(next->data, name, size);
		if (c == 0) return (char*)next->data;
		if (c > 0) break;
		prev = next;
	}

	dieif(s->_end - s->_beg <= (sizeof(struct list) + size + 1), "memory overrun");

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

static int readTok(ccEnv s, astn tok) {
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
				if (chr == -1) warn(s->s, 9, s->file, line, "no newline at end of file");
				else if (s->line != line+1) warn(s->s, 9, s->file, line, "multi-line comment");
				if (fmt && (s->doc == (char*)-1)) {
					*ptr++ = 0;
					s->doc = mapstr(s, beg, ptr - beg, -1);
				}
			}
			else if (next == '*') {
				int c = 0, prev_chr = 0;
				//~ debug("!!!comment.stream: {\n/*");
				for (readChr(s); chr != -1; chr = readChr(s)) {
					//~ if (prev_chr) debug("%c", chr);
					if (prev_chr == '*' && chr == '/') break;
					if (prev_chr == '/' && chr == '*' && c++)
						warn(s->s, 8, s->file, s->line, "\"/*\" within comment(%d)", c - 1);
					prev_chr = chr;
				}
				if (chr == -1) error(s->s, line, "unterminated comment2");
				//~ else debug("\n!!!comment.box:}\n");
				chr = readChr(s);
			}
			else if (next == '+') {
				int l = 1, c = 0, prev_chr = 0;
				//~ debug("!!!comment.block:{\n/+");
				for (readChr(s); chr != -1; chr = readChr(s)) {
					//~ if (prev_chr) debug("%c", chr);
					if (prev_chr == '/' && chr == '+' && c++) l++;
					if (prev_chr == '+' && chr == '/' && !--l) break;
					prev_chr = chr;
				}
				if (chr == -1) error(s->s, line, "unterminated comment1");
				//~ else debug("\n!!!comment.block:}\n");
				chr = readChr(s);
			}
		}
		if (chr == 0) {
			warn(s->s, 5, s->file, s->line, "null character(s) ignored");
			while (!chr) chr = readChr(s);
		}

		if (!(chr_map[chr & 0xff] & SPACE)) break;
		chr = readChr(s);
	}

	memset(tok, 0, sizeof(*tok));
	tok->line = s->line;
	ptr = beg = s->_beg;

	// scan
	if (chr != -1) switch (chr) {
		default: {
			if (chr_map[chr & 0xff] & (ALPHA|UNDER))
				goto read_idf;

			if (chr_map[chr & 0xff] & DIGIT)
				goto read_num;

			// control(<32) or non ascii character(>127) ?
			error(s->s, s->line, "Invalid character: '%c'", chr);
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
		case ':': {		// TODO: ':=' ?
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
			else tok->kind = OPER_pls;
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
			else tok->kind = OPER_mns;
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
		case '\'': {		// \'[^\'\n]*
			int val = 0;
			while (ptr < end && (chr = readChr(s)) != -1) {

				if (chr == '\'')
					break;

				if (chr == '\n')
					break;

				if (chr == '\\')
					chr = escchr(s);

				val = val << 8 | chr;
				*ptr++ = chr;
			}
			*ptr = 0;

			if (chr != '\'') {
				error(s->s, s->line, "unclosed character constant ('\'' missing)");
				return tok->kind = TOKN_err;
			}
			if (ptr == beg) {
				error(s->s, s->line, "empty character constant");
				return tok->kind = TOKN_err;
			}

			if (ptr > beg + sizeof(char))
				warn(s->s, 5, s->file, s->line, "multi character constant");
			if (ptr > beg + sizeof(val))
				warn(s->s, 1, s->file, s->line, "character constant truncated");

			tok->kind = TYPE_int;
			//~ tok->type = type_i32;	// or type_chr
			tok->con.cint = val;
		} break;
		case '\"': {		// \"[^\"\n]*
			while (ptr < end && (chr = readChr(s)) != -1) {
				if (chr == '\"') break;
				else if (chr == '\n') break;
				else if (chr == '\\') chr = escchr(s);
				*ptr++ = chr;
			}
			*ptr++ = 0;
			if (chr != '\"') {					// error
				error(s->s, s->line, "unclosed string constant ('\"' missing)");
				return tok->kind = TOKN_err;
			}

			tok->kind = TYPE_str;
			//~ tok->type = type_str;
			tok->id.hash = rehash(beg, ptr - beg) % TBLS;
			tok->id.name = mapstr(s, beg, ptr - beg, tok->id.hash);
		} break;
		read_idf: {			// [a-zA-Z_][a-zA-Z0-9_]*
			while (ptr < end && chr != -1) {
				if (!(chr_map[chr & 0xff] & CWORD))
					break;
				*ptr++ = chr;
				chr = readChr(s);
			}
			backChr(s, chr);
			*ptr++ = 0;

			if (1) {
				static int test = 1;		// TODO: DelMe: test keywords
				static struct {
					char *name;
					int type;
					//~ int _pad;
				}
				keywords[] = {
					// my SciTe can sort these lines !!!
					{"define", TYPE_def},
					{"else", STMT_els},
					{"emit", EMIT_opc},
					{"enum", TYPE_enu},
					{"for", STMT_for},
					{"if", STMT_if},
					{"module", UNIT_def},
					{"operator", OPER_op},
					{"parralel", QUAL_par},
					{"static", QUAL_sta},
					{"struct", TYPE_rec},
				};

				int lo = 0;
				int hi = sizeof(keywords) / sizeof(*keywords);
				int cmp = -1;

				if (test != 0) {
					int i, j, n = 0;
					for (i = 0; i < tok_last; ++i) {
						if (tok_tbl[i].type == 0xff) {
							for (j = 0; j < hi; ++j) {
								if (i == keywords[j].type)
									break;
							}
							//~ debug("keyword: %t", i);
							dieif(j > hi, "FixMe: %t", i);
							dieif(strcmp(tok_tbl[i].name, keywords[j].name) != 0, "FixMe: %t", i);
							n += 1;
						}
					}
					dieif(n != hi, "FixMe %d/%d", n, hi);
					test = 0;
				}
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
					tok->type = tok->id.link = 0;
					tok->id.hash = rehash(beg, ptr - beg) % TBLS;
					tok->id.name = mapstr(s, beg, ptr - beg, tok->id.hash);
					//~ debug("idf: %k", tok);
				}
			}
			else {
				tok->kind = TYPE_ref;
				tok->type = tok->id.link = 0;
				tok->id.hash = rehash(beg, ptr - beg) % TBLS;
				tok->id.name = mapstr(s, beg, ptr - beg, tok->id.hash);
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

				if (i64v > 0x7fffffffffffffffULL / 10) {
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

			f64v = i64v;

			if (ovrf)
				warn(s->s, 4, s->file, s->line, "value overflow");

			//~ ('.'[0-9]*)?
			if (radix == 10 && chr == '.') {
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

			//~ ([eE]([+-]?)[0-9]+)?
			if (radix == 10 && (chr == 'e' || chr == 'E')) {
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
					error(s->s, tok->line, "no exponent in numeric constant");
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

			suffix = ptr;
			while (chr != -1) {
				if (chr == '.' || chr == '_') ;
				else if (chr >= '0' && chr <= '9') ;
				else if (chr >= 'a' && chr <= 'z') ;
				else if (chr >= 'A' && chr <= 'Z') ;
				else {*ptr = 0; break;}
				*ptr++ = chr;
				chr = readChr(s);
			}
			backChr(s, chr);

			if (*suffix) {	// error
				error(s->s, tok->line, "invalid suffix in numeric constant '%s'", suffix);
				tok->kind = TYPE_any;
			}
			if (flt) {		// float
				tok->kind = TYPE_flt;
				//~ tok->type = type_f64;
				tok->con.cflt = f64v;
			}
			else {			// integer
				tok->kind = TYPE_int;
				//~ tok->type = type_i64;
				tok->con.cint = i64v;
			}
		} break;
	}
	if (ptr >= end) {
		//~ debug("mem overrun %t", tok->kind);
		fatal("mem overrun %t", tok->kind);
		return 0;
	}
	return tok->kind;
}

astn peekTok(ccEnv s, int kind) {
	//~ dieif(!s->_cnt, "FixMe: invalid ccEnv");
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

//~ TODO: remove these
astn peek(ccEnv s) {return peekTok(s, 0);}
int line(ccEnv s) {return peekTok(s, 0) ? s->_tok->line : s->line;}

// static
astn next(ccEnv s, int kind) {
	if (peekTok(s, kind)) {
		astn ast = s->_tok;
		s->_tok = ast->next;
		ast->next = 0;
		return ast;
	}
	return 0;
}

static void backTok(ccEnv s, astn ast) {
	//~ dieif(!s->_cnt, "FixMe: invalid ccEnv");
	ast->next = s->_tok;
	s->_tok = ast;
}

//~ TODO: remove one of these (test preferably)
static int test(ccEnv s, int kind) {
	astn ast = peek(s);
	if (!ast || (kind && ast->kind != kind))
		return 0;
	return ast->kind;
}// */
static int skip(ccEnv s, int kind) {
	astn ast = peekTok(s, 0);
	if (!ast || (kind && ast->kind != kind))
		return 0;
	s->_tok = ast->next;
	eatnode(s, ast);
	return 1;
}

static int skiptok(ccEnv s, int kind, int report) {
	if (!skip(s, kind)) {
		//~ int report = 0;
		if (report)
			error(s->s, s->line, "`%t` excepted, got `%k`", kind, peek(s));

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

//~ TODO: this should go to type.c
static void redefine(ccEnv s, symn sym) {
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

		// if not redefineable
		if (ptr->read)
			break;

		while (arg1 && arg2) {
			if (arg1->type != arg2->type)
				break;
			arg1 = arg1->next;
			arg2 = arg2->next;
		}

		if (arg1 || arg2)
			continue;

		break;
	}

	if (ptr && (ptr->nest >= sym->nest/*  || ptr->read */)) {
		error(s->s, sym->line, "redefinition of `%-T`", sym);
		if (ptr->file && ptr->line)
			info(s->s, ptr->file, ptr->line, "first defined here as `%-T`", ptr);
	}
}

//~    astn unit(ccEnv, int mode);		// parse unit			(mode: script/unit)
static astn stmt(ccEnv, int mode);		// parse statement		(mode: enable decl)
//~    astn decl(ccEnv, int mode);		// parse declaration	(mode: enable spec)
static astn spec(ccEnv s/* , int qual */);
static symn type(ccEnv s/* , int qual */);	// TODO: this should also return an ast node

astn expr(ccEnv s, int mode) {
	astn buff[TOKS], *base = buff + TOKS;
	astn *post = buff, *prec = base, tok;
	char sym[TOKS];							// symbol stack {, [, (, ?
	int level = 0, unary = 1;				// precedence, top of sym , start in unary mode
	sym[level] = 0;

	while ((tok = next(s, 0))) {					// parse
		int pri = level << 4;
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
						error(s->s, tok->line, "syntax error before '%k'", tok);
					}
					unary = 1;
					goto tok_op;
				}
				else {
					if (!unary)
						error(s->s, tok->line, "syntax error before '%k'", tok);
					unary = 0;
					goto tok_id;
				}
			} break;
			case PNCT_lp: {			// '('
				if (unary)			// a + (3*b)
					*post++ = 0;
				else if (skip(s, PNCT_rp)) {	// a()
					*post++ = 0;
					tok->kind = OPER_fnc;
					goto tok_op;
				}
				//~ else			// a(...)
				tok->kind = OPER_fnc;
				sym[++level] = '(';
				unary = 1;
			} goto tok_op;
			case PNCT_rp: {			// ')'
				if (!unary && sym[level] == '(') {
					tok->kind = STMT_do;
					level -= 1;
				}
				else if (level == 0) {
					backTok(s, tok);
					tok = 0;
					break;
				}
				else {
					error(s->s, tok->line, "syntax error before '%k'", tok);
					break;
				}
				unary = 0;
			} goto tok_op;
			case PNCT_lc: {			// '['
				if (!unary)
					tok->kind = OPER_idx;
				else {
					error(s->s, tok->line, "syntax error before '%k'", tok);
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
					error(s->s, tok->line, "syntax error before '%k'", tok);
					break;
				}
				unary = 0;
			} goto tok_op;
			case PNCT_qst: {		// '?'
				if (unary) {
					*post++ = 0;		// ???
					error(s->s, tok->line, "syntax error before '%k'", tok);
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
					error(s->s, tok->line, "syntax error before '%k'", tok);
					break;
				}
				unary = 1;
			} goto tok_op;
			case OPER_pls: {		// '+'
				if (!unary)
					tok->kind = OPER_add;
				unary = 1;
			} goto tok_op;
			case OPER_mns: {		// '-'
				if (!unary)
					tok->kind = OPER_sub;
				unary = 1;
			} goto tok_op;
			case OPER_not: {		// '!'
				if (!unary)
					error(s->s, tok->line, "syntax error before '%k'", tok);
				unary = 1;
			} goto tok_op;
			case OPER_cmt: {		// '~'
				if (!unary)
					error(s->s, tok->line, "syntax error before '%k'", tok);
				unary = 1;
			} goto tok_op;
			case OPER_com: {		// ','
				if (unary)
					error(s->s, tok->line, "syntax error before '%k'", tok);
				unary = 1;
			} goto tok_op;
		}
		if (post >= prec) {
			error(s->s, s->line, "Expression too complex");
			return 0;
		}
		if (!tok)
			break;
	}
	if (unary || level) {							// error
		char *missing = "expression";
		if (level) switch (sym[level]) {
			case '(': missing = "')'"; break;
			case '[': missing = "']'"; break;
			case '?': missing = "':'"; break;
			default : fatal("FixMe");
		}
		error(s->s, s->line, "missing %s", missing);
		//~ debug("%k(%d)", peek(s), line(s));
	}
	else if (prec > buff) {							// build
		astn *ptr, *lhs;

		while (prec < buff + TOKS)					// flush
			*post++ = *prec++;
		*post = NULL;

		for (lhs = ptr = buff; ptr < post; ptr += 1) {
			if ((tok = *ptr) == NULL) ;				// skip
			else if (tok_tbl[tok->kind].argc) {		// oper
				int argc = tok_tbl[tok->kind].argc;
				if ((lhs -= argc) < buff) {
					fatal("FixMe");
					return 0;
				}
				switch (argc) {
					case 1: {
						//~ tok->op.test = 0;
						//~ tok->op.lhso = 0;
						tok->op.rhso = lhs[0];
					} break;
					case 2: {
						//~ tok->op.test = 0;
						tok->op.lhso = lhs[0];
						tok->op.rhso = lhs[1];
					} break;
					case 3: {
						tok->op.test = lhs[0];
						tok->op.lhso = lhs[1];
						tok->op.rhso = lhs[2];
					} break;
					default: fatal("FixMe");
				}
				#ifdef DEBUGGING
				switch (tok->kind) {
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
		if (typecheck(s, 0, tok) == 0) {
			//~ error(s->s, tok->line, "invalid expression: %+k in statement %+k\n%7K", s->root, tok, tok);
			error(s->s, tok->line, "invalid expression: `%+k` in `%+k`", s->root, tok);
			debug("%7K", tok);
		}

		// usually mode in [TYPE_bit, TYPE_int, TYPE_vid]
		if (mode != TYPE_def && !castTo(tok, mode))
			error(s->s, tok->line, "invalid expression: %+k", tok);
	}
	return tok;
}

static symn type(ccEnv s) {
	symn def = 0;
	astn tok;
	while ((tok = peekTok(s, TYPE_ref))) {		// type(.type)*
		symn sym = typecheck(s, def, tok);	// TODO: use: lookup
		if (!istype(tok)) {
			def = NULL;
			break;
		}
		next(s, 0);
		def = sym;

		if (!skip(s, OPER_dot)) break;
		// TODO: build tree and push back if not a typename ?
	}// */
	return def;
}
static astn args(ccEnv s, int mode) {
	astn ast = NULL;

	if (test(s, PNCT_rp))
		return s->argz;

	while (peek(s)) {
		astn atag = 0;
		symn atyp = 0;
		symn arg = 0;
		int byref = 0;

		if (!(atyp = type(s)) && !mode) {
			error(s->s, s->line, "typename expected `%k`", peek(s));
			break;
		}

		byref = skip(s, OPER_and);

		if (!(atag = next(s, TYPE_ref))) {
			error(s->s, s->line, "unexpected `%k`", peek(s));
			break;
		}

		arg = declare(s, TYPE_ref, atag, atyp);
		arg->cast = byref ? TYPE_ref : 0;
		atag->csts = atyp->cast;
		//~ arg->load = ref != 0;

		if (ast) {
			astn tmp = newnode(s, OPER_com);
			tmp->op.lhso = ast;
			tmp->op.rhso = atag;
			atag = tmp;
		}
		ast = atag;

		if (!skip(s, OPER_com))
			break;
	}

	return ast;
}

astn unit(ccEnv cc, int mode1) {
	symn def = NULL;

	if (skip(cc, UNIT_def)) {
		astn tag = next(cc, TYPE_ref);

		if (tag == NULL) {
			error(cc->s, cc->line, "Identifyer expected");
			skiptok(cc, STMT_do, 1);
			return NULL;
		}

		def = declare(cc, TYPE_enu, tag, type_vid);
		//~ def->cast = TYPE_enu;
	}// */

	// we want a new scope and scan a list of statement
	backTok(cc, newnode(cc, STMT_beg));
	cc->root = stmt(cc, def == NULL);

	if (peekTok(cc, 0)) {
		error(cc->s, cc->line, "unexpected token: %k", peekTok(cc, 0));
		return NULL;
	}
	if (def) {
		def->args = leave(cc, def);
	}

	if (cc->nest)
		error(cc->s, cc->line, "premature end of file: %d", cc->nest);

	return cc->root;
}

static astn stmt(ccEnv s, int mode) {
	//~ int newscope = mode >= 0;
	astn ast = 0, tmp;
	int qual = 0;			// static | parallel

	//~ these tokens wont be read
	switch (test(s, 0)) {
		case STMT_end:		// '}' TODO: in case of errors
		case PNCT_rp :		// ')' TODO: in case of errors
		case PNCT_rc :		// ']' TODO: in case of errors
			error(s->s, s->line, "unexpected token '%k'", peek(s));
			skip(s, 0);
			return 0;
	}// */

	// check statement construct
	if ((ast = next(s, QUAL_par))) {		// 'parralel' ('for' | '{')
		switch (test(s, 0)) {
			//~ case STMT_beg:	// parralel task
			case STMT_for:		// parralel loop
				qual = QUAL_par;
				break;
			default:
				backTok(s, ast);
		}
		ast = 0;
	}
	if ((ast = next(s, QUAL_sta))) {		// 'static' ('if' | 'for')
		switch (test(s, 0)) {
			//~ case STMT_for:	// loop unroll
			case STMT_if:		// compile time if
				qual = QUAL_sta;
				break;
			default:
				backTok(s, ast);
		}
		ast = 0;
	}

	// scan the statement
	if (skip(s, STMT_do)) {}				// ;
	else if ((ast = next(s, STMT_beg))) {	// { ... }
		int newscope = !mode;
		astn end = 0;

		if (newscope)
			enter(s, 0);

		while (!skip(s, STMT_end)) {
			if (!peek(s)) {
				newscope = 0;
				break;
			}
			if ((tmp = stmt(s, 0))) {
				if (!end) ast->stmt.stmt = tmp;
				else end->next = tmp;
				end = tmp;
			}
		}

		ast->type = type_vid;
		if (!ast->stmt.stmt) {			// eat sort of {{;{;};{}{}}}
			eatnode(s, ast);
			ast = 0;
		}

		if (newscope)
			leave(s, NULL);	// TODO: ???
	}
	else if ((ast = next(s, STMT_if))) {	// if (...)
		int newscope = 1;
		skiptok(s, PNCT_lp, 1);
		ast->stmt.test = expr(s, TYPE_bit);
		skiptok(s, PNCT_rp, 1);
		if (qual == QUAL_sta) {
			struct astn ift;
			if (!eval(&ift, ast->stmt.test) || (!test(s, STMT_beg) && !test(s, STMT_do))) {
				error(s->s, ast->line, "invalid static if construct");
				return 0;
			}
			if (constbol(&ift))
				newscope = 0;
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
			enter(s, 0);

		ast->stmt.stmt = stmt(s, 1);	// no new scope / no decl

		if (skip(s, STMT_els))
			ast->stmt.step = stmt(s, 1);

		ast->type = type_vid;
		ast->csts = qual;

		if (newscope)
			leave(s, NULL);			// TODO: destruct
	}
	else if ((ast = next(s, STMT_for))) {	// for (...)
		enter(s, 0);
		skiptok(s, PNCT_lp, 1);
		if (!skip(s, STMT_do)) {		// init
			// TODO: enable var decl only	// TYPE_ref
			ast->stmt.init = decl(s, 0);
			if (!ast->stmt.init) {
				ast->stmt.init = expr(s, TYPE_vid);
				skiptok(s, STMT_do, 1);
			}
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
		ast->csts = qual;
		leave(s, NULL);
	}

	else if ((ast = decl(s, 1))) {
		astn tmp = newnode(s, STMT_do);
		tmp->line = ast->line;
		tmp->type = ast->type;
		tmp->stmt.stmt = ast;

		if (mode)
			error(s->s, s->line, "unexpected declaration %+k", ast);

		ast = tmp;
	}
	else if ((ast = expr(s, TYPE_vid))) {
		astn tmp = newnode(s, STMT_do);
		tmp->line = ast->line;
		tmp->type = type_vid;
		tmp->stmt.stmt = ast;
		ast = tmp;
		skiptok(s, STMT_do, 1);
	}

	else if ((ast = peek(s))) {
		error(s->s, ast->line, "unexpected token: %k", ast);
		skiptok(s, STMT_do, 1);
	}
	else {
		error(s->s, s->line, "unexpected end of file");
	}

	//~ dieif(qual, "FixMe");
	return ast;
}

astn decl(ccEnv s, int mode) {
	symn typ;
	astn tag = NULL;
	//~ astn arg = NULL;

	if (mode && (tag = spec(s)))
		return tag;

	//~ type ID ('('args')')? ('['expr']')* init?
	if ((typ = type(s))) {
		symn ref = NULL;

		if (!(tag = next(s, TYPE_ref))) {
			debug("id expected, not %k", peek(s));
			return 0;
		}

		ref = declare(s, TYPE_ref, tag, typ);

		if (skip(s, PNCT_lp)) {				// int a(...)

			enter(s, NULL);
			tag->id.args = args(s, 0);
			skiptok(s, PNCT_rp, 1);

			if (test(s, STMT_beg)) {		// int sqr(int a) {return a * a;}
				enter(s, NULL);
				installex(s, "result", TYPE_ref, 0, typ, NULL);		// TODO: this should be the first argument
				ref->init = stmt(s, 1);
				leave(s, NULL);
				backTok(s, newnode(s, STMT_do));
			}
			ref->args = leave(s, ref);
			ref->call = 1;
			if (ref->args == NULL) {
				tag->id.args = s->argz;
				ref->args = s->argz->id.link;
			}
		}

		else if (skip(s, PNCT_lc)) {		// int a[...]
			symn tmp = newdefn(s, TYPE_arr);
			tmp->type = typ;
			tmp->size = -1;
			typ = tmp;

			ref->type = typ;
			tag->type = typ;

			if (!test(s, PNCT_rc)) {
				struct astn val;
				typ->init = expr(s, TYPE_def);
				if (eval(&val, tmp->init) != TYPE_int)
					error(s->s, s->line, "integer constant expected");
				else
					typ->size = val.con.cint;
			}
			skiptok(s, PNCT_rc, 1);

			//~ /* Multi dimensional arrays
			while (skip(s, PNCT_lc)) {
				symn tmp = newdefn(s, TYPE_arr);
				tmp->type = typ->type;
				typ->type = tmp;
				tmp->size = -1;
				typ = tmp;

				if (!test(s, PNCT_rc)) {
					struct astn val;
					typ->init = expr(s, TYPE_def);
					if (eval(&val, tmp->init) != TYPE_int)
						error(s->s, s->line, "integer constant expected");
					else
						typ->size = val.con.cint;
				}
				skiptok(s, PNCT_rc, 1);

			} // */
		}

		if (skip(s, ASGN_set)) {
			ref->init = expr(s, TYPE_def);
		}

		skiptok(s, STMT_do, 1);

		redefine(s, ref);
		//~ debug("%+k", ref->init);
		if (kindOf(ref->init) == STMT_beg) {
		}
		else if (ref->init && (ref->init->type != emit_opc && ref->init->type != ref->type && !promote(ref->init->type, ref->type))) {
			debug("%-T := %T(%+k)", ref->type, ref->init->type, ref->init);
			error(s->s, s->line, "invalid initializer: %-T(%+k)", ref->type, ref->init);
			//return 0;
		}
		tag->kind = TYPE_def;
	}

	return tag;
}

extern int padded(int offs, int align);

#if __WORDSIZE == 64
#define ptrgtz(__PTR) ((long long)(__PTR) > 0)
#else //if __WORDSIZE == 32
#define ptrgtz(__PTR) ((long)(__PTR) > 0)
#endif

static astn spec(ccEnv s/* , int qual */) {
	astn tok, tag = 0;
	symn tmp, def = 0;

	int offs = 0;
	int size = 0;

	if (skip(s, OPER_op)) {			// operator
		skiptok(s, STMT_do, 1);
	}// */
	if (skip(s, TYPE_def)) {			// define
		symn typ = 0;
		//~ int op = skip(s, OPER_dot);
		if (!(tag = next(s, TYPE_ref))) {
			error(s->s, s->line, "Identifyer expected");
			skiptok(s, STMT_do, 1);
			return NULL;
		}
		else if (skip(s, ASGN_set)) {			// define PI = 3.14...;
			struct astn tmp;
			astn val = expr(s, TYPE_def);
			if (eval(&tmp, val)) {
				def = declare(s, TYPE_def, tag, val->type);
				//~ val = cpynode(s, &tmp);
				def->init = val;
				typ = def->type;
			}
			else {
				error(s->s, val->line, "Constant expression expected, got: '%+k'", val);
				def = declare(s, TYPE_def, tag, 0);
				def->init = val;
			}
		}
		else if (skip(s, PNCT_lp)) {			// define isNan(float64 x) = (x != x);
			symn arg;
			int warnfun = 0;

			def = declare(s, TYPE_def, tag, NULL);
			def->name = NULL;

			enter(s, NULL);
			args(s, 0);
			skiptok(s, PNCT_rp, 1);
			def->name = tag->id.name;

			if (def && skip(s, ASGN_set)) {		// define sqr(int a) = (a * a);
				char* name = def->name;
				def->name = 0;					// exclude recursive inlineing
				if ((def->init = expr(s, TYPE_def))) {
					if (def->init->kind != OPER_fnc) {
						astn tmp = newnode(s, OPER_fnc);
						warnfun = 1;
						tmp->type = def->init->type;
						tmp->csts = def->init->csts;
						tmp->line = def->init->line;
						tmp->op.rhso = def->init;
						def->init = tmp;
						if (!typecheck(s, 0, tmp)) {	//TODO: find a nother way to do this
							error(s->s, tmp->line, "invalid expression: %+k", tmp);
						}
					}
					typ = def->init->type;
				}
				def->name = name;
			}

			def->args = leave(s, NULL);
			def->type = typ;
			//~ def->cast = typ->cast;
			def->call = 1;
			if (def->args == NULL)
				def->args = s->argz->id.link;

			for (arg = def->args; arg; arg = arg->next) {
				if (arg->cast == TYPE_ref)
					arg->cast = arg->type->cast;
				else
					arg->cast = TYPE_def;
			}

			/*if (op && def->name) {
				int oper, argc = 0;
				for (tmp = def->args; tmp; tmp = tmp->next) {
					argc += 1;
				}
				for (oper = OPER_idx; oper < OPER_com; ++oper) {
					const char* cmpTo = tok_tbl[oper].name;

					if (!cmpTo || cmpTo[0] != '.')
						continue;

					if (argc != tok_tbl[oper].argc)
						continue;

					if (strcmp(def->name, cmpTo + 1) != 0)
						continue;

					def->name = (char*)cmpTo;
					op = 0;
					break;
				}
			}// */

			if (warnfun) {
				warn(s->s, 8, s->file, tag->line, "%+k should be a function", tag);
				info(s->s, s->file, tag->line, "defined as: %-T", def);
			}
		}
		else if ((typ = type(s))) {				// define sin math.sin;	???
			def = declare(s, TYPE_def, tag, typ);
		}
		else error(s->s, tag->line, "typename excepted");
		//~ if (op) error(s->s, tag->line, "invalid operator");
		skiptok(s, STMT_do, 1);

		tag->kind = TYPE_def;
		tag->type = typ;
		tag->id.link = def;

		redefine(s, def);
	}// */
	else if (skip(s, TYPE_rec)) {		// struct
		int pack = 4;
		if (!(tag = next(s, TYPE_ref))) {	// name?
			tag = newnode(s, TYPE_ref);
			//~ tag->type = type_vid;
			tag->line = s->line;
		}
		if (skip(s, PNCT_cln)) {			// pack?
			tok = NULL; //next(s, TYPE_int);
			if (1) {
				struct astn ift;
				tok = expr(s, TYPE_int);
				if (eval(&ift, tok) == TYPE_int) {
					tok = &ift;
				}
			}

			if (tok && tok->kind == TYPE_int) {
				switch ((int)constint(tok)) {
					default:
						error(s->s, s->line, "alignment must be a small power of two, not %k", tok);
						break;
					case  0: pack =  0; break;
					case  1: pack =  1; break;
					case  2: pack =  2; break;
					case  4: pack =  4; break;
					case  8: pack =  8; break;
					case 16: pack = 16; break;
				}
			}
			else {
				error(s->s, s->line, "alignment must be an integer constant", tok);
				//~ return 0;
			}
		}
		if (skip(s, STMT_beg)) {			// body
			int salign = -1;
			def = declare(s, TYPE_rec, tag, type_vid);
			redefine(s, def);
			enter(s, NULL);
			while (!skip(s, STMT_end)) {
				symn typ = type(s);
				tok = next(s, TYPE_ref);

				//~ /* this is optional:
				// id ':' typename | struct | enum '{' ... '}'

				if (typ) {
					//~ skiptok(s, STMT_do, 1);
				}
				else if (tok && skip(s, PNCT_cln)) {		// var: typename | struct | enum {...}
					astn nn = spec(s);
					if (nn == NULL) {
						typ = type(s);
						if (typ == NULL) {
							return 0;
						}
					}
					else {
						dieif(nn->kind == TYPE_ref, "FixMe");
						typ = nn->type;//id.link;
					}
					//~ skip(s, STMT_do);
				}
				else {
					error(s->s, s->line, "declaration expected ");
				} // */

				if (tok && typ) {
					symn ref = 0;
					int align = (typ->pack && (pack < typ->pack)) ? pack : typ->pack;
					offs = padded(offs, align);
					ref = declare(s, TYPE_ref, tok, typ);
					skiptok(s, STMT_do, 1);
					redefine(s, ref);
					ref->offs = offs;
					offs += typ->size;
					if (size < offs) size = offs;
					if (salign < typ->pack)
						salign = typ->pack;

					s->doc = (char*)-1;
					//if (peek(s) && (itn)s->doc > 0) {
					if (peek(s) && ptrgtz(s->doc)) {
						ref->pfmt = s->doc;
						s->doc = 0;
					}
				}
				else {
					error(s->s, s->line, "declaration expected in[%T]", def);
					if (!skiptok(s, STMT_do, 0))
						break;
				}
			}
			def->size = size;
			def->cast = TYPE_rec;	// TODO: RemMe
			def->args = leave(s, def);

			if (def->args) {
				def->pack = salign;
				//~ debug("Align(%T): %d", def, def->algn);
			}

			if (!def->args && !def->type)
				warn(s->s, 2, s->file, s->line, "empty declaration");
		}
		tag->type = def;
	}
	//else if (skip(s, TYPE_cls));		// class
	else if (skip(s, TYPE_enu)) {		// enum
		symn base = type_i32;			// bydef

		tag = next(s, TYPE_ref);
		if (skip(s, PNCT_cln)) {			// inherit, typeof enumerations
			tok = next(s, TYPE_ref);
			if (tok != NULL) {
				// TODO: what's this ?
				base = typecheck(s, NULL, tok);
				if (base != tok->id.link) {		// it is not a type, or typedef
					base = NULL;
				}
			}

			if (!base) {
				error(s->s, s->line, "typename expected");
				base = type_i32;
			}
		}

		skiptok(s, STMT_beg, 1);
		if (tag) {
			def = declare(s, TYPE_enu, tag, base);
			enter(s, NULL);
		}
		else {
			tag = newnode(s, TYPE_enu);
			tag->type = base;
		}

		while (!skip(s, STMT_end)) {
			if (test(s, TYPE_def)) {
				spec(s);
				continue;
			}
			if ((tok = next(s, TYPE_ref))) {
				tmp = declare(s, TYPE_def, tok, base);
				if (skip(s, ASGN_set)) {
					tmp->init = expr(s, TYPE_def);
					if (!promote(tmp->init->type, base))
						error(s->s, tmp->init->line, "%T constant expected, got %T", base, tmp->init->type);
				}
				else if (base->cast == TYPE_i32) {		// if casts to TYPE_i32
					tmp->init = newnode(s, TYPE_int);
					tmp->init->con.cint = offs;
					tmp->init->type = base;
					offs += 1;
				}
				else
					error(s->s, tmp->line, "%T constant expected", base);
				redefine(s, tmp);
			}
			skiptok(s, STMT_do, 1);
		}
		if (def) {
			def->args = leave(s, def);
			redefine(s, def);
		}
	}
	return tag;
}

//}

int parse(ccEnv cc, srcType mode) {

	//~ astn ast;

	if (mode)
		return -1;

	unit(cc, 0);
	//~ while ((ast = next(cc, 0))) {fputfmt(stdout,"%k", ast);if (ast->kind == STMT_do) fputfmt(stdout,"\n"); }
	//~ while ((ast = next(cc, 0))) {info(cc->s, cc->file, ast->line, "%k", ast);}

	/*switch (mode) {
		case srcUnit: unit(cc, 0); break;
		case srcDecl: decl(cc, 0); break;
		case srcExpr: expr(cc, TYPE_vid); break;
	}//*/
	return ccDone(cc->s);
}

/** unit -----------------------------------------------------------------------
 * scan a source
 * @param mode: script mode (enable non declaration statements)
 *
 * unit:
 *	| ('module' <name> ';') <stmt>*
 *	| <stmt>*
 *	;
**/
/** stmt -----------------------------------------------------------------------
 * scan a statement (mode ? enable decl : do not)
 * @param mode: enable or not <decl>, enetr new scope

 * stmt: ';'
 *	| '{' <stmt>* '}'
 *	| ('static')? 'if' '(' <decl> | <expr> ')' <stmt> ('else' <stmt>)?
 *	| ('static' | 'parallel')? 'for' '(' (<decl> | <expr>)?; <expr>?; <expr>? ')' <stmt>
 *	| mode && <decl>
 *	| <expr> ';'
 *	;

 TODO:
 +	| 'return' <expr>? ';'
 +	| 'continue' ';'
 +	| 'break' ';'
**/
/** decl -----------------------------------------------------------------------
 * declarations
 * @param mode: enable or not <spec>.
 * decl := (mode && <spec>)

 * # variable or array
 * | <type> <name> ('['<expr>?']')? (= <expr>)? ';'
 
 * # function
 * | <type> <name> '(' <args> ')' (('{' <stmt>* '}') | ('=' <name> ';') | (';'))
 * ;

 * spec :=
 * # operator
 * | 'operator' <OP> ('(' <type> rhs ')') '=' <expr> ';'
 * | 'operator' ('(' <type> lhs ')') <OP> ('(' <type> rhs ')') '=' <expr> ';'
   // operator - (Complex x) = Complex(-x.re, -x.im);
   // operator (Complex x) - (Complex y) = Complex(x.re - y.re, x.im - y.im);

 * # define
 *	| 'define' <name> <type>;						// typedef
 *	| 'define' <name> ('(' <args> ')')? = <expr>;	// inline

 * # struct, union enum
 *	| 'struct' <name>? '{' <decl>* '}'
 *	| 'enum' <name>? (':' <type>)? '{' (<name> (= <expr>)?;)+ '}'
 *	;
 * 
 * type := typename ('.' typename)*
 * args := <type>? <name> (',' <type>? <name>)*
 * spec := define | struct | enum | ...
**/
/** expr -----------------------------------------------------------------------
 * expression
 * expr := ID
 *	| expr '[' expr ']'		// \arr	array
 ?	| '[' expr ']'				// \ptr	pointer
 *	| expr '(' expr? ')'		// \fnc	function
 *	| '(' expr ')'				// \pri	precedence
 *	| ['+', '-', '~', '!'] expr	// \uop	unary
 *	| expr ['+', '*', ...] expr	// \bop	binary
 *	| expr '?' expr ':' expr	// \top	ternary
 *	;
**/
