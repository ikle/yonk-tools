/*
 * E-Mail-Style Envelope Lexer
 *
 * Copyright (c) 2004-2026 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <string.h>

#include "envelope-lexer.h"

int envelope_lexer (const char *path, envelope_cb cb, void *cookie)
{
	FILE *in;
	char line[256], *s, *data;
	int cont;

	if ((in = fopen (path, "r")) == NULL)
		return 0;

	for (cont = 1; cont && fgets (line, sizeof (line), in) != NULL; )
		if (line[0] == 040 || line[0] == 011)
			cont = cb (ENVELOPE_CONT, line + 1, cookie);
		else
		if (line[0] == 043)
			cont = cb (ENVELOPE_COMMENT, line, cookie);
		else
		if ((s = strchr (line, 072)) != NULL) {
			*s = '\0';
			data = s[1] == 040 || s[1] == 011 ? s + 2: s + 1;
			cont = cb (ENVELOPE_NAME, line, cookie) &&
			       cb (ENVELOPE_DATA, data, cookie);
		}
		else
		if (line[0] == 012 || (line[0] == 015 && line[1] == 012))
			cont = cb (ENVELOPE_EMPTY, line, cookie);
		else
			cont = 0;

	cont &= cb (ENVELOPE_END, NULL, cookie);

	fclose (in);
	return cont;
}
