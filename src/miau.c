/*
 * -------------------------------------------------------
 * Copyright (C) 2002-2007 Tommi Saviranta <wnd@iki.fi>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include "miau.h"
#include "conntype.h"
#include "perm.h"
#include "qlog.h"
#include "irc.h"
#include "commands.h"
#include "error.h"
#include "messages.h"
#include "parser.h"
#include "tools.h"
#include "ascii.h"
#include "onconnect.h"
#include "chanlog.h"
#include "privlog.h"
#include "automode.h"
#include "ignore.h"
#include "common.h"
#include "dcc.h"
#include "etc.h"
#include "log.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#if HAVE_CRYPT_H
#include <crypt.h>
#endif /* ifdef HAVE_CRYPT_H */



#ifdef INBOX
FILE	*inbox = NULL;
#endif /* ifdef INBOX */


static int read_newclient(void);
static int check_config(void);

static void fakeconnect(connection_type *newclient);
static void sig_term(int a);
static void connect_timeout(int a);
static void create_listen(void);
static void rehash(int a);
static void run(void);
static void pre_init(void);
static void init(void);

static void read_cfg(void);
static void check_timers(void);
static void miau_welcome(void);
static int proceed_timer(int *timer, const int warn, const int exceed);
static int proceed_timer_safe(int *timer, const int warn, const int exceed,
		const int repeat);
static void escape(void);
static void setup_atexit(void);

static void setup_home(char *s);

#ifdef DUMPSTATUS
void dump_status(int a);
#endif /* ifdef DUMPSTATUS */


status_type 		status;
cfg_type cfg = {
	1,	/* statelog: write stdout into file */
#ifdef QUICKLOG
	30,	/* qloglength: 30 minutes */
	-1,	/* autoqlog: full quicklog */
#ifdef QLOGSTAMP
	0,	/* timestamp: no timestamp */
#endif /* ifdef QLOGSTAMP */
	1,	/* flushqlog: flush */
#endif /* ifdef QUICKLOG */
#ifdef DCCBOUNCE
	0,	/* dccbounce: no */
#endif /* ifdef DCCBOUNCE */
#ifdef AUTOMODE
	30,	/* automodedelay: 30 seconds */
#endif /* ifdef AUTOMODE */
#ifdef INBOX
	1,
#endif /* ifdef INBOX */
	0,	/* listenport: 0 */
	2,	/* floodtimer */
	5,	/* burstsize */
	30,	/* jointries */
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
	2,	/* chandiscon: part */
	9,	/* maxnicklen: 9 chars */
	3,	/* maxclients: 3 clients */
	1,	/* usequitmsg: yes */
	1,	/* autoaway: detach */
#ifdef PRIVLOG
	0,	/* privlog: no privlog */
#endif /* ifdef PRIVLOG */

	DEFAULT_NICKFILL,	/* nickfillchar */
	
#ifdef NEED_LOGGING
	NULL,			/* logsuffix */
#endif /* ifdef NEED_LOGGING */
#ifdef DCCBOUNCE
	NULL,			/* dccbindhost */
#endif /* ifdef DCCBOUNCE */
#ifdef NEED_CMDPASSWD
	NULL,			/* cmdpasswd */
#endif /* ifdef NEED_CMDPASSWD */
	NULL,	/* username */
	NULL,	/* realname */
	NULL,	/* password */
	NULL,	/* leavemsg */
	NULL,	/* bind */
	NULL,	/* listenhost */
	NULL,	/* awaymsg */
	NULL,	/* forwardmsg */
	180,	/* forwardtime */
	NULL,	/* channels */
	NULL,	/* home */
	NULL,	/* usermode */

	0,	/* no_idmsg_capab */
	NULL,	/* privmsg_fmt */
	0,	/* newserv_disconn: none. see enum in miau.h */
};
nicknames_type		nicknames;

permlist_type		connhostlist;
permlist_type		ignorelist;
timer_type		timers;
#ifdef AUTOMODE
extern permlist_type	automodelist;
extern llist_list	*tobeautomode;
#endif /* ifdef AUTOMODE */

extern llist_list	active_channels;
extern llist_list	passive_channels;


client_info	i_newclient;
connection_type	c_newclient;


int		listensocket = 0;  	/* listensocket */
char		*forwardmsg = NULL;
int		forwardmsgsize = 0;

int		error_code;		/* Used for EXIT-macro. Ugly. */



#ifdef PINGSTAT
int	ping_sent = 0;
int	ping_got = 0;
#endif /* ifdef PINGSTAT */



/*
 * Free some resources.
 * Only free those that are _not_ must-haves (except nicknames).
 *
 * Should be called from escape() and from rehash().
 */
static void
free_resources(void)
{
	LLIST_EMPTY(nicknames.nicks.head, &nicknames.nicks);
	empty_perm(&ignorelist);
	empty_perm(&connhostlist);
#ifdef AUTOMODE
	empty_perm(&automodelist);
#endif /* ifdef AUTOMODE */
#ifdef ONCONNECT
	LLIST_EMPTY(onconnect_actions.head, &onconnect_actions);
#endif /* ifdef ONCONNECT */
	
	FREE(cfg.username);
	FREE(cfg.realname);
	FREE(cfg.password);

	FREE(cfg.privmsg_fmt);
	FREE(cfg.bind);
	FREE(cfg.usermode);
	FREE(cfg.awaymsg);
	FREE(cfg.leavemsg);
	FREE(cfg.listenhost);
	FREE(cfg.channels);
	FREE(cfg.forwardmsg);
	FREE(cfg.channels);
#ifdef DCCBOUNCE
	FREE(cfg.dccbindhost);
#endif /* ifdef DCCBOUNCE */
#ifdef CHANLOG
	chanlog_del_rules();
#endif /* ifdef CHANLOG */

#ifdef QUICKLOG
	/* flush all of qlog */
	qlog_flush(time(NULL), 0);
#endif /* ifdef QUICKLOG */
	FREE(status.awaymsg);
} /* static void free_resources(void) */



/*
 * Finish things up and quit miau.
 */
static void
escape(void)
{
	int		n;

	/* Clear send queue. */
	irc_clear_queue();

#ifdef PRIVLOG
	privlog_close_all();
#endif /* ifdef PRIVLOG */

	/* Close connections and free client list. */
	rawsock_close(listensocket);
	sock_close(&c_server);
	sock_close(&c_newclient);

	LLIST_WALK_H(c_clients.clients->head, connection_type *);
		sock_close(data);
		llist_delete(node, c_clients.clients);
		xfree(data);
	LLIST_WALK_F;
	c_clients.connected = 0;

	/* Close log-file and remove PID-file. */
#ifdef INBOX
	if (inbox != NULL) {
		fclose(inbox);
	}
#endif /* ifdef INBOX */
	unlink(FILE_PID);

	/* Free permission lists. */
	empty_perm(&connhostlist);
#ifdef AUTOMODE
	empty_perm(&automodelist);
#endif /* ifdef AUTOMODE */

	/* Free server-info. */
	xfree(i_server.realname);
	for (n = 0; n < RPL_MYINFO_LEN; n++) {
		xfree(i_server.greeting[n]);
	}
	for (n = 0; n < RPL_ISUPPORT_LEN; n++) {
		xfree(i_server.isupport[n]);
	}

	/* Free configuration parameters. */
	xfree(cfg.home);
#ifdef NEED_LOGGING
	xfree(cfg.logsuffix);
#endif /* ifdef NEED_LOGGING */
#ifdef NEED_CMDPASSWD
	xfree(cfg.cmdpasswd);
#endif /* ifdef NEED_CMDPASSWD */

	xfree(forwardmsg);

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
	
	/* We're done. */
	error(MIAU_ERREXIT);
} /* static void escape(void) */



/*
 * (Re)read configuration file.
 */
static void
read_cfg(void)
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
	
	/* Read configuration file. */
	ret = parse_cfg(MIAURC);
	if (ret == -1) {
		error(MIAU_ERRCFG, cfg.home);
		exit(ERR_CODE_CONFIG);
	}
#ifdef NEED_LOGGING
	/* cfg.logsuffix _must_ be set */
	if (cfg.logsuffix == NULL) {
		cfg.logsuffix = xstrdup("");
	}
#endif /* ifdef NEED_LOGGING */

	report(MIAU_READ_RC);
} /* static void read_cfg(void) */



static void
sig_term(int a)
{
	server_drop(MIAU_SIGTERM);
	error(MIAU_SIGTERM);
	exit(EXIT_SUCCESS);
} /* static void sig_term(int a) */


#ifdef DUMPSTATUS
static char	*dumpdata;
static int	foocount = 0;

static void
dump_add(char *data)
{
	int addlen;
	int dumplen;
	addlen = strlen(data);
	dumplen = strlen(dumpdata) + addlen;
	dumpdata = (char *) xrealloc(dumpdata, dumplen + 1);
	strncat(dumpdata, data, addlen);
	dumpdata[dumplen] = '\0';
	foocount += addlen;
} /* static void dump_add(char *data) */

static void
dump_dump(void)
{
	if (dumpdata[0] != '\0') {
		fprintf(stderr, "%s\n", dumpdata);
		irc_mnotice(&c_clients, status.nickname, dumpdata);
		dumpdata[0] = '\0';
		foocount = 0;
	}
} /* static void dump_dump(void) */

static void
dump_finish(void)
{
	if (dumpdata[0] != '\0') {
		dump_dump();
	}
} /* static void dump_finish(void) */

static void
dump_status_int(const char *id, const int val)
{
	char buf[IRC_MSGLEN];
	snprintf(buf, IRC_MSGLEN, "    %s=%d", id, val);
	buf[IRC_MSGLEN - 1] = '\0';
	if (foocount + strlen(buf) > 80) {
		dump_dump();
	}
	dump_add(buf);
} /* static void dump_status_int(const char *id, conat int val) */

static void
dump_status_char(const char *id, const char *val)
{
	char buf[IRC_MSGLEN];
	snprintf(buf, IRC_MSGLEN, "    %s='%s'", id, val != NULL ?
			val : "(nothing)");
	buf[IRC_MSGLEN - 1] = '\0';
	if (foocount + strlen(buf) > 80) {
		dump_dump();
	}
	dump_add(buf);
} /* static void dump_status_char(const char *id, const char *val) */

static void
dump_string(const char *data)
{
	fprintf(stderr, "%s\n", data);
	irc_mnotice(&c_clients, status.nickname, "%s", data);
} /* static void dump_string(const char *data) */

void
dump_status(int a)
{
	dumpdata = (char *) xmalloc(1);
	dumpdata[0] = '\0';
#ifdef QUICKLOG
	/* First check qlog. */
	qlog_check(cfg.qloglength * 60);
#endif /* ifdef QUICKLOG */
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
#ifdef QLOGSTAMP
	dump_status_int("timestamp", cfg.timestamp);
#endif /* ifdef QLOGSTAMP */
	dump_status_int("flushqlog", cfg.flushqlog);
#endif /* ifdef QUICKLOG */
#ifdef DCCBOUNCE
	dump_status_int("dccbounce", cfg.dccbounce);
	dump_status_char("dccbindhost", cfg.dccbindhost);
#endif /* ifdef DCCBOUNCE */
#ifdef AUTOMODE
	dump_status_int("automodedelay", cfg.automodedelay);
#endif /* ifdef AUTOMODE */
#ifdef INBOX
	dump_status_int("inbox", cfg.inbox);
#endif /* ifdef INBOX */
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
	dump_status_int("chandiscon", cfg.chandiscon);
	dump_status_int("maxnicklen", cfg.maxnicklen);
	dump_status_int("autoaway", cfg.autoaway);
#ifdef NEED_LOGGING
	dump_status_char("logsuffix", cfg.logsuffix);
#endif /* ifdef NEED_LOGGING */
	dump_status_char("privmsg_fmt", cfg.privmsg_fmt);
	dump_status_int("newserv_disconn", cfg.newserv_disconn);
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
#endif /* ifdef UPTIME */
	dump_status_int("c_clients.connected", c_clients.connected);
	dump_dump();

	dump_string("active_channels:");
	LLIST_WALK_H(active_channels.head, channel_type *);
		dump_add("    {");
		dump_status_char("name", data->name);
		dump_status_int("name_set", data->name_set);
		dump_status_char("simple_name", data->simple_name);
		dump_status_int("simple_set", data->simple_set);
		dump_status_char("key", data->key);
#ifdef QUICKLOG
		dump_status_int("hasqlog", data->hasqlog);
#endif /* ifdef QUICKLOG */
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
#endif /* ifdef AUTOMODE */
		dump_add("    }"); dump_dump();
	LLIST_WALK_F;
	dump_dump();
	
	dump_string("passive_channels:");
	LLIST_WALK_H(passive_channels.head, channel_type *);
		dump_add("    {");
		dump_status_char("name", data->name);
		dump_status_int("name_set", data->name_set);
		dump_status_char("simple_name", data->simple_name);
		dump_status_int("simple_set", data->simple_set);
		dump_status_char("key", data->key);
		dump_status_int("jointries", data->jointries);
#ifdef AUTOMODE
		dump_status_int("oper", data->oper);
#endif /* idef AUTOMODE */
#ifdef QUICKLOG
		dump_status_int("hasqlog", data->hasqlog);
#endif /* idef QUICKLOG */
		dump_add("    }"); dump_dump();
	LLIST_WALK_F;
	dump_dump();

	dump_string("old_channels:");
	LLIST_WALK_H(old_channels.head, channel_type *);
		dump_add("    {");
		dump_status_char("name", data->name);
		dump_status_int("name_set", data->name_set);
		dump_status_char("simple_name", data->simple_name);
		dump_status_int("simple_set", data->simple_set);
		dump_status_char("key", data->key);
		dump_status_int("jointries", data->jointries);
#ifdef AUTOMODE
		dump_status_int("oper", data->oper);
#endif /* idef AUTOMODE */
#ifdef QUICKLOG
		dump_status_int("hasqlog", data->hasqlog);
#endif /* idef QUICKLOG */
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
#endif /* idef PRIVLOG */

#ifdef AUTOMODE
	dump_string("automodes:");
	LLIST_WALK_H(automodelist.list.head, automode_type *);
		dump_add("    {");
		dump_status_char("mask", data->nick);
		dump_status_int("true", (int) data->mode);
		dump_add("  }"); dump_dump();
	LLIST_WALK_F;
	dump_dump();
#endif /* idef AUTOMODE */

	/*
	dump_string("forwardmsg:");
	dump_string(forwardmsg ? forwardmsg : "-");
	*/

	dump_finish();
	xfree(dumpdata);
} /* void dump_status(int a) */
#endif /* idef DUMPSTATUS */



void
drop_newclient(char *reason)
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
} /* void drop_newclient(char *reason) */



/*
 * Move timer.
 *
 * Wrapper for proceed_timer_safe with 0 as last parameter.
 */
static int
proceed_timer(int *timer, const int warn, const int exceed)
{
	return proceed_timer_safe(timer, warn, exceed, 0);
} /* static int proceed_timer(int *timer, const int warn, const int exceed) */



/*
 * Move timer.
 *
 * Return 1 if timer > warn, 2 if timer > exceed, otherwise 0.
 */
static int
proceed_timer_safe(int *timer, const int warn, const int exceed,
		const int repeat)
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
} /* static int proceed_timer_safe(int *timer, const int warn, const int exceed,
		const int repeat) */


/*
 * Create socket for listening clients.
 *
 * If old socket exists, close it. If something goes wrong, go down, hard.
 */
static void
create_listen(void)
{
	if (listensocket) {
		rawsock_close(listensocket);
	}

	/* Create listener. */
	listensocket = sock_open();
	if (listensocket == -1) {
		error(SOCK_ERROPEN, net_errstr);
		exit(ERR_CODE_NETWORK);
	}

	if (! sock_bind(listensocket, cfg.listenhost, cfg.listenport)) {
		if (cfg.listenhost) {
			error(SOCK_ERRBINDHOST, cfg.listenhost,
					cfg.listenport, net_errstr);
		} else {
			error(SOCK_ERRBIND, cfg.listenport, net_errstr);
		}

		exit(ERR_CODE_NETWORK);
	}
	
	if (! sock_listen(listensocket)) {
		error(SOCK_ERRLISTEN);
		exit(ERR_CODE_NETWORK);
	}
	
	if (cfg.listenhost) {
		report(SOCK_LISTENOKHOST, cfg.listenhost, cfg.listenport);
	} else {
		report(SOCK_LISTENOK, cfg.listenport);
	}
} /* static void create_listen(void) */



/*
 * Function noting connect() timeouted. Called thru alert().
 */
static void
connect_timeout(int a)
{
	error(SOCK_ERRCONNECT, ((server_type *) i_server.current->data)->name,
			SOCK_ERRTIMEOUT);
	sock_close(&c_server);
} /* static void connect_timeout(int a) */



static void
create_dirs(void)
{
#ifdef NEED_LOGGING
	{
		struct stat ds;
		int t;
		int logdir;

		logdir = 0;
#ifdef CHANLOG
		if (chanlog_list.head != NULL || global_logtype != 0) {
			logdir = 1;
		}
#endif /* ifdef CHANLOG */
#ifdef PRIVLOG
		if (cfg.privlog != 0) {
			logdir = 1;
		}
#endif /* ifdef PRIVLOG */

		if (logdir == 1) {
			t = stat(LOGDIR, &ds);
			if (t == 0) {
				/* Exists. */
				if (! S_ISDIR(ds.st_mode)) {
					error(MIAU_ERRLOGDIR, LOGDIR);
					exit(ERR_CODE_HOME);
				}
			} else {
				if (mkdir(LOGDIR, 0700) == -1) {
					error(MIAU_ERRCREATELOGDIR, LOGDIR);
					exit(ERR_CODE_HOME);
				}
			}
		}
	}
#endif /* ifdef NEED_LOGGING */
}



/*
 * Reread configuration file.
 *
 * Clear lists but don't touch settings aleady set. Perhaps this isn't a good
 * idea and we should reset all values to default...
 *
 * Parameter for signal handler, value ignored.
 */
static void
rehash(int sigparam)
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
	char		*oldbind;
	int		oldlistenport;
	char		*oldlistenhost;
	llist_node	*node;
	int host_changed;
	int bind_changed;

	/* First backup some essential stuff. */
	oldrealname = xstrdup(cfg.realname);
	oldusername = xstrdup(cfg.username);
	oldpassword = xstrdup(cfg.password);
	oldlistenport = cfg.listenport;
	oldlistenhost = (cfg.listenhost != NULL)
		? xstrdup(cfg.listenhost) : NULL;
	oldbind = (cfg.bind != NULL) ? xstrdup(cfg.bind) : NULL;

	/* Free non-must parameters. */
	free_resources();
	cfg.listenport = 0;
	
#ifdef CHANLOG
	/* Close open logfiles. */
	LLIST_WALK_H(active_channels.head, channel_type *);
		chanlog_close(data);
	LLIST_WALK_F;
	global_logtype = 0;	/* No logging by default. */
#endif /* idef CHANLOG */
	
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
	if (cfg.realname == NULL) { cfg.realname = xstrdup(oldrealname); }
	if (cfg.username == NULL) { cfg.username = xstrdup(oldusername); }
	if (cfg.password == NULL) { cfg.password = xstrdup(oldpassword); }
	if (cfg.listenport == 0) { cfg.listenport = oldlistenport; }
	
#ifdef CHANLOG
	/* Open logs. */
	LLIST_WALK_H(active_channels.head, channel_type *);
		chanlog_open(data);
	LLIST_WALK_F;
#endif /* idef CHANLOG */

#ifdef QUICKLOG
	/*
	 * Empty quicklog if flushqlog is set to true and we have clients
	 * connected or if qloglength is set to 0.
	 */
	if ((cfg.flushqlog == 1 && c_clients.connected > 0) ||
			cfg.qloglength == 0) {
		qlog_flush(time(NULL), 0);
	}
#endif /* idef QUICKLOG */

	/* By default, we don't know anything about server. */
	servers.fresh = 1;

	/*
	 * If there was no nicks defined in configuration-file, save current
	 * nick to the list. Warnings have been send already.
	 */
	if (nicknames.nicks.head == NULL) {
		node = llist_create(xstrdup(status.nickname));
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
	if (cfg.listenhost == NULL || oldlistenhost == NULL) {
		if (cfg.listenhost != oldlistenhost) {
			host_changed = 1;
		} else {
			host_changed = 0;
		}
	} else {
		host_changed = xstrcmp(oldlistenhost, cfg.listenhost);
	}
	if (oldlistenport != cfg.listenport || host_changed != 0) {
		create_listen();
	}

	/* Bind address changed. */
	if (cfg.bind == NULL || oldbind == NULL) {
		if (cfg.bind != oldbind) {
			bind_changed = 1;
		} else {
			bind_changed = 0;
		}
	} else {
		bind_changed = xstrcmp(oldbind, cfg.bind);
	}
	if (bind_changed != 0) {
		server_drop(MIAU_RECONNECT);
		server_change(1, 0); /* reconnect to current */
	}

#ifdef INBOX
	/* We're logging no more. */
	if (cfg.inbox == 0 && inbox != NULL) {
		fclose(inbox);
		inbox = NULL;
	}
	
	/* We should start logging. */
	if (cfg.inbox == 1 && inbox == NULL) {
		inbox = fopen(FILE_INBOX, "a+");
		if (inbox == NULL) {
			report(MIAU_ERRINBOXFILE);
		}
	}
#endif /* ifdef INBOX */

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
	if (xstrcmp(oldrealname, cfg.realname) != 0
			|| xstrcmp(oldusername, cfg.username) != 0) {
		server_drop(MIAU_RECONNECT);
		report(MIAU_RECONNECT);
		i_server.current--;
		status.reconnectdelay = cfg.reconnectdelay;
		timers.connect = cfg.reconnectdelay - 3;
		server_change(1, 0);
	}

	/* Free backuped stuff. */
	xfree(oldrealname);
	xfree(oldpassword);
	xfree(oldusername);
	xfree(oldlistenhost);
	xfree(oldbind);

	create_dirs();
} /* static void rehash(int a) */



/*
 * Client disconnected from bouncer.
 *
 * If appropriate, leave channels and set user away.
 */
/* TODO: Oh man this function and client_drop are a mess together! */
void
clients_left(const char *reason)
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

	chans = (char *) xcalloc(1, 1);
	
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
		int nlen;
		/* Build a string of channel list. */
		LLIST_WALK_H(active_channels.head, channel_type *);
			/* paranoid */
			nlen = strlen(data->name);
			chans = (char *) xrealloc(chans, nlen + 2
					+ (int) strlen(chans));
			strcat(chans, ",");
			strncat(chans, data->name, nlen);
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
#endif /* ifdef AUTOMODE */
					llist_add_tail(llist_create(data),
							&passive_channels);
				} else {
					/*
					 * Not moving channels from list to
					 * list, therefore freeing
					 * resources.
					 */
					channel_free(data);
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
#endif /* ifdef CHANLOG */
		
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
		else if (cfg.leave == 1 && (cfg.leavemsg != NULL
					|| reason != NULL)) {
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

	if (cfg.autoaway != 0) {
		/* Try setting user away with given message. */
		set_away(awaymsg);
	}
} /* void clients_left(const char *reason) */



/*
 * Check timers.
 */
static void
check_timers(void)
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
	 * If cfg.floodtimer second(s) have elapsed (since last allowance) allow
	 * sending anohter message. Don't forget to limit the counter to
	 * cfg.burstsize.
	 */
	if (newtime - floodtime >= cfg.floodtimer && msgtimer < cfg.burstsize) {
		if (cfg.floodtimer != 0) {
			msgtimer += (newtime - floodtime) / cfg.floodtimer;
			if (msgtimer > cfg.burstsize) {
				msgtimer = cfg.burstsize;
			}
		} else {
			/*
			 * cfg.floodtimer == 0, send cfg.burstsize
			 * lines at once.
			 */
			msgtimer = cfg.burstsize;
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

#ifdef NEED_PROCESS_IGNORES
	ignores_process();
#endif /* ifdef NEED_PROCESS_IGNORES */

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
				client_drop(client_con, CLNT_STONED,
						DISCONNECT_ERROR, 1, NULL);
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
		int timeout = (i_server.connected == 2) ?
			cfg.stonedtimeout : 120;
		int warn;

		/*
		 * If we have already registered our nick, timeout is
		 * configured in miaurc (default 90 seconds). If we are
		 * still registering our nick, timeout is 120 seconds.
		 *
		 * Thanks to netsplits we cannot simply wait until, say, 30
		 * seconds before timeout, but we'll have to contantly ping the
		 * server... I hope pinging (at least) every 45 seconds is ok.
		 */
		warn = (timeout / 2 > 45) ? 45 : timeout / 2;
		if (warn < 5) {
			warn = 5;
		}
		switch (proceed_timer_safe(&c_server.timer, warn, timeout, 5)) {
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
#ifdef ENDUSERDEBUG
				if (c_clients.connected > 0
						&& c_server.timer >
						(timeout - 20 >= 5 ?
						timeout - 20 : 5)) {
					enduserdebug("pinging server (%d/%d)",
							c_server.timer,
							timeout);
				}
#endif /* ifdef ENDUSERDEBUG */
#endif /* ifdef PINGSTAT */
				break;
			case 2:
				server_drop(SERV_STONED);
				error(SERV_STONED);
				server_change(1, 0);
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
		 * Let's just try joining unjoined channels once a minute
		 * -- unless we're done trying of course.
		 */
		if (proceed_timer(&timers.join, 0, JOINTRYINTERVAL) == 2) {
			channel_join_list(LIST_PASSIVE, 0, NULL);
		}
	}			

	/* Handle forwarded messages. */
	if (cfg.forwardmsg) {
		switch (proceed_timer(&timers.forward, 0, cfg.forwardtime)) {
			case 0:
			case 1:
				break;
			case 2:
				if (forwardmsg == NULL) {
					/* Nothing to forward. */
					break;
				}

				/* yup, popen isn't safe */
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
					xfree(forwardmsg);
					forwardmsg = NULL;
					/* redundant, but makes it clearer */
					forwardmsgsize = 0;
				}
				
				break;
		}
	}
	
#ifdef AUTOMODE
	/* Act as op-o-matic. */
	if (status.automodes > 0 && proceed_timer(&timers.automode,
				0, cfg.automodedelay) == 2) {
		/* automode() checks if we have operator-status. */
		automode_do();
	}
#endif /* ifdef AUTOMODE */

#ifdef PRIVLOG
	/* Close logfiles that haven't been used for a while. */
	if (privlog_has_open() && proceed_timer(&timers.privlog,
				0, PRIVLOG_CHECK_PERIOD)) {
		privlog_close_old();
	}
#endif /* ifdef PRIVLOG */
#ifdef NEED_LOGGING
	/* Reset timer for repeating warnings about logfiles. */
	if (proceed_timer(&timers.logfile_warn, 0, LOGWARN_TIMER)) {
		log_reset_warn_timer();
	}
#endif /* ifdef NEED_LOGGING */

#ifdef DCCBOUNCE
	/* Bounce DCCs. */
	if (cfg.dccbounce) {
		dcc_timer();
	}
#endif /* ifdef DCCBOUNCE */
} /* static void check_timers(void) */



void
get_nick(char *format)
{
	/*
	 * Change nick if we're still introducing outself to the server or if
	 * we were forced to change ...
	 */
	if (i_server.connected != 2) {
		char *badnick;
		badnick = xstrdup(status.nickname);
		/* if (badnick == NULL) { escape(); } */

		/* Try first nick. */
		if (nicknames.next == NICK_FIRST) {
			nicknames.current = nicknames.nicks.head;
			nicknames.next = NICK_NEXT;
			nicknames.gen_tries = 0;
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
					xstrdup(nicknames.current->data);
			}
		}
		/*
		 * else if (nicknames.current == NULL &&
		 *		cfg.nickfillchar != '\0') {
		 */
		else if (nicknames.current == NULL) {
			/* create a nick */
			nicknames.next = NICK_GEN;

			if (nicknames.gen_tries == 0) {
				status.nickname =
					(char *) xrealloc(status.nickname,
							  cfg.maxnicklen + 1);
				strncpy(status.nickname,
						nicknames.nicks.head->data,
						cfg.maxnicklen);
				status.nickname[cfg.maxnicklen] = '\0';
			}
			if (nicknames.gen_tries == cfg.maxnicklen * 2) {
				/* No luck rotating. Try all random. */
				/* len * 2 is overkill but enough. ;-) */
				status.nickname[0] = '\0';
			} else {
				nicknames.gen_tries++;
			}
			status.nickname = xrealloc(status.nickname,
					cfg.maxnicklen + 1);
			randname(status.nickname, cfg.maxnicklen,
					cfg.nickfillchar);
		} else {
			xfree(status.nickname);
			status.nickname = (char *) xstrdup((char *)
					nicknames.current->data);
		}

		report(format, badnick, status.nickname);
		/* Getting nick, increase sent NICK-command counter. */
		status.getting_nick++;
		irc_write(&c_server, "NICK %s", status.nickname);
		
		xfree(badnick);
	}
	status.got_nick = 0;
} /* void get_nick(char *format) */



/*
 * User has connected to bouncer.
 */
static void
fakeconnect(connection_type *newclient)
{
	int		i;
#ifdef ASCIIART
	int		pic;
#endif /* ifdef ASCIIART */

	if (status.nickname == NULL) {
		/* Get us a nick if we don't have one already. */
		status.nickname = xstrdup((char *) nicknames.nicks.head->data);
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

		/*
		 * Don't take it for granted that we would get all these...
		 * Network could be broken you know.
		 */
		for (i = 1; i < 4; i++) {
			if (i_server.greeting[i] != NULL) {
				irc_write(newclient,
						":%s %03d %s %s",
						i_server.realname,
						i + 1,
						status.nickname,
						i_server.greeting[i]);
			}
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
#endif /* ifdef ASCIIART */

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
#ifdef QUICKLOG
	/* Move old qlog-lines to privmsglog. */
	qlog_flush(time(NULL) - cfg.qloglength * 60, 1);
#endif /* ifdef QUICKLOG */
	if (inbox != NULL && ftell(inbox) != 0) {
		irc_write(newclient, ":%s 372 %s :- "CLNT_HAVEMSGS,
				i_server.realname,
				status.nickname);
	} else {
		irc_write(newclient, ":%s 372 %s :- "CLNT_INBOXEMPTY,
				i_server.realname,
				status.nickname);
	}
#endif /* ifdef INBOX */

	irc_write(newclient, ":%s 376 %s :"MIAU_END_OF_MOTD,
			i_server.realname,
			status.nickname);

	if (i_server.connected == 2) {
		/* tell client to change nick. yes, case sensitive. */
		if (xstrcmp(i_client.nickname, status.nickname) != 0) {
			irc_write(newclient, ":%s NICK :%s",
					i_client.nickname,
					status.nickname);
			xfree(i_client.nickname);
			i_client.nickname = xstrdup(status.nickname);
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
#ifdef QUICKLOG
		int qlog;
		if (cfg.autoqlog == -1) {
			qlog = cfg.qloglength;
		} else {
			qlog = cfg.autoqlog;
		}
		
		if (qlog > 0) {
			qlog_check(qlog * 60);
		}
#endif /* QUICKLOG */

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

#ifdef QUICKLOG
#endif /* ifdef QUICKLOG */

		/*
		 * Next, tell client to join active channels.
		 *
		 * Active channels are channels that we currently are on.
		 *
		 * This means this list won't get flushed either.
		 */
		channel_join_list(LIST_ACTIVE, 0, newclient);

#ifdef QUICKLOG
		if (qlog > 0) {
			/* print headers, replay log, print footer */
			qlog_replay_header(newclient);

			qlog_replay(newclient, qlog * 60);

			qlog_replay_footer(newclient);

			if (cfg.flushqlog == 1) {
				qlog_flush(time(NULL), 0);
			}
		}
#endif /* ifdef QUICKLOG */
	}
} /* static void fakeconnect(connection_type *newclient) */



/*
 * Say hello to bouncer.
 *
 * Some IRC-clients take server's name from this message - this is why we
 * send them this welcome-message when jumping off the server.
 */
static void
miau_welcome(void)
{
	irc_mwrite(&c_clients, ":miau 001 %s :%s %s!user@host",
			status.nickname,
			MIAU_WELCOME,
			status.nickname);
} /* static void miau_welcome(void) */



/*
 * Set user away if...
 */
void
set_away(const char *msg)
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
		status.awaymsg = xstrdup(msg);
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
} /* void set_away(const char *msg) */



static int
read_newclient(void)
{
	llist_node	*node;
	char		*command;
	char		*param;
	int t;

	t = irc_read(&c_newclient);
	
	if (t <= 0) {
		return t;
	}
	
	c_newclient.buffer[30] = 0;
	command = strtok(c_newclient.buffer, " ");
	param = strtok(NULL, "\0");

	if (command == NULL || param == NULL) {
		return 0;
	}

	upcase(command);
	if ((xstrcmp(command, "PASS") == 0) && !(status.init & 1)) {
		/* Only accept password once */
		if (*param == ':') {
			param++;
		}
		if (strlen(cfg.password) == 13) {
			/* Assume it's crypted */
			char *crypted;
			crypted = crypt(param, cfg.password);
			if (xstrcmp(crypted, cfg.password) == 0) {
				status.passok = 1;
			}
		} else {
			if (xstrcmp(param, cfg.password) == 0) {
				status.passok = 1;
			}
		}

		status.init = status.init | 1;
	}

	if (xstrcmp(command, "NICK") == 0) {
		status.init = status.init | 2;
		xfree(i_newclient.nickname);
		i_newclient.nickname = xstrdup(
				strtok(param, " "));
	}

	if (xstrcmp(command, "USER") == 0) {
		status.init = status.init | 4;
		xfree(i_newclient.username);
		i_newclient.username = xstrdup(
				strtok(param, " "));
	}

	if (status.init == 7 && status.passok == 1) {
		connection_type	*newclient;

		/* Client is in! */
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

		newclient = (connection_type *)
			xmalloc(sizeof(connection_type));
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
			report(CLNT_CLIENTS, c_clients.connected);
		}
	}

	return 0;
} /* static int read_newclient(void) */



#ifdef QUICKLOG
static time_t
get_time(char *param)
{
	char *ptr;
	int days, hours, minutes;

	if (param == NULL) {
		return 0;
	}

	days = hours = minutes = 0;
	ptr = strrchr(param, (int) ':');
	if (ptr == NULL) {
		minutes = atoi(param);
	} else {
		minutes = atoi(ptr + 1);
		*ptr = '\0';

		ptr = strrchr(param, (int) ':');
		if (ptr == NULL) {
			hours = atoi(param);
		} else {
			hours = atoi(ptr + 1);
			*ptr = '\0';

			days = atoi(param);
		}
	}

	return days * 1440 + hours * 60 + minutes;
} /* static time_t get_time(char *param) */
#endif /* ifdef QUICKLOG */



/*
 * Handle commands provided to miau from attached IRC-client.
 */
void
miau_commands(char *command, char *param, connection_type *client)
{
	int	corr = 0;
	int	i;

	/* No command at all ? */
	if (command == NULL) {
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

	if (xstrcmp(command, "REHASH") == 0) {
		corr++;
		rehash(0);
	}

#ifdef INBOX
	if (xstrcmp(command, "READ") == 0) {
		corr++;
		if (inbox != NULL && ftell(inbox) != 0) {
			char s[1024];
			char *r;
			fflush(inbox);
			rewind(inbox);

			irc_notice(client, status.nickname, CLNT_INBOXSTART);

			while (1) {
				size_t size;
				r = fgets(s, 1023, inbox);
				if (r == NULL) {
					break;
				}
				size = strlen(s);
				if (s[size - 1] == '\n') {
					s[size - 1] = '\0';
				}
				s[1023] = '\0';
				irc_notice(client, status.nickname, "%s", s);
			}

			irc_notice(client, status.nickname, CLNT_INBOXEND);
			fseek(inbox, 0, SEEK_END);
		}
		else {
			irc_notice(client, status.nickname, CLNT_INBOXEMPTY);
		}
	}

	else if (xstrcmp(command, "DEL") == 0) {
		corr++;
		if (inbox != NULL) {
			fclose(inbox);
		}
		unlink(FILE_INBOX);
		irc_notice(client, status.nickname, CLNT_KILLEDMSGS);

		if (cfg.inbox == 1) {
			inbox = fopen(FILE_INBOX, "w+");
			if (inbox == NULL) {
				report(MIAU_ERRINBOXFILE);
			}
		}
	}
#endif /* ifdef INBOX */

	else if (xstrcmp(command, "RESET") == 0) {
		corr++;
		irc_mnotice(&c_clients, status.nickname, MIAU_RESET);
		server_reset();
	}

#ifdef QUICKLOG
	else if (xstrcmp(command, "FLUSHQLOG") == 0) {
		time_t keep;

		corr++;

		keep = get_time(param);

		qlog_flush(time(NULL) - keep * 60, 0);
		if (keep == 0) {
			irc_notice(client, status.nickname, MIAU_FLUSHQLOGALL);
		} else {
			int days, hours, minutes;
			minutes = keep % 60;
			hours = (keep / 60) % 24;
			days = keep / 1440;
			
			irc_notice(client, status.nickname, MIAU_FLUSHQLOG,
					days, hours, minutes);
		}
	}

	else if (xstrcmp(command, "QUICKLOG") == 0) {
		time_t oldest;

		corr++;
		oldest = get_time(param);

		qlog_check(oldest * 60);
		qlog_replay_header(client);
		qlog_replay(client, oldest * 60);
		qlog_replay_footer(client);
	}
#endif /* ifdef QUICKLOG */

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
#endif /* ifdef PINGSTAT */

	
#ifdef FAKECMD
	/* Developement only. */
	else if (xstrcmp(command, "FAKECMD") == 0) {
		corr++;
		char	*tbackup;
		char	*torigin;
		char	*tcommand;
		char	*tparam1;
		char	*tparam2;
		int	tcommandno;

		tbackup = xstrdup(param);
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
#endif /* ifdef FAKECMD */


#ifdef UPTIME
	else if (xstrcmp(command, "UPTIME") == 0) {
		time_t now;
		int seconds, minutes, hours, days;
		
		corr++;
		time(&now);
		now -= status.startup;
		getuptime(now, &days, &hours, &minutes, &seconds),
		
		irc_notice(client, status.nickname, MIAU_UPTIME,
				days, hours, minutes, seconds);
	}
#endif /* ifdef UPTIME */
	
#ifdef DUMPSTATUS
	else if (xstrcmp(command, "DUMP") == 0) {
		corr++;
		dump_status(0);
	}
#endif /* ifdef DUMPSTATUS */

	else if (xstrcmp(command, "STONED") == 0) {
		server_drop("faked stoned server");
		error("faked stoned server");
		server_change(1, 0);
	}

	else if (xstrcmp(command, "JUMP") == 0) {
		corr++;
		/* Are there any other servers ? */
		if (servers.amount != 0) {
			server_drop(MIAU_JUMP);
			report(MIAU_JUMP);
			miau_welcome();

			if (param != NULL) {
				i = atoi(param);
				if (i < 1) {
					i = 1;
				}
				if (i >= servers.amount) {
					i = servers.amount - 1;
				}
				i_server.current = servers.servers.head;
				for (; i > 0; i--) {
					i_server.current =
						i_server.current->next;
				}
				((server_type *) i_server.current->data)
					->working = 1;
				/* we know what we're doing */
				servers.fresh = 0;
				server_change(0, 0);
			} else {
				/* Don't try this server again. :-) */
				status.good_server = 0;
				server_change(1,0);
			}

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
		char *reason;
		
		reason = (param == NULL) ?
			xstrdup(MIAU_DIE_CL) : xstrdup(param);
		/*
		 * If user issues a DIE-command, (s)he most likely knows why
		 * connection was lost. Therefore we don't need to tell clients
		 * what's going on.
		 */
		/* (all clients, reason, notice, echo, no) */
		drop_newclient(NULL);
		client_drop(NULL, reason, DISCONNECT_DYING, 1, NULL);
		server_drop(reason);
		xfree(reason);
		/*
		 * Finally free memory allocated by copy of user's command.
		 * (See client_free() for more info.
		 */
		client_free();
		exit(EXIT_SUCCESS);
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
} /* void miau_commands(char *command, char *param, connection_type *client) */



/*
 * Main loop.
 *
 * Checks if there's data coming from sockets etc.
 */
static void
run(void)
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
			status.nickname = xstrdup((char *)
					nicknames.nicks.head->data);
			status.got_nick = 1;
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
					exit(ERR_CODE_NETWORK);
					break;
			
				case CONN_LOOKUP:
					error(SOCK_ERRRESOLVE, server->name);
					server_drop(NULL);
					server_change(1, 1);
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
					exit(ERR_CODE_NETWORK);
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
					server_change(1, 1);
					break;
					
				case CONN_WRITE:
					error(SOCK_ERRWRITE, server->name);
					server_drop(NULL);
					server_change(1, 0);
					break;
			
				case CONN_OTHER:
					error(SOCK_GENERROR, net_errstr);
					exit(ERR_CODE_NETWORK);
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
#endif /* ifdef DCCBOUNCE */
	
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
					server_change(1, 0);
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
					int ret;
					ret = client_read(client_conn);
					if (ret < 0) {
						/* client closed connection */
						/*
						 * (single client, message,
						 * report, no echo, no)
						 */
						client_drop(client_conn,
								CLNT_DROPPED,
							DISCONNECT_REPORT, 0,
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
#endif /* ifdef DCCBOUNCE */
		}
		
		check_timers();
	}
} /* static void run(void) */



static void
pre_init(void)
{
	/* Initialize structure for nicknames. */
	nicknames.nicks.head = NULL;
	nicknames.nicks.tail = NULL;

	/* Initialize some client-structures. */
	c_clients.clients = (llist_list *) xmalloc(sizeof(llist_list));
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

#ifdef NEED_LOGGING
	cfg.logsuffix = xstrdup("");
#endif /* ifdef NEED_LOGGING */
} /* static void pre_init(void) */



static void
init(void)
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

	/* clear timers */
	memset(&timers, 0, sizeof(timers));

#ifdef DUMPSTATUS
	sv.sa_handler = dump_status;
	sigaction(SIGUSR2, &sv, NULL);
#endif /* ifdef DUMPSTATUS */

	sv.sa_handler = SIG_IGN;
	sigaction(SIGUSR1, &sv, NULL);
	sigaction(SIGPIPE, &sv, NULL);	/* Ignore SIGPIPEs for good. */

	atexit(client_free);		/* free temp memory for client_read */
	atexit(free_resources);		/* free mem taken by config etc. */
	atexit(command_free);		/* free mem taken by hash table */

	umask(~S_IRUSR & ~S_IWUSR);	/* For logfile(s). */

	status.autojoindone = 0;	/* Haven't joined cfg.channels yet. */
	status.awaymsg = NULL;		/* No non-default away message. */
	status.awaystate = 0;		/* No custom awaymsg | not away. */

	status.allowreply = 1;
	status.got_nick = 1;		/* Assume everything's fine. */
	status.getting_nick = 0;	/* Not getting the nick right now. */
	status.allowconnect = 1;	/* We're listening for clients. */
#ifdef UPTIME
	status.startup = 0;		/* Uptime set to zero. */
#endif /* ifdef UPTIME */

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
#endif /* ifdef AUTOMODE */

	i_server.realname = xstrdup("miau");

	srand(time(NULL));

	create_listen();

#ifdef INBOX
	if (cfg.inbox == 1) {
		inbox = fopen(FILE_INBOX, "a+");
		if (inbox == NULL) {
			report(MIAU_ERRINBOXFILE);
		}
	}
#endif /* ifdef INBOX */
} /* static void init(void) */



/*
 * Check configuration and report errors.
 *
 * Returns number of errors.
 */
static int
check_config(void)
{
	int err = 0;
#define REPORTERROR(x) { error(PARSE_MK, x); err++; }
	
	if (nicknames.nicks.head == NULL)	REPORTERROR("nicknames");
	if (cfg.realname == NULL)		REPORTERROR("realname");
	if (cfg.username == NULL)		REPORTERROR("username");
	if (cfg.password == NULL)		REPORTERROR("password");
	if (cfg.listenport == 0)		REPORTERROR("listenport");
	if (connhostlist.list.head == NULL)	REPORTERROR("connhosts");
	/*
	 * Actually, we did we do this?
	 * 
	if (! cfg.listenhost && cfg.bind) cfg.listenhost = xstrdup(cfg.bind);
	 */

	server_check_list();

	if (servers.amount == 1) {
		err++;
	} else {
		status.reconnectdelay = cfg.reconnectdelay;
	}

	return err;
} /* static int check_config(void) */



/*
 * Setup home and create directories if they don't exist already.
 */
static void
setup_home(char *s)
{
	if (s != NULL) {
		int hsize;
		hsize = strlen(s) + 2;
		cfg.home = xmalloc(hsize);
		snprintf(cfg.home, hsize, "%s/", s);
		cfg.home[hsize - 1] = '\0';
	}

	else {
		int hsize;
		/*
		 * If user attacks miau with environment variables then
		 * so be it! It's his choice and his choice alone.
		 */
		s = getenv("HOME");
		if (s == NULL) {
			error(MIAU_ERRNOHOME);
			exit(ERR_CODE_HOME);
		}

		hsize = strlen(s) + strlen(MIAUDIR) + 2;
		cfg.home = xmalloc(hsize);
		snprintf(cfg.home, hsize, "%s/%s", s, MIAUDIR);
		cfg.home[hsize - 1] = '\0';
	}

	if (chdir(cfg.home) < 0) {
		error(MIAU_ERRCHDIR, cfg.home);
		exit(ERR_CODE_HOME);
	}
} /* static void setup_home(char *s) */



static void
setup_atexit(void)
{
	int r;
	
	r = atexit(escape);
	if (r != 0) {
		error(ERR_CANT_ATEXIT);
		escape();
		exit(EXIT_FAILURE);
	}
} /* static void setup_atexit(void) */



int
main(int argc, char **argv)
{
	int	pid = 0;
	FILE	*pidfile;
	char	*miaudir;
	int	dofork = 1;
	int	c;

	printf("%s", BANNER);

	miaudir = NULL;
	opterr = 0;

	/*
	 * getopt could have an overflow problem. If user runs miau with
	 * evil parameters, that his problem!
	 */
	while (1) {
#ifdef MKPASSWD
		c = getopt(argc, argv, "cfd:v");
#else /* ifdef MKPASSWD */
		c = getopt(argc, argv, "fd:v");
#endif /* ifdef else MKPASSWD */
		if (c == -1) {
			break;
		}

		switch (c) {
#ifdef MKPASSWD
		        case 'c':
				{
					char salt[3];
					char *pass;
					char *crypted;
				
					salt[0] = '\0';
					srand(time(NULL));
					randname(salt, 2, ' ');

					/*
					 * "The function getpass returns a
					 * pointer to a static buffer", but
					 * valgrind says "still reachable" but
					 * leaked anyhow.
					 *
					 * As a bonus: getpass may not be safe,
					 * but once we run it, we exit!
					 */
					pass = getpass(MIAU_ENTERPASS);
					crypted = crypt(pass, salt);
					printf(MIAU_THISPASS, crypted);
				}

				return EXIT_SUCCESS;
				break;
#endif /* ifdef MKPASSWD */
			case 'd':
				miaudir = optarg;
				break;
			case 'f':
				dofork = 0;
				break;
			case 'v':
				exit(EXIT_SUCCESS);
			case ':':
				error(MIAU_ERRNEEDARG, optopt);
				exit(EXIT_FAILURE);
				break;
			default:
				printf("\n");
				printf(SYNTAX, argv[0]);
				return EXIT_FAILURE;
				break;
		}
	}

	pre_init();

	setup_home(miaudir);

	read_cfg();
	if (check_config() != 0) {
		exit(ERR_CODE_CONFIG);
	}

	command_setup();

	create_dirs();
	init();

	if (dofork == 1) {
		if (cfg.statelog == 1) {
			/*
			 * See if file can be written to. If freopen fails,
			 * stdout would be lost. This check is not fool-proof,
			 * if someone changes the permissions before we get
			 * to call freopen, we will still fail. Lets consider
			 * this effort good enough.
			 */
			FILE *f;
			f = fopen(FILE_LOG, "a");
			if (f == NULL) {
				error(MIAU_LOGNOWRITE, FILE_LOG);
				exit(ERR_CODE_LOG);
			}
			fclose(f);
		}

		pid = fork();
		if (pid < 0) {
			error(MIAU_ERRFORK);
			exit(EXIT_FAILURE);
		}
		
		if (pid == 0) {
			/*
			 * Redirect stdout in file unless requested
			 * otherwise. Useful to disable this if you're
			 * low on disk space.
			 */
			if (cfg.statelog == 1) {
				FILE *f;
				
				f = freopen(FILE_LOG, "a", stdout);
				if (f == NULL) {
					error(MIAU_ERRFILE, cfg.home);
					exit(ERR_CODE_HOME);
				}
#ifndef SETVBUF_REVERSED
				setvbuf(stdout, NULL, _IONBF, 0);
#else /* ifndef SETVBUF_REVERSED */
				setvbuf(stdout, _IONBF, NULL, 0);
#endif /* ifndef else SETVBUF_REVERSED */
			}
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

			setup_atexit();

			run();
		}
	
		else {		/* pid != 0 */
			pidfile = fopen(FILE_PID, "w");
			if (pidfile == NULL) {
				error(MIAU_ERRFILE, cfg.home);
				kill(pid, SIGTERM);
				exit(ERR_CODE_HOME);
			}
			fprintf(pidfile, "%d\n", pid);
			fclose(pidfile);
			
			report(MIAU_NICK, (char *) nicknames.nicks.head->data);
			report(MIAU_FORKED, pid);
		}
	} else {
		setup_atexit();

		run();
	}

	return EXIT_SUCCESS;
} /* int main(int argc, char **argv) */
