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

#include <config.h>
#include "qlog.h"

#include "error.h"
#include "miau.h"
#include "commands.h"
#include "channels.h"
#include "tools.h"
#include "llist.h"
#include "irc.h"
#include "table.h"
#include "log.h"
#include "messages.h"



#ifdef QUICKLOG
extern clientlist_type	c_clients;
llist_list		qlog;



#ifdef QLOGSTAMP
void qlog_add_timestamp(qlogentry *data, char *buf, int size);
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
	channel_type *chan;

	/* First set each channel not to have qlog. */
	LLIST_WALK_H(active_channels.head, channel_type *);
		data->hasqlog = 0;
	LLIST_WALK_F;
	
	LLIST_WALK_H(qlog.head, qlogentry *);
		/* Don't waste time looking for channel if line's privmsg. */
#ifdef INBOX
		if (! data->privmsg) {
#endif /* ifdef INBOX */
			chan = qlog_get_channel(data->text);
			if (chan != NULL) {
				chan->hasqlog = 1;
			}
#ifdef INBOX
		}
#endif /* ifdef INBOX */
	LLIST_WALK_F;
} /* qlog_check(void) */

		

/*
 * (Replay and) clean quicklog data.
 */
void
qlog_replay(connection_type *client, const int keep)
{
#ifdef QLOGSTAMP
	char qlogbuf[IRC_MSGLEN];
#endif /* QLOGSTAMP */
	
	qlog_drop_old();

	/* Walk thru quicklog. */
	LLIST_WALK_H(qlog.head, qlogentry *);
		if (client != NULL) {
#ifdef QLOGSTAMP
			if (cfg.timestamp != TS_NONE) {
				qlog_add_timestamp(data, qlogbuf, IRC_MSGLEN);
				irc_write(client, "%s", qlogbuf);
			} else {
				irc_write(client, "%s", data->text);
			}
#else /* ifdef QLOGSTAMP */
			irc_write(client, "%s", data->text);
#endif /* QLOGSTAMP */
		}
		if (! keep) {
			xfree(data->text);
			xfree(data);
			llist_delete(node, &qlog);
		}
	LLIST_WALK_F;
} /* void qlog_replay(connection_type *client, const int keep) */



#ifdef QLOGSTAMP
/*
 * Add timestamp in qlogentry.
 */
void
qlog_add_timestamp(qlogentry *data, char *buf, int size)
{
	int		i = -1;
	int		len;
	char		*p;
	char		*t;
/* TSLEN: "[HH:MM:SS] \0" == 12 */
#define TSLEN	12
	char		stamp[TSLEN];

	strncpy(buf, data->text, size);
	buf[size - 1] = '\0';
	
	p = nextword(data->text);
	/*
	 * If we can't handle it - ignore it. We give up in "if (! (i == ...".
	 * What we can't see, can't hurt us, right? :-)
	 */
	if (p != NULL) {
		/* Next find out what command it was. */
		i = pos(p, ' ');
		if (i < TSLEN - 1 || i > 16) {
			char tmp[18];
			strncpy(tmp, p, i);
			tmp[i - 1] = '\0';
			i = command_find(tmp);
		} else {
			return;
		}
	}

	/* Is this something we can handle? */
	if (i != CMD_PRIVMSG && i != CMD_NOTICE && i != CMD_QUIT &&
			i != CMD_PART && i != CMD_KICK && i != CMD_KILL) {
		return;
	}

	p = buf + (p - data->text); /* Skip first word as discovered before. */

	/* Get place to insert timestamp. */
	switch (cfg.timestamp) {
		case TS_BEGINNING:
			p = strchr(p, (int) ':');
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
				return;
			}
			p++;
			break;

		case TS_END:
			p = strchr(p, (int) ':');
			if (p == NULL) {
				return;
			}
			t = strrchr(p + 1, '\1');
			if (t != NULL) {
				p = t;
			} else {
				p = buf + strlen(buf);
			}
			break;

		default:
			/* Do nothing by default. Return, don't break. */
			return;
	}
	
	/* Insert timestamp. */
	i = (int) (p - buf);
	len = (int) strlen(p);
	memmove(p + TSLEN - 1, p, len); /* Move '\0' as well. */
	switch (cfg.timestamp) {
		case TS_BEGINNING:
			strftime(stamp, TSLEN, "[%H:%M:%S] ",
					localtime(&data->timestamp));
			break;
		case TS_END:
			strftime(stamp, TSLEN, " [%H:%M:%S]",
					localtime(&data->timestamp));
			break;
		default:
			break;
	}
	memcpy(p, stamp, TSLEN - 1);
} /* void qlog_add_timestamp(qlogentry *data, char *buf, int size) */
#endif /* QLOGSTAMP */



/*
 * Remove old lines from quicklog.
 */
void
qlog_drop_old(void)
{
	qlogentry	*line;
	time_t		oldest;

	if (qlog.head == NULL) { return; } /* Perhaps there's nothing to do. */

	/* We want seconds. */
	oldest = time(NULL) - (cfg.qloglength * 60);

	line = (qlogentry *) qlog.head->data;
	while (line != NULL && line->timestamp <= oldest) {
#ifdef INBOX
		if (line->privmsg == 1) {
			char *message;

			/* Get sender (split it) and beginning of payload. */
			message = strchr(line->text + 1, (int) ':');

			if (message == NULL) {
#ifdef ENDUSERDEBUG
				enduserdebug("converting invalid qlog-line?");
				enduserdebug("%s", line->text);
#endif /* ifdef ENDUSERDEBUG */
				continue;
			}
			strtok(line->text, " ");

			if (line->text[0] == '\0' || message[0] == '\0') {
#ifdef ENDUSERDEBUG
				enduserdebug("invalud stuff in qlog");
#endif /* ifdef else ENDUSERDEBUG */
				continue;
			}

			/* termination and validity guaranteed */
			fprintf(inbox, "%s <%s> %s\n",
					get_timestamp(&line->timestamp,
						TIMESTAMP_SHORT),
					line->text + 1, message + 1);
			fflush(inbox);
		}
#endif /* INBOX */

		xfree(line->text);
		xfree(line);
		llist_delete(qlog.head, &qlog);
		if (qlog.head != NULL) {
			line = (qlogentry *) qlog.head->data;
		} else {
			line = NULL;
		}
	}
} /* void qlog_drop_old(void) */



/*
 * Write lines to quick log.
 */
void
qlog_write(const int privmsg, char *format, ...)
{
	qlogentry	*line;
	va_list		va;
	char		buf[BUFFERSIZE];

	/* First remove possible outdated lines. */
	qlog_drop_old();

	va_start(va, format);
	vsnprintf(buf, BUFFERSIZE - 3, format, va);
	va_end(va);
	buf[BUFFERSIZE - 3] = '\0';

	/* Create new line of quicklog. */
	line = (qlogentry *) xmalloc(sizeof(qlogentry));
	time(&line->timestamp);
	line->text = xstrdup(buf);
#ifdef INBOX
	line->privmsg = privmsg;
#endif /* ifdef INBOX */
	llist_add_tail(llist_create(line), &qlog);
} /* void qlog_write(const int privmsg, char *format, ...) */



static channel_type *
qlog_get_channel(const char *msg)
{
	channel_type	*chan = NULL;
	char		*b;
	int		l;
	char		*t;

	b = strchr(msg, (int) ' ');
	if (b != NULL) {
		b = strchr(b + 1, (int) ' ');
		if (b != NULL) {
			l = pos(b + 1, ' ');
			if (l != -1) {
				t = (char *) xcalloc(l + 1, 1);
				memcpy(t, b + 1, l);
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



#endif /* QUICKLOG */
