/* $Id */
/* gcc -DTESTING -DDUMPSTATUS -g -I.. -Wall -Werror test_matchlist.c matchlist.c match.c list.c tools.c common.c error.c && valgrind --leak-check=yes --leak-resolution=high --num-callers=512 --show-reachable=yes ./a.out */

#include <stdio.h>
#include "matchlist.h"



#ifdef DUMP
#define DUMP_POINTERS(_a_) \
	{ \
		list_type *ptr; \
		printf("list = %p\n", (void *) _a_); \
		for (ptr = _a_; ptr != NULL; ptr = ptr->next) { \
			printf("\t<-%p ->%p l=%p\n", (void *) ptr->prev, \
					(void *) ptr->next, \
					(void *) ptr->last); \
		} \
		printf("\n"); \
	}
#else
#define DUMP_POINTERS(_a_)
#endif



int
main(
    )
{
	list_type *list;

	list = NULL;

	printf("add to list:\n");
	list = matchlist_add(list, "#debian*", "UTF-8");
	list = matchlist_add(list, "#*.fi", "ISO-8859-15");
	list = matchlist_add(list, "#*.jp", "EUC");
	list = matchlist_add(list, "#foobar", "KOR");
	list = matchlist_add(list, "*", "ISO-8859-1");
	printf("%s", matchlist_dump(list));

	printf("matching...:\n");
#define MATCH(_a_) { printf("\t%s: %s\n", _a_, \
		(char *) matchlist_get(list, _a_)); }
	MATCH("#foo.fi");
	MATCH("#foo.jp");
	MATCH("#debian.fi");
	MATCH("#debian.jp");
	MATCH("#tko-äly");
	MATCH("#serveri");
	MATCH("#foobar");

	/* Free rules. */
	list = matchlist_flush(list, NULL);

	return 0;
}
