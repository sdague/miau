/*
 * -------------------------------------------------------
 * Copyright (C) 2002-2003 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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

#include "log.h"

#include "miau.h"
#include "messages.h"
#include "tools.h"

#ifdef LOGGING

int		global_logtype;
llist_list	log_list;

extern clientlist_type	c_clients;	/* Needed. No, really. */



/*
 * Creates a logging entry in log_list.
 */
void
log_add_rule(
		char	*channels,
		char	*file,
		int	type
	    )
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

	for (chan = strtok(channels, ","); chan; chan = strtok(NULL, ",")) {
		llist_node	*node;
		struct logentry	*logptr;
		
		/* Check we havent already done this channel */
		for (node = log_list.head; node != NULL; node = node->next) {
			logptr = node->data;
			if (xstrcasecmp(chan, logptr->channel) == 0) {
				return;
			}
		}
		
		logptr = (struct logentry *) xcalloc(
				sizeof(struct logentry), 1);
		node = llist_create(logptr);
		llist_add(node, &log_list);
		
		logptr->channel = strdup(chan);
		logptr->type = type;
		
		/*
		 * If we were not given a logfilename or we're creating more
		 * than one entry at once, create a logfilename.
		 */
		if (file == NULL || multi) {
			char	*temp_log;
			temp_log = xmalloc(strlen(chan) +
					(cfg.logpostfix != NULL ?
						strlen(cfg.logpostfix) : 0));
			sprintf(temp_log, "%s%s", chan,
					(cfg.logpostfix != NULL ?
						cfg.logpostfix : ""));
			logptr->filename = temp_log;
		}
		else {
			logptr->filename = strdup(file);
		}
	}
} 



/*
 * Delete all logging rules.
 */
void
log_del_rules(
	     )
{
	LLIST_WALK_H(log_list.head, struct logentry *);
		xfree(data->channel);
		xfree(data->filename);
		xfree(data);

		llist_delete(node, &log_list);
	LLIST_WALK_F;

}



/*
 * Open a logfile.
 */
void
log_open(
		channel_type	*channel
	)
{
	/*
	 * Should have no need for this.
	 * 
	if (channel->log->logfile != NULL) {
		return;
	}
	*/
	
	/* See if a rule applies directly to this channel. */
	LLIST_WALK_H(log_list.head, struct logentry *);
		if (xstrcasecmp(channel->name, data->channel) == 0) {
			channel->log = (struct channel_log *)
				xcalloc(sizeof(struct channel_log), 1);
			channel->log->file = fopen(data->filename, "a");
				
			if (channel->log->file == NULL) {
				return;
			}

			channel->log->type = data->type;
			break;
		}
	LLIST_WALK_F;

	/* Didn't find a direct match. Use global logtype. */
	if (channel->log == NULL && global_logtype) {
		char	*p;

		channel->log = (struct channel_log *)
			xcalloc(sizeof(struct channel_log), 1);

		p = xmalloc(strlen(channel->name) + (cfg.logpostfix != NULL ?
					strlen(cfg.logpostfix) : 0) + 1);
		sprintf(p, "%s%s", channel->name, (cfg.logpostfix != NULL ?
					cfg.logpostfix : ""));
		channel->log->file = fopen(p, "a");

		if (channel->log->file == NULL) {
			return;
		}
		xfree(p);


		channel->log->type = global_logtype;
	}

	/* Still unaware where to log ? */
	if (channel->log == NULL) {
		return;
	}
	
	/* ...and start logging. */
	log_write_entry(channel, LOGM_LOGOPEN, log_get_timestamp());
}



/*
 * Close logfile and free resources.
 */
void
log_close(
		channel_type	*channel
       )
{
	if (channel->log != NULL) {
		log_write_entry(channel, LOGM_LOGCLOSE, log_get_timestamp());
		fclose(channel->log->file);
		
		FREE(channel->log);
	}
#ifdef ENDUSERDEBUG
	else {
		enduserdebug("trying to log_close channel with no log");
	}
#endif /* ENDUSERDEBUG */
}



/*
 * Writes a logging entry to the logfile.
 */
void
log_write_entry(
		channel_type	*chptr,
		char		*format,
		...
		)
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
	vsnprintf(buffer, 1023, format, va);
	va_end(va);

	fprintf(chptr->log->file, "%s", buffer);
	fflush(chptr->log->file);
}



/*
 * Writes a logging entry to all matching logfiles.
 */
void
log_write_entry_all(
		int	type,
		char	*format,
		...
		)
{
	char		buffer[1024];
	va_list		va;

	va_start(va, format);
	vsnprintf(buffer, 1023, format, va);
	va_end(va);

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
			log_write_entry(data, "%s", buffer);
		}
	LLIST_WALK_F;
}


#endif /* LOGGING */



/*
 * creates a timestamp in the form:
 *    Sun Jan 02 11:53:33 2002
 */
char *
log_get_timestamp(
		)
{
	time_t		now;
	struct tm	*form;
	static char	stamp[100];

	time(&now);
	form = localtime(&now);
	strftime(stamp, 99, "%a %b %d %H:%M:%S %Y", form);
	
	return stamp;
}
