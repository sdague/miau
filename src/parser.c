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

#include "miau.h"
#include "parser.h"

#include "messages.h"
#include "perm.h"
#include "tools.h"
#include "channels.h"
#ifdef LOGGING
#  include "log.h"
#endif /* LOGGING */
#ifdef ONCONNECT
#  include "onconnect.h"
#endif /* ONCONNECT */


static int	virgin = 1;	/* CFG_CHANNELS has effect only at start up. */

int	line;			/* Line we're processing. */
int	listid = CFG_NOLIST;	/* List ID. */



/*
 * Return pointer to new value.
 *
 * New value is trimmed out of extra spaces.
 */
char *
assign_param(
		char	*target,
		char	*source
	    )
{
	if (target != NULL) {
		free(target);
	}
	source = trim(source, SPACES);
	return (strlen(source) != 0 ? strdup(source) : NULL);
} /* char *assign_param(char *, char *) */



/*
 * Add server to server-list.
 */
void
add_server(
		const char	*name,
		int		port,
		const char	*pass,
		int		timeout
	  )
{
	server_type	*server;
	llist_node	*node;

	/* Must have a name for this server. */
	if (! name) { return; }
	/* If port was not defined, use default. */
	if (port == 0) { port = DEFAULT_PORT; }

	/*
	 * We don't check if user has duplicate servers. If he does, he
	 * probably has a reason for that.
	 */

	server = (server_type *) xmalloc(sizeof(server_type));
	server->name = strdup(name);
	server->port = port;
	server->password = (pass == NULL) ? NULL : strdup(pass);
	server->timeout = timeout;
	server->working = 1;
	node = llist_create(server);
	llist_add_tail(node, &servers.servers);
	servers.amount++;
} /* void add_server(const char *, int, const char *, int) */



/*
 * Remove quotes around *data.
 * 
 * Return pointer to trimmed string or NULL if there are no quotes.
 */
char *
trimquotes(
		char	*data
	  )
{
	int	l = strlen(data) - 1;
	if (data[0] != '"' || data[l] != '"') {
		parse_error();		/* Bad quoting. */
		return NULL;
	}
	data[l] = '\0';
	return data + 1;
} /* char *trimquotes(char *) */



/*
 * Remove useless characters like whitespaces and comments.
 *
 * Return pointer to trimmed string.
 */
char *
trim(
		char		*data,
		const int	mode
    )
{
	int	inside = 0;
	char	*ptr;
	
	/* Skip whitespaces. */
	while (*data == ' ' || *data == '\t') { data++; }

	if (mode == LINE) {
		/* Remove comments. */

		ptr = data;
		while (*ptr != '\0') {
			if (*ptr == '"') {
				inside ^= 1;
			} else if (*ptr == '#' && ! inside) {
				*ptr = '\0';
				break;
			}
			ptr++;
		}
	}
			
	ptr = data + strlen(data) - 1;
	/* Remove trailing whitespaces. */
	while (ptr >= data && (*ptr == ' ' || *ptr == '\t')) {
		*ptr = '\0';
		ptr--;
	}
	
	return data;
} /* char *trim(char *, const int) */



/*
 * Get an integer out of data.
 *
 * Return parsed integer.
 */
int
parse_int(
		const char	*data,
		const int	min
	 )
{
	int	n;

	if (data != NULL) {
		n = atoi(data);
	
		/*
		 * We could do something like this, but we really don't care
		 * about errors this time... Lets just use atoi().
		 *
		n = strtol(data, (char **) NULL, 10);
		if (errno == ERANGE) {
			report("bad value");
		}
		 */
		
		if (n < min) { n = min; }
	} else {
		n = min;
	}

	return n;
} /* int parse_int(const char *, const int) */



/*
 * Parse boolean value out of data.
 *
 * Return 0 or 1 if 'data' represented false or true respectively.
 *
 * Strings beginning with 'y', 't' and '1' are concidered as 'true' and the
 * rest, 'false'.
 */
int
parse_boolean(
		const char	*data
	     )
{
	if (*data == 'y' || *data == 't' || *data == '1') {
		return 1;
	}
	return 0;
} /* int parse_boolean(const char *) */



/*
 * Parse parameter definition that was already extracted from configuration
 * file.
 */
void
parse_param(
		char	*data
	   )
{
	char	*t = strchr(data, '=');
	char	*val;
	if (t == NULL) {
		parse_error();	/* Didn't get mandatory '='-character. */
		return;
	}
	*t = '\0';

	data = trim(data, SPACES);
	if (strchr(data, ' ') != NULL || strchr(data, '\t') != NULL) {
		parse_error();
		return;
	}
	
	val = trim(t + 1, SPACES);
	if (*val == '{') {
		/* Not expecting any other data. */
		if (val[1] != '\0') { parse_error(); }
		
		/* Resolve list-id. */
		if (xstrcmp(data, "nicknames") == 0) {		/* nicknames */
			listid = CFG_NICKNAMES;
		} else if (xstrcmp(data, "servers") == 0) {	/* servers */
			listid = CFG_SERVERS;
		} else if (xstrcmp(data, "connhosts") == 0) {	/* connhosts */
			listid = CFG_CONNHOSTS;
		} else if (xstrcmp(data, "ignore") == 0) {	/* ignore */
			listid = CFG_IGNORE;
#ifdef AUTOMODE
		} else if (xstrcmp(data, "automodes") == 0) {	/* automode */
			listid = CFG_AUTOMODELIST;
#endif /* AUTOMODE */
#ifdef LOGGING
		} else if (xstrcmp(data, "log") == 0) {		/* log */
			listid = CFG_LOG;
#endif /* LOGGING */
		} else if (xstrcmp(data, "channels") == 0) {	/* channels */
			listid = CFG_CHANNELS;
#ifdef ONCONNECT
		} else if (xstrcmp(data, "onconnect") == 0) {	/* onconnect */
			listid = CFG_ONCONNECT;
#endif /* ONCONNECT */
		} else {
			listid = CFG_INVALID;
			parse_error();
		}
		return;
	}
	val = trimquotes(val);

	if (xstrcmp(data, "realname") == 0) {		/* realname */
		cfg.realname = assign_param(cfg.realname, val);
	} else if (xstrcmp(data, "username") == 0) {	/* username */
		cfg.username = assign_param(cfg.username, val);
#ifdef _NEED_CMDPASSWD
	} else if (xstrcmp(data, "cmdpasswd") == 0) {	/* cmdpasswd */
		cfg.cmdpasswd = assign_param(cfg.cmdpasswd, val);
#endif /* _NEED_CMDPASSWD */
#ifdef QUICKLOG
	} else if (xstrcmp(data, "qloglength") == 0) {	/* qloglength */
		cfg.qloglength = parse_int(val, 0);
	} else if (xstrcmp(data, "flushqlog") == 0) {	/* flushqlog */
		cfg.flushqlog = parse_boolean(val);
#endif /* QUICKLOG */
#ifdef LOGGING
	} else if (xstrcmp(data, "logpostfix") == 0) {	/* logpostfix */
		cfg.logpostfix = assign_param(cfg.logpostfix, val);
#endif /* LOGGING */
#ifdef PRIVMSGLOG
	} else if (xstrcmp(data, "logging") == 0) {	/* logging */
		cfg.logging = parse_boolean(val);
#endif /* PRIVMSGLOG */
	} else if (xstrcmp(data, "listenport") == 0) {	/* listenport */
		cfg.listenport = parse_int(val, 0);
	} else if (xstrcmp(data, "listenhost") == 0) {	/* listenhost */
		cfg.listenhost = assign_param(cfg.listenhost, val);
	} else if (xstrcmp(data, "password") == 0) {	/* password */
		cfg.password = assign_param(cfg.password, val);
	} else if (xstrcmp(data, "leave") == 0) {	/* leave */
		cfg.leave = parse_boolean(val);
	} else if (xstrcmp(data, "leavemsg") == 0) {	/* leavemsg */
		cfg.leavemsg = assign_param(cfg.leavemsg, val);
	} else if (xstrcmp(data, "awaymsg") == 0) {	/* awaymsg */
		cfg.awaymsg = assign_param(cfg.awaymsg, val);
	} else if (xstrcmp(data, "usequitmsg") == 0) {	/* usequitmsg */
		cfg.usequitmsg = parse_boolean(val);
	} else if (xstrcmp(data, "getnick") == 0) {	/* getnick */
		cfg.getnick = parse_int(val, 0);
	} else if (xstrcmp(data, "getnickinterval") == 0) {/* getnickinterval */
		cfg.getnickinterval = parse_int(val, 0);
	} else if (xstrcmp(data, "bind") == 0) {	/* bind */
		cfg.bind = assign_param(cfg.bind, val);
#ifdef AUTOMODE
	} else if (xstrcmp(data, "automodedelay") == 0) { /* automodedelay */
		cfg.automodedelay = parse_int(val, 0);
#endif /* AUTOMODE */
	} else if (xstrcmp(data, "antiidle") == 0) {	/* antiide */
		cfg.antiidle = parse_int(val, 0);
	} else if (xstrcmp(data, "nevergiveup") == 0) {	/* nevergiveup */
		cfg.nevergiveup = parse_boolean(val);
	} else if (xstrcmp(data, "norestricted") == 0) { /* norestricted */
		cfg.jumprestricted = parse_boolean(val);
	} else if (xstrcmp(data, "stonedtimeout") == 0) { /* stonedtimeout*/
		cfg.stonedtimeout = parse_int(val, MINSTONEDTIMEOUT);
	} else if (xstrcmp(data, "connecttimeout") == 0) { /* connecttimeout */
		cfg.connecttimeout = parse_int(val, MINCONNECTTIMEOUT);
	} else if (xstrcmp(data, "reconnectdelay") == 0) { /* reconnectdelay */
		cfg.reconnectdelay = parse_int(val, MINRECONNECTDELAY);
	} else if (xstrcmp(data, "rejoin") == 0) {	/* rejoin */
		cfg.rejoin = parse_boolean(val);
	} else if (xstrcmp(data, "forwardmsg") == 0) {	/* forwardmsg */
		cfg.forwardmsg = assign_param(cfg.forwardmsg, val);
	} else if (xstrcmp(data, "maxclients") == 0) {	/* maxclients */
		cfg.maxclients = parse_int(val, 1);
#ifdef DCCBOUNCE
	} else if (xstrcmp(data, "dccbounce") == 0) {	/* dccbounce */
		cfg.dccbounce = parse_boolean(val);
	} else if (xstrcmp(data, "dccbindhost") == 0) {
		cfg.dccbindhost = assign_param(cfg.dccbindhost, val);
#endif /* DCCBOUNCE */
	} else if (xstrcmp(data, "nickfillchar") == 0) { /* nickfillchar */
		cfg.nickfillchar = val[0];
	} else if (xstrcmp(data, "usermode") == 0) {	/* usermode */
		cfg.usermode = assign_param(cfg.usermode, val);
	} else if (xstrcmp(data, "maxnicklen") == 0) {	/* maxnicklen */
		cfg.maxnicklen = parse_int(val, 3);
	} else {
		parse_error();
	}
} /* void parse_param(char *) */



/*
 * Parse list-item.
 */
void
parse_list_line(
		char	*data
	       )
{
	permlist_type	*permlist = NULL;
	char		**param;
	int		paramcount = 0;
	int		n;
	int		eol = 0;
	int		ok = 0;
	
	int		inside = 0;		/* Inside quotes. */
	char		*ptr;
	char		*par;
#ifdef LOGGING
	int		logtype;
#endif

	/* Starting a new list, eh ? */
	if (strchr(data, '=') != NULL && strchr(data, '{') != NULL) {
		parse_error();
		listid = CFG_INVALID;
		parse_param(data);
		return;
	}

	/* Ending a list. */
	if (data[0] == '}') {
		/* No trailing data, thank you. */
		if (data[1] != '\0') { parse_error(); }
		listid = CFG_NOLIST;
		return;
	}

	/* Read parameters. */
	
	/* We can't use strtok(), because it eats subsequent delimeters. */
	param = xmalloc(sizeof(char *) * MAXNUMOFPARAMS);
	ptr = data;
	par = ptr;
	
	/* Initially set all parameters to NULL. */
	for (n = 0; n < MAXNUMOFPARAMS; n++) {
		param[n] = NULL;
	}

	/*
	 * This is ugly; we parse line until '\0', which _breaks out_ from
	 * while(1)-loop...
	 */
	do {
		/* Toggle "inside quotes" -status and remove quotes. */
		if (*ptr == '"') {
			inside++;
		}

		/* End of line ? */
		else if (*ptr == '\0') { eol = 1; }


		/* Got end of parameter. */
		if ((*ptr == ':' || *ptr == '\0') &&
				(inside == 0 || inside == 2)) {
			*ptr = '\0';

			par = trim(par, SPACES);
			if (strlen(par) > 0) {
				if (par[strlen(par) - 1] != '"' ||
						par[0]  != '"') {
					parse_error();
					return;
				}
				par++;
				par[strlen(par) - 1] = '\0';
			
				/* Ok, got our parameter. */
				param[paramcount] = strdup(par);
			}
			paramcount++;
			par = ptr + 1;
			inside = 0;
		}
		
		ptr++;
	} while (! eol);
	/* If still inside quotes, the line was bad. */
	if (inside) {
		parse_error();
		return;
	}
	/* We want at least one parameter. */
	if (paramcount == 0) {
		parse_error();
		return;
	}

	/* Process parameters. */
	switch (listid) {
		case CFG_NICKNAMES:
			if (paramcount == 1) {
				llist_add_tail(llist_create(strdup(param[0])),
						&nicknames.nicks);
				ok = 1;
			}
			break;
			
		case CFG_SERVERS:
			if (paramcount <= 4) {
				add_server(param[0], parse_int(param[1], 0),
						param[2], parse_int(param[3],
							0));
				ok = 1;
			}
			break;
			
		case CFG_CONNHOSTS:
			if (paramcount <= 2) {
				permlist = &connhostlist;
				ok = 1;
			}
			break;
			
		case CFG_IGNORE:
			if (paramcount <= 2) {
				permlist = &ignorelist;
				ok = 1;
			}
			break;
			
#ifdef AUTOMODE
		case CFG_AUTOMODELIST:
			if (paramcount <= 2 && param[0][1] == ':') {
				permlist = &automodelist;
				ok = 1;
			}
			break;
#endif /* AUTOMODE */
			
#ifdef LOGGING
		case CFG_LOG:
			if (paramcount < 2 || paramcount > 3) {
				break;
			}
			logtype = 0;
			ptr = param[1];
			while (*ptr != '\0') {
				switch (*ptr) {
					case LOG_MESSAGE_C:
						logtype |= LOG_MESSAGE;
						break;
					case LOG_JOIN_C:
						logtype |= LOG_JOIN;
						break;
					case LOG_PART_C:
						logtype |= LOG_PART;
						break;
					case LOG_QUIT_C:
						logtype |= LOG_QUIT;
						break;
					case LOG_MODE_C:
						logtype |= LOG_MODE;
						break;
					case LOG_NICK_C:
						logtype |= LOG_NICK;
						break;
					case LOG_MISC_C:
						logtype |= LOG_MISC;
						break;
					case LOG_MIAU_C:
						logtype |= LOG_MIAU;
						break;
					case LOG_ALL_C:
						logtype |= LOG_ALL;
						break;
					case LOG_ATTACHED_C:
						logtype |= LOG_ATTACHED;
						break;
					case LOG_DETACHED_C:
						logtype |= LOG_DETACHED;
						break;
					case LOG_CONTIN_C:
						logtype |= LOG_CONTIN;
						break;
					default:
						parse_error();
						break;
				}
				ptr++;
			}
			/*
			 * If no LOG_ATTACHED nor LOG_DETACHED was
			 * defined, use default: LOG_CONTIN
			 */
			if (! (logtype & LOG_ATTACHED) &&
					! (logtype & LOG_DETACHED)) {
				logtype |= LOG_CONTIN;
			}
			log_add_rule(param[0], param[2], logtype);
			ok = 1;
			break;
#endif /* LOGGING */

		case CFG_CHANNELS:
			if (paramcount <= 2) {
				/* CFG_CHANNELS only has effect at start up. */
				if (virgin) {
					channel_type	*channel;
					if (param[0] == NULL ||
							strlen(param[0]) == 0) {
						break;
					}
					/* Not adding same channel twice. */
					if (channel_find(param[0], LIST_PASSIVE)
							!= NULL) {
						break;
					}
					/* channel_add will set up us a key. */
					channel = channel_add(param[0],
							LIST_PASSIVE);
				}
				ok = 1;
			}
			break;

#ifdef ONCONNECT
		case CFG_ONCONNECT:
			if (paramcount <= 3) {
				if (*param[0] == 'p' || *param[0] == 'n' ||
						*param[0] == 'r') {
					onconnect_add(*(param[0]), param[1],
							param[2]);
					ok = 1;
				}
			}
			break;
#endif /* ONCONNECT */
		case CFG_INVALID:
			ok = 1;
			break;
	}

	/* Did we make it ? */
	if (! ok) {
		parse_error();
	} else if (permlist != NULL) {
		/* Everything went just fine and there's something to do... */
		if (paramcount == 1) {
			add_perm(permlist, strdup(param[0]), 1);
		} else if (paramcount == 2) {
			add_perm(permlist, strdup(param[0]),
					parse_boolean(param[1]));
		}
	}

	/* Free parameters. */
	while (paramcount > 0) {
		paramcount--;
		xfree(param[paramcount]);
	}
	xfree(param);
} /* void parse_list_line(char *) */



/*
 * Parse configuration file.
 */
int
parse_cfg(
		const char	*cfgfile
	     )
{
	FILE		*file;
	int		filelen;
	
	char		*buf;
	char		*bufptr;
	char		*nextptr;

	buf = malloc(READBUFSIZE);

	line = 1;
	file = fopen(cfgfile, "r");
	if (file == NULL) {
		return -1;
	}

	/* Make sure configuration-file ends. */
	filelen = fread(buf, 1, READBUFSIZE - 2, file);
	buf[filelen] = '\n';
	buf[filelen + 1] = '\0';
	
	bufptr = buf;
	nextptr = strchr(buf, '\n');
	while (nextptr >= bufptr) {
		*nextptr = '\0';
		bufptr = trim(bufptr, LINE);
		if (strlen(bufptr) > 0 && *bufptr != '#') {
			if (listid == CFG_NOLIST) {
				parse_param(bufptr);
			} else {
				parse_list_line(bufptr);
			}
		}
		bufptr = nextptr + 1;
		nextptr = strchr(bufptr, '\n');
		line++;
	}

	if (listid != CFG_NOLIST) {
		parse_error();	/* Unfinished list. */
	}

	fclose(file);
	xfree(buf);

	virgin = 0;	/* Lost virginity. ;-) */

	return 0;
} /* int parse_cfg(const char *) */



void
parse_error(
	   )
{
	error(PARSE_SE, line);
} /* void parse_error() */
