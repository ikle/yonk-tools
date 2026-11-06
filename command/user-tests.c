/*
 * Check whether the user is a member of the specified group
 *
 * Copyright (c) 2006-2026 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdlib.h>

#include <sys/types.h>
#include <grp.h>
#include <unistd.h>

#include "user-tests.h"

int user_is_member (const char *group)
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
