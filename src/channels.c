/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2002-2005 Tommi Saviranta <wnd@iki.fi>
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
#include "error.h"
#include "channels.h"
#include "tools.h"
#include "llist.h"
#include "irc.h"
#include "table.h"
#include "chanlog.h"
#include "qlog.h"
#include "messages.h"
#include "automode.h"



/*
 * active_channels
 * 	miau is on these channels
 *
 * passive_channels
 * 	miau will be on these channels.
 * 	For example, if cfg.leave == cfg.rejoin == true, active_channels are
 * 	moved to passive_channels when client detached.
 *
 * old_channels
 *	miau was on these channels - and they still contain some
 */
llist_list	active_channels;
llist_list	passive_channels;
llist_list	old_channels;

extern clientlist_type	c_clients;



/*
 * Free data in channel_type -structure.
 */
void
channel_free(channel_type *chan)
{
	if (chan == NULL) {
#ifdef ENDUSERDEBUG
		enduserdebug("%s(NULL)", __FUNCTION__);
#endif /* ifdef ENDUSERDEBUG */
		return;
	}
	if (chan->simple_name != chan->name) {
		xfree(chan->simple_name);
	}
	xfree(chan->name);
	xfree(chan->topic);
	xfree(chan->topicwhen);
	xfree(chan->topicwho);
	xfree(chan->key);
	xfree(chan);
} /* void channel_free(channel_type *chan) */



/*
 * Adds a channel to miaus internal list.
 */
channel_type *
channel_add(const char *channel, const char *key, const int list)
{
	llist_list *target;
	channel_type *chptr;

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
		llist_list *source;
		
		/* find a list to remove from */
		chptr = channel_find(channel, LIST_PASSIVE);
		if (chptr != NULL) {
			source = &passive_channels;
		} else {
			chptr = channel_find(channel, LIST_OLD);
			if (chptr != NULL) {
				source = &old_channels;
			}
		}

		if (chptr != NULL) {
			llist_node *ptr;
			ptr = llist_find((void *) chptr, source);
			if (ptr != NULL) {
				llist_delete(ptr, source);
			}
#ifdef ENDUSERDEBUG
			else {
				enduserdebug("first the channel found, now it's not. what's going on!?");
			}
#endif /* ifdef ENDUSERDEBUG */
			target = &active_channels;
		}
	} else {
		chptr = NULL;
	}

	/* Perhaps channel was not on passive/old_channels, after all. */
	if (chptr == NULL) {
		/* Create new node to channel list. */
		chptr = (channel_type *) xcalloc(1, sizeof(channel_type));
		chptr->name = xstrdup(channel);
		chptr->simple_name = chptr->name; /* assume to be the same... */
		chptr->simple_set = 0; /* ...but mark as not confirmed */
		chptr->name_set = 0; /* real name isn't known either */
		/*
		 * We don't need to touch other variabels - calloc did the
		 * job. This is neat, since a few values are set to 0 by
		 * default.
		 */
		chptr->key = xstrdup(key == NULL ? "-" : key);
		chptr->jointries = JOINTRIES_UNSET;
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
	
	if (list == LIST_ACTIVE) {
		/* adding to ACTIVE -> we know real name of the channel */
		if (chptr->name_set == 0) {
			/* real name wasn't known before */
			xfree(chptr->name);
			chptr->name = xstrdup(channel);
			chptr->name_set = 1;
			if (chptr->simple_set == 0) {
				chptr->simple_name = chptr->name;
			}
		}
		if (chptr->simple_set == 0) {
			chptr->simple_name = channel_simplify_name(chptr->name);
			chptr->simple_set = 1;
		}
#ifdef CHANLOG
		/* active channels may need log-structures. */
		chanlog_open(chptr);
#endif /* CHANLOG */
	}

	return chptr;
} /* channel_type *channel_add(const char *channel, const char *key,
		const int list) */



/*
 * Rmoves a channel from miaus internal list.
 *
 * "list" defines list hannel is to be removed from.
 */
void
channel_rem(channel_type *chptr, const int list)
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

#ifdef CHANLOG
	/* Close the logfile if we have one. */
	if (chptr->log != NULL) {
		chanlog_close(chptr);
	}
#endif /* CHANLOG */

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
		channel_free(chptr);
	}
#else /* QUICKLOG */
	/* Free resources. */
	channel_free(chptr);
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
channel_find(const char *name, int list)
{
	llist_node *node;
	channel_type *chan;
	channel_type *bmatch;
	char *sname;

	/* this means we won't work with new channel modes */
	if (channel_is_name(name) == 0) {
		return NULL;
	}

	if (list == LIST_ACTIVE) {
		node = active_channels.head;
	} else if (list == LIST_PASSIVE) {
		node = passive_channels.head;
	} else {
		node = old_channels.head;
	}

	sname = channel_simplify_name(name);

	for (bmatch = NULL; node != NULL; node = node->next) {
		chan = (channel_type *) node->data;

		if (xstrcmp(chan->name, name) == 0) {
			/* exact match */
			xfree(sname);
			return chan;
		} else if (xstrcmp(chan->simple_name, sname) == 0) {
			bmatch = chan;
		}
	}
	
	xfree(sname);
	if (bmatch != NULL) {
		return bmatch;
	}
	
	/* no match found */
	return NULL;
} /* channel_type *channel_find(char *name, int list) */



/*
 * Stores a topic for a channel we're in.
 */
void
channel_topic(channel_type *chan, const char *topic)
{
	xfree(chan->topic);
	xfree(chan->topicwho);
	xfree(chan->topicwhen);
	chan->topic = NULL;
	chan->topicwho = NULL;
	chan->topicwhen = NULL;

	if (topic != NULL && topic[0] != '\0') {
		chan->topic = xstrdup(topic);
	}
} /* void channel_topic(channel_type *chan, const char *topic) */



/*
 * Stores who set the topic for a channel we're in.
 */
void
channel_when(channel_type *chan, const char *who, const char *when)
{
	if (chan->topic != NULL) {
		xfree(chan->topicwho);
		xfree(chan->topicwhen);
	
		chan->topicwho = xstrdup(who);
		chan->topicwhen = xstrdup(when);
	}
} /* void channel_when(channel_type *chan, const char *who, char *when) */



void
channel_join_list(const int list, const int rejoin, connection_type *client)
{
	llist_node *first = (list == LIST_PASSIVE) ?
		passive_channels.head : active_channels.head;
	char	*chans = NULL;
	char	*keys = NULL;
	int	try_joining = 0;

	/* Join old_channels if client was defined. */
	if (client != NULL) {
		LLIST_WALK_H(old_channels.head, channel_type *);
			irc_write(client, ":%s!%s JOIN :%s",
					status.nickname,
					status.idhostname,
					data->name);
		LLIST_WALK_F;
	}

	if (first != NULL) {
		chans = (char *) xcalloc(1, 1);
		keys = (char *) xcalloc(1, 1);
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
			if ((rejoin == 1 || (cfg.rejoin == 1 && cfg.leave == 0))
					&& data->jointries == JOINTRIES_UNSET) {
				/*
				 * Set data->jointries to 1 even if
				 * cfg.jointries = 0. This way we can try to
				 * join channels in miaurc. If the channel
				 * is a safe channel, try joining only once.
				 */
				if (cfg.jointries == 0) {
					data->jointries = 1;
				} else {
					if (data->name[0] == '!') {
						data->jointries = 1;
					} else {
						data->jointries = cfg.jointries;
					}
				}
			}
			
			if (data->jointries > 0) {
				int nlen, klen;
				try_joining = 1;
				data->jointries--;
				/* Add channel and key in queue. */
				/*
				 * name and key guaranteed
				 * terminated and valid
				 *
				 * strncat == paranoid
				 */
				/*
				 * join with "simple" name, "real" name
				 * of a safe channel won't do here
				 */
				nlen = strlen(data->name);
				klen = strlen(data->key);
				chans = (char *) xrealloc(chans,
						nlen + strlen(chans) + 2);
				keys = (char *) xrealloc(keys,
						klen + strlen(keys) + 2);
				strcat(chans, ",");
				strncat(chans, data->name, nlen);
				strcat(keys, ",");
				strncat(keys, data->key, klen);
			} else if (data->jointries == 0) {
				channel_type *chan;
				/*
				 * If cfg.jointries is 0, remove channel from
				 * list after unsuccesfull attempt to join it.
				 */
				chan = channel_find(data->name, LIST_PASSIVE);
				channel_rem(chan, LIST_PASSIVE);
			}
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
#ifdef CNANLOG
			if (chanlog_has_log((channel_type *) data, LOG_MIAU)) {
				log_write_entry(data, LOGM_MIAU,
						get_short_localtime(), "");
			}
#endif /* CHANLOG */
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

	if (try_joining == 1) {
		report(rejoin ? MIAU_REINTRODUCE : MIAU_JOINING, chans + 1);
		irc_write(&c_server, "JOIN %s %s", chans + 1, keys + 1);
	}
	
	if (first != NULL) {
		xfree(chans);
		xfree(keys);
	}
} /* void channel_join_list(const int, const int, connection_type *) */



char *
channel_simplify_name(const char *chan)
{
	if (chan[0] != '!') {
		/* not safe-channel */
		return xstrdup(chan);
	} else {
		size_t len;
		char *name;
		
		len = strlen(chan);
		if (len < 6) {
#ifdef ENDUSERDEBUG
			enduserdebug("weird channel: '%s'", chan);
#endif /* ifdef ENDUSERDEBUG */
			return xstrdup(chan); /* some weird channel */
		}
		
		/* safe channel */
		len -= 4; /* original length - 5 + terminator */
		name = (char *) xmalloc(len);
		snprintf(name, len, "!%s", chan + 6);
		name[len - 1] = '\0';
		return name;
	}
} /* char *channel_simplify_name(const char *chan) */



/*
 * Check if name could be a channel name.
 *
 * Return non-zero is name could be a channel name.
 */
int
channel_is_name(const char *name)
{
	if (name == NULL) {
		return 0;
	}

	switch (name[0]) {
		case '#': /* ordinary channel (rfc 1459) */
		case '&': /* local channel (rfc 1459) */
		case '!': /* safe channel (rfc 2811) */
		case '+': /* modeless channel (rfc 2811) */
		case '.': /* programmable channel (kineircd) */
		case '~': /* global channel (what is that? kineircd) */
			return (int) name[0];
			break; /* dummy */

		default:
			return 0;
			break; /* dummy */
	}
} /* int channel_is_name(const char *name) */



#ifdef OBSOLETE /* Never defined - ignore this. */
static void
set_simple_name(channel_type *chan, const char *name)
{
	if (chan->name[0] != '!') {
		/* not safe-channel */
		chan->simple_set = 1;
	} else {
		if (strlen(chan->name) < 2 || strlen(name) < 6) {
			/* channel name too short -- not safe channel */
			return;
		}
		if (xstrcasecmp(chan->name + 2, name + 6) == 0) {
			chan->simplename = xstrdup(name);
			chan->simple_set = 1;
		}
	}
} /* static void set_simple_name(channel_type *chan, const char *name) */



/*
 * Creates a hash value based on the first 25 chars of a channel name.
 */
unsigned int
channel_hash(char *p)
{
	int		i = 25;
	unsigned int	hash = 0;

	while (*p && --i) {
		hash = (hash << 4) - (hash + (unsigned char) tolower(*p++));
	}

	return (hash & (MAX_CHANNELS - 1));
} /* unsigned int channel_hash(char *) */
#endif /* OBSOLETE */
