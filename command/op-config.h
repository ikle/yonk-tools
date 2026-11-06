/*
 * Command Configuration helpers
 *
 * Copyright (c) 2017-2026 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef OP_CONFIG_H
#define OP_CONFIG_H  1

#include <stdlib.h>

struct op_config {
	char *help, *index, *cat, *run;
};

static inline void op_config_init (struct op_config *o)
{
	o->help  = NULL;
	o->index = NULL;
	o->cat   = NULL;
	o->run   = NULL;
}

static inline void op_config_fini (struct op_config *o)
{
	free (o->help);
	free (o->index);
	free (o->cat);
	free (o->run);
}

int op_config_load (struct op_config *o, const char *path);

#endif  /* OP_CONFIG_H */
