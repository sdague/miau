/*
 * -------------------------------------------------------
 * Copyright 2002-2003 Tommi Saviranta <tsaviran@cs.helsinki.fi>
 *	(C) 2002 Lee Hardy <lee@leeh.co.uk>
 *	(C) 1998-2002 Sebastian Kienzl <zap@riot.org>
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

#ifndef _CHANNELS_H
#define _CHANNELS_H

#include "conntype.h"
#include "llist.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define LIST_PASSIVE	0
#define LIST_ACTIVE	1
#define LIST_OLD	2


struct channel_log {
	int	type;
	FILE	*file;
};

typedef struct {
	char		*name;		/* Channel name. */
	char		*topic;		/* Channel topic. */
	char		*topicwho;	/* Topic set by ... */
	char		*topicwhen;	/* Topic set in ... */
	char		*key;		/* Channel key. */
	int		jointries;	/* Will try to join # times. */
#ifdef AUTOMODE
	/*
	 * oper:
	 *   0: not operator and we know it
	 *   1: operator and we know it
	 *  -1: don't know if we're operator
	 */
	int		oper;
	llist_list	mode_queue;	/* Queue for channel modes. */
#endif /* AUTOMODE */
#ifdef QUICKLOG
	int		hasqlog;	/* Channel has qlog. */
#endif /* QUICKLOG */
	struct channel_log	*log;
} channel_type;

struct			llist_list;
extern llist_list	active_channels;
extern llist_list	passive_channels;
extern llist_list	old_channels;

channel_type *channel_add(const char *channel, const char *key, const int list);
void channel_rem(channel_type *chptr, const int list);
void channel_drop_all(const int keeplog);
channel_type *channel_find(const char *channel, const int list);
void channel_join_list(const int list, const int rejoin,
		connection_type *client);

void channel_topic(channel_type *, char *);
void channel_when(channel_type *, char *, char *);

#ifdef OBSOLETE
extern unsigned int channel_hash(char *);
#endif /* OBSOLETE */

#endif /* _CHANNELS_H */
