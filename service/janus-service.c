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

static char *cuf, *el, *setaf, *op;

static void term_init (void)
{
	char *term = getenv ("TERM");

	if (term == NULL || term[0] == '\0')
		term = "ansi";

	setupterm (term, 1, NULL);

	cuf   = tigetstr ("cuf");
	el    = tigetstr ("el");
	setaf = tigetstr ("setaf");
	op    = tigetstr ("op");
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

static void print_status (const char *verb, const char *desc, int ok)
{
	FILE *to = stderr;

	fprintf (to, "\r%s%s %s", tiparm (el), verb, desc);

	term_pos (to, -8);
	fprintf (to, "[ ");

	if (ok) {
		term_setaf (to, COLOR_GREEN);
		fprintf (to, " ok ");
	}
	else {
		term_setaf (to, COLOR_RED);
		fprintf (to, "fail");
	}

	term_op (to); fprintf (to, " ]\n");

	syslog (ok ? LOG_NOTICE: LOG_ERR, "%s %s: %s", verb, desc,
		ok ? "ok" : "failed");
}

static char *name, *desc, *daemon_path, *pidfile;

#define DAEMON_FMT	"/usr/sbin/%s"
#define PIDFILE_FMT	"/var/run/%s.pid"
#define START_FMT	"start-stop-daemon -q -S -p %s -x %s %s"
#define STOP_FMT	"start-stop-daemon -q -K -o -p %s %s"

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

static int service_status (int quiet)
{
	int status;
	pid_t pid;

	if ((pid = service_pid ()) == -1)
		status = 0;
	else
		status = kill (pid, 0) == 0;

	if (!quiet)
		printf ("Service %s is %srunning\n", desc,
			status ? "" : "not ");

	return status;
}

static void service_start (const char *opts)
{
	int len;
	char *cmd;
	int status;

	if (service_status (1)) {
		printf ("Service %s already running\n", desc);
		exit (0);
	}

	fprintf (stderr, "\rStarting %s...", desc);

	if (opts == NULL)
		opts = "";

	len = snprintf (NULL, 0, START_FMT, pidfile, daemon_path, opts) + 1;

	if ((cmd = malloc (len)) == NULL)
		err (1, "E");

	snprintf (cmd, len, START_FMT, pidfile, daemon_path, opts);
	status = system (cmd);
	free (cmd);
  
	print_status ("Start", desc, status == 0);
	exit (status);
}

static void service_stop (const char *opts)
{
	int len;
	char *cmd;
	int status;

	if (!service_status (1)) {
		printf ("Service %s is not running\n", desc);
		exit (0);
	}

	fprintf (stderr, "\rStopping %s...", desc);

	if (opts == NULL)
		opts = "";

	len = snprintf (NULL, 0, STOP_FMT, pidfile, opts) + 1;

	if ((cmd = malloc (len)) == NULL)
		err (1, "E");

	snprintf (cmd, len, STOP_FMT, pidfile, opts);
	status = system (cmd);
	free (cmd);

	print_status ("Stop", desc, status == 0);
	exit (status);
}

int main (int argc, char *argv[])
{
	service_init ();
	term_init ();

	if (access (daemon_path, X_OK) != 0)
		return 0;

	if (argc == 2 && strcmp (argv[1], "status") == 0)
		return service_status (0) ? 0 : 1;
	if (argc == 2 && strcmp (argv[1], "usage") == 0) {
		fprintf (stderr, "usage:\n\t/etc/init.d/%s "
				 "(start|stop|status|restart)\n", name);
		return 0;
	}
	else if ((argc == 2 || argc == 3) && strcmp (argv[1], "start") == 0)
		service_start (argv[2]);
	else if ((argc == 2 || argc == 3) && strcmp (argv[1], "stop") == 0)
		service_stop (argv[2]);

	fprintf (stderr, "usage:\n"
			 "\tjanus-service (status | usage)\n"
			 "\tjanus-service (start|stop) [opts]\n");
	return 1;
}
