/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2002-2005 Tommi Saviranta <wnd@iki.fi>
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

#ifndef TOOLS_H_
#define TOOLS_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <time.h>



typedef enum {
	TIMESTAMP_LONG,
	TIMESTAMP_SHORT
} timestamp_t;



void upcase(char *str);
void lowcase(char *str);
void randname(char *target, const size_t length, const char fillchar);

int pos(const char *s, const int c);
int lastpos(const char *s, const int c);

char *nextword(char *s);
char *lastword(char *s);

#ifdef UPTIME
void getuptime(time_t now, int *days, int *hours, int *minutes, int *seconds);
#endif	/* UPTIME */

const char *get_timestamp(time_t *t, const timestamp_t mode);
const char *get_short_localtime(void);



#endif /* ifndef TOOLS_H_ */
