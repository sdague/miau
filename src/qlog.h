/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2003-2005 Tommi Saviranta <wnd@iki.fi>
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

#ifndef _QLOG_H
#define _QLOG_H

#include <config.h>
#include "miau.h"
#include "llist.h"

#include "channels.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>



#ifdef QUICKLOG

#ifdef QLOGSTAMP 
#define TS_TYPES	"nbe"
#define TS_NONE		0
#define TS_BEGINNING	1
#define TS_END		2
#endif /* QLOGSTAMP */

llist_list	qlog;



/* Entry per line in quicklog. */
typedef struct {
	char	*text;
	time_t	timestamp;
#ifdef INBOX
	int	privmsg;
#endif /* INBOX */
} qlogentry;


void qlog_check(void);
void qlog_replay(connection_type *client, const int keep);
void qlog_drop_old(void);
void qlog_write(const int privmsg, char *format, ...);

channel_type *qlog_get_channel(const char *msg);
#endif /* QUICKLOG */

#endif /* _QLOG_H */
