/*
 * -------------------------------------------------------
 * Copyright (C) 2003-2007 Tommi Saviranta <wnd@iki.fi>
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

#ifdef NEED_CMDPASSWD

#include "remote.h"
#include "error.h"
#include "common.h"
#include "miau.h"
#include "irc.h"
#include "messages.h"

#include <string.h>



/*
 * Process remote sent (thru IRC-network) commands.
 */
int
remote_cmd(const char *command, const char *params, const char *nick)
{
	/* Each command consists of up to four parameters. */
	char	**param = (char **) xmalloc((sizeof(char *)) * MAXCMDPARAMS);
	int	paramno = 0;
	char	*splitted = xstrdup(params);
	char	*p;
	int	i = 0;
	int	pass = 1;

	
	/* We know there's at least one word, so no need to check this. */
	p = strtok(splitted, " ");
	do {
		param[paramno] = p;
		paramno++;
		p = strtok(NULL, " ");
	} while (p != NULL && paramno < MAXCMDPARAMS);
	/* Set unused parameters to NULL. */
	i = paramno;
	while (i < MAXCMDPARAMS) {
		param[i] = NULL;
		i++;
	}
	
	/*
	 * Because we don't know what features are compiled in, we can't use
	 * "else if" at all.
	 */
#ifdef RELEASENICK
	if (xstrcmp(command, "NICK") == 0 && paramno == 2) {
		int	time = atoi(param[1]);
		if (time < 10) { time = 10; }
		if (strlen(param[0]) > 0) {
			irc_notice(&c_server, nick, MIAU_RELEASENICK,
					param[0], time);
			irc_write(&c_server, "NICK %s", param[0]);
			report(MIAU_RELEASENICK, param[0], time);
			timers.nickname = -time;
			pass = 0;
		}
	}
#endif /* ifdef RELEASENICK */
	
	/* Free parameter-list. */
	xfree(param);
	xfree(splitted);

	return pass;
} /* int remote_cmd(const char *command, const char *params,
		const char *nick) */



#endif /* ifdef NEED_CMDPASSWD */
