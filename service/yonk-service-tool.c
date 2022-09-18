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

#include "service.h"
#include "term-status.h"

static
void print_status (const char *verb, const char *desc, int ok, int silent)
{
	syslog (ok > 0 ? LOG_NOTICE : ok < 0 ? LOG_INFO : LOG_ERR,
		"%s %s: %s", verb, desc,
		ok > 0 ? "ok" : ok < 0 ? "skipped" : "failed");

	if (!silent)
		term_show_status (stderr, verb, desc, ok);
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
