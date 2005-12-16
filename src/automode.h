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

#ifndef AUTOMODE_H
#define AUTOMODE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#ifdef AUTOMODE

#include "llist.h"
#include "channels.h"



#define ANY_MODE	'\0'

typedef struct {
	char	*nick;		/* Nick to be mode'd */
	char	mode;		/* Mode this nick. */
} automode_type;



void automode_do(void);
void automode_queue(const char *nick, const char *hostname,
		channel_type *channel);
void automode_clear(llist_list *queue);
void automode_drop_nick(const char *nick, const char mode);
void automode_drop_channel(channel_type *channel, const char *nick,
		const char mode);
llist_node *automode_lookup(const char *nick, channel_type *channel,
		const char mode);



#endif /* ifdef AUTOMODE */

#endif /* ifndef AUTOMODE_H_ */
