/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2004-2005 Tommi Saviranta <wnd@iki.fi>
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

#include "list.h"
#include "common.h"
#include "error.h"



#ifdef USE_MOVE_TO
static list_type *add_head_priv(list_type *list, list_type *node);
static list_type *add_tail_priv(list_type *list, list_type *node);
static list_type *delete_priv(list_type *list, list_type *node);
static list_type *insert_at_priv(list_type *list, list_type *pos,
		list_type *node);
#endif

#ifdef USE_POOL
static list_type *get_node();
static void free_node(list_type *node);



#define POOL_RESIZE	256
static list_type	*pool = NULL;
static list_type	**pool_free = NULL;
static int		pool_size = 0;
static int		free_ptr = 0;
#endif /* ifdef USE_POOL */



/*
 * Create node and add it to the beginning of list.
 * @list:	List to add data to
 * @data:	Stuff to add
 *
 * Returns: Pointer to list.
 */
list_type *
list_add_head(list_type *list, void *data)
{
	list_type *new;
	
#ifdef USE_POOL
	new = get_node();
#else /* ifdef USE_POOL */
	new = (list_type *) xmalloc(sizeof(list_type));
#endif /* ifdef else USE_POOL */
	new->data = data;
	
	new->prev = NULL;
	new->next = list;
	if (list == NULL) {
		new->last = new;
	} else {
		new->last = list->last;
		list->prev = new;
	}

	return new;
} /* list_type *list_add_head(list_type *list, void *data) */



/*
 * Create node and add it to the end of list.
 * @list:	List to add data to
 * @data:	Stuff to add
 *
 * Returns: Pointer to list.
 */
list_type *
list_add_tail(list_type *list, void *data)
{
	list_type *new;
	
#ifdef USE_POOL
	new = get_node();
#else /* ifdef USE_POOL */
	new = (list_type *) xmalloc(sizeof(list_type));
#endif /* ifdef else USE_POOL */
	new->data = data;
	
	new->next = NULL;
	
	if (list == NULL) {
		new->prev = NULL;
		new->last = new;
		return new;
	} else {
		new->prev = list->last;
		list->last->next = new;
		list->last = new;
		return list;
	}
} /* list_type *list_add_tail(list_type *list, void *data) */



/*
 * Inserts a node before specified position.
 * @list:	List to add data to
 * @dest:	Location where to insert
 * @data:	What to insert
 *
 * Returns: Pointer to list.
 *
 * If dest == NULL, node will be inserted as last.
 */
list_type *
list_insert_at(list_type *list, list_type *dest, void *data)
{
	list_type *new;

	if (dest == list) {
		return list_add_head(list, data);
	} else if (dest == NULL) {
		return list_add_tail(list, data);
	}
	
#ifdef USE_POOL
	new = get_node();
#else /* ifdef USE_POOL */
	new = (list_type *) xmalloc(sizeof(list_type));
#endif /* ifdef else USE_POOL */
	new->data = data;
	
	new->prev = dest->prev;
	new->next = dest;
	dest->prev->next = new;
	dest->prev = new;

	return list;
} /* list_type *list_insert_at(list_type *list, list_type *dest, void *data) */



#ifdef USE_MOVE_TO
/*
 * Insert a node to node in list.
 * @list: 	List to add data to
 * @node:	Location where to insert. If node == list, node is added as
 * 		the first element of the list. If node == NULL, node will
 * 		be added to the end of list. First rule implies that new data
 * 		will be inserted before pos.
 * @data: 	Stuff to add
 *
 * Returns: Pointer to list
 */
list_type *
list_insert_at(list_type *list, list_type *pos, void *data)
{
	list_type *new;

	/* Adding to head/tail? */
	if (pos == list) {
		return llist_add_head(list, data);
	} else if (pos == NULL) {
		return llist_add_tail(list, data);
	}

	/* Inserting for real. Proceed. */
#ifdef USE_POOL
	new = get_node();
#else /* ifdef USE_POOL */
	new = (list_type *) xmalloc(sizeof(llist_t));
#endif /* ifdef else USE_POOL */
	new->data = data;
	
	new->prev = pos->prev;
	new->next = pos;
	pos->prev->next = new;
	pos->prev = new;

	return list;
} /* list_type *list_insert_at(list_type *list, list_type *pos, void *data) */



/*
 * Move src to dest.
 * @list:	List where to move
 * @src:	Node to move
 * @dest:	Target location
 *
 * Returns: Pointer to list.
 *
 * If src == list, item will be moved as first item. If dest == NULL, item will
 * be moved as last timem. First rule implies that item is moved to location
 * just before dest.
 */
list_type *
list_move_to(list_type *list, list_type *src, list_type *dest)
{
	list_type *first;
	list_type *ptr;

	ptr = src;
	first = delete_priv(list, src);
	first = insert_at_priv(list, dest, ptr);

	return first;
} /* list_type *list_move_to(list_type *list, list_type *src,
		list_type *dest) */



/*
 * Insert a node to node in list (doesn't allocate memory).
 * @list: 	List to add data to
 * @node:	Location where to insert. If node == list, node is added as
 * 		the first element of the list. If node == NULL, node will
 * 		be added to the end of list. First rule implies that new data
 * 		will be inserted before pos.
 *
 * Returns: Pointer to list
 */
static list_type *
insert_at_priv(list_type *list, list_type *pos, list_type *node)
{
	/* Adding to head/tail? */
	if (pos == list) {
		return add_head_priv(list, node);
	} else if (pos == NULL) {
		return add_tail_priv(list, node);
	}

	
	node->prev = pos->prev;
	node->next = pos;
	pos->prev->next = node;
	pos->prev = node;

	return list;
} /* static list_type *insert_at_priv(list_type *list, list_type *pos) */
#endif



/*
 * Delete a node from list.
 * @list: list to remove from
 * @node: node to remove
 *
 * Returns: pointer to list
 *
 * Data should be freed before calling this function.
 */
list_type *
list_delete(list_type *list, list_type *node)
{
	list_type *first;

#ifdef ENDUSERDEBUG
	if (list == NULL) {
		enduserdebug("Trying to delete stuff from empty list");
		return NULL;
	}
#endif

	/* Update next-link of previous item. */
	if (node->prev != NULL) {
		node->prev->next = node->next;
	}

	/* Update prev-link of next item. */
	if (node->next != NULL) {
		node->next->prev = node->prev;
	}

	/* If deleted item was the first one, update new first item. */
	if (node == list) {
		if (node->next != NULL) {
			node->next->last = node->last;
			first = node->next;
		} else {
			first = NULL;
		}
	} else {
		first = list;
	}

	/* If deleted item was the last one, update new item. */
	if (node == list->last) {
		list->last = node->prev;
	}

#ifdef USE_POOL
	free_node(node);
#else /* ifdef USE_POOL */
	xfree(node);
#endif /* ifdef else USE_POOL */

	return first;
} /* list_type *list_delete(list_type *list, list_type *node) */



/*
 * Find node that points to data.
 * @list: list to look from
 * @data: data to look for
 *
 * Returns: node or NULL if data was not found.
 */
list_type *
list_find(list_type *list, void *data)
{
	list_type *ptr;

	for (ptr = list; ptr != NULL; ptr = ptr->next) {
		if (ptr->data == data) {
			return ptr;
		}
	}

	return NULL;
} /* list_type *list_find(list_type *list, void *data) */



list_type *
list_move_first_to(list_type *list, list_type *dest)
{
	list_type *new;
	list_type *moved;

/* printf("list=%p dest=%p\n", (void *) list, (void *) dest); */

	if (dest == list) {
		/* Moving item as item before the second item. */
		return list;
	} else if (list->next == dest) {
		/* Moving single item in list doesn't do anything. */
		return list;
	}
	
	moved = list;
	new = list->next;
	new->prev = NULL;

	if (dest != NULL) {
		new->last = moved->last;
		moved->prev = dest->prev;
		moved->next = dest;
		dest->prev->next = moved;
		dest->prev = moved;
	} else {
		new->last = moved;
		moved->last->next = moved;
		moved->prev = moved->last;
		moved->next = NULL;
	}

	return new;
} /* list_type *list_move_first_to(list_type *list, list_type *dest) */



#ifdef USE_POOL
static list_type *
get_node(void)
{
	if (free_ptr == 0) {
		int i;
		pool = (list_type *) xrealloc(pool, (pool_size + POOL_RESIZE)
				* sizeof(llist_t));
printf("pool now %p...%p\n", (void *) pool, (void *) (pool + pool_size + POOL_RESIZE - 1));
		/* The following is done exactly once! */
		if (pool_free == NULL) {
			pool_free = (list_type **) xmalloc(
					(POOL_RESIZE + POOL_RESIZE / 2)
					* sizeof(list_type *));
		}
		/* Update pool_free. */
		for (i = 0; i < POOL_RESIZE; i++) {
			pool_free[i] = pool + i + pool_size;
		}
		pool_size += POOL_RESIZE;
		free_ptr = POOL_RESIZE;
	}

	free_ptr--;
printf("use %p\n", (void *) pool_free[free_ptr]);
	return pool_free[free_ptr];
} /* static list_type *get_node(void) */



static void
free_node(list_type *node)
{
	pool_free[free_ptr] = node;
	free_ptr++;

	if (free_ptr >= POOL_RESIZE + POOL_RESIZE / 2) {
		pool_size -= POOL_RESIZE;
		pool = (list_type *) xrealloc(pool, pool_size * sizeof(llist_t));
		free_ptr -= POOL_RESIZE;
	}
} /* static void free_node(list_type *node) */



void
list_free(void)
{
	xfree(pool_free);
	xfree(pool);
} /* void list_free(void) */
#endif /* ifdef USE_POOL */



#ifdef DUMPSTATUS
#define LLIST_BUFSIZE	65536
const char *
list_dump(list_type *list)
{
	static char buf[LLIST_BUFSIZE];
	/*
	char *bufptr;
	list_type *ptr;

	printf("%p -- %p -----\n", (void *) buf, (void *) 0);
	bufptr = buf + snprintf(buf, LLIST_BUFSIZE, "list:\n");
	bufptr = buf;
	for (ptr = list; ptr != NULL; ptr = ptr->next) {
		bufptr += snprintf(bufptr, LLIST_BUFSIZE - (int) (buf - bufptr),
				"<-%p/%p/%p-> %p-->\n",
				(void *) ptr->prev, (void *) ptr,
				(void *) ptr->next, (void *) ptr->last);
	}
	*/
	buf[0] = '\0';
	return buf;
} /* const char *list_dump(list_type *list) */
#endif /* ifdef DUMPSTATUS */
