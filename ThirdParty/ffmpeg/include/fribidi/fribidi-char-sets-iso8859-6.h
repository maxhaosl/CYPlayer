/* FriBidi
 * fribidi-char-sets-iso8859-6.h - ISO8859-6 character set conversion routines
 *
 * Authors:
 *   Behdad Esfahbod, 2001, 2002, 2004
 *   Dov Grobgeld, 1999, 2000
 *
 * Copyright (C) 2004 Sharif FarsiWeb, Inc
 * Copyright (C) 2001,2002 Behdad Esfahbod
 * Copyright (C) 1999,2000 Dov Grobgeld
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library, in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA
 * 
 * For licensing issues, contact <fribidi.license@gmail.com>.
 */

#ifndef _FRIBIDI_CHAR_SETS_ISO8859_6_H
#define _FRIBIDI_CHAR_SETS_ISO8859_6_H

#include "fribidi-common.h"

#include "fribidi-types.h"

#include "fribidi-begindecls.h"

#define fribidi_char_set_name_iso8859_6 "ISO8859-6"
#define fribidi_char_set_title_iso8859_6 "ISO8859-6 (Arabic)"
#define fribidi_char_set_desc_iso8859_6 NULL

FriBidiChar fribidi_iso8859_6_to_unicode_c (
  char ch
);

char fribidi_unicode_to_iso8859_6_c (
  FriBidiChar uch
);

#include "fribidi-enddecls.h"

#endif /* !_FRIBIDI_CHAR_SETS_ISO8859_6_H */
/* Editor directions:
 * vim:textwidth=78:tabstop=8:shiftwidth=2:autoindent:cindent
 */
