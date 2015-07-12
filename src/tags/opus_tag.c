/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014 Abhinav Jangda <abhijangda@hotmail.com>
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
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "config.h" /* For definition of ENABLE_OPUS */

#ifdef ENABLE_OPUS

#include <glib/gi18n.h>
#include <opus/opus.h>
#include <vorbis/codec.h>

#include "opus_tag.h"
#include "opus_header.h"
#include "ogg_tag.h"
#include "et_core.h"
#include "misc.h"
#include "picture.h"
#include "setting.h"
#include "charset.h"

#define MULTIFIELD_SEPARATOR " - "

/*
 * et_opus_tag_read_file_tag:
 * @filename: file from which to read tags
 * @FileTag: File_Tag to read tag into
 * @error: a GError or %NULL
 *
 * Read file tags and store into File_Tag.
 *
 * Returns: %TRUE if successful otherwise %FALSE
 */
gboolean
et_opus_tag_read_file_tag (GFile *gfile, File_Tag *FileTag,
                           GError **error)
{
    OggOpusFile *file;
    const OpusTags *tags;

    g_return_val_if_fail (gfile != NULL && FileTag != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    file = et_opus_open_file (gfile, error);

    if (!file)
    {
        g_assert (error == NULL || *error != NULL);
        return FALSE;
    }

    tags = op_tags (file, 0);

    /* The cast is safe according to the opusfile documentation. */
    et_add_file_tags_from_vorbis_comments ((vorbis_comment *)tags, FileTag);

    op_free (file);

    g_assert (error == NULL || *error == NULL);
    return TRUE;
}

#endif
