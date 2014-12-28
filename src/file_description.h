/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef ET_FILE_DESCRIPTION_H_
#define ET_FILE_DESCRIPTION_H_

#include <glib.h>

G_BEGIN_DECLS

#include "core_types.h"

/*
 * Structure for descripting supported files
 */
typedef struct
{
    ET_File_Type FileType;    /* Type of file (ex: MP3) */
    const gchar *Extension; /* Extension (ex: ".mp3") */
    ET_Tag_Type  TagType;     /* Type of tag (ex: ID3) */
} ET_File_Description;

/*
 * Description of supported files
 */
extern const ET_File_Description ETFileDescription[];

/* Calculate the last index of the previous tab */
extern const gsize ET_FILE_DESCRIPTION_SIZE;

const gchar * ET_Get_File_Extension (const gchar *filename);
const ET_File_Description * ET_Get_File_Description (const gchar *filename);
gboolean et_file_is_supported (const gchar *filename);

G_END_DECLS

#endif /* !ET_FILE_DESCRIPTION_H_ */
