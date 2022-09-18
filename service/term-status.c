/*
 * Terminal Status Report helper
 *
 * Copyright (c) 2016-2022 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <curses.h>
#include <term.h>

#include "term-status.h"

static char *cuf, *el, *setaf, *op;

void term_init (FILE *to)
{
	char *term = getenv ("TERM");
	int err;

	if (!isatty (fileno (to)))
		return;

	if (term == NULL || term[0] == '\0')
		term = "ansi";

	if (setupterm (term, 1, &err) != OK)
		return;

	cuf   = tigetstr ("cuf");	/* cursor forward	*/
	el    = tigetstr ("el");	/* clear to end of line	*/
	setaf = tigetstr ("setaf");	/* set foreground color	*/
	op    = tigetstr ("op");	/* set default attrs	*/
}

static void term_pos (FILE *to, int pos)
{
	if (cuf == NULL)
		return;

	fprintf (to, "\r");

	if (pos < 0)
		pos += columns;

	fprintf (to, "%s", tiparm (cuf, pos));
}

static void term_setaf (FILE *to, int color)
{
	if (setaf != NULL)
		fprintf (to, "%s", tiparm (setaf, color));
}

static void term_op (FILE *to)
{
	if (op != NULL)
		fprintf (to, "%s", tiparm (op));
}

void term_show_status (FILE *to, const char *verb, const char *desc, int ok)
{
	fprintf (to, "\r%s%s %s", tiparm (el), verb, desc);

	term_pos (to, -8);
	fprintf (to, "[ ");

	if (ok > 0) {
		term_setaf (to, COLOR_GREEN);
		fprintf (to, " ok ");
	}
	else if (ok < 0) {
		term_setaf (to, COLOR_YELLOW);
		fprintf (to, "skip");
	}
	else {
		term_setaf (to, COLOR_RED);
		fprintf (to, "fail");
	}

	term_op (to); fprintf (to, " ]\n");
}
