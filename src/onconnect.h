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

#ifndef _ONCONNECT_H
#define _ONCONNECT_H

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
void onconnect_flush();
void onconnect_do();


#endif /* ONCONNECT */

#endif /* _ONCONNECT_H */
