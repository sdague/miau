/* $Id */
/* gcc -DTESTING -g -I.. -Wall -Werror test_list.c list.c common.c && valgrind --leak-check=yes --show-reachable=yes ./a.out */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"



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

#define EMPTY_LIST(_a_) \
	{ \
		while (_a_ != NULL) { \
			_a_ = list_delete(_a_, _a_); \
		} \
	}

#define TEST_INIT(_a_, _b_) \
	char __ret[65536]; \
	char *__retp; \
	char __exp[] = _b_; \
	__retp = __ret; \
	printf("%s: ", _a_); fflush(stdout);

#define TEST_FINISH(_a_) \
	{ \
		list_type *ptr; \
		for (ptr = _a_; ptr != NULL; ptr = ptr->next) { \
			*__retp++ = (char) ((int) ptr->data); \
		} \
		*__retp = '\0'; \
		if (strcmp(__ret, __exp) != 0) { \
			printf("failed!\nExpected: '%s'\n     Got: '%s'\n", \
					__exp, __ret); \
		} else { \
			printf("ok\n"); \
		} \
		while (_a_ != NULL) { \
			_a_ = list_delete(_a_, _a_); \
		} \
	}



list_type *
create_testing(
	      )
{
	list_type *list;
	list = NULL;
	list = list_add_tail(list, (void *) 't'); DUMP_POINTERS(list);
	list = list_add_tail(list, (void *) 'e'); DUMP_POINTERS(list);
	list = list_add_tail(list, (void *) 's'); DUMP_POINTERS(list);
	list = list_add_tail(list, (void *) 't'); DUMP_POINTERS(list);
	list = list_add_tail(list, (void *) 'i'); DUMP_POINTERS(list);
	list = list_add_tail(list, (void *) 'n'); DUMP_POINTERS(list);
	list = list_add_tail(list, (void *) 'g'); DUMP_POINTERS(list);

	return list;
}



void
test_list_add_tail(
		)
{
	list_type *list;
	TEST_INIT("list_add_tail", "testing");

	list = NULL;
	list = list_add_tail(list, (void *) 't'); DUMP_POINTERS(list);
	list = list_add_tail(list, (void *) 'e'); DUMP_POINTERS(list);
	list = list_add_tail(list, (void *) 's'); DUMP_POINTERS(list);
	list = list_add_tail(list, (void *) 't'); DUMP_POINTERS(list);
	list = list_add_tail(list, (void *) 'i'); DUMP_POINTERS(list);
	list = list_add_tail(list, (void *) 'n'); DUMP_POINTERS(list);
	list = list_add_tail(list, (void *) 'g'); DUMP_POINTERS(list);
	TEST_FINISH(list);
}



void
test_list_add_head(
		)
{
	list_type *list;
	TEST_INIT("list_add_head", "testing");

	list = NULL;
	list = list_add_head(list, (void *) 'g'); DUMP_POINTERS(list);
	list = list_add_head(list, (void *) 'n'); DUMP_POINTERS(list);
	list = list_add_head(list, (void *) 'i'); DUMP_POINTERS(list);
	list = list_add_head(list, (void *) 't'); DUMP_POINTERS(list);
	list = list_add_head(list, (void *) 's'); DUMP_POINTERS(list);
	list = list_add_head(list, (void *) 'e'); DUMP_POINTERS(list);
	list = list_add_head(list, (void *) 't'); DUMP_POINTERS(list);
	TEST_FINISH(list);
}



void
test_list_delete_first(
		)
{
	list_type *list;
	TEST_INIT("list_delete (first)", "ting");
	list = create_testing();

	list = list_delete(list, list);
	list = list_delete(list, list);
	list = list_delete(list, list);
	TEST_FINISH(list);
}



void
test_list_delete_middle(
		)
{
	list_type *list;
	list_type *ptr;
	TEST_INIT("list_delete (middle)", "tstg");
	list = create_testing();

	ptr = list_find(list, (void *) 'i');
	list = list_delete(list, ptr);
	ptr = list_find(list, (void *) 'e');
	list = list_delete(list, ptr);
	ptr = list_find(list, (void *) 'n');
	list = list_delete(list, ptr);
	TEST_FINISH(list);
}



void
test_list_delete_last(
		)
{
	list_type *list;
	TEST_INIT("list_delete (last)", "test");
	list = create_testing();

	list = list_delete(list, list->last);
	list = list_delete(list, list->last);
	list = list_delete(list, list->last);
	TEST_FINISH(list);
}



void
test_list_find_first(
		)
{
	list_type *list;
	list_type *ptr;
	printf("list_find (first): ");
	list = create_testing();

	ptr = list_find(list, (void *) 't');
	if (ptr == list) {
		printf("ok\n");
	} else {
		printf("failed!\nGot %p, expected %p!\n",
				(void *) ptr, (void *) list);
	}
	EMPTY_LIST(list);
}



void
test_list_find_last(
		)
{
	list_type *list;
	list_type *ptr;
	printf("list_find (last): ");
	list = create_testing();

	ptr = list_find(list, (void *) 'g');
	if (ptr == list->last) {
		printf("ok\n");
	} else {
		printf("failed!\nGot %p, expected %p!\n",
				(void *) ptr, (void *) list->last);
	}
	EMPTY_LIST(list);
}



void
test_list_find_middle(
		)
{
	list_type *list;
	list_type *ptr;
	printf("list_find (middle): ");
	list = create_testing();

	ptr = list_find(list, (void *) 'i');
	if (ptr == list->last->prev->prev) {
		printf("ok\n");
	} else {
		printf("failed!\nGot %p, expected %p!\n",
				(void *) ptr, (void *) list->last->prev->prev);
	}
	EMPTY_LIST(list);
}



void
test_list_insert_at_first(
		)
{
	list_type *list;
	TEST_INIT("list_insert_at (first)", "XXXtesting");
	list = create_testing();

	list = list_insert_at(list, list, (void *) 'X');
	list = list_insert_at(list, list, (void *) 'X');
	list = list_insert_at(list, list, (void *) 'X');
	TEST_FINISH(list);
}



void
test_list_insert_at_last(
		)
{
	list_type *list;
	TEST_INIT("list_insert_at (first)", "testingXXX");
	list = create_testing();

	list = list_insert_at(list, NULL, (void *) 'X');
	list = list_insert_at(list, NULL, (void *) 'X');
	list = list_insert_at(list, NULL, (void *) 'X');
	TEST_FINISH(list);
}



void
test_list_insert_at_middle(
		)
{
	list_type *list;
	list_type *ptr;
	TEST_INIT("list_insert_at (middle)", "teXstXiXng");
	list = create_testing();

	ptr = list_find(list, (void *) 'i');
	list = list_insert_at(list, ptr, (void *) 'X');
	ptr = list_find(list, (void *) 's');
	list = list_insert_at(list, ptr, (void *) 'X');
	ptr = list_find(list, (void *) 'n');
	list = list_insert_at(list, ptr, (void *) 'X');
	TEST_FINISH(list);
}



#ifdef USE_MOVE_TO
void
test_list_move_to_first_to(
		)
{
	list_type *list;
	list_type *ptr;
	TEST_INIT("list_move_to (-> first)", "ntestig");
	list = create_testing();

	ptr = list_find(list, (void *) 'n');
	list = list_move_to(list, ptr, list);
	TEST_FINISH(list);
}



void
test_list_move_to_last_to(
		)
{
	list_type *list;
	list_type *ptr;
	TEST_INIT("list_move_to (-> last)", "testign");
	list = create_testing();

	ptr = list_find(list, (void *) 'n');
	list = list_move_to(list, ptr, NULL);
	TEST_FINISH(list);
}



void
test_list_move_to_middle(
		)
{
	list_type *list;
	list_type *ptr0, *ptr1;
	TEST_INIT("list_move_to (middle)", "tenstig");
	list = create_testing();

	ptr0 = list_find(list, (void *) 'n');
	ptr1 = list_find(list, (void *) 's');
	list = list_move_to(list, ptr0, ptr1);
	TEST_FINISH(list);
}



void
test_list_move_to_first_from(
		)
{
	list_type *list;
	list_type *ptr;
	TEST_INIT("list_move_to (first ->)", "estitng");
	list = create_testing();

	ptr = list_find(list, (void *) 'n');
	list =llist_move_to(list, list, ptr);
	TEST_FINISH(list);
}



void
test_list_move_to_last_from(
		)
{
	list_type *list;
	list_type *ptr;
	TEST_INIT("list_move_to (last ->)", "tegstin");
	list = create_testing();

	ptr = list_find(list, (void *) 's');
	list = list_move_to(list, NULL, ptr);
	TEST_FINISH(list);
}



void
test_list_move_to_first_to_last(
		)
{
	list_type *list;
	TEST_INIT("list_move_to (first -> last)", "estingt");
	list = create_testing();

	list = list_move_to(list, list, NULL);
	TEST_FINISH(list);
}



void
test_list_move_to_last_to_first(
		)
{
	list_type *list;
	TEST_INIT("list_move_to (last -> first)", "gtestin");
	list = create_testing();

	list = list_move_to(list, NULL, list);
	TEST_FINISH(list);
}
#endif



void
test_list_move_first_to_first(
		)
{
	list_type *list;
	
	TEST_INIT("list_move_first_to (first)", "testing");
	list = create_testing();

	list = list_move_first_to(list, list);
	list = list_move_first_to(list, list);
	list = list_move_first_to(list, list);
	TEST_FINISH(list);
}



void
test_list_move_first_to_middle(
		)
{
	list_type *list;
	list_type *ptr;
	
	TEST_INIT("list_move_first_to (middle)", "tsitneg");
/*	TEST_INIT("list_move_first_to (middle)", "estitng"); */
	list = create_testing();

	ptr = list_find(list, (void *) 'n');
	list = list_move_first_to(list, ptr);
printf("%s", list_dump(list));
	ptr = list_find(list, (void *) 'g');
	list = list_move_first_to(list, ptr);
printf("%s", list_dump(list));
	ptr = list_find(list, (void *) 'i');
	list = list_move_first_to(list, ptr);
printf("%s", list_dump(list));
	TEST_FINISH(list);
}



void
test_list_move_first_to_last(
		)
{
	list_type *list;
	
	TEST_INIT("list_move_first_to (last)", "tingtes");
	list = create_testing();

	list = list_move_first_to(list, NULL);
	list = list_move_first_to(list, NULL);
	list = list_move_first_to(list, NULL);
	TEST_FINISH(list);
}



void
test_list_speed(
		int	n,
		int	s
	       )
{
	int i;
	int l;
	int m;
	int t;
	int r;
	list_type *list;
	
	list = NULL;
	m = l = r = 0;
	srand((unsigned int) s);
	
	while (n > r) {
		t = rand() & 0xff;
		if (n < r) {
			r = n;
		}
		
		if (rand() & 1) {
			l += t;
			if (l > m) {
				m = l;
			}
			for (i = 0; i < t; i++) {
				list = list_add_tail(list, (void *) 0);
			}
		} else {
			if (t > l) {
				t = l;
			}
			for (i = 0; i < t; i++) {
				list = list_delete(list, list);
			}
			l -= t;
		}
		r += t;
	}

	printf("maximum length: %d\n", m);

	while (list != NULL) {
		list = list_delete(list, list);
	}
} /* void test_list_speed(int, int) */



int
main(
		int	argc,
		char	**argv
    )
{
	if (argc == 3) {
		test_list_speed(atoi(argv[1]), atoi(argv[2]));
		return 0;
	}
	
	test_list_add_head();
	test_list_add_tail();

	test_list_delete_first();
	test_list_delete_last();
	test_list_delete_middle();

	test_list_find_first();
	test_list_find_middle();
	test_list_find_last();

	test_list_insert_at_first();
	test_list_insert_at_last();
	test_list_insert_at_middle();
	
#ifdef USE_MOVE_TO
	test_list_insert_at();
	test_list_move_to_first_to();
	test_list_move_to_last_to();
	test_list_move_to_first_from();
	test_list_move_to_last_from();
	test_list_move_to_middle();
	test_list_move_to_first_to_last();
	test_list_move_to_last_to_first();
#endif

	test_list_move_first_to_first();
	test_list_move_first_to_middle();
	test_list_move_first_to_last();

	return 0;
}
