/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2002-2004 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "messages.h"
#include "error.h"



#define REPORT_ERROR(func) error(ERR_UNEXPECTED, func, file, line)



int
_xstrcmp(
		const char	*s1,
		const char	*s2
		DEBUG_ADDPARMS
	)
{
	if (s1 != NULL && s2 != NULL) {
		return strcmp(s1, s2);
	} else {
#ifdef DEBUG_ADDPARMS
		REPORT_ERROR("xstrcmp");
#endif
		return 1;
	}
} /* int _xstrcmp(const char *s1, const char *s2 DEBUG_ADDPARMS) */



int
_xstrncmp(
		const char	*s1,
		const char	*s2,
		size_t		n
		DEBUG_ADDPARMS
	 )
{
	if (s1 != NULL && s2 != NULL) {
		return strncmp(s1, s2, n);
	} else {
#ifdef DEBUG_ADDPARMS
		REPORT_ERROR("xstrncmp");
#endif
		return 1;
	}
} /* int _xstrncmp(const char *s1, const char *s2, size_t n DEBUG_ADDPARMS) */



int
_xstrcasecmp(
		const char	*s1,
		const char	*s2
		DEBUG_ADDPARMS
	    )
{
	if (s1 != NULL && s2 != NULL) {
		return strcasecmp(s1, s2);
	} else {
#ifdef DEBUG_ADDPARMS
		REPORT_ERROR("xstrcasecmp");
#endif
		return 1;
	}
} /* int _xstrcasecmp(const char *s1, const char *s2 DEBUG_ADDPARMS) */



int
_xstrncasecmp(
		const char	*s1,
		const char	*s2,
		size_t		n
		DEBUG_ADDPARMS
	     )
{
	if (s1 != NULL && s2 != NULL) {
		return strncasecmp(s1, s2, n);
	} else {
#ifdef DEBUG_ADDPARMS
		REPORT_ERROR("xstrncasecmp");
#endif
		return 1;
	}
} /* int _xstrncasecmp(const char *s1, const char *s2, size_t n
		DEBUG_ADDPARMS) */



char *
_xstrcpy(
		char		*dest,
		const char	*src
		DEBUG_ADDPARMS
	)
{
	if (dest != NULL && src != NULL) {
		return strcpy(dest, src);
	} else {
#ifdef DEBUG_ADDPARMS
		REPORT_ERROR("xstrcpy");
#endif
		return 0;
	}
} /* char *_xstrcpy(char *dest, const char *src DEBUG_ADDPARMS) */



char *
_xstrncpy(
		char		*dest,
		const char	*src,
		size_t		n
		DEBUG_ADDPARMS
	 )
{
	if (dest != NULL && src != NULL) {
		return strncpy(dest, src, n);
	} else {
#ifdef DEBUG_ADDPARMS
		REPORT_ERROR("xstrncpy");
#endif
		return 0;
	}
} /* char *_xstrncpy(char *dest, const char *src, size_t n DEBUG_ADDPARMS) */



char *
_xstrdup(
		const char	*s
		DEBUG_ADDPARMS
       )
{
	char *p;
	if (s != NULL) {
		p = strdup(s);
		if (p == NULL) {
			error(ERR_MEMORY);
			exit(ERR_CODE_MEMORY);
		}
		return p;
	} else {
#ifdef DEBUG_ADDPARMS
		REPORT_ERROR("xstrdup");
#endif
		return NULL;
	}
} /* char *_xstrdup(const char *s DEBUG_ADDPARMS) */



void *
xmalloc(
		size_t	size
       )
{
	void *ret = malloc(size);
	if (ret == NULL) {
		error(ERR_MEMORY);
		exit(ERR_CODE_MEMORY);
	}	
	return ret;
} /* void *xmalloc(size_t size) */



void *
xcalloc(
		size_t	nmemb,
		size_t	size
       )
{
	void *ret = calloc(nmemb, size);
	if (ret == NULL) {
		error(ERR_MEMORY);
		exit(ERR_CODE_MEMORY);
	}
	return ret;
} /* void *xcalloc(size_t nmemb, size_t size) */



void
xfree(
		void	*ptr
     )
{
	if (ptr != NULL) {
		free(ptr);
	}
} /* void xfree(void *ptr) */



void *
xrealloc(
		void	*ptr,
		size_t	size
	)
{
	void *ret = realloc(ptr, size);
	if (ret == NULL && size > 0) {
		error(ERR_MEMORY);
		exit(ERR_CODE_MEMORY);
	}
	return ret;
} /* void *xrealloc(void *ptr, size_t size) */
