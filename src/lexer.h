/**
 * convert text to tokens
 */
#ifndef CMPL_LEXER_H
#define CMPL_LEXER_H

#include "tree.h"

/**
 * Retrieves the next token that matches the specified criteria
 * and optionally raises an error if no matching token is found.
 *
 * @param ctx Compiler context.
 * @param match: read next token only if matches.
 * @return next token, or null.
 */
astn nextToken(ccContext ctx, ccToken match, int raise);

/**
 * Push back a token, to be read next time.
 *
 * @param ctx compiler context.
 * @param token the token to be pushed back.
 */
astn backToken(ccContext ctx, astn token);

/**
 * Read the next token and recycle it.
 *
 * @param ctx compiler context.
 * @param match read next token only if matches.
 * @param raise raise an error if the token was not skipped.
 * @return kind of read token.
 */
ccToken skipToken(ccContext ctx, ccToken match, int raise);

/**
 * Peek the next token.
 *
 * @param ctx compiler context.
 * @param match read next token only if matches the given kind.
 * @return next token, or null.
 */
static inline astn peekToken(ccContext ctx, ccToken match) {
	return backToken(ctx, nextToken(ctx, match, 0));
}

#endif
