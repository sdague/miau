/*
 * -------------------------------------------------------
 * Copyright (C) 2003-2007 Tommi Saviranta <wnd@iki.fi>
 *      (C) 2002 Lee Hardy <lee@leeh.co.uk>
 *      (C) 1998-2002 Sebastian Kienzl <zap@riot.org>
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

#include "server.h"
#include "client.h"
#include "llist.h"
#include "messages.h"
#include "irc.h"
#include "miau.h"
#include "error.h"
#include "tools.h"
#include "perm.h"
#include "ignore.h"
#include "commands.h"
#include "qlog.h"
#include "chanlog.h"
#include "privlog.h"
#include "log.h"
#include "onconnect.h"
#include "automode.h"
#include "remote.h"
#include "etc.h"
#include "common.h"
#include "dcc.h"

#include <string.h>
#include <sys/time.h>
#include <time.h>

#if HAVE_CRYPT_H
#include <crypt.h>
#endif /* ifdef HAVE_CRYPT_H */



serverlist_type	servers;
server_info	i_server;
connection_type	c_server;



/*
 * Drop connection to the server.
 *
 * Give client (and server) a reason, which may be set to NULL.
 */
void
server_drop(char *reason)
{
	llist_node	*node;
	llist_node	*nextnode;
	const char	*part_reason;

	if (reason == NULL) {
		part_reason = SERV_DISCONNECT;
	} else {
		part_reason = reason;
	}

	/* As we're no longer connected to server, send queue is useless. */
	irc_clear_queue();
	
	if (c_server.socket) {
		char buf[IRC_MSGLEN];
		int r;

		snprintf(buf, IRC_MSGLEN, "QUIT :%s\r\n", reason);
		buf[IRC_MSGLEN - 1] = '\0';
		r = sock_setblock(c_server.socket);
		irc_write_real(&c_server, buf);
#ifdef CHANLOG
		chanlog_write_entry_all(LOG_QUIT, LOGM_QUIT,
				get_short_localtime(), status.nickname, 
				reason, "", "");
#endif /* ifdef CHANLOG */
		sock_close(&c_server);
	}
	i_server.connected = 0;

	/*
	 * Walk active_channels and either remove channels or move to
	 * passive_channels, depending on cfg.rejoin.
	 */
	node = active_channels.head;
	while (node != NULL) {
		channel_type *data;
		nextnode = node->next;
		data = (channel_type *) node->data;

#ifdef AUTOMODE
		/* We no longer know if we're an operator or not. */
		data->oper = -1;
#endif /* ifdef AUTOMODE */
		/*
		 * We don't need to reset channel topic, it will be erased/reset
		 * when joining the channel.
		 */

		if (cfg.chandiscon == 1) {
			irc_mwrite(&c_clients, ":%s NOTICE %s :%s",
					status.nickname,
					(char *) data->name,
					part_reason);
		} else if (cfg.chandiscon == 2) {
			/*
			 * RFC 2812 says "Servers MUST be able to parse
			 * arguments in the form of a list of target, but
			 * SHOULD NOT use lists when sending PART messages to
			 * clients." and therefore we don't part all the
			 * channels with one command.
			 */
			irc_mwrite(&c_clients, ":%s!%s@%s PART %s :%s",
					status.nickname,
					i_client.username,
					i_client.hostname,
					(char *) data->name,
					part_reason);
		}

		if (cfg.rejoin == 1) {
			char *simple;
			llist_node *node;
			/*
			 * Need to move channel from active_channels to
			 * passive_channels. Also revert real channel name
			 * (which we know sure sure) back to simple form.
			 * There's no need to reset simple name -- it won't
			 * change.
			 */
			simple = channel_simplify_name(data->name);
			xfree(data->name);
			data->name = simple;
			data->name_set = 0;

			node = llist_create(data);
			llist_add_tail(node, &passive_channels);
		} else {
			/*
			 * * Not moving channels from list to list, therefore
			 * freeing resources.
			 */
			channel_free(data);
		}
		/* Remove channel node from old list. */
		llist_delete(node, &active_channels);

		node = nextnode;
	}

	if (cfg.chandiscon == 3) {
		irc_mwrite(&c_clients, ":miau PRIVMSG %s: %s",
				status.nickname,
				part_reason);
	}

	/* Reset server-name. */
	xfree(i_server.realname);
	i_server.realname = xstrdup("miau");

	/* Don't try connecting next server right away. */
	timers.connect = 0;

#ifdef UPTIME
	status.startup = 0;	/* Reset uptime-counter. */
#endif /* ifdef UPTIME */
} /* void server_drop(char *reason) */



/*
 * Set "fallback" or "failsafe" server.
 *
 * This server is needed when no server have been defined and we need some
 * real information about the server we're connected to.
 */
void
server_set_fallback(const llist_node *safenode)
{
	server_type	*fallback = (server_type *) servers.servers.head->data;
	server_type	*safe = (server_type *) safenode->data;
	/* Free old data. */

	/*
	 * If we try to replace failsafe-server with failsafe-server, data
	 * will be freed before it can be copied. This is why we won't do
	 * that.
	 */
	if (fallback == safe) {
		return;
	}

	xfree(fallback->name);
	xfree(fallback->password);

	/* Copy current values to fallback server. */
	fallback->name = xstrdup(safe->name);
	fallback->port = safe->port;
	fallback->password = (safe->password != NULL) ? 
		xstrdup(safe->password) : NULL;
	fallback->working = 1;
	fallback->timeout = safe->timeout;
} /* void server_set_fallback(const llist_node *safenode) */



/*
 * Resets all servers to 'working'.
 */
void
server_reset(void)
{
	llist_node	*node;

	for (node = servers.servers.head->next; node != NULL;
			node = node->next) {
		((server_type *) node->data)->working = 1;
	}
} /* void server_reset(void) */



/*
 * Change server.
 *
 * If next is set and there are no more servers to connect, miau will either
 * reset all servers to working or quit, depending on the configuration.
 *
 * If disable != 0, old server will be marked as disfunctional.
 */
void
server_change(int next, int disable)
{
	llist_node *i;

	if (status.good_server == 1) {
		status.good_server = 0;
		return;
	}

	i_server.connected = 0;

	if (servers.amount == 1) {
		server_check_list();

		return;
	}

	if (status.reconnectdelay == CONN_DISABLED) {
		return;
	}

	i = i_server.current;

	if (servers.fresh == 1) {
		/*
		 * We don't know which server of those new ones we are on, so
		 * let's use the fallback one. It's the server we're on anyway.
		 */
		i_server.current = servers.servers.head;
		i = servers.servers.head;
	}

	if (disable == 1 && i != servers.servers.head) {
		((server_type *) i_server.current->data)->working = 0;
	}

	if (next == 1) {
		do {
			i_server.current = i_server.current->next;
			if (i_server.current == NULL) {
				i_server.current = servers.servers.head->next;
			}
		} while (! ((server_type *) i_server.current->data)->working &&
				i_server.current != i);
	}
	
	if (((server_type *) i_server.current->data)->working == 0) {
		if (cfg.nevergiveup == 1) {
			report(MIAU_OUTOFSERVERSNEVER);
			server_reset();
			i_server.current = servers.servers.head->next;
		} else {
			error(MIAU_OUTOFSERVERS);
			exit(EXIT_SUCCESS);
		}
	}

	if (next == 0) {
		report(SOCK_RECONNECTNOW,
				((server_type *) i_server.current->data)->name);
		timers.connect = cfg.reconnectdelay - 1;
		if (timers.connect < 0) {
			timers.connect = 0;
		}
	} else if (i == i_server.current) {
		report(SOCK_RECONNECT,
				((server_type *) i_server.current->data)->name,
				cfg.reconnectdelay);
		timers.connect = 0;
	}
} /* void server_change(int next, int disable) */



void
server_commands(char *command, char *param, int *pass)
{
	upcase(command);

	if (xstrcmp(command, "PING") == 0) {
		*pass = 0;
		if (param != NULL && *param == ':') param++;
		/* Don't make this global (see ERROR) */
		if (param) {
			/*
			 * Although there should be no need to PONG the server
			 * if there is stuff in queue (server shouldn't need
			 * to ping client if it's sending data), we priorize
			 * PONGs over everything else - just in case.
			 */
			irc_write_head(&c_server, "PONG :%s", param);
		}
	}

	else if (xstrcmp(command, "ERROR") == 0) {
		*pass = 0;

		server_drop((param == NULL) ? SERV_ERR : param);
		irc_mwrite(&c_clients, MIAU_CLOSINGLINK,
				(param == NULL) ? SERV_ERR : param);
		drop_newclient(NULL);
		error(IRC_SERVERERROR, (param == NULL) ? "unknown" : param);

		server_change(1, i_server.connected == 0);
	}
} /* void server_commands(char *command, char *param, int *pass) */



static void
parse_msg_me_ctcp(const char *origin, const char *nick, const char *hostname,
		const char *param1, const char *param2, int cmdindex, int *pass)
{
#ifdef DCCBOUNCE
	if (c_clients.connected > 0
			&& cmdindex == CMD_PRIVMSG
			&& cfg.dccbounce
			&& (xstrcasecmp(param2 + 2, "DCC\1") == 0)) {
		char dcct[IRC_MSGLEN];
		strncpy(dcct, param2 + 1, IRC_MSGLEN);
		if (dcc_initiate(dcct, IRC_MSGLEN, 0)) {
			irc_mwrite(&c_clients, ":%s PRIVMSG %s :%s",
					origin, param1, dcct);
			*pass = 0;
		}
	}
#endif /* ifdef DCCBOUNCE */
#ifdef DCCBOUNCE
#ifdef CTCTPREPLIES
	else
#endif /* ifdef CTCPREPLIES */
#endif /* ifdef DCCBOUNCE */
#ifdef CTCPREPLIES
	if (! is_ignore(hostname, IGNORE_CTCP)
			&& c_clients.connected == 0
			&& status.allowreply == 1) {
		report(CLNT_CTCP, param2 + 1, origin);
		
		if (xstrcmp(param2 + 2, "VERSION\1") == 0) {
			irc_notice(&c_server, nick, VERSIONREPLY);
		}
		
		else if (xstrcmp(param2 + 2, "PING") == 0) {
			if (strlen(param2 + 1) > 6) {
				irc_notice(&c_server, nick, "%s", param2 + 1);
			}
		}
		
		else if (xstrcmp(param2 + 2, "CLIENTINFO\1") == 0) {
			irc_notice(&c_server, nick, CLIENTINFOREPLY);
		}

		ignore_add(hostname, 6, IGNORE_CTCP);
		status.allowreply = 0;
		timers.reply = 0;
	} /* CTCP-replies */
	else if (is_ignore(hostname, IGNORE_CTCP)
			|| status.allowreply == 0) {
		report(CLNT_CTCPNOREPLY, param2 + 1, origin);
	}
#endif /* ifdef CTCPREPLIES */
} /* static void parse_msg_me_ctcp(const char *origin, const char *nick,
		const char *hostname, const char *param1, const char *param2,
		int cmdindex, int *pass) */



static int
parse_msg_me(const char *origin, const char *nick, const char *hostname,
		const char *param1, const char *param2, int cmdindex, int *pass)
{
	if (is_perm(&ignorelist, origin)) {
		return 0;
	}

#ifdef PRIVLOG
	/* Should we log? */
	if ((c_clients.connected == 0 && (cfg.privlog & 0x01))
			|| (c_clients.connected > 0 && (cfg.privlog & 0x02))) {
		privlog_write(nick, PRIVLOG_IN, cmdindex, param2 + 1);
	}	
#endif /* ifdef PRIVLOG */

	/* Is this a special (CTCP/DCC) -message ? */
	if (param2[1] == '\1') {
		parse_msg_me_ctcp(origin, nick, hostname, param1, param2,
				cmdindex, pass);
		return 0;
	}
#ifdef NEED_CMDPASSWD
	/* Remote command for bouncer. */
	else if (cfg.cmdpasswd != NULL && param2 != NULL
			&& param2[0] != '\0' && param2[1] == ':') {
		int passok = 0;
		int passlen;
		char *lparam;
		lparam = xstrdup(param2 + 2);
		
		passlen = pos(lparam, ' ');
		if (passlen != -1) {
			lparam[passlen] = '\0';

			if (strlen(cfg.cmdpasswd) == 13) {
				/* Assume it's crypted */
				if (xstrcmp(crypt(lparam, cfg.cmdpasswd),
							cfg.cmdpasswd) == 0) {
					passok = 1;
				}
			} else if (xstrcmp(lparam, cfg.cmdpasswd) == 0) {
				passok = 1;
			}
			lparam[passlen] = ' ';
		}

		if (passok) {
			char *t;
			char *command;
			char *params = NULL;

			t = strtok(lparam, " ");
			command = strtok(NULL, " ");
			if (command != NULL) {
				params = strchr(command, '\0') + 1;
			}
			if (params != NULL) {
				upcase(command);
				*pass = remote_cmd(command, params, nick);
				xfree(lparam);
				return 0;
			}
		}
		xfree(lparam);
	}
#endif /* ifdef NEED_CMDPASSWD */

	/* Normal PRIVMSG/NOTICE to client. */
#ifdef INBOX
#ifndef QUICKLOG
	/*
	 * Note that we do inbox here only is privmsglog is enabled and
	 * quicklogging is disabled.
	 */
	if (inbox != NULL) {
		/* termination + validity guaranteed */
		fprintf(inbox, "%s(%s) %s\n",
				get_short_localtime(), origin, param2 + 1);
		fflush(inbox);
	}
#endif /* ifdef QUICKLOG */
#endif /* ifdef INBOX */

	if (cfg.forwardmsg) {
		int pos;

		timers.forward = 0;
		if (forwardmsg == NULL) {
			/* initial size */
			/* need space for terminator */
			forwardmsgsize = 1;
		}
		pos = forwardmsgsize - 1;
		forwardmsgsize += strlen(origin) + strlen(param2 + 1) + 4;
		/* strlen("() \n") == 4 */
		forwardmsg = (char *) xrealloc(forwardmsg, forwardmsgsize);
		/* paranoid! */
		snprintf(forwardmsg + pos, forwardmsgsize - pos,
				"(%s) %s\n",
				origin, param2 + 1);
		forwardmsg[forwardmsgsize - 1] = '\0';
	}

	return 1;
} /* static int parse_msg_me(const char *origin, const char *nick,
		const char *hostname, const char *param1, const char *param2,
		int cmdindex, int *pass) */



static int
parse_msg_chan(const char *origin, const char *nick, const char *hostname,
		const char *param1, const char *param2, int cmdindex, int *pass)
{
#ifdef CHANLOG
	channel_type	*chptr;
#endif /* ifdef CHANLOG */
	const char *chan;

	/* channel wallops - notice @#channel etc :-) */
	if ((param1[0] == '@' || param1[0] == '%' || param1[0] == '+')
			&& channel_is_name(param1 + 1) != 0) {
		chan = param1 + 1;
	} else {
		chan = param1;
	}

#ifdef CHANLOG
	/*
	 * evil kludge: it's way too easy to confuse normal message to
	 * channel "++foo" with a channel wallop (mode + to channel
	 * "+foo"), so we have to try both. At least we know to try
	 * the more obvious first.
	 */
	chptr = channel_find(chan, LIST_ACTIVE);
	if (chptr == NULL) {
		chptr = channel_find(param1, LIST_ACTIVE);
	}
	if (chptr != NULL && chanlog_has_log(chptr, LOG_MESSAGE)) {
		char *t;

		t = log_prepare_entry(nick, param2 + 1);
		if (t == NULL) {
			if (cmdindex == CMD_PRIVMSG + MINCOMMANDVALUE) {
				chanlog_write_entry(chptr, LOGM_MESSAGE,
						get_short_localtime(),
						nick, param2 + 1);
			} else { /* must be notice then */
				chanlog_write_entry(chptr, LOGM_NOTICE,
						get_short_localtime(),
						nick, param2 + 1);
			}
		} else {
			chanlog_write_entry(chptr, "%s", t);
		}
	}
#endif /* ifdef CHANLOG */

	return 0;
} /*  static int parse_msg_chan(const char *origin, const char *nick,
		const char *hostname, const char *param1, const char *param2,
		int cmdindex, int *pass) */



static int
parse_privmsg(char *param1, char *param2, char *nick, char *hostname,
		const int cmdindex, int *pass)
{
	char *origin;
	int osize;
	int isprivmsg = 0;

	if (nick == NULL || hostname == NULL || param2 == NULL) {
#ifdef ENDUSERDEBUG
		enduserdebug("parse_privmsg(): "
				"param1 = %s, param2 = %s, "
				"nick = %s, hostname = %d",
				param1 == NULL ? "NULL" : param1,
				param2 == NULL ? "NULL" : param2,
				nick == NULL ? "NULL" : nick,
				hostname == NULL ? "NULL" : hostname);
#endif /* ifdef ENDUSERDEBUG */
		return 0;
	}

	/* paranoid */
	osize = strlen(nick) + strlen(hostname) + 2;
	origin = xmalloc(osize);
	snprintf(origin, osize, "%s!%s", nick, hostname);
	origin[osize - 1] = '\0';

	/* who is it for? */
	if (xstrcasecmp(param1, status.nickname) == 0) {
		parse_msg_me(origin, nick, hostname, param1, param2,
				cmdindex, pass);
	} else {
		parse_msg_chan(origin, nick, hostname, param1, param2,
				cmdindex, pass);
	}

	xfree(origin);

	return isprivmsg;
} /* static int parse_privmsg(char *param1, char *param2, char *nick,
	char *hostname, const int cmdindex, int *pass) */



int
server_read(void)
{
	char	*backup = NULL;
	char	*origin, *command, *param1, *param2;
	int	rstate;
	int	pass = 0;
	int	commandno;

	rstate = irc_read(&c_server);

	if (rstate <= 0) {
		return rstate;
	}

	if (c_server.buffer[0] == '\0') {
		return 0;
	}
	
	/* new data... go for it ! */
	pass = 1;

	backup = xstrdup(c_server.buffer);
		
	if (c_server.buffer[0] == ':') {
		/* reply */
		origin = strtok(c_server.buffer + 1, " ");
		command = strtok(NULL, " ");
		param1 = strtok(NULL, " ");
		param2 = strtok(NULL, "\0");
#ifdef DEBUG
#ifdef OBSOLETE
		printf("[%s] [%s] [%s] [%s]\n", origin, command,
				param1, param2);
#endif /* ifdef OBSOLETE */
#endif /* ifdef DEBUG */
		if (command != 0) {
			commandno = atoi(command);
			if (commandno == 0) {
				commandno = MINCOMMANDVALUE +
					command_find(command);
			}
			server_reply(commandno, backup, origin,
					param1, param2, &pass);
		}
	}
			
	else {
		/* Command */
		command = strtok(c_server.buffer, " ");
		param1 = strtok(NULL, "\0");
				
		if (command) {
			server_commands(command, param1, &pass);
		}
	}

	/* We wouldn't need to check c_clients.connected... */
	if (c_clients.connected > 0 && pass) {
		/*
		 * Having '"%s", buffer' instead of plain
		 * 'buffer' is essential because we don't want
		 * our string processed any further by va.
		 */
		irc_mwrite(&c_clients, "%s", backup);
	}

	xfree(backup);
	
	return 0;
} /* int server_read(void) */




/*
 * Check number of servers and consistency of i_server.currect.
 * 
 * If there are only fallback-server (or no servers at all !?) left,
 * warn the user about this. Also, if the server we're connected to is on the
 * list, set i_server.current to index of it.
 */
void
server_check_list(void)
{
	llist_node	*ptr;
	
	if (servers.amount <= 1) {
		/* There are no other servers ! */
		/* This is important ! */
		irc_mwrite(&c_clients, ":miau NOTICE %s :%s", 
				status.nickname,
				CLNT_NOSERVERS);
		/* Don't try to reconnect. */
		status.reconnectdelay = CONN_DISABLED;
		return;
	}

	/* Next try to connect due in cfg.reconndelay seconds. */
	status.reconnectdelay = cfg.reconnectdelay;

	/* See if our server is on the list. */
	for (ptr = servers.servers.head->next; ptr != NULL; ptr = ptr->next) {
		/* We'll just forget the passwords. Right ? */
		if (xstrcmp(((server_type *) i_server.current->data)->name,
					((server_type *) ptr->data)->name) == 0
				&& ((server_type *)
					i_server.current->data)->port ==
				((server_type *) ptr->data)->port) {
			i_server.current = ptr;
			servers.fresh = 0;
		}
	}
} /* void server_check_list(void) */



void
server_reply(const int command, char *original, char *origin, char *param1,
		char *param2, int *pass)
{
	channel_type	*chptr;
	char		*work = NULL;
	char		*nick, *hostname;
	char		*t;
	int		isprivmsg = 0;
	int		n;

	t = strchr(origin, '!');
	if (t != NULL) {
		*t = '\0';
		t++;
		nick = xstrdup(origin);
		hostname = xstrdup(t);
	} else {
		nick = xstrdup(origin);
		hostname = xstrdup(origin);
	}

	switch (command) {
		/* Replies. */
	
		/* Just signed in to server. */
		case RPL_WELCOME:
			i_server.connected++;

			xfree(i_server.realname);
			i_server.realname = xstrdup(origin);

			xfree(status.nickname);
			status.nickname = xstrdup(param1);

			xfree(i_server.greeting[0]);
			i_server.greeting[0] = xstrdup(param2);
			n = lastpos(i_server.greeting[0], ' ');
			if (n != -1) {
				i_server.greeting[0][n] = '\0';
			}

			xfree(status.idhostname);
			t = strchr(param2, '!');
			if (t != NULL) {
				status.idhostname = xstrdup(t + 1);
				status.goodhostname =
					pos(status.idhostname, '@') + 1;
			} else {
				/*
				 * While not giving hostname is ok with RFC,
				 * clients like Chatzilla expect to get it.
				 * Thanks to James Ross and Oliver Eikemeier
				 * for pointing this out.
				 *
			 * http://bugzilla.mozilla.org/show_bug.cgi?id=242095
				 */
				status.idhostname = xstrdup("miau@miau");
				status.goodhostname = 5;
			}

#ifdef UPTIME
			if (! status.startup) {
				time(&status.startup);
			}
#endif /* ifdef UPTIME */

			report(IRC_CONNECTED, i_server.realname);

#ifdef ONCONNECT
			onconnect_do();
#endif /* ifdef ONCONNECT */

			/* Set user modes if any and if no clients connected. */
			if (cfg.usermode != NULL && c_clients.connected == 0) {
				irc_write(&c_server, "MODE %s %s",
						status.nickname,
						cfg.usermode);
			}
			/* 
			 * Be default we're not away, but set_away() may
			 * change this.
			 */
			status.awaystate &= ~AWAY;
			set_away(NULL);	/* No special message. */

			/* See if we should join channels. */
			timers.join = JOINTRYINTERVAL;
			/* Also reset channel's join-count. */
			LLIST_WALK_H(passive_channels.head, channel_type *);
				data->jointries = JOINTRIES_UNSET;
			LLIST_WALK_F;

			for (n = 0; n < RPL_ISUPPORT_LEN; n++) {
				FREE(i_server.isupport[n]);
			}

			break;

		/* More registeration-time replies... */
		case RPL_YOURHOST:
		case RPL_CREATED:
		case RPL_MYINFO:
			xfree(i_server.greeting[command - 1]);
			i_server.greeting[command - 1] = xstrdup(param2);
			break;
		
		/* Supported features */
		case RPL_ISUPPORT:
			for (n = 0; n < RPL_ISUPPORT_LEN; n++) {
				if (i_server.isupport[n] == NULL) {
					i_server.isupport[n] = xstrdup(param2);
					break;
				}
			}
			break;

		/* This server is restricted. */
		case RPL_RESTRICTED:
			if (cfg.jumprestricted) {
				server_drop(CLNT_RESTRICTED);
				report(SERV_RESTRICTED);
				server_change(1, 1);
			}
			break;
			
		/* Channel has no topic. */
		case RPL_NOTOPIC:
			/*
			 * :<server> RPL_NOTOPIC <client> <channel>
			 * 	:No topic is set
			 */
			t = strchr(param2, ' ');
			if (t != NULL) {
				*t = '\0';
			} else {
				break;
			}

			chptr = channel_find(param2, LIST_ACTIVE);
			if (chptr != NULL) {
				channel_topic(chptr, NULL);
			}
			break;

		/* Channel topic is... */
		case RPL_TOPIC:
			/* :<server> RPL_TOPIC <client> <channel> :<topic> */
			{
				channel_type	*chptr;
				char		*p;
				
				p = strchr(param2, ' ');
				if (p != NULL) {
					*p++ = '\0';
				} else {
					return;
				}
				
				chptr = channel_find(param2, LIST_ACTIVE);
				if (chptr != NULL) {
					channel_topic(chptr, p);
				}
			}
			break;
		
		/* Who set this topic ? */
		case RPL_TOPICWHO:
			/*
			 * :<server> RPL_TOPICWHO <client>
			 *	<channel> <who> <time>
			 */
			{
				channel_type	*chptr;
				char		*p;
				char		*topicwho;
				
				p = strchr(param2, ' ');
				if (p != NULL) {
					*p++ = '\0';
				} else {
					return;
				}
				
				topicwho = p;
				
				p = strchr(topicwho, ' ');
				if (p != NULL) {
					*p++ = '\0';
				} else {
					return;
				}
				
				chptr = channel_find(param2, LIST_ACTIVE);
				if (chptr != NULL) {
					channel_when(chptr, topicwho, p);
				}
			}
			break;

		/* Channel has modes. */
		case RPL_CHANNELMODEIS:
			/*
			 * :server RPL_CNANNELMODEIS <client>
			 *	<channel> <mode> <mode params>
			 */
			/* Kludge. :-) */
			t = strchr(param2, ' ');
			if (t != NULL) {
				char	*channel;
				*t = '\0';
				channel = xstrdup(param2);
				*t = ' ';
				
				parse_modes(channel, nextword(param2));
				xfree(channel);
			}
			break;

		case RPL_NAMREPLY:
			/*
			 * :server RPL_NAMREPLY <client>
			 *	<type> <channel> :<[@ / +]<nick>> ...
			 */
			{
				char	*channel;
				work = xstrdup(param2);

				t = strchr(work, ' ');
				if (t == NULL) {
#ifdef ENDUSERDEBUG
					enduserdebug("no channel at NAMREPLY");
#endif /* ifdef ENDUSERDEBUG */
					break;
				}
				channel = strtok(t + 1, " ");
				chptr = channel_find(channel, LIST_ACTIVE);
#ifdef AUTOMODE
				if (chptr == NULL || chptr->oper != -1) {
#else /* ifdef AUTOMODE */
				if (chptr == NULL) {
#endif /* ifdef else AUTOMODE */
#ifdef ENDUSERDEBUG
					if (chptr == NULL) {
						enduserdebug("NAMREPLY on unjoined channel (%s)", channel);
					}
#endif /* ifdef ENDUSERDEBUG */
					break;
				}

				t = strtok(NULL, " ");
#ifdef ENDUSERDEBUG
				if (t[0] != ':') {
					enduserdebug("no users on NAMREPLY");
					break;
				}
#endif /* ifdef ENDUSERDEBUG */
				n = 0;
				while (t != NULL) {
					n++;
					t = strtok(NULL, " ");
				}
#ifdef AUTOMODE
				chptr->oper = (n == 1 ? 1 : 0);
#endif /* ifdef AUTOMODE */
			}
			break;


		/* Error replies. */

		/* Couldn't join channel. */
		case ERR_INVITEONLYCHAN:
		case ERR_CHANNELISFULL:
		case ERR_TOOMANYTARGETS:
		case ERR_BANNEDFROMCHAN:
		case ERR_BADCHANNELKEY:
		case ERR_BADCHANMASK:
		case ERR_TOOMANYCHANNELS:
		case ERR_UNAVAILRESOURCE:
			/* 
			 * Look for channel and see if we're tryingto join it.
			 */
			work = xstrdup(param2);
			t = strtok(work, " ");
			chptr = channel_find(t, LIST_PASSIVE);
			if (chptr == NULL) {
				break;
			}

			if (chptr->jointries > 0) {
				/*
				 * Automatic join, suppress error.
				 * Nice things, btw, when last try was made
				 * to join the channel, jointries was set to
				 * 0 which means we get to pass the message
				 * to the client.
				 */
				*pass = 0;
			}
			break;
				

			
		/* Commands. */

		/* Someone chaning nick. */
		case CMD_NICK + MINCOMMANDVALUE:
			/* Is that us who changed nick ? */
			if (xstrcasecmp(status.nickname, nick) == 0) {
				xfree(status.nickname);
				status.nickname = xstrdup(param1 + 1);

				if (xstrcasecmp(status.nickname,
						(char *) nicknames.nicks.head->data) == 0) {
					status.got_nick = 1;
					report(MIAU_GOTNICK, status.nickname);
					status.getting_nick = 0;
				} else {
					status.got_nick = 0;
				}
			}

#ifdef CHANLOG
			chanlog_write_entry_all(LOG_NICK, LOGM_NICK,
					get_short_localtime(),
					nick, param1 + 1);
#endif /* ifdef CHANLOG */
			break;

		/* Ping ?  Pong. */
		case CMD_PONG + MINCOMMANDVALUE:
			/* We don't need to reset timer - it is done in irc.c */
#ifdef PINGSTAT
			ping_got++;
#endif /* ifdef PINGSTAT */
			*pass = 0;
			break;

		/* Look ma, he's flying. */
		case CMD_KICK + MINCOMMANDVALUE:
			chptr = channel_find(param1, LIST_ACTIVE);
			if (chptr == NULL) {
#ifdef ENDUSERDEBUG
				enduserdebug("KICK on channel we're not on");
				enduserdebug("command = %d param1 = '%s' "
						"param2 = '%s'",
						command, param1, param2);
#endif /* ifdef ENDUSERDEBUG */
				break;
			}

#ifdef CHANLOG
			if (chanlog_has_log(chptr, LOG_PART)) {
				t = strchr(param2, ' ');
				/* Ugly, done because we cant break up param2 */
				if (t != NULL) {
					char	*target;

					*t = '\0';
					target = xstrdup(param2);
					*t = ' ';

					chanlog_write_entry(chptr, LOGM_KICK,
							get_short_localtime(),
							target, nick, 
							nextword(param2) + 1);
					xfree(target);
				}
			}
#endif /* ifdef CHANLOG */

			/* Me being kicked ? */
			{
				size_t t;
				t = pos(param2, ' ');
				if (xstrncasecmp(status.nickname,
							param2, t) == 0 &&
						strlen(status.nickname) == t) {
					report(CLNT_KICK, origin, param1,
							nextword(param2) + 1);
					channel_rem(chptr, LIST_ACTIVE);
				}
			}
			break;

		/* Someone joining. */
		case CMD_JOIN + MINCOMMANDVALUE:
			n = (param1[0] == ':' ? 1 : 0);
			/* Was that me ? */
			if (xstrcasecmp(status.nickname, nick) == 0) {
				/* Add channel to active list. */
				channel_add(param1 + n, param2, LIST_ACTIVE);
			}

			/* Get pointer to this channel. */
			chptr = channel_find(param1 + n, LIST_ACTIVE);
			if (chptr == NULL) {
#ifdef ENDUSERDEBUG
				enduserdebug("JOIN on channel we're not on");
				enduserdebug("command = %d / param1 + n = '%s'"
						" / param2 = '%s'",
						command, param1 + n, param2);
#endif /* ifdef ENDUSERDEBUG */
				break;
			}

#ifdef AUTOMODE
			/* Don't care if it's me joining. */
			if (xstrcasecmp(nick, status.nickname) != 0) {
				automode_queue(nick, hostname, chptr);
			}
#endif /* ifdef AUTOMODE */
	
#ifdef CHANLOG
			if (chanlog_has_log(chptr, LOG_JOIN)) {
				chanlog_write_entry(chptr, LOGM_JOIN,
						get_short_localtime(),
						nick, hostname,
						chptr->simple_name);
			}
#endif /* ifdef CHANLOG */

			break;

		/* ...and someone parting our party. */
		case CMD_PART + MINCOMMANDVALUE:
			/* Get pointer to this channel. */
			chptr = channel_find(param1, LIST_ACTIVE);
			/* 
			 * If we have sent PART (or JOIN 0) lets assume we
			 * have parted those channels. This means that channel
			 * entries of those channels are removed from
			 * active_channels and therefore cannot be found. This
			 * is why why "chptr == NULL" is nothing to worry
			 * about.
			 */
			if (chptr == NULL) { break; }

#ifdef AUTOMODE
			/* No automodes for that person. */
			automode_drop_channel(chptr, nick, '\0');
#endif /* ifdef AUTOMODE */

#ifdef CHANLOG
			if (chanlog_has_log(chptr, LOG_PART)) {
				chanlog_write_entry(chptr, LOGM_PART,
						get_short_localtime(),
						nick, chptr->simple_name,
						(param2) ? (param2 + 1) ?
						param2 + 1 : "" : "");
			}
#endif /* ifdef CHANLOG */

			/* Remove channel from list if it was me leaving. */
			if (xstrcasecmp(nick, status.nickname) == 0) {
				channel_rem(chptr, LIST_ACTIVE);
			}

			break;

		/* Someone's leaving for good. */
		case CMD_QUIT + MINCOMMANDVALUE:
#ifdef AUTOMODE
			automode_drop_nick(nick, '\0');
#endif /* ifdef AUTOMODE */

#ifdef CHANLOG
			chanlog_write_entry_all(LOG_QUIT, LOGM_QUIT,
					get_short_localtime(),
					nick, param1 + 1, " ", param2);
#endif /* ifdef CHANLOG */

			break;

		/* Ouch. That must hurt. Someone just went down, hard. */
		case CMD_KILL + MINCOMMANDVALUE:
			/* Me ?-) */
			if (xstrcasecmp(status.nickname, nick) == 0) {
				error(IRC_KILL, nick);
			}
			*pass = 0;	/* We'll handle this by ourself. */

			break;

		/* Changing modes... */
		case CMD_MODE + MINCOMMANDVALUE:
			if (status.goodhostname == 0
					&& pos(hostname, '@') != -1
					&& xstrcasecmp(param1,
						status.nickname) == 0) {
				xfree(status.idhostname);
				status.idhostname = xstrdup(hostname);
				status.goodhostname = pos(hostname, '@') + 1;
			}
			chptr = channel_find(param1, LIST_ACTIVE);
			if (chptr == NULL) {
#ifdef ENDUSERDEBUG
				if (xstrcasecmp(param1, status.nickname) != 0) {
					enduserdebug("MODE on unknown channel");
				}
#endif /* ifdef ENDUSERDEBUG */
				break;
			}

			parse_modes(param1, param2);
			
#ifdef CHANLOG
			if (chanlog_has_log(chptr, LOG_MODE)) {
				chanlog_write_entry(chptr, LOGM_MODE,
						get_short_localtime(),
						nick, param2);
			}
#endif /* ifdef CHANLOG */

			break;

		/* Someone changing topic. */
		case CMD_TOPIC + MINCOMMANDVALUE:
			/* :<source> TOPIC <channel> :<topic> */

			chptr = channel_find(param1, LIST_ACTIVE);
			if (chptr == NULL) {
#ifdef ENDUSERDEBUG
				enduserdebug("TOPIC on channel we're not on");
#endif /* ifdef ENDUSERDEBUG */
				break;
			}

			{
				struct timeval	now;
				struct timezone	tz;
				char		timebuf[20];

				channel_topic(chptr, param2);

				gettimeofday(&now, &tz);
				/*
				 * strftime("%s") can't be used (not in ISO C),
				 * This should work as a replacement,
				 */
				snprintf(timebuf, 20, "%d", (int) now.tv_sec
						- tz.tz_minuteswest * 60);
				timebuf[19] = '\0';
				channel_when(chptr, origin, timebuf);
			}

#ifdef CHANLOG
			if (chanlog_has_log(chptr, LOG_MISC)) {
				chanlog_write_entry(chptr, LOGM_TOPIC,
						get_short_localtime(), nick,
						(param2 + 1) ? param2 + 1 : "");
			}
#endif /* ifdef CHANLOG */
			
			break;

		/* I hear someone talking... */
		case CMD_NOTICE + MINCOMMANDVALUE:
		case CMD_PRIVMSG + MINCOMMANDVALUE:
			/* hostname = username@hostname */
			isprivmsg = parse_privmsg(param1, param2,
					nick, hostname, command, pass);
			break;

#ifdef OBSOLETE /* Dummy. */
		default:
			/* Got something we don't recognize. */
			break;
#endif /* ifdef OBSOLETE */
	}

	if (command < MINCOMMANDVALUE) {
		/*
		 * Client is connected, we're registered with the server and
		 * there are error messages we want to handle.
		 */
		if (i_server.connected && status.getting_nick > 0 &&
				(command == ERR_NICKNAMEINUSE ||
					command == ERR_UNAVAILRESOURCE)) {
			/* Decrease counter for sent NICK-commands. */
			status.getting_nick--;
			*pass = 0;
		}

		/*
		 * There are a few things we don't need to pass to qlog:
		 *	- message from server acknowleding we're away
		 *	- welcome-messages
		 */
		if (c_clients.connected == 0 &&
				(command == RPL_NOWAWAY || command <= 4)) {
			*pass = 0;
		}

		/*
		 * We're trying to connect the server and we're getting
		 * something from the server.
		 */
		if (i_server.connected != 2 &&
				(command == ERR_UNAVAILRESOURCE ||
					command == ERR_NICKNAMEINUSE ||
					command == ERR_ERRONEUSNICKNAME)) {
			get_nick(command == ERR_UNAVAILRESOURCE ?
					IRC_NICKUNAVAIL :
					(command == ERR_NICKNAMEINUSE) ?
					IRC_NICKINUSE : IRC_BADNICK);
		}
	} /* Command 000-999 */
	
#ifdef QUICKLOG
	/* Perhaps we need to write something to quicklog. */
	if ((c_clients.connected == 0 || cfg.flushqlog == 0) && *pass &&
				param1 != NULL) {
		qlog_write(isprivmsg, "%s", original);
	}
#endif /* ifdef QUICKLOG */

	xfree(work);
	xfree(nick);
	xfree(hostname);
} /* void server_reply(const int command, char *original, char *origin,
		char *param1, char *param2, int *pass) */



void
parse_modes(const char *channel, const char *original)
{
	channel_type	*chptr = channel_find(channel, LIST_ACTIVE);
	char		*buf;
	char		*ptr;
	char		*param;
	char		modetype = '+';

	if (chptr == NULL) {
#ifdef ENDUSERDEBUG
		enduserdebug("MODE on channel we're not on");
#endif /* ifdef ENDUSERDEBUG */
		return;
	}

	buf = xstrdup(original);

	ptr = strtok(buf, " ");
	param = strtok(NULL, " ");
	while (ptr[0] != '\0') {
		/* See if we found modetype. */
		if (ptr[0] == '+' || ptr[0] == '-') {
			modetype = ptr[0];
			/* Assume there's only [-+]. */
			ptr++;
		}

		switch (ptr[0]) {
			/* miau thinks 'o' and 'O' are the same. */
			case 'O':	/* Channel creator. */
			case 'o':	/* Channel operator. */
			case 'v':	/* Voice privilege. */
#ifdef AUTOMODE
				/*
				 * Appears that some servers think 'O' flag is
				 * for "oper only" channel (no parameter), some
				 * think it's a user flag for channel creator.
				 * This means if 'O' comes with no parameter,
				 * we can pretty much safely ignore it.
				 */
				
				if (ptr[0] == 'O' || param == NULL) {
					break;
				}
				if (ptr[0] == 'O') {
					ptr[0] = 'o';
				}
				if (modetype == '-') {	/* Taking... */
					if (xstrcasecmp(status.nickname,
								param) == 0
							&& ptr[0] == 'o') {
						/* They took my pride... */
						chptr->oper = 0;
					}
				} else {
					if (xstrcasecmp(status.nickname,
								param) == 0
							&& ptr[0] == 'o') {
						chptr->oper = 1;
					} else {
						automode_drop_channel(chptr,
								param,
								ptr[0]);
					}
				}

#endif /* ifdef AUTOMODE */
				param = strtok(NULL, " ");
				break;

			case 'k':	/* Channel key. */
				/*
				 * If there's a channel with key "123" and user
				 * limit "321" at GalaxyNet, mode query returns
				 * "+lk 123". I suppose we just have to live
				 * without that missing parameter.
				 */
				if (modetype == '+' && param != NULL) {
					chptr->key = xstrdup(param);
				}
				/* No need to clear unset key. */
				/* Even removing key needs parameter. */
				param = strtok(NULL, " ");
				break;
				
			case 'l':	/* Limit. */
				/*
				 * It's not like we would care, but we need
				 * to jump to next parameter.
				 */
				if (modetype == '+') {
					param = strtok(NULL, " ");
				}
				break;

			case 'b':	/* Ban mask. */
			case 'e':	/* Exception mask. */
			case 'I':	/* Invitation mask. */
				/* Get next parameter. */
				param = strtok(NULL, " ");
				break;

#if USE_DISABLED
			case 'a':	/* anonymous */
			case 'i':	/* invite-only */
			case 'm':	/* moderated */
			case 'n':	/* no messages from outside */
			case 'q':	/* quiet */
			case 's':	/* secret */
			case 'r':	/* server re-op */
			case 't':	/* topic */
#endif /* ifdef USE_DISABLED */
		}

		ptr++;
	}

	xfree(buf);
} /* void parse_modes(const char *channel, const char *original) */
