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

#include "client.h"

#include "conntype.h"
#include "irc.h"
#include "tools.h"
#include "log.h"
#include "privlog.h"
#include "chanlog.h"
#include "dcc.h"



client_info	i_client;
clientlist_type	c_clients;


void client_drop_real(connection_type *, char *, const int, const int);



/*
 * "work" must be global so when miau_command is called, "work" can be freed
 * if users want to quit miau.
 */
char	*work = NULL;	/* Temporary copy of input. */



/*
 * Drop client from bouncer.
 *
 * If client == NULL, drop all clients.
 */
void
client_drop(
		connection_type	*client,
		char		*reason,
		const int	msgtype,
		const int	echo
	   )
{
	if (client == NULL) {
		/* Drop all clients. */
		while (c_clients.clients->head != NULL) {
			client_drop_real((connection_type *)
					c_clients.clients->head->data, reason,
					echo, msgtype == DYING);
		}
	} else {
		client_drop_real(client, reason, echo, msgtype == DYING);
	}

	if (msgtype == ERROR) {
		error("%s", reason);
	} else {
		report("%s%s", msgtype == DYING ? CLNT_DIE : "", reason);
	}
	/* Reporting number of clients is irrevelant if only one is allowed. */
	if (cfg.maxclients != 1) {
		report(CLNT_CLIENTS, c_clients.connected);
	}
} /* void client_drop(connection_type *, char *, const int, const int) */



/*
 * Drop client from bouncer.
 */
void
client_drop_real(
		connection_type	*client,
		char		*reason,
		const int	echo,
		const int	dying
		)
{
	llist_node	*node;
	/* We could skip testing reason, but lets play safe. */
	if (echo && reason != NULL) {
		/* Set back to blocking. */
		sock_setblock(c_server.socket);
		if (reason[0] == ':') {
			irc_write(client, "ERROR %s", reason);
		} else {
			irc_write(client, dying
					? MIAU_USERKILLED : MIAU_CLOSINGLINK,
					reason);
		}
	}
	sock_close(client);
	node = llist_find((void *) client, c_clients.clients);
	xfree(node->data);
	llist_delete(node, c_clients.clients);
	c_clients.connected--;
} /* void client_drop_real(connection_type *, char *, const int, const int) */



int
client_read(
		connection_type	*client
	   )
{
	int	c_status;
	char	*command, *param1, *param2;
	int	pass = 1;
	
	c_status = irc_read(client);
	
	if (c_status <= 0) {
		/* Something went unexpected. */
		return c_status;
	}
	
	/* Ok, got something. */
	if (strlen(client->buffer) == 0) {
		/* Darn, got nothing after all. */
		return 0;
	}
	
	work = strdup(client->buffer);

	command = strtok(work, " ");
	param1 = strtok(NULL, " ");
	param2 = strtok(NULL, "\0");
	if (param1 && param1[0] == ':') {
		param1++;
	}

	if (command) {
		upcase(command);

		/* Commands from client when not connected to IRC-server. */
		if (i_server.connected != 2) {
			/* Willing to leave channels. */
			if (xstrcmp(command, "PART") == 0) {
	/* User wants to leave list of channels. */
	channel_type	*chptr;
	char		*channel;

	channel = strtok(param1, ":");	/* Set EOS for channels. */
	channel = strtok(param1, ",");
	while (channel != NULL) {
		chptr = channel_find(channel, LIST_PASSIVE);
		if (chptr == NULL) {
			irc_mwrite(&c_clients, ":miau %d %s %s :"
					"No such channel",
					ERR_NOSUCHCHANNEL,
					status.nickname,
					channel);
		} else {
			channel_rem(chptr, LIST_PASSIVE);
		}
		channel = strtok(NULL, ",");
	}
	pass = 0;
			}
			
			/* Willing to join or leave channels. */
			else if (xstrcmp(command, "JOIN") == 0) {
				/* Want to part all channels ? */
				if (param1 != NULL && param1[0] == '0' &&
						(param1[1] == ' ' ||
							param1[1] == '\0')) {
	/* User want to leave all channels. */
	LLIST_WALK_H(passive_channels.head, channel_type *);
		channel_rem(data, LIST_PASSIVE);
	LLIST_WALK_F;
				} else {
	/* User wants to join list of channels. */
	channel_type	*chptr;
	char		*chan;		/* Channels. */
	char		*key;		/* Keys. */
	char		*chanhelp;
	char		*keyhelp = NULL;
	
	chan = param1;
	key = param2;
	while (chan != NULL) {
		chanhelp = strchr(chan, ',');
		if (chanhelp != NULL) {
			*chanhelp = '\0';
			chanhelp++;
		}
		if (key != NULL) {
			keyhelp = strchr(key, ',');
			if (keyhelp != NULL) {
				*keyhelp = '\0';
				keyhelp++;
			}
		}
		chptr = channel_add(chan, LIST_PASSIVE);
		if (chptr != NULL) {
			if (key != NULL) {
				xfree(chptr->key);
				chptr->key = strdup(key);
			}
		}

		chan = chanhelp;
		key = keyhelp;
	}
				}
				pass = 0;
			} /* JOIN */
		} /* if (i_server.connected != 2) */
			

		if (xstrcmp(command, "PRIVMSG") == 0) {		/* PRIVMSG */
#ifdef DCCBOUNCE
			if (cfg.dccbounce && xstrncmp(param2,
						":\1DCC", 5) == 0) {
				if (dcc_initiate(param2 + 1, 1)) {
					irc_write(&c_server, "PRIVMSG %s %s",
							param1, param2);
					pass = 0;
				}
#ifdef LOGGING
			} else {
#endif /* LOGGING */
#else /* DCCBOUNCE */
#ifdef LOGGING
			if (xstrncmp(param2, ":\1DCC", 5) != 0) {
#endif /* LOGGING */
#endif /* DCCBOUNCE */

#ifdef LOGGING
#ifdef PRIVLOG
	if (param1[0] != '#') {
		/*
		 * We could say
		 * 
		if ((c_clients.connected > 0 && (cfg.privlog & 0x02))
				|| (c_clients.connected == 0
					&& cfg.privlog == PRIVLOG_DETACHED)) {
		 *
		 * ...but do disconnected clients really talk?-)
		 *
		 * Therefore we say the following instead:
		 */
		if (cfg.privlog != 0) {
			privlog_write(param1, PRIVLOG_OUT, param2 + 1);
		}
	}
#endif /* PRIVLOG */

#ifdef CHANLOG
				if (param1[0] == '#') {
					channel_type *chptr;
					chptr = channel_find(param1,
							LIST_ACTIVE);
					if (chptr != NULL && HAS_LOG(chptr,
								LOG_MESSAGE)) {
	char *t;
	t = log_prepare_entry(status.nickname, param2 + 1);
	if (t == NULL) {
		chanlog_write_entry(chptr, LOGM_MESSAGE, get_short_localtime(),
				status.nickname, param2 + 1);
	} else {
		chanlog_write_entry(chptr, "%s", t);
	}
					}
				}
#endif /* CHANLOG */
			}
#endif /* LOGGING */
		}

		else if (xstrcmp(command, "PONG") == 0) {	/* PONG */
			pass = 0;	/* Munch munch munch. */
		}
				
		else if (xstrcmp(command, "PING") == 0) {	/* PING */
			irc_write(client, ":%s PONG %s :%s",
					i_server.realname,
					i_server.realname,
					status.nickname);
			pass = 0;
		}

		
		else if (xstrcmp(command, "MIAU") == 0) {	/* MIAU */
			miau_commands(param1, param2, client);
			pass = 0;
		}
				
		else if (xstrcmp(command, "QUIT") == 0) {	/* QUIT */
			char *reason = NULL;
			int len = (param1 != NULL) ? param1 - work : 0;
			if (len > 0) {
				/*
				 * If we got quit-message, backup before
				 * "client" is freed.
				 */
				reason = strdup(client->buffer + len);
			}
			/* (single clients, message, report, echo) */
			client_drop(client, CLNT_LEFT, REPORT, 0);
			
			/* If there are no more clients connected... */
			if (c_clients.connected == 0) {
				clients_left(cfg.usequitmsg ? reason : NULL);
			} else if (cfg.usequitmsg == 1) {
				set_away(reason);
			}
			xfree(reason);
			pass = 0;
		}

		else if (xstrcmp(command, "AWAY") == 0) {	/* AWAY */
			if (param1 == NULL) {
				FREE(status.awaymsg);
				/* Not away / no custom message. */
				status.awaystate &= ~AWAY & ~CUSTOM;
			} else {
				/* This should be safe by now. */
				char *t = strchr(client->buffer, (int) ' ') + 2;
#ifdef EMPTYAWAY
				xfree(status.awaymsg);
				status.awaymsg = strdup(t);
				/* Away / custom message. */
				status.awaystate |= AWAY | CUSTOM;
#else /* EMPTYAWAY */
				FREE(status.awaymsg);
				if (*t != '\0') {
					status.awaymsg = strdup(t);
					/* Away / custom message. */
					status.awaystate |= AWAY | CUSTOM;
				} else {
					/* Not away / no custom message. */
					status.awaystate &= ~AWAY & ~CUSTOM;
				}
#endif /* EMPTYAWAY */
			}
			pass = 1;
		}
	}
		
	if (pass) {
		if (i_server.connected == 2) {
			int	log = xstrcmp(command, "PRIVMSG") == 0 ||
				xstrcmp(command, "NOTICE") == 0;
			/* Pass the message to server. */
			irc_write(&c_server, "%s", client->buffer);
			/* Echo the meesage to other clients. */
			if (log) {
				llist_node	*client_this;
				for (client_this = c_clients.clients->head;
						client_this != NULL;
						client_this = client_this->next)
				{
					if (client_this->data != client) {
						/* Bad nesting. */
						irc_write((connection_type *) client_this->data,
							":%s!%s@%s %s",
							status.nickname,
							i_client.username,
							i_client.hostname,
							client->buffer);
					}
				}
			}
#ifdef QUICKLOG
			/* Also put it in quicklog. If necessary, or course. */
			if (cfg.flushqlog == 0 && param1 != NULL && log) {
				qlog_write(0, ":%s!%s@%s %s",
						status.nickname,
						i_client.username,
						i_client.hostname,
						client->buffer);
			}
#endif /* QUICKLOG */
		} else {
			irc_notice(client, status.nickname, CLNT_NOTCONNECT);
		}
	}
	client_free();
	
	/* All ok. */
	return 0;
} /* int client_read(connection_type *) */



/*
 * Free copy of user's command.
 *
 * This function is a bit ugly and could be considered useless. When
 * client_read() is called, a copy of user's command is made, and call to
 * miau_command() may be made. If command was "DIE", this memory would leave
 * unfreed before quitting miau. So, this function is needed so that miau.c
 * can ask this memory to be freed without returning to client_read().
 */
void
client_free(
	   )
{
	xfree(work);
} /* void_client_free() */
