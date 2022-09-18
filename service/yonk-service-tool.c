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

static int daemonize;

static
void print_status (const char *verb, const char *desc, int ok, int silent)
{
	syslog (ok > 0 ? LOG_NOTICE : ok < 0 ? LOG_INFO : LOG_ERR,
		"%s %s: %s", verb, desc,
		ok > 0 ? "ok" : ok < 0 ? "skipped" : "failed");

	if (!silent)
		term_show_status (stderr, verb, desc, ok);
}

static char *bundle, *name, *desc, *daemon_path, *pidfile, *conf;

static void service_init (void)
{
	bundle = getenv ("BUNDLE");

	if ((name = getenv ("NAME")) == NULL)
		errx (1, "E: service name required");

	if ((desc = getenv ("DESC")) == NULL)
		errx (1, "E: service description required");

	if ((daemon_path = getenv ("DAEMON")) == NULL) {
		static char buf[128];

		if (bundle != NULL)
			snprintf (buf, sizeof (buf), "/usr/lib/%s/%s",
				  bundle, name);
		else
			snprintf (buf, sizeof (buf), "/usr/sbin/%s", name);

		daemon_path = buf;
	}

	if ((pidfile = getenv ("PIDFILE")) == NULL) {
		static char buf[128];

		if (bundle != NULL)
			snprintf (buf, sizeof (buf), "/var/run/%s/%s.pid",
				  bundle, name);
		else
			snprintf (buf, sizeof (buf), "/var/run/%s.pid", name);

		pidfile = buf;
	}

	conf = getenv ("CONF");

	openlog (name, 0, LOG_DAEMON);
}

static pid_t service_pid (void)
{
	FILE *f;
	long pid;

	if ((f = fopen (pidfile, "r")) == NULL)
		return -1;

	if (fscanf (f, "%ld", &pid) != 1)
		pid = -1;

	fclose (f);
	return pid;
}

static int service_is_running (void)
{
	pid_t pid;
	char path[32];  /* strlen ("/proc/" + (2^64 - 1)) = 24 */

	if ((pid = service_pid ()) == -1)
		return 0;

	if (kill (pid, 0) == 0)
		return 1;

	if (errno == ESRCH)
		return 0;

	snprintf (path, sizeof (path), "/proc/%lu", (unsigned long) pid);

	return access (path, F_OK) == 0;
}

static int service_reload (void)
{
	pid_t pid;

	if ((pid = service_pid ()) == -1)
		return 0;

	return kill (pid, SIGHUP) == 0;
}

#define START_FMT  "start-stop-daemon -q -S -p %s -x %s %s"

static int service_start (void)
{
	char *args = getenv ("ARGS");
	const char *fmt = (args == NULL) ? START_FMT : START_FMT " -- %s";
	const char *bg = daemonize ? "-m -b" : "";
	int len;
	char *cmd;
	int ok;

	if (service_is_running ())
		return -1;

	len = snprintf (NULL, 0, fmt, pidfile, daemon_path, bg, args) + 1;

	if ((cmd = malloc (len)) == NULL)
		err (1, "E");

	snprintf (cmd, len, fmt, pidfile, daemon_path, bg, args);
	ok = system (cmd) == 0;
	free (cmd);
  
	return ok;
}

static int service_stop (int verbose)
{
	pid_t pid;
	int timeout;

	if ((pid = service_pid ()) == -1)
		return 0;

	if (kill (pid, SIGTERM) != 0)
		return 0;

	for (timeout = 5; service_is_running () && timeout > 0; --timeout) {
		if (verbose)
			fputc ('.', stderr);

		sleep (1);
	}

	if (timeout == 0 && kill (pid, SIGKILL) != 0)
		return 0;

	(void) unlink (pidfile);
	return 1;
}

static int do_service_status (int silent)
{
	int ok = service_is_running ();

	if (!silent)
		fprintf (stderr, "Service %s is %srunning\n", desc,
			 ok ? "" : "not ");

	return ok ? 0 : 1;
}

static int do_service_reload (int silent)
{
	int ok = service_reload ();

	print_status ("Reload", desc, ok, silent);
	return ok ? 0 : 1;
}

static int do_service_usage (void)
{
	fprintf (stderr, "usage:\n\t/etc/init.d/%s "
			 "(start|stop|status|reload|restart)\n", name);
	return 0;
}

static int do_service_start (int silent, int restart)
{
	int ok;

	if (access (daemon_path, X_OK) != 0)
		return 0;

	if (conf != NULL && access (conf, R_OK) != 0) {
		print_status ("Start", desc, -1, silent);
		return 0;
	}

	if (!silent)
		fprintf (stderr, "\rStarting %s...", desc);

	if ((ok = service_start ()) < 0) {
		if (!silent)
			fprintf (stderr, "\r%s already running\n", desc);

		return 0;
	}

	print_status (restart ? "Restart" : "Start", desc, ok, silent);
	return ok > 0 ? 0 : 1;
}

static int do_service_stop (int silent, int restart)
{
	int ok;

	if (access (daemon_path, X_OK) != 0)
		return 0;

	if (!silent)
		fprintf (stderr, "\rStopping %s..", desc);

	ok = service_stop (!silent);

	print_status ("Stop", desc, ok, silent | restart);
	return ok ? 0 : 1;
}

int main (int argc, char *argv[])
{
	int silent = !isatty (fileno (stderr));

	service_init ();
	term_init (stderr);

	if (argc > 1 && strcmp (argv[1], "-d") == 0)
		daemonize = 1, --argc, ++argv;

	switch (argc) {
	case 2:
		if (strcmp (argv[1], "status") == 0)
			return do_service_status (silent);

		if (strcmp (argv[1], "reload") == 0)
			return do_service_reload (silent);

		if (strcmp (argv[1], "usage") == 0)
			return do_service_usage ();

		if (strcmp (argv[1], "start") == 0)
			return do_service_start (silent, 0);

		if (strcmp (argv[1], "stop") == 0)
			return do_service_stop (silent, 0);

		if (strcmp (argv[1], "restart") == 0) {
			(void) do_service_stop  (silent, 1);
			return do_service_start (silent, 1);
		}
	}

	fprintf (stderr, "usage:\n"
			 "\tyonk-service (reload|status|usage)\n"
			 "\tyonk-service [-d] (start|stop|restart)\n");
	return 1;
}
