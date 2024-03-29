/*******************************************************************************
 *   File: lexer.c
 *   Date: 2011/06/23
 *   Desc: input and lexer
 *******************************************************************************
 * convert text to tokens
 */

#include <limits.h>
#include <fcntl.h>
#include "utils.h"
#include "internal.h"

#if !(defined _MSC_VER)
#include <unistd.h>
#else
#include <io.h>
#endif

static const struct {
	char *name;
	ccToken type;
} keywords[] = {
	// Warning: keep keywords sorted by name, binary search is used to match keywords
	{"break",    STMT_brk},
	{"const",    CONST_kwd},
	{"continue", STMT_con},
	{"else",     ELSE_kwd},
	{"emit",     EMIT_kwd},
	{"enum",     ENUM_kwd},
	{"for",      STMT_for},
	{"if",       STMT_if},
	{"inline",   INLINE_kwd},
	{"return",   STMT_ret},
	{"static",   STATIC_kwd},
	{"struct",   RECORD_kwd}
};

// lexer context
typedef struct lexContext {
	ccContext cc;
	rtContext rt;

	//~ current column = filePos - linePos
	size_t filePos;		// current file position
	int    nextChr;		// next look-ahead character

	// buffered reading
	int     _fin;		// file handle
	char    *_ptr;		// pointer parsing through source
	size_t  _cnt;		// chars left in buffer
	uint8_t _buf[1024];	// memory file buffer
} *lexContext;

/**
 * Fill some characters from the file.
 *
 * @param ctx lexer context.
 * @return number of characters in buffer.
 */
static size_t fillBuf(lexContext ctx) {
	if (ctx->_fin >= 0) {
		memcpy(ctx->_buf, ctx->_ptr, ctx->_cnt);

		void *base = ctx->_buf + ctx->_cnt;
		size_t size = sizeof(ctx->_buf) - ctx->_cnt;
		ssize_t l = read(ctx->_fin, base, size);
		if (l <= 0) {	// end of file or error
			dieif(l < 0, ERR_INTERNAL_ERROR);
			ctx->_buf[ctx->_cnt] = 0;
			close(ctx->_fin);
			ctx->_fin = -1;
		}
		ctx->_ptr = (char*)ctx->_buf;
		ctx->_cnt += l;
	}
	return ctx->_cnt;
}

/**
 * Read the next character from input stream.

 * @param ctx lexer context.
 * @return the next character or -1 on end, or error.
 */
static int readChr(lexContext ctx) {
	int chr = ctx->nextChr;
	if (chr != -1) {
		ctx->nextChr = -1;
		return chr;
	}

	// fill in the buffer.
	if (ctx->_cnt < 2 && fillBuf(ctx) < 1) {
		return -1;
	}

	chr = *ctx->_ptr;
	if (chr == '\n' || chr == '\r') {

		// threat 'cr', 'lf' and 'cr + lf' as new line (lf: '\n')
		if (chr == '\r' && ctx->_ptr[1] == '\n') {
			ctx->_cnt -= 1;
			ctx->_ptr += 1;
			ctx->filePos += 1;
		}

		// advance to next line
		if (ctx->cc->line > 0) {
			ctx->cc->line += 1;
		}

		chr = '\n';
	}

	ctx->_cnt -= 1;
	ctx->_ptr += 1;
	ctx->filePos += 1;
	return chr;
}

/**
 * Peek the next character from input stream.
 *
 * @param ctx lexer context.
 * @return the next character or -1 on end, or error.
 */
static int peekChr(lexContext ctx) {
	if (ctx->nextChr == -1) {
		ctx->nextChr = readChr(ctx);
	}
	return ctx->nextChr;
}

/**
 * Skip the next character if it matches `chr`.
 *
 * @param ctx lexer context.
 * @param chr filter: 0 matches everything.
 * @return the character skipped.
 */
static int skipChr(lexContext ctx, int chr) {
	if (!chr || chr == peekChr(ctx)) {
		return readChr(ctx);
	}
	return 0;
}

/**
 * Push back a character to be read next time.
 *
 * @param ctx lexer context.
 * @param chr the character to be pushed back.
 * @return the character pushed back, -1 on fail.
 */
static int backChr(lexContext ctx, int chr) {
	if(ctx->nextChr != -1) {
		// can not put back more than one character
		fatal(ERR_INTERNAL_ERROR);
		return -1;
	}
	return ctx->nextChr = chr;
}

/**
 * Read the next token from input stream.
 *
 * @param ctx lexer context.
 * @param tok (out parameter) to be filled with data.
 * @return the kind of token, TOKEN_any (0) if error occurred.
 */
static ccToken readTok(lexContext ctx, astn tok) {
	enum {
		OTHER = 0x00000000,

		SPACE = 0x00000100,    // white spaces
		UNDER = 0x00000200,    // underscore
		PUNCT = 0x00000400,    // punctuation
		CNTRL = 0x00000800,    // control characters

		DIGIT = 0x00001000,    // digits: 0-9
		LOWER = 0x00002000,    // lowercase letters: a-z
		UPPER = 0x00004000,    // uppercase letters: A-Z
		SCTRL = CNTRL|SPACE,   // whitespace or control chars
		ALPHA = UPPER|LOWER,   // letters
		ALNUM = DIGIT|ALPHA,   // letters and numbers
		CWORD = UNDER|ALNUM,   // letters, numbers and underscore
	};
	static int const chr_map[256] = {
		/* 000 nul */	CNTRL,
		/* 001 soh */	CNTRL,
		/* 002 stx */	CNTRL,
		/* 003 etx */	CNTRL,
		/* 004 eot */	CNTRL,
		/* 005 enq */	CNTRL,
		/* 006 ack */	CNTRL,
		/* 007 bel */	CNTRL,
		/* 010 bs  */	CNTRL,
		/* 011 ht  */	SCTRL,	// horizontal tab
		/* 012 nl  */	SCTRL,	// \n
		/* 013 vt  */	SCTRL,	// vertical tab,
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
	const char *end = (char*)ctx->rt->_end;
	char *beg = (char*)ctx->rt->_beg;
	int chr = readChr(ctx);

	// skip white spaces and comments
	while (chr != -1) {
		if (chr == '/') {
			const int line = ctx->cc->line;      // comment begin line
			const int next = peekChr(ctx);       // comment begin char
			char *doc = NULL;
			char *ptr = beg;
			int newLine = INT_MAX;

			// line comment
			if (next == '/') {
				chr = readChr(ctx);
				chr = readChr(ctx);
				if (chr == '/') {
					// '/// ...'
					doc = ptr;
				}

				while (ptr < end && chr != -1) {
					if (chr == '\n') {
						break;
					}
					chr = readChr(ctx);
					if (newLine && !(chr_map[chr & 0xff] & SPACE)) {
						newLine = 0;
					}
					if (!newLine) {
						*ptr++ = (char) chr;
					}
				}
				if (chr == -1) {
					warn(ctx->rt, raise_warn_lex9, ctx->cc->file, line, WARN_NO_NEW_LINE_AT_END);
				}
				else if (ctx->cc->line != line + 1) {
					warn(ctx->rt, raise_warn_lex9, ctx->cc->file, line, WARN_COMMENT_MULTI_LINE, ptr);
				}
			}

			// block comment
			else if (next == '*') {
				chr = readChr(ctx);
				chr = readChr(ctx);
				if (chr == '*') {
					// '/** ...'
					if (peekChr(ctx) != '/') {
						doc = ptr;
					}
				}

				int level = 1;
				while (ptr < end && chr != -1) {
					int prev_chr = chr;
					chr = readChr(ctx);

					if (prev_chr == next && chr == '/') {
						level -= 1;
						if (level == 0) {
							chr = ' ';
							break;
						}
					}
					if (prev_chr == '/' && chr == next) {
						level += 1;
						if (level > 1 && next == '*') {
							warn(ctx->rt, raise_warn_lex9, ctx->cc->file, ctx->cc->line, WARN_COMMENT_NESTED);
							level = 1;
						}
						chr = 0;	// disable reading as valid comment: /*/
					}

					if (newLine > 0 && !(chr_map[chr & 0xff] & SPACE)) {
						if (chr != next) {
							newLine = 0;
						}
					}

					if (newLine == 0) {
						*ptr++ = (char) chr;
						if (chr == '\n') {
							newLine = INT_MAX;
						}
					}
					else if (chr == next) {
						newLine = 1;
					}
					else {
						newLine -= 1;
					}
				}

				if (chr == -1) {
					error(ctx->rt, ctx->cc->file, line, ERR_INVALID_COMMENT);
				}
			}

			if (doc != NULL) {
				if (!ctx->cc->genDocumentation) {
					doc = (char *) type_doc_public;
					ptr = doc + strlen(doc) + 1;
				}
				else {
					if (ptr == doc) {
						ptr = beg + 1;
					}
					ptr[-1] = 0;
				}

				memset(tok, 0, sizeof(*tok));
				tok->kind = TOKEN_doc;
				tok->type = ctx->cc->type_vid;
				tok->file = ctx->cc->file;
				tok->line = line;
				tok->id.hash = rehash(doc, ptr - doc) % hashTableSize;
				tok->id.name = ccUniqueStr(ctx->cc, doc, ptr - doc, tok->id.hash);
				return TOKEN_doc;
			}
		}

		if (chr_map[chr & 0xff] == CNTRL) {
			warn(ctx->rt, raiseWarn, ctx->cc->file, ctx->cc->line, ERR_INVALID_CHARACTER, chr);
			while (chr == 0) {
				chr = readChr(ctx);
			}
		}

		if (!(chr_map[chr & 0xff] & SPACE)) {
			break;
		}

		chr = readChr(ctx);
	}

	if (tok == NULL) {
		return TOKEN_any;
	}

	char *ptr = beg = (char*)ctx->rt->_beg;

	// our token begins here
	memset(tok, 0, sizeof(*tok));
	tok->file = ctx->cc->file;
	tok->line = ctx->cc->line;

	// scan
	if (chr != -1) switch (chr) {
		default:
			if (chr_map[chr & 0xff] & DIGIT) {
				goto read_num;
			}
			if (chr_map[chr & 0xff] & CWORD) {
				goto read_idf;
			}
			error(ctx->rt, tok->file, tok->line, ERR_INVALID_CHARACTER, chr);
			tok->kind = TOKEN_any;
			break;

		case '.':
			if (chr_map[peekChr(ctx) & 0xff] == DIGIT) {
				goto read_num;
			}
			tok->kind = OPER_dot;
			if (skipChr(ctx, '.')) {
				if (skipChr(ctx, '.')) {
					tok->kind = PNCT_dot3;
				}
				else {
					tok->kind = TOKEN_any;
				}
			}
			break;

		case ';':
			tok->kind = STMT_end;
			break;

		case ',':
			tok->kind = OPER_com;
			break;

		case '{':
			tok->kind = STMT_beg;
			break;

		case '[':
			tok->kind = LEFT_sqr;
			break;

		case '(':
			tok->kind = LEFT_par;
			break;

		case '}':
			tok->kind = RIGHT_crl;
			break;

		case ']':
			tok->kind = RIGHT_sqr;
			break;

		case ')':
			tok->kind = RIGHT_par;
			break;

		case '?':
			tok->kind = PNCT_qst;
			break;

		case ':':
			tok->kind = PNCT_cln;
			break;

		case '~':
			tok->kind = OPER_cmt;
			break;

		case '=':
			if (skipChr(ctx, '=')) {
				tok->kind = OPER_ceq;
				break;
			}
			tok->kind = ASGN_set;
			break;

		case '!':
			if (skipChr(ctx, '=')) {
				tok->kind = OPER_cne;
				break;
			}
			tok->kind = OPER_not;
			break;

		case '>':
			chr = peekChr(ctx);
			if (chr == '>') {
				readChr(ctx);
				if (skipChr(ctx, '=')) {
					tok->kind = ASGN_shr;
					break;
				}
				tok->kind = OPER_shr;
				break;
			}
			if (chr == '=') {
				readChr(ctx);
				tok->kind = OPER_cge;
				break;
			}
			tok->kind = OPER_cgt;
			break;

		case '<':
			chr = peekChr(ctx);
			if (chr == '<') {
				readChr(ctx);
				if (skipChr(ctx, '=')) {
					tok->kind = ASGN_shl;
					break;
				}
				tok->kind = OPER_shl;
				break;
			}
			if (chr == '=') {
				readChr(ctx);
				tok->kind = OPER_cle;
				break;
			}
			tok->kind = OPER_clt;
			break;

		case '&':
			chr = peekChr(ctx);
			if (chr == '=') {
				readChr(ctx);
				tok->kind = ASGN_and;
				break;
			}
			if (chr == '&') {
				readChr(ctx);
				tok->kind = OPER_all;
				break;
			}
			tok->kind = OPER_and;
			break;

		case '|':
			chr = peekChr(ctx);
			if (chr == '=') {
				readChr(ctx);
				tok->kind = ASGN_ior;
				break;
			}
			if (chr == '|') {
				readChr(ctx);
				tok->kind = OPER_any;
				break;
			}
			tok->kind = OPER_ior;
			break;

		case '^':
			if (skipChr(ctx, '=')) {
				tok->kind = ASGN_xor;
				break;
			}
			tok->kind = OPER_xor;
			break;

		case '+':
			if (skipChr(ctx, '=')) {
				tok->kind = ASGN_add;
				break;
			}
			if (skipChr(ctx, '+')) {
				// disable `++` operator
				error(ctx->rt, ctx->cc->file, ctx->cc->line, ERR_INVALID_CHARACTER, chr);
				tok->kind = TOKEN_any;
				break;
			}
			tok->kind = OPER_add;
			break;

		case '-':
			if (skipChr(ctx, '=')) {
				tok->kind = ASGN_sub;
				break;
			}
			if (skipChr(ctx, '-')) {
				// disable `--` operator
				error(ctx->rt, ctx->cc->file, ctx->cc->line, ERR_INVALID_CHARACTER, chr);
				tok->kind = TOKEN_any;
				break;
			}
			tok->kind = OPER_sub;
			break;

		case '*':
			if (skipChr(ctx, '=')) {
				tok->kind = ASGN_mul;
				break;
			}
			tok->kind = OPER_mul;
			break;

		case '/':
			if (skipChr(ctx, '=')) {
				tok->kind = ASGN_div;
				break;
			}
			tok->kind = OPER_div;
			break;

		case '%':
			if (skipChr(ctx, '=')) {
				tok->kind = ASGN_mod;
				break;
			}
			tok->kind = OPER_mod;
			break;

		case '\'':			// \'[^\'\n]*
		case '"': {			// \"[^\"\n]*
			int multi_line = 0;		// multi line string constant.
			int start_char = chr;	// literals start character.

			while (ptr < end && (chr = readChr(ctx)) != -1) {

				// end reached
				if (chr == start_char)
					break;

				// new line reached
				if (chr == '\n' && !multi_line)
					break;

				// escape sequence
				if (chr == '\\') {
					int oct, hex1, hex2;
					switch (chr = readChr(ctx)) {
						default:
							error(ctx->rt, tok->file, tok->line, ERR_INVALID_ESC_SEQ, chr);
							chr = 0;
							break;

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
							if ((chr = peekChr(ctx)) >= '0' && chr <= '7') {
								oct = (oct << 3) | (chr - '0');
								readChr(ctx);
								if ((chr = peekChr(ctx)) >= '0' && chr <= '7') {
									oct = (oct << 3) | (chr - '0');
									readChr(ctx);
								}
							}
							if (oct & 0xffffff00) {
								warn(ctx->rt, raise_warn_lex2, tok->file, tok->line, WARN_OCT_ESC_SEQ_OVERFLOW);
							}
							chr = oct & 0xff;
							break;

						case 'x':
							// hexadecimal sequence (length = 2)
							hex1 = readChr(ctx);
							hex2 = readChr(ctx);
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
								error(ctx->rt, tok->file, tok->line, ERR_INVALID_HEX_SEQ, hex1);
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
								error(ctx->rt, tok->file, tok->line, ERR_INVALID_HEX_SEQ, hex2);
								break;
							}
							chr = hex1 << 4 | hex2;
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
							break;
					}
				}
				*ptr++ = (char) chr;
			}
			*ptr++ = 0;

			if (chr != start_char) {
				error(ctx->rt, tok->file, tok->line, ERR_INVALID_LITERAL, start_char);
				return tok->kind = TOKEN_any;
			}

			if (start_char == '"') {
				tok->kind = TOKEN_val;
				tok->type = ctx->cc->type_str;
				tok->id.hash = rehash(beg, ptr - beg) % hashTableSize;
				tok->id.name = ccUniqueStr(ctx->cc, beg, ptr - beg, tok->id.hash);
			}
			else {
				int val = 0;

				if (ptr == beg) {
					error(ctx->rt, tok->file, tok->line, ERR_EMPTY_CHAR_CONSTANT);
					return tok->kind = TOKEN_any;
				}
				if (ptr > beg + vm_stk_align + 1) {
					warn(ctx->rt, raise_warn_lex2, tok->file, tok->line, WARN_CHR_CONST_OVERFLOW, ptr);
				}
				else if (ptr > beg + ctx->cc->type_chr->size + 1) {
					warn(ctx->rt, raise_warn_lex3, tok->file, tok->line, WARN_MULTI_CHAR_CONSTANT);
				}

				for (ptr = beg; *ptr; ++ptr) {
					val = val << 8 | *ptr;
				}

				tok->kind = TOKEN_val;
				tok->type = ctx->cc->type_chr;
				tok->cInt = val;
			}
			break;
		}
		read_idf: {			// [a-zA-Z_][a-zA-Z0-9_]*
			int lo = 0;
			int hi = lengthOf(keywords);
			int cmp = -1;

			while (ptr < end && chr != -1) {
				if (!(chr_map[chr & 0xff] & CWORD)) {
					break;
				}
				*ptr++ = (char) chr;
				chr = readChr(ctx);
			}
			backChr(ctx, chr);
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
				switch (keywords[hi].type) {
					default:
						tok->kind = keywords[hi].type;
						break;

					case EMIT_kwd:
						tok->kind = TOKEN_var;
						tok->type = ctx->cc->emit_opc;
						tok->id.link = ctx->cc->emit_opc;
						tok->id.hash = rehash(beg, ptr - beg) % hashTableSize;
						tok->id.name = ccUniqueStr(ctx->cc, beg, ptr - beg, tok->id.hash);
						break;
				}
			}
			else {
				tok->kind = TOKEN_var;
				tok->type = tok->id.link = NULL;
				tok->id.hash = rehash(beg, ptr - beg) % hashTableSize;
				tok->id.name = ccUniqueStr(ctx->cc, beg, ptr - beg, tok->id.hash);
			}
			break;
		}
		read_num: {			// int | ([0-9]+'.'[0-9]* | '.'[0-9]+)([eE][+-]?[0-9]+)?
			//~ 0[.oObBxX]?
			int radix = 10;
			if (chr == '0') {
				*ptr++ = (char) chr;
				chr = readChr(ctx);

				switch (chr) {
					default:
						radix = 8;
						break;

					case '.':		// do not skip
						break;

					case 'b':
					case 'B':
						radix = 2;
						*ptr++ = (char) chr;
						chr = readChr(ctx);
						break;

					case 'o':
					case 'O':
						radix = 8;
						*ptr++ = (char) chr;
						chr = readChr(ctx);
						break;

					case 'x':
					case 'X':
						radix = 16;
						*ptr++ = (char) chr;
						chr = readChr(ctx);
						break;
				}
			}

			//~ ([0-9a-fA-F])*
			int overflow = 0;
			int64_t i64v = 0;
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

				// check for overflow
				if (radix == 10 && i64v > (0x7fffffffffffffffLL - value) / radix) {
					overflow = 1;
				}

				i64v = i64v * radix + value;

				*ptr++ = (char) chr;
				chr = readChr(ctx);
			}

			if (overflow != 0) {
				warn(ctx->rt, raise_warn_lex2, tok->file, tok->line, WARN_VALUE_OVERFLOW);
			}

			ccKind cast = CAST_i64;
			if ((int32_t)i64v == i64v) {
				cast = CAST_i32;
			}

			//~ ('.'[0-9]*)? ([eE]([+-]?)[0-9]+)?
			float64_t f64v = i64v;
			if (radix == 10) {

				if (chr == '.') {
					long double val = 0;
					long double exp = 1;

					*ptr++ = (char) chr;
					chr = readChr(ctx);

					while (chr >= '0' && chr <= '9') {
						val = val * 10 + (chr - '0');
						exp *= 10;

						*ptr++ = (char) chr;
						chr = readChr(ctx);
					}

					f64v += val / exp;
					cast = CAST_f64;
				}

				if (chr == 'e' || chr == 'E') {
					*ptr++ = (char) chr;
					chr = readChr(ctx);

					int sgn = 1;
					switch (chr) {
						case '-':
							sgn = -1;
							// fall through
						case '+':
							*ptr++ = (char) chr;
							chr = readChr(ctx);
						default:
							break;
					}

					overflow = 0;
					char *suffix = ptr;
					unsigned int val = 0;
					while (chr >= '0' && chr <= '9') {
						val = val * 10 + (chr - '0');
						if (val > 1024) {
							overflow = 1;
						}

						*ptr++ = (char) chr;
						chr = readChr(ctx);
					}

					if (suffix == ptr) {
						error(ctx->rt, tok->file, tok->line, ERR_INVALID_EXPONENT);
					}
					else if (overflow) {
						warn(ctx->rt, raise_warn_lex2, tok->file, tok->line, WARN_EXPONENT_OVERFLOW);
					}

					long double p = 1;
					long double m = 10;
					while (val) {		// pow(10, val);
						if (val & 1) {
							p *= m;
						}
						val >>= 1;
						m *= m;
					}

					if (sgn < 0) {
						f64v /= p;
					} else {
						f64v *= p;
					}
					cast = CAST_f64;
				}
			}

			char *suffix = ptr;
			while (ptr < end && chr != -1) {
				if (!(chr_map[chr & 0xff] & CWORD)) {
					break;
				}
				*ptr++ = (char) chr;
				chr = readChr(ctx);
			}
			backChr(ctx, chr);
			*ptr++ = 0;

			if (*suffix) {
				if (suffix[0] == 'd' && suffix[1] == 0) {
					cast = CAST_i32;
				}
				else if (suffix[0] == 'D' && suffix[1] == 0) {
					cast = CAST_i64;
				}
				else if (suffix[0] == 'u' && suffix[1] == 0) {
					cast = CAST_u32;
				}
				else if (suffix[0] == 'U' && suffix[1] == 0) {
					cast = CAST_u64;
				}
				else if (suffix[0] == 'f' && suffix[1] == 0) {
					cast = CAST_f32;
				}
				else if (suffix[0] == 'F' && suffix[1] == 0) {
					cast = CAST_f64;
				}
				else {
					error(ctx->rt, tok->file, tok->line, ERR_INVALID_SUFFIX, suffix);
					tok->kind = TOKEN_any;
					cast = CAST_any;
				}
			}

			switch (cast) {
				default:
					tok->kind = TOKEN_any;
					break;
				case CAST_i32:
					tok->kind = TOKEN_val;
					tok->type = ctx->cc->type_i32;
					tok->cInt = i64v;
					break;
				case CAST_i64:
					tok->kind = TOKEN_val;
					tok->type = ctx->cc->type_i64;
					tok->cInt = i64v;
					break;
				case CAST_u32:
					tok->kind = TOKEN_val;
					tok->type = ctx->cc->type_u32;
					tok->cInt = i64v;
					break;
				case CAST_u64:
					tok->kind = TOKEN_val;
					tok->type = ctx->cc->type_u64;
					tok->cInt = i64v;
					break;
				case CAST_f32:
					tok->kind = TOKEN_val;
					tok->type = ctx->cc->type_f32;
					tok->cFlt = f64v;
					break;
				case CAST_f64:
					tok->kind = TOKEN_val;
					tok->type = ctx->cc->type_f64;
					tok->cFlt = f64v;
					break;
			}
			break;
		}
	}

	if (ptr >= end) {
		fatal(ERR_MEMORY_OVERRUN);
		return TOKEN_any;
	}

	return tok->kind;
}

astn peekTok(ccContext cc, ccToken match) {
	// read lookahead token
	astn token = cc->tokNext;
	while (token != NULL) {
		if (token->kind != TOKEN_doc) {
			break;
		}
		token = token->next;
	}
	if (token == NULL) {
		return NULL;
	}
	if (match && match != token->kind) {
		return NULL;
	}
	return token;
}

astn nextTok(ccContext cc, ccToken match, int raise) {
	astn token = peekTok(cc, match);
	if (token != NULL) {
		while (cc->tokNext != NULL) {
			astn temp = cc->tokNext;
			if (temp == token) {
				break;
			}
			cc->tokNext = temp->next;
			recycle(cc, temp);
		}
		cc->tokNext = token->next;
		token->next = NULL;
		return token;
	}
	if (raise) {
		const char *file = cc->file;
		int line = cc->line;
		token = cc->tokNext;
		if (token && token->file && token->line) {
			file = token->file;
			line = token->line;
		}
		error(cc->rt, file, line, ERR_UNMATCHED_TOKEN, token, match);
	}
	return NULL;
}

ccToken skipTok(ccContext cc, ccToken match, int raise) {
	astn token = nextTok(cc, match, raise);
	if (token != NULL) {
		ccToken result = token->kind;
		recycle(cc, token);
		return result;
	}
	return TOKEN_any;
}

astn backTok(ccContext cc, astn token) {
	if (token != NULL) {
		token->next = cc->tokNext;
		cc->tokNext = token;
	}
	return token;
}

int ccOpen(ccContext cc, const char *file, int line, char *text) {
	struct lexContext input;
	if (text == NULL && file != NULL) {
		if ((input._fin = open(file, O_RDONLY)) <= 0) {
			return -1;
		}
		input._ptr = 0;
		input._cnt = 0;
	}
	else if (text != NULL) {
		input._fin = -1;
		input._ptr = text;
		input._cnt = strlen(text);
	}
	else {
		input._fin = -1;
		input._ptr = 0;
		input._cnt = 0;
	}

	if (file != NULL) {
		file = ccUniqueStr(cc, file, -1, -1);
	}
	input.cc = cc;
	input.rt = cc->rt;
	input.filePos = 0;
	input.nextChr = -1;

	cc->file = file;
	cc->line = line;

	if (fillBuf(&input) > 2) {
		// skip first line if it begins with: #!
		if (input._ptr[0] == '#' && input._ptr[1] == '!') {
			int chr = readChr(&input);
			while (chr != -1) {
				if (chr == '\n') {
					break;
				}
				chr = readChr(&input);
			}
		}
	}

	astn head = NULL;
	astn tail = NULL;
	for ( ; ; ) {
		astn tok = newNode(cc, TOKEN_any);
		if (tok == NULL) {
			break;
		}
		if (!readTok(&input, tok)) {
			recycle(cc, tok);
			break;
		}
		if (head == NULL) {
			head = tok;
		}
		if (tail != NULL) {
			tail->next = tok;
		}
		tail = tok;
	}
	if (tail != NULL) {
		tail->next = cc->tokNext;
	}
	if (head != NULL) {
		cc->tokNext = head;
	}
	return 0;
}

int ccClose(ccContext cc) {
	// not initialized
	if (cc == NULL) {
		return -1;
	}

	// check no token left to read
	astn ast = nextTok(cc, TOKEN_any, 0);
	if (ast != NULL) {
		error(cc->rt, ast->file, ast->line, ERR_UNEXPECTED_TOKEN, ast);
	}

	// close input
	ccOpen(cc, NULL, 0, NULL);

	// return errors
	return cc->rt->errors;
}
