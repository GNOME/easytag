/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
 * Copyright (C) 2001-2003  Jerome Couderc <easytag@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef ET_ID3TAG_H_
#define ET_ID3TAG_H_

#include <glib.h>
#include "et_core.h"

G_BEGIN_DECLS

#define ID3_INVALID_GENRE 255

gboolean Id3tag_Read_File_Tag (const gchar *filename, File_Tag *FileTag);
gboolean Id3tag_Write_File_v24Tag (ET_File *ETFile);
gboolean Id3tag_Write_File_Tag    (ET_File *ETFile);

const gchar * Id3tag_Genre_To_String (unsigned char genre_code);
guchar Id3tag_String_To_Genre (const gchar *genre);

gchar *et_id3tag_get_tpos_from_file_tag (File_Tag *file_tag);

G_END_DECLS

#endif /* ET_ID3TAG_H_ */
