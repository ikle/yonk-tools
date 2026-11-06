/*
 * Yonk Action Loader
 *
 * Copyright (c) 2017-2026 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string.h>

#include "action.h"
#include "strbuf.h"

static char *pull_data (char *buf, size_t size, char *s, FILE *in)
{
	struct strbuf sb = STRBUF_INIT;
	int c;

	for (; s[0] == ' ' || s[0] == '\t' ; ++s) {}

	do {
		if (!strbuf_add (&sb, s))
			goto no_add;

		switch (c = fgetc (in)) {
		default:	ungetc (c, in); goto stop;
		case '\t':
		case ' ':	s = fgets (buf, size, in);
		}
	}
	while (s != NULL);
stop:
	return sb.data;
no_add:
	free (sb.data);
	return NULL;
}

#define MATCH(pfx, n, field)						\
	strncmp (s, pfx, n) == 0 ?					\
		o->field == NULL &&					\
		(o->field = pull_data (buf, size, s + n, in)) != NULL

int yonk_action_pull (struct yonk_action *o, FILE *in)
{
	char buf[256], *s;
	const size_t size = sizeof (buf);
	int ok = 1;

	yonk_action_init (o);

	while (ok && (s = fgets (buf, size, in)) != NULL && s[0] != '\n')
		ok = MATCH ("path:", 5, path) :
		     MATCH ("help:", 5, help) :
		     MATCH ("cat:",  4, cat ) :
		     MATCH ("exec:", 5, exec) : 0;

	if (!ok)
		yonk_action_fini (o);

	return ok;
}
