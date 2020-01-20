/* human.c -- print human readable file size

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004,
   2005, 2006, 2007 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Paul Eggert and Larry McVoy.  */

#include "human.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char power_letter[] =
        {
                0,	/* not used */
                'K',	/* kibi ('k' for kilo is a special case) */
                'M',	/* mega or mebi */
                'G',	/* giga or gibi */
                'T',	/* tera or tebi */
                'P',	/* peta or pebi */
                'E',	/* exa or exbi */
                'Z',	/* zetta or 2**70 */
                'Y'	/* yotta or 2**80 */
        };



/* The default block size used for output.  This number may change in
   the future as disks get larger.  */
#ifndef DEFAULT_BLOCK_SIZE
# define DEFAULT_BLOCK_SIZE 1024
#endif
