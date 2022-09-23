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
#include <wordexp.h>

extern char **environ;

#include "service.h"
#include "spawn.h"

void service_init (struct service *o, const char *device, int daemonize)
{
	const char *p;

	o->bundle = getenv ("BUNDLE");
	o->device = device;

	if ((o->name = getenv ("NAME")) == NULL)
		errx (1, "E: service name required");

	if ((p = getenv ("DESC")) == NULL)
		errx (1, "E: service description required");

	if (device != NULL)
		snprintf (o->desc, sizeof (o->desc), "%s on %s", p, device);
	else
		snprintf (o->desc, sizeof (o->desc), "%s", p);

	if ((p = getenv ("DAEMON")) != NULL)
		snprintf (o->daemon, sizeof (o->daemon), "%s", p);
	else
	if (o->bundle != NULL)
		snprintf (o->daemon, sizeof (o->daemon), "/usr/lib/%s/%s",
			  o->bundle, o->name);
	else
		snprintf (o->daemon, sizeof (o->daemon), "/usr/sbin/%s",
			  o->name);

	if ((p = getenv ("PIDFILE")) != NULL)
		snprintf (o->pidfile, sizeof (o->pidfile), "%s", p);
	else
	if (o->bundle != NULL)
		snprintf (o->pidfile, sizeof (o->pidfile), "/var/run/%s/%s.pid",
			  o->bundle, o->name);
	else
	if (device != NULL)
		snprintf (o->pidfile, sizeof (o->pidfile), "/var/run/%s/%s.pid",
			  o->name, device);
	else
		snprintf (o->pidfile, sizeof (o->pidfile), "/var/run/%s.pid",
			  o->name);

	o->conf = getenv ("CONF");
	o->daemonize = daemonize;

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

	if ((pid = service_pid (o)) < 0)
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

	if ((pid = service_pid (o)) < 0)
		return 0;

	return kill (pid, SIGHUP) == 0;
}

int service_start (struct service *o)
{
	const char *args;
	const char *pidfile = o->daemonize ? o->pidfile : NULL;
	wordexp_t we;
	int ok;

	if (service_is_running (o))
		return -1;

	if (setenv ("PIDFILE", o->pidfile, 1) != 0 ||
	    (o->device != NULL && setenv ("DEVICE", o->device, 1) != 0))
		return 0;

	if ((args = getenv ("ARGS")) == NULL)
		args = "";

	we.we_offs = 1;

	if (wordexp (args, &we, WRDE_DOOFFS) != 0)
		return 0;

	we.we_wordv[0] = o->daemon;
	ok = spawn_ve (pidfile, we.we_wordv, environ) == 0;

	we.we_wordv[0] = NULL;
	wordfree (&we);
	return ok;
}

int service_stop (struct service *o, int verbose)
{
	pid_t pid;
	int timeout;

	if ((pid = service_pid (o)) < 0)
		return -1;

	if (kill (pid, SIGTERM) != 0)
		return errno == ESRCH ? -1 : 0;

	for (timeout = 500; service_is_running (o) && timeout > 0; --timeout) {
		if (verbose && timeout % 20 == 0)
			fputc ('.', stderr);

		usleep (10000);
	}

	if (timeout == 0 && kill (pid, SIGKILL) != 0 && errno != ESRCH)
		return 0;

	(void) unlink (o->pidfile);
	return 1;
}
