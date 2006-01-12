/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2002-2006 Tommi Saviranta <wnd@iki.fi>
 *	(C) 2002 Lee Hardy <lee@leeh.co.uk>
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

#ifdef CHANLOG

#include "chanlog.h"
#include "llist.h"
#include "common.h"
#include "channels.h"
#include "client.h"
#include "miau.h"
#include "tools.h"
#include "messages.h"
#include "etc.h"
#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>



int		global_logtype;
llist_list	chanlog_list;



/*
 * Creates a logging entry in log_list.
 */
void
chanlog_add_rule(char *channels, char *file, int type)
{
	char	*chan;
	int	multi = 0;

	/* Nothing to log. */
	if (channels == NULL || channels[0] == '\0') {
		return;
	}

	/*
	 * The "global" logentry, mark the logtype and then open logfiles
	 * for channels we dont have specific entries for.
	 */
	if (xstrcmp(channels, "*") == 0) {
		global_logtype = type;
		return;
	}

	/*
	 * If we're doing multi channels, mark as such so we don't use a
	 * specified logfile, revert to #channel.log.
	 */
	if (strchr(channels, ',') != NULL) {
		multi = 1;
	}

	for (chan = strtok(channels, ","); chan != NULL;
			chan = strtok(NULL, ",")) {
		llist_node *node;
		struct chanlogentry *logptr;

		/* Check we haven't already done this channel */
		for (node = chanlog_list.head; node != NULL;
				node = node->next) {
			logptr = (struct chanlogentry *) node->data;
			if (xstrcasecmp(chan, logptr->channel) == 0) {
				return;
			}
		}

		logptr = (struct chanlogentry *)
			xcalloc(sizeof(struct chanlogentry), 1);
		node = llist_create(logptr);
		llist_add(node, &chanlog_list);

		logptr->channel = xstrdup(chan);
		logptr->type = type;

		/*
		 * If we were not given a logfilename or we're creating more
		 * than one entry at once, create a logfilename.
		 */
		if (file == NULL || multi) {
			char *file;
			size_t size;
			/* termination and validity guaranteed */
			size = strlen(LOGDIR) + strlen(chan)
				+ strlen(cfg.logsuffix) + 3;
			file = (char *) xmalloc(size);
			snprintf(file, size, LOGDIR"/%s%s",
					chan, cfg.logsuffix);
			file[size - 1] = '\0';
			logptr->filename = file;
		} else { /* basically "if (file != NULL)" */
			size_t size;
			/* If filename is relative, add LOGDIR. */
			/* termination and validity guaranteed */
			size = strlen(LOGDIR) + strlen(file) + 3;
			logptr->filename = (char *) xmalloc(size);
			snprintf(logptr->filename, size, LOGDIR"/%s",
					file);
			logptr->filename[size - 1] = '\0';
		}
	}
} /* void chanlog_add_rule(char *channels, char *file, int type) */



/*
 * Delete all logging rules.
 */
void
chanlog_del_rules(void)
{
	LLIST_WALK_H(chanlog_list.head, struct chanlogentry *);
		xfree(data->channel);
		xfree(data->filename);
		xfree(data);

		llist_delete(node, &chanlog_list);
	LLIST_WALK_F;

} /* void chanlog_del_rules(void) */



/*
 * Open a logfile.
 */
void
chanlog_open(channel_type *channel)
{
	/*
	 * Should have no need for this.
	 *
	if (channel->log->logfile != NULL) {
		return;
	}
	*/

	/* See if a rule applies directly to this channel. */
	LLIST_WALK_H(chanlog_list.head, struct chanlogentry *);
		if (xstrcasecmp(channel->simple_name, data->channel) == 0) {
			channel->log = (struct channel_log *)
				xcalloc(sizeof(struct channel_log), 1);
			/* filename termination and validity guaranteed */
			channel->log->file = fopen(data->filename, "a");

			if (channel->log->file == NULL) {
				log_cannot_write(data->filename);
				return;
			}

			channel->log->type = data->type;
			break;
		}
	LLIST_WALK_F;

	/* Didn't find a direct match. Use global logtype. */
	if (channel->log == NULL && global_logtype) {
		size_t plen;
		char *p;
		char *lowchan;

		/* convert simple channel name to lowercase */
		lowchan = xstrdup(channel->simple_name);
		lowcase(lowchan);

		channel->log = (struct channel_log *)
			xcalloc(sizeof(struct channel_log), 1);

		/* termination and validity guaranteed */
		plen = strlen(LOGDIR) + strlen(lowchan)
			+ strlen(cfg.logsuffix) + 3;
		p = (char *) xmalloc(plen);
		snprintf(p, plen, LOGDIR"/%s%s", lowchan, cfg.logsuffix);
		p[plen - 1] = '\0';
		channel->log->file = fopen(p, "a");
		xfree(p);
		xfree(lowchan);

		if (channel->log->file == NULL) {
			return;
		}

		channel->log->type = global_logtype;
	}

	/* Still unaware where to log ? */
	if (channel->log == NULL) {
		return;
	}

	/* ...and start logging. */
	chanlog_write_entry(channel, LOGM_LOGOPEN,
			get_timestamp(NULL, TIMESTAMP_LONG));
} /* void chanlog_open(channel_type *channel) */



/*
 * Close logfile and free resources.
 */
void
chanlog_close(channel_type *channel)
{
	if (channel->log != NULL) {
		chanlog_write_entry(channel, LOGM_LOGCLOSE,
				get_timestamp(NULL, TIMESTAMP_LONG));
		if (channel->log->file != NULL) {
		fclose(channel->log->file);
		}

		FREE(channel->log);
	}
} /* void chanlog_close(channel_type *channel) */



/*
 * Writes a logging entry to the logfile.
 */
void
chanlog_write_entry(channel_type *chptr, char *format, ...)
{
	char	buffer[1024];
	va_list	va;

	/* No logfile for this channel. */
	if (chptr->log == NULL || chptr->log->file == NULL) {
		return;
	}

	/* Probably not logging when client is attached/detached. */
	if ((c_clients.connected == 0 &&
			! (chptr->log->type & LOG_DETACHED)) ||
			(c_clients.connected > 0 &&
				! (chptr->log->type & LOG_ATTACHED))) {
		return;
	}

	va_start(va, format);
	vsnprintf(buffer, 1024, format, va);
	va_end(va);
	buffer[1023] = '\0';

	fprintf(chptr->log->file, "%s", buffer);
	fflush(chptr->log->file);
} /* void chanlog_write_entry(channel_type *chptr, char *format, ...) */



/*
 * Writes a logging entry to all matching logfiles.
 */
void
chanlog_write_entry_all(int type, char *format, ...)
{
	char		buffer[1024];
	va_list		va;

	va_start(va, format);
	vsnprintf(buffer, 1024, format, va);
	va_end(va);
	buffer[1023] = '\0';

	LLIST_WALK_H(active_channels.head, channel_type *);
		/*
		 * We could have it this way...
		 *
		if (data->log == NULL || data->log->file == NULL) {
			LLIST_WALK_CONTINUE;
		}

		if (data->log->type & type) {
			log_write_entry(data, "%s", buffer);
		}
		 *
		 * ...but this should compile shorter. ;-)
		 */
		if (! (data->log == NULL || data->log->file == NULL)
				&& data->log->type & type) {
			chanlog_write_entry(data, "%s", buffer);
		}
	LLIST_WALK_F;
} /* void chanlog_write_entry_all(int type, char *format, ...) */



int
chanlog_has_log(const channel_type *chan, int type)
{
	if (chan == NULL || chan->log == NULL) {
		return 0;
	} else {
		return (chan->log->type & type) == type;
	}
} /* int chanlog_has_log(const channel_type *chan, int type) */



#endif /* ifdef CHANLOG */
