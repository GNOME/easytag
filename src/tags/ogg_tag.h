/* EasyTAG - Tag editor for MP3 and Ogg Vorbis files
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

#ifndef ET_OGG_TAG_H_
#define ET_OGG_TAG_H_

#include "config.h"

#include <glib.h>

#ifdef ENABLE_OGG

#include "vcedit.h"
#include "et_core.h"

G_BEGIN_DECLS

gboolean ogg_tag_read_file_tag (GFile *file, File_Tag *FileTag, GError **error);
gboolean ogg_tag_write_file_tag (const ET_File *ETFile, GError **error);

void et_add_file_tags_from_vorbis_comments (vorbis_comment *vc, File_Tag *FileTag);
void et_add_vorbis_comments_from_file_tags (vorbis_comment *vc, File_Tag *FileTag);

G_END_DECLS

#endif /* ENABLE_OGG */

#endif /* ET_OGG_TAG_H_ */
