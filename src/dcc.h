/*
 * -------------------------------------------------------
 * Copyright (C) 2003 Tommi Saviranta <tsaviran@cs.helsinki.fi>
 *	(C) 1998-2001 Sebastian Kienzl <zap@riot.org>
 * -------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _DCC_H
#define _DCC_H

#include <config.h>
#include <string.h>

#ifdef DCCBOUNCE

extern char *dcc_initiate(char* param, int fromclient);
extern void dcc_socketsubscribe(fd_set* readset, fd_set* writeset);
extern void dcc_socketcheck(fd_set* readset, fd_set* writeset);
extern void dcc_timer(); /* To be called every second ! */

#endif /* DCCBOUNCE */

#endif /* _DCC_H */
