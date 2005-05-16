/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2002-2005 Tommi Saviranta <wnd@iki.fi>
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
#include "automode.h"
#include "channels.h"
#include "llist.h"
#include "irc.h"
#include "perm.h"

#ifdef AUTOMODE
permlist_type	automodelist;



/*
 * Process mode queues.
 */
void
automode_do(void)
{
	channel_type	*channel;	/* Channel the modes are for. */
	char		modes[4];	/* Buffer for three modes */
	char		*nicks = malloc(1);	/* Buffer for nicks */
	int		modecount;

	bzero(modes, 4);		/* Clear modes. */
	

	LLIST_WALK_H(active_channels.head, channel_type *);
		channel = data;

		if (channel->oper == 1) {
			nicks[0] = '\0';	/* Clear nicks. */
			bzero(modes, 4);	/* Clear modes. */
			modecount = 0;
		
			/* Commit three modes at a time. */
			LLIST_WALK_H(channel->mode_queue.head, automode_type *);
				modes[modecount] = data->mode;
				nicks = (char *) xrealloc(nicks, strlen(nicks)
						+ strlen(data->nick) + 2);
				strcat(nicks, " ");
				strcat(nicks, data->nick);
				modecount++;
				if (modecount == 3) {
					irc_write_head(&c_server,
							"MODE %s +%s%s",
							channel->name,
							modes,
							nicks);
					nicks[0] = '\0'; /* Clear nicks. */
					bzero(modes, 4); /* Clear modes. */
					modecount = 0;
				}
			LLIST_WALK_F; /* walk queue */

			/* Commit remaining modes. */
			if (modecount > 0) {
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
	
	char		modes[2] = "ov";
	int		mode_c = 2;

	mask = (char *) xmalloc(strlen(nick) + strlen(hostname)
			+ strlen(channel->name) + 5);

	/* Generate mask and see if any automode should take place. */
	while (mode_c-- > 0) {
		sprintf(mask, "%c:%s!%s/%s", modes[mode_c], nick, hostname,
				channel->name);
		if (is_perm(&automodelist, mask) && automode_lookup(nick,
					channel, modes[mode_c]) == NULL) {
			modeact = (automode_type *)
				xmalloc(sizeof(automode_type));
			modeact->nick = strdup(nick);
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
		AUTOMODE_DROP(data, node, queue);
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
		if (nick_ok && mode_ok) {
			AUTOMODE_DROP(data, node, &channel->mode_queue);
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



#endif /* AUTOMODE */
