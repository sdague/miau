/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2002-2005 Tommi Saviranta <wnd@iki.fi>
 *	(C) 2002 Lee Hardy <lee@leeh.co.uk>
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


#ifndef LLIST_H_
#define LLIST_H_
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */



typedef struct _llist_node llist_node;
typedef struct _llist_list llist_list;

struct _llist_node
{
	void		*data;
	llist_node	*next;
	llist_node	*prev;
};

struct _llist_list
{
	llist_node	*head;
	llist_node	*tail;
};

llist_node *llist_create(void *data);

void llist_add(llist_node *node, llist_list *list);
void llist_add_tail(llist_node *node, llist_list *list);
void llist_delete(llist_node *node, llist_list *list);
llist_node *llist_find(void *data, llist_list *list);
llist_node **llist_get_indexed(const llist_list *list);

#define LLIST_EMPTY(firstnode, list) \
{ \
	llist_node	*node = (firstnode); \
	llist_node	*nextnode; \
	while (node != NULL) { \
		nextnode = node->next; \
		xfree(node->data); \
		llist_delete(node, (list)); \
		node = nextnode; \
	} \
}



/*
 * Following macros can be used to walk thru the list even when elements
 * will be removed during it.
 *
 * Example:
 * LLIST_WALK_H(nicknames.nicks.head, char *);
 * xfree(data);
 * LLIST_WALK_F;
 *
 * Using these macros, "node" points to current node and "data" to node->data.
 */
#define LLIST_WALK_H(firstitem, datatype) { \
	datatype	data; \
	llist_node	*node; \
	llist_node	*nextnode; \
	node = (firstitem); \
	while (node != NULL) { \
		nextnode = node->next; \
		data = (datatype) node->data;
#define LLIST_WALK_F \
		node = nextnode; \
	} }
#define LLIST_WALK_CONTINUE \
		node = nextnode; \
		continue;
/* Setting nextnode to NULL is just a pre-caution. */
#define LLIST_WALK_BREAK \
		nextnode = NULL; \
		break;



/*
 * Not worth the trouble (bytes) to do this:
void llist_empty(llist_list *list, void (* action) (void *));
*/


#endif /* LLIST_H_ */
