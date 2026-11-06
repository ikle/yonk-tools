/*
 * E-Mail-Style Envelope Lexer
 *
 * Copyright (c) 2004-2026 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef ENVELOPE_LEXER_H
#define ENVELOPE_LEXER_H  1

enum envelope_type {
	ENVELOPE_NAME,		/* field name		*/
	ENVELOPE_DATA,		/* field data line	*/
	ENVELOPE_CONT,		/* field next data line	*/
	ENVELOPE_EMPTY,		/* empty line		*/
	ENVELOPE_COMMENT,	/* hash-comment line	*/
	ENVELOPE_END,		/* stop parsing		*/
};

typedef int (*envelope_cb) (int type, const char *line, void *cookie);

int envelope_lexer (const char *path, envelope_cb cb, void *cookie);

#endif  /* ENVELOPE_LEXER_H */
