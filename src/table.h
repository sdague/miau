/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2003-2005 Tommi Saviranta <wnd@iki.fi>
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

#ifndef TABLE_H_
#define TABLE_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include "etc.h"

#ifdef NEED_TABLE



void **table_add_item(void **data, int elementsize, int *entries, int *indx);
void **table_rem_item(void **data, int number, int *entries);
void **table_compact(void **data, int *entries);
void **table_free(void **data, int *entries, int clear);



#endif /* ifdef NEED_TABLE */

#endif /* ifndef TABLE_H_ */
