/*
 * -------------------------------------------------------
 * Copyright (C) 2002-2004 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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


#ifndef _PERM_H
#define _PERM_H

#include <config.h>



typedef struct {
	char	*name;
	int	allowed;
} perm_type;


typedef struct {
	llist_list	list;
	int		amount;
} permlist_type;


void add_perm(permlist_type *table, char *name, const int allowed);
void empty_perm(permlist_type *table);
int is_perm(permlist_type *table, char *name);


extern permlist_type connhostlist;
extern permlist_type ignorelist;
extern permlist_type automodelist;



#ifdef DUMPSTATUS
char *perm_dump(permlist_type *);
#endif /* DUMPSTATUS */



#endif /* _PERM_H */
