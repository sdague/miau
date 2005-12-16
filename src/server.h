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



#ifndef SERVER_H_
#define SERVER_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include "llist.h"
#include "conntype.h"



typedef struct {
	char	*name;
	int	port;
	char	*password;
	int	timeout;
	int	working;
} server_type;


typedef struct {
	llist_list	servers;
	int		amount;
	int		fresh;
} serverlist_type;


typedef struct {
	int		connected;
	char		*realname;
	char		*greeting[4];	/* RPL_SERVERVER_LEN */
	char		*isupport[3];	/* RPL_ISUPPORT_LEN */
	llist_node	*current;	/* Node of current server. */
} server_info;



void server_drop(char *reason);
void server_set_fallback(const llist_node *safenode);
void server_reset(void);
void server_next(const int disablecurrent);
int server_read(void);
void server_commands(char *command, char *param, int *pass);
int parse_privmsg(char *param1, char *param2, char *nick, char *hostname,
		const int cmdindex, int *pass);
int server_read(void);
void server_check_list(void);
void server_reply(const int command, char *original, char *origin,
		char *param1, char *param2, int *pass);
void parse_modes(const char *channel, const char *original);


extern serverlist_type	servers;
extern server_info	i_server;
extern connection_type	c_server;



#endif /* ifndef SERVER_H_ */
