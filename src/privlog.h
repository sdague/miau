/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2004-2005 Tommi Saviranta <wnd@iki.fi>
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

#ifndef PRIVLOG_H_
#define PRIVLOG_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include "llist.h"

#include <stdio.h>
#include <time.h>



#ifdef PRIVLOG



typedef struct {
	char	*nick;		/* Log for... */
	FILE	*file;		/* Pointer to logfile. */
	time_t	updated;	/* Last entry written in... */
} privlog_type;



/* For privmsg logging. */
#define PRIVLOG_NEVER		0
#define	PRIVLOG_DETACHED	1
#define PRIVLOG_ATTACHED	2
#define PRIVLOG_ALWAYS		3



/* Check logs for old-age every 30 seconds. */
#define PRIVLOG_CHECK_PERIOD	30
/* Close logs that have been idle for five minutes. */
#define PRIVLOG_TIME_OPEN	300
/* Log-headers won't be written if log is reopened within an hour. */
#define PRIVLOG_TIME_GRACE	3600



#define PRIVLOG_IN	0	/* Other one talking. */
#define PRIVLOG_OUT	1	/* User talking. */



int privlog_write(const char *nick, int in_out, int cmd, const char *message);
void privlog_close_old(void);
void privlog_close_all(void);
int privlog_has_open(void);
#ifdef DUMPSTATUS
llist_list *privlog_get_list(void);
#endif /* ifdef DUMPSTATUS */



#endif /* ifdef PRIVLOG */

#endif /* ifndef PRIVLOG_H_ */
