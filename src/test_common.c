/* gcc -DTESTING -g -I.. -Wall -Werror test_common.c common.c && valgrind --leak-check=yes --show-reachable=yes ./a.out */
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int
main(
    )
{
	char t0[64];
	char t1[64];
	char *t2;
#define MATCH2(_f_, _fn_, _i0_, _i1_, _r_) \
		if ((_f_(_i0_, _i1_) == 0) != _r_) { \
			printf("%s fails!\n", _fn_); \
		}
#define MATCH3(_f_, _fn_, _i0_, _i1_, _n_, _r_) \
		if ((_f_(_i0_, _i1_, _n_) == 0) != _r_) { \
			printf("%s fails!\n", _fn_); \
		}
#define COMPARE(_fn_, _i0_, _i1_, _r_) \
		if ((strcmp(_i0_, _i1_) == 0) != _r_) { \
			printf("%s fails!\n", _fn_); \
		}

	sprintf(t0, "testing");
	sprintf(t1, "testing");
	/* "testing", "testing" */
	MATCH2(xstrcmp, "xstrcmp", t0, t1, 1);
	sprintf(t1, "testXXX");
	/* "testing", "testXXX" */
	MATCH2(xstrcmp, "xstrcmp", t0, t1, 0);
	
	/* "testing", "testXXX" */
	MATCH3(xstrncmp, "xstrncmp", t0, t1, 4, 1);
	/* "testing", "testXXX" */
	MATCH3(xstrncmp, "xstrncmp", t0, t1, 6, 0);

	/* "testing", "testXXX" */
	MATCH2(xstrcasecmp, "xstrcasecmp", t0, t1, 0);
	sprintf(t1, "TeStInG");
	/* "testing", "TeStInG" */
	MATCH2(xstrcasecmp, "xstrcasecmp", t0, t1, 1);
	
	sprintf(t1, "TeStIXXX");
	/* "testing", "TeStIXXX" */
	MATCH3(xstrncasecmp, "xstrncasecmp", t0, t1, 6, 0);
	MATCH3(xstrncasecmp, "xstrncasecmp", t0, t1, 4, 1);

	xstrcpy(t1, t0);
	/* "testing", "testing" */
	COMPARE("xstrcpy", t0, t1, 1);
	
	xstrncpy(t1, "TESTING", 4);
	/* "testing", "TESTing" */
	COMPARE("xstrncpy", t1, "TESTing", 1);

	t2 = xstrdup(t0);
	/* "testing", "testing" */
	COMPARE("xstrdup", t0, t2, 1);
	/* "testing", "TESTing" */
	COMPARE("xstrdup", t1, t2, 0);

	xfree(t2);

	t2 = xmalloc(16);
	xfree(t2);

	t2 = calloc(16, 16);
	t2 = xrealloc(t2, 128);
	xfree(t2);
	
	return 0;
}
