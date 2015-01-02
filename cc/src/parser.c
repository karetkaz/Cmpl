/*******************************************************************************
 *   File: parse.c
 *   Date: 2011/06/23
 *   Desc: input, lexer and parser
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
			?module,
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

//#{~~~~~~~~~ Input ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/** Fill some characters from the file.
 * @brief fill the memory buffer from file.
 * @param cc compiler context.
 * @return number of characters in buffer.
 */
static int fillBuf(ccState cc) {
	if (cc->fin._fin >= 0) {
		unsigned char* base = cc->fin._buf + cc->fin._cnt;
		int l, size = sizeof(cc->fin._buf) - cc->fin._cnt;
		memcpy(cc->fin._buf, cc->fin._ptr, cc->fin._cnt);
		cc->fin._cnt += l = read(cc->fin._fin, base, size);
		if (l == 0) {
			cc->fin._buf[cc->fin._cnt] = 0;
			close(cc->fin._fin);
			cc->fin._fin = -1;
		}
		cc->fin._ptr = (char*)cc->fin._buf;
	}
	return cc->fin._cnt;
}

/** Read the next character.
 * @brief read the next character from input stream.
 * @param cc compiler context.
 * @return the next character or -1 on end, or error.
 */
static int readChr(ccState cc) {
	int chr = cc->_chr;
	if (chr != -1) {
		cc->_chr = -1;
		return chr;
	}

	// fill in the buffer.
	if (cc->fin._cnt <= 4 && fillBuf(cc) <= 0) {
		return -1;
	}

	chr = *cc->fin._ptr;
	if (chr == '\n' || chr == '\r') {

		// threat 'cr', 'lf' and 'cr + lf' as new line (lf: '\n')
		if (chr == '\r' && cc->fin._ptr[1] == '\n') {
			cc->fin._cnt -= 1;
			cc->fin._ptr += 1;
			cc->fpos += 1;
		}

		// advance to next line
		if (cc->line > 0) {
			cc->line += 1;
		}

		// save where the next line begins.
		cc->lpos = cc->fpos + 1;
		chr = '\n';
	}

	cc->fin._cnt -= 1;
	cc->fin._ptr += 1;
	cc->fpos += 1;
	return chr;
}

/** Peek the next character.
 * @brief peek the next character from input stream.
 * @param cc compiler context.
 * @return the next character or -1 on end, or error.
 */
static int peekChr(ccState cc) {
	if (cc->_chr == -1) {
		cc->_chr = readChr(cc);
	}
	return cc->_chr;
}

/** Skip the next character.
 * @brief read the next character from input stream if it maches param chr.
 * @param cc compiler context.
 * @param chr filter: 0 matches everithing.
 * @return the character skiped.
 */
static int skipChr(ccState cc, int chr) {
	if (!chr || chr == peekChr(cc)) {
		return readChr(cc);
	}
	return 0;
}

/** Push back a character.
 * @brief putback the character chr to be read next time.
 * @param cc compiler context.
 * @param chr the character to be pushed back.
 * @return the character pushed back.
 */
static int backChr(ccState cc, int chr) {
	dieif(cc->_chr != -1, "can not put back more than one character");
	return cc->_chr = chr;
}
//#}

//#{~~~~~~~~~ Lexer ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/** Calculate hash of string.
 * @brief calculate the hash of a string.
 * @param str string to be hashed.
 * @param len length of string.
 * @return hash code of the input.
 */
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
		if (len == (unsigned)(-1))
			len = strlen(str) + 1;
		while (len-- > 0)
			hs = (hs >> 8) ^ crc_tab[(hs ^ (*str++)) & 0xff];
	}

	return hs ^ 0xffffffff;
}

/** Add string to string table.
 * @brief add string to string table.
 * @param cc compiler context.
 * @param str the string to be mapped.
 * @param len the length of the string, -1 recalculates using strlen.
 * @param hash precalculated hashcode, -1 recalculates.
 * @return the mapped string in the string table.
 */
char* mapstr(ccState cc, char* str, unsigned len/* = -1U*/, unsigned hash/* = -1U*/) {
	state rt = cc->s;
	list newn, next, prev = 0;

	if (len == (unsigned)(-1)) {
		len = strlen(str) + 1;
	}

	if (hash == (unsigned)(-1)) {
		hash = rehash(str, len) % TBLS;
	}

	dieif(str[len - 1] != 0, "FixMe: %s[%d]", str, len);
	for (next = cc->strt[hash]; next; next = next->next) {
		int cmp = memcmp(next->data, str, len);
		if (cmp == 0) {
			return (char*)next->data;
		}
		if (cmp > 0) {
			break;
		}
		prev = next;
	}

	dieif(rt->_beg >= rt->_end - (sizeof(struct list) + len), "memory overrun");
	if (str != (char *)rt->_beg) {
		// copy data from constants ?
		memcpy(rt->_beg, str, len + 1);
		str = (char*)rt->_beg;
	}

	// allocate list node as temp data (end of memory)
	rt->_end -= sizeof(struct list);
	newn = (list)rt->_end;

	rt->_beg += len;

	if (!prev) {
		cc->strt[hash] = newn;
	}
	else {
		prev->next = newn;
	}

	newn->next = next;
	newn->data = (unsigned char*)str;

	return str;
}

/** Read the next token.
 * @brief read the next token from input.
 * @param cc compiler context.
 * @param tok out parameter to be filled with data.
 * @return the kind of token, TOKN_err (0) if error occurred.
 */
static int readTok(ccState cc, astn tok) {
	enum {
		OTHER = 0x00000000,

		SPACE = 0x00000100,    // white spaces
		UNDER = 0x00000200,    // underscore
		PUNCT = 0x00000400,    // punctuation
		CNTRL = 0x00000800,    // controll characters

		DIGIT = 0x00001000,    // digits: 0-9
		LOWER = 0x00002000,    // lowercase letters: a-z
		UPPER = 0x00004000,    // uppercase letters: A-Z
		SCTRL = CNTRL|SPACE,   // whitespace or controll chars
		ALPHA = UPPER|LOWER,   // letters
		ALNUM = DIGIT|ALPHA,   // letters and numbers
		CWORD = UNDER|ALNUM,   // letters numbers and underscore
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
	const char* end = (char*)cc->s->_end;

	char* beg = NULL;
	char* ptr = NULL;
	int chr = readChr(cc);

	// skip white spaces and comments
	while (chr != -1) {
		if (chr == '/') {
			const int line = cc->line;		// comment begin line
			const int next = peekChr(cc);	// comment begin char

			// line comment
			if (skipChr(cc, '/')) {
				int fmt = skipChr(cc, '%');

				chr = readChr(cc);
				ptr = beg = (char*)cc->s->_beg;

				while (ptr < end && chr != -1) {
					if (chr == '\n') {
						break;
					}
					*ptr++ = chr;
					chr = readChr(cc);
				}
				if (chr == -1) {
					warn(cc->s, 9, cc->file, line, "no newline at end of file");
				}
				else if (cc->line != line + 1) {
					warn(cc->s, 9, cc->file, line, "multi-line comment: `%s`", ptr);
				}

				// record print format if comment is on the same line as the symbol.
				// example: struct hex32: int32; //%%0x8x
				if (fmt && cc->pfmt && cc->pfmt->line == line) {
					*ptr++ = 0;
					cc->pfmt->pfmt = mapstr(cc, beg, ptr - beg, -1);
				}
			}

			// block comment
			else if (next == '*' || next == '+') {
				int level = 0;

				while (chr != -1) {
					int prev_chr = chr;
					chr = readChr(cc);

					if (prev_chr == '/' && chr == next) {
						level += 1;
						if (level > 1 && next == '*') {
							warn(cc->s, 9, cc->file, cc->line, "ignoring nested comment");
							level = 1;
						}
						chr = 0;	// disable reading as valid comment: /*/ and /+/
					}

					if (prev_chr == next && chr == '/') {
						level -= 1;
						if (level == 0) {
							break;
						}
					}
				}

				if (chr == -1) {
					error(cc->s, cc->file, line, "unterminated block comment");
				}
				chr = readChr(cc);
			}

			// print format reset
			cc->pfmt = NULL;
		}

		if (chr_map[chr & 0xff] == CNTRL) {
			warn(cc->s, 5, cc->file, cc->line, "ignoring controll character: `%c`", chr);
			while (chr == 0) {
				chr = readChr(cc);
			}
		}

		if (!(chr_map[chr & 0xff] & SPACE)) {
			break;
		}

		chr = readChr(cc);
	}

	ptr = beg = (char*)cc->s->_beg;
	cc->pfmt = NULL;

	// our token begins here
	memset(tok, 0, sizeof(*tok));
	tok->file = cc->file;
	tok->line = cc->line;
	tok->colp = cc->fpos - cc->lpos;

	// scan
	if (chr != -1) switch (chr) {
		default:
			if (chr_map[chr & 0xff] & DIGIT) {
				goto read_num;
			}
			if (chr_map[chr & 0xff] & CWORD) {
				goto read_idf;
			}
			error(cc->s, cc->file, cc->line, "invalid character: '%c'", chr);
			tok->kind = TOKN_err;
			break;

		case '.':
			if (chr_map[peekChr(cc) & 0xff] == DIGIT) {
				goto read_num;
			}
			tok->kind = OPER_dot;
			break;

		case ';':
			tok->kind = STMT_do;
			break;

		case ',':
			tok->kind = OPER_com;
			break;

		case '{':
			tok->kind = STMT_beg;
			break;

		case '[':
			tok->kind = PNCT_lc;
			break;

		case '(':
			tok->kind = PNCT_lp;
			break;

		case '}':
			tok->kind = STMT_end;
			break;

		case ']':
			tok->kind = PNCT_rc;
			break;

		case ')':
			tok->kind = PNCT_rp;
			break;

		case '?':
			tok->kind = PNCT_qst;
			break;

		case ':':
			if (skipChr(cc, '=')) {
				tok->kind = ASGN_set;
				break;
			}
			tok->kind = PNCT_cln;
			break;

		case '~':
			tok->kind = OPER_cmt;
			break;

		case '=':
			if (skipChr(cc, '=')) {
				tok->kind = OPER_equ;
				break;
			}
			tok->kind = ASGN_set;
			break;

		case '!':
			if (skipChr(cc, '=')) {
				tok->kind = OPER_neq;
				break;
			}
			tok->kind = OPER_not;
			break;

		case '>':
			chr = peekChr(cc);
			if (chr == '>') {
				readChr(cc);
				if (skipChr(cc, '=')) {
					tok->kind = ASGN_shr;
					break;
				}
				tok->kind = OPER_shr;
				break;
			}
			if (chr == '=') {
				readChr(cc);
				tok->kind = OPER_geq;
				break;
			}
			tok->kind = OPER_gte;
			break;

		case '<':
			chr = peekChr(cc);
			if (chr == '<') {
				readChr(cc);
				if (skipChr(cc, '=')) {
					tok->kind = ASGN_shl;
					break;
				}
				tok->kind = OPER_shl;
				break;
			}
			if (chr == '=') {
				readChr(cc);
				tok->kind = OPER_leq;
				break;
			}
			tok->kind = OPER_lte;
			break;

		case '&':
			chr = peekChr(cc);
			if (chr == '=') {
				readChr(cc);
				tok->kind = ASGN_and;
				break;
			}
			if (chr == '&') {
				readChr(cc);
				tok->kind = OPER_lnd;
				break;
			}
			tok->kind = OPER_and;
			break;

		case '|':
			chr = peekChr(cc);
			if (chr == '=') {
				readChr(cc);
				tok->kind = ASGN_ior;
				break;
			}
			if (chr == '|') {
				readChr(cc);
				tok->kind = OPER_lor;
				break;
			}
			tok->kind = OPER_ior;
			break;

		case '^':
			if (skipChr(cc, '=')) {
				tok->kind = ASGN_xor;
				break;
			}
			tok->kind = OPER_xor;
			break;

		case '+':
			if (skipChr(cc, '=')) {
				tok->kind = ASGN_add;
				break;
			}
			tok->kind = OPER_add;
			break;

		case '-':
			if (skipChr(cc, '=')) {
				tok->kind = ASGN_sub;
				break;
			}
			tok->kind = OPER_sub;
			break;

		case '*':
			if (skipChr(cc, '=')) {
				tok->kind = ASGN_mul;
				break;
			}
			tok->kind = OPER_mul;
			break;

		case '/':
			if (skipChr(cc, '=')) {
				tok->kind = ASGN_div;
				break;
			}
			tok->kind = OPER_div;
			break;

		case '%':
			if (skipChr(cc, '=')) {
				tok->kind = ASGN_mod;
				break;
			}
			tok->kind = OPER_mod;
			break;

		case '\'':			// \'[^\'\n]*
		case '"': {			// \"[^\"\n]*
			int multi_line = 0;		// multi line string constant.
			int start_char = chr;	// literals start character.

			while (ptr < end && (chr = readChr(cc)) != -1) {

				// end reached
				if (chr == start_char)
					break;

				// new line reached
				if (chr == '\n' && !multi_line)
					break;

				// escape sequence
				if (chr == '\\') {
					int oct, hex1, hex2;
					switch (chr = readChr(cc)) {
						case '\'':
							chr = '\'';
							break;

						case '\"':
							chr = '\"';
							break;

						case '\?':
							chr = '\?';
							break;

						case '\\':
							chr = '\\';
							break;

						case 'a':
							//~ audible bell
							chr = '\a';
							break;

						case 'b':
							//~ backspace
							chr = '\b';
							break;

						case 'f':
							//~ form feed - new page
							chr = '\f';
							break;

						case 'n':
							//~ line feed - new line
							chr = '\n';
							break;

						case 'r':
							//~ carriage return
							chr = '\r';
							break;

						case 't':
							//~ horizontal tab
							chr = '\t';
							break;

						case 'v':
							//~ vertical tab
							chr = '\v';
							break;

						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
							//~ octal sequence (max length = 3)
							oct = chr - '0';
							if ((chr = peekChr(cc)) >= '0' && chr <= '7') {
								oct = (oct << 3) | (chr - '0');
								readChr(cc);
								if ((chr = peekChr(cc)) >= '0' && chr <= '7') {
									oct = (oct << 3) | (chr - '0');
									readChr(cc);
								}
							}
							if (oct & 0xffffff00) {
								warn(cc->s, 4, cc->file, cc->line, "octal escape sequence overflow");
							}
							chr = oct & 0xff;
							break;

						case 'x':
							// hexadecimal sequence (length = 2)
							hex1 = readChr(cc);
							hex2 = readChr(cc);
							if (hex1 >= '0' && hex1 <= '9') {
								hex1 -= '0';
							}
							else if (hex1 >= 'a' && hex1 <= 'f') {
								hex1 -= 'a' - 10;
							}
							else if (hex1 >= 'A' && hex1 <= 'F') {
								hex1 -= 'A' - 10;
							}
							else {
								error(cc->s, cc->file, cc->line, "hex escape sequence invalid");
								break;
							}
							if (hex2 >= '0' && hex2 <= '9') {
								hex2 -= '0';
							}
							else if (hex2 >= 'a' && hex2 <= 'f') {
								hex2 -= 'a' - 10;
							}
							else if (hex2 >= 'A' && hex2 <= 'F') {
								hex2 -= 'A' - 10;
							}
							else {
								error(cc->s, cc->file, cc->line, "hex escape sequence invalid");
								break;
							}
							chr = hex1 << 4 | hex2;
							break;

						default:
							error(cc->s, cc->file, cc->line, "invalid escape sequence '\\%c'", chr);
							chr = 0;
							break;

						case '\n':
							// start multi line string or continue on next line.
							if (start_char == '"') {
								if (beg == ptr) {
									multi_line = 1;
								}
								// wrap on next line.
								continue;
							}
					}
				}

				*ptr++ = chr;
			}

			*ptr++ = 0;

			if (chr != start_char) {
				error(cc->s, cc->file, cc->line, "unclosed %s literal", start_char == '"' ? "string" : "character");
				return tok->kind = TOKN_err;
			}

			if (start_char == '"') {
				tok->kind = TYPE_str;
				tok->ref.hash = rehash(beg, ptr - beg) % TBLS;
				tok->ref.name = mapstr(cc, beg, ptr - beg, tok->ref.hash);
			}
			else {
				int val = 0;

				if (ptr == beg) {
					error(cc->s, cc->file, cc->line, "empty character constant");
					return tok->kind = TOKN_err;
				}
				if (ptr > beg + vm_size + 1) {
					warn(cc->s, 1, cc->file, cc->line, "character constant truncated");
				}
				//~ TODO: size of char is type_chr->size, not sizeof(char)
				else if (ptr > beg + sizeof(char) + 1) {
					warn(cc->s, 5, cc->file, cc->line, "multi character constant");
				}

				for (ptr = beg; *ptr; ++ptr) {
					val = val << 8 | *ptr;
				}

				tok->kind = TYPE_int;
				tok->cint = val;
			}
		} break;
		read_idf: {			// [a-zA-Z_][a-zA-Z0-9_]*
			static const struct {
				char* name;
				int type;
			}
			keywords[] = {
				// Warning: sort keyword list by name
				//~ {"operator", OPER_def},
				{"break", STMT_brk},
				{"const", ATTR_const},
				{"continue", STMT_con},
				{"define", TYPE_def},
				{"else", STMT_els},
				{"emit", EMIT_opc},
				{"enum", ENUM_kwd},
				{"for", STMT_for},
				{"if", STMT_if},
				{"parallel", QUAL_par},
				{"return", STMT_ret},
				{"static", ATTR_stat},
				{"struct", TYPE_rec}
			};

			int lo = 0;
			int hi = lengthOf(keywords);
			int cmp = -1;

			while (ptr < end && chr != -1) {
				if (!(chr_map[chr & 0xff] & CWORD)) {
					break;
				}
				*ptr++ = chr;
				chr = readChr(cc);
			}
			backChr(cc, chr);
			*ptr++ = 0;

			// binary search for keyword
			while (cmp != 0 && lo < hi) {
				int mid = lo + ((hi - lo) / 2);
				cmp = strcmp(keywords[mid].name, beg);
				if (cmp < 0) {
					lo = mid + 1;
				}
				else {
					hi = mid;
				}
			}
			if (cmp == 0) {
				tok->kind = keywords[hi].type;
			}
			else {
				tok->kind = TYPE_ref;
				tok->type = tok->ref.link = NULL;
				tok->ref.hash = rehash(beg, ptr - beg) % TBLS;
				tok->ref.name = mapstr(cc, beg, ptr - beg, tok->ref.hash);
			}
		} break;
		read_num: {			// int | ([0-9]+'.'[0-9]* | '.'[0-9]+)([eE][+-]?[0-9]+)?
			int flt = 0;			// floating
			int ovrf = 0;			// overflow
			int radix = 10;
			int64_t i64v = 0;
			float64_t f64v = 0;
			char* suffix = NULL;

			//~ 0[.oObBxX]?
			if (chr == '0') {
				*ptr++ = chr;
				chr = readChr(cc);

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
						chr = readChr(cc);
						break;

					case 'o':
					case 'O':
						radix = 8;
						*ptr++ = chr;
						chr = readChr(cc);
						break;

					case 'x':
					case 'X':
						radix = 16;
						*ptr++ = chr;
						chr = readChr(cc);
						break;
				}
			}

			//~ ([0-9a-fA-F])*
			while (chr != -1) {
				int value = radix;
				if (chr >= '0' && chr <= '9') {
					value = chr - '0';
				}
				else if (chr >= 'a' && chr <= 'f') {
					value = chr - 'a' + 10;
				}
				else if (chr >= 'A' && chr <= 'F') {
					value = chr - 'A' + 10;
				}

				if (value >= radix) {
					break;
				}

				// check overflow
				if (radix == 10 && i64v > (0x7fffffffffffffffLL - value) / radix) {
					ovrf = 1;
				}

				i64v = i64v * radix + value;

				*ptr++ = chr;
				chr = readChr(cc);
			}

			if (ovrf != 0) {
				warn(cc->s, 4, cc->file, cc->line, "value overflow");
			}

			//~ ('.'[0-9]*)? ([eE]([+-]?)[0-9]+)?
			if (radix == 10) {

				f64v = (float64_t)i64v;

				if (chr == '.') {
					long double val = 0;
					long double exp = 1;

					*ptr++ = chr;
					chr = readChr(cc);

					while (chr >= '0' && chr <= '9') {
						val = val * 10 + (chr - '0');
						exp *= 10;

						*ptr++ = chr;
						chr = readChr(cc);
					}

					f64v += val / exp;
					flt = 1;
				}

				if (chr == 'e' || chr == 'E') {
					long double p = 1, m = 10;
					unsigned int val = 0;
					int sgn = 1;

					*ptr++ = chr;
					chr = readChr(cc);

					switch (chr) {
						case '-':
							sgn = -1;
						case '+':
							*ptr++ = chr;
							chr = readChr(cc);
					}

					ovrf = 0;
					suffix = ptr;
					while (chr >= '0' && chr <= '9') {
						val = val * 10 + (chr - '0');
						if (val > 1024) {
							ovrf = 1;
						}

						*ptr++ = chr;
						chr = readChr(cc);
					}

					if (suffix == ptr) {
						error(cc->s, tok->file, tok->line, "no exponent in numeric constant");
					}
					else if (ovrf) {
						warn(cc->s, 4, cc->file, cc->line, "exponent overflow");
					}

					while (val) {		// pow(10, val);
						if (val & 1) {
							p *= m;
						}
						val >>= 1;
						m *= m;
					}

					if (sgn < 0) {
						f64v /= p;
					}
					else {
						f64v *= p;
					}
					flt = 1;
				}
			}

			suffix = ptr;
			while (ptr < end && chr != -1) {
				if (!(chr_map[chr & 0xff] & CWORD)) {
					break;
				}
				*ptr++ = chr;
				chr = readChr(cc);
			}
			backChr(cc, chr);
			*ptr++ = 0;

			if (*suffix) {	// error
				if (suffix[0] == 'f' && suffix[1] == 0) {
					tok->cst2 = TYPE_f32;
					flt = 1;
				}
				else {
					error(cc->s, tok->file, tok->line, "invalid suffix in numeric constant '%s'", suffix);
					tok->kind = TOKN_err;
				}
			}
			if (flt != 0) {	// float
				tok->kind = TYPE_flt;
				tok->cflt = f64v;
			}
			else {			// integer
				tok->kind = TYPE_int;
				tok->cint = i64v;
			}
		} break;
	}
	dieif(ptr >= end, "mem overrun %t", tok->kind);
	return tok->kind;
}

/** Push back a token.
 * @brief put back token to be read next time.
 * @param cc compiler context.
 * @param tok the token to be pushed back.
 */
static void backTok(ccState cc, astn tok) {
	tok->next = cc->_tok;
	cc->_tok = tok;
}

/** Peek the next token.
 * @brief peek the next token from input.
 * @param cc compiler context.
 * @param kind filter: 0 matches everithing.
 * @return next token, or null.
 */
static astn peekTok(ccState cc, ccToken kind) {
	if (cc->_tok == NULL) {
		cc->_tok = newnode(cc, 0);
		if (!readTok(cc, cc->_tok)) {
			eatnode(cc, cc->_tok);
			cc->_tok = NULL;
			return NULL;
		}
	}
	if (!kind || cc->_tok->kind == kind) {
		return cc->_tok;
	}
	return NULL;
}

/** Read the next token.
 * @brief read the next token from input.
 * @param cc compiler context.
 * @param kind filter: 0 matches everithing.
 * @return next token, or null.
 */
static astn next(ccState cc, ccToken kind) {
	if (peekTok(cc, kind)) {
		astn ast = cc->_tok;
		cc->_tok = ast->next;
		ast->next = 0;
		return ast;
	}
	return 0;
}

/** Skip the next token.
 * @brief skip the next token.
 * @param cc compiler context.
 * @param kind filter: 0 matches everithing.
 * @return true if token was skipped.
 */
int skip(ccState cc, ccToken kind) {
	astn ast = peekTok(cc, 0);
	if (!ast || (kind && ast->kind != kind)) {
		return 0;
	}
	cc->_tok = ast->next;
	eatnode(cc, ast);
	return 1;
}

// TODO: to be removed.
static int test(ccState cc) {
	astn tok = peekTok(cc, 0);
	return tok ? tok->kind : 0;
}

// TODO: rename, review.
static int skiptok(ccState cc, ccToken kind, int raise) {
	if (!skip(cc, kind)) {
		if (raise) {
			if (!peekTok(cc, 0)) {
				error(cc->s, cc->file, cc->line, "unexpected end of file, `%t` excepted", kind);
			}
			else {
				error(cc->s, cc->file, cc->line, "`%t` excepted, got `%k`", kind, peekTok(cc, 0));
			}
		}

		switch (kind) {
			case STMT_do:
			case STMT_end:
			case PNCT_rp:
			case PNCT_rc:
				break;

			default:
				return 0;
		}
		while (!skip(cc, kind)) {
			//~ debug("skipping('%k')", peek(s));
			if (skip(cc, STMT_do)) return 0;
			if (skip(cc, STMT_end)) return 0;
			if (skip(cc, PNCT_rp)) return 0;
			if (skip(cc, PNCT_rc)) return 0;
			if (!skip(cc, 0)) return 0;		// eof?
		}
		return 0;
	}
	return kind;
}
//#}

/** Construct arguments.
 * @brief construct a new node for function arguments
 * @param cc compiler context.
 * @param lhs arguments or first argument.
 * @param rhs next argument.
 * @return if lhs is null return (rhs) else return (lhs, rhs).
 */
static inline astn argnode(ccState cc, astn lhs, astn rhs) {
	return lhs ? opnode(cc, OPER_com, lhs, rhs) : rhs;
}

/** Construct block node.
 * @brief construct a new node for block of statements
 * @param cc compiler context.
 * @param node next argument.
 * @return if lhs is null return (rhs) else return (lhs, rhs).
 */
static inline astn blockNode(ccState cc, astn node) {
	astn block = newnode(cc, STMT_beg);
	block->cst2 = TYPE_vid;
	block->type = cc->type_vid;
	if (node != NULL) {
		block->file = node->file;
		block->line = node->line;
		block->stmt.stmt = node;
	}
	return block;
}

/** Construct reference node.
 * @brief construct a new node for variables.
 * @param cc compiler context.
 * @param name name of the node.
 * @return the new node.
 */
static inline astn tagnode(ccState cc, char* name) {
	astn ast = NULL;
	if (cc != NULL && name != NULL) {
		ast = newnode(cc, TYPE_ref);
		if (ast != NULL) {
			int slen = strlen(name);
			ast->kind = TYPE_ref;
			ast->file = cc->file;
			ast->line = cc->line;
			ast->type = NULL;
			ast->ref.link = NULL;
			ast->ref.hash = rehash(name, slen + 1) % TBLS;
			ast->ref.name = mapstr(cc, name, slen + 1, ast->ref.hash);
		}
	}
	return ast;
}

/** Add length property.
 * @brief add length property to array type
 * @param cc compiler context.
 * @param sym the symbol of the array.
 * @param init size of the length:
 *     constant int node for static length arrays,
 *     null for dynamic length arrays
 * @return symbol of the length property.
 */
static symn addLength(ccState cc, symn sym, astn init) {
	/* TODO: review return value.
	 *
	 * dynamic array size kind: ATTR_const | TYPE_ref
	 * static array size kind: ATTR_stat | ATTR_const | TYPE_def
	 *     using static we get the warning accessing static member through instance.
	 */
	ccToken kind = ATTR_const | (init != NULL ? TYPE_def : TYPE_ref);
	symn args = sym->flds;
	symn result = NULL;

	enter(cc, NULL);
	result = install(cc, "length", kind, 0, 0, cc->type_i32, init);
	sym->flds = leave(cc, sym, 0);
	dieif(sym->flds != result, "FixMe");

	// non static member
	if (sym->flds) {
		sym->flds->next = args;
	}
	else {
		sym->flds = args;
	}
	return result;
}

/** Check for redefinition of symbol.
 * @brief Check for redefinition of symbol.
 * @param cc compiler context.
 * @param sym symbol to check.
 */
//~ TODO: move to type.c.
static void redefine(ccState cc, symn sym) {
	struct astNode tag;
	symn ptr;

	if (sym == NULL) {
		return;
	}

	memset(&tag, 0, sizeof(tag));
	tag.kind = TYPE_ref;
	// checking symbols with the same hash code.
	for (ptr = sym->next; ptr; ptr = ptr->next) {
		symn arg1 = ptr->prms;
		symn arg2 = sym->prms;

		// there should be no anonimus declarationd with this hash
		if (ptr->name == NULL)
			continue;

		// may override default initializer
		if (sym->call != ptr->call)
			continue;

		// hash may match but name be different.
		if (strcmp(sym->name, ptr->name) != 0)
			continue;

		while (arg1 && arg2) {
			tag.ref.link = arg1;
			tag.type = arg1->type;
			if (!canAssign(cc, arg2, &tag, 1)) {
				tag.ref.link = arg2;
				tag.type = arg2->type;
				if (!canAssign(cc, arg1, &tag, 1)) {
					break;
				}
			}
			arg1 = arg1->next;
			arg2 = arg2->next;
		}

		if (arg1 == NULL && arg2 == NULL) {
			break;
		}
	}

	if (ptr != NULL && (ptr->nest >= sym->nest)) {
		error(cc->s, sym->file, sym->line, "redefinition of `%-T`", sym);
		if (ptr->file && ptr->line) {
			info(cc->s, ptr->file, ptr->line, "first defined here as `%-T`", ptr);
		}
	}
}

/** TODO: to be deleted. replace with js like initialization.
 * @brief make constructor using fields as arguments.
 * @param cc compiler context.
 * @param rec the record.
 * @return the constructors symbol.
 */
static symn ctorArg(ccState cc, symn rec) {
	symn ctor = install(cc, rec->name, TYPE_def, 0, 0, rec, NULL);
	if (ctor != NULL) {
		astn root = NULL;
		symn arg, newarg;

		// TODO: params should not be dupplicated: ctor->prms = rec->prms
		enter(cc, NULL);
		for (arg = rec->flds; arg; arg = arg->next) {

			if (arg->stat)
				continue;

			if (arg->kind != TYPE_ref)
				continue;

			newarg = install(cc, arg->name, TYPE_def, arg->type->cast, 0, arg->type, NULL);
			//~ newarg = install(cc, arg->name, TYPE_ref, TYPE_rec, 0, arg->type, NULL);
			//~ newarg->cast = arg->type->cast;

			if (newarg != NULL) {
				astn tag;
				newarg->call = arg->call;
				newarg->prms = arg->prms;

				tag = lnknode(cc, newarg);
				tag->cst2 = arg->cast;
				root = argnode(cc, root, tag);
			}
		}
		ctor->prms = leave(cc, ctor, 0);

		ctor->kind = TYPE_def;
		ctor->call = 1;
		ctor->cnst = 1;	// returns constant (if arguments are constants)
		ctor->init = opnode(cc, OPER_fnc, cc->emit_tag, root);
		ctor->init->type = rec;
	}
	return ctor;
}

// TODO: to be deleted.
static int mkConst(astn ast, ccToken cast) {
	struct astNode tmp;

	dieif(!ast, "FixMe");

	if (cast == TYPE_any) {
		cast = ast->cst2;
	}

	if (!eval(&tmp, ast)) {
		debug("%+k", ast);
		return 0;
	}

	ast->kind = tmp.kind;
	ast->cint = tmp.cint;

	switch (ast->cst2 = cast) {
		default:
			fatal("FixMe: %+k: %t", ast, cast);
			return 0;

		case TYPE_vid:
		case TYPE_any:
		case TYPE_rec:
			return 0;

		case TYPE_bit:
			ast->cint = constbol(ast);
			ast->kind = TYPE_int;
			break;

		case TYPE_u32:
		case TYPE_i32:
		case TYPE_i64:
			ast->cint = constint(ast);
			ast->kind = TYPE_int;
			break;

		case TYPE_f32:
		case TYPE_f64:
			ast->cflt = constflt(ast);
			ast->kind = TYPE_flt;
			break;
	}

	return ast->kind;
}

//#{~~~~~~~~~ Parser ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//~ TODO: to be deleted: use expr instead, then check if it is a typename.
static astn type(ccState cc/* , int mode */) {	// type(.type)*
	symn def = NULL;
	astn tok, list = NULL;
	while ((tok = next(cc, TYPE_ref))) {

		symn loc = def ? def->flds : cc->deft[tok->ref.hash];
		symn sym = lookup(cc, loc, tok, NULL, 0);

		tok->next = list;
		list = tok;

		def = sym;

		if (!istype(sym)) {
			def = NULL;
			break;
		}

		if ((tok = next(cc, OPER_dot))) {
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
			backTok(cc, back);
			//~ trace("not a type `%k`", back);
		}
		list = NULL;
	}
	else if (list) {
		list->type = def;
	}
	return list;
}

static astn stmt(ccState, int mode);	// parse statement		(mode: enable decl)
static astn decl(ccState, int mode);	// parse declaration	(mode: enable spec)
//~ astn args(ccState, int mode);		// parse arguments		(mode: ...)
//~ astn unit(ccState, int mode);		// parse unit			(mode: script/unit)

/** Parse qualifiers.
 * @brief scan for qualifiers: const static ?paralell
 * @param cc compiler context.
 * @param mode scannable qualifiers.
 * @return qualifiers.
 */
static int qual(ccState cc, int mode) {
	int result = 0;
	astn ast;

	while ((ast = peekTok(cc, 0))) {
		trloop("%k", peekTok(cc, 0));
		switch (ast->kind) {

			default:
				//~ trace("next %k", peek(s));
				return result;

			case ATTR_const:
				if (!(mode & ATTR_const))
					return result;

				if (result & ATTR_const) {
					error(cc->s, ast->file, ast->line, "qualifier `%t` declared more than once", ast);
				}
				result |= ATTR_const;
				skip(cc, 0);
				break;

			case ATTR_stat:
				if (!(mode & ATTR_stat))
					return result;

				if (result & ATTR_stat) {
					error(cc->s, ast->file, ast->line, "qualifier `%t` declared more than once", ast);
				}
				result |= ATTR_stat;
				skip(cc, 0);
				break;
		}
	}

	return result;
}

/** TODO: remove typechecking.
 * @brief Parse expression.
 * @param cc compiler context.
 * @param mode
 * @return
 */
static astn expr(ccState cc, int mode) {
	astn buff[TOKS], tok;
	const astn* base = buff + TOKS;
	astn* prec = (astn*)base;
	astn* post = buff;
	char sym[TOKS];						// symbol stack {, [, (, ?
	int unary = 1;						// start in unary mode
	int level = 0;						// precedence, top of sym

	sym[level] = 0;
	if (!peekTok(cc, 0)) {
		trace("null");
		return NULL;
	}

	while ((tok = next(cc, 0))) {					// parse
		int pri = level << 4;
		trloop("%k", peek(cc));
		// statements are not allowed in expressions !!!!
		if (tok->kind >= STMT_beg && tok->kind <= STMT_end) {
			backTok(cc, tok);
			tok = 0;
			break;
		}
		switch (tok->kind) {
			tok_id: {
				*post++ = tok;
			} break;
			tok_op: {
				int oppri = tok_tbl[tok->kind].type;
				tok->op.prec = pri | (oppri & 0x0f);
				if (oppri & 0x10) while (prec < base) {
					if ((*prec)->op.prec <= tok->op.prec)
						break;
					*post++ = *prec++;
				}
				else while (prec < base) {
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
						error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
					}
					unary = 1;
					goto tok_op;
				}
				else {
					if (!unary)
						error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
					unary = 0;
					goto tok_id;
				}
			} break;

			case PNCT_lp: {			// '('
				if (unary)			// a + (3*b)
					*post++ = 0;
				else if (peekTok(cc, PNCT_rp)) {	// a()
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
					backTok(cc, tok);
					tok = 0;
					break;
				}
				else {
					error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
					break;
				}
				unary = 0;
			} goto tok_op;
			case PNCT_lc: {			// '['
				if (!unary)
					tok->kind = OPER_idx;
				else {
					error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
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
					backTok(cc, tok);
					tok = 0;
					break;
				}
				else {
					error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
					break;
				}
				unary = 0;
			} goto tok_op;
			case PNCT_qst: {		// '?'
				if (unary) {
					*post++ = 0;		// ???
					error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
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
					backTok(cc, tok);
					tok = 0;
					break;
				}
				else {
					error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
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
					error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
				unary = 1;
			} goto tok_op;
			case OPER_cmt: {		// '~'
				if (!unary)
					error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
				unary = 1;
			} goto tok_op;
			case OPER_com: {		// ','
				if (unary)
					error(cc->s, tok->file, tok->line, "syntax error before '%k'", tok);
				unary = 1;
			} goto tok_op;
		}
		if (post >= prec) {
			error(cc->s, cc->file, cc->line, "Expression too complex");
			return 0;
		}
		if (!tok)
			break;
	}
	if (unary || level) {							// error
		char* missing = "expression";
		if (level) switch (sym[level]) {
			default:
				fatal("FixMe");
				break;
			case '(': missing = "')'"; break;
			case '[': missing = "']'"; break;
			case '?': missing = "':'"; break;
		}
		error(cc->s, cc->file, cc->line, "missing %s, %k", missing, peekTok(cc, 0));
	}
	else if (prec > buff) {							// build
		astn* ptr;
		astn* lhs;

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
						astn tmp = NULL;
						switch (tok->kind) {
							default:
								fatal("FixMe");
								return NULL;

							case ASGN_add:
								tmp = newnode(cc, OPER_add);
								break;
							case ASGN_sub:
								tmp = newnode(cc, OPER_sub);
								break;
							case ASGN_mul:
								tmp = newnode(cc, OPER_mul);
								break;
							case ASGN_div:
								tmp = newnode(cc, OPER_div);
								break;
							case ASGN_mod:
								tmp = newnode(cc, OPER_mod);
								break;
							case ASGN_shl:
								tmp = newnode(cc, OPER_shl);
								break;
							case ASGN_shr:
								tmp = newnode(cc, OPER_shr);
								break;
							case ASGN_and:
								tmp = newnode(cc, OPER_and);
								break;
							case ASGN_ior:
								tmp = newnode(cc, OPER_ior);
								break;
							case ASGN_xor:
								tmp = newnode(cc, OPER_xor);
								break;
						}
						tmp->op.rhso = tok->op.rhso;
						tmp->op.lhso = tok->op.lhso;
						tmp->file = tok->file;
						tmp->line = tok->line;
						tok->kind = ASGN_set;
						tok->op.rhso = tmp;
						break;
					}
				}
			}
			*lhs++ = tok;
		}
	}

	if (mode && tok) {								// check
		astn root = cc->root;
		cc->root = NULL;
		if (!typecheck(cc, NULL, tok)) {
			if (cc->root != NULL) {
				error(cc->s, tok->file, tok->line, "invalid expression: `%+k` in `%+k`", cc->root, tok);
			}
			else {
				error(cc->s, tok->file, tok->line, "invalid expression: `%+k`", tok);
			}
		}
		cc->root = root;
	}
	dieif(mode == 0, "internal error");
	return tok;
}

/** Parse arguments.
 * @brief Parse the parameters of a function declaration.
 * @param cc compiler context.
 * @param mode
 * @return expression tree.
 */
static astn args(ccState cc, int mode) {
	astn root = NULL;

	if (peekTok(cc, PNCT_rp)) {
		return cc->void_tag;
	}

	while (peekTok(cc, 0)) {
		int attr = qual(cc, ATTR_const);
		astn arg = decl_var(cc, NULL, mode);

		if (arg != NULL) {
			root = argnode(cc, root, arg);
			if (attr & ATTR_const) {
				arg->ref.link->cnst = 1;
			}
		}

		if (!skip(cc, OPER_com)) {
			break;
		}
	}
	return root;
}

/** Parse initialization.
 * @brief parse the initializer of a declaration.
 * @param cc compiler context.
 * @param var the declared variable symbol.
 * @return the initializer expression.
 */
static astn init_var(ccState cc, symn var) {
	if (skip(cc, ASGN_set)) {
		symn typ = var->type;
		ccToken cast = var->cast;
		int mkcon = var->cnst;
		int arrayInit = 0;

		if (typ->kind == TYPE_arr) {
			if (skip(cc, STMT_beg)) {
				arrayInit = STMT_end;
			}
			else if (skip(cc, PNCT_lc)) {
				arrayInit = PNCT_rc;
			}
			else if (skip(cc, PNCT_lp)) {
				arrayInit = PNCT_rp;
			}
		}
		var->init = expr(cc, TYPE_def);
		if (arrayInit) {
			skiptok(cc, arrayInit, 1);
		}

		if (var->init != NULL) {

			//~ TODO: try to typecheck the same as using ASGN_set
			// assigning an emit expression: ... x = emit(struct, ...)
			if (var->init->type == cc->emit_opc) {
				var->init->type = var->type;
				var->init->cst2 = cast;
				if (mkcon) {
					error(cc->s, var->file, var->line, "invalid constant initialization `%+k`", var->init);
				}
			}

			// assigning to an array
			else if (arrayInit) {
				astn init = var->init;
				symn base = var->prms;
				int nelem = 1;

				// TODO: base should not be stored in var->args !!!
				cast = base ? base->cast : 0;

				if (base == typ->type && typ->init == NULL) {
					mkcon = 0;
				}

				while (init->kind == OPER_com) {
					astn val = init->op.rhso;
					if (!canAssign(cc, base, val, 0)) {
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
				if (!canAssign(cc, base, init, 0)) {
					trace("canAssignArray(%-T, %+k, %t)", base, init, cast);
					return NULL;
				}
				if (!castTo(init, cast)) {
					trace("canAssignCast(%-T, %+k, %t)", base, init, cast);
					return NULL;
				}
				if (mkcon && !mkConst(init, cast)) {
					trace("canAssignCast(%-T, %+k, %t)", base, init, cast);
					return NULL;
				}

				// int a[] = {1, 2, 3, 4, 5, 6};
				// TODO:! var initialization parsing should not modify the variable.
				if (base == typ->type && typ->init == NULL && var->cnst) {
					typ->init = intnode(cc, nelem);
					// ArraySize
					typ->size = nelem * typ->type->size;
					typ->offs = nelem;
					typ->cast = TYPE_ref;
					typ->stat = 1;
					var->cast = TYPE_any;
					var->size = typ->size;

					addLength(cc, typ, typ->init);
				}
				else if (typ->init == NULL) {
					error(cc->s, var->file, var->line, "invalid initialization `%+k`", var->init);
					return NULL;
				}
				var->init->type = var->prms;
			}

			// check if value can be assigned to variable.
			if (canAssign(cc, var, var->init, cast == TYPE_ref)) {

				// assign enum variable to base type.
				if (var->cast != ENUM_kwd && var->init->type->cast == ENUM_kwd) {
					astn ast = var->init;
					symn typ = ast->type->type;
					if (!castTo(ast, castOf(ast->type = typ))) {
						trace("%+k", ast);
						return NULL;
					}
				}

				// TODO:! var initialization parsing should not modify the variable.
				if (var->cast == TYPE_arr) {
					var->size = 2 * vm_size;
				}

				//~ trace("%+T -> %t (%s:%u)", var, cast, var->file, var->line);
				if (!castTo(var->init, cast)) {
					trace("%t", cast);
					return NULL;
				}
			}
			else {
				error(cc->s, var->file, var->line, "invalid initialization `%-T` := `%+k`", var, var->init);
				return NULL;
			}
		}
		else {
			// var->init is null and expr raised the error.
			//~ error(s->s, var->file, var->line, "invalid initialization of `%-T`", var);
		}
	}
	return var->init;
}

/** Parse alias declaration.
 * @brief parse the declaration of an alias/inline expression.
 * @param cc compiler context.
 * @return root of declaration.
 */
static astn decl_alias(ccState cc) {
	astn tag = NULL;
	symn def = NULL;
	symn typ = NULL;

	if (!skiptok(cc, TYPE_def, 1)) {	// 'define'
		trace("%+k", tag);
		return NULL;
	}

	if (!(tag = next(cc, TYPE_ref))) {	// name?
		error(cc->s, cc->file, cc->line, "Identifyer expected");
		skiptok(cc, STMT_do, 1);
		return NULL;
	}

	if (skip(cc, ASGN_set)) {				// const:   define PI = 3.14...;
		// can not call decl_init
		// TODO: check if locals are used.
		astn val = expr(cc, TYPE_def);
		if (val != NULL) {
			def = declare(cc, TYPE_def, tag, val->type);
			if (!isType(val)) {
				if (isConst(val)) {
					def->stat = 1;
					def->cnst = 1;
				}
				else if (isStatic(cc, val)) {
					def->stat = 1;
				}
				def->init = val;
			}
			typ = def->type;
		}
		else {
			error(cc->s, cc->file, cc->line, "expression expected");
		}
	}
	else if (skip(cc, PNCT_lp)) {			// inline:  define isNan(float64 x) = x != x;
		astn init = NULL;
		symn param;

		def = declare(cc, TYPE_def, tag, NULL);
		def->name = NULL; // disable lookup as variable.

		enter(cc, NULL);
		args(cc, TYPE_def);
		skiptok(cc, PNCT_rp, 1);

		if (skip(cc, ASGN_set)) {
			// define sqr(int a) = (a * a);
			if ((init = expr(cc, TYPE_def))) {
				typ = init->type;
			}
		}

		if (!(param = leave(cc, def, 0))) {
			param = cc->void_tag->ref.link;
		}

		def->name = tag->ref.name;
		def->stat = isStatic(cc, init);
		def->cnst = isConst(init);
		def->type = typ;
		def->prms = param;
		def->init = init;
		def->call = 1;

		//* TODO: in-out parameters: remove
		// TODO: this should go to fixargs
		for (param = def->prms; param; param = param->next) {
			// is explicitly set to be cached.
			if (param->cast == TYPE_ref) {
				param->cast = param->type->cast;
			}
			else {
				int used = usages(param) - 1;
				// warn to cache if it is used more than once
				if (used > 1) {
					warn(cc->s, 8, param->file, param->line, "parameter `%T` may be cached (used %d times in expression)", param, used);
				}
				else {
					param->kind = TYPE_def;
				}
			}
		}// */

		if (init == NULL) {
			error(cc->s, cc->file, cc->line, "expression expected");
		}
	}
	else {
		error(cc->s, tag->file, tag->line, "typename excepted");
		return NULL;
	}

	if (!skiptok(cc, STMT_do, 1)) {
		trace("%+k", tag);
		return NULL;
	}

	tag->kind = TYPE_def;
	tag->type = typ;
	tag->ref.link = def;
	return tag;
}

/** Parse enum declaration.
 * @brief parse the declaration of an enumeration.
 * @param cc compiler context.
 * @return root of declaration.
 */
static astn decl_enum(ccState cc) {
	symn def = NULL, base = cc->type_i32;
	astn tok, tag;

	int enuminc = 1;
	int64_t lastval = -1;

	if (!skiptok(cc, ENUM_kwd, 1)) {	// 'enum'
		trace("%+k", peekTok(cc, 0));
		return NULL;
	}

	tag = next(cc, TYPE_ref);			// ref?

	if (skip(cc, PNCT_cln)) {			// ':' type
		base = NULL;
		if ((tok = expr(cc, TYPE_def))) {
			if (isType(tok)) {
				base = linkOf(tok);
			}
			else {
				error(cc->s, cc->file, cc->line, "typename expected, got `%k`", tok);
			}
		}
		else {
			error(cc->s, cc->file, cc->line, "typename expected, got `%k`", peekTok(cc, 0));
		}
		enuminc = base && (base->cast == TYPE_i32 || base->cast == TYPE_i64);
	}

	if (!skiptok(cc, STMT_beg, 1)) {	// '{'
		trace("%+k", peekTok(cc, 0));
		return NULL;
	}

	if (tag != NULL) {
		def = declare(cc, ATTR_stat | ATTR_const | TYPE_rec, tag, base);
		tag->kind = TYPE_def;
		enter(cc, tag);
	}
	else {
		tag = tagnode(cc, ".anonymous");
		tag->ref.link = base;
		tag->type = base;
	}

	while (peekTok(cc, 0)) {			// enum members
		trloop("%k", peek(cc));

		if (peekTok(cc, STMT_end)) {
			break;
		}

		if ((tok = next(cc, TYPE_ref))) {
			// HACK: declare as a variable to be assignable, then revert it to be an alias.
			symn ref = declare(cc, TYPE_ref, tok, base);
			ref->stat = ref->cnst = 1;
			redefine(cc, ref);

			if (init_var(cc, ref)) {
				if (!mkConst(ref->init, TYPE_any)) {
					warn(cc->s, 2, ref->file, ref->line, "constant expected, got: `%+k`", ref->init);
				}
				else if (enuminc) {
					lastval = constint(ref->init);
				}
			}
			else if (enuminc) {
				ref->init = intnode(cc, lastval += 1);
				ref->init->type = base;
			}
			else {
				error(cc->s, ref->file, ref->line, "constant expected");
			}
			ref->kind = TYPE_def;
		}
		skiptok(cc, STMT_do, 1);
	}

	if (def != NULL) {
		def->flds = leave(cc, def, 1);
		def->prms = base->prms;
		def->cast = ENUM_kwd;
	}

	if (!skiptok(cc, STMT_end, 1)) {	// '}'
		trace("%+k", tag);
		return NULL;
	}

	// Error checking
	// ... ?

	return tag;
}

/** Parse struct declaration.
 * @brief parse the declaration of a structure.
 * @param cc compiler context.
 * @param attr attributes: static or const.
 * @return root of declaration.
 */
static astn decl_struct(ccState cc, int Attr) {
	symn def = NULL, base = NULL;
	astn tok, tag;

	int byref = 0;
	int pack = vm_size;

	if (!skiptok(cc, TYPE_rec, 1)) {	// 'struct'
		trace("%+k", peekTok(cc, 0));
		return NULL;
	}

	tag = next(cc, TYPE_ref);			// ref

	if (skip(cc, PNCT_cln)) {			// ':' type or pack
		if ((tok = expr(cc, TYPE_def))) {
			if (tok->kind == TYPE_int) {
				if (tok->cint > 16 || (tok->cint & -tok->cint) != tok->cint) {
					error(cc->s, tok->file, tok->line, "alignment must be a small power of two, not %k", tok);
				}
				else if (Attr == ATTR_stat) {
					error(cc->s, cc->file, cc->line, "alignment can not be applied to static struct");
				}
				pack = (int32_t)tok->cint;
			}
			else if (isType(tok)) {
				base = linkOf(tok);
			}
			else {
				error(cc->s, cc->file, cc->line, "alignment must be an integer constant");
			}
		}
		else {
			error(cc->s, cc->file, cc->line, "alignment must be an integer constant, got `%k`", peekTok(cc, 0));
		}
	}

	if (!skiptok(cc, STMT_beg, 1)) {	// '{'
		trace("%+k", peekTok(cc, 0));
		return NULL;
	}

	if (tag == NULL) {
		error(cc->s, cc->file, cc->line, "identifier expected");
		tag = tagnode(cc, ".anonymous");
		tag->kind = TYPE_def;
		enter(cc, tag);
	}
	def = declare(cc, ATTR_stat | ATTR_const | TYPE_rec, tag, cc->type_rec);
	tag->kind = TYPE_def;
	enter(cc, tag);

	while (peekTok(cc, 0)) {			// struct members
		trloop("%k", peekTok(cc, 0));

		if (peekTok(cc, STMT_end)) {
			break;
		}

		if (!decl(cc, 0)) {
			error(cc->s, cc->file, cc->line, "declaration expected, got: `%+k`", peekTok(cc, 0));
			skiptok(cc, STMT_do, 0);
		}
	}

	def->flds = leave(cc, def, (Attr & ATTR_stat) != 0);
	def->prms = def->flds;
	def->size = fixargs(def, pack, 0);

	if (!skiptok(cc, STMT_end, 1)) {	// '}'
		trace("%+k", peekTok(cc, 0));
		return tag;
	}

	if (base != NULL) {
		def->size += base->size;
		def->pfmt = base->pfmt;
		cc->pfmt = def;
		def->cast = base->cast;
	}
	else {
		def->cast = TYPE_rec;
	}

	if ((Attr & ATTR_stat) != 0 || def->size == 0) {
		// make uninstantiable
		def->cast = TYPE_vid;
	}
	else {
		if (def->flds && pack == vm_size && !byref) {
			ctorArg(cc, def);
		}

		if (!def->flds && !def->type) {
			warn(cc->s, 2, cc->file, cc->line, "empty declaration");
		}
	}

	// Error checking
	if (base && base->cast != TYPE_ref) {
		// TODO: to be removed: declarations like: struct hex32: int32;
		if (skip(cc, STMT_do)) {
			warn(cc->s, 1, tag->file, tag->line, "deprecated declaration of `%k`", tag);
			backTok(cc, newnode(cc, STMT_end));
			backTok(cc, newnode(cc, STMT_beg));
		}
		else {
			error(cc->s, tag->file, tag->line, "must extend a reference type, not `%+T`", base);
		}
	}

	if (Attr == ATTR_stat) {
		// TODO: static structs can not allign 'static struct Struct: 1'
		// TODO: static structs can not inherit 'static struct Struct: object'

		//error(cc->s, tag->file, tag->line, "alignment can not be applied static struct: `%?T`", def);
	}

	//tag->type = def;
	return tag;
}

/** Parse variable or function declaration.
 * @brief parse the declaration of a variable or function.
 * @param cc compiler context.
 * @param argv out parameter for function arguments.
 * @param mode
 * @return parsed syntax tree.
 */
astn decl_var(ccState cc, astn* argv, int mode) {
	symn typ, ref = NULL;
	astn tag = NULL;
	int inout, byref;

	if (!(tag = type(cc))) {
		debug("%+k", peekTok(cc, 0));
		return NULL;
	}
	typ = tag->type;

	inout = skip(cc, OPER_lnd);
	byref = inout || skip(cc, OPER_and);

	if (byref && typ->cast == TYPE_ref) {
		warn(cc->s, 2, tag->file, tag->line, "ignoring &, %-T is already a reference type", typ);
	}

	if (!(tag = next(cc, TYPE_ref))) {
		if (mode != TYPE_def) {
			trace("id expected, not `%k` at(%s:%u)", peekTok(cc, 0), cc->file, cc->line);
			return NULL;
		}
		tag = tagnode(cc, "");
	}

	ref = declare(cc, TYPE_ref, tag, typ);
	ref->size = byref ? vm_size : sizeOf(typ);

	if (argv != NULL) {
		*argv = NULL;
	}

	if (skip(cc, PNCT_lp)) {			// int a(...)
		astn argroot;
		enter(cc, tag);
		argroot = args(cc, TYPE_ref);
		ref->prms = leave(cc, ref, 0);
		skiptok(cc, PNCT_rp, 1);

		ref->size = vm_size;
		ref->call = 1;

		if (ref->prms == NULL) {
			ref->prms = cc->void_tag->ref.link;
			argroot = cc->void_tag;
		}

		if (argv) {
			*argv = argroot;
		}

		if (inout || byref) {
			// int& find(int array[], int element);
			// int&& find(int array[], int element);
			error(cc->s, tag->file, tag->line, "TODO: declaration returning reference: `%+T`", ref);
		}
		byref = 0;
	}

	if (skip(cc, PNCT_lc)) {			// int a[...]
		symn arr = newdefn(cc, TYPE_arr);
		symn tmp = arr;
		symn base = typ;
		int dynarr = 1;		// dynamic sized array: slice

		tmp->type = typ;
		tmp->size = -1;
		typ = tmp;

		ref->type = typ;
		tag->type = typ;
		//~ arr->offs = vmOffset(cc->s, arr);
		//~ arr->stat = 1;

		if (!peekTok(cc, PNCT_rc)) {
			struct astNode val;
			astn init = expr(cc, TYPE_def);
			typ->init = init;

			if (init != NULL) {
				if (eval(&val, init) == TYPE_int) {
					// add static const length property to array type.
					addLength(cc, typ, init);
					typ->size = (int)val.cint * typ->type->size;
					typ->offs = (int)val.cint;
					typ->cast = TYPE_ref;
					typ->init = init;
					ref->cast = 0;

					dynarr = 0;
					if (val.cint < 0) {
						error(cc->s, init->file, init->line, "positive integer constant expected, got `%+k`", init);
					}
				}
				else {
					error(cc->s, init->file, init->line, "integer constant expected declaring array `%T`", ref);
				}
			}
		}
		if (dynarr) {
			// add dynamic length property to array type.
			symn len = addLength(cc, typ, NULL);
			dieif(len == NULL, "FixMe");
			ref->size = 2 * vm_size;	// slice is a struct {pointer data, int32 length}
			ref->cast = TYPE_arr;
			len->offs = vm_size;		// offset for length.
			typ->cast = TYPE_arr;
		}
		else {
			typ->cast = TYPE_ref;
			typ->stat = 1;
		}
		skiptok(cc, PNCT_rc, 1);

		// Multi dimensional arrays
		while (skip(cc, PNCT_lc)) {
			tmp = newdefn(cc, TYPE_arr);
			trloop("%k", peekTok(cc, 0));
			tmp->type = typ->type;
			//~ typ->decl = tmp;
			tmp->decl = typ;
			typ->type = tmp;
			tmp->size = -1;
			//~ tmp->offs = vmOffset(cc->s, tmp);
			//~ tmp->stat = 1;
			typ = tmp;

			if (!peekTok(cc, PNCT_rc)) {
				struct astNode val;
				astn init = expr(cc, TYPE_def);
				if (eval(&val, init) == TYPE_int) {
					//~ ArraySize
					addLength(cc, typ, init);
					typ->size = (int)val.cint * typ->type->size;
					typ->offs = (int)val.cint;
					typ->init = init;
				}
				else {
					error(cc->s, init->file, init->line, "integer constant expected");
				}
			}
			if (typ->init == NULL) {
				// add dynamic length property to array type.
				symn len = addLength(cc, typ, NULL);
				dieif(len == NULL, "FixMe");
				ref->size = 2 * vm_size;	// slice is a struct {pointer data, int32 length}
				ref->cast = TYPE_arr;
				len->offs = vm_size;		// offset of length property.
				typ->cast = TYPE_arr;
			}
			else {
				typ->cast = TYPE_ref;
				typ->stat = 1;
			}

			if (dynarr != (typ->init == NULL)) {
				error(cc->s, ref->file, ref->line, "invalid mixing of dynamic sized arrays");
			}

			skiptok(cc, PNCT_rc, 1);
		}

		ref->prms = base;	// fixme (temporarly used)
		dynarr = base->size;
		for (; typ; typ = typ->decl) {
			typ->size = (dynarr *= typ->offs);
			//~ typ->offs = vmOffset(cc->s, typ);
		}
		ref->size = byref ? vm_size : sizeOf(arr);

		if (inout || byref || base->cast == TYPE_ref) {
			// int& a[200] a contains 200 references to integers
			// int&& a[200] a contains 200 references to integers
			error(cc->s, tag->file, tag->line, "TODO: declaring array of references: `%+T`", ref);
		}
		if (ref->call) {
			// int rgbCopy(int a, int b)[8] = [rgbCpy, rgbAnd, ...];
			error(cc->s, tag->file, tag->line, "TODO: declaring array of functions: `%+T`", ref);
		}
		byref = 0;
	}

	/* TODO: in-out parameters
	if (inout) {
		ref->cast = TYPE_def;
	}
	else */if (byref) {
		ref->cast = TYPE_ref;
	}
	return tag;
}

/** Parse declaration.
 * @brief parse the declaration of an alias, enum, struct, variable or function.
 * @param cc compiler context.
 * @param argv out parameter for function arguments.
 * @param mode
 * @return parsed syntax tree.
 */
static astn decl(ccState cc, int mode) {
	int Attr = qual(cc, ATTR_const | ATTR_stat);
	int line = cc->line;
	char* file = cc->file;
	astn tag = NULL;

	if (peekTok(cc, TYPE_def)) {				// define
		if ((tag = decl_alias(cc)) != NULL) {

			//? enable static and const qualifiers
			//~ Attr &= ~(ATTR_stat | ATTR_const);

			redefine(cc, tag->ref.link);
		}
	}
	else if (peekTok(cc, ENUM_kwd)) {			// enum
		if ((tag = decl_enum(cc)) != NULL) {
			redefine(cc, tag->ref.link);
		}
	}
	else if (peekTok(cc, TYPE_rec)) {			// struct
		if ((tag = decl_struct(cc, Attr)) != NULL) {

			// static structures are valid
			Attr &= ~(ATTR_stat);

			redefine(cc, tag->ref.link);
		}
	}
	else if ((tag = decl_var(cc, NULL, 0))) {	// variable, function, ...
		symn typ = tag->type;
		symn ref = tag->ref.link;

		if ((mode & decl_NoInit) == 0) {
			/* function declaration with implementation.
			 * int sqr(int a) {return a * a;}
			 *		static const non indirect reference.
			 */
			if (ref->call && peekTok(cc, STMT_beg)) {
				symn tmp, result = NULL;
				int maxlevel = cc->maxlevel;

				enter(cc, tag);
				cc->maxlevel = cc->nest;
				ref->gdef = cc->func;
				cc->func = ref;

				result = install(cc, "result", TYPE_ref, TYPE_any, sizeOf(typ), typ, NULL);
				ref->flds = result;

				if (result) {
					result->decl = ref;

					// result is the first argument
					result->offs = sizeOf(result);
					// TODO: ref->stat = 1;
					ref->size = result->offs + fixargs(ref, vm_size, -result->offs);
				}

				// reinstall all args
				for (tmp = ref->prms; tmp; tmp = tmp->next) {
					//~ TODO: make just a symlink to the symbol, not a copy of it.
					//~ TODO: install(cc, tmp->name, TYPE_def, 0, 0, tmp->type, tmp->used);
					symn arg = install(cc, tmp->name, TYPE_ref, 0, 0, NULL, NULL);
					if (arg != NULL) {
						arg->type = tmp->type;
						arg->flds = tmp->flds;
						arg->prms = tmp->prms;
						arg->cast = tmp->cast;
						arg->size = tmp->size;
						arg->offs = tmp->offs;
						arg->Attr = tmp->Attr;
					}
				}

				ref->stat = (Attr & ATTR_stat) != 0;
				ref->init = stmt(cc, 1);

				if (ref->init == NULL) {
					ref->init = newnode(cc, STMT_beg);
					ref->init->type = cc->type_vid;
				}

				cc->func = cc->func->gdef;
				cc->maxlevel = maxlevel;
				ref->gdef = NULL;

				ref->flds = leave(cc, ref, 0);
				dieif(ref->flds != result, "FixMe %-T", ref);

				backTok(cc, newnode(cc, STMT_do));
			}
			/* function reference declaration.
			 * TODO: int sqrt(int a);		// forward declaration.
			 * TODO: 	static const indirect reference.
			 * int sqrt(int a) = sqrt_386;	// initialized 
			 *		static indirect reference.
			 */
			else if (ref->call) {
				ref->size = vm_size;
			}

			// variable or function initialization.
			if (peekTok(cc, ASGN_set)) {
				if (ref->call) {
					ref->cast = TYPE_ref;
				}
				if (Attr & ATTR_const) {
					ref->cnst = 1;
				}
				if (!init_var(cc, ref)) {
					trace("%+k", tag);
					return NULL;
				}

			}
		}
		dieif(!ref->call && ref->size == 0, "FixMe: %-T", ref);

		// for (int a : range(10, 20)) ...
		if ((mode & decl_ItDecl) != 0) {
			if (peekTok(cc, PNCT_cln)) {
				backTok(cc, newnode(cc, STMT_do));
			}
		}

		cc->pfmt = ref;
		skiptok(cc, STMT_do, 1);

		// TODO: functions type == return type
		if (typ->cast == TYPE_vid && !ref->call) {	// void x;
			error(cc->s, ref->file, ref->line, "invalid variable declaration of type `%+T`", typ);
		}

		if (Attr & ATTR_stat) {
			ref->stat = 1;
		}

		if (Attr & ATTR_const) {
			ref->cnst = 1;
		}

		Attr &= ~(ATTR_stat | ATTR_const);		// static and const qualifiers are valid
		tag->kind = TYPE_def;
		redefine(cc, ref);
	}

	if (Attr & ATTR_const) {
		error(cc->s, file, line, "qualifier `const` can not be applied to `%+k`", tag);
	}
	if (Attr & ATTR_stat) {
		error(cc->s, file, line, "qualifier `static` can not be applied to `%+k`", tag);
	}

	//~ dieif(Attr, "FixMe: %+k", tag);
	return tag;
}

/** Parse statement list.
 * @brief read a list of statements.
 * @param cc compiler context.
 * @param mode raise error if (decl / stmt) found
 * @return parsed syntax tree.
 */
static astn block(ccState cc, int mode) {
	astn head = NULL;
	astn tail = NULL;

	while (peekTok(cc, 0)) {
		int stop = 0;
		astn node;
		trloop("%k", peekTok(cc, 0));

		switch (test(cc)) {
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

		if ((node = stmt(cc, 0))) {
			if (tail == NULL) {
				head = node;
			}
			else {
				tail->next = node;
			}

			tail = node;
		}
	}

	return head;
	(void)mode;
}

/** Parse statement.
 * @brief parse a statement.
 * @param cc compiler context.
 * @param mode
 * @return parsed syntax tree.
 */
static astn stmt(ccState cc, int mode) {
	astn node = NULL;
	int qual = 0;			// static | parallel

	//~ invalid start of statement
	switch (test(cc)) {
		case STMT_end:
		case PNCT_rp:
		case PNCT_rc:
		case 0:	// end of file
			trace("%+k", peekTok(cc, 0));
			skiptok(cc, 0, 1);
			return NULL;
	}
	if (!peekTok(cc, 0)) {
		trace("null");
		return NULL;
	}

	// check statement construct
	if ((node = next(cc, ATTR_stat))) {			// 'static' ('if' | 'for')
		switch (test(cc)) {
			case STMT_if:		// compile time if
			case STMT_for:		// loop unroll
				qual = ATTR_stat;
				break;
			default:
				backTok(cc, node);
				break;
		}
		node = NULL;
	}
	else if ((node = next(cc, QUAL_par))) {		// 'parallel' ('{' | 'for')
		switch (test(cc)) {
			case STMT_beg:		// parallel task
			case STMT_for:		// parallel loop
				qual = QUAL_par;
				break;
			default:
				backTok(cc, node);
				break;
		}
		node = NULL;
	}

	// scan the statement
	if (skip(cc, STMT_do)) {				// ;
		warn(cc->s, 2, cc->file, cc->line, WARN_EMPTY_STATEMENT);
	}
	else if ((node = next(cc, STMT_beg))) {	// { ... }
		int newscope = !mode;
		astn blk;

		if (newscope) {
			enter(cc, node);
		}

		if ((blk = block(cc, 0)) != NULL) {
			node->stmt.stmt = blk;
			node->type = cc->type_vid;
			node->cst2 = qual;
			qual = 0;
		}
		else {
			// eat code like: {{;{;};{}{}}}
			eatnode(cc, node);
			node = 0;
		}

		if (newscope) {
			leave(cc, NULL, 0);
		}

		skiptok(cc, STMT_end, 1);
	}
	else if ((node = next(cc, STMT_if ))) {	// if (...)
		int siffalse = cc->siff;
		int newscope = 1;

		skiptok(cc, PNCT_lp, 1);
		node->stmt.test = expr(cc, TYPE_bit);
		skiptok(cc, PNCT_rp, 1);

		// static if (true) does not need entering a new scope.
		// every declaration should be available outside if.
		if (qual == ATTR_stat) {
			struct astNode value;
			if (!eval(&value, node->stmt.test) || !peekTok(cc, STMT_beg)) {
				error(cc->s, node->file, node->line, "invalid static if construct");
				trace("%+k", peekTok(cc, 0));
			}
			if (constbol(&value)) {
				newscope = 0;
			}

			cc->siff |= newscope;
		}

		if (newscope) {
			enter(cc, node);
		}
		node->stmt.stmt = stmt(cc, 1);	// no new scope & disable decl
		if (node->stmt.stmt != NULL) {	// suggest using `if (...) {...}`
			switch (node->stmt.stmt->kind) {
				default:
					warn(cc->s, 4, cc->file, cc->line, WARN_USE_BLOCK_STATEMENT, node->stmt.stmt);
					node->stmt.stmt = blockNode(cc, node->stmt.stmt);
					break;

				// do not sugest using block statement for return, break and continue.
				case STMT_beg:
				case STMT_ret:
				case STMT_brk:
				case STMT_con:
					break;
			}
		}
		if (skip(cc, STMT_els)) {		// optionally parse else part
			if (newscope) {
				leave(cc, NULL, 0);
				enter(cc, node);
			}
			node->stmt.step = stmt(cc, 1);
			if (node->stmt.step != NULL) {
				switch (node->stmt.step->kind) {
					default:
						warn(cc->s, 4, cc->file, cc->line, WARN_USE_BLOCK_STATEMENT, node->stmt.step);
						node->stmt.step = blockNode(cc, node->stmt.step);
						break;

					// do not sugest using block statement for else part if it is a block or if statement.
					case STMT_beg:
					case STMT_if:
						break;
				}
			}
		}
		if (newscope) {
			leave(cc, NULL, 0);
		}


		node->type = cc->type_vid;
		node->cst2 = qual;
		qual = 0;

		cc->siff = siffalse;
	}
	else if ((node = next(cc, STMT_for))) {	// for (...)

		enter(cc, node);
		skiptok(cc, PNCT_lp, 1);
		if (!skip(cc, STMT_do)) {		// init or iterate
			node->stmt.init = decl(cc, decl_NoDefs|decl_ItDecl);

			if (node->stmt.init == NULL) {
				node->stmt.init = expr(cc, TYPE_vid);
				skiptok(cc, STMT_do, 1);
			}
			else if (skip(cc, PNCT_cln)) {		// iterate
				//~ for (Type value : iterable) {...}
				astn iton = expr(cc, TYPE_vid); // iterate on expression

				dieif(node->stmt.init->kind != TYPE_def, "declaration expected");

				if (iton != NULL && iton->type != NULL) {
					// iterator step function.
					astn itStep = NULL;
					// variable to iterate with.
					astn itVar = node->stmt.init;

					// iterator initializer: `iterator(iterable)`
					astn itCtor = tagnode(cc, "iterator");
					symn sym = lookup(cc, cc->deft[itCtor->ref.hash], itCtor, iton, 0);

					if (sym != NULL) {
						// iterator constructor: `iterator(iterable)`
						astn itInit = opnode(cc, OPER_fnc, itCtor, iton);
						astn itNext = tagnode(cc, "next");
						itInit->type = sym->type;

						// for (Iterator it : iterable) ...
						if (sym->type == itVar->type) {

							// initialize iterator: Iterator it = iterator(iterable);
							symn x = linkOf(itVar);
							x->init = itInit;
							sym = x;

							// iterate using the iterator function: `bool next(Iterator &it)`
							itStep = opnode(cc, OPER_fnc, itNext, opnode(cc, OPER_adr, NULL, tagnode(cc, itVar->ref.name)));
						}
						// for (Type value : iterable) ...
						else {

							// declare iterator: Iterator .it = iterator(iterable);
							astn itTmp = tagnode(cc, ".it");
							symn x = declare(cc, TYPE_ref, itTmp, sym->type);
							x->init = itInit;
							sym = x;

							// make for statement initializer a block having 2 declarations.
							node->stmt.init = newnode(cc, STMT_beg);
							node->stmt.init->type = cc->type_vid;
							node->stmt.init->cst2 = TYPE_rec;

							// set the 2 declaration in the block.
							node->stmt.init->stmt.stmt = itVar;
							itVar->next = itTmp;
							itTmp->next = NULL;

							// make node
							itTmp->kind = TYPE_def;
							itTmp->file = itVar->file;
							itTmp->line = itVar->line;

							// iterate using the iterator function: `bool next(Iterator &it, Type &&value)`
							itStep = opnode(cc, OPER_fnc, itNext,
								opnode(cc, OPER_com,
									opnode(cc, OPER_adr, NULL, tagnode(cc, itTmp->ref.name)),
									opnode(cc, OPER_adr, NULL, tagnode(cc, itVar->ref.name))
								)
							);
						}
					}

					if (itStep == NULL || !typecheck(cc, NULL, sym->init)) {
						itStep = opnode(cc, OPER_fnc, tagnode(cc, "next"), NULL);
						error(cc->s, node->file, node->line, "iterator not defined for `%T`", iton->type);
						info(cc->s, node->file, node->line, "a function `%k(%T)` should be declared", itCtor, iton->type);
					}
					else if (typecheck(cc, NULL, itStep) != cc->type_bol) {
						error(cc->s, node->file, node->line, "iterator not found for `%+k`: %+k", iton, itStep);
						if (itStep->op.rhso->kind == OPER_com) {
							info(cc->s, node->file, node->line, "a function `%k(%T &, %T): %T` should be declared", itStep->op.lhso, itStep->op.rhso->op.lhso->type, itStep->op.rhso->op.rhso->type, cc->type_bol);
						}
						else {
							info(cc->s, node->file, node->line, "a function `%k(%T &): %T` should be declared", itStep->op.lhso, itStep->op.rhso->type, cc->type_bol);
						}
					}
					else {
						symn itFunNext = linkOf(itStep);
						if (itFunNext != NULL && itFunNext->prms != NULL) {
							if (itFunNext->prms->cast != TYPE_ref) {
								error(cc->s, itFunNext->file, itFunNext->line, "iterator arguments are not by ref");
							}
							if (itFunNext->prms->next && (itFunNext->prms->next->cast != TYPE_ref && itFunNext->prms->next->cast != TYPE_arr)) {
								error(cc->s, itFunNext->file, itFunNext->line, "iterator arguments are not by ref");
							}
						}
					}

					itStep->cst2 = TYPE_bit;
					node->stmt.test = itStep;
					sym->init->cst2 = TYPE_rec;
				}
				if (qual == ATTR_stat) {
					error(cc->s, node->file, node->line, "invalid use of static iteration");
				}
				backTok(cc, newnode(cc, STMT_do));
			}
		}
		if (!skip(cc, STMT_do)) {		// test
			node->stmt.test = expr(cc, TYPE_bit);
			skiptok(cc, STMT_do, 1);
		}
		if (!skip(cc, PNCT_rp)) {		// incr
			node->stmt.step = expr(cc, TYPE_vid);
			skiptok(cc, PNCT_rp, 1);
		}
		node->stmt.stmt = stmt(cc, 1);	// no new scope & disable decl
		if (node->stmt.stmt != NULL) {	// suggest using `for (...) {...}`
			if (node->stmt.stmt->kind != STMT_beg) {
				warn(cc->s, 4, cc->file, cc->line, WARN_USE_BLOCK_STATEMENT, node->stmt.stmt);
				node->stmt.stmt = blockNode(cc, node->stmt.stmt);
			}
		}
		leave(cc, NULL, 0);

		node->type = cc->type_vid;
		node->cst2 = qual;
		qual = 0;
	}
	else if ((node = next(cc, STMT_brk))) {	// break;
		node->type = cc->type_vid;
		skiptok(cc, STMT_do, 1);
	}
	else if ((node = next(cc, STMT_con))) {	// continue;
		node->type = cc->type_vid;
		skiptok(cc, STMT_do, 1);
	}
	else if ((node = next(cc, STMT_ret))) {	// return;
		symn result = cc->func->flds;
		node->type = cc->type_vid;

		if (!peekTok(cc, STMT_do)) {
			astn val = expr(cc, TYPE_vid);		// do lookup
			if (val == NULL) {/* error */}
			else if (val->kind == TYPE_ref && val->ref.link == result) {
				// skip 'return result;' statements
			}
			else if (result->type == cc->type_vid && val->type == result->type) {
				// return void expression from a function returning void.
				node->stmt.stmt = val;
			}
			else {
				node->stmt.stmt = opnode(cc, ASGN_set, lnknode(cc, result), val);
				node->stmt.stmt->type = val->type;
				if (!typecheck(cc, NULL, node->stmt.stmt)) {
					error(cc->s, val->file, val->line, "invalid return value: `%+k`", val);
				}
			}
		}
		else if (result->type != cc->type_vid) {
			warn(cc->s, 4, cc->file, cc->line, "returning from function with no value");
		}
		skiptok(cc, STMT_do, 1);
	}
	else if ((node = decl(cc, TYPE_any))) {	// declaration
		if (mode) {
			error(cc->s, node->file, node->line, "unexpected declaration `%+k`", node);
		}
	}
	else if ((node = expr(cc, TYPE_vid))) {	// expression
		skiptok(cc, STMT_do, 1);
		switch (node->kind) {
			default:
				warn(cc->s, 1, node->file, node->line, WARN_INVALID_EXPRESSION_STATEMENT, node);
				break;

			case OPER_fnc:
			case ASGN_set:
				break;
		}
	}
	else if ((node = peekTok(cc, 0))) {
		error(cc->s, node->file, node->line, "unexpected token: `%k`", node);
		skiptok(cc, STMT_do, 1);
	}
	else {
		error(cc->s, cc->file, cc->line, "unexpected end of file");
	}

	dieif(qual != 0, "FixMe");

	return node;
}

//#}

/**
 * @brief set source(file or string) to be parsed.
 * @param cc compiler context.
 * @param src file name or text to pe parsed.
 * @param isFile src is a filename.
 * @return boolean value of success.
 */
static int source(ccState cc, int isFile, char* src) {
	if (cc->fin._fin > 3) {
		// close previous opened file
		close(cc->fin._fin);
	}

	cc->fin._fin = -1;
	cc->fin._ptr = 0;
	cc->fin._cnt = 0;
	cc->_chr = -1;
	cc->_tok = 0;

	cc->file = NULL;
	cc->line = 0;

	if (isFile && src != NULL) {
#ifdef __WATCOMC__
		if ((cc->fin._fin = open(src, O_RDONLY|O_BINARY)) <= 0) {
#else
		if ((cc->fin._fin = open(src, O_RDONLY)) <= 0) {
#endif
			return -1;
		}

		cc->file = mapstr(cc, src, -1, -1);
		cc->line = 1;

		if (fillBuf(cc) > 2) {
			// skip first line if it begins with: #!
			if (cc->fin._ptr[0] == '#' && cc->fin._ptr[1] == '!') {
				int chr = readChr(cc);
				while (chr != -1) {
					if (chr == '\n') {
						break;
					}
					chr = readChr(cc);
				}
			}
		}
	}
	else if (src != NULL) {
		cc->fin._ptr = src;
		cc->fin._cnt = strlen(src);
	}
	return 0;
}

/**
 * @brief parse the source setted by source.
 * @param cc
 * @param asUnit
 * @param warn
 * @return
 */
static astn parse(ccState cc, int asUnit, int warn) {
	astn root, oldRoot = cc->root;
	int level = cc->nest;

	cc->warn = warn;

	if (0) {
		astn root = NULL;
		astn ast, prev = NULL;
		while ((ast = next(cc, 0))) {
			if (prev == NULL) {
				root = ast;
				prev = ast;
			}
			else {
				prev->next = ast;
			}
			prev = ast;
		}
		backTok(cc, root);
	}

	if ((root = block(cc, 0))) {
		if (asUnit != 0) {
			//! disables globals_on_stack values after execution.
			astn unit = newnode(cc, STMT_beg);
			unit->type = cc->type_vid;
			unit->file = cc->file;
			unit->line = 1;
			unit->cst2 = TYPE_vid;
			unit->stmt.stmt = root;
			root = unit;
		}

		if (oldRoot) {
			astn end = oldRoot->stmt.stmt;
			if (end != NULL) {
				while (end->next) {
					end = end->next;
				}

				end->next = root;
			}
			else {
				oldRoot->stmt.stmt = root;
			}
			root = oldRoot;
		}
		cc->root = root;

		if (peekTok(cc, 0)) {
			error(cc->s, cc->file, cc->line, "unexpected token: %k", peekTok(cc, 0));
			return 0;
		}

		if (cc->nest != level) {
			error(cc->s, cc->file, cc->line, "premature end of file: %d", cc->nest);
			return NULL;
		}
	}

	return root;
}

/// @see: header
ccState ccOpen(state rt, char* file, int line, char* text) {
	ccState result = rt->cc;

	if (result == NULL) {
		// initilaize only once.
		result = ccInit(rt, creg_def, NULL);
		if (result == NULL) {
			return NULL;
		}
	}

	if (source(rt->cc, text == NULL, text ? text : file) != 0) {
		return NULL;
	}

	if (file != NULL) {
		rt->cc->file = mapstr(rt->cc, file, -1, -1);
	}
	else {
		rt->cc->file = NULL;
	}

	rt->cc->fin.nest = rt->cc->nest;
	rt->cc->line = line;
	return result;
}

/// Compile file or text; @see state.api.ccAddCode
int ccAddCode(state rt, int warn, char* file, int line, char* text) {
	ccState cc = ccOpen(rt, file, line, text);
	if (cc == NULL) {
		error(rt, NULL, 0, "can not open: %s", file);
		return 0;
	}
	parse(cc, 0, warn);
	return ccDone(cc) == 0;
}

/// @see: header
int ccAddUnit(state rt, int unit(state), int warn, char *file) {
	int unitCode = unit(rt);
	if (unitCode == 0 && file != NULL) {
		return ccAddCode(rt, warn, file, 1, NULL);
	}
	return unitCode == 0;
}

/// @see: header
int ccDone(ccState cc) {
	astn ast;

	// not initialized
	if (cc == NULL)
		return -1;

	// check no token left to read
	if ((ast = peekTok(cc, 0))) {
		error(cc->s, ast->file, ast->line, "unexpected: `%k`", ast);
		return -1;
	}

	// check we are on the same nesting level.
	if (cc->fin.nest != cc->nest) {
		error(cc->s, cc->file, cc->line, "unexpected: end of input: %s", cc->fin._buf);
		return -1;
	}

	// close input
	source(cc, 0, NULL);

	// return errors
	return cc->s->errc;
}
