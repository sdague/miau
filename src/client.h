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

#ifndef CLIENT_H_
#define CLIENT_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include "channels.h"
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



#if 0
#include "conntype.h"
#include "messages.h"
#ifdef QUICKLOG
#include "qlog.h"
#endif /* QUICKLOG */
#include "tools.h"
#endif


enum {
	DISCONNECT_ERROR = 1,
	DISCONNECT_REPORT = 2,
	DISCONNECT_DYING = 9
};
void client_drop(connection_type *client, char *reason, const int error,
		const int echo, const char *);
int client_read(connection_type *client);
void client_free(void);



/* export global stuff */
extern client_info	i_client;
extern clientlist_type	c_clients;



#endif /* ifndef CLIENT_H_ */
