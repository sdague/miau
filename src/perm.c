/*
 * -------------------------------------------------------
 * Copyright (C) 2002-2003 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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

#include "miau.h"
#include "perm.h"
#include "match.h"
#include "table.h"



/*
 * Add entry to permlist.
 *
 * Skip adding if parameters are invalid.
 */
void
add_perm(
		permlist_type	*list,
		char		*name,
		const int	allowed
	)
{
	perm_type	*perm;
	llist_node	*node;

	/* Don't allow permissions without name (identifier). */
	if (! name)	return;

	perm = (perm_type *) xmalloc(sizeof(perm_type));
	perm->name = name;
	perm->allowed = allowed;
	node = llist_create(perm);
	llist_add_tail(node, &list->list);
} /* void add_perm(permlist_type *, char *, const int) */



void
empty_perm(
		permlist_type	*list
	  )
{
	llist_node	*node;
	llist_node	*next;

	node = list->list.head;
	while (node != NULL) {
		next = node->next;
		xfree(((perm_type *) node->data)->name);
		xfree(node->data);
		llist_delete(node, &list->list);
		node = next;
	}
} /* void empty_perm(permlist_type *) */



/*
 * Is given mask permitted to ?
 */
int
is_perm(
		permlist_type	*list,
		char		*name
       )
{
	llist_node	*node;
	int		allowed = 0;

	for (node = list->list.head; node != NULL; node = node->next) {
		if (match(name, ((perm_type *) node->data)->name)) {
			allowed = ((perm_type *) node->data)->allowed;
		}
	}

	return allowed;
} /* int is_perm(permlist_type *, char *name) */



#ifdef DUMPSTATUS
char *
perm_dump(
		permlist_type	*list
	 )
{
#define BUFSIZE	65536
	static char buf[BUFSIZE];
	int len, t;
	llist_node	*node;

	buf[0] = '\0';
	strcat(buf, "    ");
	len = strlen(buf);
	for (node = list->list.head; node != NULL; node = node->next) {
		t = strlen(((perm_type *) node->data)->name);
		if (len + t + 10 < BUFSIZE) {
			len += t + 3;
			strcat(buf, ((perm_type *) node->data)->name);
			strcat(buf, "=  ");
			buf[len - 2] = '0' + ((perm_type *)
					node->data)->allowed;
			buf[len] = '\0';
		} else {
			return buf;
		}
	}

	return buf;
} /* char *perm_dump(permlist_type *) */
#endif /* DUMPSTATUS */
