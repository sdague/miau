/*
 * -------------------------------------------------------
 * Copyright (C) 2004 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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

#ifndef _PRIVLOG_H
#define _PRIVLOG_H

#include <config.h>

#include <stdio.h>
#include <time.h>

#include "llist.h"



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



int privlog_write(const char *, const int, const char *);
void privlog_close_old();
void privlog_close_all();
int privlog_has_open();
#ifdef DUMPSTATUS
llist_list *privlog_get_list();
#endif /* DUMPSTATUS */



#endif /* PRIVLOG */

#endif /* _PRIVLOG_H */
