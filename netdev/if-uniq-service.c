/*
 * Utility to make unique set of interface names
 *
 * This utility can be used to pre-rename a group of interfaces to avoid
 * renaming conflicts when using udev rules.
 *
 * Copyright (c) 2023 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/param.h>

#include <unistd.h>

static const char *filter = "eth";
static size_t flen = 3;
static const char *prefix = "eth-";
static int max = 256;

static int parse_args (int argc, char *argv[])
{
	if (argc >= 3 && strcmp (argv[1], "-f") == 0) {
		filter = argv[2];
		flen = MIN (strlen (filter), IFNAMSIZ - 1);
		argv += 2, argc -= 2;
	}

	if (argc >= 3 && strcmp (argv[1], "-p") == 0) {
		prefix = argv[2];
		argv += 2, argc -= 2;
	}

	if (argc >= 3 && strcmp (argv[1], "-m") == 0) {
		max = atoi (argv[2]);
		argv += 2, argc -= 2;
	}

	if (argc <= 1)
		return 1;

	fprintf (stderr, "usage:\n"
		 "\tif-uniq [-f filter-prefix] [-p new-prefix] [-m max-if-index]\n");
	return 0;
}

int main (int argc, char *argv[])
{
	int s, i;
	struct ifreq r;

	if (!parse_args (argc, argv))
		return 1;

	if ((s = socket (AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror ("E: Cannot open inet socket");
		return 1;
	}

	for (i = 1; i <= max; ++i) {
		r.ifr_ifindex = i;

		if (ioctl (s, SIOCGIFNAME, &r) != 0)
			continue;

		if (strncmp (r.ifr_name, filter, flen) != 0)
			continue;

		snprintf (r.ifr_newname, sizeof (r.ifr_newname),
			  "%s%d", prefix, i);

		if (ioctl (s, SIOCSIFNAME, &r) != 0)
			fprintf (stderr, "W: Cannot rename netdev %d: %s\n",
				 i, strerror (errno));
	}

	close (s);
	return 0;
}
