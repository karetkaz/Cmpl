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
 * @param match The token type to be matched.
 * @param raise Flag indicating whether to raise an error or be silent.
 * @return The next token, NULL if the token does not match.
 */
astn nextToken(ccContext ctx, ccToken match, int raise);

/**
 * Pushes a token back to be read next time.
 *
 * @param ctx Compiler context.
 * @param token The token to be pushed back.
 * @return The token that was pushed back.
 */
astn backToken(ccContext ctx, astn token);

/**
 * Skips the current token and returns its kind.
 *
 * @param ctx Compiler context.
 * @param match The token type to be matched.
 * @param raise Flag indicating whether to raise an error or be silent.
 * @return The kind of the token that was skipped or TOKEN_any if no token was found.
 */
ccToken skipToken(ccContext ctx, ccToken match, int raise);

/**
 * Peek the next token.
 *
 * @param ctx Compiler context.
 * @param match Read next token only if matches the given kind.
 * @return Next token, or null.
 */
static inline astn peekToken(ccContext ctx, ccToken match) {
	return backToken(ctx, nextToken(ctx, match, 0));
}

#endif
