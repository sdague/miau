/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2003-2004 Tommi Saviranta <wnd@iki.fi>
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

#ifndef ERROR_H_
#define ERROR_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* ifdef HAVE_CONFIF_H */

#include <stdarg.h>



#ifdef ENDUSERDEBUG
void enduserdebug(char *format, ...);
#endif /* ifdef ENDUSERDEBUG */

void report(char *format, ...);
void error(char *format, ...);



#endif /* ifndef ERROR_H_ */
