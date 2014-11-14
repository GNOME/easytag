/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
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

#ifndef ET_ID3_PRIVATE_H_
#define ET_ID3_PRIVATE_H_

#include "config.h"

#include <glib.h>
#include "et_core.h"

G_BEGIN_DECLS

#define ID3_INVALID_GENRE 255

gboolean id3tag_write_file_v24tag (const ET_File *ETFile, GError **error);
guchar Id3tag_String_To_Genre (const gchar *genre);
gchar *et_id3tag_get_tpos_from_file_tag (const File_Tag *file_tag);

#ifdef ENABLE_MP3
#include <id3tag.h>
gboolean et_id3tag_fill_file_tag_from_id3tag (struct id3_tag *tag, File_Tag *file_tag);
void et_id3tag_set_id3_tag_from_file_tag (const File_Tag *FileTag, struct id3_tag *v1tag, struct id3_tag *v2tag, gboolean *strip_tags);
#ifdef ENABLE_ID3LIB
#include <id3.h>
gchar * Id3tag_Get_Error_Message (ID3_Err error);
void et_id3tag_set_id3tag_from_file_tag (const File_Tag *FileTag, ID3Tag *id3_tag, gboolean *strip_tags);
#endif /* ENABLE_ID3LIB */
#endif /* ENABLE_MP3 */

G_END_DECLS

#endif /* ET_ID3TAG_H_ */
