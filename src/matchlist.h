/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2004 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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


#ifndef MATCHLIST_H_
#define MATCHLIST_H_



#include <config.h>
#include "list.h"



list_type *matchlist_add(list_type *list, char *rule, void *state);
list_type *matchlist_flush(list_type *list, void (free_cb)(void *));
void *matchlist_get(list_type *list, const char *cand);
#ifdef DUMPSTATUS
const char *matchlist_dump(list_type *list);
#endif /* ifdef DUMPSTATUS */



#endif /* ifndef MATCHLIST_H_ */
