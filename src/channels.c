/*
 * -------------------------------------------------------
 * Copyright (C) 2002-2003 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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

#include "miau.h"
#include "channels.h"
#include "tools.h"
#include "llist.h"
#include "irc.h"
#include "table.h"
#include "log.h"
#include "qlog.h"
#include "messages.h"
#ifdef AUTOMODE
#  include "automode.h"
#endif /* AUTOMODE */



llist_list	active_channels;
llist_list	passive_channels;
llist_list	old_channels;

extern clientlist_type	c_clients;



/*
 * Adds a channel to miaus internal list.
 */
channel_type *
channel_add(
		const char	*channel,
		const int	list
	   )
{
	llist_list	*target = NULL;	/* List on which to add. */
	llist_node	*ptr;
	channel_type	*chptr = NULL;

	/* See if channel is already on active list. */
	if (channel_find(channel, LIST_ACTIVE) != NULL) {
#ifdef ENDUSERDEBUG
		if (list == LIST_PASSIVE) {
			enduserdebug("add chan to pass when already on active");
		}
#endif /* ENDUSERDEBUG */
		return NULL;
	}

	/*
	 * See if channel is already on passive list. If it is, it isn't
	 * a bad thing, tho.
	 */
	if (list == LIST_PASSIVE) {
		if (channel_find(channel, LIST_PASSIVE) != NULL) {
			return NULL;
		}
	}

	/*
	 * See if we could simply move channels from passive/old_channels to
	 * active_channels. If we can do this, it's all we need to do.
	 */
	if (list == LIST_ACTIVE) {
		llist_list	*source = NULL;
		/* See if channel's on passive_channels. */
		chptr = channel_find(channel, LIST_PASSIVE);
		if (chptr != NULL) {
			source = &passive_channels;
		} else {
			/* See if channe's on old_channels, then. */
			chptr = channel_find(channel, LIST_OLD);
			if (chptr != NULL) {
				source = &old_channels;
			}
		}
		/* Channel was found on a list ? */
		if (chptr != NULL) {
			/* Remove old node. */
			ptr = llist_find((void *) chptr, source);
			if (ptr != NULL) {
				llist_delete(ptr, source);
			}
			target = &active_channels;
		}
	}

	/* Perhaps channel was not on passive/old_channels, after all. */
	if (chptr == NULL) {
		/* Create new node to channel list. */
		chptr = (channel_type *) xcalloc(1, sizeof(channel_type));
		chptr->name = strdup(channel);
		/*
		 * * We don't need to touch other variabels - calloc did the
		 * job. This is neat, since a few values are set to 0 by
		 * default.
		 */
		chptr->key = strdup("-");	/* Not keyword protected. */
#ifdef AUTOMODE
		chptr->oper = -1;		/* Don't know our status. */
#endif /* AUTOMODE */
	
		/* Get list on which to add this channel to. */
		if (list == LIST_ACTIVE) {
			target = &active_channels;
		} else if (list == LIST_PASSIVE) {
			target = &passive_channels;
		} else {
			target = &old_channels;
		}
	}

	llist_add_tail(llist_create(chptr), target);

	/*
	 * From now on, we know two things:
	 *   - channel is only on one list
	 *   - chptr is non-NULL
	 */
	
#ifdef LOGGING
	/* Active channels may need log-structures. */
	if (list == LIST_ACTIVE) {
		log_open(chptr);
	}
#endif /* LOGGING */

	return chptr;
} /* channel_type *channel_add(const char *, const int) */



/*
 * Rmoves a channel from miaus internal list.
 *
 * "list" defines list hannel is to be removed from.
 */
void
channel_rem(
		channel_type	*chptr,
		const int	list
	   )
{
	llist_list	*source;
	llist_node	*node;

	if (list == LIST_ACTIVE) {
		source = &active_channels;
#ifdef QUICKLOG
		/* Need to know which active channels have qlog. */
		qlog_check();
#endif /* QUICKLOG */
	} else {
		source = (list == LIST_PASSIVE) ?
			&passive_channels : &old_channels;
	}
	
	/* Only remove existing channels. :-) */
	if (chptr == NULL) {
#ifdef ENDUSERDEBUG
		enduserdebug("channel_rem(NULL, %d) called", list);
#endif /* ENDUSERDEBUG */
		return;
	}

#ifdef AUTOMODE
	automode_drop_channel(chptr, NULL, ANY_MODE);
#endif /* AUTOMODE */

#ifdef LOGGING
	/* Close the logfile if we have one. */
	if (chptr->log != NULL) {
		log_close(chptr);
	}
#endif /* LOGGING */

	node = llist_find(chptr, source);
	if (node == NULL) {
#ifdef ENDUSERDEBUG
		if (list == LIST_ACTIVE) {
			enduserdebug("removing non-existing channel");
		}
#endif /* ENDUSERDEBUG */
		return;
	}

#ifdef QUICKLOG
	/* See if we need to move this channel to list of old channels. */
	if (chptr->hasqlog && list == LIST_ACTIVE) {
		/* Yep, we'll just move it. */
		llist_add_tail(llist_create(chptr), &old_channels);
	} else {
		/* No moving, just freeing the resources. */
		xfree(chptr->name);
		xfree(chptr->topic);
		xfree(chptr->topicwho);
		xfree(chptr->topicwhen);
		xfree(chptr->key);
		xfree(chptr);
	}
#else /* QUICKLOG */
	/* Free resources. */
	xfree(chptr->name);
	xfree(chptr->topic);
	xfree(chptr->topicwho);
	xfree(chptr->topicwhen);
	xfree(chptr->key);
	xfree(chptr);
#endif /* QUICKLOG */

	/* And finally, remove channel from the list. */
	llist_delete(node, source);
} /* void channel_rem(channel_type *, const int) */



/*
 * Searches for a channel, returning it if found, else NULL.
 *
 * "list" declares which list to search.
 */
channel_type *
channel_find(
		const char	*channel,
		const int	list
	    )
{
	llist_node	*node;
	channel_type	*chptr;

	if (list == LIST_ACTIVE) {
		node = active_channels.head;
	} else if (list == LIST_PASSIVE) {
		node = passive_channels.head;
	} else {
		node = old_channels.head;
	}

	for (;node != NULL; node = node->next) {
		chptr = node->data;
		
		if (xstrcasecmp(chptr->name, channel) == 0) return chptr;
	}

	return NULL;
} /* channel_type *channel_find(const char *, const int) */



/*
 * Stores a topic for a channel we're in.
 */
void
channel_topic(
		channel_type	*chptr,
		char		*topic
	     )
{
	xfree(chptr->topic);
	xfree(chptr->topicwho);
	xfree(chptr->topicwhen);
	chptr->topic = NULL;
	chptr->topicwho = NULL;
	chptr->topicwhen = NULL;

	if (topic != NULL && topic[0] != '\0') {
		chptr->topic = strdup(topic);
	}
} /* void channel_topic(channel_type *, char *) */



/*
 * Stores who set the topic for a channel we're in.
 */
void
channel_when(
		channel_type	*chptr,
		char		*topicwho,
		char		*topicwhen
	    )
{
	if (chptr->topic != NULL) {
		xfree(chptr->topicwho);
		xfree(chptr->topicwhen);
		
		chptr->topicwho = strdup(topicwho);
		chptr->topicwhen = strdup(topicwhen);
	}
} /* void channel_when(channel_type *, char *, char *) */



void
channel_join_list(
		const int	list,
		const int	rejoin,
		connection_type	*client
		)
{
	llist_node *first = (list == LIST_PASSIVE) ?
		passive_channels.head : active_channels.head;
	char	*chans = NULL;
	char	*keys = NULL;

	/* Join old_channels. */
	LLIST_WALK_H(old_channels.head, channel_type *);
		irc_write(client, ":%s!%s JOIN :%s",
				status.nickname,
				status.idhostname,
				data->name);
	LLIST_WALK_F;

	if (first != NULL) {
		chans = xcalloc(1, 1);
		keys = xcalloc(1, 1);
	}

#ifdef QUICKLOG
	/*
	 * If replaying qlog for active channels, see if channels have qlog.
	 * We need to do this because we want to print header for qlog.
	 */
	if (list == LIST_ACTIVE) {
		qlog_check();
	}
#endif /* QUICKLOG */
	
	LLIST_WALK_H(first, channel_type *);
		if (list == LIST_PASSIVE) {
			/* We're trying to join this just a second later. */
			data->joining = 1;
			/* Add channel and key in queue. */
			chans = xrealloc(chans, strlen(data->name) +
					strlen(chans) + 2);
			keys = xrealloc(keys, strlen(keys) +
					strlen(data->key) + 2);
			strcat(chans, ",");
			strcat(chans, data->name);
			strcat(keys, ",");
			strcat(keys, data->key);
		} else {
			/* Tell client to join this channel. */
			irc_write(client, ":%s!%s JOIN :%s",
					status.nickname,
					status.idhostname,
					data->name);

			/* Also tell client about this channel. */
			irc_write(client, ":%s %d %s %s %s",
					i_server.realname,
					(data->topic != NULL ?
						RPL_TOPIC : RPL_NOTOPIC),
					status.nickname,
					data->name,
					(data->topic != NULL ?
						data->topic :
						":No topic is set"));
			if (data->topicwho != NULL && data->topicwhen != NULL) {
				irc_write(client, "%s %d %s %s %s %s",
						i_server.realname,
						RPL_TOPICWHO,
						status.nickname,
						data->name,
						data->topicwho,
						data->topicwhen);
			}
#ifdef LOGGING
			if (HAS_LOG(data, LOG_MIAU)) {
				log_write_entry(data, LOGM_MIAU,
						gettimestamp(0), "");
			}
#endif /* LOGGING */
			irc_write(&c_server, "NAMES %s", data->name);
#ifdef QUICKLOG
			/* Write header for qlog. */
			if (data->hasqlog) {
				irc_write(client, ":%s NOTICE %s :%s",
						status.nickname,
						data->name,
						CLNT_QLOGSTART);
			}
#endif /* QUICKLOG */
		}
	LLIST_WALK_F;
	
#ifdef QUICKLOG
	if (list == LIST_ACTIVE) {
		/* Replay quicklog. */
		qlog_replay(client, ! cfg.flushqlog);

		/* Write footer for qlog. */
		LLIST_WALK_H(first, channel_type *);
			if (data->hasqlog) {
				irc_write(client, ":%s NOTICE %s :%s",
						status.nickname,
						data->name,
						CLNT_QLOGEND);
			}
		LLIST_WALK_F;
	}
#endif /* QUICKLOG */

	if (chans != NULL && list == LIST_PASSIVE) {
		report(rejoin ? MIAU_REINTRODUCE : MIAU_JOINING, chans + 1);
		irc_write(&c_server, "JOIN %s %s", chans + 1, keys + 1);
	}
	
	if (first != NULL) {
		xfree(chans);
		xfree(keys);
	}
} /* void channel_join_list(const int, const int, connection_type *) */



#ifdef OBSOLETE /* Never defined - ignore this. */
/*
 * Creates a hash value based on the first 25 chars of a channel name.
 */
unsigned int
channel_hash(
		char	*p
		)
{
	int		i = 25;
	unsigned int	hash = 0;

	while (*p && --i) {
		hash = (hash << 4) - (hash + (unsigned char) tolower(*p++));
	}

	return (hash & (MAX_CHANNELS - 1));
} /* unsigned int channel_hash(char *) */
#endif /* OBSOLETE */