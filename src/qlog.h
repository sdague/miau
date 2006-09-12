/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2003-2006 Tommi Saviranta <wnd@iki.fi>
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

#ifndef QLOG_H_
#define QLOG_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#ifdef QUICKLOG

#include "conntype.h"

#include <time.h>



#ifdef QLOGSTAMP 
#define TS_TYPES	"nbe"
#define TS_NONE		0
#define TS_BEGINNING	1
#define TS_END		2
#endif /* ifdef QLOGSTAMP */



/* Entry per line in quicklog. */
typedef struct {
	char	*text;
	time_t	timestamp;
#ifdef INBOX
	int	privmsg;
#endif /* ifdef INBOX */
} qlogentry;


void qlog_check(int age);
void qlog_replay_header(connection_type *client);
void qlog_replay_footer(connection_type *client);
void qlog_replay(connection_type *client, time_t oldest);
void qlog_flush(time_t oldest, int move_to_inbox);
void qlog_write(const int privmsg, char *format, ...);



#endif /* ifdef QUICKLOG */

#endif /* ifdef QLOG_H_ */
