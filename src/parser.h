/*
 * -------------------------------------------------------
 * Copyright (C) 2003-2004 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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

#ifndef _PARSER_H
#define _PARSER_H

#include <config.h>

#define MAXNUMOFPARAMS	4

#define CFG_INVALID	-1
#define CFG_NOLIST	0
#define CFG_NICKNAMES	1
#define CFG_SERVERS	2
#define CFG_CONNHOSTS	3
#define CFG_IGNORE	4
#define CFG_AUTOMODELIST	5
#define CFG_LOG		6
#define CFG_CHANNELS	7
#define CFG_ONCONNECT	8

#define SPACES		0
#define LINE		1

#define READBUFSIZE	16384



void add_server(const char *name, int port, const char *pass, int timeout);
int parse_cfg(const char *cfgfile);



#endif /* _PARSER_H */
