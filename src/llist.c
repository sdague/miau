/*
 * -------------------------------------------------------
 * Copyright (C) 2002-2004 Tommi Saviranta <tsaviran@cs.helsinki.fi>
 *	(C) 2002 Lee Hardy <lee@leeh.co.uk>
 * * -------------------------------------------------------
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

#include "llist.h"
#include "common.h"



/*
 * Create a new node with no links to other nodes.
 *
 * Return pointer to this node.
 */
llist_node *
llist_create(
		void	*data
	    )
{
	llist_node	*node;

	node = (llist_node *) xmalloc(sizeof(llist_node));
	node->data = data;
	/*
	 * Basically we wouldn't have to do this...
	node->next = NULL;
	node->prev = NULL;
	 */

	return node;
} /* llist_node *llist_create(void *) */



/*
 * Add node to the beginning of the list.
 */
void
llist_add(
		llist_node	*node,
		llist_list	*list
	 )
{
	/* New node has no previous node. */
	node->next = list->head;
	node->prev = NULL;

	if (list->head != NULL) {
		list->head->prev = node;
	} else if (list->tail == NULL) {
		list->tail = node;
	}

	list->head = node;
} /* void llist_add(llist_node *, llist_list *) */



/*
 * Add node to the end of list.
 */
void
llist_add_tail(
		llist_node	*node,
		llist_list	*list
	      )
{
	node->next = NULL;
	node->prev = list->tail;
  
	if (list->head == NULL) {
		list->head = node;
	} else if (list->tail != NULL) {
		list->tail->next = node;
	}

	list->tail = node;
} /* void llist_add_tail(llist_node *, llist_list *) */



/*
 * Delete links to node and free memory allocated by node.
 *
 * Note that user _must_ free memory that was possibly allocated in
 * node->data.
 */
void
llist_delete(
		llist_node	*node,
		llist_list	*list
	    )
{
	/* Item is at head. */
	if (node->prev == NULL) {
		list->head = node->next;
	} else {
		node->prev->next = node->next;
	}

	/* Item is at tail. */
	if (node->next == NULL) {
		list->tail = node->prev;
	} else {
		node->next->prev = node->prev;
	}

	/*
	 * Free some resources. Use _must_ free m->data _before_ calling
	 * llist_delete.
	 */
	xfree(node);
} /* void llist_delete(llist_node *, llist_list *) */



/*
 * Find node that points to data.
 *
 * Return pointer to this node.
 */
llist_node *
llist_find(
		void		*data,
		llist_list	*list
	  )
{
	llist_node	*ptr;

	for (ptr = list->head; ptr; ptr = ptr->next) {
		if (ptr->data == data) {
			return ptr;
		}
	}
	
	return NULL;
} /* llist_node *llist_find(void *, llist_list *) */



#ifdef OBSOLETE	/* Ignore this. */
/*
 * We don't want to use this - it just takes more space that doind it by
 * hand time after time.
 */
void
llist_empty(
		llist_list	*list,
		void		(* action) (void *)
	   )
{
	llist_node	*node = list->head;
	llist_node	*nextnode;

	while (node != NULL) {
		nextnode = node->next;
		action(node->data);
		llist_delete(node, list);
		node = nextnode;
	}
} /* void llist_empty(llist_list *, void (* action) (void *) */



llist_node **
llist_get_indexed(
		const llist_list	*list
		)
{
	llist_node	**ret = xmalloc(sizeof(llist_node *));
	llist_node	*ptr = list->head;
	int		size = 0;

	do {
		if (ptr != NULL) {
			ret = xrealloc(ret, sizeof(llist_node *) * (size + 2));
			ret[size] = ptr;
			size++;
			ptr = ptr->next;
		}
		ret[size] = NULL;
	} while (ptr != NULL);

	return ret;
} /* llist_node **llist_get_indexed(const llist_list *) */
#endif /* OBSOLETE */
