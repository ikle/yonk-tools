/*
 * Run command or start daemon
 *
 * Copyright (c) 2020-2022 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef SPAWN_H
#define SPAWN_H  1

int spawn_ve (const char *pidfile, char *const argv[], char *const envp[]);

#endif  /* SPAWN_H */
