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

#ifdef QUICKLOG

#include "qlog.h"
#include "client.h"
#include "irc.h"
#include "list.h"
#include "llist.h"
#include "miau.h"
#include "tools.h"
#include "commands.h"
#include "error.h"
#include "common.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>



static list_type *qlog = NULL;



#ifdef QLOGSTAMP
static const char *qlog_add_timestamp(qlogentry *entry, char *buf,
		size_t size);
#endif /* QLOGSTAMP */

static channel_type *qlog_get_channel(const char *msg);



/*
 * Walk thru qlog to see if any of active channels has some log to be replayed.
 * Basically this could be done while writing qlog, but this would be waste of
 * CPU-time as qlog can be checked just as well before replaying.
 */
void
qlog_check(void)
{
	llist_node *iterl;
	list_type *iter;

	/* First set each channel not to have qlog. */
	for (iterl = active_channels.head; iterl != NULL; iterl = iterl->next) {
		channel_type *chan;
		chan = (channel_type *) iterl->data;
		chan->hasqlog = 0;
	}

	for (iter = qlog; iter != NULL; iter = iter->next) {
		qlogentry *entry;
		entry = (qlogentry *) iter->data;
		/* Don't waste time looking for channel if line's privmsg. */
#ifdef INBOX
		if (entry->privmsg == 0)
#endif /* ifdef else INBOX */
		{
			channel_type *chan;
			chan = qlog_get_channel(entry->text);
			if (chan != NULL) {
				chan->hasqlog = 1;
			}
		}
	}
} /* void qlog_check(void) */

		

/*
 * (Replay and) clean quicklog data.
 */
void
qlog_replay(connection_type *client, int keep)
{
	list_type *iter;
	list_type *next;
#ifdef QLOGSTAMP
	char qlogbuf[IRC_MSGLEN];
#endif /* QLOGSTAMP */
	
	qlog_drop_old();

	/* Walk thru quicklog. */
	for (iter = qlog; iter != NULL; ) { /* handle next at the end */
		qlogentry *entry;
		next = iter->next;
		entry = (qlogentry *) iter->data;
		
		if (client != NULL) {
#ifdef QLOGSTAMP
			if (cfg.timestamp != TS_NONE) {
				const char *out;
				out = qlog_add_timestamp(entry, qlogbuf,
						IRC_MSGLEN);
				irc_write(client, "%s", out);
			} else {
				irc_write(client, "%s", entry->text);
			}
#else /* ifdef QLOGSTAMP */
			irc_write(client, "%s", entry->text);
#endif /* ifdef else QLOGSTAMP */
		}
		if (keep == 0) {
			xfree(entry->text);
			xfree(entry);
			qlog = list_delete(qlog, iter);
		}
		iter = next;
	}
} /* void qlog_replay(connection_type *client, int keep) */



#ifdef QLOGSTAMP
/*
 * Add timestamp in qlogentry.
 */
static const char *
qlog_add_timestamp(qlogentry *entry, char *buf, size_t size)
{
	int cmd, i;
	char *p;
/* TSLEN: "[HH:MM:SS]\0" == 11 */
#define TSLEN 11
	char stamp[TSLEN];
	
	/* attach tag only if we know what we're doing */
	p = nextword(entry->text);
	if (p == NULL) {
		return entry->text;
	}
	/* Next find out what command it was. */
	i = pos(p, ' ');
	if (i != -1 && i < TSLEN) {
		char tmp[18];
		strncpy(tmp, p, i);
		tmp[i] = '\0';
		cmd = command_find(tmp);
	} else {
		return entry->text;
	}

	/* Is this something we can handle? */
	if (cmd != CMD_PRIVMSG && cmd != CMD_NOTICE && cmd != CMD_QUIT &&
			cmd != CMD_PART && cmd != CMD_KICK && cmd != CMD_KILL) {
		return entry->text;
	}

	switch (cfg.timestamp) {
		case TS_BEGINNING:
		{
			char rep;

			p = strchr(entry->text + 1, (int) ':');
			if (p != NULL) {
				if (p[1] == '\0') {
					p = NULL;
				} else if (p[1] == '\1') {
					p = strchr(p, (int) ' ');
				}
			}
			/*
			 * Confused? Don't break already broken things any
			 * further.
			 */
			if (p == NULL) {
				return entry->text;
			}
			rep = *p;
			*p = '\0'; /* ugly, but makes life easier */

			strftime(stamp, TSLEN, "[%H:%M:%S]",
					localtime(&entry->timestamp));

			snprintf(buf, size, "%s%c%s %s",
					entry->text, rep, stamp, p + 1);
			buf[size - 1] = '\0';
			*p = rep;
			
			return buf;
		}

		case TS_END:
		{
			int add_one;
			int len;

			len = strlen(entry->text);
			if (entry->text[len - 1] == '\1') {
				entry->text[len - 1] = '\0';
				add_one = 1;
			} else {
				add_one = 0;
			}

			strftime(stamp, TSLEN, "[%H:%M:%S]",
					localtime(&entry->timestamp));

			snprintf(buf, size, "%s %s%s", entry->text,
					stamp, add_one == 1 ? "\1" : "");
			buf[size - 1] = '\0';
			return buf;
		}

		default:
			return entry->text;
	}
} /* static const char *qlog_add_timestamp(qlogentry *entry, char *buf,
		size_t size) */
#endif /* ifdef QLOGSTAMP */



/*
 * Remove old lines from quicklog.
 */
void
qlog_drop_old(void)
{
	time_t oldest;

	if (qlog == NULL) {
		return;
	}

	oldest = time(NULL) - (cfg.qloglength * 60);

	while (qlog != NULL) { /* secondary exit condition */
		qlogentry *entry;
		entry = (qlogentry *) qlog->data;

		/* primary exit condition */
		if (entry->timestamp > oldest) {
			break;
		}

#ifdef INBOX
		if (entry->privmsg == 1) {
			char *message;

			/* Get sender (split it) and beginning of payload. */
			message = strchr(entry->text + 1, (int) ':');

			if (message == NULL) {
#ifdef ENDUSERDEBUG
				enduserdebug("converting invalid qlog-line?");
				enduserdebug("%s", entry->text);
#endif /* ifdef ENDUSERDEBUG */
				goto drop_free;
			}
			strtok(entry->text, " ");

			if (entry->text[0] == '\0' || message[0] == '\0') {
#ifdef ENDUSERDEBUG
				enduserdebug("invalud stuff in qlog");
#endif /* ifdef else ENDUSERDEBUG */
				goto drop_free;
			}

			/* termination and validity guaranteed */
			fprintf(inbox, "%s <%s> %s\n",
					get_timestamp(&entry->timestamp,
						TIMESTAMP_SHORT),
					entry->text + 1, message + 1);
			fflush(inbox);
		}
#endif /* INBOX */

drop_free:
		xfree(entry->text);
		xfree(entry);
		qlog = list_delete(qlog, qlog);
	}
} /* void qlog_drop_old(void) */



/*
 * Write lines to quick log.
 */
void
qlog_write(const int privmsg, char *format, ...)
{
	qlogentry *line;
	va_list va;
	char buf[BUFFERSIZE];

	/* First remove possible outdated lines. */
	qlog_drop_old();

	va_start(va, format);
	vsnprintf(buf, IRC_MSGLEN - 2, format, va);
	va_end(va);
	buf[IRC_MSGLEN - 3] = '\0';

	/* Create new line of quicklog. */
	line = (qlogentry *) xmalloc(sizeof(qlogentry));
	time(&line->timestamp);
	line->text = xstrdup(buf);
#ifdef INBOX
	line->privmsg = privmsg;
#endif /* ifdef INBOX */
	qlog = list_add_tail(qlog, line);
} /* void qlog_write(const int privmsg, char *format, ...) */



static channel_type *
qlog_get_channel(const char *msg)
{
	channel_type *chan;
	char *b;

	chan = NULL;
	b = strchr(msg, (int) ' ');
	if (b != NULL) {
		b = strchr(b + 1, (int) ' ');
		if (b != NULL) {
			int l;
			l = pos(b + 1, ' ');
			if (l != -1) {
				char *t;
				t = (char *) xmalloc(l + 1);
				memcpy(t, b + 1, l);
				t[l] = '\0';
				/* Check active/old_channels. */
				chan = channel_find(t, LIST_ACTIVE);
				if (chan == NULL) {
					chan = channel_find(t, LIST_OLD);
				}
				xfree(t);
			}
		}
	}

	return chan;
} /* static channel_type *qlog_get_channel(const char *msg) */



#endif /* ifdef QUICKLOG */
