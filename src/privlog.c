/*
 * -------------------------------------------------------
 * Copyright (C) 2004 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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

#include "privlog.h"

#include "miau.h"
#include "messages.h"
#include "tools.h"
#include "log.h"



#ifdef PRIVLOG

llist_list	open_logs;



FILE *open_file(char *nick);
void finalize_log(privlog_type *);



FILE *
open_file(
		char	*nick
	 )
{
	char *lownick;
	char *filename;
	FILE *file;

	/* Get nick in lowercase. */
	/* We could use filename as temporary buffer but that isn't safe. */
	lownick = strdup(nick);
	lowcase(lownick);
	
	filename = xmalloc(strlen(LOGDIR) + strlen(lownick)
			+ strlen(cfg.logpostfix) + 2);
	sprintf(filename, LOGDIR"/%s%s", lownick, cfg.logpostfix);
	file = fopen(filename, "a+");
	xfree(filename);
	xfree(lownick);

	return file;
} /* FILE *open_file(char *) */



/*
 * Writes entry in logfile.
 *
 * "nick" declares who we are talking with.
 * 
 * If file isn't open already, open file first.
 * (Logfiles are closed periodically in miau.c)
 */
int
privlog_write(
		const char	*nick,
		const int	in_out,
		const char	*message
	     )
{
	const char *active;
	char *t;
	int newentry = 0;
	
	privlog_type *entry = NULL;

	/* First see if log is already open. */
	LLIST_WALK_H(open_logs.head, privlog_type *);
		if (xstrcmp(data->nick, nick) == 0) {
			entry = data;
			LLIST_WALK_BREAK;
		}
	LLIST_WALK_F;

	if (entry == NULL) {
		/* Create new entry. */
		newentry = 1;
		entry = xmalloc(sizeof(privlog_type));
		entry->nick = strdup(nick);
		entry->file = NULL;
		
		/* Newly created is likely to be needed first. I think. */
		llist_add(llist_create(entry), &open_logs);
	}
	
	/* If file is closed, open it. */
	if (entry->file == NULL) {
		entry->file = open_file(entry->nick);
		if (entry->file == NULL) {
			return -1;
		}
	}
	/* Update timestamp. */
	time(&entry->updated);

	/* New entry? Write header. */
	if (newentry) {
		fprintf(entry->file, LOGM_LOGOPEN, get_timestamp(
					TIMESTAMP_NOW, TIMESTAMP_LONG));
	}

	/* Write log. */
	active = (in_out == PRIVLOG_IN) ? nick : status.nickname;
	t = log_prepare_entry(active, message);
	if (t == NULL) {
		fprintf(entry->file, LOGM_MESSAGE, get_short_localtime(),
				active, message);
	} else {
		fprintf(entry->file, "%s", t);
	}
	fflush(entry->file);

	return 0;
} /* int privlog_write(const char *, const int, const char *) */



/*
 * Close all logfiles.
 */
void
privlog_close_all(
		)
{
	LLIST_WALK_H(open_logs.head, privlog_type *);
		finalize_log(data);
		llist_delete(node, &open_logs);
	LLIST_WALK_F;
} /* void privlog_close_all() */



/*
 * Finalize log.
 *
 * Reopen file for writing footer, then close file and free resources -
 * including structure for logfile.
 */
void
finalize_log(
		privlog_type	*log
	    )
{
	/*
	 * Remove entry from list. This allows writing new headers into file
	 * when conversation continues.
	 */
	/* Reopen closed file for writing footer. */
	if (log->file == NULL) {
		log->file = open_file(log->nick);
	}
	if (log->file != NULL) {
		fprintf(log->file, LOGM_LOGCLOSE, get_timestamp(
					log->updated, TIMESTAMP_LONG));
		fclose(log->file);
	}
	xfree(log->nick);
	xfree(log);
} /* void finalize_log(privlog_type *) */



/*
 * Close logfile.
 */
void
privlog_close_old(
		)
{
	time_t close;
	time_t loggrace;
	close = time(NULL) - PRIVLOG_TIME_OPEN;
	loggrace = time(NULL) - PRIVLOG_TIME_GRACE;
	
	LLIST_WALK_H(open_logs.head, privlog_type *);
		if (data->updated < loggrace) {
			finalize_log(data);
			llist_delete(node, &open_logs);
		} else if (data->updated < close && data->file != NULL) {
			/*
			 * Close file but remember it. If converastion continues
			 * in a while, new headers will not be written.
			 */
			fclose(data->file);
			data->file = NULL;
		}
	LLIST_WALK_F;
} /* void privlog_close_old() */



int
privlog_has_open(
		)
{
	return open_logs.head != NULL;
} /* int privlog_has_open() */



#ifdef DUMPSTATUS
llist_list *
privlog_get_list(
		)
{
	return &open_logs;
} /* privlog_get_list() */
#endif /* DUMPSTATUS */



#endif /* PRIVLOG */
