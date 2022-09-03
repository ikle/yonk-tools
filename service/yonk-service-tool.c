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

#include <curses.h>
#include <term.h>

static int daemonize;
static char *cuf, *el, *setaf, *op;

static void term_init (void)
{
	char *term = getenv ("TERM");
	int err;

	if (!isatty (fileno (stderr)))
		return;

	if (term == NULL || term[0] == '\0')
		term = "ansi";

	if (setupterm (term, 1, &err) != OK)
		return;

	cuf   = tigetstr ("cuf");	/* cursor forward	*/
	el    = tigetstr ("el");	/* clear to end of line	*/
	setaf = tigetstr ("setaf");	/* set foreground color	*/
	op    = tigetstr ("op");	/* set default attrs	*/
}

static void term_pos (FILE *to, int pos)
{
	if (cuf == NULL)
		return;

	fprintf (to, "\r");

	if (pos < 0)
		pos += columns;

	fprintf (to, "%s", tiparm (cuf, pos));
}

static void term_setaf (FILE *to, int color)
{
	if (setaf != NULL)
		fprintf (to, "%s", tiparm (setaf, color));
}

static void term_op (FILE *to)
{
	if (op != NULL)
		fprintf (to, "%s", tiparm (op));
}

static
void print_term_status (FILE *to, const char *verb, const char *desc, int ok)
{
	fprintf (to, "\r%s%s %s", tiparm (el), verb, desc);

	term_pos (to, -8);
	fprintf (to, "[ ");

	if (ok > 0) {
		term_setaf (to, COLOR_GREEN);
		fprintf (to, " ok ");
	}
	else if (ok < 0) {
		term_setaf (to, COLOR_YELLOW);
		fprintf (to, "skip");
	}
	else {
		term_setaf (to, COLOR_RED);
		fprintf (to, "fail");
	}

	term_op (to); fprintf (to, " ]\n");
}

static
void print_status (const char *verb, const char *desc, int ok, int silent)
{
	FILE *to = stderr;

	syslog (ok ? LOG_NOTICE : LOG_ERR, "%s %s: %s", verb, desc,
		ok > 0 ? "ok" : ok < 0 ? "skipped" : "failed");

	if (!silent)
		print_term_status (to, verb, desc, ok);
}

static char *name, *desc, *daemon_path, *pidfile, *conf;

#define DAEMON_FMT   "/usr/sbin/%s"
#define PIDFILE_FMT  "/var/run/%s.pid"

static void service_init (void)
{
	int len;

	if ((name = getenv ("NAME")) == NULL)
		errx (1, "E: service name required");

	if ((desc = getenv ("DESC")) == NULL)
		desc = name;

	if ((daemon_path = getenv ("DAEMON")) == NULL) {
		len = snprintf (NULL, 0, DAEMON_FMT, name) + 1;

		if ((daemon_path = malloc (len)) == NULL)
			err (1, "E");

		snprintf (daemon_path, len, DAEMON_FMT, name);
	}

	if ((pidfile = getenv ("PIDFILE")) == NULL) {
		len = snprintf (NULL, 0, PIDFILE_FMT, name) + 1;

		if ((pidfile = malloc (len)) == NULL)
			err (1, "E");

		snprintf (pidfile, len, PIDFILE_FMT, name);
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

	if (fscanf (f, "%ld", &pid) != 1) {
		warnx ("W: broken pidfile: %s", pidfile);
		pid = -1;
	}

	fclose (f);
	return pid;
}

static int service_status (int silent)
{
	int status;
	pid_t pid;
	char path[32];  /* strlen ("/proc/" + (2^64 - 1)) = 24 */

	if ((pid = service_pid ()) == -1)
		status = 1;
	else if (kill (pid, 0) == 0)
		status = 0;
	else if (errno == ESRCH)
		status = 1;
	else {
		snprintf (path, sizeof (path), "/proc/%lu",
			  (unsigned long) pid);
		status = access (path, F_OK) == 0 ? 0 : 1;
	}

	if (!silent)
		fprintf (stderr, "Service %s is %srunning\n", desc,
			 status == 0 ? "" : "not ");

	return status;
}

static int service_reload (int silent)
{
	int status;
	pid_t pid;

	if ((pid = service_pid ()) == -1)
		status = 1;
	else
		status = kill (pid, SIGHUP) == 0 ? 0 : 1;

	print_status ("Reload", desc, status == 0, silent);
	return status;
}

#define START_FMT  "start-stop-daemon -q -S -p %s -x %s %s %s"

static int service_start (const char *opts, int silent, int restart)
{
	char *args = getenv ("ARGS");
	const char *fmt = (args == NULL) ? START_FMT : START_FMT " -- %s";
	const char *bg = daemonize ? "-m -b" : "";
	int len;
	char *cmd;
	int status;

	if (conf != NULL && access (conf, R_OK) != 0) {
		print_status ("Start", desc, -1, silent);
		return 0;
	}

	if (service_status (1) == 0) {
		if (!silent)
			fprintf (stderr, "Service %s already running\n", desc);

		return 0;
	}

	if (!silent)
		fprintf (stderr, "\rStarting %s...", desc);

	if (opts == NULL)
		opts = "";

	len = snprintf (NULL, 0, fmt, pidfile, daemon_path, bg, opts, args) + 1;

	if ((cmd = malloc (len)) == NULL)
		err (1, "E");

	snprintf (cmd, len, fmt, pidfile, daemon_path, bg, opts, args);
	status = system (cmd);
	free (cmd);
  
	print_status (restart ? "Restart" : "Start", desc, status == 0, silent);
	return status;
}

#define STOP_FMT  "start-stop-daemon -q -K -o -p %s %s"

static int service_stop (const char *opts, int silent, int restart)
{
	int len;
	char *cmd;
	int status;

	if (service_status (1) != 0) {
		print_status ("Stop", desc, 1, silent | restart);
		return 0;
	}

	if (!silent)
		fprintf (stderr, "\rStopping %s...", desc);

	if (opts == NULL)
		opts = "";

	len = snprintf (NULL, 0, STOP_FMT, pidfile, opts) + 1;

	if ((cmd = malloc (len)) == NULL)
		err (1, "E");

	snprintf (cmd, len, STOP_FMT, pidfile, opts);
	status = system (cmd);
	free (cmd);

	(void) unlink (pidfile);

	print_status ("Stop", desc, status == 0, silent | restart);
	return status;
}

static int service_usage (void)
{
	fprintf (stderr, "usage:\n\t/etc/init.d/%s "
			 "(start|stop|status|reload|restart)\n", name);
	return 0;
}

int main (int argc, char *argv[])
{
	int silent = !isatty (fileno (stderr));

	service_init ();
	term_init ();

	if (access (daemon_path, X_OK) != 0)
		return 0;

	if (argc > 1 && strcmp (argv[1], "-d") == 0)
		daemonize = 1, --argc, ++argv;

	switch (argc) {
	case 2:
		if (strcmp (argv[1], "status") == 0)
			return service_status (silent);

		if (strcmp (argv[1], "reload") == 0)
			return service_reload (silent);

		if (strcmp (argv[1], "usage") == 0)
			return service_usage ();

		/* pass throught, argv[2] is NULL */
	case 3:
		if (strcmp (argv[1], "start") == 0)
			return service_start (argv[2], silent, 0);

		if (strcmp (argv[1], "stop") == 0)
			return service_stop (argv[2], silent, 0);

		if (strcmp (argv[1], "restart") == 0) {
			(void) service_stop  (argv[2], silent, 1);
			return service_start (argv[2], silent, 1);
		}
	}

	fprintf (stderr, "usage:\n"
			 "\tyonk-service (reload|status|usage)\n"
			 "\tyonk-service [-d] (start|stop|restart) [opts]\n");
	return 1;
}
