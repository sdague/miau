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

#include <config.h>
#include "miau.h"
#include "irc.h"
#include "perm.h"
#include "server.h"
#include "tools.h"
#include "llist.h"

/* #define DEBUG */

#ifdef HAVE_HSTRERROR
#ifdef HSTRERROR_PROTO
const char *hstrerror(int err);
#endif /* ifdef HSTRERROR_PROTO */
#endif /* ifdef HAVE_HSTRERROR */


#define HEAD	1
#define TAIL	2

void track_highest();
void track_add(int s);
void track_del(int s);
int irc_write_real(connection_type *connection, char *buffer);
int irc_write_smart(connection_type *connection, char *buffer, int queue);

struct hostent *hostinfo = NULL;
#ifdef IPV6
struct sockaddr_in6 addr;
#else
struct sockaddr_in addr;
#endif

const char *net_errstr;

#define TRACK 512
int track_socks[TRACK];

int highest_socket = 0;

int		msgtimer;	/* Message timer... */
llist_list	msg_queue;	/* Message queue */



void
track_highest(void)
{
	int	i;
	highest_socket = 0;
	for (i = 0; i < TRACK; i++) {
		if (track_socks[i] > highest_socket) {
			highest_socket = track_socks[i];
		}
	}
} /* void track_highest(void) */



void
track_add(int s)
{
	int	i = 0;
	while (track_socks[i] && i < TRACK) {
		i++;
	}
	if (i < TRACK) {
		track_socks[i] = s;
	}
	track_highest();
} /* void track_add(int s) */



void
track_del(int s)
{
	int	i;
	for (i = 0; i < TRACK; i++) {
		if (track_socks[i] == s) {
			track_socks[i] = 0;
		}
	}
	track_highest();
} /* void track_del(int s) */



/*
 * Creates a socket.
 *
 * Returns number of opened socket or -1 if something went wrong.
 */
int
sock_open(void)
{
	int	i;

#ifdef IPV6
	if ((i = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0)
#else
	if ((i = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
#endif
	{
		net_errstr = strerror(errno);
		return -1;
	}
	track_add(i);
	sock_setreuse(i);
	/* local reuse by default */
	return i;
} /* int sock_open(void) */



int
rawsock_close(int sock)
{
	if (! sock) return 1;

	track_del(sock);
	close(sock);
	return 1;
} /* int rawsock_close(int sock) */



int
sock_close(connection_type *connection)
{
	rawsock_close(connection->socket);
	connection->socket = 0;
	return 1;
} /* int sock_close(connection_type *connection) */



int
sock_setnonblock(int sock)
{
	if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
		net_errstr = strerror(errno);
		return 0;
	} else {
		return 1;
	}
} /* int sock_setnonblock(int sock) */



int
sock_setblock(int sock)
{
	int	flags;

	flags = fcntl(sock, F_GETFL, 0);
	if (flags == -1) {
		net_errstr = strerror(errno);
		return 0;
	}

	flags &= ~O_NONBLOCK;

	if (fcntl(sock, F_SETFL, flags) < 0) {
		net_errstr = strerror(errno);
		return 0;
	}

	return 1;
} /* int sock_setblock(int sock) */



int
sock_setreuse(int sock)
{
	int	i = 1;
	
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &i,
				sizeof(i)) < 0) {
		net_errstr = strerror(errno);
		return 0;
	} else {
		return 1;
	}
} /* int sock_setreuse(int sock) */



struct hostent *
name_lookup(char *host)
{
#ifdef IPV6
	hostinfo = gethostbyname2(host, AF_INET6);
	if (hostinfo != NULL) {
		return hostinfo;
	}
#else /* IPV6 */
	hostinfo = gethostbyname(host);
	if (hostinfo != NULL) {
		return hostinfo;
	}
#endif /* IPV6 */

	/* TODO: hstrerror is obsolete */
#ifdef HAVE_HSTRERROR
	net_errstr = (const char *) hstrerror(h_errno);
#else
	net_errstr = "unable to resolve";
#endif
	return NULL;
} /* struct hostent *name_lookup(char *host) */



#ifdef IPV6
int
sock_bind(int sock, char *bindhost, int port)
{
	/* We'd better cast &addr to void * to keep Digital-UNIX happy. */
	bzero((void *) &addr, sizeof(struct sockaddr_in6));
	
	addr.sin6_addr = in6addr_any;
	addr.sin6_family = AF_INET6;
	
	if (bindhost) {
		if (! name_lookup(bindhost)) {
			return 0;
		}

		memcpy((char *) &addr.sin6_addr, hostinfo->h_addr,
				hostinfo->h_length);
		addr.sin6_family = hostinfo->h_addrtype;
	}

	addr.sin6_port = htons((u_short)port);

	if (bind(sock, (struct sockaddr *) &addr,
				sizeof(struct sockaddr_in6)) < 0) {
		net_errstr = strerror(errno);
		return 0;
	}

	return 1;
} /* int sock_bind(int sock, char *bindhost, int port) */



int
sock_bindlookedup(int sock, int port)
{
	/* We'd better cast &addr to void * to keep Digital-UNIX happy. */
	bzero((void *) &addr, sizeof(struct sockaddr_in6));

	addr.sin6_addr = in6addr_any;
	addr.sin6_family = AF_INET6;
	
	if (hostinfo) {
		memcpy((char *) &addr.sin6_addr, hostinfo->h_addr,
				hostinfo->h_length);
		addr.sin6_family = hostinfo->h_addrtype;
	}

	addr.sin6_port = htons((u_short)port);
	
	if (bind(sock, (struct sockaddr *)&addr,
				sizeof(struct sockaddr_in6)) < 0) {
		net_errstr = strerror(errno);
		return 0;
	}

	return 1;
} /* int sock_bindlookedup(int sock, int port) */

#else

int
sock_bind(int sock, char *bindhost, int port)
{
	/* We'd better cast &addr to void * to keep Digital-UNIX happy. */
	bzero((void *) &addr, sizeof(struct sockaddr_in));

	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	
	if (bindhost) {
		if (name_lookup(bindhost) == NULL) {
			return 0;
		}
		
		memcpy((char *) &addr.sin_addr, hostinfo->h_addr,
				hostinfo->h_length);
		addr.sin_family = hostinfo->h_addrtype;
	}
	
	addr.sin_port = htons((u_short) port);
	
	if (bind(sock, (struct sockaddr *) &addr, 
				sizeof(struct sockaddr)) < 0) {
		net_errstr = strerror(errno);
		return 0;
	}

	return 1;
} /* int sock_bind(int sock, char *bindhost, int port) */



int
sock_bindlookedup(int sock, int port)
{
	/* We'd better cast &addr to void * to keep Digital-UNIX happy. */
	bzero((void *) &addr, sizeof(struct sockaddr_in));

	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	
	if (hostinfo) {
		memcpy((char *) &addr.sin_addr, hostinfo->h_addr,
				hostinfo->h_length);
		addr.sin_family = hostinfo->h_addrtype;
	}

	addr.sin_port = htons((u_short) port);
	
	if (bind(sock, (struct sockaddr *) &addr,
				sizeof(struct sockaddr)) < 0) {
		net_errstr = strerror(errno);
		return 0;
	}
	
	return 1;
} /* int sock_bindlookedup(int sock, int port) */

#endif



int
sock_listen(int sock)
{
	if (! sock_setnonblock(sock)) {
		return 0;
	}

	if (listen(sock, QUEUESIZE) < 0) {
		net_errstr = strerror(errno);
		return 0;
	}
	
	return 1;
} /* int sock_listen(int sock) */



/*
 * Accept socket. Return socket.
 *
 * If checkperm is non-zero, a check is made to be sure connecting IP is allowed
 * to establish a connection with us. If connection is unauthorized, connection
 * will be closed and function returns -1.
 */
int
sock_accept(int sock, char **s, int checkperm)
{
	socklen_t	temp;
	int		store;
#ifdef IPV6
	char		ip[40];
	char		ipv6[512];
#else
	char		*ip;
#endif
	int		perm;

#ifdef IPV6
	temp = sizeof(struct sockaddr_in6);
#else
	temp = sizeof(struct sockaddr_in);
#endif
	if ((store = accept(sock, (struct sockaddr *) &addr, &temp)) < 0) {
		net_errstr = strerror(errno);
		return 0;
	}

#ifdef IPV6
	inet_ntop(AF_INET6, (char *) &addr.sin6_addr, ip,
			sizeof(addr.sin6_addr));
#else
	ip = inet_ntoa(addr.sin_addr);
#endif
	perm = is_perm(&connhostlist, ip);

#ifdef IPV6
	if (! getnameinfo((struct sockaddr *) &addr, sizeof(addr), ipv6,
				sizeof(ipv6), 0, 0, 0)) {
		*s = xstrdup(ipv6);
		perm |= is_perm(&connhostlist, *s);
	} else {
		*s = xstrdup(ip);
	}

#else
	hostinfo = gethostbyaddr((char *) &addr.sin_addr.s_addr,
			sizeof(struct in_addr), AF_INET);
	
	if (hostinfo) {
		*s = xstrdup(hostinfo->h_name);
		perm |= is_perm(&connhostlist, *s);
	} else {
		*s = xstrdup(ip);
	}
#endif

	if (! checkperm || perm) {
		track_add(store);
		return store;
	} else {
		close(store);
		return -1;
	}
} /* int sock_accept(int sock, char **s, int checkperm) */



/*
 * Send data to all connected clients.
 */
int
irc_mwrite(clientlist_type *clients, char *format, ...)
{
	llist_node	*client;
	va_list		va;
	char		buffer[BUFFERSIZE];
	int		ret = 0;

	if (clients->connected == 0) {
		return 0;
	}

	va_start(va, format);
	vsnprintf(buffer, BUFFERSIZE - 3, format, va);
	va_end(va);
	buffer[BUFFERSIZE - 3] = '\0';
	for (client = clients->clients->head; client != NULL;
			client = client->next) {
		/*
		 * Having '"%s", buffer' instead of plain 'buffer' is essential
		 * because we don't want our string processed any further by
		 * va.
		 */
		ret += irc_write((connection_type *) client->data,
				"%s", buffer);
	}

	return (ret != 0);
} /* int irc_mwrite(clientlist_type *clients, char *format, ...) */



/*
 * Put message in front of queue.
 *
 * We need this to make sure PONGs are not delayed for too long.
 */
int
irc_write_head(connection_type *connection, char *format, ...)
{
	va_list	va;
	char	buffer[BUFFERSIZE];

	va_start(va, format);
	vsnprintf(buffer, BUFFERSIZE - 3, format, va);
	va_end(va);
	buffer[BUFFERSIZE - 3] = '\0';
	strcat(buffer, "\r\n");

	return irc_write_smart(connection, buffer, HEAD);
} /* int irc_write_head(connection_type *connection, char *format, ...) */



int
irc_write(connection_type *connection, char *format, ...)
{
	va_list	va;
	char	buffer[BUFFERSIZE];

	va_start(va, format);
	vsnprintf(buffer, BUFFERSIZE - 3, format, va);
	va_end(va);
	buffer[BUFFERSIZE - 3] = '\0';
	strcat(buffer, "\r\n");

	return irc_write_smart(connection, buffer, TAIL);
} /* int irc_write(connection_type *connection, char *format, ...) */


/*
 * Send messages "the smart way".
 *
 * If message is being sent to client, send it instantly. If message is going
 * to the server and flood control allows this, send message and decrease flood
 * counter. Otherwise put message in queue.
 */
int
irc_write_smart(connection_type *connection, char *buffer, const int queue)
{
	if (connection != &c_server) {
		return irc_write_real(connection, buffer);
	} else if (msgtimer > 1) {
		msgtimer--;
		return irc_write_real(connection, buffer);
	} else {
		/*
		 * All messages in queue are due to sending to server so
		 * there is no need to save target of these messages.
		 */
		if (queue == TAIL) {
			llist_add_tail(llist_create(xstrdup(buffer)),
					&msg_queue);
		} else {
			llist_add(llist_create(xstrdup(buffer)),
					&msg_queue);
		}
		return strlen(buffer);
	}
} /* int irc_write_smart(connection_type *connection, char *buffer,
		const int queue) */



int
irc_write_real(connection_type *connection, char *buffer)
{
#ifdef DEBUG
	fprintf(stdout, ">>%03d>> %s", connection->socket, buffer);
	fflush(stdout);
#endif
	return send(connection->socket, buffer, strlen(buffer), 0);
} /* int irc_write_real(connection_type *connection, char *buffer) */



/*
 * Process send queue.
 *
 * miau can throttle messages (to server) so that we don't get disconnected
 * from the server for excess flood. As long as queue is non-empty and flood
 * control allows us to send, send first message in queue and decrese flood
 * control counter.
 */
void
irc_process_queue(void)
{
	char	*buf;
	/*
	 * Basically there can be only one message in queue that can be
	 * sent "per round" but there are exeptions. If executing miau is
	 * delayed because of heavy system load or suspended miau, next
	 * time timers are checked, msgtimer is increased (possibly by
	 * more than one) like there was no delay (in execution of miau).
	 *
	 * We could, of course, increase counter by one no matter what
	 * happened, but seriously, the code would be only _very_ little
	 * smaller... And this "the way to go", don't you think ?-)
	 */
	while (msg_queue.head != NULL && msgtimer > 0) {
		buf = (char *) msg_queue.head->data;
		irc_write_real(&c_server, buf);
		/* If message is 'MODE' message, add additional penalty. */
		if (strlen(buf) >= 4 && buf[0] == 'M' && buf[1] == 'O'
				&& buf[2] == 'D' && buf[3] == 'E') {
			msgtimer--;
		}
		/* After message is sent, remove it from queue. */
		xfree(msg_queue.head->data);
		llist_delete(msg_queue.head, &msg_queue);
		/* Finally decrease flood control counter. */
		msgtimer--;
	}
} /* void irc_process_queue(void) */



/*
 * Clear send queue.
 *
 * Removes all lines from send queue. This function is most likely called
 * when miau disconnects from the server or when miau is being shut down.
 */
void
irc_clear_queue(void)
{
	while (msg_queue.head != NULL) {
		xfree(msg_queue.head->data);
		llist_delete(msg_queue.head, &msg_queue);
	}
} /* void irc_clear_queue(void) */



/*
 * Send data to all connected clients.
 */
int
irc_mnotice(clientlist_type *clients, char nickname[], char *format, ...)
{
	llist_node	*client;
	va_list		va;
	char		buffer[BUFFERSIZE];
	int		ret = 0;

	if (clients->connected == 0) {
		return 0;
	}

	va_start(va, format);
	vsnprintf(buffer, BUFFERSIZE - 10, format, va);
	va_end(va);
	buffer[BUFFERSIZE - 9] = '\0';

	for (client = clients->clients->head; client != NULL;
			client = client->next) {
		ret += irc_write((connection_type *) client->data,
				"NOTICE %s :%s", nickname, buffer);
	}

	return ret;
} /* int irc_mnotice(clientlist_type *clients, char nickname[],
		char *format, ...) */



void
irc_notice(connection_type *connection, char nickname[], char *format, ...)
{
	va_list	va;
	char	buffer[BUFFERSIZE];
	
	va_start(va, format);
	vsnprintf(buffer, BUFFERSIZE - 10, format, va);
	va_end(va);
	buffer[BUFFERSIZE - 9] = '\0';

	irc_write(connection, "NOTICE %s :%s", nickname, buffer);
} /* void irc_notice(connection_type *connection, char nickname[],
		char *format, ...) */



/*
 * Read data.
 *
 * Return values:
 * 	-1	An error occured
 * 	0	No data
 * 	1	Received data
 * Returns number of bytes received, -1 if there was an error.
 */
int
irc_read(connection_type *connection)
{
	int	ret;

	do {
		ret = recv(connection->socket,
				connection->buffer + connection->offset, 1, 0);
		if (ret == 0) return -1;
		if (ret == -1 && errno == EAGAIN) return 0;
		connection->offset++;
	} while (connection->buffer[connection->offset - 1] != '\n' &&
			connection->offset < BUFFERSIZE - 4);
	connection->buffer[connection->offset - 1] = '\0';
	if (connection->buffer[connection->offset - 2] == '\r') {
		connection->buffer[connection->offset - 2] = '\0';
	}
	connection->buffer[BUFFERSIZE - 1] = '\0';

#ifdef DEBUG
	fprintf(stdout, "<<%03d<< %s\n", connection->socket,
			connection->buffer);
	fflush(stdout);
#endif
	connection->offset = 0;
	connection->timer = 0;		/* Got data, reset timer. */
	return 1;
} /* int irc_read(connection_type *connection) */



/*
 * Connect to IRC-server.
 *
 * Return values:
 *	CONN_OK		All ok
 *	CONN_SOCK	Can't create socket
 *	CONN_LOOKUP	Can't resolve address
 *	CONN_BIND	Can't bind port
 *	CONN_CONNECT	Can't connect
 *	CONN_WRITE	Couldn't send data
 *	CONN_OTHER	Setting to nonblocking failed
 */
int
irc_connect(connection_type *connection, server_type *server, char *nickname,
		char *username, char *realname, char *bindto)
{
	int		randport = 0;
	int		ri, attempts;

	connection->timer = 0;

	if ((connection->socket = sock_open()) < 0) {
		return CONN_SOCK;
	}

	hostinfo = 0;
	if (bindto) {
		if (name_lookup(bindto) == NULL) {
			return CONN_BIND;
		}
	}
	
	attempts = 15;
	do {
		/* random() here is totally safe */
		for (ri = random() & 0xff; ri; ri--) {
			randport = (random() & 0xffff) | 1024;
		}
#ifdef IPV6
		addr.sin6_port = htons(randport);
#else
		addr.sin_port = htons(randport);
#endif
		attempts--;
	} while (! sock_bindlookedup(connection->socket, randport) && attempts);
	
	if (! attempts) {
		return CONN_BIND;
	}

	if (name_lookup(server->name) == NULL) {
		return CONN_LOOKUP;
	}

#ifdef IPV6
	memcpy((char *) &addr.sin6_addr, hostinfo->h_addr, hostinfo->h_length);
	addr.sin6_port = htons((u_short) server->port);
	addr.sin6_family = hostinfo->h_addrtype;
	if (connect(connection->socket, (struct sockaddr *) &addr,
				sizeof(struct sockaddr_in6)) < 0) {
		return CONN_CONNECT;
	}
#else
	memcpy((char *) &addr.sin_addr, hostinfo->h_addr, hostinfo->h_length);
	addr.sin_port = htons((u_short) server->port);
	
	addr.sin_family = hostinfo->h_addrtype;
	if (connect(connection->socket, (struct sockaddr *) &addr,
				sizeof(struct sockaddr_in)) < 0) {
		return CONN_CONNECT;
	}
#endif

	if (server->password) {
		irc_write(connection, "PASS %s", server->password);
	}
	if (irc_write(connection, "NICK %s", nickname) < 0) {
		return CONN_WRITE;
	}
	/* We be lazy - modes not set here. */
	if (irc_write(connection, "USER %s 0 * :%s", username, realname) < 0) {
		return CONN_WRITE;
	}
	if (! sock_setnonblock(connection->socket)) {
		return CONN_OTHER;
	}
	
	return CONN_OK;
} /* int irc_connect(connection_type *connection, server_type *server,
		char *nickname, char *username, char *realname, char *bindto) */
