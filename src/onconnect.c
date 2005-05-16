/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2003-2005 Tommi Saviranta <wnd@iki.fi>
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

#include "onconnect.h"

#include "irc.h"


#ifdef ONCONNECT


llist_list	onconnect_actions;


void
onconnect_add(const char type, const char *target, const char *data)
{
	char	*msg;

	switch (type) {
		case 'p':
		case 'n':
			msg = (char *) xmalloc(strlen(target)
					+ strlen(data) + 11);
			sprintf(msg, "%s %s :%s",
					(type == 'p') ? "PRIVMSG" : "NOTICE",
					target, data);
			break;
		case 'r':
			msg = strdup(target);
			break;
		default:
			return;
	}

	llist_add_tail(llist_create(msg), &onconnect_actions);
} /* void onconnect_add(const int, const char, const char) */



void
onconnect_flush(void)
{
} /* void onconnect_flush(void) */



void
onconnect_do(void)
{
	LLIST_WALK_H(onconnect_actions.head, char *);
		irc_write(&c_server, data);
	LLIST_WALK_F;
} /* void onconnect_commit(void) */


#endif /* ONCONNECT */
