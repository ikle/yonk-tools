/*
 * Yonk Action Leaf Saver
 *
 * Copyright (c) 2017-2026 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>

#include "action.h"

#define WRITE(name)  \
	(o->name == NULL || fprintf (out, #name ": %s", o->name) >= 0)

int yonk_action_save (struct yonk_action *o, const char *path)
{
	FILE *out;
	int ok;

	if ((out = fopen (path, "w")) == NULL)
		return 0;

	ok = WRITE (help) & WRITE (cat) & WRITE (exec);

	return (fclose (out) == 0 && ok) || (remove (path), 0);
}
