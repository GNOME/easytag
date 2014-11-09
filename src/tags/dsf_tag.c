/* EasyTAG - Tag editor for audio files.
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

#include "config.h"

#include <glib/gi18n.h>
#include <errno.h>

#include "dsf_tag.h"

#include "dsf_private.h"
#include "easytag.h"
#include "et_core.h"
#include "misc.h"
#include "tag_private.h"

#ifdef ENABLE_MP3

#include <id3tag.h>
#include "id3_tag.h"

#define DSF_METADATA_CHUNK_OFFSET 20

gboolean
et_dsf_tag_read_file_tag (GFile *file,
                          File_Tag *FileTag,
                          GError **error)
{
    GFileInputStream *file_istream;
    GInputStream *istream;
    guchar header[DSF_HEADER_LENGTH];
    gsize bytes_read;
    guint64 size;
    guint64 id3_offset;
    GSeekable *seekable;
    guchar id3_query[ID3_TAG_QUERYSIZE];
    long tagsize;
    guchar *id3_buffer;
    struct id3_tag *tag;
    gboolean update;

    g_return_val_if_fail (file != NULL && FileTag != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    file_istream = g_file_read (file, NULL, error);

    if (file_istream == NULL)
    {
        return FALSE;
    }

    istream = G_INPUT_STREAM (file_istream);

    /* Read the complete header from the file. */
    if (!g_input_stream_read_all (istream, &header, DSF_HEADER_LENGTH,
                                  &bytes_read, NULL, error))
    {
        g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %d "
                 "bytes were read", bytes_read, DSF_HEADER_LENGTH);
        goto err;
    }

    if (memcmp (&header, DSF_HEADER_MAGIC, 4) != 0)
    {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                     _("Not a DSF file"));
        goto err;
    }

    if (memcmp (&header[28], DSF_HEADER_FORMAT_MAGIC, 4) != 0)
    {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                     _("Not a DSF file"));
        goto err;
    }

    /* 12 (8 bytes) total file size. */
    size = guint64_from_le_bytes (&header[DSF_HEADER_FILE_SIZE_OFFSET]);
    
    /* 20 (8 bytes) metadata chunk offset, or 0 if no tag is present. */
    id3_offset = guint64_from_le_bytes (&header[DSF_METADATA_CHUNK_OFFSET]);
    
    if (id3_offset == 0)
    {
        /* No ID3v2 tag. */
        g_object_unref (file_istream);
        return TRUE;
    }

    if (id3_offset >= size)
    {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                     _("Invalid DSF header"));
        goto err;
    }

    seekable = G_SEEKABLE (istream);

    if (!g_seekable_can_seek (seekable))
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_BADF, "%s",
                     g_strerror (EBADF));
        goto err;
    }

    if (!g_seekable_seek (seekable, id3_offset, G_SEEK_SET, NULL, error))
    {
        goto err;
    }

    if (!g_input_stream_read_all (istream, id3_query, ID3_TAG_QUERYSIZE,
                                  &bytes_read, NULL, error))
    {
        goto err;
    }
    else if (bytes_read != ID3_TAG_QUERYSIZE)
    {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_PARTIAL_INPUT, "%s",
                     _("Error reading tags from file"));
        goto err;
    }

    if ((tagsize = id3_tag_query ((id3_byte_t const *)id3_query,
                                  ID3_TAG_QUERYSIZE)) <= ID3_TAG_QUERYSIZE)
    {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_PARTIAL_INPUT, "%s",
                     _("Error reading tags from file"));
        goto err;
    }

    id3_buffer = g_malloc0 (tagsize);
    /* Copy the query buffer. */
    memcpy (id3_buffer, id3_query, ID3_TAG_QUERYSIZE);

    if (!g_input_stream_read_all (istream, &id3_buffer[ID3_TAG_QUERYSIZE],
                                  tagsize - ID3_TAG_QUERYSIZE, &bytes_read,
                                  NULL, error))
    {
        g_free (id3_buffer);
        goto err;
    }
    else if (bytes_read != tagsize - ID3_TAG_QUERYSIZE)
    {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_PARTIAL_INPUT, "%s",
                     _("Error reading tags from file"));
        g_free (id3_buffer);
        goto err;
    }

    g_object_unref (file_istream);

    if ((tag = id3_tag_parse ((id3_byte_t const *)id3_buffer, tagsize)))
    {
        unsigned version = id3_tag_version (tag);

#ifdef ENABLE_ID3LIB
        /* Besides upgrading old tags, downgrade ID3v2.4 to ID3v2.3 */
        if (g_settings_get_boolean (MainSettings, "id3v2-version-4"))
        {
            update = (ID3_TAG_VERSION_MAJOR (version) < 4);
        }
        else
        {
            update = ((ID3_TAG_VERSION_MAJOR (version) < 3)
                      | (ID3_TAG_VERSION_MAJOR (version) == 4));
        }
#else
        update = (ID3_TAG_VERSION_MAJOR (version) < 4);
#endif
    }
    else
    {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                     _("Error reading tags from file"));
        g_free (id3_buffer);
        return FALSE;
    }

    g_free (id3_buffer);

    /* Check for an empty tag. */
    if (tag->nframes == 0)
    {
        id3_tag_delete (tag);
        return TRUE;
    }

    update |= et_id3tag_fill_file_tag_from_id3tag (tag, FileTag);

    if (update)
    {
        FileTag->saved = FALSE;
    }

    id3_tag_delete (tag);

    return TRUE;

err:
    g_object_unref (file_istream);
    return FALSE;
}

gboolean
et_dsf_tag_write_file_tag (const ET_File *ETFile,
                           GError **error)
{
    g_return_val_if_fail (ETFile != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    /* TODO: Implement! */
    /* TODO: Read header, seek to ID3v2 offset, truncate file, write new tag,
     * using ID3v2.3 or 2.4 depending on id3v2-version-4 setting. */
    return FALSE;
}

#endif /* ENABLE_MP3 */
