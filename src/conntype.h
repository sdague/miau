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

#ifndef _CONNTYPE_H
#define _CONNTYPE_H

#include <sys/types.h>
#include <sys/socket.h>

#define BUFFERSIZE	1024



typedef struct {
	int	socket;
	int	timer;
	char	buffer[BUFFERSIZE];
	int	offset;
} connection_type;



#endif /* _CONNTYPE_H */
