/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2002-2004 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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
#include "tools.h"
#include "common.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>



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
	 * Someone must have done this (smaller, better and safe)
	 * before...
	 * 
	 * Now we simply ignore the threat and keep going.
	 */

	return vsprintf(str, format, ap);
} /* int vsnprintf(char *, size_t, const char *, va_list) */
#endif /* ifdef VSNPRINTF_WORKAROUND */



/**
 * Convert to upper case.
 * @str:	String to convert
 */
void
upcase(
		char	*str
      )
{
	char *ptr;
	for (ptr = str; ptr != NULL && *ptr != '\0'; ptr++) {
		*ptr = (char) toupper((int) *ptr);
	}
} /* void upcade(char *str) */



/**
 * Convert to lower case.
 * @str:	String to convert
 */
void
lowcase(
		char	*str
       )
{
	char *ptr;
	for (ptr = str; ptr != NULL && *ptr != '\0'; ptr++) {
		*ptr = (char) tolower((int) *ptr);
	}
} /* void lowcase(char *str) */



/**
 * Generates a random string or new nick out of the old one.
 * @target:	Target. It *target != '\0', target will be treated as old nick.
 * 		If *target == '\0', completely random string will be created.
 * @length:	(Maximum) length for target
 * @fillchar:	Character to use pad undersized target
 *
 * Target must have space for length + 1 characters.
 * 
 * This function doesn't work with multibyte characters.
 */
void
randname(
		char		*target,
		const size_t	length,
		const char	fillchar
	)
{
	size_t oldlen;
	size_t i;
	char shift;
	
	/* TODO paranoia */
	if (target == NULL) {
		/* No target - nothing to do. */
		return;
	}

	/* Genereate from scratch? */
	if (target[0] == '\0') {
		for (i = 0; i < length; i++) {
			target[i] = (char)('A' + (rand() % 56));
		}
		target[length] = '\0';
		return;
	}

	/* Try to generate a new nick from the old one. */
	oldlen = strlen(target);

	if (oldlen < length) {
		/*
		 * Nick is shorter than maximum length, try adding fillchar
		 * at the end.
		 */
		target[oldlen] = fillchar;
		target[oldlen + 1] = '\0';
		return;
	}

	/* Nick is already as long as it can be. Try rotating the nick. */
	shift = target[length - 1];
	memmove(target + 1, target, length - 1);
	target[0] = shift;
} /* void randname(char *target, const size_t length, const char fillchar) */



/**
 * Returns index of first occurance of "c" in "str".
 * @str:	String to look from
 * @c:		Character to find
 *
 * Returns: Index of last occurance of "c" in "str". -1 if "c" was not found.
 *
 * This function does _not_ work with multibyte characters. We get "int c"
 * instead of "char c" so that this could be fixed without major rewriting.
 */
int
pos(
		const char	*s,
		const int	c
   )
{
	/* It takes less space to do this than to use strchr. */
	int i;

	for (i = 0; s[i] != '\0'; i++) {
		if (s[i] == c) {
			return i;
		}
	}

	return -1;
} /* int pos(const char *s, const int c) */



/**
 * Returns index of last occurance of "c" in "str".
 * @str:	String to look from
 * @c:		Character to find
 *
 * Returns: Index of last occurance of "c" in "str". -1 if "c" was not found.
 *
 * This function does _not_ work with multibyte characters. We get "int c"
 * instead of "char c" so that this could be fixed without major rewriting.
 */
int
lastpos(
		const char	*s,
		const int	c
       )
{
	/* It takes less space to do this than to use strchr. */
	int i;

	if (s == NULL) {
		return -1;
	}

	for (i = strlen(s); i >= 0; i--) {
		if (s[i] == c) {
			return i;
		}
	}

	return -1;
} /* int lastpos(const char *s, const int c) */



/**
 * Finds next word starting from s.
 * @s:	Where to start searching.
 *
 * Returns: Pointer to stuff starting from next word, NULL if there are no
 * words left.
 */
char *
nextword(
		char	*s
	)
{
	int i;
	
	i = pos(s, ' ');
	if (i == -1) {
		return NULL;
	} else {
		return s + i + 1;
	}
} /* char *nextword(char *s) */



/**
 * Finds last word, starting from s.
 * @s:	Where to start searching.
 *
 * Returns: Pointer to last word, NULL if there are no words before s.
 */
char *
lastword(
		char	*s
	)
{
	int i;
	
	i = lastpos(s, ' ');
	if (i == -1) {
		return s;
	} else {
		return s + i + 1;
	}
} /* char *lastword(char *s) */



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
} /* void getuptime(time_t now, int *days, int *hours, int *minutes,
		int *seconds) */
#endif	/* UPTIME */



/**
 * Creates a timestamp.
 * @t:		Pointer to time, NULL if current time.
 * @mode:	Timestamp format
 *
 * Returns: Pointer to timestamp. Allocated memory must not be freed.
 *
 * Creates timestamps like:
 *	Sun Jan 02 11:53:33 2002
 * or
 *	Jan 02 11:53:33
 */
const char *
get_timestamp(
		time_t			*t,
		const timestamp_t	mode
	     )
{
	struct tm	*form;
	static char	stamp[100];
	time_t		ts;

	if (t == NULL) {
		time(&ts);
	} else {
		ts = *t;
	}
	form = localtime(&ts);
	switch (mode) {
		case TIMESTAMP_LONG:
			strftime(stamp, 99, "%a %b %d %H:%M:%S %Y", form);
			break;
		case TIMESTAMP_SHORT:
			strftime(stamp, 99, "%b %d %H:%M:%S", form);
	}

	return stamp;
} /* char *get_timestamp(time_t, const timestamp_t mode) */



/**
 * Creates a short current timestamp.
 *
 * Returns: Pointer to timestamp. Allocated memory must not be freed.
 *
 * Creates a timestamp like: "Jan 02 11:53:33"
 */
const char *
get_short_localtime(
		)
{
	return get_timestamp(NULL, TIMESTAMP_SHORT);
} /* const char *get_short_localtime() */
