/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2002-2005 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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



#ifndef LIST_H_
#define LIST_H_



typedef struct _list_type list_type;

struct _list_type
{
	void		*data;		/* Payload. */
	list_type	*prev;		/* Link to previous item. */
	list_type	*next;		/* Link to next item. */
	list_type	*last;		/* Link to last item. */
};



#define LIST_CLEAR(_list_) \
	{ \
		list_type *__list__0; \
		__list__0 = _list_; \
		while (__list__0 != NULL) { \
			__list__0 = list_delete(__list__0, __list__0); \
		} \
		_list_ = NULL; \
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
#define LIST_WALK_H(firstitem, datatype) { \
	datatype	data; \
	list_type	*node; \
	list_type	*nextnode; \
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



list_type *list_add_head(list_type *list, void *data);
list_type *list_add_tail(list_type *list, void *data);
list_type *list_delete(list_type *list, list_type *node);
list_type *list_find(list_type *list, void *data);
list_type *list_move_first_to(list_type *list, list_type *dest);
list_type *list_insert_at(list_type *list, list_type *dest, void *data);
/*
list_type *list_move_to(list_type *list, list_type *src, list_type *dest);
*/

#ifdef USE_POOL
void list_free();
#endif /* ifdef USE_POOL */

#ifdef DUMPSTATUS
const char *list_dump(list_type *list);
#endif /* DUMPSTATUS */



#endif /* ifndef LIST_H_ */
