/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2003-2006 Tommi Saviranta <wnd@iki.fi>
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

#include "client.h"
#include "conntype.h"
#include "error.h"
#include "messages.h"
#include "miau.h"
#include "irc.h"
#include "qlog.h"
#include "tools.h"
#include "channels.h"
#include "dcc.h"
#include "privlog.h"
#include "commands.h"
#include "chanlog.h"
#include "log.h"
#include "common.h"

#include <string.h>



client_info	i_client;
clientlist_type	c_clients;


void client_drop_real(connection_type *client, char *reason,
		const int echo, const int dying);



/*
 * "work" must be global so when miau_command is called, "work" can be freed
 * if users want to quit miau.
 */
static char *work = NULL;	/* Temporary copy of input. */



/*
 * Drop client from bouncer.
 *
 * If client == NULL, drop all clients.
 */
void
client_drop(connection_type *client, char *reason, const int msgtype,
		const int echo, const char *awaymsg)
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

	if (c_clients.connected == 0) {
		clients_left(cfg.usequitmsg ? awaymsg : NULL);
	} else if (cfg.autoaway == 1) {
		if (cfg.usequitmsg == 1 && awaymsg != NULL){
			set_away(awaymsg);
		} else {
			set_away(cfg.awaymsg);
		}
	}
} /* void client_drop(connection_type *, char *, const int, const int,
	const char *) */



/*
 * Drop client from bouncer.
 */
void
client_drop_real(connection_type *client, char *reason, const int echo,
		const int dying)
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



static void
cmd_part(char *par0, char *par1)
{
	/* User wants to leave list of channels. */
	channel_type *chan;
	char *name;

	name = strtok(par0, ":");	/* Set EOS for channels. */
	name = strtok(par0, ",");
	while (name != NULL) {
		chan = channel_find(name, LIST_PASSIVE);
		if (chan == NULL) {
			irc_mwrite(&c_clients, ":miau %d %s %s :%s",
					ERR_NOSUCHCHANNEL,
					status.nickname,
					name,
					IRC_NOSUCHCHAN);
		} else {
			channel_rem(chan, LIST_PASSIVE);
		}
		name = strtok(NULL, ",");
	}
} /* static void cmd_part(char *par0, char *par1) */



static void
cmd_join(char *par0, char *par1)
{
	/* user wants to join a list of channels */
	char *chan;	/* channel name */
	char *key;	/* channel key */
	char *chanseek;
	char *keyseek;
	
	if (par0 != NULL && xstrcmp(par0, "0") == 0) {
		/* user want to leave all channels */
		LLIST_WALK_H(passive_channels.head, channel_type *);
			channel_rem(data, LIST_PASSIVE);
		LLIST_WALK_F;

		return;
	}

	chan = par0;
	key = par1;
	keyseek = NULL;

	while (chan != NULL && *chan != '\0') {
		chanseek = strchr(chan, ',');
		if (chanseek != NULL) {
			*chanseek = '\0';
			chanseek++;
		}
		if (key != NULL && *key != '\0') {
			keyseek = strchr(key, ',');
			if (keyseek != NULL) {
				*keyseek = '\0';
				keyseek++;
			}
		}
		channel_add(chan, key, LIST_PASSIVE);
		chan = chanseek;
		key = keyseek;
	}
} /* static void cmd_join(char *par0, char *par1) */



#ifdef DCCBOUNCE
static int
cmd_privmsg_ctcp(char *par0, char *par1)
{
	char dcct[IRC_MSGLEN];
	char *ret;

	if (cfg.dccbounce == 0) {
		return 1;
	}

	upcase(par1);
	if (xstrncmp(par1, ":\1DCC", 5) != 0) {
		return 1;
	}

	strncpy(dcct, par1 + 1, IRC_MSGLEN);
	ret = dcc_initiate(dcct, IRC_MSGLEN, 1);

	if (ret != NULL) {
		irc_write(&c_server, "PRIVMSG %s %s", par0, dcct);
		return 0;
	} else {
		return 1;
	}
} /* static int cmd_privmsg_dcc(char *par0, char *par1) */
#endif /* ifdef DCCBOUNCE */



#ifdef NEED_LOGGING
static int
cmd_privmsg(char *par0, char *par1)
{
	int is_chan;
	if (par0 == NULL) {
		return 1;
	}

	is_chan = channel_is_name(par0);

#ifdef PRIVLOG
	if (is_chan == 0) {
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
			privlog_write(par0, PRIVLOG_OUT, 
					CMD_PRIVMSG + MINCOMMANDVALUE,
					par1 + 1);
		}
	}
#endif /* PRIVLOG */

#ifdef CHANLOG
	if (is_chan != 0) {
		channel_type *chan;
		chan = channel_find(par0, LIST_ACTIVE);
		if (chan != NULL && chanlog_has_log(chan, LOG_MESSAGE)) {
			char *t;
			t = log_prepare_entry(status.nickname, par1 + 1);
			if (t == NULL) {
				chanlog_write_entry(chan, LOGM_MESSAGE,
						get_short_localtime(),
						status.nickname, par1 + 1);
			} else {
				chanlog_write_entry(chan, "%s", t);
			}
		}
	}
#endif /* CHANLOG */

	return 1;
} /* static int cmd_privmsg(char *par0, char *par1) */
#endif /* ifdef NEED_LOGGING */



static int
cmd_away(const char *orig, char *par0, char *par1)
{
	if (par0 == NULL) {
		FREE(status.awaymsg);
		/* Not away / no custom message. */
		status.awaystate &= ~AWAY & ~CUSTOM;
	} else {
		/* This should be safe by now. */
		char *t = strchr(orig, (int) ' ') + 2;
#ifdef EMPTYAWAY
		xfree(status.awaymsg);
		status.awaymsg = xstrdup(t);
		/* Away / custom message. */
		status.awaystate |= AWAY | CUSTOM;
#else /* EMPTYAWAY */
		FREE(status.awaymsg);
		if (*t != '\0') {
			status.awaymsg = xstrdup(t);
			/* Away / custom message. */
			status.awaystate |= AWAY | CUSTOM;
		} else {
			/* Not away / no custom message. */
			status.awaystate &= ~AWAY & ~CUSTOM;
		}
#endif /* EMPTYAWAY */
	}

	return 1;
} /* static int cmd_away(char *par0, char *par1) */



static void
pass_cmd(connection_type *client, char *cmd, char *par0, char *par1,
		const char *bufbu)
{
	const char *buf;
	/*
	 * Use buffer from client unless bufbu is set. Having bufbu set
	 * means client is invalid and should not be used.
	 */
	if (bufbu != NULL) {
		buf = bufbu;
	} else {
		buf = client->buffer;
	}

	if (i_server.connected == 2) {
		int msg;

		/* Pass the message to server. */
		irc_write(&c_server, "%s", buf);

		msg = ((xstrcmp(cmd, "PRIVMSG") == 0) ||
			(xstrcmp(cmd, "NOTICE") == 0)) ? 1 : 0;
		/* Echo the meesage to other clients. */
		if (msg == 1) {
			llist_node *iter;
			for (iter = c_clients.clients->head; iter != NULL;
					iter = iter->next) {
				if (iter->data == client) {
					continue;
				}

				irc_write((connection_type *) iter->data,
						":%s!%s %s",
						status.nickname,
						status.idhostname,
						buf);
			}
		}

#ifdef QUICKLOG
		/* Also put it in quicklog. If necessary, or course. */
		if (cfg.flushqlog == 0 && par0 != NULL && msg == 1) {
			qlog_write(0, ":%s!%s@%s %s",
					status.nickname,
					i_client.username,
					i_client.hostname,
					buf);
		}
#endif /* QUICKLOG */
	} else {
		irc_notice(client, status.nickname, CLNT_NOTCONNECT);
	}
} /* static void pass_cmd(connection_type *client, char *cmd,
		char *par0, char *par1, const char *bufbu) */



int
client_read(connection_type *client)
{
	int c_status;
	char *command, *param1, *param2;
	char *orig;
	int pass;

	c_status = irc_read(client);

	if (c_status <= 0) {
		/* Something went unexpected. */
		return c_status;
	}

	/* Ok, got something. */
	if (client->buffer[0] == '\0') {
		/* Darn, got nothing after all. */
		return 0;
	}

	work = xstrdup(client->buffer);
	orig = NULL;

	command = strtok(work, " ");

	if (command == NULL) {
		xfree(work);
		work = NULL;
		return 0;
	}

	param1 = strtok(NULL, " ");
	param2 = strtok(NULL, "\0");
	if (param1 != NULL && *param1 == ':') {
		param1++;
	}

	upcase(command);

	/* Commands from client when not connected to IRC-server. */
	if (i_server.connected != 2) {
		/* Willing to leave channels. */
		if (xstrcmp(command, "PART") == 0) {
			cmd_part(param1, param2);
		}
		/* Willing to join or leave channels. */
		else if (xstrcmp(command, "JOIN") == 0) {
			cmd_join(param1, param2);
		}
		pass = 0;
	}

	else if (xstrcmp(command, "PRIVMSG") == 0) {
		pass = 1; /* default */
		if (param2 == NULL) {
#ifdef ENDUSERDEBUG
			enduserdebug("client_read(): PRIVMSG, param2 = NULL");
#endif /* ifdef ENDUSERDEBUG */
		}
#ifdef DCCBOUNCE
		else if (param2[0] == ':' && param2[1] == '\1') {
			pass = cmd_privmsg_ctcp(param1, param2);
		}
#endif /* ifdef DCCBOUNCE */
#ifdef NEED_LOGGING
		else {
			pass = cmd_privmsg(param1, param2);
		}
#endif /* ifdef NEED_LOGGING */
	}

	else if (xstrcmp(command, "PONG") == 0) {
		pass = 0;	/* Munch munch munch. */
	}

	else if (xstrcmp(command, "PING") == 0) {
		irc_write(client, ":%s PONG %s :%s",
				i_server.realname,
				param2 != NULL ? param2 : i_server.realname,
				param1 != NULL ? param1 : status.nickname);
		pass = 0;
	}

	else if (xstrcmp(command, "MIAU") == 0) {
		miau_commands(param1, param2, client);
		pass = 0;
	}

	else if (xstrcmp(command, "QUIT") == 0) {
		const char *reason;

		orig = xstrdup(client->buffer);

		if (param1 != NULL) {
			reason = orig + (param1 - work);
		} else {
			reason = NULL;
		}
		/* (single client, message, report, echo) */
		client_drop(client, CLNT_LEFT, REPORT, 0, reason);
		pass = 0;
	}

	else if (xstrcmp(command, "AWAY") == 0) {
		pass = cmd_away(client->buffer, param1, param2);
	}

	else {
		pass = 1;
	}

	if (pass == 1) {
		/* orig is set only if client was detached */
		if (orig != NULL) {
			pass_cmd(client, command, param1, param2, orig);
		} else {
			pass_cmd(client, command, param1, param2,
					client->buffer);
		}
	}
	xfree(orig);
	xfree(work);
	work = NULL;

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
client_free(void)
{
	if (work != NULL) {
		xfree(work);
		work = NULL;
	}
} /* void_client_free(void) */
