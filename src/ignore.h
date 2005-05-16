/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2003-2005 Tommi Saviranta <wnd@iki.fi>
 *	(C) 1998-2000 Sebastian Kienzl <zap@riot.org>
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

#ifndef IGNORE_H_
#define IGNORE_H_

#ifdef CTCPREPLIES

/* ignore-types */
#define IGNORE_MESSAGE 0
#define IGNORE_CTCP 1

void add_ignore(char *hostname, int ttl, int type);
void del_ignore(char *hostname);
void process_ignores(void);
int is_ignore(char *hostname, int type);

#endif /* CTCPREPLIES */

#endif /* IGNORE_H_ */
