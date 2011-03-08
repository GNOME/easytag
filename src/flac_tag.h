/* flac_tag.h - 2003/12/27 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2001-2003  Jerome Couderc <easytag@gmail.com>
 *  Copyright (C) 2003       Pavel Minayev <thalion@front.ru>
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


#ifndef __FLAC_TAG_H__
#define __FLAC_TAG_H__


#include <glib.h>
#include "et_core.h"

/***************
 * Declaration *
 ***************/
const gchar *flac_error_msg;


/**************
 * Prototypes *
 **************/
gboolean Flac_Tag_Read_File_Tag  (gchar *filename, File_Tag *FileTag);
gboolean Flac_Tag_Write_File_Tag (ET_File *ETFile);


#endif /* __FLAC_TAG_H__ */
