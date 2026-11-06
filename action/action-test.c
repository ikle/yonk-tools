/*
 * Yonk Action Test
 *
 * Copyright (c) 2017-2026 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <string.h>

#include "action.h"

int main (int argc, char *argv[])
{
	struct yonk_action c;

	if (argc != 2) {
		fprintf (stderr, "usage:\n\taction-test <comfig>\n");
		return 1;
	}

	if (!yonk_action_load (&c, argv[1])) {
		fprintf (stderr, "E: Cannot load config\n");
		return 1;
	}

	if (c.path != NULL)  printf ("path = %s", c.help);
	if (c.help != NULL)  printf ("help = %s", c.help);
	if (c.cat  != NULL)  printf ("cat  = %s", c.cat );
	if (c.exec != NULL)  printf ("exec = %s", c.exec);

	if (yonk_action_access (&c))
		fprintf (stderr, "N: User in category\n");

	yonk_action_fini (&c);
	return 0;
}
