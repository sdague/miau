/*
 * -------------------------------------------------------
 * Copyright (C) 2002-2003 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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



#ifndef _AUTOMODE_H
#define _AUTOMODE_H

#include <config.h>

#ifdef AUTOMODE

#include "llist.h"
#include "miau.h"

#include <stdlib.h>
#include <string.h>


#define ANY_MODE	'\0'

#define AUTOMODE_DROP(mode_act, node, list) \
	xfree((mode_act)->nick); \
	xfree((node)->data); \
	llist_delete((node), list);



typedef struct {
	char	*nick;		/* Nick to be mode'd */
	char	mode;		/* Mode this nick. */
} automode_type;



void automode_do();
void automode_queue(const char *nick, const char *hostname,
		channel_type *channel);
void automode_clear(llist_list *queue);
void automode_drop_nick(const char *nick, const char mode);
void automode_drop_channel(channel_type *channel, const char *nick,
		const char mode);
llist_node *automode_lookup(const char *nick, channel_type *channel,
		const char mode);

#endif /* AUTOMODE */

#endif /* _AUTOMODE_H */
