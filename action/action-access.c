/*
 * Yonk Action Access
 *
 * Copyright (c) 2006-2026 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <grp.h>
#include <unistd.h>

#include "action.h"

/*
 * Check whether the user is a member of the specified group
 */
static int user_is_member (const char *group)
{
	struct group *g;
	int count, i;
	gid_t *list;

	if ((g = getgrnam (group)) == NULL)
		return 0;

	if (getgid () == g->gr_gid)
		return 1;

	if ((count = getgroups (0, NULL)) < 0 ||
	    (list = malloc (sizeof (list[0]) * count)) == NULL)
		return 0;

	for (count = getgroups (count, list), i = 0; i < count; ++i)
		if (list[i] == g->gr_gid) {
			free (list);
			return 1;
		}

	free (list);
	return 0;
}

int yonk_action_access (struct yonk_action *o)
{
	const char *s;
	char name[32];
	size_t len;

	if (o->cat == NULL)
		return 1;

	for (s = o->cat; *s != '\0'; s += strspn (s, "\t\n ")) {
		if ((len = strcspn (s, "\t\n ")) >= sizeof (name))
			return 0;

		memcpy (name, s, len); name[len] = '\0';

		if (user_is_member (name))
			return 1;
	}

	return 0;
}
