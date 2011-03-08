/* wavpack_tag.c - 2007/02/15 */
/*
 *  EasyTAG - Tag editor for many file types
 *  Copyright (C) 2007 Maarten Maathuis (madman2003@gmail.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifndef __WAVPACK_TAG_H__
#define __WAVPACK_TAG_H__


#include <glib.h>
#include "et_core.h"

/***************
 * Declaration *
 ***************/

#define MAXLEN 1024

/**************
 * Prototypes *
 **************/
gboolean Wavpack_Tag_Read_File_Tag  (gchar *filename, File_Tag *FileTag);
gboolean Wavpack_Tag_Write_File_Tag (ET_File *ETFile);


#endif /* __WAVPACK_TAG_H__ */
