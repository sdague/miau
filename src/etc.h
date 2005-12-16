/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2005 Tommi Saviranta <wnd@iki.fi>
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

#ifndef ETC_H_
#define ETC_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */


#ifndef HAVE_RANDOM
#define random()	(rand()/16)
#endif /* ifndef HAVE_RANDOM */

#ifndef HAVE_SIGACTION     /* old "weird signals" */
#define sigaction	sigvec
#ifndef sa_handler
#define sa_handler	sv_handler
#define sa_mask		sv_mask
#define sa_flags	sv_flags
#endif /* ifndef sa_handler */
#endif /* ifndef HAVE_SIGACTION */



#if 0
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
#endif



/* NEED_LOGGING */
#ifdef CHANLOG
#define NEED_LOGGING
#endif /* ifdef CHANLOG */
#ifdef PRIVLOG
#define NEED_LOGGING
#endif /* ifdef PRIVLOG */

/* NEED_CMDPASSWD */
#ifdef RELEASENICK
#define NEED_CMDPASSWD
#endif /* ifdef RELEASENICK */

/* NEED_TABLE */
#ifdef DCCBOUNCE
#define NEED_TABLE
#endif /* ifdef DCCBOUNCE */
#ifdef CTCPREPLIES
#define NEED_TABLE
#endif /* ifdef CTCPREPLIES */

/* NEED_PROCESS_IGNORES */
#ifdef CTCPREPLIES
#define NEED_PROCESS_IGNORES
#endif /* ifdef CTCPREPLIES */



#endif /* ifndef ETC_H_ */
