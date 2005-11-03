/* $Id$ */
/* gcc -DTESTING -DDUMPSTATUS -g -I.. -Wall -Werror test_match.c match.c tools.c common.c && valgrind --leak-check=yes --leak-resolution=high --show-reachable=yes --num-callers=512 ./a.out */
#include "match.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define MATCH(_s_, _p_, _r_) \
	if (match(_s_, _p_) != _r_) { \
		printf("matching \"%s\" against \"%s\" failed!\n", _s_, _p_); \
	}
#define MATCH_R(_s_, _p_, _f_, _fl_, _r_, _rt_) \
	if (match_replace(_s_, _p_, _f_, _fl_, _r_) != _rt_) { \
		printf("matching \"%s\" against \"%s\" (replace \"%s\" " \
				"with \"%s\", len = %d) failed!\n", \
				_s_, _p_, _f_, _r_, _fl_); \
	}

int
main(
    )
{
	MATCH("foo", "foo", 1);
	MATCH("foo", "FOO", 1);
	MATCH("foo", "bar", 0);
	
	MATCH("foo", "?oo", 1);
	MATCH("foo", "f?o", 1);
	MATCH("foo", "fo?", 1);
	MATCH("foo", "foo?", 0);
	
	MATCH("foobar", "foo*", 1);
	MATCH("foobar", "FOO*", 1);
	MATCH("foobar", "*BAR", 1);
/*	MATCH("foobar", "*OBA*", 1);	limitations */
	MATCH("foobar", "*BAAR", 0);

	MATCH("o:TOmmppa!Kage@b-216-253.cable.kpy.customers.dnainternet.fi/#serveri", "o:Tommppa!Kage@*.cable.kpy.customers.dnainternet.fi/#serveri", 1);
	MATCH("foobar", "foobar*", 1);

	MATCH_R("foo@host", "foo@X", "X", -1, "host", 1);
	MATCH_R("foo@host-goo", "foo@@@-goo", "@@", 2, "host", 1);
	
	return 0;
}
