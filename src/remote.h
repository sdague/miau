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



#ifndef _REMOTE_H
#define _REMOTE_H



#include <config.h>
#include "miau.h"



#ifdef _NEED_CMDPASSWD
int remote_cmd(char *command, char *params, char *nick, char *hostname);
#endif /* _NEED_CMDPASSWD */



#endif /* _REMOTE_H */
