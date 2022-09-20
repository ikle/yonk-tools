/*
 * Run command or start daemon
 *
 * Copyright (c) 2020-2022 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/wait.h>
#include <unistd.h>

#include "spawn.h"

int spawn_ve (const char *pidfile, char *const argv[], char *const envp[])
{
	pid_t pid;
	FILE *f;
	int status;

	if ((pid = fork ()) == -1)
		return pid;

	if (pid == 0) {
		if (pidfile != NULL) {
			if (daemon (0, 0) != 0)
				_exit (2);

			if ((f = fopen (pidfile, "w")) != NULL) {
				fprintf (f, "%ld\n", (long) getpid ());
				fclose (f);
			}
		}

		(void) execve (argv[0], argv, envp);
		_exit (2);
	}

	while (waitpid (pid, &status, 0) != pid)
		if (errno != EINTR)
			return -1;

	if (WIFEXITED (status))
		return WEXITSTATUS (status);

	errno = EINTR;
	return -1;
}
