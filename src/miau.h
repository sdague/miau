/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2002-2006 Tommi Saviranta <wnd@iki.fi>
 *	(C) 2002 Lee Hardy <lee@leeh.co.uk>
 *	(C) 1998-2002 Sebastian Kienzl <zap@riot.org>
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

#ifndef MIAU_H_
#define MIAU_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include "etc.h"

#include "server.h"
#include "client.h"
#include "llist.h"

#include <time.h>


#define MIAURC		"miaurc"
#define MIAUDIR		".miau/"
#define FILE_PID	"pid"
#define FILE_LOG	"log"
#define FILE_INBOX	"inbox"

#ifndef VERSION
#define VERSION		"???"
#endif
#define DEFAULT_NICKFILL	'_'
#define DEFAULT_PORT		6667
#define MINSTONEDTIMEOUT	30
#define MINCONNECTTIMEOUT	5
#define MINRECONNECTDELAY	1
#define JOINTRYINTERVAL		60
#define MAXCMDPARAMS		4

#define GOOD_SERVER_DELAY	60

#define CONN_DISABLED	999


typedef struct {
	char	*nickname;
	int	got_nick;
	int	getting_nick;	/* ...so we could suppress "NICKNAMEINUSE" */
	int	passok;
	int	init;
	int	supress;
	int	allowconnect;
	int	allowreply;
	int	reconnectdelay;	/* Time before next try to connection. */
	int	autojoindone;	/* Joined cfg.channels once ? */
	char 	*awaymsg;	/* Current away-message, NULL is not away. */
	int	awaystate;	/* User-set away-message. */
	int	good_server;	/* This server is recognized as "good". */
#ifdef UPTIME
	time_t	startup;
#endif /* ifdef UPTIME */
#ifdef AUTOMODE
	int	automodes;
#endif /* ifdef AUTOMODE */
	char	*idhostname;	/* ident@host where miau runs from */
	int	goodhostname;	/* -1 if we haven't got hostname containing @ */
} status_type;



#define AWAY	0x01
#define CUSTOM	0x02



typedef struct {
	llist_list	nicks;		/* Defined nicks. */
	llist_node	*current;	/* Current nick. */
	int		next;		/* Nick-status. */
	int		gen_tries;	/* N of tries to generate a nick. */
} nicknames_type;

#define NICK_FIRST	0	/* Try the first nick on the list. */
#define NICK_NEXT	1	/* Try next nick on the list. */
#define NICK_GEN	2	/* Generate a nick. */


typedef struct {
	int	statelog;	/* stdout to log */
#ifdef QUICKLOG
	int	qloglength;
	int	autoqlog;
#ifdef QLOGSTAMP
	int	timestamp;	/* Timestamp type in quicklog. */
#endif /* ifdef QLOGSTAMP */
	int	flushqlog;	/* Flush quicklog on fakeconnect() ? */
#endif /* ifdef QUICKLOG */
#ifdef DCCBOUNCE
	int	dccbounce;	/* DCC-bounce */
#endif /* ifdef DCCBOUNCE */
#ifdef AUTOMODE
	int	automodedelay;
#endif /* ifdef AUTOMODE */
#ifdef INBOX
	int	inbox;
#endif /* ifdef INBOX */
	int	listenport;
	int	floodtimer;	/* Sending one message takes n seconds. */
	int	burstsize;	/* We may send up to n messages in a burst. */
	int	jointries;	/* Times to try joining a channel. */
	int	getnick;
	int	getnickinterval;
	int	antiidle;
	int	nevergiveup;
	int	jumprestricted;
	int	stonedtimeout;	/* Stoned server -timeout. */
	int	rejoin;
	int	connecttimeout;	/* Timeout for connect() (s). 0 to disable. */
	int	reconnectdelay;	/* Time before next try to connect (s). */
	int	leave;		/* Leave channels at detach. */
	int	chandiscon;	/* What to do with channels at disconnect. */
	int	maxnicklen;	/* Maximum length for nick. */
	int	maxclients;	/* Maximum number of clients connected. */
	int	usequitmsg;	/* Use quit-message as away/leavemsg */
	int	autoaway;	/* Autoaway never/detach/noclients */
#ifdef PRIVLOG
	int	privlog;	/* Write log of _private_ messages. */
#endif /* ifdef PRIVLOG */

	char	nickfillchar;	/* Character to fill nick with. */

#ifdef NEED_LOGGING
	char	*logsuffix;	/* Suffix for global logfiles. */
#endif /* ifdef NEED_LOGGING */
#ifdef DCCBOUNCE
	char	*dccbindhost;
#endif /* ifdef DCCBOUNCE */
#ifdef NEED_CMDPASSWD
	char	*cmdpasswd;
#endif /* ifdef NEED_CMDPASSWD */
	char	*username;
	char	*realname;
	char	*password;
	char	*leavemsg;
	char	*bind;
	char	*listenhost;
	char	*awaymsg;
	char	*forwardmsg;
	int	forwardtime;
	char	*channels;
	char	*home;
	char	*usermode;	/* User modes. Set on connect. */
	int	no_identify_capab; /* suppress request of "CAPAB IDENTIFY-*" */
	char	*privmsg_fmt;
} cfg_type;

typedef struct {
	int		reply;
	int		listen;
	signed int	nickname;
	int		antiidle;
	int		forward;
	int		connect;
	int		join;
	int		good_server;
#ifdef AUTOMODE
	int		automode;
#endif /* ifdef AUTOMODE */
#ifdef PRIVLOG
	int		privlog;
#endif /* ifdef PRIVLOG */
#ifdef NEED_LOGGING
	int		logfile_warn;
#endif /* ifdef NEED_LOGGING */
} timer_type;



/* export global stuff */
extern serverlist_type  servers;

extern server_info      i_server;
extern client_info	i_client;
extern client_info	i_newclient;

extern connection_type  c_server;
/* extern connection_type	c_client; */
extern connection_type	c_newclient;
extern clientlist_type	c_clients;

extern cfg_type		cfg;
extern nicknames_type	nicknames;
extern status_type	status;

extern FILE		*inbox;

extern timer_type	timers;

extern char		*forwardmsg;
extern int		forwardmsgsize;


#ifdef PINGSTAT
extern int	ping_sent;
extern int	ping_got;
#endif /* ifdef PINGSTAT */

extern int	error_code;



void get_nick(char *format);
void join_channels(connection_type *client);
void miau_commands(char *command, char *param, connection_type *client);


void set_away(const char *reason);

void clients_left(const char *reason);
void drop_newclient(char *reason);



#endif /* ifndef MIAU_H_ */
