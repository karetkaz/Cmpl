/*******************************************************************************
 *   File: lexer.c
 *   Date: 2011/06/23
 *   Desc: input and lexer
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
			else,
			emit,
			enum,
			for,
			if,
			inline,
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
#include "core.h"

//#{~~~~~~~~~ Input ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// used mostly by the lexer

/** Fill some characters from the file.
 * @brief fill the memory buffer from file.
 * @param cc compiler context.
 * @return number of characters in buffer.
 */
static size_t fillBuf(ccState cc) {
	if (cc->fin._fin >= 0) {
		unsigned char* base = cc->fin._buf + cc->fin._cnt;
		size_t l, size = sizeof(cc->fin._buf) - cc->fin._cnt;
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
	if(cc->_chr != -1) {
		fatal("can not put back more than one character");
		return -1;
	}
	return cc->_chr = chr;
}

/**
 * @brief set source(file or string) to be parsed.
 * @param cc compiler context.
 * @param src file name or text to pe parsed.
 * @param isFile src is a filename.
 * @return boolean value of success.
 */
int source(ccState cc, int isFile, char* src) {
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
		if ((cc->fin._fin = open(src, O_RDONLY|O_BINARY)) <= 0)
#else
		if ((cc->fin._fin = open(src, O_RDONLY)) <= 0)
#endif
		{
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

//#}

/** Calculate hash of string.
 * @brief calculate the hash of a string.
 * @param str string to be hashed.
 * @param len length of string.
 * @return hash code of the input.
 */
unsigned rehash(const char* str, size_t len) {
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

	if (str != NULL) {
		if (len == -1)
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
char* mapstr(ccState cc, char* str, size_t len/* = -1*/, unsigned hash/* = -1*/) {
	state rt = cc->s;
	list newn, next, prev = 0;

	if (len == -1) {
		len = strlen(str) + 1;
	}
	else if (str[len - 1] != 0) {
		fatal(ERR_INTERNAL_ERROR);
		return NULL;
	}

	if (hash == -1) {
		hash = rehash(str, len) % TBLS;
	}
	else if (hash >= TBLS) {
		fatal(ERR_INTERNAL_ERROR);
		return NULL;
	}

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

	if (rt->_beg >= rt->_end - (sizeof(struct list) + len)) {
		trace(ERR_MEMORY_OVERRUN);
		return NULL;
	}
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
				ccToken type;
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
				{"inline", TYPE_def},
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
						default:
							break;
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

	if (ptr >= end) {
		trace(ERR_MEMORY_OVERRUN);
		return TOKN_err;
	}

	return tok->kind;
}

/** Push back a token.
 * @brief put back token to be read next time.
 * @param cc compiler context.
 * @param tok the token to be pushed back.
 */
void backTok(ccState cc, astn tok) {
	tok->next = cc->_tok;
	cc->_tok = tok;
}

/** Peek the next token.
 * @brief peek the next token from input.
 * @param cc compiler context.
 * @param kind filter: 0 matches everithing.
 * @return next token, or null.
 */
astn peekTok(ccState cc, ccToken kind) {
	if (cc->_tok == NULL) {
		cc->_tok = newnode(cc, TOKN_err);
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
astn next(ccState cc, ccToken kind) {
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
	astn ast = peekTok(cc, TYPE_any);
	if (!ast || (kind && ast->kind != kind)) {
		return 0;
	}
	cc->_tok = ast->next;
	eatnode(cc, ast);
	return 1;
}

// TODO: to be removed.
ccToken test(ccState cc) {
	astn tok = peekTok(cc, TYPE_any);
	return tok ? tok->kind : TYPE_any;
}

// TODO: rename, review.
ccToken skiptok(ccState cc, ccToken kind, int raise) {
	if (!skip(cc, kind)) {
		if (raise) {
			if (!peekTok(cc, TYPE_any)) {
				error(cc->s, cc->file, cc->line, "unexpected end of file, `%t` excepted", kind);
			}
			else {
				error(cc->s, cc->file, cc->line, "`%t` excepted, got `%k`", kind, peekTok(cc, TYPE_any));
			}
		}

		switch (kind) {
			case STMT_do:
			case STMT_end:
			case PNCT_rp:
			case PNCT_rc:
				break;

			default:
				return TYPE_any;
		}
		while (!skip(cc, kind)) {
			if (skip(cc, STMT_do))
				return TYPE_any;
			if (skip(cc, STMT_end))
				return TYPE_any;
			if (skip(cc, PNCT_rp))
				return TYPE_any;
			if (skip(cc, PNCT_rc))
				return TYPE_any;
			if (!skip(cc, TYPE_any))
				return TYPE_any;		// eof?
		}
		return TYPE_any;
	}
	return kind;
}
