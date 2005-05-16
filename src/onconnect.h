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

#ifndef ONCONNECT_H_
#define ONCONNECT_H_

#include <config.h>
#include "miau.h"
#include "llist.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>



#ifdef ONCONNECT



llist_list	onconnect_actions;


void onconnect_add(const char type, const char *target, const char *data);
void onconnect_flush(void);
void onconnect_do(void);


#endif /* ONCONNECT */

#endif /* ONCONNECT_H_ */
