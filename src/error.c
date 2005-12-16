/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2003-2005 Tommi Saviranta <wnd@iki.fi>
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
#ifndef TESTING
#include "irc.h"
#endif /* ifndef TESTING */
#include "client.h"
#include "miau.h"

#include <stdio.h>
#include <stdarg.h>



#ifdef ENDUSERDEBUG
#define ENDUSER_BUF_SIZE	8196

void
enduserdebug(char *format, ...)
{
	va_list va;
	char buf0[ENDUSER_BUF_SIZE];

	va_start(va, format);
	vsnprintf(buf0, ENDUSER_BUF_SIZE, format, va);
	va_end(va);
	buf0[ENDUSER_BUF_SIZE - 1] = '\0';
#ifndef TESTING
	if (c_clients.connected > 0) {
		char buf1[ENDUSER_BUF_SIZE + 160];
		/* termination and validity guaranteed */
		snprintf(buf1, ENDUSER_BUF_SIZE + 159,
				":debug PRIVMSG %s :%s: %s",
				status.nickname,
				get_timestamp(NULL, TIMESTAMP_LONG),
				buf0);
		buf1[ENDUSER_BUF_SIZE + 159] = '\0';
		irc_mwrite(&c_clients, "%s", buf1);
	}
#endif /* ifndef TESTING */
} /* void enduserdebug(char *format, ...) */
#endif /* ifdef ENDUSERDEBUG */



void
report(char *format, ...)
{
	char	buffer[256];
	va_list	va;
	
	va_start(va, format);
	vsnprintf(buffer, 256, format, va);
	va_end(va);
	buffer[255] = '\0';

	fprintf(stdout, "%s + %s\n", get_short_localtime(), buffer);
#ifndef TESTING
	irc_mnotice(&c_clients, status.nickname, buffer);
#endif /* ifndef TESTING */
} /* void report(char *format, ...) */



void
error(char *format, ...)
{
	char	buffer[256];
	va_list	va;

	va_start(va, format);
	vsnprintf(buffer, 256, format, va);
	va_end(va);
	buffer[255] = '\0';
	
	fprintf(stdout, "%s - %s\n", get_short_localtime(), buffer);
#ifndef TESTING
	irc_mnotice(&c_clients, status.nickname, buffer);
#endif /* ifndef TESTING */
} /* void error(char *format, ...) */
