/*
 * -------------------------------------------------------
 * Copyright (C) 2002-2003 Tommi Saviranta <tsaviran@cs.helsinki.fi>
 *	(C) 2002 Lee Hardy <lee@leeh.co.uk>
 *	(C) 1998-2002 Sebastian Kienzl <zap@riot.org>
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

#include "miau.h"
#include "tools.h"

#include "irc.h"



#ifdef VSNPRINTF_WORKAROUND
int
vsnprintf(
		char		*str,
		size_t		size,
		const char	*format,
		va_list		ap
	     )
{
	/*
	 * TODO:
	 * Someone must have done this (smaller, better and safer)
	 * before...
	 * 
	 * Now we simply ignore the threat and keep going.
	 */

	return vsprintf(str, format, ap);
}
#endif /* VSNPRINTF_WORKAROUND */



/*
 * Convert to upper case.
 */
void
upcase(
		char	*what
	   )
{
	char	*doit;
	if (what) {
		for (doit = what; doit && *doit; doit++) {
			*doit = (char) toupper((int) *doit);
		}
	}
}



/*
 * Generates a random string or new nick out of the old one.
 */
void
randname(
		char	*randchar,
		char	*oldnick,
		int	length
		)
{
	int	i;
	int	oldlen;
	
	if (randchar == NULL) {
		/* No target - nothing to do. */
		return;
	}

	if (oldnick == NULL) {
		/* No old nick (or generating salt), just generate a new one. */
		for (i = 0; i < length; i++ ) {
			randchar[i] = (char)('A' + (rand() % 56));
		}
		randchar[length] = '\0';
		return;
	}

	/* Try to generate a new nick from the old one. */
	oldlen = strlen(oldnick);

	if (oldlen < length) {
		/*
		 * Nick is shorter than maximum length, try adding a '^'
		 * at the end.
		 */
		xstrncpy(randchar, oldnick, oldlen);
		randchar[oldlen] = cfg.nickfillchar;
		randchar[oldlen + 1] = '\0';
		return;
	}

	/* Nick is already as long as it can be. Try rotating the nick. */
	xstrncpy(randchar + 1, oldnick, length - 1);
	xstrncpy(randchar, oldnick + length - 1, 1);
}



/*
 * Returns index of what in *str.
 */
int
pos(
		const char	*str,
		const char	what
       )
{
	/* It takes less space to do this than to use strchr. */
	int	i = 0;

	if (str != NULL) {
		while (str[i]) {
			if (str[i] == what) return i;
			i++;
		}
	}

	return -1;
}



int
lastpos(
		const char	*str,
		const char	what
	   )
{
	int	i;
	if (str != NULL) {
		i = strlen(str) - 1;
		while (i) {
			if (str[i] == what) return i;
			i--;
		}
	}
	return -1;
}



char *
nextword(
		char	*string
	      )
{
	int	i = pos(string, ' ');
	if (i == -1) {
		return NULL;
	} else {
		return (string + i + 1);
	}
}


char *
lastword(
		char	*from
	      )
{
	int	i = lastpos(from, ' ');
	if (i == -1) {
		return from;
	} else {
		return (from + i + 1);
	}
}



/*
 * Creates a timestamp in the form:
 *	Mar 21 11:23:12
 */
char *
gettimestamp(
		int	oldtime
	    )
{
	time_t		t;
	struct tm	*lt;
	static char	stamp[100];

	if (oldtime == 0) {
		time(&t);
	} else {
		t = (time_t) oldtime;
	}
	lt = localtime(&t);
	strftime(stamp, 99, "%b %d %H:%M:%S", lt);
	
	return stamp;
}



#ifdef UPTIME
void
getuptime(
		time_t	now,
		int	*days,
		int	*hours,
		int	*minutes,
		int	*seconds
	      )
{
	*days = now / 86400;
	now %= 86400;
	*hours = now / 3600;
	now %= 3600;
	*minutes = now / 60;
	*seconds = now % 60;
}
#endif	/* UPTIME */



void
report(
		char	*format,
		...
	   )
{
	char	buffer[150];
	va_list	va;
	
	va_start(va, format);
	vsnprintf(buffer, 149, format, va);
	va_end(va);

	fprintf(stdout, "%s + %s\n", gettimestamp(0), buffer);
	irc_mnotice(&c_clients, status.nickname, buffer);
}



void
error(
		char	*format,
		...
	  )
{
	char	buffer[150];
	va_list	va;

	va_start(va, format);
	vsnprintf(buffer, 149, format, va);
	va_end(va);
	
	fprintf(stdout, "%s - %s\n", gettimestamp(0), buffer);
	irc_mnotice(&c_clients, status.nickname, buffer);
}
