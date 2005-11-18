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
#include "irc.h"
#include "error.h"



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
	static char buf[IRC_MSGLEN];
	int i;
	int len, rpos;

	if (nick == NULL || msg == NULL) {
#ifdef ENDUSERDEBUG
		enduserdebug("%s: nick = %s, msg = %s",
				__FUNCTION__,
				nick == NULL ? "NULL" : nick,
				msg == NULL ? "NULL" : msg);
#endif /* ifdef ENDUSERDEBUG */
		return NULL;
	}

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
	/* termination and validity guaranteed */
	/* even msg + 8 is safe, see xstrncmp above */
	snprintf(buf, IRC_MSGLEN - 1, LOGM_ACTION, get_short_localtime(),
			nick, msg + 8);
	buf[IRC_MSGLEN - 1] = '\0';
	/* Remove trailing '\1'. */
	len = (int) strlen(buf);
	memmove(buf + len - rpos, buf + len - rpos + 1, rpos);

	return buf;
} /* char *log_prepare_entry(const char *nick, const char *msg) */
#endif /* LOGGING */
