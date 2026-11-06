/*
 * Yonk Action Configuration Compiler
 *
 * Copyright (c) 2017-2026 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <unistd.h>

#include "action.h"

static inline int error (const char *reason, FILE *in)
{
	long count, line = 0;

	if (in != NULL && (count = ftell (in)) > 0)
		for (rewind (in); count > 0; --count)
			if (fgetc (in) == '\n')
				++line;

	if (line > 0)
		fprintf (stderr, "E: %s (near line %ld)\n", reason, line);
	else
		fprintf (stderr, "E: %s\n", reason);

	return 0;
}

static int match_name (const char *s)
{
	const char *e = s;

	if (*s == 'T' || *s == 'V')
		return *++e == ' ' || *e == '\n' || *e == '\t' || *e == '\0';

	if (*e < 'a' || *e > 'z')
		return 0;

match:	++e;

	if (*e == ' ' || *e == '\n' || *e == '\t' || *e == '\0')
		return e - s > 32 ? 0 : e - s;

	if (*e == '-')
		++e;

	if ((*e >= '0' && *e <= '9') || (*e >= 'a' && *e <= 'z'))
		goto match;

	return 0;
}

static size_t path_build (char *buf, size_t size, const char *path)
{
	int len, n;
	size_t total = 0;

	for (; *path != '\0'; path += strspn (path, " \n\t")) {
		if ((len = match_name (path)) > 0)
			n = snprintf (buf, size, "/%.*s", len, path);
		else
			return error ("Broken path component", NULL);

		path  += len;
		total += n;
		buf   += n;
		size  -= size > n ? n : size;
	}

	return total > 0 ? total : error ("Empty path", NULL);
}

#define CONFIG  "/.config"

static int action_build (struct yonk_action *o, const char *root)
{
	const size_t base = root == NULL ? 0 : strlen (root);
	size_t len;
	char *path, *end, last;
	int ok;

	if ((len = path_build (NULL, 0, o->path)) == 0)
		return error ("Cannot build action node path", NULL);

	if (root == NULL)
		return 1;

	if ((path = malloc (base + len + sizeof (CONFIG))) == NULL)
		return error ("No enough memory", NULL);

	strcpy (path, root);
	path_build (path + base, len + 1, o->path);
	end = path + base + len;
	last = end[-1];

	if (last == 'T' || last == 'V') {
		struct yonk_action parent = { NULL, o->help, o->cat, NULL    };
		struct yonk_action leaf   = { NULL, NULL,    NULL,   o->exec };

		end[-2] = '\0';
		(void) mkdir (path, 0777);
		sprintf (end - 2, CONFIG);
		ok = yonk_action_save (&parent, path) || (rmdir (path), 0);

		sprintf (end - 2, "/%c", last);
		(void) mkdir (path, 0777);
		sprintf (end, CONFIG);
		ok &= yonk_action_save (&leaf, path) || (rmdir (path), 0);
	}
	else {
		(void) mkdir (path, 0777);
		sprintf (end, CONFIG);
		ok = yonk_action_save (o, path) || (rmdir (path), 0);
	}

	free (path);
	return ok || error ("Cannot create action node file", NULL);
}

int main (int argc, char *argv[])
{
	FILE *in;
	struct yonk_action o;

	if (argc < 2 || argc > 3) {
		fprintf (stderr, "usage:\n"
				 "\tyonk-action-compile CONF [ROOT]\n");
		return 1;
	}

	if ((in = fopen (argv[1], "r")) == NULL) {
		perror ("E: Cannot open configuration");
		return 1;
	}

	while (!feof (in)) {
		if (!yonk_action_pull (&o, in)) {
			error ("Cannot parse action config", in);
			goto fail;
		}

		if (o.path == NULL) {
			error ("Path does not specified", in);
			goto fail;
		}

		if (o.help == NULL) {
			error ("Help does not specified", in);
			goto fail;
		}

		if (!action_build (&o, argv[2])) {
			error ("Cannot build action node", in);
			goto fail;
		}
	}

	fclose (in);
	return 0;
fail:
	fclose (in);
	return 1;
}
