/*
 * -------------------------------------------------------
 * Copyright (C) 2003-2004 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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

#include "server.h"
#include "miau.h"

#include "automode.h"
#include "channels.h"
#include "commands.h"
#include "irc.h"
#include "llist.h"
#include "messages.h"
#include "perm.h"
#include "chanlog.h"
#include "privlog.h"
#include "log.h"
#include "qlog.h"
#include "dcc.h"
#include "tools.h"
#ifdef _NEED_PROCESS_IGNORES
#  include "ignore.h"
#endif /* _NEED_PROCESS_IGNORES */
#include "remote.h"
#include "onconnect.h"


serverlist_type	servers;
server_info	i_server;
connection_type	c_server;



extern clientlist_type	c_clients;
extern timer_type	timers;
extern char		*forwardmsg;



/*
 * Drop connection to the server.
 *
 * Give client (and server) a reason, which may be set to NULL.
 */
void
server_drop(
		char	*reason
	   )
{
	llist_node	*node;
	llist_node	*nextnode;
	channel_type	*data;

	/* As we're no longer connected to server, send queue is useless. */
	irc_clear_queue();
	
	if (c_server.socket) {
		sock_setblock(c_server.socket);
		irc_write(&c_server, "QUIT :%s", reason);
#ifdef CHANLOG
		chanlog_write_entry_all(LOG_QUIT, LOGM_QUIT,
				get_short_localtime(), status.nickname, 
				reason, "", "");
#endif /* CHANLOG */
		sock_close(&c_server);
	}
	i_server.connected = 0;

	/*
	 * Walk active_channels and either remove channels or move to
	 * passive_channels, depending on cfg.rejoin.
	 */
	node = active_channels.head;
	while (node != NULL) {
		nextnode = node->next;
		data = (channel_type *) node->data;

#ifdef AUTOMODE
		/* We no longer know if we're an operator or not. */
		data->oper = -1;
#endif /* AUTOMODE */
		/*
		 * We don't need to reset channel topic, it will be erased/reset
		 * when joining the channel.
		 */

		/*
		 * RFC 2812 says "Servers MUST be able to parse arguments in
		 * the form of a list of target, but SHOULD NOT use lists when
		 * sending PART messages to clients." and therefore we don't
		 * part all the channels with one command.
		 */
		irc_mwrite(&c_clients, ":%s!%s@%s PART %s :%s",
				status.nickname,
				i_client.username,
				i_client.hostname,
				(char *) data->name,
				(reason != NULL ? reason : SERV_DISCONNECT));

		if (cfg.rejoin) {
			/*
			 * Need to move channel from active_channels to
			 * passive_channels.
			 */
			llist_add_tail(llist_create(data), &passive_channels);
		} else {
			/*
			 * * Not moving channels from list to list, therefore
			 * freeing resources.
			 */
			xfree(data->name);
			xfree(data->topic);
			xfree(data->topicwho);
			xfree(data->key);
			xfree(data);
		}
		/* Remove channel node from old list. */
		llist_delete(node, &active_channels);

		node = nextnode;
	}

	/* Reset server-name. */
	xfree(i_server.realname);
	i_server.realname = strdup("miau");

	/* Don't try connecting next server right away. */
	timers.connect = 0;

#ifdef UPTIME
	status.startup = 0;	/* Reset uptime-counter. */
#endif /* UPTIME */
} /* void server_drop(char *) */



/*
 * Set "fallback" or "failsafe" server.
 *
 * This server is needed when no server have been defined and we need some
 * real information about the server we're connected to.
 */
void
server_set_fallback(
		const llist_node	*safenode
		)
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
	fallback->name = strdup(safe->name);
	fallback->port = safe->port;
	fallback->password = (safe->password != NULL) ? 
		strdup(safe->password) : NULL;
	fallback->working = 1;		/* TODO: Is this necessary ? */
	fallback->timeout = safe->timeout;
} /* void server_set_fallback(const llist_node *) */



/*
 * Resets all servers to 'working'.
 */
void
server_reset(
	    )
{
	llist_node	*node;

	for (node = servers.servers.head->next; node != NULL;
			node = node->next) {
		((server_type *) node->data)->working = 1;
	}
} /* void server_reset() */



/*
 * Jump to next server on list.
 *
 * If there are no more servers to connect, miau will either reset all servers
 * to working or quit, depending on the configuration.
 *
 * If disablecurrent != 0, old server will be marked as disfunctional.
 */
void
server_next(
		const int	disablecurrent
	   )
{
	llist_node	*i = i_server.current;

	if (status.good_server) {
		status.good_server = 0;
		return;
	}

	i_server.connected = 0;

	if (servers.amount == 1) {
		check_servers();

		return;
	}

	if (servers.fresh == 1) {
		/*
		 * We don't know which server of those new ones we are on, so
		 * let's use the fallback one. It's the server we're on anyway.
		 */
		i_server.current = servers.servers.head;
		i = servers.servers.head;
	}

	if (disablecurrent && i != servers.servers.head) {
		((server_type *) i_server.current->data)->working = 0;
	}

	do {
		i_server.current = i_server.current->next;
		if (i_server.current == NULL) {
			i_server.current = servers.servers.head->next;
		}
	} while (! ((server_type *) i_server.current->data)->working &&
			i_server.current != i);
	
	if (! ((server_type *) i_server.current->data)->working) {
		if (cfg.nevergiveup) {
			report(MIAU_OUTOFSERVERSNEVER);
			server_reset();
			i_server.current = servers.servers.head->next;
		}

		else {
			error(MIAU_OUTOFSERVERS);
			escape();
		}
	}

	if (i == i_server.current) {
		report(SOCK_RECONNECT,
				((server_type *) i_server.current->data)->name,
				cfg.reconnectdelay);
		timers.connect = 0;
	}
} /* void server_next(const int) */



void
server_commands(
		char	*command,
		char	*param,
		int	*pass
	       )
{
	upcase(command);

	if (xstrcmp(command, "PING") == 0) {
		*pass = 0;
		if (param && param[0] == ':') param++;
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
				"", (param == NULL) ? SERV_ERR : param);
		drop_newclient(NULL);
		error(IRC_SERVERERROR, (param == NULL) ? "unknown" : param);

		server_next(i_server.connected == 0);
	}
} /* void server_commands(char *, char *, int *) */



int
parse_privmsg(
		char		*param1,
		char		*param2,
		char		*nick,
		char		*hostname,
		const int	cmdindex,
		int		*pass
	     )
{
#ifdef CHANLOG
	channel_type	*chptr;
#endif /* CHANLOG */
	char		*origin = xmalloc(strlen(nick) + strlen(hostname) + 2);
	int		isprivmsg = 0;
	int		i, l;
	int		normal = 1;	/* ...just a normal message... */

	sprintf(origin, "%s!%s", nick, hostname);

	/* Is it to who ? */
	if (status.nickname && xstrcasecmp(param1, status.nickname) == 0) {
		/* It's for me. Whee ! :-) */
		
#ifdef PRIVLOG
		/* Should we log? */
		if ((c_clients.connected > 0 && (cfg.privlog & 0x02))
				|| (c_clients.connected == 0
					&& (cfg.privlog & 0x01))) {
			privlog_write(nick, PRIVLOG_IN, param2 + 1);
		}	
#endif /* PRIVLOG */
				
		
		/* ignorelist tells who are ignore - not who are allowed. */
		if (! is_perm(&ignorelist, origin)) {
			/* Is this  a special (CTCP/DCC) -message ? */
			if (param2[1] == '\1') {
				normal = 0;
				
				upcase(param2);
#ifdef DCCBOUNCE
				if (c_clients.connected > 0 &&
						cmdindex == CMD_PRIVMSG &&
						cfg.dccbounce &&
						(xstrcmp(param2 + 2, "DCC\1") ==
						 0)) {
					if (dcc_initiate(param2 + 1, 0)) {
						irc_mwrite(&c_clients, ":%s PRIVMSG %s :%s", origin, param1, param2 + 1);
						*pass = 0;
					}
				}
#endif /* DCCBOUNCE */
#ifdef DCCBOUNCE
#  ifdef CTCTPREPLIES
				else
#  endif /* CTCPREPLIES */
#endif /* DCCBOUNCE */
#ifdef CTCPREPLIES
				if (! is_ignore(hostname, IGNORE_CTCP) &&
						c_clients.connected == 0 &&
						status.allowreply) {
					report(CLNT_CTCP, param2 + 1, origin);
					
					if (xstrncmp(param2 + 2, "PING", 4) == 0) {
						if (strlen(param2 + 1) > 6) {
							/* Bad nesting. */
							irc_notice(&c_server,
								nick, "%s",
								param2 + 1);
						}
					}
					
					else if (xstrcmp(param2 + 2, "VERSION\1") == 0) {
						irc_notice(&c_server, nick, VERSIONREPLY);
					}
					
					else if (xstrcmp(param2 + 2, "TIME\1") == 0) {
						time_t	now;
						char	timebuffer[120];
						struct tm *tmptr;

						time(&now);
						tmptr = localtime(&now);
						strftime(timebuffer, 120, "%a %b %d %H:%M:%S %Y", tmptr);
						
						irc_notice(&c_server, nick, TIMEREPLY, timebuffer);
					}

					/* dont bother sending USERINFO/CLIENTINFO/FINGER -- fl_ */
					if (xstrcmp(param2 + 2, "USERINFO\1") == 0) {
						irc_notice(&c_server, nick, USERINFOREPLY);
					}
					if (xstrcmp(param2 + 2, "CLIENTINFO\1") == 0) {
						irc_notice(&c_server, nick, CLIENTINFOREPLY);
					}
					if (xstrcmp(param2 + 2, "FINGER\1") == 0) {
						irc_notice(&c_server, nick, FINGERREPLY);
					}
					
					add_ignore(hostname, 6, IGNORE_CTCP);
					status.allowreply = 0;
					timers.reply = 0;
				} /* CTCP-replies */
				else
				{
					report(CLNT_CTCPNOREPLY, param2 + 1, origin);
				}
#endif /* CTCPREPLIES */
			} /* Special (CTCP/DCC) -message. */
			
#ifdef _NEED_CMDPASSWD
			/* Remote command for bouncer. */
			else if (cfg.cmdpasswd != NULL && param2 != NULL &&
					param2[0] != '\0' && param2[1] == ':') {
				int	passok = 0;
				int	passlen;
				
			passlen = pos(param2 + 2, ' ');
			if (passlen != -1) {
				param2[2 + passlen] = '\0';

				if (strlen(cfg.cmdpasswd) == 13) {
					/* Assume it's crypted */
					/* Bad nesting. */
					if (xstrcmp(crypt(param2 + 2,
							cfg.cmdpasswd),
							cfg.cmdpasswd) == 0) {
						passok = 1;
					}
				} else if (xstrcmp(param2 + 2,
							cfg.cmdpasswd) == 0) {
					passok = 1;
				}
				param2[2 + passlen] = ' ';
			}

				if (passok) {
					char	*t;
					char	*command;
					char	*params = NULL;

					t = strtok(param2 + 1, " ");
					command = strtok(NULL, " ");
					if (command != NULL) {
						params = strchr(command, '\0') +
							1;
					}
					if (params != NULL) {
						upcase(command);
						*pass = remote_cmd(command,
								params,
								nick, hostname);
						normal = 0;
					}
				}
			}
#endif /* _NEED_CMDPASSWD */

			/* Normal PRIVMSG/NOTICE to client. */
			if (normal) {
				isprivmsg = 1;
#ifdef INBOX
#  ifndef QUICKLOG
/*
 * Note that we do inbox here only is privmsglog is enabled and
 * quicklogging is disabled.
 */
				if (inbox) {
					fprintf(inbox, "%s(%s) %s\n",
							get_short_localtime(),
							origin, param2 + 1);
					fflush(inbox);
				}
#  endif /* QUICKLOG */
#endif /* INBOX */
				
				if (cfg.forwardmsg) {
					timers.forward = 0;
					l = forwardmsg ? strlen(forwardmsg) : 0;
					i = l + strlen(origin) +
						strlen(param2 + 1) + 5;
					forwardmsg = (char *)
						xrealloc(forwardmsg, i);
					sprintf(forwardmsg + l, "(%s) %s\n",
							origin, param2 + 1);
				}
			}
		}
	}
	
	/* Bah, it wasn't for me. */
	else {
		int chw = 0;

		/* channel wallops - notice @#channel etc :) */
		if (param1[0] == '@' || param1[0] == '%'
				|| param1[0] == '+') {
			chw = 1;
			param1++;
		}

#ifdef CHANLOG
		chptr = channel_find(param1, LIST_ACTIVE);
		if (chptr != NULL && HAS_LOG(chptr, LOG_MESSAGE)) {
			if (chw) {
				param1--;
			}
		
			if (cmdindex == CMD_PRIVMSG + MINCOMMANDVALUE) {
				char *t;
				t = log_prepare_entry(nick, param2 + 1);
				if (t == NULL) {
					chanlog_write_entry(chptr, LOGM_MESSAGE,
							get_short_localtime(),
							nick, param2 + 1);
				} else {
					chanlog_write_entry(chptr, "%s", t);
				}
			}
		}
#endif /* CHANLOG */
	}

	xfree(origin);

	return isprivmsg;
} /* int parse_privmsg(char *, char *, char *, char *, const int, int *) */



int
server_read(
	   )
{
	char	*backup = 0;
	char	*origin, *command, *param1, *param2;
	int	rstate;
	int	pass = 0;
	int	commandno;

	rstate = irc_read(&c_server);
	if (rstate > 0) {
		/* new data... go for it ! */
		rstate = 0;
		if (strlen(c_server.buffer) > 0) {
			pass = 1;

			backup = strdup(c_server.buffer);
			
			if (c_server.buffer[0] == ':') {
				/* reply */
				origin = strtok(c_server.buffer + 1, " ");
				command = strtok(NULL, " ");
				param1 = strtok(NULL, " ");
				param2 = strtok(NULL, "\0");
#ifdef DEBUG
#  ifdef OBSOLETE
				printf("[%s] [%s] [%s] [%s]\n",
						origin, command,
						param1, param2);
#  endif /* OBSOLETE */
#endif /* DEBUG */
				if (command) {
					commandno = atoi(command);
					if (commandno == 0) {
						commandno = MINCOMMANDVALUE +
							command_find(command);
					}
					server_reply(commandno,
							backup,
							origin,
							param1,
							param2,
							&pass);
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
		}
	}
	
	return rstate;
} /* int server_read() */




/*
 * Check number of servers and consistency of i_server.currect.
 * 
 * If there are only fallback-server (or no servers at all !?) left,
 * warn the user about this. Also, if the server we're connected to is on the
 * list, set i_server.current to index of it.
 */
void
check_servers(
	     )
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
} /* void check_servers() */



void
server_reply(
		const int	command,
		char		*original,
		char		*origin,
		char		*param1,
		char		*param2,
		int		*pass
	    )
{
	channel_type	*chptr;
	char		*work = NULL;
	char		*nick, *hostname;
	char		*t;
	int		isprivmsg = 0;
	int		n;

	t = strchr(origin, '!');
	if (t != NULL) {
		*t++ = '\0';
		nick = strdup(origin);
		hostname = strdup(t);
	} else {
		nick = strdup(origin);
		hostname = strdup(origin);
	}


	switch (command) {

		/* Replies. */

	
		/* Just signed in to server. */
		case RPL_WELCOME:
			i_server.connected++;

			xfree(i_server.realname);
			i_server.realname = strdup(origin);

			xfree(status.nickname);
			status.nickname = strdup(param1);

			xfree(i_server.greeting[0]);
			i_server.greeting[0] = strdup(param2);

			xfree(status.idhostname);
			t = strchr(param2, '!');
			if (t != NULL) {
				status.idhostname = strdup(t + 1);
				status.goodhostname =
					pos(status.idhostname, '@') + 1;
			} else {
				/* Should we do something about this ? */
				status.idhostname = strdup("miau");
				status.goodhostname = 0;
			}

#ifdef UPTIME
			if (! status.startup) {
				time(&status.startup);
			}
#endif /* UPTIME */

			i_server.greeting[0][lastpos(
					i_server.greeting[0], ' ')] = 0;
			report(IRC_CONNECTED, i_server.realname);

#ifdef ONCONNECT
			onconnect_do();
#endif /* ONCONNECT */

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

			/* Need to join passive_channels ? */
			if (! status.autojoindone && cfg.leave) {
				/* 
				 * passive_channels are now joined. It doesn't
				 * matter if passive_channels is empty or not.
				 */
				status.autojoindone = 1;
				channel_join_list(LIST_PASSIVE, 0, NULL);
			}

			/* ASAP, see if we should join channels. */
			timers.join = JOINTRYINTERVAL;

			for (n = 0; n < RPL_ISUPPORT_LEN; n++) {
				xfree(i_server.isupport[n]);
			}

			break;

		/* Still shaking hands... */
		case RPL_YOURHOST:
		case RPL_SERVERIS:
		case RPL_SERVERVER:
			/* your-host, your-server-is, server-version replies */
			xfree(i_server.greeting[command - 1]);
			i_server.greeting[command - 1] = strdup(param2);
			break;
		
		/* Talking about weather. */
		case RPL_ISUPPORT:
			for (n = 0; n < RPL_ISUPPORT_LEN; n++) {
				if (i_server.isupport[n] == NULL) {
					i_server.isupport[n] = strdup(param2);
					break;
				}
			}
			break;

		/* This server is restricted. */
		case RPL_RESTRICTED:
			if (cfg.jumprestricted) {
				server_drop(CLNT_RESTRICTED);
				report(SERV_RESTRICTED);
				server_next(1);
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
				channel = strdup(param2);
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
				work = strdup(param2);

				t = strchr(work, ' ');
				if (t == NULL) {
#ifdef ENDUSERDEBUG
					enduserdebug("no channel at NAMREPLY");
#endif /* ENDUSERDEBUG */
					break;
				}
				channel = strtok(t + 1, " ");
				chptr = channel_find(channel, LIST_ACTIVE);
#  ifdef AUTOMODE
				if (chptr == NULL || chptr->oper != -1) {
#  else /* AUTOMODE */
				if (chptr == NULL) {
#  endif /* AUTOMODE */
#ifdef ENDUSERDEBUG
					if (chptr == NULL) {
						enduserdebug("NAMREPLY on unjoined channel (%s)", channel);
					}
#endif /* ENDUSERDEBUG */
					break;
				}

				t = strtok(NULL, " ");
#ifdef ENDUSERDEBUG
				if (t[0] != ':') {
					enduserdebug("no users on NAMREPLY");
					break;
				}
#endif /* ENDUSERDEBUG */
				n = 0;
				while (t != NULL) {
					n++;
					t = strtok(NULL, " ");
				}
#ifdef AUTOMODE
				chptr->oper = (n == 1 ? 1 : 0);
#endif /* AUTOMODE */
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
			work = strdup(param2);
			t = strtok(work, " ");
			chptr = channel_find(t, LIST_PASSIVE);
			if (chptr == NULL) {
				break;
			}

			if (chptr->joining == 1) {
				/* Ok, nothing to worry about. */
				chptr->joining = 0;
				*pass = 0;
			}
			break;
				

			
		/* Commands. */

		/* Someone chaning nick. */
		case CMD_NICK + MINCOMMANDVALUE:
			/* Is that us who changed nick ? */
			if (xstrcasecmp(status.nickname, nick) == 0) {
				xfree(status.nickname);
				status.nickname = strdup(param1 + 1);

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
#endif /* CHANLOG */
			break;

		/* Ping ?  Pong. */
		case CMD_PONG + MINCOMMANDVALUE:
			/* We don't need to reset timer - it is done in irc.c */
#ifdef PINGSTAT
			ping_got++;
#endif /* PINGSTAT */
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
#endif /* ENDUSERDEBUG */
				break;
			}

#ifdef CHANLOG
			if (HAS_LOG(chptr, LOG_PART)) {
				t = strchr(param2, ' ');
				/* Ugly, done because we cant break up param2 */
				if (t != NULL) {
					char	*target;

					*t = '\0';
					target = strdup(param2);
					*t = ' ';

					chanlog_write_entry(chptr, LOGM_KICK,
							get_short_localtime(),
							target, nick, 
							nextword(param2) + 1);
					xfree(target);
				}
			}
#endif /* CHANLOG */

			/* Me being kicked ? */
			if (xstrncmp(status.nickname, param2,
						pos(param2, ' ') - 1) == 0) {
				report(CLNT_KICK, origin, param1,
						nextword(param2) + 1);
				channel_rem(chptr, LIST_ACTIVE);
			}
			break;

		/* Someone joining. */
		case CMD_JOIN + MINCOMMANDVALUE:
			n = (param1[0] == ':' ? 1 : 0);
			/* Was that me ? */
			if (xstrcasecmp(status.nickname, nick) == 0) {
				/* Add channel to active list. */
				channel_add(param1 + n, LIST_ACTIVE);
			}

			/* Get pointer to this channel. */
			chptr = channel_find(param1 + n, LIST_ACTIVE);
			if (chptr == NULL) {
#ifdef ENDUSERDEBUG
				enduserdebug("JOIN on channel we're not on");
				enduserdebug("command = %d / param1 + n = '%s'"
						" / param2 = '%s'",
						command, param1 + n, param2);
#endif /* ENDUSERDEBUG */
				break;
			}

#ifdef AUTOMODE
			/* Don't care if it's me joining. */
			if (xstrcasecmp(nick, status.nickname) != 0) {
				automode_queue(nick, hostname, chptr);
			}
#endif /* AUTOMODE */
	
#ifdef CHANLOG
			if (HAS_LOG(chptr, LOG_JOIN)) {
				chanlog_write_entry(chptr, LOGM_JOIN,
						get_short_localtime(),
						nick, hostname, param1 + 1);
			}
#endif /* CHANLOG */

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
#endif /* AUTOMODE */

			/* Remove channel from list if it was me leaving. */
			if (xstrcmp(nick, status.nickname) == 0) {
				channel_rem(chptr, LIST_ACTIVE);
			}

#ifdef CHANLOG
			if (HAS_LOG(chptr, LOG_PART)) {
				chanlog_write_entry(chptr, LOGM_PART,
						get_short_localtime(),
						nick, param1,
						(param2) ? (param2 + 1) ?
						param2 + 1 : "" : "");
			}
#endif /* CHANLOG */
			break;

		/* Someone's leaving for good. */
		case CMD_QUIT + MINCOMMANDVALUE:
#ifdef AUTOMODE
			automode_drop_nick(nick, '\0');
#endif /* AUTOMODE */

#ifdef CHANLOG
			chanlog_write_entry_all(LOG_QUIT, LOGM_QUIT,
					get_short_localtime(),
					nick, param1 + 1, " ", param2);
#endif /* CHANLOG */

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
				status.idhostname = strdup(hostname);
				status.goodhostname = pos(hostname, '@') + 1;
			}
			chptr = channel_find(param1, LIST_ACTIVE);
			if (chptr == NULL) {
#ifdef ENDUSERDEBUG
				if (xstrcasecmp(param1, status.nickname) != 0) {
					enduserdebug("MODE on unknown channel");
				}
#endif /* ENDUSERDEBUG */
				break;
			}

			parse_modes(param1, param2);
			
#ifdef CHANLOG
			if (HAS_LOG(chptr, LOG_MODE)) {
				chanlog_write_entry(chptr, LOGM_MODE,
						get_short_localtime(),
						nick, param2);
			}
#endif /* CHANLOG */

			break;

		/* Someone changing topic. */
		case CMD_TOPIC + MINCOMMANDVALUE:
			/* :<source> TOPIC <channel> :<topic> */

			chptr = channel_find(param1, LIST_ACTIVE);
			if (chptr == NULL) {
#ifdef ENDUSERDEBUG
				enduserdebug("TOPIC on channel we're not on");
#endif /* ENDUSERDEBUG */
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
				sprintf(timebuf, "%d", (int) now.tv_sec
						- tz.tz_minuteswest * 60);
				channel_when(chptr, origin, timebuf);
			}

#ifdef CHANLOG
			if (HAS_LOG(chptr, LOG_MISC)) {
				chanlog_write_entry(chptr, LOGM_TOPIC,
						get_short_localtime(), nick,
						(param2 + 1) ? param2 + 1 : "");
			}
#endif /* CHANLOG */
			
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
#endif /* OBSOLETE */
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
#endif /* QUICKLOG */

	xfree(work);
	xfree(nick);
	xfree(hostname);
} /* void server_reply(const int, char *, char *, char *, char *, int *) */



void
parse_modes(
		const char	*channel,
		const char	*original
	   )
{
	channel_type	*chptr = channel_find(channel, LIST_ACTIVE);
	char		*buf;
	char		*ptr;
	char		*param;
	char		modetype = '+';

	if (chptr == NULL) {
#ifdef ENDUSERDEBUG
		enduserdebug("MODE on channel we're not on");
#endif /* ENDUSERDEBUG */
		return;
	}

	buf = strdup(original);
	
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
			case 'o':	/* Mode queue -modes. */
			case 'v':
#ifdef AUTOMODE
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
#endif /* AUTOMODE */
				param = strtok(NULL, " ");
				break;

			case 'k':	/* Channel key. */
				if (modetype == '+') {	/* Setting. */
					xfree(chptr->key);
					chptr->key = strdup(param);
				} /* No need to clear unset key. */
				break;
				param = strtok(NULL, " ");

			case 'l':	/* Limit. */
				/*
				 * It's not like we would care, but we need
				 * to jump to next parameter.
				 */
				param = strtok(NULL, " ");
				break;
		}

		ptr++;
	}

	xfree(buf);
} /* void parse_modes(const char *, const char *) */

