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

#ifndef PARSER_H_
#define PARSER_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */



#define MAXNUMOFPARAMS	4

enum {
	CFG_INVALID = -1,
	CFG_NOLIST,
	CFG_NICKNAMES,
	CFG_SERVERS,
	CFG_CONNHOSTS,
	CFG_IGNORE,
	CFG_AUTOMODELIST,
	CFG_CHANLOG,
	CFG_CHANNELS,
	CFG_ONCONNECT
};


#define SPACES		0
#define LINE		1

#define READBUFSIZE	65536



void add_server(const char *name, int port, const char *pass, int timeout);
int parse_cfg(const char *cfgfile);



#endif /* ifdef PARSER_H_ */
