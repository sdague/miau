/*
 * -------------------------------------------------------
 * Copyright (C) 2003 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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

#ifndef _CLIENT_H
#define _CLIENT_H



#include <config.h>

#include "llist.h"



typedef struct {
	int	connected;
	char	*nickname;
	char	*username;
	char	*hostname;
} client_info;


typedef struct {
	int		connected;
	llist_list	*clients;
} clientlist_type;



#include "channels.h"
#include "conntype.h"
#include "messages.h"
#ifdef QUICKLOG
#include "qlog.h"
#endif /* QUICKLOG */
#include "tools.h"



#define ERROR	1
#define REPORT	2
#define DYING	9
void client_drop(connection_type *client, char *reason, const int error,
		const int echo, const char *);
int client_read(connection_type *client);
void client_free();



extern client_info	i_client;
extern clientlist_type	c_clients;



#endif /* _CLIENT_H */
