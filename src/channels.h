/* $Id$
 * -------------------------------------------------------
 * Copyright 2002-2005 Tommi Saviranta <wnd@iki.fi>
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

#ifndef CHANNELS_H_
#define CHANNELS_H_

#include "conntype.h"
#include "llist.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


enum {
	LIST_PASSIVE = 0,
	LIST_ACTIVE,
	LIST_OLD
};

#define JOINTRIES_UNSET	-1

struct channel_log {
	int	type;
	FILE	*file;
};

typedef struct {
	/*
	 * name is the real name of a channel. This field should differ from
	 * simplename only in case of a safe channel or such. If the channel is
	 * a safe channel, name can be something like "!ONZGEfoobar" while
	 * simplename is "!foobar". simplename is used e.g. for logging and
	 * joining the channel. name and simplename will point to the same
	 * address if they're the same; simple_set will be set once it have
	 * been checked if they should differ.
	 */
	char		*name;
	char		*simple_name;
	int		name_set;	/* real name have been set */
	int		simple_set;	/* simple name have been set */
	char		*topic;		/* Channel topic. */
	char		*topicwho;	/* Topic set by ... */
	char		*topicwhen;	/* Topic set in ... */
	char		*key;		/* Channel key. */
	int		jointries;	/* Remaining # of jointries */
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

void channel_free(channel_type *chan);
channel_type *channel_add(const char *channel, const char *key, const int list);
void channel_rem(channel_type *chptr, const int list);
void channel_drop_all(const int keeplog);
channel_type *channel_find(const char *name, int list);
void channel_join_list(const int list, const int rejoin,
		connection_type *client);

int channel_is_name(const char *name);

void channel_topic(channel_type *chan, const char *topic);
void channel_when(channel_type *chan, const char *who, const char *when);

char *channel_simplify_name(const char *chan);

#ifdef OBSOLETE
extern unsigned int channel_hash(char *);
#endif /* OBSOLETE */

#endif /* ifndef CHANNELS_H_ */
