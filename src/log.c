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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include "etc.h"

#ifdef NEED_LOGGING

#include "log.h"
#include "error.h"
#include "irc.h"
#include "messages.h"
#include "tools.h"
#include "common.h"

#include <string.h>



int warned_already = 0;


void
log_cannot_write(const char *file)
{
	char *rfile;
	size_t size;

	if (file == NULL) {
		debug("log_cannot_write(NULL)");
		return;
	}

	if (warned_already == 1) {
		return;
	}
	
	size = strlen(LOGDIR) + strlen(file) + 2;
	rfile = xmalloc(size);
	snprintf(rfile, size, "%s/%s", LOGDIR, file);
	rfile[size - 1] = '\0';
	report(MIAU_LOGNOWRITE, rfile);
	xfree(rfile);

	warned_already = 1;
} /* void log_cannot_write(const char *file) */



void
log_reset_warn_timer(void)
{
	warned_already = 0;
} /* void log_reset_warn_timer(void) */



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
		enduserdebug("log_prepare_entry(): nick = %s, msg = %s",
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
	snprintf(buf, IRC_MSGLEN, LOGM_ACTION,
			get_short_localtime(), nick, msg + 8);
	buf[IRC_MSGLEN - 1] = '\0';
	/* Remove trailing '\1'. */
	len = (int) strlen(buf);
	memmove(buf + len - rpos, buf + len - rpos + 1, rpos);

	return buf;
} /* char *log_prepare_entry(const char *nick, const char *msg) */



#endif /* ifdef NEED_LOGGING */
