/* gcc -DTESTING -g -I.. -Wall -Werror test_tools.c tools.c && valgrind --leak-check=yes --show-reachable=yes ./a.out */
#include "tools.h"
#include <stdio.h>
#include <stdlib.h>



int
main(
    )
{
	/* upcase() && lowcase() */
	{
	char foo[64];
	
	sprintf(foo, "foobar");
	printf("Before case-conversion: '%s'\n", foo);
	upcase(foo);
	printf("After upcase: '%s'\n", foo);
	lowcase(foo);
	printf("After lowcase: '%s'\n", foo);

	sprintf(foo, "unicode öä ¹²³");
	printf("Before case-conversion: '%s'\n", foo);
	upcase(foo);
	printf("After upcase: '%s'\n", foo);
	lowcase(foo);
	printf("After lowcase: '%s'\n", foo);
	}

	/* randname() */
	{
	char foo[64];
	int i, j;

#define LEN	8
	for (i = 0; i < 2; i++) {
		switch (i) {
			case 0:
				sprintf(foo, "user");
				break;
			case 1:
				sprintf(foo, "username");
				break;
		}
		printf("randname: starting with '%s' (len = %d)\n", foo, LEN);
		for (j = 0; j < LEN * 2; j++) {
			randname(foo, LEN, '#');
			printf("round %02d: '%s'\n", j, foo);
		}
	}
	printf("Generating random stuff:\n");
	for (i = 0; i < 4; i++) {
		foo[0] = '\0';
		randname(foo, LEN, '#');
		printf("round %d: '%s'\n", i, foo);
	}
	}

	/* pos() && lastpos() */
	{
	char foo[64];
	
	sprintf(foo, "There is no greater power in the universe than "
			"the need for freedom.");
	printf("String: '%s'\n", foo);
	printf("First occurance of 'o' is at %d\n", pos(foo, 'o'));
	printf("Last occurance of 'o' is at %d\n", lastpos(foo, 'o'));
	}
	
	/* nextword() && lastword() */
	{
	char foo[64];
	char *ptr;

	sprintf(foo, "There is no greater power in the universe than "
			"the need for freedom.");
	printf("Words (after the first one):\n");
	ptr = nextword(foo);
	while (ptr != NULL) {
		printf("'%s'\n", ptr);
		ptr = nextword(ptr);
	}
	printf("Last word: '%s'\n", lastword(foo));
	}

	/* get_short_localtime() && get_timestamp() */
	{
	time_t t;
	t = rand();
	printf("Some timestamps:\n");
	printf("Localtime: '%s'\n", get_short_localtime());
	printf("Long: '%s'\n", get_timestamp(NULL, TIMESTAMP_LONG));
	printf("Short: '%s'\n", get_timestamp(NULL, TIMESTAMP_SHORT));
	printf("Abritary: '%s'\n", get_timestamp(&t, TIMESTAMP_LONG));
	}
	

	return 0;
}
