/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2003-2005 Tommi Saviranta <wnd@iki.fi>
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

#ifndef DCC_H_
#define DCC_H_

#include <config.h>
#include <string.h>
#include <unistd.h>

#ifdef DCCBOUNCE

extern char *dcc_initiate(char* param, int fromclient);
extern void dcc_socketsubscribe(fd_set* readset, fd_set* writeset);
extern void dcc_socketcheck(fd_set* readset, fd_set* writeset);
extern void dcc_timer(void); /* To be called every second ! */

#endif /* DCCBOUNCE */

#endif /* DCC_H_ */
