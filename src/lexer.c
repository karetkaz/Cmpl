#include "lexer.h"
#include "code.h"
#include "util.h"
#include "printer.h"
#include "compiler.h"

#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

// Lexer errors and warnings
#define ERR_OPENING_FILE "can not open file: %s"
#define ERR_UNMATCHED_TOKEN "unexpected token `%?.t`, matching `%.k`"
#define ERR_UNPROCESSED_TOKEN "unprocessed token `%.t`"
#define ERR_UNEXPECTED_END_OF_FILE "unexpected end of file"
#define WARN_NO_NEW_LINE_AT_END "expected <new line> before end of input"
#define WARN_EMPTY_FILE "empty file"

// comments
#define ERR_COMMENT_INVALID "unterminated block comment"
#define WARN_COMMENT_NESTED "ignoring nested comment"
#define WARN_COMMENT_MULTI_LINE "multi-line comment: `%s`"

// numbers
#define ERR_OCT_ESC_SEQ_DEPRECATED "deprecated octal escape sequence, use 'o' or 'O' to differentiate base: 0o1754"
#define ERR_INVALID_HEX_SEQ "hex escape sequence invalid '%c'"
#define ERR_INVALID_EXPONENT "no exponent in numeric constant"
#define ERR_INVALID_NUMBER "invalid numeric constant: '%s'"
#define WARN_VALUE_OVERFLOW "value overflow"
#define WARN_EXPONENT_OVERFLOW "exponent overflow"

// char and text
#define ERR_INVALID_CHARACTER "invalid character: '%c'"
#define ERR_INVALID_ESC_SEQ "invalid escape sequence '\\%c'"
#define ERR_INVALID_LITERAL "unclosed '%c' literal"
#define ERR_INVALID_SUFFIX "invalid suffix in numeric constant '%s'"
// todo: remove
#define ERR_EMPTY_CHAR_CONSTANT "empty character constant"
#define WARN_MULTI_CHAR_CONSTANT "multi character constant"
#define WARN_CHR_CONST_OVERFLOW "character constant truncated"

static const struct {
	char *name;
	ccToken type;
} keywords[] = {
	{"break",    STMT_brk},
	{"continue", STMT_con},
	{"else",     ELSE_kwd},
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
	rtContext rt;
	ccContext cc;

	//~ current column = filePos - linePos
	size_t filePos;         // current file position
	size_t linePos;         // current line position
	int nextChr;            // next look-ahead character

	// buffered reading
	int fid;               // file handle
	char *ptr;             // pointer parsing through the source
	size_t cnt;            // chars left in the buffer
	uint8_t buff[1024];    // memory buffer
} *lexContext;

enum {
	OTHER = 0x00,

	SPACE = 0x01,    // white spaces
	UNDER = 0x02,    // underscore
	PUNCT = 0x04,    // punctuation
	CNTRL = 0x08,    // control characters

	DIGIT = 0x10,    // digits: 0-9
	LOWER = 0x20,    // lowercase letters: a-z
	UPPER = 0x40,    // uppercase letters: A-Z
	SCTRL = CNTRL | SPACE,   // whitespace or control chars
	ALPHA = UPPER | LOWER,   // letters
	ALNUM = DIGIT | ALPHA,   // letters and numbers
	CWORD = UNDER | ALNUM,   // letters, numbers and underscore
};
static int const chr_map[256];

/**
 * Fill some characters from the file.
 *
 * @param ctx Lexer context.
 * @return number of characters in the buffer.
 */
static size_t fillBuffer(lexContext ctx) {
	dieif(ctx == NULL, ERR_INTERNAL_ERROR);
	if (ctx->fid < 0) {
		return ctx->cnt;
	}

	memmove(ctx->buff, ctx->ptr, ctx->cnt);
	ctx->ptr = (char *) ctx->buff;

	void *base = ctx->buff + ctx->cnt;
	size_t size = sizeof(ctx->buff) - ctx->cnt;
	ssize_t l = read(ctx->fid, base, size);
	if (l <= 0) {
		// end of file or error
		dieif(l < 0, ERR_INTERNAL_ERROR);
		ctx->buff[ctx->cnt] = 0;
	} else {
		ctx->cnt += l;
	}
	return ctx->cnt;
}

/**
 * Read the next character from the input stream.
 *
 * @param ctx Lexer context.
 * @return The next character or -1 on end, or error.
 */
static int readChar(lexContext ctx) {
	int chr = ctx->nextChr;
	if (chr != -1) {
		ctx->nextChr = -1;
		return chr;
	}

	// fill the buffer with characters, at least 2 chars needed to check cr + lf.
	if (ctx->cnt < 2 && fillBuffer(ctx) < 1) {
		return -1;
	}

	chr = *(unsigned char *) ctx->ptr;
	if (chr == '\n' || chr == '\r') {
		// threat 'cr', 'lf' and 'cr + lf' as new line (lf: '\n')
		if (chr == '\r' && ctx->ptr[1] == '\n') {
			ctx->cnt -= 1;
			ctx->ptr += 1;
			ctx->filePos += 1;
		}

		// advance to next line
		if (ctx->cc->line > 0) {
			ctx->cc->line += 1;
		}

		// save where the next line begins.
		ctx->linePos = ctx->filePos + 1;
		chr = '\n';
	}

	ctx->cnt -= 1;
	ctx->ptr += 1;
	ctx->filePos += 1;
	return chr;
}

/**
 * Push back a character to be read next time.
 *
 * @param ctx Lexer context.
 * @param chr The character to be pushed back.
 * @return The character pushed back.
 */
static int backChar(lexContext ctx, int chr) {
	// cannot push back more than one character
	dieif(ctx->nextChr != -1, ERR_INTERNAL_ERROR);
	ctx->nextChr = chr;
	return chr;
}

/**
 * Peek the next character from the input stream.
 *
 * @param ctx Lexer context.
 * @return The next character or -1 on end, or error.
 */
static int peekChar(lexContext ctx) {
	// return backChar(ctx, readChar(ctx));
	int chr = ctx->nextChr;
	if (chr != -1) {
		return chr;
	}

	chr = readChar(ctx);
	ctx->nextChr = chr;
	return chr;
}

/**
 * Skip the next character if it matches `match`.
 *
 * @param ctx Lexer context.
 * @param match Character to be matched and skipped.
 * @return The character skipped or 0 if did not match.
 */
static int skipChar(lexContext ctx, int match) {
	int result = readChar(ctx);
	if (match != result) {
		backChar(ctx, result);
		return 0;
	}
	return result;
}

static ccToken readNumber(lexContext ctx, astn tok, int chr) {
	const char *end = (char *) ctx->rt->_end;
	const char *beg = (char *) ctx->rt->_beg;
	char *ptr = (char *) ctx->rt->_beg;

	// int | ([0-9]+'.'[0-9]* | '.'[0-9]+)([eE][+-]?[0-9]+)?
	int radix = 10;
	int overflow = 0;
	ccKind cast = CAST_i64;

	//~ 0[.oObBxX]?
	if (chr == '0') {
		*ptr++ = (char) chr;
		chr = readChar(ctx);

		switch (chr) {
			default:
				if (chr >= '0' && chr <= '9') {
					warn(ctx->rt, tok->file, tok->line, ERR_OCT_ESC_SEQ_DEPRECATED);
					radix = 8;
				}
				break;

			case '.': // do not skip
				break;

			case 'b':
			case 'B':
				radix = 2;
				*ptr++ = (char) chr;
				chr = readChar(ctx);
				break;

			case 'o':
			case 'O':
				radix = 8;
				*ptr++ = (char) chr;
				chr = readChar(ctx);
				break;

			case 'x':
			case 'X':
				radix = 16;
				*ptr++ = (char) chr;
				chr = readChar(ctx);
				break;
		}
	}

	//~ ([0-9a-fA-F])*
	int64_t valueInt = 0;
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
		if (radix == 10 && valueInt > (0x7fffffffffffffffLL - value) / radix) {
			overflow = 1;
		}

		valueInt = valueInt * radix + value;

		*ptr++ = (char) chr;
		chr = readChar(ctx);
	}

	//~ ('.'[0-9]*)?([eE]([+-]?)[0-9]+)?
	float64_t valueFlt = (float64_t) valueInt;
	if (radix == 10) {
		if (chr == '.') {
			*ptr++ = (char) chr;
			chr = readChar(ctx);

			while (chr >= '0' && chr <= '9') {
				*ptr++ = (char) chr;
				chr = readChar(ctx);
			}
			cast = CAST_f64;
		}

		if (chr == 'e' || chr == 'E') {
			*ptr++ = (char) chr;
			chr = readChar(ctx);

			switch (chr) {
				case '-':
				case '+':
					*ptr++ = (char) chr;
					chr = readChar(ctx);
				default:
					break;
			}

			int exp = 0;
			const char *suffix = ptr;
			while (chr >= '0' && chr <= '9') {
				exp = exp * 10 + (chr - '0');
				if (exp > 1024) {
					overflow = 1;
				}

				*ptr++ = (char) chr;
				chr = readChar(ctx);
			}

			if (suffix == ptr) {
				error(ctx->rt, tok->file, tok->line, ERR_INVALID_EXPONENT);
				return tok->kind = TOKEN_any;;
			}
			cast = CAST_f64;
		}

		*ptr = 0;
		char *suffix = ptr;
		valueFlt = strtod(beg, &suffix);
		if (suffix != ptr) {
			error(ctx->rt, tok->file, tok->line, ERR_INVALID_NUMBER, beg);
			return tok->kind = TOKEN_any;
		}
	}

	// todo: remove suffixes and keep only two types: int64 and float64
	const char *suffix = ptr;
	while (chr != -1) {
		dieif(ptr >= end, ERR_MEMORY_OVERRUN);
		if (!(chr_map[chr & 0xff] & CWORD)) {
			backChar(ctx, chr);
			*ptr = 0;

			if (ptr - suffix > 1) {
				error(ctx->rt, tok->file, tok->line, ERR_INVALID_SUFFIX, suffix);
				return tok->kind = TOKEN_any;;
			}

			// todo: do not ignoring errors
			switch (*suffix) {
				default:
					break;

				case 0:
					if (cast == CAST_i64) {
						if ((int32_t) valueInt == valueInt) {
							cast = CAST_i32;
						}
					}
					break;

				case 'u':
				case 'U':
				case 'd':
				case 'D':
					if (cast == CAST_f64) {
						error(ctx->rt, tok->file, tok->line, ERR_INVALID_SUFFIX, beg);
						return tok->kind = TOKEN_any;
					}
					break;

				case 'f':
				case 'F':
					if (radix != 10) {
						error(ctx->rt, tok->file, tok->line, ERR_INVALID_SUFFIX, beg);
						return tok->kind = TOKEN_any;
					}
					break;
			}
			switch (*suffix) {
				default:
					error(ctx->rt, tok->file, tok->line, ERR_INVALID_SUFFIX, suffix);
					return tok->kind = TOKEN_any;

				case 0:
					break;

				case 'u':
					cast = CAST_u32;
					break;

				case 'U':
					cast = CAST_u64;
					break;

				case 'd':
					cast = CAST_i32;
					break;

				case 'D':
					cast = CAST_i64;
					break;

				case 'f':
					cast = CAST_f32;
					break;

				case 'F':
					cast = CAST_f64;
					break;
			}
			break;
		}
		*ptr++ = (char) chr;
		chr = readChar(ctx);
	}

	if (overflow != 0) {
		warn(ctx->rt, tok->file, tok->line, WARN_VALUE_OVERFLOW);
	}

	switch (cast) {
		default: break;

		case CAST_i32:
			tok->cInt = valueInt;
			tok->type = ctx->cc->type_i32;
			return tok->kind = TOKEN_val;

		case CAST_i64:
			tok->cInt = valueInt;
			tok->type = ctx->cc->type_i64;
			return tok->kind = TOKEN_val;

		case CAST_u32:
			tok->cInt = valueInt;
			tok->type = ctx->cc->type_u32;
			return tok->kind = TOKEN_val;

		case CAST_u64:
			tok->cInt = valueInt;
			tok->type = ctx->cc->type_u64;
			return tok->kind = TOKEN_val;

		case CAST_f32:
			tok->cFlt = valueFlt;
			tok->type = ctx->cc->type_f32;
			return tok->kind = TOKEN_val;

		case CAST_f64:
			tok->cFlt = valueFlt;
			tok->type = ctx->cc->type_f64;
			return tok->kind = TOKEN_val;
	}

	error(ctx->rt, tok->file, tok->line, ERR_INVALID_NUMBER, beg);
	return tok->kind = TOKEN_any;
}

static ccToken readText(lexContext ctx, astn tok, int chr) {
	const char *end = (char *) ctx->rt->_end;
	const char *beg = (char *) ctx->rt->_beg;
	char *ptr = (char *) ctx->rt->_beg;

	int multi_line = 0; // multi-line string constant.
	int start_char = chr; // the literal start character.

	while (chr > 0) {
		dieif(ptr >= end, ERR_MEMORY_OVERRUN);
		chr = readChar(ctx);

		// end reached
		if (chr == start_char) {
			break;
		}

		// new line reached
		if (chr == '\n' && !multi_line)
			break;

		// escape sequence
		if (chr == '\\') {
			int hex1, hex2;
			switch (chr = readChar(ctx)) {
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

				case 'x':
					// hexadecimal sequence (length = 2)
					hex1 = readChar(ctx);
					hex2 = readChar(ctx);
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

	// todo: make all string literals, can be implicitly casted to char, if contains a single character
	if (start_char == '\'') {
		int val = 0;

		if (ptr == beg) {
			error(ctx->rt, tok->file, tok->line, ERR_EMPTY_CHAR_CONSTANT);
			return tok->kind = TOKEN_any;
		}
		if (ptr > beg + vm_stk_align + 1) {
			warn(ctx->rt, tok->file, tok->line, WARN_CHR_CONST_OVERFLOW, ptr);
		}
		else if (ptr > beg + ctx->cc->type_chr->size + 1) {
			warn(ctx->rt, tok->file, tok->line, WARN_MULTI_CHAR_CONSTANT);
		}

		for (const char *p = beg; *p; ++p) {
			val = val << 8 | *p;
		}

		tok->type = ctx->cc->type_chr;
		tok->cInt = val;
		return tok->kind = TOKEN_val;
	}

	tok->type = ctx->cc->type_str;
	tok->id.hash = rehash(beg, ptr - beg);
	tok->id.name = ccUniqueStr(ctx->cc, beg, ptr - beg, tok->id.hash);
	return tok->kind = TOKEN_val;
}

static ccToken readName(lexContext ctx, astn tok, int chr) {
	const char *end = (char *) ctx->rt->_end;
	const char *beg = (char *) ctx->rt->_beg;
	char *ptr = (char *) ctx->rt->_beg;

	while (chr != -1) {
		dieif(ptr >= end, ERR_MEMORY_OVERRUN);
		if (!(chr_map[chr & 0xff] & CWORD)) {
			backChar(ctx, chr);
			break;
		}
		*ptr++ = (char) chr;
		chr = readChar(ctx);
	}
	*ptr++ = 0;

	for (size_t i = 0; i < lengthOf(keywords); i += 1) {
		if (strcmp(beg, keywords[i].name) == 0) {
			return tok->kind = keywords[i].type;
		}
	}

	tok->kind = TOKEN_var;
	tok->type = tok->id.link = NULL;
	tok->id.hash = rehash(beg, ptr - beg);
	tok->id.name = ccUniqueStr(ctx->cc, beg, ptr - beg, tok->id.hash);
	return TOKEN_var;
}

/**
 * Reads the next token from the input stream.
 *
 * @param ctx Lexer context.
 * @param tok The token structure to be updated with the parsed token.
 * @return The type of token that was identified and processed.
 */
static ccToken readToken(lexContext ctx, astn tok) {
	const char *end = (char *) ctx->rt->_end;
	char *beg = (char *) ctx->rt->_beg;
	int chr = readChar(ctx);

	// skip white spaces and comments
	while (chr != -1) {
		if (chr == '/') {
			const int line = ctx->cc->line;      // comment begin line
			const int next = peekChar(ctx);       // comment begin char
			char *doc = NULL;
			char *ptr = beg;
			int newLine = INT_MAX;

			// line comment
			if (next == '/') {
				chr = readChar(ctx);
				chr = readChar(ctx);
				if (chr == '/') {
					// '/// ...'
					doc = ptr;
				}

				while (ptr < end && chr != -1) {
					if (chr == '\n') {
						break;
					}
					chr = readChar(ctx);
					if (newLine && !(chr_map[chr & 0xff] & SPACE)) {
						newLine = 0;
					}
					if (!newLine) {
						*ptr++ = (char) chr;
					}
				}
				if (chr == -1) {
					warn(ctx->rt, ctx->cc->file, line, WARN_NO_NEW_LINE_AT_END);
				}
				else if (ctx->cc->file != NULL && ctx->cc->line > 0 && ctx->cc->line != line + 1) {
					warn(ctx->rt, ctx->cc->file, line, WARN_COMMENT_MULTI_LINE, ptr);
				}
			}

			// block comment
			else if (next == '*') {
				chr = readChar(ctx);
				chr = readChar(ctx);
				if (chr == '*') {
					// '/** ...'
					if (peekChar(ctx) != '/') {
						doc = ptr;
					}
				}

				int level = 1;
				while (chr != -1) {
					dieif(ptr >= end, ERR_MEMORY_OVERRUN);
					int prev_chr = chr;
					chr = readChar(ctx);

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
							warn(ctx->rt, ctx->cc->file, ctx->cc->line, WARN_COMMENT_NESTED);
							level = 1;
						}
						chr = 0; // disable reading as valid comment: /*/
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
					error(ctx->rt, ctx->cc->file, line, ERR_COMMENT_INVALID);
				}
			}

			if (chr == -1 && line == ctx->cc->line) {
				warn(ctx->rt, ctx->cc->file, ctx->cc->line, WARN_NO_NEW_LINE_AT_END);
			}
			if (doc != NULL) {
				if (!ctx->cc->genDocumentation) {
					doc = (char *) type_doc_builtin;
					ptr = doc + strlen(doc) + 1;
				} else {
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
				tok->id.hash = rehash(doc, ptr - doc);
				tok->id.name = ccUniqueStr(ctx->cc, doc, ptr - doc, tok->id.hash);
				return TOKEN_doc;
			}
		}

		if (chr_map[chr & 0xff] == CNTRL) {
			warn(ctx->rt, ctx->cc->file, ctx->cc->line, ERR_INVALID_CHARACTER, chr);
		}

		if (!(chr_map[chr & 0xff] & SPACE)) {
			break;
		}

		chr = readChar(ctx);
	}

	if (chr < 0) {
		return TOKEN_any;
	}

	// our token begins here
	memset(tok, 0, sizeof(*tok));
	tok->file = ctx->cc->file;
	tok->line = ctx->cc->line;

	// scan
	switch (chr) {
		case '.':
			if (skipChar(ctx, '.')) {
				if (skipChar(ctx, '.')) {
					return tok->kind = PNCT_dot3;
				}
				return tok->kind = TOKEN_any;// dot2: ..
			}

			if (chr_map[peekChar(ctx) & 0xff] == DIGIT) {
				return readNumber(ctx, tok, chr);
			}
			return tok->kind = OPER_dot;

		case ';':
			return tok->kind = STMT_end;

		case ',':
			return tok->kind = OPER_com;

		case '{':
			return tok->kind = STMT_beg;

		case '[':
			return tok->kind = OPER_idx;

		case '(':
			return tok->kind = OPER_fnc;

		case '}':
			return tok->kind = RIGHT_crl;

		case ']':
			return tok->kind = RIGHT_sqr;

		case ')':
			return tok->kind = RIGHT_par;

		case '?':
			return tok->kind = OPER_sel;

		case ':':
			return tok->kind = OPER_cln;

		case '~':
			return tok->kind = OPER_cmt;

		case '=':
			if (skipChar(ctx, '=')) {
				return tok->kind = OPER_ceq;
			}
			return tok->kind = ASGN_set;

		case '!':
			if (skipChar(ctx, '=')) {
				return tok->kind = OPER_cne;
			}
			return tok->kind = OPER_not;

		case '>':
			if (skipChar(ctx, '>')) {
				if (skipChar(ctx, '=')) {
					return tok->kind = ASGN_shr;
				}
				return tok->kind = OPER_shr;
			}
			if (skipChar(ctx, '=')) {
				return tok->kind = OPER_cge;
			}
			return tok->kind = OPER_cgt;

		case '<':
			if (skipChar(ctx, '<')) {
				if (skipChar(ctx, '=')) {
					return tok->kind = ASGN_shl;
				}
				return tok->kind = OPER_shl;
			}
			if (skipChar(ctx, '=')) {
				return tok->kind = OPER_cle;
			}
			return tok->kind = OPER_clt;

		case '&':
			if (skipChar(ctx, '&')) {
				return tok->kind = OPER_all;
			}
			if (skipChar(ctx, '=')) {
				return tok->kind = ASGN_and;
			}
			return tok->kind = OPER_and;

		case '|':
			if (skipChar(ctx, '|')) {
				return tok->kind = OPER_any;
			}
			if (skipChar(ctx, '=')) {
				return tok->kind = ASGN_ior;
			}
			return tok->kind = OPER_ior;

		case '^':
			if (skipChar(ctx, '=')) {
				return tok->kind = ASGN_xor;
			}
			return tok->kind = OPER_xor;

		case '+':
			if (skipChar(ctx, '+')) {
				// disable `++` operator
				error(ctx->rt, tok->file, tok->line, ERR_INVALID_CHARACTER, chr);
				return tok->kind = TOKEN_any;
			}
			if (skipChar(ctx, '=')) {
				return tok->kind = ASGN_add;
			}
			return tok->kind = OPER_add;

		case '-':
			if (skipChar(ctx, '-')) {
				// disable `--` operator
				error(ctx->rt, tok->file, tok->line, ERR_INVALID_CHARACTER, chr);
				return tok->kind = TOKEN_any;
			}
			if (skipChar(ctx, '=')) {
				return tok->kind = ASGN_sub;
			}
			return tok->kind = OPER_sub;

		case '*':
			if (skipChar(ctx, '=')) {
				return tok->kind = ASGN_mul;
			}
			return tok->kind = OPER_mul;

		case '/':
			if (skipChar(ctx, '=')) {
				return tok->kind = ASGN_div;
			}
			return tok->kind = OPER_div;

		case '%':
			if (skipChar(ctx, '=')) {
				return tok->kind = ASGN_mod;
			}
			return tok->kind = OPER_mod;

		case '\'': // \'[^\'\n]*
		case '"':  // \"[^\"\n]*
			return readText(ctx, tok, chr);

		default:
			if (chr_map[chr & 0xff] & DIGIT) {
				return readNumber(ctx, tok, chr);
			}
			if (chr_map[chr & 0xff] & CWORD) {
				return readName(ctx, tok, chr);
			}
			break;
	}

	error(ctx->rt, tok->file, tok->line, ERR_INVALID_CHARACTER, chr);
	return tok->kind = TOKEN_any;
}

astn nextToken(ccContext ctx, ccToken match, int raise) {
	astn token = ctx->nextToken;
	if (token == NULL) {
		if (raise) {
			error(ctx->rt, NULL, 0, ERR_UNEXPECTED_END_OF_FILE);
		}
		return NULL;
	}

	if (match != 0 && match != token->kind) {
		if (raise) {
			error(ctx->rt, token->file, token->line, ERR_UNMATCHED_TOKEN, token, match);
		}
		return NULL;
	}

	ctx->nextToken = token->next;
	token->next = NULL;
	return token;
}

astn backToken(ccContext ctx, astn token) {
	if (token == NULL) {
		return NULL;
	}

	token->next = ctx->nextToken;
	ctx->nextToken = token;
	return token;
}

ccToken skipToken(ccContext ctx, ccToken match, int raise) {
	astn token = nextToken(ctx, match, raise);
	if (token == NULL) {
		return TOKEN_any;
	}

	ccToken kind = token->kind;
	recycle(ctx, token);
	return kind;
}

int ccParse(ccContext ctx, const char *file, int line, char *content) {
	if (file == NULL && content == NULL) {
		astn ast = ctx->nextToken;
		if (ast != NULL) {
			error(ctx->rt, ast->file, ast->line, ERR_UNPROCESSED_TOKEN, ast);
			return -1;
		}
		// just making sure no tokens left in the stream
		return 0;
	}

	struct lexContext input = {0};
	if (content == NULL) {
		input.fid = open(file, O_RDONLY);
		if (input.fid <= 0) {
			error(ctx->rt, file, line, ERR_OPENING_FILE, file);
			return -1;
		}
	} else {
		input.fid = -1;
		input.ptr = content;
		input.cnt = strlen(content);
	}

	if (file != NULL) {
		file = ccUniqueStr(ctx, file, -1, -1);
	}

	input.cc = ctx;
	input.rt = ctx->rt;
	input.filePos = 0;
	input.linePos = 0;
	input.nextChr = -1;

	ctx->file = file;
	ctx->line = line;

	if (fillBuffer(&input) > 2) {
		// skip the first line if it begins with hash-bang: #!
		if (input.ptr[0] == '#' && input.ptr[1] == '!') {
			int chr = readChar(&input);
			while (chr != -1 && chr != '\n') {
				chr = readChar(&input);
			}
		}
	}

	astn head = NULL;
	astn tail = NULL;
	for (;;) {
		astn tok = newAstn(ctx, TOKEN_any);
		if (tok == NULL) {
			break;
		}
		if (!readToken(&input, tok)) {
			if (peekChar(&input) == -1) {
				recycle(ctx, tok);
				break;
			}
		}
		if (tail != NULL) {
			tail->next = tok;
		} else {
			head = tok;
		}
		tail = tok;
	}

	if (input.fid > 0) {
		close(input.fid);
	}
	if (tail == NULL) {
		warn(ctx->rt, ctx->file, ctx->line, WARN_EMPTY_FILE);
		return 0;
	}
	if (content == NULL && tail->line == ctx->line) {
		warn(ctx->rt, ctx->file, ctx->line, WARN_NO_NEW_LINE_AT_END);
	}

	// insert the newly parsed tokens at the beginning of the stream (will be used to inline another file)
	tail->next = ctx->nextToken;
	ctx->nextToken = head;
	return 0;
}

const char *ccUniqueStr(ccContext ctx, const char *str, size_t size, unsigned hash) {
	if (str == NULL) {
		return NULL;
	}

	if (size == (size_t) -1) {
		size = strlen(str) + 1;
	}
	else if (str[size - 1] != 0) {
		fatal(ERR_INTERNAL_ERROR);
		return NULL;
	}

	if (hash == (unsigned) -1) {
		hash = rehash(str, size);
	}
	if (hash >= lengthOf(ctx->stringTable)) {
		hash %= lengthOf(ctx->stringTable);
	}

	list prev = NULL;
	list next = ctx->stringTable[hash];
	while (next != NULL) {
		int cmp = strncmp(next->data, str, size);
		if (cmp == 0) {
			return next->data;
		}
		if (cmp > 0) {
			break;
		}
		prev = next;
		next = next->next;
	}

	rtContext rt = ctx->rt;
	if (rt->_beg >= rt->_end - (sizeof(struct list) + size)) {
		fatal(ERR_MEMORY_OVERRUN);
		return NULL;
	}
	if (str != (char *)rt->_beg) {
		// copy data from constants?
		memcpy(rt->_beg, str, size + 1);
		str = (char*)rt->_beg;
	}

	// allocate list node as temp data (end of memory)
	rt->_end -= sizeof(struct list);
	list node = (list)rt->_end;

	rt->_beg += size;

	if (!prev) {
		ctx->stringTable[hash] = node;
	}
	else {
		prev->next = node;
	}

	node->next = next;
	node->data = str;
	return str;
}

static int const chr_map[256] = {
	/* 000 nul */    CNTRL, // Null character
	/* 001 soh */    CNTRL, // Start of Heading
	/* 002 stx */    CNTRL, // Start of Text
	/* 003 etx */    CNTRL, // End of Text
	/* 004 eot */    CNTRL, // End of Transmission
	/* 005 enq */    CNTRL, // Enquiry
	/* 006 ack */    CNTRL, // Acknowledge
	/* 007 bel */    CNTRL, // Bell, Alert
	/* 010 bs  */    CNTRL, // Backspace
	/* 011 ht  */    SCTRL, // Horizontal Tab
	/* 012 nl  */    SCTRL, // Line Feed
	/* 013 vt  */    SCTRL, // Vertical Tabulation
	/* 014 ff  */    SCTRL, // Form Feed
	/* 015 cr  */    SCTRL, // Carriage Return
	/* 016 so  */    CNTRL, // Shift Out
	/* 017 si  */    CNTRL, // Shift In
	/* 020 dle */    CNTRL, // Data Link Escape
	/* 021 dc1 */    CNTRL, // Device Control One (XON)
	/* 022 dc2 */    CNTRL, // Device Control Two
	/* 023 dc3 */    CNTRL, // Device Control Three (XOFF)
	/* 024 dc4 */    CNTRL, // Device Control Four
	/* 025 nak */    CNTRL, // Negative Acknowledge
	/* 026 syn */    CNTRL, // Synchronous Idle
	/* 027 etb */    CNTRL, // End of Transmission Block
	/* 030 can */    CNTRL, // Cancel
	/* 031 em  */    CNTRL, // End of Medium
	/* 032 sub */    CNTRL, // Substitute
	/* 033 esc */    CNTRL, // Escape
	/* 034 fs  */    CNTRL, // File Separator
	/* 035 gs  */    CNTRL, // Group Separator
	/* 036 rs  */    CNTRL, // Record Separator
	/* 037 us  */    CNTRL, // Unit Separator
	/* 040 sp  */    SPACE, // Space
	/* 041 !   */    PUNCT,
	/* 042 "   */    PUNCT,
	/* 043 #   */    PUNCT,
	/* 044 $   */    PUNCT,
	/* 045 %   */    PUNCT,
	/* 046 &   */    PUNCT,
	/* 047 '   */    PUNCT,
	/* 050 (   */    PUNCT,
	/* 051 )   */    PUNCT,
	/* 052 *   */    PUNCT,
	/* 053 +   */    PUNCT,
	/* 054 ,   */    PUNCT,
	/* 055 -   */    PUNCT,
	/* 056 .   */    PUNCT,
	/* 057 /   */    PUNCT,
	/* 060 0   */    DIGIT,
	/* 061 1   */    DIGIT,
	/* 062 2   */    DIGIT,
	/* 063 3   */    DIGIT,
	/* 064 4   */    DIGIT,
	/* 065 5   */    DIGIT,
	/* 066 6   */    DIGIT,
	/* 067 7   */    DIGIT,
	/* 070 8   */    DIGIT,
	/* 071 9   */    DIGIT,
	/* 072 :   */    PUNCT,
	/* 073 ;   */    PUNCT,
	/* 074 <   */    PUNCT,
	/* 075 =   */    PUNCT,
	/* 076 >   */    PUNCT,
	/* 077 ?   */    PUNCT,
	/* 100 @   */    PUNCT,
	/* 101 A   */    UPPER,
	/* 102 B   */    UPPER,
	/* 103 C   */    UPPER,
	/* 104 D   */    UPPER,
	/* 105 E   */    UPPER,
	/* 106 F   */    UPPER,
	/* 107 G   */    UPPER,
	/* 110 H   */    UPPER,
	/* 111 I   */    UPPER,
	/* 112 J   */    UPPER,
	/* 113 K   */    UPPER,
	/* 114 L   */    UPPER,
	/* 115 M   */    UPPER,
	/* 116 N   */    UPPER,
	/* 117 O   */    UPPER,
	/* 120 P   */    UPPER,
	/* 121 Q   */    UPPER,
	/* 122 R   */    UPPER,
	/* 123 S   */    UPPER,
	/* 124 T   */    UPPER,
	/* 125 U   */    UPPER,
	/* 126 V   */    UPPER,
	/* 127 W   */    UPPER,
	/* 130 X   */    UPPER,
	/* 131 Y   */    UPPER,
	/* 132 Z   */    UPPER,
	/* 133 [   */    PUNCT,
	/* 134 \   */    PUNCT,
	/* 135 ]   */    PUNCT,
	/* 136 ^   */    PUNCT,
	/* 137 _   */    UNDER,
	/* 140 `   */    PUNCT,
	/* 141 a   */    LOWER,
	/* 142 b   */    LOWER,
	/* 143 c   */    LOWER,
	/* 144 d   */    LOWER,
	/* 145 e   */    LOWER,
	/* 146 f   */    LOWER,
	/* 147 g   */    LOWER,
	/* 150 h   */    LOWER,
	/* 151 i   */    LOWER,
	/* 152 j   */    LOWER,
	/* 153 k   */    LOWER,
	/* 154 l   */    LOWER,
	/* 155 m   */    LOWER,
	/* 156 n   */    LOWER,
	/* 157 o   */    LOWER,
	/* 160 p   */    LOWER,
	/* 161 q   */    LOWER,
	/* 162 r   */    LOWER,
	/* 163 s   */    LOWER,
	/* 164 t   */    LOWER,
	/* 165 u   */    LOWER,
	/* 166 v   */    LOWER,
	/* 167 w   */    LOWER,
	/* 170 x   */    LOWER,
	/* 171 y   */    LOWER,
	/* 172 z   */    LOWER,
	/* 173 {   */    PUNCT,
	/* 174 |   */    PUNCT,
	/* 175 }   */    PUNCT,
	/* 176 ~   */    PUNCT,
	/* 177  */    CNTRL, // Delete
	/* 200     */    OTHER,
};
