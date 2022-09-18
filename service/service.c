/*
 * Service Management helpers
 *
 * Copyright (c) 2016-2022 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <signal.h>
#include <syslog.h>

#include "service.h"

void service_init (struct service *o)
{
	o->bundle = getenv ("BUNDLE");

	if ((o->name = getenv ("NAME")) == NULL)
		errx (1, "E: service name required");

	if ((o->desc = getenv ("DESC")) == NULL)
		errx (1, "E: service description required");

	if ((o->daemon = getenv ("DAEMON")) == NULL) {
		static char buf[128];

		if (o->bundle != NULL)
			snprintf (buf, sizeof (buf), "/usr/lib/%s/%s",
				  o->bundle, o->name);
		else
			snprintf (buf, sizeof (buf), "/usr/sbin/%s", o->name);

		o->daemon = buf;
	}

	if ((o->pidfile = getenv ("PIDFILE")) == NULL) {
		static char buf[128];

		if (o->bundle != NULL)
			snprintf (buf, sizeof (buf), "/var/run/%s/%s.pid",
				  o->bundle, o->name);
		else
			snprintf (buf, sizeof (buf), "/var/run/%s.pid", o->name);

		o->pidfile = buf;
	}

	o->conf = getenv ("CONF");
	o->daemonize = 0;

	openlog (o->name, 0, LOG_DAEMON);
}

static pid_t service_pid (struct service *o)
{
	FILE *f;
	long pid;

	if ((f = fopen (o->pidfile, "r")) == NULL)
		return -1;

	if (fscanf (f, "%ld", &pid) != 1)
		pid = -1;

	fclose (f);
	return pid;
}

int service_is_running (struct service *o)
{
	pid_t pid;
	char path[32];  /* strlen ("/proc/" + (2^64 - 1)) = 24 */

	if ((pid = service_pid (o)) == -1)
		return 0;

	if (kill (pid, 0) == 0)
		return 1;

	if (errno == ESRCH)
		return 0;

	snprintf (path, sizeof (path), "/proc/%lu", (unsigned long) pid);

	return access (path, F_OK) == 0;
}

int service_reload (struct service *o)
{
	pid_t pid;

	if ((pid = service_pid (o)) == -1)
		return 0;

	return kill (pid, SIGHUP) == 0;
}

#define START_FMT  "start-stop-daemon -q -S -p %s -x %s %s"

int service_start (struct service *o)
{
	char *args = getenv ("ARGS");
	const char *fmt = (args == NULL) ? START_FMT : START_FMT " -- %s";
	const char *bg = o->daemonize ? "-m -b" : "";
	int len;
	char *cmd;
	int ok;

	if (service_is_running (o))
		return -1;

	len = snprintf (NULL, 0, fmt, o->pidfile, o->daemon, bg, args) + 1;

	if ((cmd = malloc (len)) == NULL)
		err (1, "E");

	snprintf (cmd, len, fmt, o->pidfile, o->daemon, bg, args);
	ok = system (cmd) == 0;
	free (cmd);
  
	return ok;
}

int service_stop (struct service *o, int verbose)
{
	pid_t pid;
	int timeout;

	if ((pid = service_pid (o)) == -1)
		return 0;

	if (kill (pid, SIGTERM) != 0)
		return 0;

	for (timeout = 5; service_is_running (o) && timeout > 0; --timeout) {
		if (verbose)
			fputc ('.', stderr);

		sleep (1);
	}

	if (timeout == 0 && kill (pid, SIGKILL) != 0)
		return 0;

	(void) unlink (o->pidfile);
	return 1;
}
