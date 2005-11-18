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
open_file(char *nick)
{
	char *lownick;
	char *filename;
	int fsize;
	FILE *file;

	/* Get nick in lowercase. */
	/* We could use filename as temporary buffer but that isn't safe. */
	lownick = xstrdup(nick);
	lowcase(lownick);

	/* termination and validity guaranteed */
	fsize = strlen(LOGDIR) + strlen(lownick) + strlen(cfg.logpostfix) + 3;
	filename = (char *) xmalloc(fsize);
	snprintf(filename, fsize - 1, LOGDIR"/%s%s", lownick, cfg.logpostfix);
	filename[fsize - 1] = '\0';
	file = fopen(filename, "a+");
	xfree(filename);
	xfree(lownick);

	return file;
} /* FILE *open_file(char *nick) */



/*
 * Writes entry in logfile.
 *
 * "nick" declares who we are talking with.
 * 
 * If file isn't open already, open file first.
 * (Logfiles are closed periodically in miau.c)
 */
int
privlog_write(const char *nick, const int in_out, const char *message)
{
	const char *active;
	char *t;
	int newentry = 0;
	
	privlog_type *line = NULL;

	/* First see if log is already open. */
	LLIST_WALK_H(open_logs.head, privlog_type *);
		if (xstrcmp(data->nick, nick) == 0) {
			line = data;
			LLIST_WALK_BREAK;
		}
	LLIST_WALK_F;

	if (line == NULL) {
		/* Create new entry. */
		newentry = 1;
		line = (privlog_type *) xmalloc(sizeof(privlog_type));
		line->nick = xstrdup(nick);
		line->file = NULL;
		
		/* Newly created is likely to be needed first. I think. */
		llist_add(llist_create(line), &open_logs);
	}
	
	/* If file is closed, open it. */
	if (line->file == NULL) {
		line->file = open_file(line->nick);
		if (line->file == NULL) {
			return -1;
		}
	}
	/* Update timestamp. */
	time(&line->updated);

	/* New entry? Write header. */
	if (newentry) {
		/* termination guaranteed in get_timestamp() */
		fprintf(line->file, LOGM_LOGOPEN, get_timestamp(
					NULL, TIMESTAMP_LONG));
	}

	/* Write log. */
	active = (in_out == PRIVLOG_IN) ? nick : status.nickname;
	t = log_prepare_entry(active, message);
	if (t == NULL) {
		/* termination guaranteed in get_short_localtime() */
		fprintf(line->file, LOGM_MESSAGE, get_short_localtime(),
				active, message);
	} else {
		/* termination guaranteed in log_prepare_entry() */
		fprintf(line->file, "%s", t);
	}
	fflush(line->file);

	return 0;
} /* int privlog_write(const char *nick, const int in_out,
		const char *message) */



/*
 * Close all logfiles.
 */
void
privlog_close_all(void)
{
	LLIST_WALK_H(open_logs.head, privlog_type *);
		finalize_log(data);
		llist_delete(node, &open_logs);
	LLIST_WALK_F;
} /* void privlog_close_all(void) */



/*
 * Finalize log.
 *
 * Reopen file for writing footer, then close file and free resources -
 * including structure for logfile.
 */
void
finalize_log(privlog_type *log)
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
		/* termination guaranteed in get_timestamp() */
		fprintf(log->file, LOGM_LOGCLOSE, get_timestamp(
					&log->updated, TIMESTAMP_LONG));
		fclose(log->file);
	}
	xfree(log->nick);
	xfree(log);
} /* void finalize_log(privlog_type *log) */



/*
 * Close logfile.
 */
void
privlog_close_old(void)
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
} /* void privlog_close_old(void) */



int
privlog_has_open(void)
{
	return open_logs.head != NULL;
} /* int privlog_has_open(void) */



#ifdef DUMPSTATUS
llist_list *
privlog_get_list(void)
{
	return &open_logs;
} /* privlog_get_list(void) */
#endif /* DUMPSTATUS */



#endif /* PRIVLOG */
