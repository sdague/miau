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

#ifndef _TOOLS_H
#define _TOOLS_H



#define TIMESTAMP_NOW	0
#define TIMESTAMP_LONG	1
#define TIMESTAMP_SHORT	2



#ifdef VSNPRINTF_WORKAROUND
#include <stdarg.h>
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
#endif /* VSNPRINTF_WORKAROUND */

void upcase(char *what);
void randname(char *randchar, char *oldname, int length);

int pos(const char *str, const char what);
int lastpos(const char *str, const char what);
char *lastword(char *from);
char *nextword(char *string);
#ifdef UPTIME
void getuptime(time_t, int *, int *, int *, int *);
#endif	/* UPTIME */
void report(char *format, ...);
void error(char *format, ...);

char *get_timestamp(time_t, const int);
char *get_short_localtime();



#endif /* _TOOLS_H */
