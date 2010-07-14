/* ape_tag.h - 2001/11/08 */
/*
 *  EasyTAG - Tag editor for MP3, Ogg Vorbis and MPC files
 *  Copyright (C) 2001-2003  Jerome Couderc <easytag@gmail.com>
 *  Copyright (C) 2002-2003  Artur Polaczyñski <artii@o2.pl>
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


#ifndef __APE_TAG_H__
#define __APE_TAG_H__


#include "et_core.h"

/***************
 * Declaration *
 ***************/


/**************
 * Prototypes *
 **************/
gboolean Ape_Tag_Read_File_Tag  (gchar *filename, File_Tag *FileTag);
gboolean Ape_Tag_Write_File_Tag (ET_File *ETFile);

#endif /* __APE_TAG_H__ */
