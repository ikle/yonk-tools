/*
 * Yonk Action Leaf Loader
 *
 * Copyright (c) 2017-2026 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "action.h"

int yonk_action_load (struct yonk_action *o, const char *path)
{
	FILE *in;
	int ok;

	if ((in = fopen (path, "r")) == NULL)
		return 0;

	ok = yonk_action_pull (o, in) && feof (in);

	fclose (in);
	return ok;
}
