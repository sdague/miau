/*
 * -------------------------------------------------------
 * Copyright (C) 2002-2003 Tommi Saviranta <tsaviran@cs.helsinki.fi>
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

/* ASCII-art by Felix Lee, see http://tigerfood.org/felix/ascii-art */

#include "ascii.h"

#ifdef ASCIIART

/*
 * Defining char-arrays like this seems to be a problem on HP-UX 1.x -
 * at least if we believe HP-UX 2.x's (linker) ld.
 */

char pics[2][PIC_Y][PIC_X] = 
{
{ "    |\\      _,,,---,,_      ",
  "    /,`.-'`'    -.  ;-;;,_  ",
  "   |,4-  ) )-,_..;\\ (  `'-' ",
  "  '---''(_/--'  `-'\\_)  fL  " },

{ "                            ",
  "     )\\._.,--....,'``.      ",
  "    /,   _.. \\   _\\  ;`._ ,.",
  "fL `._.-(,_..'--(,_..'`-.;.'" }

};

#endif /* ASCIIART */
