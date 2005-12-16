/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2002-2005 Tommi Saviranta <wnd@iki.fi>
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

#ifndef IRC_H_
#define IRC_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include "server.h"
#include "client.h"



#define QUEUESIZE	16

/*
 * 520 characters should be enough for all PRIVMSGs and ACTIONs.
 * RFC2812 says "IRC messages are always lines of characters terminated with a
 * CR-LF (Carriage Return - Line Feed) pair, and these messages SHALL NOT
 * exceed 512 characters in length, counting all characters including the
 * trailing CR-LF.".
 */
#define IRC_MSGLEN	520

/*
 * all these function return 0 on error (except sock_open, this will return -1)
 * on error net_errstr will point to the error_string
 */
int sock_open();
int sock_close(connection_type *connection);
int sock_listen(int sock);
int sock_setnonblock(int sock);
int sock_setblock(int sock);
int sock_setreuse(int sock);
int sock_bind(int sock, char *bindhost, int port);
int sock_bindlookedup(int sock, int port);
int sock_accept(int sock, char **hostname, int checkperm);

/* this one returns -1 if hostname is not permitted to connect */
int rawsock_close(int sock);
struct hostent *name_lookup(char *bindhost);

void irc_process_queue();
void irc_clear_queue();

int irc_mwrite(clientlist_type *clients, char *format, ...);
int irc_write(connection_type *connection, char *format, ...);
int irc_write_head(connection_type *connection, char *format, ...);
/* returns: on success -> number of written bytes; -1 on error */

int irc_read(connection_type *connection);
/* returns: 1/0(no data (if blocking)) on success; -1 on error */

#define CONN_FINALIZING	-2	/* Connected, but waiting for thread to die. */
#define CONN_BUSY	-1	/* Still connecting... */
#define CONN_OK		0	/* All ok. */
#define CONN_SOCK	1	/* sock_open() failed. */
#define CONN_LOOKUP	2	/* remotelookup failed. */
#define CONN_BIND	3	/* Unable to bind. */
#define CONN_CONNECT	4	/* connect() failed. */
#define CONN_WRITE	5	/* write() failed. */
#define CONN_OTHER	6	/* Other error (setting nonblocking failed) */
/* IMPORTANT! connection->connected does NOT get set ! */
int irc_connect(connection_type *connection, server_type *server,
		char *nickname, char *username, char *realname, char *bindto);

int irc_mnotice(clientlist_type *clients, char nickname[], char *format, ...);
void irc_notice(connection_type *connection, char nickname[],
		char *format, ...);


#define RPL_MYINFO_LEN		4
#define RPL_ISUPPORT_LEN	3

/* Numeric command responses */

#define RPL_WELCOME		1
#define RPL_YOURHOST		2
#define RPL_CREATED		3
#define RPL_MYINFO		4
/*
 * 005: http://www.irc.org/tech_docs/draft-brocklesby-irc-isupport-03.txt
 * This implementation assumes RPL_BOUNCE has been changed to 010.
 */
#define RPL_ISUPPORT		5
#define RPL_BOUNCE		10

#define RPL_MOTDSTART		375
#define RPL_MOTD		372
#define RPL_ENDOFMOTD		376

#define RPL_LUSERCLIENT		251
#define RPL_LUSEROP		252
#define RPL_LUSERUNKNOWN	253
#define RPL_LUSERCHANNELS	254
#define RPL_LUSERME		255

#define RPL_UNAWAY		305
#define RPL_NOWAWAY		306

#define RPL_CHANNELMODEIS	324
#define RPL_NOTOPIC		331
#define RPL_TOPIC		332
#define RPL_TOPICWHO		333

#define RPL_NAMREPLY		353

#define ERR_NOSUCHCHANNEL	403
#define ERR_TOOMANYCHANNELS	405
#define ERR_TOOMANYTARGETS	407

#define ERR_ERRONEUSNICKNAME	432
#define ERR_NICKNAMEINUSE	433
#define ERR_NICKUNAVAILABLE	437	/* what is this?-) */
#define ERR_UNAVAILRESOURCE	437
#define ERR_NOPERMFORHOST	463
#define ERR_YOUREBANNEDCREEP	465

#define ERR_CHANNELISFULL	471
#define ERR_INVITEONLYCHAN	473
#define ERR_BANNEDFROMCHAN	474
#define ERR_BADCHANNELKEY	475
#define ERR_BADCHANMASK		476

#define RPL_RESTRICTED		484

#define RPL_WHOISUSER		311
#define RPL_WHOISSERVER		312
#define RPL_WHOISOPERATOR	313
#define RPL_WHOISIDLE		317
#define RPL_WHOISCHANNELS	319
#define RPL_ENDOFWHOIS		318



/* export global stuff */
extern int		highest_socket;
extern const char	*net_errstr;

/* Flood-control counter. If greated than zero, can send messages to server. */
extern int	msgtimer;



#endif /* ifndef IRC_H_ */
