/*
 * -------------------------------------------------------
 * Copyright (C) 2003-2004 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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

#include "qlog.h"

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
void add_timestamp(qlogentry *);
#endif /* QLOGSTAMP */



/*
 * Walk thru qlog to see if any of active channels has some log to be replayed.
 * Basically this could be done while writing qlog, but this would be waste of
 * CPU-time as qlog can be checked just as well before replaying.
 */
void
qlog_check(
	  )
{
	channel_type *chan;

	/* First set each channel not to have qlog. */
	LLIST_WALK_H(active_channels.head, channel_type *);
		data->hasqlog = 0;
	LLIST_WALK_F;
	
	LLIST_WALK_H(qlog.head, qlogentry *);
		/* Don't waste time looking for channel if line's privmsg. */
		if (! data->privmsg) {
			chan = qlog_get_channel(data->text);
			if (chan != NULL) {
				chan->hasqlog = 1;
			}
		}
	LLIST_WALK_F;
} /* qlog_check() */

		

/*
 * (Replay and) clean quicklog data.
 */
void
qlog_replay(
		connection_type	*client,
		const int	keep
	   )
{
	qlog_drop_old();

	/* Walk thru quicklog. */
	LLIST_WALK_H(qlog.head, qlogentry *);
		if (client != NULL) {
#ifdef QLOGSTAMP
			if (cfg.timestamp != TS_NONE) {
				add_timestamp(data);
			}
#endif /* QLOGSTAMP */
			irc_write(client, "%s", data->text);
		}
		if (! keep) {
			xfree(data->text);
			xfree(data);
			llist_delete(node, &qlog);
		}
	LLIST_WALK_F;
} /* void qlog_replay(connection_type *, const int) */



#ifdef QLOGSTAMP
void
add_timestamp(
		qlogentry	*data
	     )
{
	int		i = -1;
	int		l;
	char		*p;
#define TSLEN	12
	static char	temp[TSLEN];
	
	p = strchr(data->text, (int) ' ');
	if (p != NULL) {
		i = pos(p + 1, ' ');
		if (i < TSLEN) {
			strncpy(temp, p + 1, i);
			temp[i] = '\0';
			i = command_find(temp);
		}
	}
	
	/* Is this something we can handle? */
	if (! (i == CMD_PRIVMSG
				|| i == CMD_NOTICE
				|| i == CMD_QUIT
				|| i == CMD_PART
				|| i == CMD_KICK
				|| i == CMD_KILL)) {
		return;
	}

	/* Get place to insert timestamp. */
	if (cfg.timestamp == TS_BEGINNING) {
		p = strchr(p + 1, (int) ':');
		if (p != NULL) {
			if (p[1] == '\0') {
				p = NULL;
			} else if (p[1] == '\1') {
				p = strchr(p, (int) ' ');
			}
		}
		/* Confused? Don't break already broken things any further. */
		if (p == NULL) {
			return;
		}
		strftime(temp, TSLEN, "[%H:%M:%S] ",
				localtime(&data->timestamp));
		p++;
	} else if (cfg.timestamp == TS_END) {
		char *t;
		p = strchr(p + 1, ':');
		if (p == NULL) {
			return;
		}
		t = strrchr(p + 1, '\1');
		if (t != NULL) {
			p = t;
		} else {
			p = data->text + strlen(data->text);
		}
		strftime(temp, TSLEN, " [%H:%M:%S]",
				localtime(&data->timestamp));
	}
	
	/* Insert timestamp. */
	i = p - data->text;
	l = strlen(data->text);
	data->text = xrealloc(data->text, l + TSLEN); // one extra for '\0'
	p = data->text + i;
	memmove(p + TSLEN - 1, p, l - i);
	strncpy(p, temp, TSLEN - 1);
} /* void add_timestamp(qlogentry *) */
#endif /* QLOGSTAMP */



/*
 * Remove old lines from quicklog.
 */
void
qlog_drop_old(
	     )
{
	qlogentry	*line;
	time_t		oldest;

	if (qlog.head == NULL) { return; } /* Perhaps there's nothing to do. */

	
	time(&oldest);
	oldest -= (cfg.qloglength * 60);	/* Minutes, not seconds. */
	/* oldest -= (cfg.qloglength); DEBUG */

	line = (qlogentry *) qlog.head->data;
	while (line != NULL && line->timestamp <= oldest) {
#ifdef PRIVMSGLOG
		if (line->privmsg == 1) {
			char *message;

			/* Get sender (split it) and beginning of payload. */
			message = strchr(line->text + 1, (int) ':');

#  ifdef ENDUSERDEBUG
			if (message == NULL) {
				enduserdebug("converting invalid qlog-line ?");
				enduserdebug("%s", line->text);
			} else {
#  else /* ENDUSERDEBUG */
			if (message != NULL) {
#  endif /* ENDUSERDEBUG */
				strtok(line->text, " ");
			
				fprintf(messagelog, "%s <%s> %s\n",
						gettimestamp(line->timestamp),
						line->text + 1, message + 1);
				fflush(messagelog);
			}
		}
#endif /* PRIVMSGLOG */		

		xfree(line->text);
		xfree(line);
		llist_delete(qlog.head, &qlog);
		if (qlog.head != NULL) {
			line = (qlogentry *) qlog.head->data;
		} else {
			line = NULL;
		}
	}
} /* void qlog_drop_old() */



/*
 * Write lines to quick log.
 */
void
qlog_write(
		const int	privmsg,
		char		*format,
		...
	  )
{
	qlogentry	*line;
	va_list		va;
	char		buf[BUFFERSIZE];

	/* First remove possible outdated lines. */
	qlog_drop_old();

	va_start(va, format);
	vsnprintf(buf, BUFFERSIZE - 3, format, va);
	va_end(va);

	/* Create new line of quicklog. */
	line = xmalloc(sizeof(qlogentry));
	time(&line->timestamp);
	line->text = strdup(buf);
	line->privmsg = privmsg;
	llist_add_tail(llist_create(line), &qlog);
} /* void qlog_write(const int, char *, ...) */



channel_type *
qlog_get_channel(
		const char	*msg
		)
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
				t = xcalloc(l + 1, 1);
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
} /* channel_type *qlog_get_channel(const char *) */



#endif /* QUICKLOG */