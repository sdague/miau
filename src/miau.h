/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2002-2005 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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

#include <config.h>



#define MIAURC		"miaurc"
#define MIAUDIR		".miau/"
#define LOGDIR		"logs"
#define FILE_PID	"pid"
#define FILE_LOG	"log"
#define FILE_INBOX	"inbox"

#define MIAU_URL	"http://miau.sourceforge.net/"
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SELECT_H
#include <sys/select.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NETDB_H
#include <netdb.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_CRYPT_H
#include <crypt.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#if !HAVE_RANDOM
#define random()	(rand()/16)
#endif

#if !HAVE_SIGACTION     /* old "weird signals" */
#define sigaction	sigvec
#ifndef sa_handler
#define sa_handler	sv_handler
#define sa_mask		sv_mask
#define sa_flags	sv_flags
#endif
#endif



#ifdef CHANLOG
#define LOGGING
#endif /* CHANLOG */
#ifdef PRIVLOG
#define LOGGING
#endif /* PRIVLOG */



#include "common.h"
#include "llist.h"
#include "server.h"
#include "client.h"



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
#endif /* UPTIME */
#ifdef AUTOMODE
	int	automodes;
#endif /* AUTOMODE */
	char	*idhostname;	/* ident@host where miau runs from */
	int	goodhostname;	/* -1 if we haven't got hostname containing @ */
} status_type;



#define AWAY	0x01
#define CUSTOM	0x02



typedef struct {
	llist_list	nicks;		/* Defined nicks. */
	llist_node	*current;	/* Current nick. */
	int		next;		/* Nick-status. */
} nicknames_type;

#define NICK_FIRST	0	/* Try the first nick on the list. */
#define NICK_NEXT	1	/* Try next nick on the list. */
#define NICK_GEN	2	/* Generate a nick. */


typedef struct {
#ifdef QUICKLOG
	int	qloglength;
#ifdef QLOGSTAMP
	int	timestamp;	/* Timestamp type in quicklog. */
#endif /* QLOGSTAMP */
	int	flushqlog;	/* Flush quicklog on fakeconnect() ? */
#endif /* QUICKLOG */
#ifdef DCCBOUNCE
	int	dccbounce;	/* DCC-bounce */
#endif /* DCCBOUNCE */
#ifdef AUTOMODE
	int	automodedelay;
#endif /* AUTOMODE */
#ifdef INBOX
	int	inbox;
#endif /* INBOX */
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
	int	maxnicklen;	/* Maximum length for nick. */
	int	maxclients;	/* Maximum number of clients connected. */
	int	usequitmsg;	/* Use quit-message as away/leavemsg */
#ifdef PRIVLOG
	int	privlog;	/* Write log of _private_ messages. */
#endif /* PRIVLOG */

	char	nickfillchar;	/* Character to fill nick with. */

#ifdef LOGGING
	char	*logpostfix;	/* Postfix for global logfiles. */
#endif /* LOGGING */
#ifdef DCCBOUNCE
	char	*dccbindhost;
#endif /* DCCBOUNCE */
#ifdef _NEED_CMDPASSWD
	char	*cmdpasswd;
#endif /* _NEED_CMDPASSWD */
	char	*username;
	char	*realname;
	char	*password;
	char	*leavemsg;
	char	*bind;
	char	*listenhost;
	char	*awaymsg;
	char	*forwardmsg;
	char	*channels;
	char	*home;
	char	*usermode;	/* User modes. Set on connect. */
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
#endif /* AUTOMODE */
#ifdef PRIVLOG
	int		privlog;
#endif /* PRIVLOG */
} timer_type;



extern serverlist_type  servers;

extern server_info      i_server;
extern client_info	i_client;
extern client_info	i_newclient;

extern connection_type  c_server;
extern connection_type	c_client;
extern connection_type	c_newclient;
extern clientlist_type	c_clients;

extern cfg_type		cfg;
extern nicknames_type	nicknames;
extern status_type	status;

extern FILE		*inbox;

extern timer_type	timers;

#ifdef PINGSTAT
extern int	ping_sent;
extern int	ping_got;
#endif /* PINGSTAT */

extern int	error_code;

void get_nick(char *format);
void join_channels(connection_type *client);
void miau_commands(char *command, char *param, connection_type *client);


void set_away(const char *reason);

void clients_left(const char *reason);
void drop_newclient();

/* parse-section */
extern int lineno;
extern FILE *yyin;


#endif /* ifndef MIAU_H_ */
