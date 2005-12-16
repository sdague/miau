/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2004-2005 Tommi Saviranta <wnd@iki.fi>
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

#ifndef LOG_H_
#define LOG_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIG */

#include "etc.h"

#ifdef NEED_LOGGING



char *log_prepare_entry(const char *nick, const char *msg);



#endif /* ifdef NEED_LOGGING */

#endif /* ifndef LOG_H_ */
