/*
 * Terminal Status Report helper
 *
 * Copyright (c) 2016-2022 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef TERM_STATUS_H
#define TERM_STATUS_H  1

#include <stdio.h>

void term_init (FILE *to);
void term_show_status (FILE *to, const char *verb, const char *desc, int ok);

#endif  /* TERM_STATUS_H */
