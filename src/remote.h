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



#ifndef REMOTE_H_
#define REMOTE_H_



#include <config.h>
#include "miau.h"



#ifdef _NEED_CMDPASSWD
int remote_cmd(char *command, char *params, char *nick);
#endif /* _NEED_CMDPASSWD */



#endif /* ifndef REMOTE_H_ */
