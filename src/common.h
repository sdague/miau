/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2003-2004 Tommi Saviranta <tsaviran@cs.helsinki.fi>
 *	(C) 2002 Sebastian Kienzl <zap@riot.org>
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

#ifndef COMMON_H_
#define COMMON_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */
#include <stdlib.h>



/* _NEED_CMDPASSWD */
#ifdef RELEASENICK
#define _NEED_CMDPASSWD
#endif /* RELEASENICK */

/* _NEED_TABLE */
#ifdef DCCBOUNCE
#define _NEED_TABLE
#endif /* DCCBOUNCE */
#ifdef CTCPREPLIES
#define _NEED_TABLE
#endif /* CTCPREPLIES */

/* _NEED_PROCESS_IGNORES */
#ifdef CTCPREPLIES
#define _NEED_PROCESS_IGNORES
#endif 



/* Error codes. */
#define ERR_CODE_HOME		2
#define ERR_CODE_CONFIG		3
#define ERR_CODE_NETWORK	4
#define ERR_CODE_MEMORY		9



/* Only if preprocessor supports __FILE__/__LINE__-stuff. */
#ifdef __FILE__
	#define xstrcmp(s1, s2) _xstrcmp(s1, s2, __FILE__, __LINE__)
	#define xstrncmp(s1, s2, n) _xstrncmp(s1, s2, n, __FILE__, __LINE__)
	#define xstrcasecmp(s1, s2) _xstrcasecmp(s1, s2, __FILE__, __LINE__)
	#define xstrncasecmp(s1, s2, n) _xstrncasecmp(s1, s2, n, __FILE__, __LINE__)
	#define xstrcpy(dest, src) _xstrcpy(dest, src, __FILE__, __LINE__)
	#define xstrncpy(dest, src, n) _xstrncpy(dest, src, n, __FILE__, __LINE__)
	#define xstrdup(s) _xstrdup(s, __FILE__, __LINE__)
	#define DEBUG_ADDPARMS , const char *file, const int line
#else /* ifdef __FILE__ */
	#define xstrcmp(s1, s2) _xstrcmp(s1, s2)
	#define xstrncmp(s1, s2, n) _xstrncmp(s1, s2)
	#define xstrcasecmp(s1, s2) _xstrcasecmp(s1, s2)
	#define xstrncasecmp(s1, s2, n) _xstrncasecmp(s1, s2, n)
	#define xstrcpy(dest, src) _xstrcpy(dest, src)
	#define xstrncpy(dest, src, n) _xstrncpy(dest, src, n)
	#define xstrdup(s) _strdup(s)
#endif /*ifdef else __FILE__ */

int _xstrcmp(const char *s1, const char *s2 DEBUG_ADDPARMS);
int _xstrncmp(const char *s1, const char *s2, size_t n DEBUG_ADDPARMS);
int _xstrcasecmp(const char *s1, const char *s2 DEBUG_ADDPARMS);
int _xstrncasecmp(const char *s1, const char *s2, size_t n DEBUG_ADDPARMS);

char *_xstrcpy(char *dest, const char *src DEBUG_ADDPARMS);
char *_xstrncpy(char *dest, const char *src, size_t n DEBUG_ADDPARMS);
char *_xstrdup(const char *s DEBUG_ADDPARMS);

void *xmalloc(size_t size);
void *xcalloc(size_t, size_t);
void xfree(void *ptr);
/* void xnullfree(void **ptr); */
void *xrealloc(void *ptr, size_t size);


/* #define FREE(ptr) xnullfree((void *) &ptr) */
#define FREE(ptr) { if (ptr != NULL) { free(ptr); ptr = NULL; } }



#endif /* ifndef COMMON_H_ */
