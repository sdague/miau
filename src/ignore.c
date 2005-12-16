/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2002-2005 Tommi Saviranta <wnd@iki.fi>
 *	(C) 1998-2002 Sebastian Kienzl <zap@riot.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#ifdef CTCPREPLIES

#include "ignore.h"
#include "common.h"
#include "table.h"
#include "etc.h"

#include <stdlib.h>



typedef struct {
	char	*hostname;
	int	ttl;
	int	type;
} ignore_type;

typedef struct {
	ignore_type	**data;
	int		amount;
} ignores_type;


ignores_type ignores;



void
ignore_add(char *hostname, int ttl, int type)
{
	int i, indx;

	if (hostname == NULL) return;

	/* Already in table ? */
	for (i = 0; i < ignores.amount; i++) {
		if (ignores.data[i] != NULL
				&& xstrcasecmp(ignores.data[i]->hostname,
					hostname) == 0
				&& ignores.data[i]->type == type) {
			return;
		}
	}
	
	ignores.data = (ignore_type **) table_add_item((void **) ignores.data,
			sizeof(ignore_type), &ignores.amount, &indx);
	ignores.data[indx]->hostname = xstrdup(hostname);
	ignores.data[indx]->ttl = ttl;
	ignores.data[indx]->type = type;
} /* void ignore_add(char *hostname, int ttl, int type) */



void
ignore_del(char *hostname)
{
	int i;
	for (i = 0; i < ignores.amount; i++) {
		if (ignores.data[i] != NULL
				&& (xstrcasecmp(ignores.data[i]->hostname,
						hostname) == 0)) {
			xfree(ignores.data[i]->hostname);
			ignores.data = (ignore_type **)
				table_rem_item((void **) ignores.data,
						i, &ignores.amount);
		}
	}
} /* void ignore_del(char *hostname) */



static void
ignore_del_by_number(int i)
{
	if (i < ignores.amount && ignores.data[i] != NULL) {
		xfree(ignores.data[i]->hostname);
		ignores.data = (ignore_type **)
			table_rem_item((void **) ignores.data,
					i, &ignores.amount);
	}
} /* void ignore_del_by_number(int i) */



void
ignores_process(void)
{
	int i;
	for (i = 0; i < ignores.amount; i++) {
		if (ignores.data[i] != NULL) {
			if (ignores.data[i]->ttl != 0) {	/* > 0 */
				ignores.data[i]->ttl--;
			} else {
				ignore_del_by_number(i);
			}
		}
	}
} /* void ignores_process(void) */



int
is_ignore(char *hostname, int type)
{
	int i;
	for (i = 0; i < ignores.amount; i++) {
		if (ignores.data[i] != NULL
				&& (xstrcasecmp(ignores.data[i]->hostname,
						hostname) == 0)
				&& (ignores.data[i]->type == type)) {
			return 1;
		}
	}
	return 0;
} /* int is_ignore(char *hostname, int type) */



#endif /* ifdef CTCPREPLIES */
