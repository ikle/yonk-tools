/*
 * Yonk Action
 *
 * Copyright (c) 2017-2026 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef YONK_ACTION_H
#define YONK_ACTION_H  1

#include <stdio.h>
#include <stdlib.h>

struct yonk_action {
	char *path, *help, *cat, *exec;
};

static inline void yonk_action_init (struct yonk_action *o)
{
	o->path = NULL;
	o->help = NULL;
	o->cat  = NULL;
	o->exec = NULL;
}

static inline void yonk_action_fini (struct yonk_action *o)
{
	free (o->path);
	free (o->help);
	free (o->cat);
	free (o->exec);
}

int yonk_action_pull   (struct yonk_action *o, FILE *in);
int yonk_action_load   (struct yonk_action *o, const char *path);
int yonk_action_save   (struct yonk_action *o, const char *path);

int yonk_action_access (struct yonk_action *o);

#endif  /* YONK_ACTION_H */
