/*
 * Simple String Buffer
 *
 * Copyright (c) 2004-2026 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef STRBUF_H
#define STRBUF_H  1

#include <stdlib.h>
#include <string.h>

struct strbuf {
	size_t len;
	char *data;
};

static inline void strbuf_open (struct strbuf *o)
{
	o->len  = 0;
	o->data = NULL;
}

static inline char *strbuf_close (struct strbuf *o)
{
	char *data = o->data;

	strbuf_open (o);
	return data;
}

static inline int strbuf_add (struct strbuf *o, const char *s)
{
	size_t len = o->len + strlen (s);
	char *data = realloc (o->data, len + 1);

	if (data == NULL)
		return 0;

	strcpy (data + o->len, s);

	o->len  = len;
	o->data = data;
	return 1;
}

#endif  /* STRBUF_H */
