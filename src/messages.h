/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2002-2006 Tommi Saviranta <wnd@iki.fi>
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

#ifndef MESSAGES_H_
#define MESSAGES_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */



/*
 * General
 */
#define IRCLF	"\r\n"

#ifdef MKPASSWD
#define OPT_CRYPT_L	"    -c\t\tCreate crypted password\n"
#define OPT_CRYPT_S	"[-c] "
#else
#define OPT_CRYPT_L	""
#define OPT_CRYPT_S	""
#endif

#define SYNTAX "\
Usage: %s [-f] "OPT_CRYPT_S"[-d dir]\n\
    -f\t\tStay in foreground\n\
    -d dir\tOther directory than default for miau-files\n\
    -v\t\tPrint version and copyright information and exit\n\
"OPT_CRYPT_L

/*
 * Parsing-section.
 */
#define PARSE_SE	"Parse error on line %d!"
#define PARSE_MK	"'%s' has not been set!"
#define PARSE_NOSERV	"No servers have been specified!"

/*
 * miau-messages.
 */
#define MIAU_WELCOME	"Welcome to miau (not connected to server)"
#define MIAU_PARSING	"Parsing configuration file..."
#define MIAU_ERRCFG	"Unable to open miaurc in %s!"
#define MIAU_ERRNEEDARG	"Option -%c requires an argument!"
#define MIAU_ERRNOHOME	"$HOME is not set! (set it or use -d)"
#define MIAU_ERRFILE	"Can't write to \"%s\"!"
#define MIAU_ERRCHDIR	"Can't chdir to \"%s\"!"
#define MIAU_ERRLOGDIR	"\"%s\" not a directory!"
#define MIAU_ERRCREATELOGDIR	"Can't create \"%s\"!"
#define MIAU_ERREXIT	"Terminating..."
#define MIAU_SIGTERM	"Caught sigterm, terminating..."
#define MIAU_OUTOFSERVERS	"Out of servers, terminating..."
#define MIAU_OUTOFSERVERSNEVER	"Out of servers, retrying..."
#define MIAU_ERRFORK	"Unable to fork!"
#define MIAU_FORKED	"miau's forked. (pid %d)"
#define MIAU_ERRINBOXFILE	"Can't open inbox, inbox disabled."
#define MIAU_LEAVING	"Leaving channels."
#define MIAU_REINTRODUCE	"Reintroducing channels."
#define MIAU_JOINING	"Autojoining channels (%s)."
#define MIAU_JOIN_QUEUE	"Queued joining channel %s"
#define MIAU_JUMP	"Changing server..."
#define MIAU_RECONNECT	"Reconnecting to server..."
#define MIAU_DIE_CL	"killed"
#define MIAU_READ_RC	"Configuration read."
#define MIAU_RESET	"Reseted all servers to working."
#define MIAU_NICK	"miau's nick is '%s'."
#define MIAU_GOTNICK	"Got nick '%s'!"
#define MIAU_NEWSESSION	"---------- NEW SESSION ----------"
#define MIAU_STARTINGLOG	"miau version "VERSION" - starting log..."
#define MIAU_CLOSINGLINK	"ERROR: Closing link: %s"
#define MIAU_USERKILLED	"ERROR: "CLNT_DIE" %s"
#define MIAU_LOGNOWRITE	"Cannot write to logfile \"%s\"!"

#define MIAU_VERSION	"- miau version "VERSION" - \""VERSIONNAME"\" -"
#define MIAU_372_RUNNING	"- Running on server %s with nickname %s"
#define MIAU_372_NOT_CONN	"- Not connected to server"
#define MIAU_END_OF_MOTD	"End of /MOTD command."
#define MIAU_ERRLOGCONN	"Can't write to logfile, disabling logging."

/* releasenick */
#ifdef RELEASENICK
#define MIAU_RELEASENICK	"Having nick '%s' for %d second(s)"
#endif /* RELEASENICK */

/* uptime */
#ifdef UPTIME
#define MIAU_UPTIME	"miau has been online: %dd %02dh %02dm %02ds"
#define CMD_UPTIME	", UPTIME"
#else	/* UPTIME */
#define	CMD_UPTIME	""
#endif	/* UPTIME */

/* pingstat */
#ifdef PINGSTAT
#define CMD_PINGSTAT	", PINGSTAT"
#else /* PINGSTAT */
#define CMD_PINGSTAT	""
#endif /* PINGSTAT */

/* dumpstatus */
#ifdef DUMPSTATUS
#define CMD_DUMP	", DUMP"
#else /* DUMPSTATUS */
#define CMD_DUMP	""
#endif /* DUMPSTATUS */

/* quicklog */
#ifdef QUICKLOG
#define MIAU_FLUSHQLOGALL	"Quicklog flushed"
#define MIAU_FLUSHQLOG	"Flushed quicklog older than " \
			"%d day(s), %d hour(s), %d minute(s)"
#define CMD_FLUSHQLOG	", QUICKLOG [[[days:]hours:]minutes], FLUSHQLOG [[[days:]hours:]minutes]"
#else /* ifdef QUICKLOG */
#define CMD_FLUSHQLOG	""
#endif /* ifdef else QUICKLOG */

/* mkpasswd */
#ifdef MKPASSWD
#define MIAU_ENTERPASS	"Enter password to crypt: "
#define MIAU_THISPASS	"Set this as password in your miaurc: %s\n\n"
#endif /* MKPASSWD */

#define MIAU_URL	"http://miau.sf.net/"
#define BANNER "\
miau v"VERSION" \""VERSIONNAME"\"\n\
  "MIAU_URL"\n\
Copyright (C) 2002-2006 Tommi Saviranta <wnd@iki.fi>\n\
        (C) 2002 Lee Hardy <lee@leeh.co.uk>\n\
        (C) 1998-2002 Sebastian Kienzl <zap@riot.org>\n\
\n\
This is free software; see the GNU General Public Licence version 2 or\n\
later for copying conditions.  There is NO warranty.\n\
Read 'COPYING' for copyright and licence details.\n"

/*
 * Socket-messages.
 */
#define SOCK_GENERROR	"General socket error (%s)!"
#define SOCK_ERROPEN	"Unable to create socket! (%s)"
#define SOCK_ERRBIND	"Unable to bind to port %d! (%s)"
#define SOCK_ERRBINDHOST	"Unable to bind to '%s':%d! (%s)"
#define SOCK_ERRLISTEN	"Unable to listen!"
#define SOCK_ERRACCEPT	"Unable to accept connection from '%s'!"
#define SOCK_LISTENOK	"Listening on port %d."
#define SOCK_LISTENOKHOST	"Listening on host %s/port %d."
#define SOCK_CONNECT	"TCP-connection to '%s' established!"
#define SOCK_ERRRESOLVE	"Unable to resolve '%s'!"
#define SOCK_ERRCONNECT	"Unable to connect to '%s'! (%s)"
#define SOCK_ERRWRITE	"Error while sending data to '%s'!"
#define SOCK_RECONNECT	"Trying to reconnect to '%s' in %d seconds."
#define SOCK_RECONNECTNOW	"Trying to reconnect to '%s'."
#define SOCK_ERRTIMEOUT	"Connection timeout"

/*
 * IRC-messages.
 */
#define IRC_CONNECTED	"Connected to '%s'."
#define IRC_BADNICK	"'%s' is an invalid nick - using '%s'."
#define IRC_NICKINUSE	"Nick '%s' is in use - using '%s'."
#define IRC_NICKUNAVAIL	"Nick '%s' unavailable - using '%s'."
#define IRC_SERVERERROR	"Server-error! (%s)"
#define IRC_KILL	"You've been killed by '%s'!"
#define IRC_AWAY	"You have been marked as being away"
#define IRC_NOSUCHCHAN	"No such channel"

/*
 * Client-related -messages.
 */
#define CLNT_AUTHFAIL	"Authorization failed!"
#define CLNT_AUTHTO	"New client timed out while authorizing!"
#define CLNT_AUTHFAILNOTICE	"Unsuccessful connect-attempt from '%s'!"
#define CLNT_AUTHOK	"Authorization successful!"
#define CLNT_LEFT	"Client signed off."
#define CLNT_DROP	"Dropped old client."
#define CLNT_DIE	"Killed by user: "
#define CLNT_STONED	"Disconnected stoned client."
#define CLNT_CLIENTS	"%d client(s) connected."
#define CLNT_SERVINFO	"Disconnected client to update server info"

#define CLNT_HAVEMSGS	"\2You have messages waiting.\2 (/miau read)"
#define CLNT_INBOXEMPTY	"Your inbox is empty."
#define CLNT_KILLEDMSGS	"Killed your messages."

#define CLNT_CAUGHT	"Client from '%s'."
#define CLNT_DENIED	"Denied client from '%s'."
#define CLNT_DROPPED	"Client dropped connection."
#define CLNT_DROPPEDUNAUTH "Client dropped connection while authorizing."

#define CLNT_CTCP	"Received a CTCP %s from %s."
#define CLNT_CTCPNOREPLY	"Received a CTCP %s from %s. (didn't reply)"
#define CLNT_KICK	"%s kicked me out of %s (%s)!"

#define CLNT_NOTCONNECT	"Not connected to server."

/*
 * Server-related -messages.
 */
#define SERV_ERR	"Closed connection after server reported error."
#define SERV_STONED	"Disconnecting from stoned server."
#define SERV_TRYING	"Trying server '%s' on port %d..."
#define SERV_DROPPED	"Server dropped connection!"
#define SERV_RESTRICTED	"Connection is restricted, jumping..."
#define SERV_DISCONNECT	"Disconnected from server."


/*
 * Client-side -messages.
 */
#ifdef CTCPREPLIES
#define VERSIONREPLY "\1VERSION miau v"VERSION" \""VERSIONNAME"\" -- "MIAU_URL"\1"
#define CLIENTINFOREPLY "\1CLIENTINFO VERSION PING CLIENTINFO ACTION\1"
#endif /* CTCPREPLIES */

#define CLNT_COMMANDS	"Available commands: HELP, READ, DEL, JUMP [n], " \
			"REHASH, RESET, DIE, PRINT" \
			CMD_FLUSHQLOG CMD_UPTIME CMD_PINGSTAT CMD_DUMP
#define CLNT_INBOXSTART	"Playing inbox..."
#define CLNT_INBOXEND	"End of inbox."
#define CLNT_NEWCLIENT	"New connection established!"
#define CLNT_RESTRICTED	"restricted connection"
#define CLNT_SERVLIST	"Servers:"
#define CLNT_NOSERVERS	PARSE_NOSERV
#define CLNT_MIAURCBEENWARNED	"Incomplete miaurc! YOU HAVE BEEN WARNED!"
#define CLNT_CURRENT	"Current server is %d%s."
#define CLNT_ANDCONNECTING	" (connecting)"
#define CLNT_CONNECTING	"Connecting to server..."

/*
 * DCC-bouncing.
 */
#ifdef DCCBOUNCE
#define DCC_SUCCESS	"DCC: Bounce from %s established! [%d]"
#define DCC_TIMEOUT	"DCC: Bounce timed out! [%d]"
#define DCC_ERRCONNECT	"DCC: Can't connect peer! (%s) [%d]"
#define DCC_ERRACCEPT	"DCC: Can't accept incoming! (%s) [%d]"
#define DCC_ERRSOCK	"DCC: Socket-error! (%s) [%d]\n"
#define DCC_START	"DCC: Starting bounce to %s:%d! [%d]"
#define DCC_END		"DCC: Ending bounce [%d]"
#endif /* DCCBOUNCE */

/*
 * Logging,
 */
#define LOGM_JOIN	"%s --> %s (%s) has joined %s\n"
#define LOGM_PART	"%s <-- %s has left %s (%s)\n"
#define LOGM_QUIT	"%s <-- %s has quit (%s%s%s)\n"
#define LOGM_KICK	"%s <-- %s was kicked by %s (%s)\n"
#define LOGM_MODE	"%s --- %s sets mode %s\n"
#define LOGM_MESSAGE	"%s <%s> %s\n"
#define LOGM_ACTION	"%s * %s %s\n"
#define LOGM_NOTICE	"%s >%s< %s\n"
#define LOGM_TOPIC	"%s --- %s has changed the topic to: %s\n"
#define LOGM_NICK	"%s --- %s is now known as %s\n"
#define LOGM_MIAU	"%s --- client %sconnected\n"
#define LOGM_LOGOPEN	"**** BEGIN LOGGING AT %s\n"
#define LOGM_LOGCLOSE	"**** ENDING LOGGING AT %s\n\n"

/*
 * quicklog,
 */
#ifdef QUICKLOG
#define CLNT_QLOGSTART	"Playing quicklog..."
#define CLNT_QLOGEND	"End of quicklog."
#endif /* QUICKLOG */

/*
 * quicklog to inbox,
 */
#ifdef QLOGTOMSGLOG
#define QLOGM_PRIVMSG	"<%s/%s> %s\n"
#define QLOGM_GENERAL	"%s\n"
#define QLOGM_LEAVE	"Forced to leave the channel"
#endif /* QLOGTOMSGLOG */

/*
 * General error-message.
 */
#define ERR_CANT_ATEXIT	"Couldn't set atexit()!"
#define ERR_MEMORY	"Out of memory, terminating!"
#define ERR_UNEXPECTED	"!!! An unexpected situation occured, please send us this: [%s, %s:%d]\n"


/*
 * Ping statistics.
 */
#ifdef PINGSTAT
#define PING_NO_PINGS	"No pings"
#define PING_STAT	"Pings sent/got: %d/%d - %d %% loss"
#endif /* PINGSTAT */



#endif /* ifndef MESSAGE_H_ */
