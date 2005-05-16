/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2004-2005 Tommi Saviranta <wnd@iki.fi>
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

#include "log.h"

#include "miau.h"
#include "messages.h"
#include "tools.h"



#ifdef LOGGING
/*
 * Prepare message for logging. Returns pointer to printable message or NULL
 * if message is normal PRIVMSG.
 *
 * Memory that returned pointer points to, must not be freed.
 */
char *
log_prepare_entry(const char *nick, const char *msg)
{
/*
 * 520 characters should be enough for all PRIVMSGs and ACTIONs.
 * RFC2812 says "IRC messages are always lines of characters terminated with a
 * CR-LF (Carriage Return - Line Feed) pair, and these messages SHALL NOT
 * exceed 512 characters in length, counting all characters including the
 * trailing CR-LF.".
 */
#define BUFLEN	520
	static char buf[BUFLEN];
	int i;
	int len, rpos;
	/* Messages not beginning with '\1ACTION ' don't need to be touched. */
	if (msg[0] != '\1' || xstrncmp(msg, "\1ACTION ", 8) != 0) {
		return NULL;
	}
	/* Neither do messages that are malformed. */
	len = (int) strlen(msg + 8);
	i = len;
	while (msg[i + 8] != '\1' && i > 0) { i--; }
	if (i == 0) {
		return NULL;
	}
	rpos = len - i + 1;
	
	/* Right now we only parse ACTIONs. */
	snprintf(buf, BUFLEN - 1, LOGM_ACTION, get_short_localtime(),
			nick, msg + 8);
	/* Remove trailing '\1'. */
	len = (int) strlen(buf);
	memmove(buf + len - rpos, buf + len - rpos + 1, rpos);
	
	return buf;
} /* char *log_prepare_entry(const char *nick, const char *msg) */
#endif /* LOGGING */
