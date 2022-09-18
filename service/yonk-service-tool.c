/*
 * Yonk Service: service start-stop helper
 *
 * Copyright (c) 2016-2022 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#include <unistd.h>
#include <signal.h>
#include <syslog.h>

#include "term-status.h"

struct service {
	const char *bundle, *name, *desc, *daemon, *pidfile, *conf;
	int daemonize;
};

static
void print_status (const char *verb, const char *desc, int ok, int silent)
{
	syslog (ok > 0 ? LOG_NOTICE : ok < 0 ? LOG_INFO : LOG_ERR,
		"%s %s: %s", verb, desc,
		ok > 0 ? "ok" : ok < 0 ? "skipped" : "failed");

	if (!silent)
		term_show_status (stderr, verb, desc, ok);
}

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

static int do_service_status (struct service *o, int silent)
{
	int ok = service_is_running (o);

	if (!silent)
		fprintf (stderr, "Service %s is %srunning\n", o->desc,
			 ok ? "" : "not ");

	return ok ? 0 : 1;
}

static int do_service_reload (struct service *o, int silent)
{
	int ok = service_reload (o);

	print_status ("Reload", o->desc, ok, silent);
	return ok ? 0 : 1;
}

static int do_service_usage (struct service *o)
{
	fprintf (stderr, "usage:\n\t/etc/init.d/%s "
			 "(start|stop|status|reload|restart)\n", o->name);
	return 0;
}

static int do_service_start (struct service *o, int silent, int restart)
{
	int ok;

	if (access (o->daemon, X_OK) != 0)
		return 0;

	if (o->conf != NULL && access (o->conf, R_OK) != 0) {
		print_status ("Start", o->desc, -1, silent);
		return 0;
	}

	if (!silent)
		fprintf (stderr, "\rStarting %s...", o->desc);

	if ((ok = service_start (o)) < 0) {
		if (!silent)
			fprintf (stderr, "\r%s already running\n", o->desc);

		return 0;
	}

	print_status (restart ? "Restart" : "Start", o->desc, ok, silent);
	return ok > 0 ? 0 : 1;
}

static int do_service_stop (struct service *o, int silent, int restart)
{
	int ok;

	if (access (o->daemon, X_OK) != 0)
		return 0;

	if (!silent)
		fprintf (stderr, "\rStopping %s..", o->desc);

	ok = service_stop (o, !silent);

	print_status ("Stop", o->desc, ok, silent | restart);
	return ok ? 0 : 1;
}

int main (int argc, char *argv[])
{
	struct service o;

	int silent = !isatty (fileno (stderr));

	service_init (&o);
	term_init (stderr);

	if (argc > 1 && strcmp (argv[1], "-d") == 0)
		o.daemonize = 1, --argc, ++argv;

	switch (argc) {
	case 2:
		if (strcmp (argv[1], "status") == 0)
			return do_service_status (&o, silent);

		if (strcmp (argv[1], "reload") == 0)
			return do_service_reload (&o, silent);

		if (strcmp (argv[1], "usage") == 0)
			return do_service_usage (&o);

		if (strcmp (argv[1], "start") == 0)
			return do_service_start (&o, silent, 0);

		if (strcmp (argv[1], "stop") == 0)
			return do_service_stop (&o, silent, 0);

		if (strcmp (argv[1], "restart") == 0) {
			(void) do_service_stop  (&o, silent, 1);
			return do_service_start (&o, silent, 1);
		}
	}

	fprintf (stderr, "usage:\n"
			 "\tyonk-service (reload|status|usage)\n"
			 "\tyonk-service [-d] (start|stop|restart)\n");
	return 1;
}
