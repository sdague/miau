/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2003-2005 Tommi Saviranta <wnd@iki.fi>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG_H */
#include "match.h"
#include "tools.h"
#include "common.h"

#include <string.h>
#include <stdio.h>



/**
 * match:
 * @string:	String to match
 * @pattern:	Pattern to match against
 *
 * Return value: 1 if string matches patters, 0 if not.
 */
int
match(const char *string, const char *pattern)
{
	char	*str, *pat;
	int	str_p, pat_p;

	if (string == NULL || pattern == NULL) {
		return 0;
	}

	str = strdup(string);
	upcase(str);
	
	pat = strdup(pattern);
	upcase(pat);

	str_p = pat_p = 0;

	/* Then to the matching part. */
	while (1) {
		if (str[str_p] == '\0' && pat[pat_p] == '\0') {
			xfree(str);
			xfree(pat);
			return 1;
		}
		if (str[str_p] == '\0' || pat[pat_p] == '\0') {
			xfree(str);
			xfree(pat);
			return 0;
		}

		if (str[str_p] == pat[pat_p] || pat[pat_p] == '?') {
			str_p++;
			pat_p++;
		} else if (pat[pat_p] == '*') {
			if (*str == pat[pat_p + 1]) {
				pat_p++;
			} else if (pat[pat_p + 1] == str[str_p + 1]) {
				str_p++;
				pat_p++;
			} else {
				str_p++;
			}
		} else {
			xfree(str);
			xfree(pat);
			return 0;
		}
	}
} /* int match(const char *string, const char *pattern) */



/**
 * match:
 * @string:	String to match
 * @pattern:	Pattern to match against
 * @foo:	String to replace. Can be NULL.
 * @foo_len:	Length of foo or -1 if unknown
 * @replace:	String to replace foo with. Can be NULL.
 *
 * Return value: 1 if string matches patters, 0 if not.
 */
int
match_replace(const char *string, const char *pattern, const char *foo,
		const int foo_len, const char *replace)
{
	char *pat;
	int pat_len;
	int replace_len;
	int foo_len_real;
	int at;
	int ret;
	
	/* Nothing to replace? Don't waste time here. */
	if (foo == NULL) {
		return match(string, pattern);
	}

	/* Replace "@@" in pattern with "replace" if it was given. */
	replace_len = (int) strlen(replace);

	foo_len_real = foo_len == -1 ? strlen(foo) : foo_len;

	for (at = strlen(pattern) - foo_len_real; at >= 0; at--) {
		if (xstrncmp(pattern + at, foo, foo_len_real) == 0) {
			break;
		}
	}
	
	/* No "foo" found. */
	if (at == -1) {
		return match(string, pattern);
	}

	pat_len = strlen(pattern) + replace_len - foo_len_real + 1;
	pat = (char *) xmalloc(pat_len);
	memcpy(pat, pattern, at);
	memcpy(pat + at, replace, replace_len);
	memcpy(pat + at + replace_len, pattern + at + foo_len_real,
			strlen(pattern) - at - foo_len_real);
	pat[pat_len - 1] = '\0';

	ret = match(string, pat);
	xfree(pat);

	return ret;
} /* int match_replace(const char *string, const char *pattern,
		const char *foo, const int foo_len, const char *replace) */
