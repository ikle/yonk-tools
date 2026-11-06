/*
 * Command Configuration helpers
 *
 * Copyright (c) 2017-2026 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string.h>

#include "envelope-lexer.h"
#include "op-config.h"
#include "strbuf.h"

enum op_config_name { N_NONE, N_HELP, N_ENUM, N_CAT, N_RUN };

struct op_config_ctx {
	struct op_config *conf;
	int name;
	struct strbuf data;
};

#define IF_NAME(conf, field, label)					\
	if (strcmp (line, conf) == 0)					\
		return c->field != NULL ? 0 : (o->name = label), 1;

static int op_config_cb (int type, const char *line, void *cookie)
{
	struct op_config_ctx *o = cookie;
	struct op_config *c = o->conf;
	char *data;

	switch (type) {
	case ENVELOPE_NAME:
	case ENVELOPE_END:
		data = strbuf_close (&o->data);

		switch (o->name) {
		case N_HELP:	c->help  = data; break;
		case N_ENUM:	c->index = data; break;
		case N_CAT:	c->cat   = data; break;
		case N_RUN:	c->run   = data; break;
		}

		o->name = N_NONE;
	}

	switch (type) {
	case ENVELOPE_NAME:
		IF_NAME ("help", help,  N_HELP) else
		IF_NAME ("enum", index, N_ENUM) else
		IF_NAME ("cat",  cat,   N_CAT ) else
		IF_NAME ("run",  run,   N_RUN ) else return 0;

	case ENVELOPE_EMPTY:
		if (o->name == N_NONE)
			return 1;

	case ENVELOPE_DATA:
	case ENVELOPE_CONT:
		return o->name == N_NONE ? 0 : strbuf_add (&o->data, line), 1;
	}

	return 1;  /* ignore comments and unknown events */
}

int op_config_load (struct op_config *o, const char *path)
{
	struct op_config_ctx c = { o, N_NONE, };
	int ok = envelope_lexer (path, op_config_cb, &c);

	if (!ok)
		op_config_fini (o);

	return ok;
}
