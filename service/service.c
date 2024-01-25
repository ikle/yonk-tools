/*
 * Service Management helpers
 *
 * Copyright (c) 2016-2024 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <wordexp.h>

extern char **environ;

#include "service.h"
#include "spawn.h"

static int make_home (char *path, mode_t mode)
{
	char *p;
	int ok;

	if ((p = strrchr (path, '/')) == NULL)
		errx (1, "E: pid-file path must be absolute");

	*p = '\0';

	ok = path[0] == '\0' ||
	     (make_home (path, mode) &&
	      (mkdir (path, mode) == 0 || errno == EEXIST));

	*p = '/';
	return ok;
}

#define make_str(s, fmt, ...) \
	snprintf (s, sizeof (s), fmt, __VA_ARGS__)

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
		make_str (o->desc, "%s on %s", p, device);
	else
		make_str (o->desc, "%s", p);

	if ((p = getenv ("DAEMON")) != NULL)
		make_str (o->daemon, "%s", p);
	else
	if (o->bundle != NULL)
		make_str (o->daemon, "/usr/lib/%s/%s", o->bundle, o->name);
	else
		make_str (o->daemon, "/usr/sbin/%s", o->name);

	if ((p = getenv ("PIDFILE")) != NULL)
		make_str (o->pidfile, "%s", p);
	else
	if (o->bundle != NULL)
		make_str (o->pidfile, "/var/run/%s/%s.pid", o->bundle, o->name);
	else
	if (device != NULL)
		make_str (o->pidfile, "/var/run/%s/%s.pid", o->name, device);
	else
		make_str (o->pidfile, "/var/run/%s.pid", o->name);

	o->conf = getenv ("CONF");
	o->daemonize = daemonize;

	openlog (o->name, 0, LOG_DAEMON);
}

static pid_t service_pid (struct service *o)
{
	FILE *f;
	long pid;

	if ((f = fopen (o->pidfile, "r")) == NULL)
		return 0;

	if (fscanf (f, "%ld", &pid) != 1)
		pid = 0;

	fclose (f);
	return pid;
}

int service_is_running (struct service *o)
{
	pid_t pid;
	char path[32];  /* strlen ("/proc/" + (2^64 - 1)) = 24 */

	if ((pid = service_pid (o)) == 0)
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

	if ((pid = service_pid (o)) == 0)
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

	if (!make_home (o->pidfile, 0755))
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

	if ((pid = service_pid (o)) == 0)
		return -1;

	if (kill (pid, SIGTERM) != 0)
		return errno == ESRCH ? -1 : 0;

	for (timeout = 500; kill (pid, 0) == 0 && timeout > 0; --timeout) {
		if (verbose && timeout % 20 == 0)
			fputc ('.', stderr);

		usleep (10000);
	}

	if (timeout == 0 && kill (pid, SIGKILL) != 0 && errno != ESRCH)
		return 0;

	(void) unlink (o->pidfile);
	return 1;
}
