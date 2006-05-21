/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2003-2006 Tommi Saviranta <wnd@iki.fi>
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

#include "parser.h"
#include "common.h"
#include "server.h"
#include "miau.h"
#include "perm.h"
#include "error.h"
#include "messages.h"
#include "chanlog.h"
#include "onconnect.h"

#include <errno.h>
#include <string.h>



static int	virgin = 1;	/* CFG_CHANNELS has effect only at start up. */

int	line;			/* Line we're processing. */
int	listid = CFG_NOLIST;	/* List ID. */



/* Prototypes. */
void assign_int(int *target, const char *data, const int min);
void assign_boolean(int *target, const char *data);
void assign_param(char **target, char *source);
void parse_error(void);
char *trim(char *data, const int mode);
char *trimquotes(char *data);
void assign_option(int *target, const char *, char *);
int parse_option(const char *, char *);



/*
 * Writes "source" over "target".
 *
 * Old data if freed and new data is trimmed.
 */
void
assign_param(char **target, char *source)
{
	xfree(*target);
	source = trim(source, SPACES);
	if (source[0] == '\0') {
		*target = NULL;
	} else {
		*target = xstrdup(source);
	}
} /* void assign_param(char **target, char *source) */



/*
 * Add server to server-list.
 */
void
add_server(const char *name, int port, const char *pass, int timeout)
{
	server_type	*server;
	llist_node	*node;

	/* Must have a name for this server. */
	if (name == NULL) { return; }
	/* If port was not defined, use default. */
	if (port == 0) { port = DEFAULT_PORT; }

	/*
	 * We don't check if user has duplicate servers. If he does, he
	 * probably has a reason for that.
	 */

	server = (server_type *) xmalloc(sizeof(server_type));
	server->name = xstrdup(name);
	server->port = port;
	server->password = (char *) ((pass == NULL) ? NULL : xstrdup(pass));
	server->timeout = (timeout > 0) ? timeout : 0;
	server->working = 1;
	node = llist_create(server);
	llist_add_tail(node, &servers.servers);
	servers.amount++;
} /* void add_server(const char *name, int port, const char *pass,
		int timeout) */



/*
 * Remove quotes around *data.
 * 
 * Return pointer to trimmed string or NULL if there are no quotes.
 */
char *
trimquotes(char *data)
{
	int l;
	l = strlen(data) - 1;
	if (data[0] != '"' || data[l] != '"') {
		parse_error();		/* Bad quoting. */
		return NULL;
	}
	data[l] = '\0';
	return data + 1;
} /* char *trimquotes(char *data) */



/*
 * Remove useless characters like whitespaces and comments.
 *
 * Return pointer to trimmed string.
 */
char *
trim(char *data, const int mode)
{
	int	inside = 0;
	char	*ptr;
	
	/* Skip whitespaces (minus linefeeds, they cannot exist). */
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
} /* char *trim(char *data, const int mode) */



/*
 * Get an integer out of data. Set it over target.
 */
void
assign_int(int *target, const char *data, const int min)
{
	int n = min; /* Default to min. */

	if (data != NULL) {
		n = strtol(data, (char **) NULL, 10);
		if (errno == ERANGE) {
			parse_error();
			return;
		}
		
		if (n < min) { n = min; }
	}

	*target = n;
} /* void assign_int(int *target, const char *data, const int min) */



/*
 * Parse boolean value out of data.
 *
 * "true", "yes", "on", and "1" gives 1,
 * "false, "no", off", and "0" gives 0,
 * everything else prints an error.
 */
void
assign_boolean(int *target, const char *data)
{
	if (parse_option(data, "true\0yes\0on\0""1\0\0") != -1) {
		*target = 1;
	} else if (parse_option(data, "false\0no\0off\0""0\0\0") != -1) {
		*target = 0;
	} else {
		parse_error();
	}
} /* void assign_boolean(int *target, const char *data) */



/*
 * Assign option. See parse_option(...).
 *
 * This prints error if val was not found.
 */
void
assign_option(int *target, const char *val, char *options)
{
	int t = parse_option(val, options);
	if (t == -1) {
		parse_error();
	} else {
		*target = t;
	}
} /* void assign_option(int *target, const char *val, char *options) */



/*
 * Parse option.
 *
 * Find "val" in "options", which is null-separated, (double) null-terminated
 * list of possible options. Index of word in "options" will define returned
 * value.
 *
 * Returns -1 if "val" is not found.
 */
int
parse_option(const char *val, char *options)
{
	int i = 0;
	/* Bad input means error in config. */
	if (val == NULL || val[0] == '\0') {
		return -1;
	}
	while (*options != '\0') {
		if (xstrcmp(val, options) == 0) {
			return i;
		}
		options = strchr(options, (int) '\0') + 1;
		i++;
	}
	return -1;
} /* int parse_option(const char *val, char *options) */



/*
 * Parse parameter definition that was already extracted from configuration
 * file.
 */
void
parse_param(char *data)
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
#endif /* ifdef AUTOMODE */
#ifdef CHANLOG
		} else if (xstrcmp(data, "chanlog") == 0) {	/* chanlog */
			listid = CFG_CHANLOG;
#endif /* ifdef CHANLOG */
		} else if (xstrcmp(data, "channels") == 0) {	/* channels */
			listid = CFG_CHANNELS;
#ifdef ONCONNECT
		} else if (xstrcmp(data, "onconnect") == 0) {	/* onconnect */
			listid = CFG_ONCONNECT;
#endif /* ifdef ONCONNECT */
		} else {
			listid = CFG_INVALID;
			parse_error();
		}
		return;
	}
	val = trimquotes(val);

	if (xstrcmp(data, "realname") == 0) {		/* realname */
		assign_param(&cfg.realname, val);
	} else if (xstrcmp(data, "username") == 0) {	/* username */
		assign_param(&cfg.username, val);
#ifdef NEED_CMDPASSWD
	} else if (xstrcmp(data, "cmdpasswd") == 0) {	/* cmdpasswd */
		assign_param(&cfg.cmdpasswd, val);
#endif /* ifdef NEED_CMDPASSWD */
#ifdef QUICKLOG
	} else if (xstrcmp(data, "qloglength") == 0) {	/* qloglength */
		assign_int(&cfg.qloglength, val, 0);
#ifdef QLOGSTAMP
	} else if (xstrcmp(data, "timestamp") == 0) {	/* timestamp */
		/* See qlog.h for options' order. */
		/* Double-terminate just to be sure. */
		assign_option(&cfg.timestamp, val,
				"none\0beginning\0end\0\0");
#endif /* ifdef QLOGSTAMP */
	} else if (xstrcmp(data, "flushqlog") == 0) {	/* flushqlog */
		assign_boolean(&cfg.flushqlog, val);
	} else if (xstrcmp(data, "autoqlog") == 0) {	/* autoqlog */
		assign_int(&cfg.autoqlog, val, 0);
#endif /* ifdef QUICKLOG */
#ifdef NEED_LOGGING
	} else if (xstrcmp(data, "logsuffix") == 0 ||	/* logsuffix */
			xstrcmp(data, "logpostfix") == 0) { /* TODO remove me */
		assign_param(&cfg.logsuffix, val);
#endif /* ifdef NEED_LOGGING */
#ifdef INBOX
	} else if (xstrcmp(data, "inbox") == 0) {	/* inbox */
		assign_boolean(&cfg.inbox, val);
#endif /* ifdef INBOX */
	} else if (xstrcmp(data, "listenport") == 0) {	/* listenport */
		assign_int(&cfg.listenport, val, 0);
	} else if (xstrcmp(data, "listenhost") == 0) {	/* listenhost */
		assign_param(&cfg.listenhost, val);
	} else if (xstrcmp(data, "password") == 0) {	/* password */
		assign_param(&cfg.password, val);
	} else if (xstrcmp(data, "leave") == 0) {	/* leave */
		assign_boolean(&cfg.leave, val);
	} else if (xstrcmp(data, "chandiscon") == 0) {	/* chandiscon */
		/*
		 * 0 = nothing
		 * 1 = notice
		 * 2 = part
		 * 3 = privmsg
		 */
		assign_option(&cfg.chandiscon, val,
				"nothing\0notice\0part\0privmsg\0\0");
	} else if (xstrcmp(data, "leavemsg") == 0) {	/* leavemsg */
		assign_param(&cfg.leavemsg, val);
	} else if (xstrcmp(data, "awaymsg") == 0) {	/* awaymsg */
		assign_param(&cfg.awaymsg, val);
	} else if (xstrcmp(data, "usequitmsg") == 0) {	/* usequitmsg */
		assign_boolean(&cfg.usequitmsg, val);
	} else if (xstrcmp(data, "autoaway") == 0) {	/* autoaway */
		/*
		 * 0 = never
		 * 1 = client detach
		 * 2 = no clients attached
		 */
		assign_option(&cfg.autoaway, val,
				"never\0detach\0noclients\0\0");
	} else if (xstrcmp(data, "getnick") == 0) {	/* getnick */
		/*
		 * 0 = never
		 * 1 = detached
		 * 2 = attached
		 * 3 = always
		 *
		 * Values are hardcoded all over miau.c.
		 * This should be fixed even though code is quite clear.
		 */
		/* Double-terminate just to be sure. */
		assign_option(&cfg.getnick, val,
				"never\0detached\0attached\0always\0\0");
	} else if (xstrcmp(data, "getnickinterval") == 0) {/* getnickinterval */
		assign_int(&cfg.getnickinterval, val, 0);
	} else if (xstrcmp(data, "bind") == 0) {	/* bind */
		assign_param(&cfg.bind, val);
#ifdef AUTOMODE
	} else if (xstrcmp(data, "automodedelay") == 0) { /* automodedelay */
		assign_int(&cfg.automodedelay, val, 0);
#endif /* ifdef AUTOMODE */
	} else if (xstrcmp(data, "antiidle") == 0) {	/* antiide */
		assign_int(&cfg.antiidle, val, 0);
	} else if (xstrcmp(data, "nevergiveup") == 0) {	/* nevergiveup */
		assign_boolean(&cfg.nevergiveup, val);
	} else if (xstrcmp(data, "norestricted") == 0) { /* norestricted */
		assign_boolean(&cfg.jumprestricted, val);
	} else if (xstrcmp(data, "stonedtimeout") == 0) { /* stonedtimeout*/
		assign_int(&cfg.stonedtimeout, val, MINSTONEDTIMEOUT);
	} else if (xstrcmp(data, "connecttimeout") == 0) { /* connecttimeout */
		assign_int(&cfg.connecttimeout, val, MINCONNECTTIMEOUT);
	} else if (xstrcmp(data, "reconnectdelay") == 0) { /* reconnectdelay */
		assign_int(&cfg.reconnectdelay, val, MINRECONNECTDELAY);
	} else if (xstrcmp(data, "rejoin") == 0) {	/* rejoin */
		assign_boolean(&cfg.rejoin, val);
	} else if (xstrcmp(data, "forwardmsg") == 0) {	/* forwardmsg */
		assign_param(&cfg.forwardmsg, val);
	} else if (xstrcmp(data, "forwardtime") == 0) {	/* forwardtime */
		assign_int(&cfg.forwardtime, val, 30);
	} else if (xstrcmp(data, "maxclients") == 0) {	/* maxclients */
		assign_int(&cfg.maxclients, val, 1);
#ifdef PRIVLOG
	} else if (xstrcmp(data, "privlog") == 0) {	/* privlog */
		/* See log.h for options' order. */
		/* Double-terminate just to be sure. */
		assign_option(&cfg.privlog, val,
				"never\0detached\0attached\0always\0\0");
#endif /* ifdef PRIVLOG */
#ifdef DCCBOUNCE
	} else if (xstrcmp(data, "dccbounce") == 0) {	/* dccbounce */
		assign_boolean(&cfg.dccbounce, val);
	} else if (xstrcmp(data, "dccbindhost") == 0) {
		assign_param(&cfg.dccbindhost, val);
#endif /* ifdef DCCBOUNCE */
	} else if (xstrcmp(data, "nickfillchar") == 0) { /* nickfillchar */
		cfg.nickfillchar = val[0];
	} else if (xstrcmp(data, "usermode") == 0) {	/* usermode */
		assign_param(&cfg.usermode, val);
	} else if (xstrcmp(data, "maxnicklen") == 0) {	/* maxnicklen */
		assign_int(&cfg.maxnicklen, val, 3);
	} else if (xstrcmp(data, "floodtimer") == 0) {	/* floodtimer */
		assign_int(&cfg.floodtimer, val, 0);
	} else if (xstrcmp(data, "burstsize") == 0) {	/* burstsize */
		assign_int(&cfg.burstsize, val, 1);
	} else if (xstrcmp(data, "jointries") == 0) {	/* jointries */
		assign_int(&cfg.jointries, val, 0);
	} else if (xstrcmp(data, "statelog") == 0) {	/* statelog */
		assign_boolean(&cfg.statelog, val);
	} else {
		parse_error();
	}
} /* void parse_param(char *data) */



/*
 * Parse list-item.
 */
void
parse_list_line(char *data)
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
#ifdef CHANLOG
	int		logtype;
#endif /* ifdef CHANLOG */

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
	param = (char **) xmalloc((sizeof(char *)) * MAXNUMOFPARAMS);
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
		else if (*ptr == '\0') {
			eol = 1;
		}

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
				param[paramcount] = xstrdup(par);
			}
			paramcount++;
			par = ptr + 1;
			inside = 0;
		}
		
		ptr++;
	} while (eol == 0);
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
			if (paramcount != 1) {
				break;
			}
			
			llist_add_tail(llist_create(xstrdup(param[0])),
					&nicknames.nicks);
			ok = 1;
			break;
			
		case CFG_SERVERS:
			if (paramcount > 4) {
				break;
			}
			{
				int t0, t1;
				t0 = t1 = 0; /* keep some compilers happy */
				assign_int(&t0, param[1], 0);
				assign_int(&t1, param[3], 0);
				add_server(param[0], t0, param[2], t1);
				ok = 1;
			}
			break;
			
		case CFG_CONNHOSTS:
			if (paramcount > 2) {
				break;
			}
			permlist = &connhostlist;
			ok = 1;
			break;
			
		case CFG_IGNORE:
			if (paramcount > 2) {
				break;
			}
			permlist = &ignorelist;
			ok = 1;
			break;
			
#ifdef AUTOMODE
		case CFG_AUTOMODELIST:
			if (paramcount > 2 || param[0][1] != ':') {
				break;
			}
			permlist = &automodelist;
			ok = 1;
			break;
#endif /* ifdef AUTOMODE */
			
#ifdef CHANLOG
		case CFG_CHANLOG:
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
			chanlog_add_rule(param[0], param[2], logtype);
			ok = 1;
			break;
#endif /* ifdef CHANLOG */

		case CFG_CHANNELS:
			if (paramcount > 2) {
				break;
			}
			
			/* CFG_CHANNELS only has effect at start up. */
			if (virgin == 1) {
				channel_type *channel;

				if (param[0] == NULL || param[0][0] == '\0') {
					break;
				}

				/*
				 * Make sure there are no channels such as
				 * !#foo in miaurc. Auto-creation of safe
				 * channels is a Bad Idea(tm) and we don't
				 * want to help with that. If user wants to
				 * define "real" channel name of a safe channel,
				 * we won't stop him from hurting himself.
				 */
				if (param[0][0] == '!' && param[0][1] == '#') {
					break;
				}

				/* Not adding same channel twice. */
				if (channel_find(param[0], LIST_PASSIVE)
						!= NULL) {
					break;
				}

				/* channel_add will set up us a key. */
				channel = channel_add(param[0],
						param[1], LIST_PASSIVE);
			}
			ok = 1;
			break;

#ifdef ONCONNECT
		case CFG_ONCONNECT:
			if (paramcount > 3) {
				break;
			}
			if (*param[0] == 'p' || *param[0] == 'n' ||
					*param[0] == 'r') {
				onconnect_add(*(param[0]), param[1], param[2]);
				ok = 1;
			}
			break;
#endif /* ifdef ONCONNECT */

		case CFG_INVALID:
			ok = 1;
			break;
	}

	/* Did we make it? */
	if (ok == 0) {
		parse_error();
	} else if (permlist != NULL) {
		/* Everything went just fine and there's something to do... */
		if (paramcount == 1) {
			add_perm(permlist, xstrdup(param[0]), 1);
		} else if (paramcount == 2) {
			assign_boolean(&n, param[1]);
			add_perm(permlist, xstrdup(param[0]), n);
		}
	}

	/* Free parameters. */
	while (paramcount > 0) {
		paramcount--;
		xfree(param[paramcount]);
	}
	xfree(param);
} /* void parse_list_line(char *data) */



/*
 * Parse configuration file.
 */
int
parse_cfg(const char *cfgfile)
{
	FILE		*file;
	int		filelen;
	
	char		*buf;
	char		*bufptr;
	char		*nextptr;

	buf = (char *) xmalloc(READBUFSIZE);

	line = 1;
	file = fopen(cfgfile, "r");
	if (file == NULL) {
		return -1;
	}

	/* Make sure configuration-file ends. */
	filelen = (int) fread(buf, 1, READBUFSIZE - 2, file);
	buf[filelen] = '\n';
	buf[filelen + 1] = '\0';
	
	bufptr = buf;
	nextptr = strchr(buf, '\n');
	while (nextptr >= bufptr) {
		*nextptr = '\0';
		if (*(nextptr - 1) == '\r') {
			*(nextptr - 1) = '\0';
		}
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
} /* int parse_cfg(const char *cfgfile) */



void
parse_error(void)
{
	error(PARSE_SE, line);
} /* void parse_error(void) */
