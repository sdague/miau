/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2003-2005 Tommi Saviranta <wnd@iki.fi>
 *	(C) 2002 Lee Hardy <lee@leeh.co.uk>
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

#ifndef CHANLOG_H_
#define CHANLOG_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#ifdef CHANLOG

#include "channels.h"
#include "llist.h"



#define MAX_CHANNELS 40


/* For channel logs. */
#define LOG_MESSAGE_C	'm'
#define LOG_MESSAGE	0x0001
#define LOG_JOIN_C	'j'
#define LOG_JOIN	0x0002
#define LOG_PART_C	'e'
#define LOG_PART	0x0004
#define LOG_QUIT_C	'q'
#define LOG_QUIT	0x0008
#define LOG_MODE_C	'c'
#define LOG_MODE	0x0010
#define LOG_NICK_C	'n'
#define LOG_NICK	0x0020
#define LOG_MISC_C	'o'
#define LOG_MISC	0x0040
#define LOG_MIAU_C	'b'
#define LOG_MIAU	0x0080

#define LOG_ALL_C	'a'
#define LOG_ALL		(LOG_MESSAGE | LOG_JOIN | LOG_PART | \
				LOG_QUIT | LOG_MODE | LOG_NICK | LOG_MISC | \
				LOG_MIAU)

#define LOG_ATTACHED_C	'A'
#define LOG_ATTACHED	0x4000
#define LOG_DETACHED_C	'D'
#define LOG_DETACHED	0x8000

#define LOG_CONTIN_C	'C'
#define LOG_CONTIN	(LOG_ATTACHED | LOG_DETACHED)



extern llist_list chanlog_list;
extern int global_logtype;


struct chanlogentry
{
	char	*channel;
	char	*filename;
	int	type;
};


void chanlog_add_rule(char *channel, char *file, int type);
void chanlog_del_rules(void);

void chanlog_open(channel_type *channel);
void chanlog_close(channel_type *channel);

void chanlog_write_entry(channel_type *chptr, char *format, ...);
void chanlog_write_entry_all(int type, char *format, ...);

int chanlog_has_log(const channel_type *chan, int type);

#endif /* ifdef CHANLOG */



#endif /* ifndef CHANLOG_H_ */
