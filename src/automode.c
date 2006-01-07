/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2002-2006 Tommi Saviranta <wnd@iki.fi>
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



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#ifdef AUTOMODE

#include "automode.h"
#include "common.h"
#include "llist.h"
#include "perm.h"
#include "irc.h"
#include "miau.h"
/* vsnprintf */
#include "tools.h"

#include <string.h>

#if HAVE_STRINGS_H
#include <strings.h>
#endif



permlist_type	automodelist;



static inline void
automode_drop(automode_type *line, llist_node *node, llist_list *list)
{
	xfree(line->nick);
	xfree(node->data);
	llist_delete(node, list);
} /* static inline void automode_drop(automode_type *line, llist_node *node,
		llist_list *list) */



/*
 * Process mode queues.
 */
void
automode_do(void)
{
	channel_type *channel;
	char modes[4];
	char *nicks;
	size_t size;

	size = 1;
	nicks = (char *) xmalloc(size);

	LLIST_WALK_H(active_channels.head, channel_type *);
		channel = data;

		if (channel->oper == 1) {
			int count;
			size_t nlen, tlen;

			nicks[0] = '\0';	/* clear nicks */
			tlen = 1;
			bzero(modes, sizeof(modes));	/* clear modes */
			count = 0;
		
			/* Commit three modes at a time. */
			LLIST_WALK_H(channel->mode_queue.head, automode_type *);
				modes[count] = data->mode;
				/* paranoid */
				nlen = strlen(data->nick);
				tlen += nlen + 1;
				if (tlen > size) {
					size = tlen;
					nicks = (char *) xrealloc(nicks, size);
				}
				strcat(nicks, " ");
				strncat(nicks, data->nick, nlen);
				count++;
				if (count == 3) {
					irc_write_head(&c_server,
							"MODE %s +%s%s",
							channel->name,
							modes,
							nicks);
					nicks[0] = '\0'; /* Clear nicks. */
					bzero(modes, 4); /* Clear modes. */
					count = 0;
					tlen = 1;
				}
			LLIST_WALK_F; /* walk queue */

			/* Commit remaining modes. */
			if (count > 0) {
				irc_write_head(&c_server,
						"MODE %s +%s%s",
						channel->name,
						modes,
						nicks);
			}
		
			/* Clear mode-queue as there are all now processed. */
			automode_clear(&data->mode_queue);
		} /* if (channel->oper == 1) */
	LLIST_WALK_F;	/* walk channels */

	xfree(nicks);
} /* void automode_do(void) */



/*
 * Add nick to be auto-opped.
 */
void
automode_queue(const char *nick, const char *hostname, channel_type *channel)
{
	automode_type	*modeact;
	char		*mask;
	size_t		msize;

	char		modes[] = "ov";
	int		mode_c;

	/* termination and validity guaranteed */
	msize = strlen(nick) + strlen(hostname) + strlen(channel->name) + 5;
	mask = (char *) xmalloc(msize);

	for (mode_c = 0; mode_c < 2; mode_c++) { /* strlen(ov) - 1 */
		/* generate mask and see if any automode should take place */
		snprintf(mask, msize, "%c:%s!%s/%s",
				modes[mode_c], nick, hostname,
				channel->name);
		mask[msize - 1] = '\0';
		if (is_perm(&automodelist, mask) && automode_lookup(nick,
					channel, modes[mode_c]) == NULL) {
			modeact = (automode_type *)
				xmalloc(sizeof(automode_type));
			modeact->nick = xstrdup(nick);
			modeact->mode = modes[mode_c];
			llist_add_tail(llist_create(modeact),
					&channel->mode_queue);
			status.automodes++;
		}
	}
	
	xfree(mask);
} /* void automode_queue(const char *, const char *, channel_type *) */



void
automode_clear(llist_list *queue)
{
	LLIST_WALK_H(queue->head, automode_type *);
		automode_drop(data, node, queue);
		status.automodes--;
	LLIST_WALK_F;
} /* void automode_clear(llist_list *queue) */



/*
 * Drop automode action from automode -queue.
 *
 * If mode == NULL, don't care about mode.
 * If channel == NULL, don't care about channel.
 */
void
automode_drop_nick(const char *nick, const char mode)
{
	LLIST_WALK_H(active_channels.head, channel_type *);
		automode_drop_channel(data, nick, mode);
	LLIST_WALK_F;
} /* void automode_drop_nick(const char *, const char) */



void
automode_drop_channel(channel_type *channel, const char *nick, const char mode)
{
	int nick_ok, mode_ok;
	LLIST_WALK_H(channel->mode_queue.head, automode_type *);
		nick_ok = (nick == NULL) || (nick != NULL
				&& xstrcasecmp(nick, data->nick) == 0);
		mode_ok = (mode == '\0') || (mode == data->mode);
		if (nick_ok == 1 && mode_ok == 1) {
			automode_drop(data, node, &channel->mode_queue);
		}
	LLIST_WALK_F;
} /* void automode_drop_channel(channel_type *, const char *, const char) */



/*
 * Look up for an auto-op action in queue.
 *
 * Returns pointer to automode_type if found, otherwise NULL:
 */
llist_node *
automode_lookup(const char *nick, channel_type *channel, const char mode)
{
	llist_node	*ptr;
	automode_type	*modeact;

	for (ptr = channel->mode_queue.head; ptr != NULL; ptr = ptr->next) {
		modeact = (automode_type *) ptr->data;
		if (xstrcasecmp(nick, modeact->nick) == 0
				&& modeact->mode == mode) {
			return ptr;
		}
	}

	return NULL;
} /* llist_nost *automode_lookup(channel_type *, const char *, const char) */



#endif /* ifdef AUTOMODE */
