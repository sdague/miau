/*
 * -------------------------------------------------------
 * Copyright (C) 2003-2004 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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

#include "match.h"
#include "miau.h"
#include "tools.h"
#include "common.h"



/*
 * Non-recursive caseinsensitive pattern-matching.
 */
int
match(
		const char	*string,
		const char	*pattern
     )
{
	char	*str;
	char	*pat = NULL;
	char	*host;
	char	*host_at;
	int	hostlen;

	if (string == NULL || pattern == NULL) {
		return 0;
	}

	str = strdup(string);
	upcase(str);
	string = str;

	if (status.goodhostname != 0) {
		host = status.idhostname + status.goodhostname;
		hostlen = (int) strlen(host);
		/*
		 * "@@" is replaced by hostname and terminator (strlen()
		 * doesn't count terminator so we can decrement lengths by one.
		 *
		 * Even if "@@" was not found, we can use duplicated pattern
		 * later or (although xrealloc would be wasteful).
		 */
		pat = (char *) xmalloc(strlen(pattern) + hostlen - 1);
		strcpy(pat, pattern); /* Could use strncpy. */
		/*
		 * Find and replace _last_ occurance of "@@" with our hostname.
		 */
		for (host_at = pat + strlen(pat) - 1; host_at >= pat;
				host_at--) {
			if (host_at[0] == '@' && host_at[1] == '@') {
				/* Terminate new pattern before. */
				host_at[hostlen + strlen(host_at + 2)] = '\0';
				memmove(host_at + hostlen, host_at + 2,
						strlen(host_at + 2));
				strncpy(host_at, host, hostlen);
				break;
			}
		}
	}
	if (pat == NULL) {
		pat = strdup(pattern);
	}
	upcase(pat);
	pattern = pat;

	/* Then to the matching part. */
	while (1) {
		if (*string == '\0' && *pattern == '\0') {
			xfree(str);
			xfree(pat);
			return 1;
		}
		if (*string == '\0' || *pattern == '\0') {
			xfree(str);
			xfree(pat);
			return 0;
		}

		if (*string == *pattern || (*string && *pattern == '?')) {
			string++;
			pattern++;
		} else if (*pattern == '*') {
			if (*string == *(pattern + 1)) {
				pattern++;
			}
			else if (*(pattern + 1) == *(string + 1)) {
				string++;
				pattern++;
			} else {
				string++;
			};
		} else {
			xfree(str);
			xfree(pat);
			return 0;
		}
	}
} /* int match(const char *, const char *) */
