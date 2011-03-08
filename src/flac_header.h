/* flac_header.h - 2002/07/03 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
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


#ifndef __FLAC_HEADER_H__
#define __FLAC_HEADER_H__


#include "et_core.h"

/****************
 * Declarations *
 ****************/


/**************
 * Prototypes *
 **************/

gboolean Flac_Header_Read_File_Info          (gchar *filename, ET_File_Info *ETFileInfo);
gboolean Flac_Header_Display_File_Info_To_UI (gchar *filename, ET_File_Info *ETFileInfo);


#endif /* __FLAC_HEADER_H__ */
