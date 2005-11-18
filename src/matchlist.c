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

#include <config.h>
#include "matchlist.h"
#include "match.h"
#include "common.h"
#include "list.h"

#include <stdio.h>
#include <string.h>



typedef struct _match_type	match_type;



union ptr_int {
	void	*ptr;
	int	i;
};

struct _match_type {
	char		*rule;
	union ptr_int	state;
};


/*
 * Add entry to matchlist.
 * @list: list where to add
 * @rule: rule to match stuff against
 * @state: data for this match
 *
 * Returns: Pointer to new matchlist.
 *
 * Rule is duplicated in the struct, state is not. This means state must be
 * freed somehow when flushing the list. Fortunately, there's an easy way
 * to do this. :-)
 */
list_type *
matchlist_add(list_type *list, char *rule, void *state)
{
	match_type *match;

	match = (match_type *) xmalloc(sizeof(match_type));
	match->rule = xstrdup(rule);
	match->state.ptr = state;

	return list_add_tail(list, match);
} /* list_type *matchlist_add(list_type *list, char *rule, void *state) */



/*
 * Clear matchlist.
 * @list: list to clear
 *
 * Returns: pointer to list
 */
list_type *
matchlist_flush(list_type *list, void (free_cb)(void *))
{
	match_type *data;
	list_type *ptr;

	/* First free data inside the list. */
	for (ptr = list; ptr != NULL; ptr = ptr->next) {
		data = (match_type *) ptr->data;
		xfree((void *) data->rule);
		if (free_cb != NULL) {
			(*free_cb)(data->state.ptr);
		}
		xfree(ptr->data);
	}

	LIST_CLEAR(list);

	return list;
} /* llist_t *matchlist_flush(llist_t *list, void (free_cb)(void *)) */



/*
 * Get match from matchlist.
 * @list: list
 * @cand: stuff to match
 *
 * Returns: pointer to data, (void *) -1 if nothing found.
 */
void *
matchlist_get(list_type *list, const char *cand)
{
	match_type	*data;
	list_type	*ptr;

	for (ptr = list; ptr != NULL; ptr = ptr->next) {
		data = (match_type *) ptr->data;
		if (match(cand, data->rule) == 1) {
			return data->state.ptr;
		}
	}

	return (void *) -1;
} /* void *matchlist_get(list_type *list, const char *cand) */



#ifdef DUMPSTATUS
#define BUFSIZE	65536
const char *
matchlist_dump(list_type *list)
{
	static char	buf[BUFSIZE];
	match_type	*data;
	list_type	*ptr;
	int		len, t;

	buf[0] = buf[BUFSIZE - 1] = '\0';
	len = (int) strlen(buf);
	for (ptr = list; ptr != NULL; ptr = ptr->next) {
		data = (match_type *) ptr->data;
		/* This is dangerous, but no-one has to call this function! */
		if ((int) data->state.i == 0 || (int) data->state.i == 1) {
			t = snprintf(buf + len, BUFSIZE - len - 1,
					"    '%s' = %d\n",
					data->rule, (int) data->state.i);
		} else {
			t = snprintf(buf + len, BUFSIZE - len - 1,
					"    '%s' = '%s'\n",
					data->rule, (char *) data->state.ptr);
		}
		buf[BUFSIZE - 1] = '\0';
		if (t < 0) {
			return buf;
		}
		len += t;
	}

	return buf;
} /* const char *matchlist_dump(list_type *list) */
#endif /* ifdef DUMPSTATUS */
