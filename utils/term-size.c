/*
 * Utility to set terminal window size to real terminal size
 *
 * Copyright (c) 2017-2018 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <err.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>

#include <poll.h>
#include <termios.h>
#include <unistd.h>

int term_get_window_size (int fd, unsigned *w, unsigned *h)
{
	struct winsize ws;

	if (ioctl (fd, TIOCGWINSZ, &ws) == -1)
		return 0;

	*w = ws.ws_col;
	*h = ws.ws_row;

	return 1;
}

int term_set_window_size (int fd, unsigned w, unsigned h)
{
	struct winsize ws;

	memset (&ws, 0, sizeof (ws));

	ws.ws_col    = w;
	ws.ws_row    = h;

	return ioctl (fd, TIOCSWINSZ, &ws) == 0;
}

ssize_t read_timeout(int fd, void *buf, size_t count, int timeout)
{
	struct pollfd p;
	int ret;

	p.fd      = fd;
	p.events  = POLLIN | POLLPRI;

	if ((ret = poll (&p, 1, timeout)) <= 0)
		return ret;

	return read (fd, buf, count);
}

int term_req_window_size (int fd, unsigned *w, unsigned *h)
{
	static const char magic[] =
		"\0337\033[s"	/* DECSC, CSI save cursor */
		"\033[999;999H"	/* CUP (999, 999) */
		"\033[6n"	/* CPR request */
		"\033[u\0338";	/* CSI restore cursor, DECRC */

	const int magic_len = sizeof (magic) - 1;  /* excluding \0 */
	struct termios to, tr;
	char line[64];
	int status;

	if (tcgetattr (fd, &to) == -1)
		return 0;

	tr = to;
	cfmakeraw (&tr);
	tr.c_cflag &= ~TOSTOP & ~CRTSCTS;
	tr.c_lflag |= ISIG;
	(void) tcsetattr (fd, TCSANOW /* FLUSH */, &tr);

	status = write (fd, magic, magic_len) == magic_len;
	if (status) {
		status = read_timeout (fd, line, sizeof (line) - 1, 100);
		if (status < 0)
			status = 0;
		else {
			line[status] = '\0';
			status = sscanf (line, "\033[%u;%uR", h, w) == 2;
		}
	}

	(void) tcsetattr (fd, TCSAFLUSH, &to);
	return status;
}

int main (int argc, char *argv[])
{
	unsigned w = 0, h = 0;

	if (!isatty (0))
		errx (1, "stdin is not a terminal");

	if (!term_get_window_size (0, &w, &h))
		err (1, "cannot get terminal window size");

	if (w != 0 && h != 0)
		return 0;  /* terminal size set already */

	if (!term_req_window_size (0, &w, &h))
		errx (1, "cannot get terminal size from terminal");

	printf ("terminal size: cols %u lines %u\n", w, h);

	if (!term_set_window_size (0, w, h))
		err (1, "cannot set terminal window size");

	return 0;
}
