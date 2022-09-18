/*
 * Service Management helpers
 *
 * Copyright (c) 2016-2022 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef SERVICE_H
#define SERVICE_H  1

struct service {
	const char *bundle, *name, *desc, *daemon, *pidfile, *conf;
	int daemonize;
};

void service_init (struct service *o);

int service_is_running	(struct service *o);
int service_reload	(struct service *o);
int service_start	(struct service *o);
int service_stop	(struct service *o, int verbose);

#endif  /* SERVICE_H */
