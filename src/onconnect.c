/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2003-2006 Tommi Saviranta <wnd@iki.fi>
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

#ifdef ONCONNECT

#include "onconnect.h"

#include "common.h"
#include "list.h"
#include "irc.h"

#include <stdio.h>
#include <string.h>




llist_list	onconnect_actions;


void
onconnect_add(const char type, const char *target, const char *data)
{
	char *msg;
	size_t mlen;

	switch (type) {
		case 'p':
		case 'n':
			/* if target/data are not ok, parser is broken */
			mlen = strlen(target) + strlen(data) + 11;
			msg = (char *) xmalloc(mlen);
			snprintf(msg, mlen, "%s %s :%s",
					(type == 'p') ? "PRIVMSG" : "NOTICE",
					target, data);
			msg[mlen - 1] = '\0';
			break;
		case 'r':
			msg = xstrdup(target);
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


#endif /* ifdef ONCONNECT */
