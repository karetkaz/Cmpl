#include <unistd.h>

#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include "main.h"

//{~~~~~~~~~ Lexer ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~ export astn peek(state);
//~ export astn next(state, int kind);
//~ export void back(state, astn ast);
//~ export bool skip(state, int kind);
//~ export bool test(state, int kind);

int fill_buf(state s);

int source(state s, LoadType mode, char* file) {
	if (s->_fin > 3)
		close(s->_fin);
	s->_fin = -1;
	s->_chr = -1;
	s->_ptr = 0;
	s->_cnt = 0;
	s->_tok = 0;
	//~ s->file = 0;
	s->line = 0;
	//~ s->nest = 0;

	switch (mode) {
		case source_none: break;
		case source_file: {
			if ((s->_fin = open(file, O_RDONLY)) <= 0) return -1;
			s->file = file;
			s->line = 1;
		} break;
		case source_buff: {
			s->_cnt = strlen(file);
			s->_ptr = file;
		} break;
		default: fatal(s, "source");
	}
	fill_buf(s);
	return 0;
}

int fill_buf(state s) {
	if (s->_fin >= 0) {
		unsigned char* base = s->_buf + s->_cnt;
		int l, size = sizeof(s->_buf) - s->_cnt;
		memcpy(s->_buf, s->_ptr, s->_cnt);
		s->_cnt += l = read(s->_fin, base, size);
		if (l == 0) {
			s->_buf[s->_cnt] = 0;
			close(s->_fin);
			s->_fin = -1;
		}
		s->_ptr = (char*)s->_buf;
	}
	//~ print(0, ERROR, __FILE__, __LINE__, "fill_buff():%d", s->_cnt);
	return s->_cnt;
}

int read_chr(state s) {
	if (s->_chr != -1) {
		int chr = s->_chr;
		s->_chr = -1;
		return chr;
	}
	if (s->_cnt <= 4 && fill_buf(s) <= 0) {
		return -1;
	}
	while (*s->_ptr == '\\' && (s->_ptr[1] == '\n' || s->_ptr[1] == '\r')) {
		if (s->_ptr[1] == '\r' && s->_ptr[2] == '\n') {
			s->_cnt -= 3;
			s->_ptr += 3;
		} else {
			s->_ptr += 2;
			s->_cnt -= 2;
		}
		if (s->_cnt <= 4 && !fill_buf(s)) {
			warn(s, 9, s->file, s->line, "backslash-newline at end of file");
			return -1;
		}
		s->line += s->line != 0;
	}
	if (*s->_ptr == '\r' || *s->_ptr == '\n') {
		if (s->_ptr[0] == '\r' && s->_ptr[1] == '\n')
			--s->_cnt, s->_ptr++;
		--s->_cnt, s->_ptr++;
		s->line += s->line != 0;
		return '\n';
	}
	return *(--s->_cnt, s->_ptr++);
}

int peek_chr(state s) {
	if (s->_chr == -1)
		s->_chr = read_chr(s);
	return s->_chr;
}

int back_chr(state s, int chr) {
	dieif (s->_chr != -1);
	return s->_chr = chr;
}

/*//~ alloc/realloc/calloc
inline unsigned memlen(list meml) {
	return meml->data - (unsigned char*)meml;
	//~ return (unsigned char*)&meml->data - meml->data;
}

void* newmem(state s, unsigned size, unsigned algn) {
	list mem;
	//~ if (oldp == (void*)-1);	// malloc
	//~ else if (oldp == (void*)0);	// calloc
	//~ else {}			// ralloc
	for (mem = s->free; mem; mem = mem->next) {
		unsigned len = memlen(mem);
		if (size >= len + sizeof(mem)) {
			list seg = (list)(mem->data + len);
			seg->data = mem->data;
			seg->next = s->used;
			s->used = seg;
			mem->data += len + sizeof(mem);
			return seg->data;
		}
	}
	return 0;
}

void delmem(state s, void* memp) {
	list ml;
	//~ if (memp == (void*)-1) return;
	//~ if (memp == (void*)0) return;
	//~ else 
	for (ml = s->used; ml; ml = ml->next) {
		if (ml->data == memp) {
			ml->next = s->free;
			s->free = ml;
			return;
		}
	}
	//~ error()
}
*/

int rehash(register const char* str, unsigned size) {
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

	if (str) while (size-- > 0)
		hs = (hs >> 8) ^ crc_tab[(hs ^ (*str++)) & 0xff];

	return (hs ^ 0xffffffff) % TBLS;
}

/**
 * @param t text to search in
 * @param p lowercase pattern
 * @return a pointer to the match, or NULL if not found.
 * search with wildcard ? and *
 */
char *strfindstr(const char *t, const char *p, int flgs) {
	int i, ic = flgs & 1;

	for (i = 0; *t && p[i]; ++t, ++i) {
		if (p[i] == '*')
			return strfindstr(t, p + i + 1, flgs) ? (char*)t - i : 0;

		if (p[i] == '?' || p[i] == *t)		// skip | match
			continue;

		if (ic && p[i] == tolower(*t))		// ignore case
			continue;

		t -= i;
		i = -1;
	}
	while (p[i] == '*')			// "a***" is "a"
		++p;					// keep i for return

	return p[i] ? 0 : (char*)t - i;
}

int parseint(register const char *str, int *res) {
	int val = 0;
	while (*str >= '0' && *str <= '9') {
		val = val * 10 + (*str - '0');
		str += 1;
	}
	if (*str == 0) {
		*res = val;
		return 1;
	}
	return 0;
}

/* TODO: parsei64, parsef64
int parsei64(register const char *str, int64t *res) {
	int64t val = 0;
	while (*str >= '0' && *str <= '9') {
		val = val * 10 + (*str - '0');
		str += 1;
	}
	if (*str == 0) {
		*res = val;
		return 1;
	}
	return 0;
}
int parsef64(register const char *str, flt64t *res) {
	flt64t val = 0;
	while (*str >= '0' && *str <= '9') {
		val = val * 10 + (*str - '0');
		str += 1;
	}
	if (*str == 0) {
		*res = val;
		return 1;
	}
	return 0;
}
*/

char *mapstr(state s, char *name, unsigned size/* = -1U*/, unsigned hash/* = -1U*/) {

	list newn, next, prev = 0;

	if (size == -1U) size = strlen(name);
	if (hash == -1U) hash = rehash(name, size);
	for (next = s->strt[hash]; next; next = next->next) {
		//~ int slen = next->data - (unsigned char*)next;
		register int c = memcmp(next->data, name, size);
		if (c == 0) return (char*)next->data;
		if (c > 0) break;
		prev = next;
	}

	if (name != s->buffp) {
		debug("lookupstr(name != s->buffp)");
		memcpy(s->buffp, name, size);
		name = s->buffp;
	}

	s->buffp += size+1;
	newn = (struct listn_t*)s->buffp;
	s->buffp += sizeof(struct listn_t);

	if (!prev) s->strt[hash] = newn;
	else prev->next = newn;

	newn->next = next;
	newn->data = (unsigned char *)name;

	return name;
}

int escchr(state s) {
	int chr;
	switch (chr = read_chr(s)) {
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
			if ((chr = peek_chr(s)) >= '0' && chr <= '7') {
				val = (val << 3) | (chr - '0');
				read_chr(s);
				if ((chr = peek_chr(s)) >= '0' && chr <= '7') {
					val = (val << 3) | (chr - '0');
					read_chr(s);
				}
			}
			if (val & 0xffffff00)
				warn(s, 4, s->file, s->line, "octal escape sequence overflow");
			return val & 0xff;
		}

		// hex sequence (len = 2)
		case 'x': {
			int h1 = read_chr(s);
			int h2 = read_chr(s);
			if (h1 >= '0' && h1 <= '9') h1 -= '0';
			else if (h1 >= 'a' && h1 <= 'f') h1 -= 'a' - 10;
			else if (h1 >= 'A' && h1 <= 'F') h1 -= 'A' - 10;
			else {
				error(s, s->line, "hex escape sequence invalid");
				return 0;
			}

			if (h2 >= '0' && h2 <= '9') h2 -= '0';
			else if (h2 >= 'a' && h2 <= 'f') h2 -= 'a' - 10;
			else if (h2 >= 'A' && h2 <= 'F') h2 -= 'A' - 10;
			else {
				error(s, s->line, "hex escape sequence invalid");
				return 0;
			}
			return h1 << 4 | h2;
		}
		default: {
			//~ back_chr(s, chr);
			error(s, s->line, "unknown escape sequence '\\%c'", chr);
		} return -1;
	}
	return 0;
}

int read_tok(state s, astn tok)
//!TODO: remove "debug"
//!TODO: fix readnum change with 2 pass reader
{
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
		/* 200    */	OTHER,
		/* 201 `  */	OTHER,
		/* 202    */	OTHER,
		/* 203 à  */	OTHER,
		/* 204 Ġ  */	OTHER,
		/* 205 Š  */	OTHER,
		/* 206 Ơ  */	OTHER,
		/* 207 Ǡ  */	OTHER,
		/* 210 Ƞ  */	OTHER,
		/* 211 ɠ  */	OTHER,
		/* 212 ʠ  */	OTHER,
		/* 213 ˠ  */	OTHER,
		/* 214 ̠  */	OTHER,
		/* 215 ͠  */	OTHER,
		/* 216 Π  */	OTHER,
		/* 217 Ϡ  */	OTHER,
		/* 220 Р  */	OTHER,
		/* 221 Ѡ  */	OTHER,
		/* 222 Ҡ  */	OTHER,
		/* 223 Ӡ  */	OTHER,
		/* 224 Ԡ  */	OTHER,
		/* 225 ՠ  */	OTHER,
		/* 226 ֠  */	OTHER,
		/* 227 נ  */	OTHER,
		/* 230 ؠ  */	OTHER,
		/* 231 ٠  */	OTHER,
		/* 232 ڠ  */	OTHER,
		/* 233 ۠  */	OTHER,
		/* 234 ܠ  */	OTHER,
		/* 235 ݠ  */	OTHER,
		/* 236 ޠ  */	OTHER,
		/* 237 ߠ  */	OTHER,
		/* 240    */	OTHER,
		/* 241 `  */	OTHER,
		/* 242    */	OTHER,
		/* 243 à  */	OTHER,
		/* 244 Ġ  */	OTHER,
		/* 245 Š  */	OTHER,
		/* 246 Ơ  */	OTHER,
		/* 247 Ǡ  */	OTHER,
		/* 250 Ƞ  */	OTHER,
		/* 251 ɠ  */	OTHER,
		/* 252 ʠ  */	OTHER,
		/* 253 ˠ  */	OTHER,
		/* 254 ̠  */	OTHER,
		/* 255 ͠  */	OTHER,
		/* 256 Π  */	OTHER,
		/* 257 Ϡ  */	OTHER,
		/* 260 Р  */	OTHER,
		/* 261 Ѡ  */	OTHER,
		/* 262 Ҡ  */	OTHER,
		/* 263 Ӡ  */	OTHER,
		/* 264 Ԡ  */	OTHER,
		/* 265 ՠ  */	OTHER,
		/* 266 ֠  */	OTHER,
		/* 267 נ  */	OTHER,
		/* 270 ؠ  */	OTHER,
		/* 271 ٠  */	OTHER,
		/* 272 ڠ  */	OTHER,
		/* 273 ۠  */	OTHER,
		/* 274 ܠ  */	OTHER,
		/* 275 ݠ  */	OTHER,
		/* 276 ޠ  */	OTHER,
		/* 277 ߠ  */	OTHER,
		/* 300    */	OTHER,
		/* 301 `  */	OTHER,
		/* 302    */	OTHER,
		/* 303 à  */	OTHER,
		/* 304 Ġ  */	OTHER,
		/* 305 Š  */	OTHER,
		/* 306 Ơ  */	OTHER,
		/* 307 Ǡ  */	OTHER,
		/* 310 Ƞ  */	OTHER,
		/* 311 ɠ  */	OTHER,
		/* 312 ʠ  */	OTHER,
		/* 313 ˠ  */	OTHER,
		/* 314 ̠  */	OTHER,
		/* 315 ͠  */	OTHER,
		/* 316 Π  */	OTHER,
		/* 317 Ϡ  */	OTHER,
		/* 320 Р  */	OTHER,
		/* 321 Ѡ  */	OTHER,
		/* 322 Ҡ  */	OTHER,
		/* 323 Ӡ  */	OTHER,
		/* 324 Ԡ  */	OTHER,
		/* 325 ՠ  */	OTHER,
		/* 326 ֠  */	OTHER,
		/* 327 נ  */	OTHER,
		/* 330 ؠ  */	OTHER,
		/* 331 ٠  */	OTHER,
		/* 332 ڠ  */	OTHER,
		/* 333 ۠  */	OTHER,
		/* 334 ܠ  */	OTHER,
		/* 335 ݠ  */	OTHER,
		/* 336 ޠ  */	OTHER,
		/* 337 ߠ  */	OTHER,
		/* 340 ࠠ */	OTHER,
		/* 341 ᠠ */	OTHER,
		/* 342 ⠠ */	OTHER,
		/* 343 㠠 */	OTHER,
		/* 344 䠠 */	OTHER,
		/* 345 堠 */	OTHER,
		/* 346 栠 */	OTHER,
		/* 347 砠 */	OTHER,
		/* 350 蠠 */	OTHER,
		/* 351 頠 */	OTHER,
		/* 352 ꠠ */	OTHER,
		/* 353 렠 */	OTHER,
		/* 354 젠 */	OTHER,
		/* 355 𘀠*/	OTHER,
		/* 356  */	OTHER,
		/* 357  */	OTHER,
		/* 360 𠠠*/	OTHER,
		/* 361 񠠠*/	OTHER,
		/* 362 򠠠*/	OTHER,
		/* 363 󠠠*/	OTHER,
		/* 364 𠠠*/	OTHER,
		/* 365 񠠠*/	OTHER,
		/* 366 򠠠*/	OTHER,
		/* 367 󠠠*/	OTHER,
		/* 370 𠠠*/	OTHER,
		/* 371 񠠠*/	OTHER,
		/* 372 򠠠*/	OTHER,
		/* 373 󠠠*/	OTHER,
		/* 374 𠠠*/	OTHER,
		/* 375 񠠠*/	OTHER,
		/* 376 򠠠*/	OTHER,
		/* 377 󠠠*/	OTHER,
	};
	char *beg, *ptr = s->buffp;
	int chr = read_chr(s);

	// skip white spaces and comments
	while (chr != -1) {
		if (chr == '/') {
			int line = s->line;
			int next = peek_chr(s);
			if (next == '/') {
				int prevchr = chr;
				//~ debug("!!!comment.line: /");
				for (read_chr(s); chr != -1; chr = read_chr(s)) {
					//~ debug("%c", chr);
					if (chr == '\n') break;
					prevchr = chr;
				}
				if (chr == -1) warn(s, 9, s->file, line, "no newline at end of file(%c)", prevchr);
				else if (s->line != line+1) warn(s, 9, s->file, line, "multi-line comment");
			}
			else if (next == '*') {
				int c = 0, prev_chr = 0;
				//~ debug("!!!comment.stream: {\n/*");
				for (read_chr(s); chr != -1; chr = read_chr(s)) {
					//~ if (prev_chr) debug("%c", chr);
					if (prev_chr == '*' && chr == '/') break;
					if (prev_chr == '/' && chr == '*' && c++)
						warn(s, 8, s->file, s->line, "\"/ *\" within comment(%d)", c - 1);
					prev_chr = chr;
				}
				if (chr == -1) error(s, line, "unterminated comment2");
				//~ else debug("\n!!!comment.box:}\n");
				chr = read_chr(s);
			}
			else if (next == '+') {
				int l = 1, c = 0, prev_chr = 0;
				//~ debug("!!!comment.block:{\n/+");
				for (read_chr(s); chr != -1; chr = read_chr(s)) {
					//~ if (prev_chr) debug("%c", chr);
					if (prev_chr == '/' && chr == '+' && c++) l++;
					if (prev_chr == '+' && chr == '/' && !--l) break;
					prev_chr = chr;
				}
				if (chr == -1) error(s, line, "unterminated comment1");
				//~ else debug("\n!!!comment.block:}\n");
				chr = read_chr(s);
			}
		}
		if (chr == 0) {
			warn(s, 5, s->file, s->line, "null character(s) ignored");
			while (!chr) chr = read_chr(s);
		}

		/*if (chr == '#') {
			char tmp[1024], *ptr = tmp;
			for (chr = read_chr(s); chr != -1; chr = read_chr(s)) {
				if (ptr < tmp + 1024)
					*ptr++ = chr;
				if (chr == '\n') break;
			}
			*ptr = 0;
			if (tmp[0] == '@') {
				for (ptr = tmp + 1; *ptr; ptr ++) {
					if (*ptr == '/') break;
				}
				if (*ptr && parseint(ptr + 1, &s->line)) {
					s->line += 1;
					debug("%d", s->line);
				}

				*ptr = 0;

				s->file = mapstr(s, tmp + 1, ptr - tmp + 1, -1);
				//~ debug("%s", tmp + 1);
				//~ debug("#pragma line ", tmp + 1);
			}
		}*/

		if (!(chr_map[chr & 0xff] & SPACE)) break;
		chr = read_chr(s);
	}
	if (chr == -1) return 0;

	memset(tok, 0, sizeof(*tok));
	tok->kind = TOKN_err;
	tok->line = s->line;
	// in case comments were recorded
	ptr = beg = s->buffp;

	// scan
	switch (chr) {
		default: {
			if (chr_map[chr & 0xff] & (ALPHA|UNDER)) goto read_idf;
			if (chr_map[chr & 0xff] & DIGIT) goto read_num;

			// control(<32) or non ascii character(>127) ?
			error(s, s->line, "error reading char: '%c'", chr);
			tok->kind = TOKN_err;
		} break;
		case '.': {
			if (chr_map[peek_chr(s) & 0xff] == DIGIT) goto read_num;
			tok->kind = OPER_dot;
		} break;
		case ';': {
			tok->kind = OPER_nop;
		} break;
		case ',': {
			tok->kind = OPER_com;
		} break;
		case '{': {		// beg?
			tok->kind = OPER_beg;
		} break;
		case '[': {		// idx?
			tok->kind = PNCT_lc;
		} break;
		case '(': {		// fnc?
			tok->kind = PNCT_lp;
		} break;
		case '}': {		// end?
			tok->kind = OPER_end;
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
			if (peek_chr(s) == '=') {
				read_chr(s);
				tok->kind = ASGN_set;
			}
			else tok->kind = PNCT_cln;
		} break;
		case '~': {
			tok->kind = OPER_cmt;
		} break;
		case '=': {
			if (peek_chr(s) == '=') {
				read_chr(s);
				tok->kind = OPER_equ;
			}
			else tok->kind = ASGN_set;
		} break;
		case '!': {
			if (peek_chr(s) == '=') {
				read_chr(s);
				tok->kind = OPER_neq;
			}
			else tok->kind = OPER_not;
		} break;
		case '>': {
			chr = peek_chr(s);
			if (chr == '>') {
				read_chr(s);
				if (peek_chr(s) == '=') {
					read_chr(s);
					tok->kind = ASGN_shr;
				}
				else tok->kind = OPER_shr;
			}
			else if (chr == '=') {
				read_chr(s);
				tok->kind = OPER_geq;
			}
			/*else if (chr == '?') {
				read_chr(s);
				tok->kind = OPER_max;
			}*/
			else tok->kind = OPER_gte;
		} break;
		case '<': {
			chr = peek_chr(s);
			if (chr == '<') {
				read_chr(s);
				if (peek_chr(s) == '=') {
					read_chr(s);
					tok->kind = ASGN_shl;
				}
				else tok->kind = OPER_shl;
			}
			else if (chr == '=') {
				read_chr(s);
				tok->kind = OPER_leq;
			}
			/*else if (chr == '?') {
				read_chr(s);
				tok->kind = OPER_min;
			}*/
			else tok->kind = OPER_lte;
		} break;
		case '&': {
			chr = peek_chr(s);
			if (chr == '=') {
				read_chr(s);
				tok->kind = ASGN_and;
			}
			else if (chr == '&') {
				read_chr(s);
				tok->kind = OPER_lnd;
			}
			else tok->kind = OPER_and;
		} break;
		case '|': {
			chr = peek_chr(s);
			if (chr == '=') {
				read_chr(s);
				tok->kind = ASGN_ior;
			}
			else if (chr == '|') {
				read_chr(s);
				tok->kind = OPER_lor;
			}
			else tok->kind = OPER_ior;
		} break;
		case '^': {
			if (peek_chr(s) == '=') {
				read_chr(s);
				tok->kind = ASGN_xor;
			}
			else tok->kind = OPER_xor;
		} break;
		case '+': {
			int chr = peek_chr(s);
			if (chr == '=') {
				read_chr(s);
				tok->kind = ASGN_add;
			}
			/*else if (chr == '+') {
				read_chr(s);
				tok->kind = OPER_inc;
			}*/
			else tok->kind = OPER_pls;
		} break;
		case '-': {
			chr = peek_chr(s);
			if (chr == '=') {
				read_chr(s);
				tok->kind = ASGN_sub;
			}
			/*else if (chr == '-') {
				read_chr(s);
				tok->kind = OPER_dec;
			}*/
			else tok->kind = OPER_mns;
		} break;
		case '*': {
			chr = peek_chr(s);
			if (chr == '=') {
				read_chr(s);
				tok->kind = ASGN_mul;
			}
			/* else if (chr == '*') {
				read_chr(s);
				if (peek_chr(s) == '=') {
					read_chr(s);
					tok->kind = ASGN_pow;
				}
				else tok->kind = OPER_pow;
			} */
			else tok->kind = OPER_mul;
		} break;
		case '/': {
			if (peek_chr(s) == '=') {
				read_chr(s);
				tok->kind = ASGN_div;
			}
			else tok->kind = OPER_div;
		} break;
		case '%': {
			if (peek_chr(s) == '=') {
				read_chr(s);
				tok->kind = ASGN_mod;
			}
			else tok->kind = OPER_mod;
		} break;
		case '\'': {		// \'[^\'\n]*
			int val = 0;
			while ((chr = read_chr(s)) != -1) {
				if (chr == '\'') break;
				else if (chr == '\n') break;
				else if (chr == '\\') chr = escchr(s);
				val = val << 8 | chr;
				*ptr++ = chr;
			}
			*ptr = 0;
			if (chr != '\'') {					// error
				error(s, s->line, "unclosed character constant ('\'' missing)");
				return tok->kind = TOKN_err;
			}
			if (ptr == beg) {
				error(s, s->line, "empty character constant");
				return tok->kind = TOKN_err;
			}
			else if (ptr > beg + sizeof(int)) warn(s, 1, s->file, s->line, "character constant truncated");
			else if (ptr > beg + sizeof(char)) warn(s, 5, s->file, s->line, "multi character constant");
			tok->cint = val;
			tok->type = type_i64;
			return tok->kind = CNST_int;
		} break;
		case '\"': {		// \"[^\"\n]*
			while ((chr = read_chr(s)) != -1) {
				if (chr == '\"') break;
				else if (chr == '\n') break;
				else if (chr == '\\') chr = escchr(s);
				*ptr++ = chr;
			}
			*ptr = 0;
			if (chr != '\"') {					// error
				error(s, s->line, "unclosed string constant ('\"' missing)");
				return tok->kind = TOKN_err;
			}
			tok->type = type_str;
			tok->hash = rehash(beg, ptr - beg);
			tok->name = mapstr(s, beg, ptr - beg, tok->hash);
			return tok->kind = CNST_str;
		} break;
		read_idf: {		// [a-zA-Z_][a-zA-Z0-9_]*
			while (chr != -1) {
				if (!(chr_map[chr & 0xff] & CWORD)) break;
				*ptr++ = chr;
				chr = read_chr(s);
			}
			*ptr = 0;
			back_chr(s, chr);
			if (!strcmp(beg, "__LINE__")) {
				tok->kind = CNST_int;
				tok->type = type_i32;
				tok->cint = s->line;
				return CNST_int;
			}
			else if (!strcmp(beg, "__FILE__")) {
				//~ return (*tok = s->ccfn).kind;
				int len = strlen(s->file);
				tok->type = type_str;
				tok->hash = rehash(s->file, len);
				tok->name = mapstr(s, beg, len, tok->hash);
				return tok->kind = CNST_str;
			}
			else if (!strcmp(beg, "__DEFN__")) {
				ptr = beg;
				for(chr = 0; chr <= s->nest; chr += 1) {
					symn sym = s->scope[chr].csym;
					if (sym && sym->name) {
						char* src = sym->name;
						if (ptr != beg) ptr[-1] = '.';
						while ((*ptr++ = *src++)) ;
					}
				}
				tok->type = type_str;
				tok->hash = rehash(beg, ptr - beg);
				tok->name = mapstr(s, beg, ptr - beg, tok->hash);
				return tok->kind = CNST_str;
			}
			//~ else if (!strcmp(beg, "__TIME__"));
			//~ else if (!strcmp(beg, "__DATE__"));
			else for(chr = 0; chr < tok_last; chr += 1) {
				if (tok_tbl[chr].kind != ID) continue;
				if (strcmp(beg, tok_tbl[chr].name) == 0) {
					tok->kind = chr;
					break;
				}
			}
			if (chr == tok_last) {
				tok->kind = TYPE_ref;
				tok->type = tok->link = 0;
				tok->hash = rehash(beg, ptr - beg);
				tok->name = mapstr(s, beg, ptr - beg, tok->hash);
			}
		} break;
		read_num: {		//!TODO ([0-9]+.[0-9]*| ...)([eE][+-]?[0-9]+)?
			enum {
				get_bin = 0x0001,
				get_hex = 0x0002,
				get_oct = 0x0008,

				got_dot = 0x0010,		// '.' taken
				got_exp = 0x0020,		// 'E' taken
				neg_exp = 0x0040,		// +/- taken
				not_oct = 0x0080,		// probably float

				get_val = 0x0100,		// read something
				val_ovf = 0x0800,		// overflov

				got_flt = got_exp | got_dot,
				exp_err = got_exp + get_val,
				oct_err = get_oct + not_oct,
			} ;int get_type = get_val;
			char *suf;				// suffix
			double flt = 0;				// float value
			int val = 0, exp = 0;			// value
			if (chr == '0') {
				*ptr++ = chr; chr = read_chr(s);
				if (chr == 'b' || chr == 'B') {
					get_type = get_bin | get_val;
					*ptr++ = chr; chr = read_chr(s);
				}
				else if (chr == 'x' || chr == 'X') {
					get_type = get_hex | get_val;
					*ptr++ = chr; chr = read_chr(s);
				}
				else get_type = get_oct;
			}
			while (chr != -1) {
				if (get_type & (get_bin | get_hex)) {
					if (get_type & get_bin) {
						if (chr >= '0' && chr <= '1') {
							if (val & 0x80000000) get_type |= val_ovf;
							val = (val << 1) | (chr - '0');
							get_type &= ~get_val;
						}
						else break;
					}
					else if (chr >= '0' && chr <= '9') {
						if (val & 0xF0000000) get_type |= val_ovf;
						val = (val << 4) | (chr - '0');
						get_type &= ~get_val;
					}
					else if (chr >= 'a' && chr <= 'f') {
						if (val & 0xF0000000) get_type |= val_ovf;
						val = (val << 4) | (chr - 'a' + 10);
						get_type &= ~get_val;
					}
					else if (chr >= 'A' && chr <= 'F') {
						if (val & 0xF0000000) get_type |= val_ovf;
						val = (val << 4) | (chr - 'A' + 10);
						get_type &= ~get_val;
					}
					else break;
				}
				else if (chr >= '0' && chr <= '9') {
					if (get_type & got_exp) {
						exp = exp * 10 + chr - '0';
					}
					else {
						if (get_type & got_dot) val -= 1;
						else {
							if (get_type & get_oct) {
								if (chr >= '0' && chr <= '7') {
									if (val & 0xE0000000) get_type |= val_ovf;
									val = (val << 3) + (chr - '0');
								}
								else get_type |= not_oct;
							}
							else {
								if (val != flt) get_type |= val_ovf;
								val = val * 10 + chr - '0';
							}
						}
						flt = flt * 10 + chr - '0';
					}
					get_type &= ~get_val;
				}
				else if (chr == 'e' || chr == 'E') {
					if (get_type & (get_val | got_exp)) break;
					if (!(get_type & got_dot)) val = 0;
					get_type |= got_exp | get_val;
					switch (peek_chr(s)) {
						case '-': get_type |= neg_exp;
						case '+': *ptr++ = chr; chr = read_chr(s);
					}
				}
				else if (chr == '.') {
					if (get_type & (got_dot | got_exp)) break;
					get_type |= got_dot;
					val = 0;
				}
				else break;
				*ptr++ = chr;
				chr = read_chr(s);
			}
			suf = ptr;
			while (chr != -1) {
				if (chr == '.' || chr == '_') ;
				else if (chr >= '0' && chr <= '9') ;
				else if (chr >= 'a' && chr <= 'z') ;
				else if (chr >= 'A' && chr <= 'Z') ;
				else {*ptr = 0; break;}
				*ptr++ = chr;
				chr = read_chr(s);
			}
			back_chr(s, chr);
			if (*suf || (get_type & get_val)) {	// error
				error(s, tok->line, "parse error in numeric constant \"%s\"", beg);
				if ((get_type & exp_err) == exp_err) error(s, tok->line, "exponent has no digits");
				else if (*suf) error(s, tok->line, "invalid suffix in numeric constant '%s'", suf);
				return tok->kind;
			}
			if (get_type & (got_flt)) {		// float
				tok->kind = CNST_flt;
				//~ tok->type = type_f64;
				tok->cflt = flt * pow(10, (get_type & neg_exp ? -exp : exp) + val);
				//~ return CNST_flt;
			}
			else {					// integer
				if ((get_type & oct_err) == oct_err) {
					error(s, s->line, "parse error in octal constant \"%s\"", beg);
					return tok->kind = TOKN_err;
				}
				if (get_type & val_ovf) {
					warn(s, 2, s->file, tok->line, "integer constant '%s' is too large", beg);
				}
				tok->kind = CNST_int;
				//~ tok->type = type_i32;
				tok->cint = val;
				//~ return CNST_int;
			}
		} break;
	}
	return tok->kind;
}

astn peek(state s) {
	if (s->_tok == 0) {
		s->_tok = newnode(s, 0);
		if (!read_tok(s, s->_tok)) {
			//~ fputfmt(stdout, "%s:%d:%k\n", s->file, s->line, s->_tok);
			eatnode(s, s->_tok);
			s->_tok = 0;
		}
	}
	return s->_tok;
}

astn next(state s, int kind) {
	if (!peek(s)) return 0;
	if (!kind || s->_tok->kind == kind) {
		astn ast = s->_tok;
		s->_tok = ast->next;
		ast->next = 0;
		return ast;
	}
	return 0;
}

void back(state s, astn ast) {
	ast->next = s->_tok;
	s->_tok = ast;
}

int test(state s, int kind) {
	astn ast = peek(s);
	if (!ast || (kind && ast->kind != kind))
		return 0;
	return 1;
}

int skip(state s, int kind) {
	astn ast = peek(s);
	if (!ast || (kind && ast->kind != kind))
		return 0;
	s->_tok = ast->next;
	eatnode(s, ast);
	return 1;
}
//}

//{~~~~~~~~~ Parser ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~ export astn scan(state s, int mode);
//~ export astn stmt(state s, int mode);
//~ export astn decl(state s, int mode);
//~ export astn expr(state s, int mode);

astn scan(state s, int mode) {
	astn tok, lst = 0, ast = 0;

	#if 0		// read ahead all tokens or not
	while (read_tok(s, tok = newnode(s, 0))) {
		if (!ast) s->_tok = tok;
		else ast->next = tok;
		ast = tok;
	}
	if (s->errc)
		return 0;
	ast = 0;
	#endif

	trace(+32, "enter:scan(%?k)", peek(s));

	//~ enter(s, newNode(s, FILE_new));
	enter(s, 0);
	while (peek(s)) {
		if ((tok = stmt(s, 0))) {
			if (!ast) ast = lst = tok;
			else lst = lst->next = tok;
			if (mode && tok->kind != OPER_nop)
				error(s, ast->line, "declaration expected");
		}
	}
	s->glob = leave(s, NULL);

	if (s->nest)
		error(s, s->line, "premature end of file");
	trace(-32, "leave:scan('%k')", peek(s));

	if (ast) {
		astn tmp = newnode(s, OPER_beg);
		tmp->name = s->file;
		tmp->rhso = ast;
		tmp->lhso = lst;
		ast = tmp;
	}

	return ast;
}

astn stmt(state s, int mode) {
	astn ast, tmp = peek(s);
	int qual = 0;// qual(s, mode);			// static | const | parallel
	trace(+16, "enter:stmt('%k')", peek(s));

	if (skip(s, OPER_end)) return 0;			// ;

	if (skip(s, OPER_nop)) ast = 0;			// ;
	else if ((ast = next(s, OPER_beg))) {	// {...}
		astn end = 0;
		//~ ast->cast = qual;
		//~ ast->cast = TYPE_vid;
		enter(s, 0);
		while (peek(s) && !skip(s, OPER_end)) {
			if (!peek(s)) break;
			if ((tmp = stmt(s, 0))) {
				if (!end) ast->stmt = tmp;
				else end->next = tmp;
				end = tmp;
			}
		}
		ast->type = leave(s, NULL);
		if (!ast->stmt) {			// eat sort of {{;{;};{}{}}}
			eatnode(s, ast);
			ast = 0;
		}// */
	}
	else if ((ast = next(s, OPER_jmp))) {	// if (...)
		enter(s, 0);
		ast->cast = qual;
		//~ ast->cast = TYPE_vid;
		// TODO parallel if -> error;
		if (skip(s, PNCT_lp)) {
			ast->test = expr(s, 1);
				//~ ast->test->cast = TYPE_bit;
			//~ if (ast->test) ast->test->cast = TYPE_bit;
			if (skip(s, PNCT_rp)) {
				ast->stmt = stmt(s, 1);
				if (skip(s, OPER_els)) {
					ast->step = stmt(s, 1);
				}
				else ast->step = NULL;
			}
			else error(s, s->line, " ')' expected, not %k", peek(s));
		}
		else error(s, s->line, " '(' expected");
		ast->type = leave(s, NULL);
	}
	else if ((ast = next(s, OPER_for))) {	// for (...)
		enter(s, 0);
		//~ ast->cast = qual;
		//~ ast->cast = TYPE_vid;
		ast->init = 0;
		ast->test = 0;
		ast->step = 0;
		if (!skip(s, PNCT_lp)) {
			error(s, s->line, " '(' expected");
		}
		if (!skip(s, OPER_nop)) {		// init
			ast->init = decl(s, 0);
			if (!ast->init)
				ast->init = expr(s, 0);
			if (!skip(s, OPER_nop))
				error(s, s->line, " ';' expected");
		}
		if (!skip(s, OPER_nop)) {		// test 
			ast->test = expr(s, 1);
			if (!skip(s, OPER_nop)) {
				error(s, s->line, " ';' expected");
			}
		}
		if (!skip(s, PNCT_rp)) {		// incr
			ast->step = expr(s, 1);
			if (!skip(s, PNCT_rp)) {
				error(s, s->line, " ')' expected");
			}
		}
		ast->stmt = stmt(s, 1);
		ast->type = leave(s, NULL);
	}
	else if ((ast = decl(s, qual))) {		// type?
		astn tmp = newnode(s, OPER_nop);
		tmp->line = ast->line;
		tmp->stmt = ast;
		ast = tmp;
		//~ ast->cast = qual;
		//~ ast->cast = TYPE_vid;

		if (ast->type && mode)
			error(s, s->line, "unexpected declaration");
	}
	else if ((ast = expr(s, 0))) {			// expr;
		astn tmp = newnode(s, OPER_nop);
		tmp->line = ast->line;
		tmp->stmt = ast;
		ast = tmp;
		//~ ast->cast = qual;
		//~ ast->cast = TYPE_vid;

		if (!skip(s, OPER_nop)) {
			error(s, s->line, " ';' expected, not `%k`", peek(s));
			while (peek(s) && !skip(s, OPER_nop)) {
				if (skip(s, OPER_end)) return 0;
				skip(s, 0);
			}
		}
	}
	else {
		fatal(s, __FILE__, __LINE__, "00dfskj-ssx");
	}

	trace(-16, "leave:stmt('%+k')", ast);
	//~ if (ast) debug("%+k", ast->stmt);
	return ast;
}

symn spec(state s, int qual);
symn type(state s, int qual);
astn dvar(state s, symn typ, int qual);

astn decl(state s, int qual) {
	symn typ;
	if ((typ = spec(s, qual))) {
		astn ast = newnode(s, TYPE_def);
		ast->type = typ;
		ast->name = typ->name;
		ast->cast = TYPE_def;
		//~ debug("%+T", typ);
		//~ debug("%+k", ast);
		return ast;
	}
	if ((typ = type(s, qual))) {
		astn ast = dvar(s, typ, qual);
		if (ast)
			ast->type = typ;
		//~ debug("%+k", ast);
		return ast;
	}
	return NULL;
}

astn expr(state s, int mode) {
	astn buff[TOKS], *base = buff + TOKS;
	astn *post = buff, *prec = base, tok;
	char sym[TOKS];							// symbol stack {, [, (, ?
	int level = 0, unary = 1;						// precedence, top of sym , start in unary mode
	sym[level] = 0;

	trace(+2, "enter:expr('%k')", peek(s));
	while ((tok = next(s, 0))) {					// parse
		int pri = level << 4;
		switch (tok->kind) {
			tok_id: {
				*post++ = tok;
			} break;
			tok_op: {
				int oppri = tok_tbl[tok->kind].type;
				tok->pril = pri | (oppri & 0x0f);
				if (oppri & 0x10) while (prec < buff + TOKS) {
					if ((*prec)->pril <= tok->pril)
						break;
					*post++ = *prec++;
				}
				else while (prec < buff + TOKS) {
					if ((*prec)->pril < tok->pril)
						break;
					*post++ = *prec++;
				}
				if (tok->kind != OPER_nop) {
					*--prec = tok;
				}
			} break;
			default: {
				if (tok_tbl[tok->kind].argc) {			// operator
					if (unary) {
						*post++ = 0;
						error(s, tok->line, "syntax error before '%k'", tok);
					}
					unary = 1;
					goto tok_op;
				} else {
					if (!unary)
						error(s, tok->line, "syntax error before '%k'", tok);
					unary = 0;
					goto tok_id;
				}
			} break;
			case PNCT_lp: {			// '('
				if (unary)			// a + (3*b)
					*post++ = 0;
				else if (skip(s, PNCT_rp)) {	// a()
					*post++ = 0;
					//~ tok->kind = OPER_fnc;
					//~ goto tok_op;
				}
				//~ else			// a(...)
				tok->kind = OPER_fnc;
				sym[++level] = '(';
				unary = 1;
			} goto tok_op;
			case PNCT_rp: {			// ')'
				if (!unary && sym[level] == '(') {
					tok->kind = OPER_nop;
					level -= 1;
				}
				else if (level == 0) {
					back(s, tok);
					tok = 0;
					break;
				}
				else {
					error(s, tok->line, "syntax error before ')' token");
					break;
				}
				unary = 0;
			} goto tok_op;
			case PNCT_lc: {			// '['
				if (!unary)
					tok->kind = OPER_idx;
				else error(s, tok->line, "syntax error before '[' token");
				sym[++level] = '[';
				unary = 1;
			} goto tok_op;
			case PNCT_rc: {			// ']'
				if (!unary && sym[level] == '[') {
					tok->kind = OPER_nop;
					level -= 1;
				}
				else if (!level) {
					back(s, tok);
					tok = 0;
					break;
				}
				else {
					error(s, tok->line, "syntax error before ']' token");
					break;
				}
				unary = 0;
			} goto tok_op;
			case PNCT_qst: {			// '?'
				if (unary) {
					*post++ = 0;		// ???
					error(s, tok->line, "syntax error before '?' token");
				}
				tok->kind = OPER_sel;
				sym[++level] = '?';
				unary = 1;
			} goto tok_op;
			case PNCT_cln: {			// ':'
				if (!unary && sym[level] == '?') {
					tok->kind = OPER_nop;
					level -= 1;
				}
				else if (level == 0) {
					back(s, tok);
					tok = 0;
					break;
				}
				else {
					error(s, tok->line, "syntax error before ':' token");
					break;
				}
				unary = 1;
			} goto tok_op;
			case OPER_pls: {			// '+'
				if (!unary)
					tok->kind = OPER_add;
				unary = 1;
			} goto tok_op;
			case OPER_mns: {			// '-'
				if (!unary)
					tok->kind = OPER_sub;
				unary = 1;
			} goto tok_op;
			case OPER_not: {			// '!'
				if (!unary)
					error(s, tok->line, "syntax error before '!' token");
				unary = 1;
			} goto tok_op;
			case OPER_cmt: {			// '~'
				if (!unary)
					error(s, tok->line, "syntax error before '~' token");
				unary = 1;
			} goto tok_op;

			case OPER_com:				// ','
				if (level || mode != TYPE_ref) {
					if (unary)
						error(s, tok->line, "syntax error before ',' token");
					unary = 1;
					goto tok_op;
				}
				// else fall
			case OPER_jmp:				// 'if'
			case OPER_for:				// 'for'
			case OPER_els:				// 'else'
			case OPER_beg:				// '{'
			case OPER_end:				// '}'
			case OPER_nop:				// ';'
				back(s, tok);
				tok = 0;
				break;
		}
		if (!tok) break;
		if (post >= prec) {
			error(s, s->line, "Expression too complex");
			return 0;
		}
	}
	if (unary || level) {							// error
		char c = level ? sym[level] : 0;
		error(s, s->line, "missing %s", c == '(' ? "')'" : c == '[' ? "']'" : c == '?' ? "':'" : "expression");
		//~ return 0;
	}
	else if (prec > buff) {							// build
		astn *ptr, *lhs;
		while (prec < buff + TOKS)					// flush
			*post++ = *prec++;
		for (lhs = ptr = buff; ptr < post; ptr += 1) {
			if ((tok = *ptr) == NULL) ;				// skip
			else if (tok_tbl[tok->kind].argc) {		// oper
				int argc = tok_tbl[tok->kind].argc;
				if ((lhs -= argc) < buff) {
					fatal(s, "expr(<overflow>) %k", tok);
					return 0;
				}
				switch (argc) {
					case 1: {
						tok->test = 0;
						tok->lhso = 0;
						tok->rhso = lhs[0];
					} break;
					case 2: {
						tok->test = 0;
						tok->lhso = lhs[0];
						tok->rhso = lhs[1];
					} break;
					case 3: {
						tok->test = lhs[0];
						tok->lhso = lhs[1];
						tok->rhso = lhs[2];
					} break;
					default:
						debug("expr(<%k>)", tok);
				}
				#if TODO_REM_TEMP // TODO: remove replacer
				switch (tok->kind) {				// temporary only cgen
					case ASGN_add: {
						astn tmp = newnode(s, OPER_add);
						tmp->rhso = tok->rhso;
						tmp->lhso = tok->lhso;
						tok->kind = ASGN_set;
						tok->rhso = tmp;
						//~ tmp->next = tok;
						//~ tok->next = tmp;
					} break;
					case ASGN_sub: {
						astn tmp = newnode(s, OPER_sub);
						tmp->rhso = tok->rhso;
						tmp->lhso = tok->lhso;
						tok->kind = ASGN_set;
						tok->rhso = tmp;
					} break;
					case ASGN_mul: {
						astn tmp = newnode(s, OPER_mul);
						tmp->rhso = tok->rhso;
						tmp->lhso = tok->lhso;
						tok->kind = ASGN_set;
						tok->rhso = tmp;
					} break;
					case ASGN_div: {
						astn tmp = newnode(s, OPER_div);
						tmp->rhso = tok->rhso;
						tmp->lhso = tok->lhso;
						tok->kind = ASGN_set;
						tok->rhso = tmp;
					} break;
					case ASGN_mod: {
						astn tmp = newnode(s, OPER_mod);
						tmp->rhso = tok->rhso;
						tmp->lhso = tok->lhso;
						tok->kind = ASGN_set;
						tok->rhso = tmp;
					} break;
					case ASGN_shl: {
						astn tmp = newnode(s, OPER_shl);
						tmp->rhso = tok->rhso;
						tmp->lhso = tok->lhso;
						tok->kind = ASGN_set;
						tok->rhso = tmp;
					} break;
					case ASGN_shr: {
						astn tmp = newnode(s, OPER_shr);
						tmp->rhso = tok->rhso;
						tmp->lhso = tok->lhso;
						tok->kind = ASGN_set;
						tok->rhso = tmp;
					} break;
					case ASGN_ior: {
						astn tmp = newnode(s, OPER_ior);
						tmp->rhso = tok->rhso;
						tmp->lhso = tok->lhso;
						tok->kind = ASGN_set;
						tok->rhso = tmp;
					} break;
				}
				#endif
			}
			/*~ find duplicate sub-trees				// ????
			for (dup = buff; dup < lhs; dup += 1) {
				if (samenode(*dup, tok)) {
					print(0, INFO, s->file, tok->line, "+dupp: %+k", *dup);
					tok->dupn = dup - buff;
					break;
				}
				//~ if (tok) print(0, 0, s->file, tok->line, "+dupp: %+k", *dup);
			} // */
			//~ tok->init = 0;
			*lhs++ = tok;
		}
	}
	trace(-2, "leave:expr('%+k') %k", tok, peek(s));
	if (tok && !lookup(s, 0, tok)) {
		debug("BUM: `%+k`", tok);
		// error(s, tok->line, "bum %+k", tok);
	}
	return tok;
}

extern int align(int offs, int pack, int size);

astn dvar(state s, symn typ, int qual) {
	//~ astn prev = 0, root = 0;
	astn tag = 0;
	symn ref = 0;

	trace(+4, "enter:dclr('%k')", peek(s));

	if (!(tag = next(s, TYPE_ref))) {
		debug("id expected, not %k", peek(s));
		return 0;
	}

	ref = declare(s, TYPE_ref, tag, typ, 0);

	if (skip(s, ASGN_set))
		ref->init = expr(s, TYPE_ref);

	if (test(s, OPER_nop));
	else if (skip(s, OPER_com)) {		// ,
		fatal(s, "multi");
	}
	else if (test(s, PNCT_lp)) {		// function
		fatal(s, "function");
	}
	else if (skip(s, PNCT_lc)) {		// array[]
		struct astn len;
		symn arr = newdefn(s, TYPE_arr);
		arr->init = expr(s, CNST_int);
		if (!skip(s, PNCT_rc)) {
			error(s, s->line, "missing ']'");
			return 0;
		}
		if (eval(&len, arr->init, CNST_int))
			arr->size = typ->size * len.cint;
		arr->type = typ;
		ref->type = arr;
	}

	if (test(s, OPER_nop)) {
		tag->kind = TYPE_def;
		tag->cast = TYPE_ref;
		tag->line = ref->line;
		tag->link = ref;
		return tag;
	}

	error(s, s->line, "unexpected %k", peek(s));

	trace(-4, "leave:dclr('%-k')", root);
	debug("dclr('%+k')", tag);
	return tag;
}// */

symn type(state s, int qual) {
	astn tok = 0;
	symn tmp = 0, def = 0;
	tmp = 0;
	while ((tok = peek(s))) {		// type(.type)*
		if (!lookup(s, tmp, tok)) break;
		if (!istype(tok)) break;
		next(s, 0);
		def = tok->type;
		if (!skip(s, OPER_dot)) break;
		tmp = def;
		//~ def = 0;
	}
	/*if (def) {
		while (skip(s, PNCT_lc)) {			// array	: int [20][30] | int[100](int)
			symn tmp = newdefn(s, TYPE_arr);
			if (!skip(s, PNCT_rc)) {
				tmp->init = expr(s, CNST_int);
				if (!skip(s, PNCT_rc)) {
					error(s, s->line, "missing ']'");
					return tmp;
				}
			}
			tmp->type = def;
			def = tmp;
		}// * /
		if (skip(s, PNCT_lp)) {				// function
			symn tmp = newdefn(s, TYPE_fnc);
			enter(s, 0);
			args(s, fl_type+fl_name);
			leave(s, 1, &tmp->args);
			tmp->type = def;
			def = tmp;
			//declare(s, TYPE_def, newnode(s, TYPE_ref), def, argv);
		}// * /
	}// */
	return def;
}

symn spec(state s, int qual) {
	astn tok, tag = 0;
	symn tmp, def = 0;
	int offs = 0;
	int size = 0;
	int pack = 4;

	trace(+8, "enter:spec('%k')", peek(s));
	if (skip(s, TYPE_def)) {		// define
		//~ define name type			// typedef
		//~ define name = expr			// macro
		// errif(qual != 0);

		qual = 0;
		if ((tag = next(s, TYPE_ref))) {
			if (skip(s, ASGN_set)) {	// define PI = 3.14;
				astn val = expr(s, 0);
				//~ debug("define: %+k", val);
				if (eval(NULL, val, 0)) {
					def = declare(s, TYPE_def, tag, val->type, 0);
					def->init = val;
				}
				else {
					error(s, val->line, "Constant expression expected, got: '%+k'", val);
					def = declare(s, TYPE_def, tag, 0, 0);
					def->type = NULL;
					def->init = val;
				}
				trace(1, "define('%T' as '%k')", def, val);
			}
			else if ((def = type(s, qual))) {	// define sin math.sin;	???
				if (skip(s, PNCT_cln)) {
					if ((tok = next(s, CNST_int))) {
						def = declare(s, TYPE_ref, tag, def, 0);
						def->offs = tok->cint;
						def->onst = 1;
					}
				}
				else// */
					def = declare(s, TYPE_def, tag, def, 0);
			}
			else if (skip(s, PNCT_cln)) {	// define a: typename@;
				
				astn val = expr(s, 0);
				//~ debug("define: %+k", val);
				if (eval(NULL, val, 0)) {
					def = declare(s, TYPE_def, tag, val->type, 0);
					def->init = val;
				}
				else {
					error(s, val->line, "Constant expression expected, got: '%+k'", val);
					def = declare(s, TYPE_def, tag, 0, 0);
					def->type = NULL;
					def->init = val;
				}
				trace(1, "define('%T' as '%k')", def, val);
			}
			else {
				error(s, tag->line, "typename excepted");
			}
			trace(1, "define('%T' as '%T')", def, def);
			if (!skip(s, OPER_nop)) {
				error(s, s->line, "unexcepted token '%k'", peek(s));
				if (!peek(s)) return 0;
				while (peek(s)) {
					if (skip(s, OPER_nop)) return 0;
					if (skip(s, OPER_end)) return 0;
					skip(s, 0);
				}
			}
		}
		else error(s, s->line, "Identifyer expected");
	}
	else if (skip(s, TYPE_rec)) {		// struct
		if (!(tag = next(s, TYPE_ref))) {	// name?
			tag = newnode(s, TYPE_ref);
			tag->line = s->line;
		}
		/*if (skip(s, PNCT_cln)) {			// pack?
			if ((tok = next(s, CNST_int))) {
				switch ((int)tok->cint) {
					default:
						error(s, s->line, "alignment must be a small power of two, not %d", tok->cint);
						break;
					case  0: pack =  0; break;
					case  1: pack =  1; break;
					case  2: pack =  2; break;
					case  4: pack =  4; break;
					case  8: pack =  8; break;
					case 16: pack = 16; break;
				}
			}
		}// */
		if (skip(s, OPER_beg)) {			// body
			def = declare(s, TYPE_rec, tag, 0, 0);
			enter(s, def);
			//~ while (skipToken(s, 'inherit'));
			while (!skip(s, OPER_end)) {
				//~ int ql = qual(s, 0);
				symn typ = spec(s, qual);
				if (!typ) typ = type(s, qual);
				if (typ && (tok = next(s, TYPE_ref))) {
					offs = align(offs, pack, typ->size);
					tmp = declare(s, TYPE_ref, tok, typ, 0);
					tmp->size = offs;
					offs += typ->size;
					if (size < offs) size = offs;
				}
				if (!skip(s, OPER_nop)) {
					error(s, s->line, "unexcepted token '%k'", peek(s));
					while ((tok = peek(s))) {
						if (skip(s, OPER_nop)) break;
						if (skip(s, OPER_end)) break;
						skip(s, 0);
					}
				}
			}
			def->size = size;
			def->args = leave(s, def);
			if (!def->args)
				warn(s, 2, s->file, s->line, "empty declaration");
		}
	}
	//else if (skip(s, TYPE_cls));		// class
	else if (skip(s, TYPE_enu)) {		// enum
		if (!(tag = next(s, TYPE_ref))) {	// name?
			tag = newnode(s, TYPE_ref);
			tag->line = s->line;
		}
		if (tag && skip(s, OPER_beg)) {		// body
			if (tag->name) def = declare(s, TYPE_enu, tag, 0, 0);
			if (tag->name) enter(s, def);
			while (!skip(s, OPER_end)) {
				if ((tok = next(s, TYPE_ref))) {
					tmp = declare(s, CNST_int, tok, type_i32, 0);
					if (skip(s, ASGN_set)) {
						tmp->init = expr(s, CNST_int);
						if (eval(NULL, tmp->init, TYPE_int) != CNST_int)
							error(s, s->line, "integer constant expected");
						//~ if (iscons(tmp->init) != CNST_int)
							//~ error(s, s->line, "integer constant expected");
					}
					else {
						tmp->init = newnode(s, CNST_int);
						tmp->init->type = type_i32;
						tmp->init->cint = offs;
						offs += 1;
					}
				}
				/*else {
					error(s, s->line, "identifyer expected");
					return 0;
				}*/
				if (!skip(s, OPER_nop)) {
					// TODO error
					if (!(tok = peek(s))) return 0;
					error(s, tok->line, "unexcepted token '%k'", tok);
					while (!skip(s, OPER_nop)) {
						if (skip(s, OPER_end)) break;//return 0;
						skip(s, 0);
					}
				}
			}
			if (tag->name)
				def->args = leave(s, def);
		}
	}
	else {					// type
	}
	trace(-8, "leave:spec('%+T')", def);
	return def;
}

//~ symn declLibc() {}
symn instlibc(state s, const char* name) {
	astn ftag = 0;
	int argsize = 0;
	symn args = 0, ftyp = 0;
	s->_ptr = (char*)name;
	s->_cnt = strlen(name);
	//~ s->_tok = 0;
	//~ s->file = "libcall : "name;

	if (!(ftag = next(s, TYPE_ref))) return 0;	// int ...
	if (!(ftyp = lookup(s, 0, ftag))) return 0;
	if (!istype(ftag)) return 0;

	if (!(ftag = next(s, TYPE_ref))) return 0;	// int bsr ...

	if (skip(s, PNCT_lp)) {				// int bsr ( ...
		if (!skip(s, PNCT_rp)) {		// int bsr ( )
			enter(s, ftyp);
			while (peek(s)) {
				astn atag = 0;
				symn atyp = 0;

				if (!(atag = next(s, TYPE_ref))) break;	// int
				if (!(atyp = lookup(s, 0, atag))) break;
				if (!istype(atag)) break;

				if (!(atag = next(s, TYPE_ref)))
					atag = newnode(s, TYPE_ref);

				declare(s, TYPE_ref, atag, atyp, 0);
				argsize += atyp->size;

				if (skip(s, PNCT_rp)) break;
				if (!skip(s, OPER_com)) break;
			}
			args = leave(s, NULL);
		}
	}
	if (!peek(s)) {
		symn def = newdefn(s, TYPE_ref);
		unsigned hash = ftag->hash;
		def->name = ftag->name;
		def->nest = s->nest;
		def->type = ftyp;
		def->args = args;
		def->libc = 1;
		def->call = 1;
		def->size = argsize;

		def->next = s->deft[hash];
		s->deft[hash] = def;

		// link
		def->defs = s->defs;
		s->defs = def;

		def->all = s->all;
		s->all = def;

		return def;
	}
	return NULL;
}// */

void enter(state s, symn def) {
	s->nest += 1;
	s->scope[s->nest].csym = def;
	//~ s->scope[s->nest].stmt = NULL;
	//~ return &s->scope[s->nest];
}
symn leave(state s, symn fldp) {
	int i;
	symn arg = 0;
	s->nest -= 1;

	// clear from table
	for (i = 0; i < TBLS; i++) {
		symn def = s->deft[i];
		while (def && def->nest > s->nest) {
			symn tmp = def;
			def = def->next;
			tmp->next = 0;

			//~ tmp->fild = fldp;
		}
		s->deft[i] = def;
	}

	// clear from stack
	while (s->defs) {
		symn sym = s->defs;

		if (sym->nest <= s->nest) break;
		s->defs = sym->defs;
		sym->next = arg;
		arg = sym;
	}

	return arg;
}

/*int Qual() {
	while ((tok = peek(s))) {		// qual: const/packed/volatile
		if (tok->kind == QUAL_con) {	// constant
			if (qual & QUAL_con) error(s, tok->line, "duplicate qualifier: %k", tok);
			qual |= QUAL_con;
			skip(s, 0);
			continue;
		}
		if (tok->kind == QUAL_vol) {	// volatile
			if (qual & QUAL_vol) error(s, tok->line, "duplicate qualifier: %k", tok);
			qual |= QUAL_vol;
			skip(s, 0);
			continue;
		}
		break;
	}
}// */

//}

/** scan -----------------------------------------------------------------------
 * scan a source
 * @param mode: script mode
**/
/** stmt -----------------------------------------------------------------------
 * scan a statement (mode ? enable decl : do not)
 * @param mode: enable or not <decl>.
 *	ex: "for( ; ; ) int a;", if mode == 0 => error
 * stmt: ';'
 *	| expr ';'
 *	| decl ';'
 *	| '{' stmt* '}'
 *	| 'static'? 'if' (expr) stmt ('else' stmt)?
 *	| ('static' | 'parallel')? 'for' '(' (decl | expr)? ';' expr?';' expr? ')' stmt
+*	| 'return' expr? ';'
+*	| 'continue' ';'
+*	| 'break' ';'
+*	| ID ':'
+*	| 'sync' ('(' ID ')')? '{' stmt '}'		// synchtonized
+*	| 'task' ('(' ID ')')? '{' stmt '}'		// parallel task
?*	| 'fork'

>* static if (...)		// compile time test
>* static for (...)		// loop unroll
>* parallel for (...)		// parallel loop
**/
/** decl -----------------------------------------------------------------------
 * scan a declaration, add to ast tree
 * @param mode: enable or not <expr>.
 * decl:

 * var
 *	typename <idtf> (= <expr>)?, (<idtf> (= <expr>)?)? \;
 + array
 *	typename <idtf>[]... (= <expr>)?, (<idtf> (= <expr>)?)? \;
 + function
 *	typename <idtf> \( <args> \) ({<stmt>})? \;
 * operator
 *	typename <oper> \( <args> \) ({<stmt>})? \;
 * property
 * template

 *	define ...
 * 	define <idtf> <type>;				// typedef
 * 	define <idtf> = <cons>;				// typed, named const
 ?	define <idtf>(args...) {stmt}			// inline

 *	enumerate ...
 *	enum <idtf>? {(<idtf> (= cintexpr)?;)+}		// enum

 *	record ...
 *	struct (\:<pack>)? (tag) '{'(<decl>)*'}'	// record


 *	typename
 *	template

 * var	(static)? <type> name = expr [, name = expr]*
 * arr	 $ (static)? <type> name'['expr']' (= '('(expr[,expr]*)')'|expr)?
X* fnc	 $ (static)? <type> name'('type name (= expr)? [,type name (= expr)?]*')'
 *

//~ template(typename t1, typename t2) pair;
//~ pair.t1

//~ template(typename ElementT, int size) class stack {
//~ 	ElementT bp[size], *sp = bp;
//~ }

//~ enum token(int prec, int argc, str name) {
//~ 	TYPE_i32(0x1, 0, "integer");
//~ 	STMT_for(0x0, 0, "for");
//~ 	OPER_add(0xc, 2, "__add");
//~ 	ASGN_mul(0x2, 2, "*=");
//~ }
**/
/** expr -----------------------------------------------------------------------
 * scan an expression
 * if mode then exit on ','
 * expr: ID
 * 	 | expr '[' expr ']'		// \arr	array
?* 	 | '[' expr ']'			// \ptr	pointer
 * 	 | expr '(' expr? ')'		// \fnc	function
 * 	 | '(' expr ')'			// \pri	precedence
 * 	 | OP expr			// \uop	unary
 * 	 | expr OP expr			// \bop	binary
 * 	 | expr '?' expr ':' expr	// \top	ternary
 * 	 ;
**/
