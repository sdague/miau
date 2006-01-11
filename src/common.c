/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2002-2005 Tommi Saviranta <wnd@iki.fi>
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

#include "common.h"
#include "error.h"
#include "messages.h"

#include <stdlib.h>
#include <string.h>



#define REPORT_ERROR(func) error(ERR_UNEXPECTED, func, file, line)



#if HAVE_MALLOC != 1
/*
 * Need to undef. Will cause warnings, but until someone can provide me a
 * better way to do this, warnings shall we have. Same goes for rpl_realloc.
 */
#undef malloc
#include <stdlib.h>

void *
rpl_malloc(size_t size)
{
	if (size == 0) {
		size = 1;
	}
	return malloc(size);
} /* void *rpl_malloc(size_t size) */
#endif /* if HAVE_MALLOC != 1 */



#if HAVE_REALLOC != 1
#undef realloc
#include <stdlib.h>

void *
rpl_realloc(void *ptr, size_t size)
{
	if (size == 0) {
		size = 1;
		if (ptr != NULL) {
			free(ptr);
			ptr = NULL;
		}
	}
	if (ptr == NULL) {
		return malloc(size);
	} else {
		return realloc(ptr, size);
	}
} /* void *rpl_realloc(void *ptr, size_t size) */
#endif /* if HAVE_REALLOC != 1 */



int
_xstrcmp(const char *s1, const char *s2  DEBUG_ADDPARMS)
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
_xstrncmp(const char *s1, const char *s2, size_t n  DEBUG_ADDPARMS)
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
_xstrcasecmp(const char *s1, const char *s2  DEBUG_ADDPARMS)
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
_xstrncasecmp(const char *s1, const char *s2, size_t n  DEBUG_ADDPARMS)
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
_xstrcpy(char *dest, const char *src  DEBUG_ADDPARMS)
{
	if (dest != NULL && src != NULL) {
		return strcpy(dest, src); /* dangerous, but can't help it */
	} else {
#ifdef DEBUG_ADDPARMS
		REPORT_ERROR("xstrcpy");
#endif
		return 0;
	}
} /* char *_xstrcpy(char *dest, const char *src DEBUG_ADDPARMS) */



char *
_xstrncpy(char *dest, const char *src, size_t n  DEBUG_ADDPARMS)
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
_xstrdup(const char *s  DEBUG_ADDPARMS)
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
xmalloc(size_t size)
{
	void *ret = malloc(size);
	if (ret == NULL) {
		error(ERR_MEMORY);
		exit(ERR_CODE_MEMORY);
	}
	return ret;
} /* void *xmalloc(size_t size) */



void *
xcalloc(size_t nmemb, size_t size)
{
	void *ret = calloc(nmemb, size);
	if (ret == NULL) {
		error(ERR_MEMORY);
		exit(ERR_CODE_MEMORY);
	}
	return ret;
} /* void *xcalloc(size_t nmemb, size_t size) */



void
xfree(void *ptr)
{
	if (ptr != NULL) {
		free(ptr);
	}
} /* void xfree(void *ptr) */



void *
xrealloc(void *ptr, size_t size)
{
	void *ret = realloc(ptr, size);
	if (ret == NULL && size > 0) {
		error(ERR_MEMORY);
		exit(ERR_CODE_MEMORY);
	}
	return ret;
} /* void *xrealloc(void *ptr, size_t size) */



/* these functions are "intentionally" unsafe! */
#ifndef HAVE_SNPRINTF
int
snprintf(char *str, size_t size, const char *format, ...)
{
	int r;
	va_list va;

	va_start(va, format);
	r = sprintf(str, format, va);
	va_end(va);
	str[size - 1] = '\0';

	return r;
} /* int snprintf(char *str, size_t size, const char *format, ...) */
#endif /* ifndef HAVE_SNPRINTF */



#ifndef HAVE_VSNPRINTF
int
vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	return vsprintf(str, format, ap);
} /* int vsnprintf(char *str, size_t size, const char *format, va_list ap) */
#endif /* ifndef HAVE_VSNPRINTF */
