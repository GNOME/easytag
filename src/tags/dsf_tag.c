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

#ifdef ENABLE_ID3LIB
#include <id3.h>
#include "id3lib/id3_bugfix.h"
#endif

#define DSF_METADATA_CHUNK_OFFSET 20

/*
 * guint64_to_le_bytes:
 * @field: the 64-bit integer to convert
 * @str: pointer to a buffer of at least 8 bytes
 *
 * Write the 64-bit integer to the next 8 characters, in little-endian byte
 * order.
 */
static void
guint64_to_le_bytes (guint64 field,
                     guchar *str)
{
    gsize i;

    for (i = 0; i < 8; i++)
    {
        str[i] = (field >> i * 8) & 0xFF;
    }
}

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

#ifdef ENABLE_ID3LIB

static gboolean
et_dsf_tag_write_id3v23 (const File_Tag *FileTag,
                         const guchar *id3_buffer,
                         long tagsize,
                         GOutputStream *ostream,
                         GError **error)
{
    GSeekable *seekable;
    goffset id3_offset;
    ID3Tag *tag;
    gboolean strip_tags = TRUE;
    size_t new_tagsize;
    guchar *new_tag_buffer;
    gsize bytes_written;

    /* TODO: Call id3tag_check_if_id3lib_is_buggy(). */

    seekable = G_SEEKABLE (ostream);
    /* Offset of the ID3v2 tag is at the current stream position. */
    id3_offset = g_seekable_tell (seekable);

    if ((tag = ID3Tag_New ()) == NULL)
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_NOMEM, "%s",
                     g_strerror (ENOMEM));
        goto err;
    }

    /* Copy the header out to a separate buffer, as id3lib expects the header
     * and remainder of the tag to be processed separately. */
    if (tagsize > ID3_TAGHEADERSIZE)
    {
        const guchar id3_header[ID3_TAGHEADERSIZE];
        ID3_Err id3_err;

        memcpy (&id3_header, id3_buffer, ID3_TAGHEADERSIZE);

        if ((id3_err = ID3Tag_Parse (tag, id3_header,
                                     &id3_buffer[ID3_TAGHEADERSIZE]))
            != ID3E_NoError)
        {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                         Id3tag_Get_Error_Message (id3_err));
            ID3Tag_Delete (tag);
            goto err;
        }
    }

    ID3Tag_SetPadding (tag, TRUE);

    /* Fill the tag with the new values. */
    et_id3tag_set_id3tag_from_file_tag (FileTag, tag, &strip_tags);

    /* Truncate the file and update the metadata offset if the tags should be
     * stripped. */
    if (strip_tags)
    {
        guchar metadata_offset[8] = { 0, };

        ID3Tag_Delete (tag);

        if (!g_seekable_can_truncate (seekable))
        {
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_BADF, "%s",
                         g_strerror (EBADF));
            goto err;
        }

        if (!g_seekable_truncate (seekable, id3_offset, NULL, error))
        {
            goto err;
        }

        if (!g_seekable_seek (seekable, DSF_METADATA_CHUNK_OFFSET, G_SEEK_SET,
                              NULL, error))
        {
            goto err;
        }

        if (!g_output_stream_write_all (ostream, metadata_offset, 8,
                                        &bytes_written, NULL, error))
        {
            goto err;
        }
        else if (bytes_written != 8)
        {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                         _("Error writing tags to file"));
            goto err;
        }

        return TRUE;
    }

    new_tagsize = ID3Tag_Size (tag);

    /* FIXME: This is probably a bug in id3lib, as a size of 0 should only be
     * returned if there are no valid frames, and frames were added earlier in
     * et_id3tag_set_id3tag_from_file_tag(). */
    if (new_tagsize == 0)
    {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                     _("Error writing tags to file"));
        ID3Tag_Delete (tag);
        goto err;
    }

    /* FIXME: The return vlaue of ID3Tag_Size() should include the header and
     * tag size, so this looks like an overallocation. However, Valgrind
     * complains about reading into unallocated memory otherwise. */
    new_tag_buffer = g_malloc0 (new_tagsize + ID3_TAGHEADERSIZE);
    /* The call to ID3Tag_Size() gives an overestimate, but ID3Tag_Render()
     * returns the real size of the tag. */
    new_tagsize = ID3Tag_Render (tag, new_tag_buffer, ID3TT_ID3V2);

    ID3Tag_Delete (tag);

    /* Only truncate the file if the new tag is smaller than the old one. */
    if (new_tagsize < tagsize)
    {
        if (!g_seekable_can_truncate (seekable))
        {
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_BADF, "%s",
                         g_strerror (EBADF));
            g_free (new_tag_buffer);
            goto err;
        }

        if (!g_seekable_truncate (seekable, id3_offset + new_tagsize, NULL,
                                  error))
        {
            g_free (new_tag_buffer);
            goto err;
        }
    }

    if (!g_output_stream_write_all (ostream, new_tag_buffer, new_tagsize,
                                    &bytes_written, NULL, error))
    {
        g_free (new_tag_buffer);
        goto err;
    }
    else if (bytes_written != new_tagsize)
    {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                     _("Error writing tags to file"));
        g_free (new_tag_buffer);
        goto err;
    }

    g_free (new_tag_buffer);
    return TRUE;

err:
    return FALSE;
}

#endif

static gboolean
et_dsf_tag_write_id3v24 (const File_Tag *FileTag,
                         const guchar *id3_buffer,
                         long tagsize,
                         GOutputStream *ostream,
                         GError **error)
{
    struct id3_tag *tag;
    GSeekable *seekable;
    gboolean strip_tags = TRUE;
    gsize bytes_written;
    guchar *new_tag_buffer;
    long new_tagsize;
    goffset id3_offset;

    if (tagsize == 0)
    {
        /* No ID3v2 tag. */
        tag = id3_tag_new ();
    }
    else
    {
        /* Create an empty tag if there was a problem reading the existing
         * tag. */
        if (!(tag = id3_tag_parse ((id3_byte_t const *)id3_buffer, tagsize)))
        {
            tag = id3_tag_new ();
        }
    }

    seekable = G_SEEKABLE (ostream);
    /* Offset of the ID3v2 tag is at the current stream position. */
    id3_offset = g_seekable_tell (seekable);

    /* Set padding. */
    if ((tag->paddedsize < 1024) || ((tag->paddedsize > 4096)
                                     && (tagsize < 1024)))
    {
        tag->paddedsize = 1024;
    }

    /* Set options. */
    id3_tag_options (tag, ID3_TAG_OPTION_UNSYNCHRONISATION
                     | ID3_TAG_OPTION_APPENDEDTAG
                     | ID3_TAG_OPTION_ID3V1
                     | ID3_TAG_OPTION_CRC
                     | ID3_TAG_OPTION_COMPRESSION,
                     0);

    if (g_settings_get_boolean (MainSettings, "id3v2-crc32"))
    {
        id3_tag_options (tag, ID3_TAG_OPTION_CRC, ~0);
    }

    if (g_settings_get_boolean (MainSettings, "id3v2-compression"))
    {
        id3_tag_options (tag, ID3_TAG_OPTION_COMPRESSION, ~0);
    }

    /* Only set the IDv2 tag. */
    et_id3tag_set_id3_tag_from_file_tag (FileTag, NULL, tag, &strip_tags);

    /* Truncate the file and update the metadata offset if the tags should be
     * stripped. */
    if (strip_tags)
    {
        guchar metadata_offset[8] = { 0, };

        id3_tag_delete (tag);

        if (!g_seekable_can_truncate (seekable))
        {
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_BADF, "%s",
                         g_strerror (EBADF));
            goto err;
        }

        if (!g_seekable_truncate (seekable, id3_offset, NULL, error))
        {
            goto err;
        }

        if (!g_seekable_seek (seekable, DSF_METADATA_CHUNK_OFFSET, G_SEEK_SET,
                              NULL, error))
        {
            goto err;
        }

        if (!g_output_stream_write_all (ostream, metadata_offset, 8,
                                        &bytes_written, NULL, error))
        {
            goto err;
        }
        else if (bytes_written != 8)
        {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                         _("Error writing tags to file"));
            goto err;
        }

        return TRUE;
    }

    new_tagsize = id3_tag_render (tag, NULL);
    new_tag_buffer = g_malloc (new_tagsize);

    if ((new_tagsize = id3_tag_render (tag, new_tag_buffer)) == 0)
    {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                     _("Error writing tags to file"));
        id3_tag_delete (tag);
        g_free (new_tag_buffer);
        goto err;
    }

    id3_tag_delete (tag);

    /* Only truncate the file if the new tag is smaller than the old one. */
    if (new_tagsize < tagsize)
    {
        if (!g_seekable_can_truncate (seekable))
        {
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_BADF, "%s",
                         g_strerror (EBADF));
            g_free (new_tag_buffer);
            goto err;
        }

        if (!g_seekable_truncate (seekable, id3_offset + new_tagsize, NULL,
                                  error))
        {
            g_free (new_tag_buffer);
            goto err;
        }
    }

    if (!g_output_stream_write_all (ostream, new_tag_buffer, new_tagsize,
                                    &bytes_written, NULL, error))
    {
        g_free (new_tag_buffer);
        goto err;
    }
    else if (bytes_written != new_tagsize)
    {
        g_free (new_tag_buffer);
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                     _("Error writing tags to file"));
        goto err;
    }

    g_free (new_tag_buffer);

    return TRUE;

err:
    return FALSE;
}

gboolean
et_dsf_tag_write_file_tag (const ET_File *ETFile,
                           GError **error)
{
    const File_Tag *FileTag;
    const gchar *filename;
    GFile *file;
    GFileIOStream *iostream;
    GInputStream *istream;
    GOutputStream *ostream;
    guchar header[DSF_HEADER_LENGTH];
    gsize bytes_read;
    guint64 file_size_header;
    GFileInfo *info;
    goffset file_size;
    guint64 id3_offset;
    GSeekable *seekable;
    guchar id3_query[ID3_TAG_QUERYSIZE];
    guchar *id3_buffer;
    long tagsize;
    goffset eos_offset;

    g_return_val_if_fail (ETFile != NULL && ETFile->FileTag != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    FileTag = (File_Tag *)ETFile->FileTag->data;
    filename = ((File_Name *)ETFile->FileNameCur->data)->value;

    file = g_file_new_for_path (filename);
    iostream = g_file_open_readwrite (file, NULL, error);
    g_object_unref (file);

    if (!iostream)
    {
        return FALSE;
    }

    istream = g_io_stream_get_input_stream (G_IO_STREAM (iostream));

    /* Read the complete header from the file. */
    if (!g_input_stream_read_all (istream, &header, DSF_HEADER_LENGTH,
                                  &bytes_read, NULL, error))
    {
        g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %d "
                 "bytes were read", bytes_read, DSF_HEADER_LENGTH);
        goto err;
    }

    if (memcmp (&header, "DSD ", 4) != 0)
    {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                     _("Not a DSF file"));
        goto err;
    }

    if (memcmp (&header[28], "fmt ", 4) != 0)
    {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                     _("Not a DSF file"));
        goto err;
    }

    /* 12 (8 bytes) total file size. */
    file_size_header = guint64_from_le_bytes (&header[DSF_HEADER_FILE_SIZE_OFFSET]);

    info = g_file_input_stream_query_info (G_FILE_INPUT_STREAM (istream),
                                           G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                           NULL, error);

    if (info == NULL)
    {
        goto err;
    }

    file_size = g_file_info_get_size (info);
    g_object_unref (info);

    if (file_size_header != file_size)
    {
        g_debug ("DSF file is %" G_GUINT64_FORMAT
                 " bytes, but the file size stored in the header is %"
                 G_GOFFSET_FORMAT " bytes", file_size, file_size_header);
    }

    /* 20 (8 bytes) metadata chunk offset, or 0 if no tag is present. */
    id3_offset = guint64_from_le_bytes (&header[DSF_METADATA_CHUNK_OFFSET]);

    if (id3_offset >= file_size)
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

    if (id3_offset == 0)
    {
        /* No ID3v2 tag. */
        id3_offset = file_size;
        id3_buffer = NULL;
        tagsize = 0;
    }
    else
    {

        /* Read in the existing ID3v2 tag, using libid3tag to determine the
         * size. */

        /* Seek to the start of the metadata chunk. */
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

        /* FIXME: id3lib seems to read past the end of a buffer that is exactly
         * the right size, so over-allocate. */
        id3_buffer = g_malloc0 (tagsize + ID3_TAG_QUERYSIZE);
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
    }

    /* No reading from this point forward. */
    g_input_stream_close (istream, NULL, NULL);

    ostream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));
    seekable = G_SEEKABLE (ostream);

    /* Seek to the start of the metadata chunk. */
    if (!g_seekable_seek (seekable, id3_offset, G_SEEK_SET, NULL, error))
    {
        g_free (id3_buffer);
        goto err;
    }

#ifdef ENABLE_ID3LIB
    if (g_settings_get_boolean (MainSettings, "id3v2-version-4"))
    {
#endif
        if (!et_dsf_tag_write_id3v24 (FileTag, id3_buffer, tagsize, ostream,
                                      error))
        {
            g_free (id3_buffer);
            goto err;
        }
#ifdef ENABLE_ID3LIB
    }
    else
    {
        if (!et_dsf_tag_write_id3v23 (FileTag, id3_buffer, tagsize, ostream,
                                      error))
        {
            g_free (id3_buffer);
            goto err;
        }
    }
#endif

    g_free (id3_buffer);

    eos_offset = g_seekable_tell (seekable);

    /* If the new tag caused the file to change in size, update the size in the
     * DSF header. */
    if (eos_offset != file_size)
    {
        guchar new_file_size[8];
        gsize bytes_written;

        if (!g_seekable_seek (seekable, DSF_HEADER_FILE_SIZE_OFFSET,
                              G_SEEK_SET, NULL, error))
        {
            goto err;
        }

        guint64_to_le_bytes (eos_offset, &new_file_size[0]);

        if (!g_output_stream_write_all (ostream, new_file_size, 8,
                                        &bytes_written, NULL, error))
        {
            goto err;
        }
        else if (bytes_written != 8)
        {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                         _("Error writing tags to file"));
            goto err;
        }
    }

    g_object_unref (iostream);
    return TRUE;

err:
    g_object_unref (iostream);
    return FALSE;
}

#endif /* ENABLE_MP3 */
