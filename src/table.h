/*
 * -------------------------------------------------------
 * Copyright (C) 2003 Tommi Saviranta <tsaviran@cs.helsinki.fi>
 *	(C) 1998-2000 Sebastian Kienzl <zap@riot.org>
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

#include <config.h>	/* Need to know what we need. */

#include "common.h"


#ifdef _NEED_TABLE


#ifndef _TABLE_H
#define _TABLE_H


void **add_item(void **data, int elementsize, int *entries, int *indx);
void **rem_item(void **data, int number, int *entries);
void **compact_table(void **data, int *entries);
void **free_table(void **data, int *entries, int clear);


#endif /* _TABLE_H */

#endif /* _NEED_TABLE */
