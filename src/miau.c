/*
 * -------------------------------------------------------
 * Copyright (C) 2002-2004 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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

/* #define DEBUG */

#include <config.h>
#include "miau.h"
#include "irc.h"
#include "tools.h"
#include "table.h"
#include "messages.h"
#include "ignore.h"
#include "channels.h"
#include "perm.h"
#include "ascii.h"
#include "commands.h"
#include "parser.h"
#include "chanlog.h"
#include "server.h"
#include "client.h"
#ifdef DCCBOUNCE
#  include "dcc.h"
#endif /* DCCBOUNCE */
#ifdef QUICKLOG
#  include "qlog.h"
#endif /* QUICKLOG */
#ifdef AUTOMODE
#  include "automode.h"
#endif /* AUTOMODE */
#ifdef ONCONNECT
#  include "onconnect.h"
#endif /* ONCONNECT */
#ifdef PRIVLOG
#  include "privlog.h"
#endif /* PRIVLOG */
#ifdef _NEED_CMDPASSWD
#  include "remote.h"
#endif /* _NEED_CMDPASSWD */



#ifdef INBOX
FILE	*inbox = NULL;
#endif /* INBOX */


int read_newclient();
int check_config();

void fakeconnect(connection_type *newclient);
void sig_term();
void create_listen();
void rehash();
void run();
void pre_init();
void init();
void connect_timeout();


void setup_home(char *s);

#ifdef DUMPSTATUS
void dump_status();
#endif /* DUMPSTATUS */


status_type 		status;
cfg_type cfg = {
#ifdef QUICKLOG
	30,	/* qloglength: 30 minutes */
#  ifdef QLOGSTAMP
	0,	/* timestamp: no timestamp */
#  endif /* QLOGSTAMP */
	1,	/* flushqlog: flush */
#endif /* QUICKLOG */
#ifdef DCCBOUNCE
	0,	/* dccbounce: no */
#endif /* DCCBOUNCE */
#ifdef AUTOMODE
	30,	/* automodedelay: 30 seconds */
#endif /* AUTOMODE */
#ifdef INBOX
	1,
#endif /* INBOX */
	0,	/* listenport: 0 */
	1,	/* getnick: disconnected */
	60,	/* getnickinterval: 60 seconds */
	0,	/* antiidle: no */
	1,	/* nevergiveup: yes, never give up */
	0,	/* jumprestricted: no */
	90,	/* stonedtimeout: 90 seconds */
	1,	/* rejoin: yes */
	30,	/* connecttimeout: 30 seconds */
	10,	/* reconnectdelay: 10 seconds */
	0,	/* leave: no */
	9,	/* maxnicklen: 9 chars */
	3,	/* maxclients: 3 clients */
	1,	/* usequitmsg: yes */
#ifdef PRIVLOG
	0,	/* privlog: no privlog */
#endif /* PRIVLOG */

	DEFAULT_NICKFILL,	/* nickfillchar */
	
#ifdef LOGGING
	NULL,			/* logpostfix */
#endif /* LOGGING */
#ifdef DCCBOUNCE
	NULL,			/* dccbindhost */
#endif /* DCCBOUNCE */
#ifdef _NEED_CMDPASSWD
	NULL,			/* cmdpasswd */
#endif /* _NEED_CMDPASSWD */
	NULL, NULL, NULL,	/* username / realname / password */
	NULL, NULL, NULL,	/* leavemsg / bind / listenhost */
	NULL, NULL, NULL,	/* awaymsg / forwardmsg / channels */
	NULL, NULL		/* home / usermode */
};
nicknames_type		nicknames;

permlist_type		connhostlist;
permlist_type		ignorelist;
timer_type		timers;
#ifdef AUTOMODE
extern permlist_type	automodelist;
extern llist_list	*tobeautomode;
#endif /* AUTOMODE */

extern llist_list	active_channels;
extern llist_list	passive_channels;


client_info	i_newclient;
connection_type	c_newclient;


int		listensocket = 0;  	/* listensocket */
char		*forwardmsg = NULL;



#ifdef PINGSTAT
int	ping_sent = 0;
int	ping_got = 0;
#endif



/*
 * Free some resources.
 * Only free those that are _not_ must-haves (except nicknames).
 *
 * Should be called from escape() and from rehash().
 */
void
free_resources(
	      )
{
	LLIST_EMPTY(nicknames.nicks.head, &nicknames.nicks);
	empty_perm(&ignorelist);
	empty_perm(&connhostlist);
#ifdef AUTOMODE
	empty_perm(&automodelist);
#endif /* AUTOMODE */
#ifdef ONCONNECT
	LLIST_EMPTY(onconnect_actions.head, &onconnect_actions);
#endif /* ONCONNECT */
	
	FREE(cfg.username);
	FREE(cfg.realname);
	FREE(cfg.password);

	FREE(cfg.usermode);
	FREE(cfg.awaymsg);
	FREE(cfg.leavemsg);
	FREE(cfg.listenhost);
	FREE(cfg.channels);
	FREE(cfg.forwardmsg);
	FREE(cfg.channels);
#ifdef DCCBOUNCE
	FREE(cfg.dccbindhost);
#endif /* DCCBOUNCE */
#ifdef CHANLOG
	chanlog_del_rules();
#endif /* CHANLOG */

#ifdef QUICKLOG
	/* Replay quicklog - no output and don't keep the logs. */
	qlog_replay(NULL, 0);
#endif /* QUICKLOG */
	xfree(status.awaymsg);
} /* void free_resources() */



/*
 * Finish things up and quit miau.
 */
void
escape(
      )
{
	int		n;

	/* Clear send queue. */
	irc_clear_queue();

#ifdef PRIVLOG
	privlog_close_all();
#endif /* PRIVLOG */

	/* Close connections and free client list. */
	rawsock_close(listensocket);
	sock_close(&c_server);
	sock_close(&c_newclient);

	LLIST_WALK_H(c_clients.clients->head, connection_type *);
		sock_close(data);
		llist_delete(node, c_clients.clients);
	LLIST_WALK_F;
	c_clients.connected = 0;

	/* Close log-file and remove PID-file. */
#ifdef INBOX
	if (inbox != NULL) { fclose(inbox); }
#endif /* INBOX */
	unlink(FILE_PID);

	/* Free permission lists. */
	empty_perm(&connhostlist);
#ifdef AUTOMODE
	empty_perm(&automodelist);
#endif /* AUTOMODE */

	/* Free server-info. */
	xfree(i_server.realname);
	for (n = 0; n < RPL_ISUPPORT_LEN; n++) { xfree(i_server.isupport[n]); }
	for (n = 0; n < RPL_SERVERVER_LEN; n++) { xfree(i_server.greeting[n]); }

	/* Free configuration parameters. */
	xfree(cfg.home);
#ifdef LOGGING
	xfree(cfg.logpostfix);
#endif /* LOGGING */
#ifdef _NEED_CMDPASSWD
	xfree(cfg.cmdpasswd);
#endif /* _NEED_CMDPASSWD */

	/* Free linked lists. */
	LLIST_WALK_H(servers.servers.head, server_type *);
		xfree(data->name);
		xfree(data->password);
		xfree(data);
		llist_delete(node, &servers.servers);
	LLIST_WALK_F;
	xfree(c_clients.clients);
	LLIST_WALK_H(active_channels.head, channel_type *);
		channel_rem(data, LIST_ACTIVE);
	LLIST_WALK_F;
	LLIST_WALK_H(passive_channels.head, channel_type *);
		channel_rem(data, LIST_PASSIVE);
	LLIST_WALK_F;

	/* Free status data. */
	xfree(status.nickname);
	xfree(status.idhostname);

	/* Free client data. */
	xfree(i_client.nickname);
	xfree(i_client.username);
	xfree(i_client.hostname);
	xfree(i_newclient.nickname);
	xfree(i_newclient.username);
	xfree(i_newclient.hostname);

	/* And some more... */
	free_resources();
	command_free();
	
	/* We're done. */
	error(MIAU_ERREXIT);
	exit(1);
} /* void escape() */



/*
 * (Re)read configuration file.
 */
void
read_cfg(
	)
{
	int	ret;
	
	/* Set current server to point the fallback server. */
	i_server.current = servers.servers.head;

	/* Remove servers - all except the first one. */
	LLIST_WALK_H(servers.servers.head->next, server_type *);
		xfree(data->name);
		xfree(data->password);
		xfree(node->data);
		llist_delete(node, &servers.servers);
	LLIST_WALK_F;
	servers.amount = 1;

#ifdef LOGGING
	/*
	 * cfg.logpostfix _must_ be non-null as it's assumed so everywhere.
	 * By assuming it's non-NULL, we don't need to do
	 * 'cfg.logpostfix != NULL ? cfg.logpostfix : ""' every time.
	 */
	cfg.logpostfix = strdup("");	/* No global logfile-postfix. */
#endif /* LOGGING */
	
	/* Read configuration file. */
	ret = parse_cfg(MIAURC);
	if (ret == -1) {
		error(MIAU_ERRCFG, cfg.home);
		escape();
	}

	report(MIAU_READ_RC);
} /* void read_cfg() */



void
sig_term(
	)
{
	error(MIAU_SIGTERM);
	escape();
} /* void sig_term() */



#ifdef DUMPSTATUS
char	*dumpdata;
int	foocount = 0;

void
dump_add(
		char	*data
	)
{
	int addlen = strlen(data);
	dumpdata = xrealloc(dumpdata, strlen(dumpdata) + addlen + 1);
	strncat(dumpdata, data, addlen);
	foocount += addlen;
} /* void dump_add(char *) */

void
dump_dump(
	 )
{
	if (dumpdata[0] != '\0') {
		fprintf(stderr, "%s\n", dumpdata);
		irc_mnotice(&c_clients, status.nickname, dumpdata);
		dumpdata[0] = '\0';
		foocount = 0;
	}
} /* void dump_dump() */

void
dump_finish(
	   )
{
	if (dumpdata[0] != '\0') {
		dump_dump();
	}
} /* void dump_finish() */

void
dump_status_int(
		const char	*id,
		const int	val
	       )
{
	char	buf[BUFFERSIZE];
	sprintf(buf, "    %s=%d", id, val);
	if (foocount + strlen(buf) > 80) {
		dump_dump();
	}
	dump_add(buf);
} /* void dump_status_int(const char, conat int) */

void
dump_status_char(
		const char	*id,
		const char	*val
		)
{
	char	buf[BUFFERSIZE];
	sprintf(buf, "    %s='%s'", id, val);
	if (foocount + strlen(buf) > 80) {
		dump_dump();
	}
	dump_add(buf);
} /* void dump_status_char(const char, const char) */

void
dump_string(
		char	*data
	   )
{
	fprintf(stderr, "%s\n", data);
	irc_mnotice(&c_clients, status.nickname, data);
} /* void dump_string(char *) */

void
dump_status(
	   )
{
	dumpdata = xmalloc(1);
	dumpdata[0] = '\0';
#ifdef QUICKLOG
	/* First check qlog. */
	qlog_check();
#endif /* QUICKLOG */
	dump_string("-- miau status --");
	dump_string("config:");
	dump_add("    nicknames = {");
	LLIST_WALK_H(nicknames.nicks.head, char *);
		dump_status_char("nick", data);
	LLIST_WALK_F;
	dump_add(" }");
	dump_dump();
	dump_status_char("realname", cfg.realname);
	dump_status_char("username", cfg.username);
	dump_status_int("listenport", cfg.listenport);
	dump_status_char("listenhost", cfg.listenhost);
	dump_status_char("bind", cfg.bind);

#ifdef QUICKLOG
	dump_status_int("qloglength", cfg.qloglength);
#  ifdef QLOGSTAMP
	dump_status_int("timestamp", cfg.timestamp);
#  endif /* QLOGSTAMP */
	dump_status_int("flushqlog", cfg.flushqlog);
#endif /* QUICKLOG */
#ifdef DCCBOUNCE
	dump_status_int("dccbounce", cfg.dccbounce);
	dump_status_char("dccbindhost", cfg.dccbindhost);
#endif /* DCCBOUNCE */
#ifdef AUTOMODE
	dump_status_int("automodedelay", cfg.automodedelay);
#endif /* AUTOMODE */
#ifdef INBOX
	dump_status_int("inbox", cfg.inbox);
#endif /* INBOX */
	dump_status_int("getnick", cfg.getnick);
	dump_status_int("getnickinterval", cfg.getnickinterval);
	dump_status_int("antiidle", cfg.antiidle);
	dump_status_int("nevergiveup", cfg.nevergiveup);
	dump_status_int("jumprestricted", cfg.jumprestricted);
	dump_status_int("stonedtimeout", cfg.stonedtimeout);
	dump_status_int("rejoin", cfg.rejoin);
	dump_status_int("connecttimeout", cfg.connecttimeout);
	dump_status_int("reconnectdelay", cfg.reconnectdelay);
	dump_status_int("leave", cfg.leave);
	dump_status_int("maxnicklen", cfg.maxnicklen);
#ifdef LOGGING
	dump_status_char("logpostfix", cfg.logpostfix);
#endif /* LOGGING */
	dump_dump();

	dump_string("connhosts:");
	dump_string(perm_dump(&connhostlist));
	dump_dump();

	dump_string("servers:");
	LLIST_WALK_H(servers.servers.head, server_type *);
		dump_add("    {");
		dump_status_char("host", data->name);
		dump_status_int("port", data->port);
		dump_status_int("working", data->working);
		dump_status_char("pass", data->password);
		dump_status_int("timeout", data->timeout);
		dump_add(" }");
		dump_dump();
	LLIST_WALK_F;

	dump_string("status:");
	dump_status_char("nickname", status.nickname);
	dump_status_char("idhostname", status.idhostname);
	dump_status_int("got_nick", status.got_nick);
	dump_status_int("getting_nick", status.getting_nick);
	dump_status_int("passok", status.passok);
	dump_status_int("init", status.init);
	dump_status_int("supress", status.supress);
	dump_status_int("allowconnect", status.allowconnect);
	dump_status_int("allowreply", status.allowreply);
	dump_status_int("reconnectdelay", status.reconnectdelay);
	dump_status_int("autojoindone", status.autojoindone);
	dump_status_char("away", status.awaymsg);
	dump_status_int("awaystate", status.awaystate);
	dump_status_int("good_server", status.good_server);
	dump_status_int("goodhostname", status.goodhostname);
#ifdef UPTIME
	dump_status_int("startup", status.startup);
#endif
	dump_status_int("c_clients.connected", c_clients.connected);
	dump_dump();

	dump_string("active_channels:");
	LLIST_WALK_H(active_channels.head, channel_type *);
		dump_add("    {");
		dump_status_char("name", data->name);
		dump_status_char("key", data->key);
#ifdef QUICKLOG
		dump_status_int("hasqlog", data->hasqlog);
#endif /* QUICKLOG */
#ifdef AUTOMODE
		dump_status_int("oper", data->oper);
		if (data->mode_queue.head == 0) {
			dump_status_int("mode_queue", 0);
		} else {
			llist_node *ptr;
			automode_type *act;
			dump_dump();
			dump_string("      mode_queue = {");
			for (ptr = data->mode_queue.head; ptr != NULL;
					ptr = ptr->next) {
				act = (automode_type *) ptr->data;
				dump_add("        {");
				dump_status_char("nick", act->nick);
				dump_status_int("mode", (int) act->mode);
				dump_add("  }"); dump_dump();
			}
			dump_string("      }");
		}
#endif /* AUTOMODE */
		dump_add("    }"); dump_dump();
	LLIST_WALK_F;
	dump_dump();
	
	dump_string("passive_channels:");
	LLIST_WALK_H(passive_channels.head, channel_type *);
		dump_add("    {");
		dump_status_char("name", data->name);
		dump_status_char("key", data->key);
#ifdef AUTOMODE
		dump_status_int("oper", data->oper);
#endif /* AUTOMODE */
#ifdef QUICKLOG
		dump_status_int("hasqlog", data->hasqlog);
#endif /* QUICKLOG */
		dump_add("    }"); dump_dump();
	LLIST_WALK_F;
	dump_dump();

	dump_string("old_channels:");
	LLIST_WALK_H(old_channels.head, channel_type *);
		dump_add("    {");
		dump_status_char("name", data->name);
		dump_status_char("key", data->key);
#ifdef AUTOMODE
		dump_status_int("oper", data->oper);
#endif /* AUTOMODE */
#ifdef QUICKLOG
		dump_status_int("hasqlog", data->hasqlog);
#endif /* QUICKLOG */
		dump_add("    }"); dump_dump();
	LLIST_WALK_F;
	dump_dump();

#ifdef PRIVLOG
	dump_string("open privlogs:");
	LLIST_WALK_H(privlog_get_list()->head, privlog_type *);
		dump_add("    {");
		dump_status_char("nick", data->nick);
		dump_status_int("time", data->updated);
		dump_add("  }"); dump_dump();
	LLIST_WALK_F;
#endif /* PRIVLOG */

#ifdef AUTOMODE
	dump_string("automodes:");
	LLIST_WALK_H(automodelist.list.head, automode_type *);
		dump_add("    {");
		dump_status_char("mask", data->nick);
		dump_status_int("true", (int) data->mode);
		dump_add("  }"); dump_dump();
	LLIST_WALK_F;
	dump_dump();
#endif /* AUTOMODE */

	dump_finish();
	xfree(dumpdata);
} /* void dump_status() */
#endif /* DUMPSTATUS */



void
drop_newclient(
		char	*reason
		)
{
	if (i_newclient.connected) {
		if (reason) {
			sock_setblock(c_server.socket);
			irc_write(&c_newclient, MIAU_CLOSINGLINK, reason);
		}
		sock_close(&c_newclient);
		i_newclient.connected = 0;
		status.init = 0;
	}
} /* void drop_newclient(char *) */



/*
 * Move timer.
 *
 * Wrapper for proceed_timer_safe with 0 as last parameter.
 */
int
proceed_timer(
		int		*timer,
		const int	warn,
		const int	exceed
		)
{
	return proceed_timer_safe(timer, warn, exceed, 0);
} /* int proceed_timer(int *, const int, const int) */



/*
 * Move timer.
 *
 * Return 1 if timer > warn, 2 if timer > exceed, otherwise 0.
 */
int
proceed_timer_safe(
		int		*timer,
		const int	warn,
		const int	exceed,
		const int	repeat
		)
{
	(*timer)++;
	/* Exceed */
	if (*timer >= exceed) {
		*timer = 0;
		return 2;
	}
	/* Normal warning */
	if (*timer == warn) {
		return 1;
	}
	/* Repeated warning */
	if (repeat != 0 && *timer > warn && (*timer - warn) % repeat == 0) {
		return 1;
	}
	
	return 0;
} /* int proceed_timer_safe(int *, const int, const int, const int) */



/*
 * Create socket for listening clients.
 *
 * If old socket exists, close it. If something goes wrong, go down, hard.
 */
void
create_listen(
	     )
{
	if (listensocket) {
		rawsock_close(listensocket);
	}

	/* Create listener. */
	listensocket = sock_open();
	if (listensocket == -1) {
		error(SOCK_ERROPEN, net_errstr);
		escape();
	}

	if (! sock_bind(listensocket, cfg.listenhost, cfg.listenport)) {
		if (cfg.listenhost) {
			error(SOCK_ERRBINDHOST, cfg.listenhost,
					cfg.listenport, net_errstr);
		} else {
			error(SOCK_ERRBIND, cfg.listenport, net_errstr);
		}

		escape();
	}
	
	if (! sock_listen(listensocket)) {
		error(SOCK_ERRLISTEN);
		escape();
	}
	
	if (cfg.listenhost) {
		report(SOCK_LISTENOKHOST, cfg.listenhost, cfg.listenport);
	} else {
		report(SOCK_LISTENOK, cfg.listenport);
	}
} /* void create_listen() */



/*
 * Function noting connect() timeouted. Called thru alert().
 */
void
connect_timeout(
	       )
{
	error(SOCK_ERRCONNECT, ((server_type *)
				i_server.current->data)->name, SOCK_ERRTIMEOUT);
	sock_close(&c_server);
} /* void connect_timeout() */



/*
 * Reread configuration file.
 *
 * Clear lists but don't touch settings aleady set. Perhaps this isn't a good
 * idea and we should reset all values to default...
 */
void
rehash(
      )
{
	/*
	 * Save old configuration so we can complain about missing entris and
	 * detect changes (like changing realname) that require further actions.
	 *
	 * We don't need to backup current nick, it's in status.nickname.
	 */
	char		*oldrealname;
	char		*oldusername;
	char		*oldpassword;
	int		oldlistenport;
	char		*oldlistenhost;
	llist_node	*node;

	/* First backup some essential stuff. */
	oldrealname = strdup(cfg.realname);
	oldusername = strdup(cfg.username);
	oldpassword = strdup(cfg.password);
	oldlistenport = cfg.listenport;
	oldlistenhost = (cfg.listenhost != NULL)
		? strdup(cfg.listenhost) : NULL;

	/* Free non-must parameters. */
	free_resources();
	cfg.listenport = 0;
	
#ifdef CHANLOG
	/* Close open logfiles. */
	LLIST_WALK_H(active_channels.head, channel_type *);
		chanlog_close(data);
	LLIST_WALK_F;
	global_logtype = 0;	/* No logging by default. */
#endif /* CHANLOG */
	
	/* Re-read miaurc and check results. */
	read_cfg();
	if (check_config() != 0) {
		irc_mwrite(&c_clients, ":miau NOTICE %s :%s",
				status.nickname,
				CLNT_MIAURCBEENWARNED);
	}
		

	/*
	 * realname, username, password and listenport are required and we want
	 * to revert then back if they're not configured. We need to duplicate
	 * them as oldvalues will be freed later.
	 */
	if (cfg.realname == NULL) { cfg.realname = strdup(oldrealname); }
	if (cfg.username == NULL) { cfg.username = strdup(oldusername); }
	if (cfg.password == NULL) { cfg.password = strdup(oldpassword); }
	if (cfg.listenport == 0) { cfg.listenport = oldlistenport; }
	
#ifdef CHANLOG
	/* Open logs. */
	LLIST_WALK_H(active_channels.head, channel_type *);
		chanlog_open(data);
	LLIST_WALK_F;
#endif /* CHANLOG */

#ifdef QUICKLOG
	/*
	 * Empty quicklog if flushqlog is set to true and we have clients
	 * connected or if qloglength is set to 0.
	 */
	if ((cfg.flushqlog == 1 && c_clients.connected > 0) ||
			cfg.qloglength == 0) {
		qlog_replay(NULL, 0);
	}
#endif /* QUICKLOG */

	/* By default, we don't know anything about server. */
	servers.fresh = 1;

	/*
	 * If there was no nicks defined in configuration-file, save current
	 * nick to the list. Warnings have been send already.
	 */
	if (nicknames.nicks.head == NULL) {
		node = llist_create(strdup(status.nickname));
		llist_add(node, &nicknames.nicks);
		/* We're happy with the nick we have. */
		status.got_nick = 1;

		if (cfg.nickfillchar == '\0') {
			cfg.nickfillchar = DEFAULT_NICKFILL;
		}
	}

	/* Ok, we know we _do_ have a nick. Lets see if it's a good one. */
	if (xstrcasecmp((char *) nicknames.nicks.head->data,
				status.nickname) != 0) {
		status.got_nick = 0;
		timers.nickname = cfg.getnickinterval;
	}
	
	/* Next nick we try is the first one on the list. */
	nicknames.next = NICK_FIRST;

	/* Listening port or host changed. */
	if (oldlistenport != cfg.listenport ||
			(oldlistenhost == NULL ?
				(cfg.listenhost == NULL ? 0 : 1) :
				(cfg.listenhost == NULL ? 1 :
					xstrcmp(oldlistenhost, cfg.listenhost))
			)) {
		create_listen();
	}

#ifdef INBOX
	/* We're logging no more. */
	if (! cfg.inbox && inbox != NULL) {
		fclose(inbox);
		inbox = NULL;
	}
	
	/* We should start logging. */
	if (cfg.inbox && inbox == NULL) {
		inbox = fopen(FILE_INBOX, "a+");
		if (inbox == NULL) {
			report(MIAU_ERRINBOXFILE);
		}
	}
#endif /* INBOX */

	/* Reopen log-file. */
	/*
	 * Hmm, this doesn't work.
	 * 
	if (! freopen(FILE_LOG, "a", stdout)) {
		irc_mnotice(&c_clients, status.nickname, MIAU_ERRLOGCONN);
		freopen("/dev/null", "a", stdout);
	}
	 */

	/* Real name or user name changed. */
	if (xstrcmp(oldrealname, cfg.realname) ||
			xstrcmp(oldusername, cfg.username)) {
		server_drop(MIAU_RECONNECT);
		report(MIAU_RECONNECT);
		i_server.current--;
		status.reconnectdelay = cfg.reconnectdelay;
		timers.connect = cfg.reconnectdelay - 3;
		server_next(0);
	}

	/* Free backuped stuff. */
	xfree(oldrealname);
	xfree(oldusername);
	xfree(oldlistenhost);
} /* void rehash() */



/*
 * Client disconnected from bouncer.
 *
 * If appropriate, leave channels and set user away.
 */
void
clients_left(
		const char	*reason
	    )
{
	char	*chans;	/* Channels we're going to part/leave. */
	char	*channel;
	const char	*leavemsg;
	const char	*awaymsg;
	if (reason == NULL) {
		awaymsg = cfg.awaymsg;
		leavemsg = cfg.leavemsg;
	} else {
		awaymsg = cfg.usequitmsg ? reason : cfg.awaymsg;
		leavemsg = cfg.usequitmsg ? reason : cfg.leavemsg;
	}

	chans = xcalloc(1, 1);
	
	/*
	 * When all clients have detached from miau...
	 *
	 * ...if cfg.leave == true, part channels depending on
	 * configuration and if cfg.rejoin == true, also move channels from
	 * active_channels to passive_channels.
	 *
	 * Because "The PART command causes the user sending the message to be
	 * removed ... This request is always granted by the server." (RFC 2812)
	 * we won't bother tracking when we actually have _left_ the channel.
	 */
	if (active_channels.head != NULL) {
		/* Build a string of channel list. */
		LLIST_WALK_H(active_channels.head, channel_type *);
			chans = xrealloc(chans, strlen(data->name) + 2 +
					strlen(chans));
			strcat(chans, ",");
			strcat(chans, data->name);
			if (cfg.leave) {
				if (cfg.rejoin) {
					/*
					 * Need to move channel from
					 * active_channels to passive_channels.
					 * This means that we have left the
					 * channels and lost all our
					 * priviledges.
					 */
#ifdef AUTOMODE
					data->oper = -1;
#endif /* AUTOMODE */
					llist_add_tail(llist_create(data),
							&passive_channels);
				} else {
					/*
					 * Not moving channels from list to
					 * list, therefore freeing
					 * resources.
					 */
					xfree(data->name);
					xfree(data->topic);
					xfree(data->topicwho);
					xfree(data->key);
					xfree(data);
				}
				/* Remove channel node from old list. */
				llist_delete(node, &active_channels);
			}
		LLIST_WALK_F;

#ifdef CHANLOG
		/* We don't want to leave channels nor message channels. */
		if (! cfg.leave && cfg.leavemsg == NULL) {
			chanlog_write_entry_all(LOG_MIAU, LOGM_MIAU,
					get_short_localtime(), "dis");
		}
		else
#endif /* CHANLOG */
		
		/* We want to send an ACTION on each channel. */
		if (! cfg.leave && cfg.leavemsg != NULL) {
			channel = strtok(chans + 1, ",");
			while (channel != NULL) {
				irc_write(&c_server,
						"PRIVMSG %s :\1ACTION %s\1",
						channel, leavemsg);
				channel = strtok(NULL, ",");
			}
		}

		/* We want to part each channel with a message. */
		else if (cfg.leave && (cfg.leavemsg || reason)) {
			irc_write(&c_server, "PART %s :%s",
					chans + 1, leavemsg);
		}
		
		/* Ok, we just want to part channels. */
		else if (cfg.leave && cfg.leavemsg == NULL) {
			report(MIAU_LEAVING);
			irc_write(&c_server, "JOIN 0");		/* RFC 2812 */
		}
	}

	/* Finally, free chans. */
	xfree(chans);

	set_away(awaymsg); /* Try setting user away with given message. */
} /* void clients_left(const char *) */



/*
 * Check timers.
 */
void
check_timers(
	    )
{
	llist_node	*client;
	llist_node	*client_next;
	connection_type	*client_con;
	static time_t	oldtime = 0;
	static time_t	floodtime = 0;
	static time_t	newtime;
	FILE		*fwd;

	/* Don't bother doing this too often. */
	if (time(&newtime) <= oldtime) {
		return;
	}
	oldtime = newtime;
	
	/*
	 * If MSGLENGTH second(s) have elapsed (since last allowance) allow
	 * sending anohter message. Don't forget to limit the counter to
	 * BURSTSIZE.
	 */
	if (newtime - floodtime >= MSGLENGTH && msgtimer < BURSTSIZE) {
		msgtimer += (newtime - floodtime) / MSGLENGTH;
		if (msgtimer > BURSTSIZE) {
			msgtimer = BURSTSIZE;
		}
		floodtime = newtime;
		irc_process_queue();
	}

	/*
	 * Countdown to next try to connect the server.
	 *
	 * We actually do nothing here - except advance the timer and make
	 * sure it doesn't reach CONN_DISABLED. Server is tried to connect in
	 * run().
	 */
	if (cfg.reconnectdelay != CONN_DISABLED) {
		timers.connect++;
	}

#ifdef _NEED_PROCESS_IGNORES
	process_ignores();
#endif /* _NEED_PROCESS_IGNORES */

	client = c_clients.clients->head;
	while (client != NULL) {
		client_next = client->next;
		client_con = (connection_type *) client->data;
		/* From time to time, see if connected clients are alive. */
		switch (proceed_timer_safe(&client_con->timer, 60, 120, 10)) {
			case 0:
				break;
				
			case 1:
				irc_write(client_con, "PING :%s",
						i_server.realname);
				break;
				
			case 2:
				/* (single client, message, error, echo, no) */
				client_drop(client_con, CLNT_STONED, ERROR, 1,
						NULL);
				/* error(CLNT_STONED); */
				break;
		}
		client = client_next;
	}

	if (c_clients.connected == 0) {
		/* Client is not connected. Anti-idle. */
		if (cfg.antiidle && proceed_timer(&timers.antiidle, 0, 
					cfg.antiidle * 60) == 2) {
			irc_write(&c_server, "PRIVMSG");
		}
	}

	if (i_server.connected > 0) {
		int	timeout = (i_server.connected == 2) ?
			cfg.stonedtimeout : 120;

		/*
		 * If we have already registered our nick, timeout is
		 * configured in miaurc (default 90 seconds). If we are
		 * still registering our nick, timeout is 120 seconds.
		 *
		 * Anyway, if we haven't got a thing from the server in
		 * last "cfg.stonedtimeout - 30" seconds (minumun 10 seconds),
		 * we'll ping it. If it doesn't reply in time, we'll keep on
		 * pinging it every five seconds until it responds, drops
		 * connection or cfg.stonedtimeout seconds have passed.
		 */
		switch (proceed_timer_safe(&c_server.timer,
					(timeout - 30 >= 5) ? timeout - 30 : 5,
					timeout, 5)) {
			case 0:
				/*
				 * We are connected and we're not lagging -
				 * not too bad at least. If the server keeps
				 * serving us this well, try this server again
				 * if we get disconnected.
				 */
				if (! status.good_server
						&& i_server.connected == 2
						&& proceed_timer(
							&timers.good_server,
							GOOD_SERVER_DELAY,
							GOOD_SERVER_DELAY)) {
					status.good_server = 1;
				}
				break;
			case 1:
				irc_write(&c_server, "PING :%s",
						i_server.realname);
#ifdef PINGSTAT
				ping_sent++;
#  ifdef ENDUSERDEBUG
				if (c_clients.connected > 0
						&& c_server.timer >
						(timeout - 20 >= 5 ?
						timeout - 20 : 5)) {
					enduserdebug("pinging server (%d/%d)",
							c_server.timer,
							timeout);
				}
#  endif /* ENDUSERDEBUG */
#endif /* PINGSTAT */
				break;
			case 2:
				server_drop(SERV_STONED);
				error(SERV_STONED);
				server_next(0);
				break;
		}
	}

	if (i_newclient.connected) {
		/* Client is connecting... */
		switch (proceed_timer(&c_newclient.timer, 0, 30)) {
			case 0:
				
			case 1:
				break;
				
			case 2:
				if (status.init) {
					error(CLNT_AUTHFAIL);
					irc_write(&c_newclient,
							":%s 464 %s :Password Incorrect",
							i_server.realname,
							(char *) nicknames.nicks.head->data);
					drop_newclient("Bad Password");
				}

				else {
					error(CLNT_AUTHTO);
					drop_newclient("Ping timeout");
				}

				if (c_clients.connected > 0) {
					irc_mnotice(&c_clients,
							status.nickname,
							CLNT_AUTHFAILNOTICE,
							i_newclient.hostname);
				}


				/*
				 * Deallocate would also happen on next
				 * newclient-connect, but let's do it here.
				 */
				FREE(i_newclient.nickname);
				FREE(i_newclient.username);
				FREE(i_newclient.hostname);

				break;
		}
	}

	/* Throttle allowed connections. */
	if (proceed_timer(&timers.listen, 0, 15) == 2) {
		status.allowconnect = 1;
	}

	/* Throttle allowed replies. */
	if (proceed_timer(&timers.reply, 0, 4) == 2) {
		status.allowreply = 1;
	}

	/*
	 * Try to get nick if message queue is non-full and other factors
	 * allow it, too.
	 */
	if (! status.got_nick && cfg.getnick != 0 && i_server.connected == 2
			&& msgtimer > 0) {
		/* FIXME: don't trust bits of cfg.getnick anymore */
		if (proceed_timer(&timers.nickname, 0,
					cfg.getnickinterval) == 2) {
			if ((c_clients.connected <= 0 && (cfg.getnick & 1)) ||
					(c_clients.connected > 0 &&
						(cfg.getnick & 2))) {
				/* Getting nick now, increase counter. */
				status.getting_nick++;
				irc_write(&c_server, "NICK %s",
						(char *)
						nicknames.nicks.head->data);
			}
		}
	}

	/* Try to join unjoined channels (when appropriate). */
	if (i_server.connected == 2 && cfg.rejoin
			&& passive_channels.head != NULL
			&& ! (c_clients.connected == 0 && cfg.leave == 1)) {
		/*
		 * Perhaps we don't need to define tryjoininterval.
		 * Let's just try joining unjoined channels once a minute.
		 */
		if (proceed_timer(&timers.join, 0, JOINTRYINTERVAL) == 2) {
			channel_join_list(LIST_PASSIVE, 0, NULL);
		}
	}			

	/* Handle forwarded messages. */
	if (cfg.forwardmsg) {
		switch (proceed_timer(&timers.forward, 0, 180)) {
			case 0:
			case 1:
				break;
			case 2:
				if (! forwardmsg) {
					/* Nothing to forward. */
					break;
				}
				
				fwd = popen(cfg.forwardmsg, "w");
				/*
				 * If there's an error while creating a pipe,
				 * save messages and try forwarding them
				 * later. It probably would be nice to limit
				 * memory allocated by forwarded messages
				 * somehow... TODO.
				 */
				if (fwd) {
					fprintf(fwd, "%s", forwardmsg);
					pclose(fwd);
					FREE(forwardmsg);
				}
				
				break;
		}
	}
	
#ifdef AUTOMODE
	/* Act as op-o-matic. */
	if (status.automodes > 0 && proceed_timer(&timers.automode,
				0, cfg.automodedelay) == 2) {
		/* automode(...) checks if we have operator-status. */
		automode_do(&c_server);
	}
#endif /* AUTOMODE */

#ifdef PRIVLOG
	/* Close logfiles that haven't been used for a while. */
	if (privlog_has_open() && proceed_timer(&timers.privlog,
				0, PRIVLOG_CHECK_PERIOD)) {
		privlog_close_old();
	}
#endif /* PRIVLOG */

#ifdef DCCBOUNCE
	/* Bounce DCCs. */
	if (cfg.dccbounce) {
		dcc_timer();
	}
#endif /* DCCBOUNCE */
} /* void check_timers() */



void
get_nick(
		char	*format
	)
{
	char	*oldnick;
	char	*badnick = strdup(status.nickname);
	/* if (badnick == NULL) { escape(); } */
	/* We actually never handle strdups returning NULL. */

	/*
	 * Change nick if we're still introducing outself to the server or if
	 * we were forced to change ...
	 */
	if (i_server.connected != 2) {
		/* Try first nick. */
		if (nicknames.next == NICK_FIRST) {
			nicknames.current = nicknames.nicks.head;
			nicknames.next = NICK_NEXT;
		}
		/* Try next nick on the list. */
		else if (nicknames.next == NICK_NEXT) {
			nicknames.current = nicknames.current->next;
		}
		/*
		 * If we still don't have a nick and we don't want to
		 * generate one.
		 */
		if (nicknames.current == NULL && cfg.nickfillchar == '\0') {
			nicknames.current = nicknames.nicks.head;
			/*
			 * If nicknames-list happends to be empty, we will -
			 * we like it or not - generate a nick.
			 */
			if (nicknames.current->data != NULL) {
				xfree(status.nickname);
				status.nickname =
					strdup(nicknames.current->data);
			}
		}
		if (nicknames.current == NULL) {
			/* Generate a nickname. */
			nicknames.next = NICK_GEN;
			/*
			 * All pre-defined nicks are in use. We must generate
			 * a new one.
			 */
			oldnick = strdup(status.nickname);
			xfree(status.nickname);
			status.nickname = (char *) xmalloc(cfg.maxnicklen + 1);
			randname(status.nickname, oldnick, cfg.maxnicklen);
			xfree(oldnick);
		} else {
			status.nickname = strdup((char *)
					nicknames.current->data);
		}

		report(format, badnick, status.nickname);
		/* Getting nick, increase sent NICK-command counter. */
		status.getting_nick++;
		irc_write(&c_server, "NICK %s", status.nickname);
	}
	xfree(badnick);
	status.got_nick = 0;
} /* void get_nick(char *) */



/*
 * User has connected to bouncer.
 */
void
fakeconnect(
		connection_type	*newclient
	   )
{
	int		i;
#ifdef ASCIIART
	int		pic;
#endif /* ASCIIART */

	if (status.nickname == NULL) {
		/* Get us a nick if we don't have one already. */
		status.nickname = strdup((char *) nicknames.nicks.head->data);
	}

	if (i_server.connected == 2) {
		/* Print server information. */
		irc_write(newclient, ":%s 001 %s %s %s!~%s@%s",
				i_server.realname,
				status.nickname,
				i_server.greeting[0],
				status.nickname,
				i_client.username,
				i_client.hostname);

		for (i = 1; i < 4; i++) {
			irc_write(newclient,
					":%s %03d %s %s",
					i_server.realname,
					i + 1,
					status.nickname,
					i_server.greeting[i]);
		}

		for (i = 0; i < 3; i++) {
			if (i_server.isupport[i] != NULL) {
				irc_write(newclient, ":%s 005 %s %s",
						i_server.realname,
						status.nickname,
						i_server.isupport[i]);
			}
		}

		/* Fetch some info about users. */
		irc_write(&c_server, "LUSERS");
	} else {
		miau_welcome();
	}

	irc_write(newclient, ":%s 375 %s :"MIAU_VERSION,
			i_server.realname,
			status.nickname);

#ifdef ASCIIART
	/* Print a pretty picture. */
	
	/* Cats sleep when they want to. */
	pic = rand() % 2;

	for (i = 0; i < PIC_Y; i++) {
		irc_write(newclient, ":%s 372 %s :%s",
				i_server.realname,
				status.nickname,
				pics[pic][i]);
	}
#endif /* ASCIIART */

	if (i_server.connected == 2) {
		irc_write(newclient, ":%s 372 %s :"MIAU_372_RUNNING,
				i_server.realname,
				status.nickname,
				((server_type *) servers.servers.head->data)->name,
				status.nickname);
	} else {
		irc_write(newclient, ":miau 372 %s :"MIAU_372_NOT_CONN,
				status.nickname);
	}

#ifdef INBOX
#  ifdef QUICKLOG
	/* Move old qlog-lines to privmsglog. */
	qlog_drop_old();
#  endif /* QUICKLOG */
	if (inbox != NULL && ftell(inbox) != 0) {
		irc_write(newclient, ":%s 372 %s :- "CLNT_HAVEMSGS,
				i_server.realname,
				status.nickname);
	} else {
		irc_write(newclient, ":%s 372 %s :- "CLNT_INBOXEMPTY,
				i_server.realname,
				status.nickname);
	}
#endif /* INBOX */

	irc_write(newclient, ":%s 376 %s :"MIAU_END_OF_MOTD,
			i_server.realname,
			status.nickname);

	if (i_server.connected == 2) {
		/* Tell client to set nick to bouncer's. */
		if (xstrcmp(i_client.nickname, status.nickname) != 0) {
			irc_write(newclient, ":%s NICK :%s",
					i_client.nickname,
					status.nickname);
			xfree(i_client.nickname);
			i_client.nickname = strdup(status.nickname);
		}
		
		/*
		 * And we were away and didn't have hand-set away-message,
		 * remove away-status.
		 */
		if (! (status.awaystate & CUSTOM)
				&& (status.awaystate & AWAY)) {
			irc_write(&c_server, "AWAY");
			status.awaystate &= ~AWAY;
		}
	}

	server_check_list();

	/* Tell client if we're supposedly marked as being away. */
	if ((status.awaystate & AWAY)) {
		irc_write(newclient, ":%s 306 %s :"IRC_AWAY,
				i_server.realname,
				status.nickname);
	}

	/* 
	 * Ok, client knows what's going on. If we're connected to server,
	 * we have things to do, mister.
	 */
	if (i_server.connected == 2) {
		/* 
		 * First of all, join passive channels.
		 * 
		 * Passive channels are channels that are either defined
		 * in miaurc and are not yet joined or channels, that we
		 * were on when clients detached.
		 *
		 * Sending JOIN to server doesn't remove channels from the
		 * list - positive JOIN-reply from server is needed for that.
		 *
		 * TODO: Accept denial from server.
		 */
		channel_join_list(LIST_PASSIVE, 1, NULL);

		/*
		 * Next, tell client to join active channels.
		 *
		 * Active channels are channels that we currently are on.
		 *
		 * This means this list won't get flushed either.
		 */
		channel_join_list(LIST_ACTIVE, 0, newclient);
	}
} /* void fakeconnect(connection_type *) */



/*
 * Say hello to bouncer.
 *
 * Some IRC-clients take server's name from this message - this is why we
 * send them this welcome-message when jumping off the server.
 */
void
miau_welcome(
	    )
{
	irc_mwrite(&c_clients, ":miau 001 %s :%s %s!user@host",
			status.nickname,
			MIAU_WELCOME,
			status.nickname);
} /* void miau_welcome() */



/*
 * Set user away if...
 */
void
set_away(
		const char	*msg
	)
{
	/*
	 * Not connected or no provided away-message and cfg.awaymsg unset?
	 * Don't change away-message!
	 * */
	if (i_server.connected != 2 || (msg == NULL && cfg.awaymsg == NULL)) {
		return;
	}

	/* Using hand-set away-message? */
	if ((status.awaystate & CUSTOM)) {
		/* If we're not away, set us away. Otherwise do nothing. */
		if (! (status.awaystate & AWAY)) {
			irc_write(&c_server, "AWAY :%s", status.awaymsg);
			status.awaystate |= AWAY;
		}
		return;
	}

	/*
	 * Ok, we got this far. We know that we're not using custom awaymsg.
	 *
	 * If away-message was not provided and we still have one more more
	 * client connected, do nothing.
	 */
	if (msg == NULL && c_clients.connected > 0) {
		return;
	}

	/*
	 * If away-reason was given, use it.
	 */
	if (msg != NULL) {
		xfree(status.awaymsg);
		status.awaymsg = strdup(msg);
		/* Unset us away so we get marked later. */
		status.awaystate &= ~AWAY;
	}

	/* Reached this far being away? Well then, there's nothing to do. */
	if ((status.awaystate & AWAY)) {
		return;
	}

	/* Finally, mark us away. */
	irc_write(&c_server, "AWAY :%s", (status.awaymsg != NULL)
			? status.awaymsg : cfg.awaymsg);
	status.awaystate |= AWAY;
} /* void set_away(const char *) */



int
read_newclient(
	      )
{
	connection_type	*newclient;
	llist_node	*node;
	char		*command;
	char		*param;
	int		c_status;

	c_status = irc_read(&c_newclient);
	
	if (c_status > 0) {
		c_status = 0;
		c_newclient.buffer[30] = 0;
		command = strtok(c_newclient.buffer, " ");
		param = strtok(NULL, "\0");
		
		if (command && param) {
			upcase(command);
			if ((xstrcmp(command, "PASS") == 0) &&
					! (status.init & 1)) {
				/* Only accept password once */
				if (*param == ':') {
					param++;
				}
				if (strlen(cfg.password) == 13) {
					/* Assume it's crypted */
					if (xstrcmp(crypt(param, cfg.password),
								cfg.password)
							== 0) {
						status.passok = 1;
					}
				}
				
				else {
					if (xstrcmp(param, cfg.password) == 0) {
						status.passok = 1;
					}
				}

				status.init = status.init | 1;
			}
			
			if (xstrcmp(command, "NICK") == 0) {
				status.init = status.init | 2;
				xfree(i_newclient.nickname);
				i_newclient.nickname = strdup(
						strtok(param, " "));
			}
			
			if (xstrcmp(command, "USER") == 0) {
				status.init = status.init | 4;
				xfree(i_newclient.username);
				i_newclient.username = strdup(
						strtok(param, " "));
			}
			
			if (status.init == 7 && status.passok) {
				/* Client is in ! */
				report(CLNT_AUTHOK);

				xfree(i_client.nickname);
				xfree(i_client.username);
				xfree(i_client.hostname);
				
				i_client.nickname = i_newclient.nickname;
				i_client.username = i_newclient.username;
				i_client.hostname = i_newclient.hostname;
				
				i_newclient.nickname = NULL;
				i_newclient.username = NULL;
				i_newclient.hostname = NULL;

				newclient = xmalloc(sizeof(connection_type));
				node = llist_create(newclient);
				llist_add(node, c_clients.clients);
				c_clients.connected++;
		
				newclient->socket = c_newclient.socket;
				newclient->timer = 0;
				newclient->offset = 0;

				i_newclient.connected = 0;
				c_newclient.socket = 0;
				status.passok = 0;
				status.init = 0;

				fakeconnect(newclient);
				
				/* 
				 * Reporting number of connected clients
				 * is irrevelant if only one is allowed at
				 * a time.
				 */
				if (cfg.maxclients != 1) {
					report(CLNT_CLIENTS,
							c_clients.connected);
				}
			}
		}
	}
	return c_status;
} /* int read_newclient() */



/*
 * Handle commands provided to miau from attached IRC-client.
 */
void
miau_commands(
		char		*command,
		char		*param,
		connection_type	*client
	     )
{
	int	corr = 0;
	int	i;

	/* No command at all ? */
	if (! command) {
		/*
		 * Basically we could just wait if-elseif-else to reach its
		 * end where we have this "(! corr)" -test, but xstrcmp
		 * wouldn't like NULL...
		 */
		irc_notice(client, status.nickname,
				CLNT_COMMANDS);
		return;
	}

	upcase(command);

	if (xstrcmp( command, "REHASH") == 0) {
		corr++;
		rehash();
	}

#ifdef INBOX
	if (xstrcmp(command, "READ") == 0) {
		char	*s;
		corr++;
		if (inbox && ftell(inbox)) {
			fflush(inbox);
			rewind(inbox);

			irc_notice(client, status.nickname, CLNT_INBOXSTART);

			s = (char *) xmalloc(1024);
			while (fgets(s, 1023, inbox)) {
				if (s[strlen(s) - 1] == '\n') {
					s[strlen(s) - 1] = 0;
				}
				irc_notice(client, status.nickname, "%s", s);
			}
			xfree(s);

			irc_notice(client, status.nickname, CLNT_INBOXEND);
			fseek(inbox, 0, SEEK_END);
		}
		else {
			irc_notice(client, status.nickname, CLNT_INBOXEMPTY);
		}
	}

	else if (xstrcmp(command, "DEL") == 0) {
		corr++;
		if (inbox) {
			fclose(inbox);
		}
		unlink(FILE_INBOX);
		inbox = fopen(FILE_INBOX, "w+");

		irc_notice(client, status.nickname, CLNT_KILLEDMSGS);
	}
#endif /* INBOX */

	else if (xstrcmp(command, "RESET") == 0) {
		corr++;
		irc_mnotice(&c_clients, status.nickname, MIAU_RESET);
		server_reset();
	}

#ifdef PINGSTAT
	else if (xstrcmp(command, "PINGSTAT") == 0) {
		if (ping_sent == 0) {
			irc_notice(client, status.nickname, PING_NO_PINGS);
		} else {
			irc_notice(client, status.nickname, PING_STAT,
					ping_sent, ping_got,
					100 - (int) ((float) ping_got /
						(float) ping_sent * 100.0));
		}
		corr++;
	}
#endif

	
#ifdef FAKECMD
	/* Developement only. */
	else if (xstrcmp(command, "FAKECMD") == 0) {
		corr++;
		char	*tbackup = strdup(param);
		char	*torigin;
		char	*tcommand;
		char	*tparam1;
		char	*tparam2;
		int	tcommandno;
		torigin = strtok(param + 1, " ");
		tcommand = strtok(NULL, " ");
		tparam1 = strtok(NULL, " ");
		tparam2 = strtok(NULL, "\0");
		if (tcommand) {
			tcommandno = atoi(tcommand);
			if (tcommandno == 0) {
				tcommandno = MINCOMMANDVALUE +
					command_find(tcommand);
			}
			server_reply(tcommandno,
					tbackup,
					torigin,
					tparam1,
					tparam2,
					&tcommandno);
		}
		xfree(tbackup);
	}
#endif /* FAKECMD */


#ifdef UPTIME
	else if(xstrcmp(command, "UPTIME") == 0) {
		time_t now;
		int seconds, minutes, hours, days;
		
		corr++;
		time(&now);
		now -= status.startup;
		getuptime(now, &days, &hours, &minutes, &seconds),
		
		irc_notice(client, status.nickname, MIAU_UPTIME,
				days, hours, minutes, seconds);
	}
#endif /* UPTIME */
	
#ifdef DUMPSTATUS
	else if (xstrcmp(command, "DUMP") == 0) {
		corr++;
		dump_status();
	}
#endif /* DUMPSTATUS */

	else if (xstrcmp(command, "JUMP") == 0) {
		status.good_server = 0;	/* Don't try this server again. :-) */
		corr++;
		/* Are there any other servers ? */
		if (servers.amount != 0) {
			server_drop(MIAU_JUMP);
			report(MIAU_JUMP);
			if (param) {
				i = atoi(param);
				if (i < 1) i = 1;
				if (i >= servers.amount) i = servers.amount - 1;
				i--;
				i_server.current = servers.servers.head;
				while (i--) {
					i_server.current =
						i_server.current->next;
				}
				((server_type *) i_server.current->next->data)->working = 1;
			}

			/* Reconnecting to current server is ok. */
			miau_welcome();
			server_next(0);

			/* Try connecting ASAP. */
			timers.connect = cfg.reconnectdelay;
		} else {
			server_check_list();
		}
	}
		

	else if (xstrcmp(command, "DIE") == 0) {
		/*
		 * We need a copy of quit-reason because client-structures,
		 * where the original reason came from, is freed when we
		 * need it.
		 */
		char *reason = (param == NULL ?
				strdup(MIAU_DIE_CL) : strdup(param));
		/*
		 * If user issues a DIE-command, (s) most likely knows why
		 * connection was lost. Therefore we don't need to tell clients
		 * what's going on.
		 */
		/* (all clients, reason, notice, echo, no) */
		client_drop(NULL, reason, DYING, 1, NULL);
		server_drop(reason);
		drop_newclient(NULL);
		if (reason != NULL) { xfree(reason); }
		/*
		 * Finally free memory allocated by copy of user's command.
		 * (See client_free() for more info.
		 */
		client_free();
		escape();
	}

	else if (xstrcmp(command, "PRINT") == 0) {
		llist_node	*node;
		server_type	*server;
		int		cur_ndx = -1;
		int		i = 1;
		corr++;
		/*
		 * We try to list the servers and do things like that - even
		 * if there aren't any servers. Oh well, at least I don't need
		 * to nest those lines... Lame. :-)
		 *
		 * In fact, we never try to view our fallback server.
		 * Perhaps we should ?
		 */
		irc_notice(client, status.nickname, (servers.amount != 1) ?
				CLNT_SERVLIST : CLNT_NOSERVERS);
		for (node = servers.servers.head->next; node != NULL;
				node = node->next) {
			server = (server_type *) node->data;
			irc_notice(client, status.nickname,
					"%c[%d] %s:%d%s (%d)",
					(server->working ? '+' : '-' ),
					i,
					server->name,
					server->port,
					(server->password ? ":*" : ""),
					(server->timeout == 0 ?
						cfg.connecttimeout :
						server->timeout));
			if (node == i_server.current) {
				cur_ndx = i;
			}
			i++;
		}
		if (servers.fresh == 0) {
			irc_notice(client, status.nickname, CLNT_CURRENT,
					cur_ndx,
					(i_server.connected == 2) ?
					"" : CLNT_ANDCONNECTING);
		} else if (i_server.connected == 1) {
			irc_notice(client, status.nickname, CLNT_CONNECTING);
		}
	}

	if (! corr) {
		irc_notice(client, status.nickname, CLNT_COMMANDS);
	}
} /* void miau_commands(char *, char *, connection_type	*) */



void
run(
   )
{
	llist_node	*client;
	llist_node	*client_next;
	connection_type	*client_conn;
	server_type	*server;
	fd_set		rfds, wfds;
	struct timeval	tv;
	int		selret;

	FD_ZERO(&wfds);		/* Needed if DCCBOUNCE is not defined. */


	/* Do this 'till the end of days - until told otherwise. :-) */
	while (1) {
		if (! i_server.connected &&		/* Not connected. */
				servers.amount > 1 &&	/* Servers on list. */
				/* Time to connect. */
				timers.connect > status.reconnectdelay) {
			server = (server_type *) i_server.current->data;
			server_set_fallback(i_server.current);

			/* Not connected to server. */
			report(SERV_TRYING, server->name, server->port);
			servers.fresh = 0;
		
			/* Assume we have our desired nick. */
			xfree(status.nickname);
			status.nickname = strdup((char *)
					nicknames.nicks.head->data);
			nicknames.current = nicknames.nicks.head;
			nicknames.next = NICK_NEXT;
			/* Don't try connecting for too long... */
			alarm(server->timeout == 0 ?
					cfg.connecttimeout : server->timeout);
			switch (irc_connect(&c_server, server,
						(char *)
						nicknames.nicks.head->data,
						cfg.username,
						cfg.realname, cfg.bind)) {
				case CONN_OK:
					report(SOCK_CONNECT, ((server_type *) i_server.current->data)->name);
					i_server.connected = 1;
					/* Clear good server -variables. */
					status.good_server = 0;
					timers.good_server = 0;
					/* Clear getting_nick -counter. */
					status.getting_nick = 0;
					break;
	
				case CONN_SOCK:
					error(SOCK_ERROPEN, net_errstr);
					escape();
					break;
			
				case CONN_LOOKUP:
					error(SOCK_ERRRESOLVE, server->name);
					server_drop(NULL);
					server_next(1);
					break;	
			
				case CONN_BIND:
					if (cfg.bind) {
						error(SOCK_ERRBINDHOST,
								cfg.bind,
								cfg.listenport,
								net_errstr);
					} else {
						error(SOCK_ERRBIND,
								cfg.listenport,
								net_errstr);
					}
					escape();
					break;
					
				case CONN_CONNECT:
					/*
					 * If timer was triggered, socket is
					 * already closed and we don't need to
					 * display another error.
					 */
					if (c_server.socket != 0) {
						error(SOCK_ERRCONNECT, server->name, strerror(errno));
						sock_close(&c_server);
					}
					server_next(1);
					break;
					
				case CONN_WRITE:
					error(SOCK_ERRWRITE, server->name);
					server_drop(NULL);
					server_next(0);
					break;
			
				case CONN_OTHER:
					error(SOCK_GENERROR, net_errstr);
					escape();
					break;
			
				default:
					break;
			}
			
			alarm(0);		/* Disable alarm. */
			timers.connect = 0;
		}

		/* TODO: Changing FD_SETs only on demand would be nice... */
		FD_ZERO(&rfds);

		/* Listen for new clients. */
		if (status.allowconnect && ! i_newclient.connected &&
				cfg.maxclients > c_clients.connected) {
			FD_SET(listensocket, &rfds);
		}
		/* Listen for the server we're connected to. */
		if (i_server.connected) {
			FD_SET(c_server.socket, &rfds);
		}
		/* Listen clients we already have. */
		for (client = c_clients.clients->head; client != NULL;
				client = client->next) {
			FD_SET(((connection_type *) client->data)->socket,
					&rfds);
		}
		/* Listen connecting client. */
		if (i_newclient.connected) {
			FD_SET(c_newclient.socket, &rfds);
		}

#ifdef DCCBOUNCE
		FD_ZERO(&wfds);
		/* If others need this, it must be taken outside ifdef. */
		if (cfg.dccbounce) {
			dcc_socketsubscribe(&rfds, &wfds);
		}
#endif /* DCCBOUNCE */
	
		tv.tv_usec = 0;
		tv.tv_sec = 1;

		selret = select(highest_socket + 1, &rfds, NULL, NULL, &tv);
		if (selret > 0) {
			/* Data from server. */
			if (FD_ISSET(c_server.socket, &rfds)) {
				/* Server closed connection. */
				if (server_read() < 0) {
					server_drop(SERV_DROPPED);
					error(SERV_DROPPED);
					server_next(0);
				}
			} /* Data from server. */

			/* Connection from a new client. */
			if (FD_ISSET(listensocket, &rfds)) {
				status.allowconnect = 0;
				xfree(i_newclient.hostname);
				c_newclient.socket = sock_accept(listensocket,
						&i_newclient.hostname, 1);
				if (c_newclient.socket != -1) {
					c_newclient.offset = 0;
					i_newclient.connected = 1;
					
					report(CLNT_CAUGHT,
							i_newclient.hostname);
				}
				else {
					/*
					 * Report unauthorized connections and
					 * connections that couldn't be
					 * accepted.
					 */
					if (c_newclient.socket) {
						/* Bad nesting. */
						report(CLNT_DENIED,
							i_newclient.hostname);
					}
					/* Error while accepting. */
					else {
						/* Bad nesting. */
						error(SOCK_ERRACCEPT,
							i_newclient.hostname);
					}
					FREE(i_newclient.hostname);
					c_newclient.socket = 0;
				}
			} /* Connection from new client. */

			/* Data from connected client. */
			client = c_clients.clients->head;
			while (client != NULL) {
				client_next = client->next;
				client_conn = (connection_type *) client->data;
				if (FD_ISSET(client_conn->socket, &rfds)) {
					/* Client closed connection. */
					if (client_read(client_conn) < 0) {
						/*
						 * (single client, message,
						 * report, no echo, no)
						 */
						client_drop(client_conn,
								CLNT_DROPPED,
								REPORT, 0,
								NULL);
						/* report(CLNT_DROPPED); */
					}
				}
				client = client_next;
			} /* Data from connected client. */

			/* Data from new client. */
			if (c_newclient.socket > 0 &&
					FD_ISSET(c_newclient.socket, &rfds)) {
				/* New client dropped connection. */
				if (read_newclient() < 0) {
					drop_newclient(NULL);
					report(CLNT_DROPPEDUNAUTH);
				}
			} /* Data from new client. */

#ifdef DCCBOUNCE
			if (cfg.dccbounce) {
				dcc_socketcheck(&rfds, &wfds);
			}
#endif
		}
		
		check_timers();
	}
} /* void run() */



void
pre_init(
	)
{
	/* Initialize structure for nicknames. */
	nicknames.nicks.head = NULL;
	nicknames.nicks.tail = NULL;

	/* Initialize some client-structures. */
	c_clients.clients = xmalloc(sizeof(llist_list));
	c_clients.clients->head = NULL;
	c_clients.clients->tail = NULL;

	/* Create fallback server entry. */
	add_server("", 0, NULL, 0);

	/*
	 * Initially, we're not on channels nor there are any channels to
	 * be joined at attach.
	 */
	active_channels.head = NULL;
	active_channels.tail = NULL;
	passive_channels.head = NULL;
	passive_channels.tail = NULL;
	old_channels.head = NULL;
	old_channels.tail = NULL;
} /* void pre_init() */



void
init(
    )
{
	struct sigaction	sv;

	/* Create */
	sigemptyset(&sv.sa_mask);
	sv.sa_flags = 0;
	sv.sa_handler = sig_term;
	sigaction(SIGTERM, &sv, NULL);
	sigaction(SIGINT, &sv, NULL);
	/* sigaction(SIGKILL, &sv, NULL); -- shouldn't be catched... */

	sv.sa_handler = connect_timeout;
	sigaction(SIGALRM, &sv, NULL);

	sv.sa_handler = rehash;
	sigaction(SIGHUP, &sv, NULL);

#ifdef DUMPSTATUS
	sv.sa_handler = dump_status;
	sigaction(SIGUSR2, &sv, NULL);
#endif /* DUMPSTATUS */

	sv.sa_handler = SIG_IGN;
	sigaction(SIGUSR1, &sv, NULL);
	sigaction(SIGPIPE, &sv, NULL);	/* Ignore SIGPIPEs for good. */

	umask(~S_IRUSR & ~S_IWUSR);	/* For logfile(s). */

	status.autojoindone = 0;	/* Haven't joined cfg.channels yet. */
	status.awaymsg = NULL;		/* No non-default away message. */
	status.awaystate = 0;		/* No custom awaymsg | not away. */

	status.got_nick = 1;		/* Assume everything's fine. */
	status.getting_nick = 0;	/* Not getting the nick right now. */
	status.allowconnect = 1;	/* We're listening for clients. */
#ifdef UPTIME
	status.startup = 0;		/* Uptime set to zero. */
#endif /* UPTIME */

	/* We try the first nick when connecting anyway. */
	nicknames.next = NICK_NEXT;
	/* Start with first real server immediately. */
	i_server.current = servers.servers.head->next;
	timers.connect = cfg.reconnectdelay;
	timers.good_server = 0;
	status.good_server = 0;
	status.goodhostname = 0;

#ifdef AUTOMODE
	status.automodes = 0;
#endif /* AUTOMODE */

	srand(time(NULL));

	create_listen();

#ifdef INBOX
	inbox = fopen(FILE_INBOX, "a+");
	if (inbox == NULL) {
		report(MIAU_ERRINBOXFILE);
	}
#endif /* INBOX */
} /* void init() */



/*
 * Check configuration and report errors.
 *
 * Returns number of errors.
 */
int
check_config(
	    )
{
	int err = 0;
#define REPORTERROR(x) { error(PARSE_MK, x); err++; }
	
	if (! nicknames.nicks.head)	REPORTERROR("nicknames");
	if (! cfg.realname)		REPORTERROR("realname");
	if (! cfg.username)		REPORTERROR("username");
	if (! cfg.password)		REPORTERROR("password");
	if (! cfg.listenport)		REPORTERROR("listenport");
	if (! connhostlist.list.head)	REPORTERROR("connhosts");
	/*
	 * Actually, we did we do this?
	 * 
	if (! cfg.listenhost && cfg.bind) cfg.listenhost = strdup(cfg.bind);
	 */

	server_check_list();

	if (servers.amount == 1) {
		err++;
	} else {
		status.reconnectdelay = cfg.reconnectdelay;
	}

	return err;
} /* int check_config() */



/*
 * Setup home and create directories if they don't exist already.
 */
void
setup_home(
		char	*s
	  )
{
	struct stat ds;
	int t;

	if (s) {
		cfg.home = strdup(s);
		if (cfg.home[strlen(cfg.home) - 1] != '/') {
			cfg.home = xrealloc(cfg.home, strlen(cfg.home) + 2);
			strcat(cfg.home, "/");
		}
	}
	
	else {
		if (! (s = getenv("HOME"))) {
			error(MIAU_ERRNOHOME);
			escape();
		}
		
		cfg.home = xmalloc(strlen(s) + strlen(MIAUDIR) + 2);
		xstrcpy(cfg.home, s);
		
		if (cfg.home[strlen(cfg.home) - 1] != '/') {
			strcat(cfg.home, "/");
		}
		strcat(cfg.home, MIAUDIR);
	}
	
	if (chdir(cfg.home) < 0) {
		error(MIAU_ERRCHDIR, cfg.home);
		escape();
	}

	/* Create directories. */
	/* If we need more directories, make new function of this routine. */
	t = stat(LOGDIR, &ds);
	if (t == 0) {
		/* Exists. */
		if (! S_ISDIR(ds.st_mode)) {
			error(MIAU_ERRLOGDIR, LOGDIR);
			escape();
		}
	} else {
		if (mkdir(LOGDIR, 0700) == -1) {
			error(MIAU_ERRCREATELOGDIR, LOGDIR);
			escape();
		}
	}
} /* void setup_home(char *) */



int
main(
		int	paramc,
		char	*params[]
    )
{
	int	pid = 0;
	FILE	*pidfile;
	char	*miaudir = 0;
	int	dofork = 1;
	int	c;
#ifdef MKPASSWD
	char	salt[3];
#endif

	fprintf(stdout, "%s%s", BANNER1, BANNER2);

	opterr = 0;

	while ((c = getopt(paramc, params, ":cfd:")) > 0) {
		switch( c ) {
#ifdef MKPASSWD
		        case 'c':
		            srand(time(NULL));
        		    randname(salt, NULL, 2);
		            printf(MIAU_THISPASS, crypt(getpass(MIAU_ENTERPASS),
						    salt));
				return 0;
				break;
#endif /* MKPASSWD */
			case 'd':
				miaudir = optarg;
				break;
			case 'f':
				dofork = 0;
				break;
			case ':':
				error(MIAU_ERRNEEDARG, optopt);
				escape();
				break;
			default:
				printf(SYNTAX, params[0]);
				return 1;
				break;
		}
	}

	pre_init();

	setup_home(miaudir);

	read_cfg();
	if (check_config() != 0) {
		escape();
	}

	command_setup();

	init();

	if (dofork) {
		pid = fork();
		if (pid < 0) {
			error(MIAU_ERRFORK);
			escape();
		}
		
		if (pid == 0) {
			if (! freopen(FILE_LOG, "a", stdout)) {
				error(MIAU_ERRFILE, cfg.home);
				escape();
			}
#ifndef SETVBUF_REVERSED
			setvbuf(stdout, NULL, _IONBF, 0);
#else
			setvbuf(stdout, _IONBF, NULL, 0);
#endif
			printf("\n");
			report(MIAU_NEWSESSION);
			report(MIAU_STARTINGLOG);
			if (cfg.listenhost) {
				report(SOCK_LISTENOKHOST, cfg.listenhost,
						cfg.listenport);
			} else {
				report(SOCK_LISTENOK, cfg.listenport);
			}
			report(MIAU_NICK, (char *) nicknames.nicks.head->data);
		}
	
		else {		/* pid != 0 */
			if (! (pidfile = fopen(FILE_PID, "w"))) {
				error(MIAU_ERRFILE, cfg.home);
				kill(pid, SIGTERM);
				escape();
			}
			fprintf(pidfile, "%d\n", pid);
			fclose(pidfile);
			report(MIAU_NICK, (char *) nicknames.nicks.head->data);
			report(MIAU_FORKED, pid);
			exit(0);
		}
	}
	run();
	return 0;
} /* int main(int, char *[]) */
