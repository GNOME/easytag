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
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include <glib/gi18n.h>

#include "dsf_header.h"

#include "easytag.h"
#include "et_core.h"
#include "misc.h"
#include "tag_private.h"

#define DSF_HEADER_LENGTH 80
#define DSF_HEADER_MAGIC "DSD "
#define DSF_HEADER_FORMAT_MAGIC "fmt "

gboolean
et_dsf_header_read_file_info (GFile *file,
                              ET_File_Info *ETFileInfo,
                              GError **error)
{
    GFileInputStream *file_istream;
    GInputStream *istream;
    guchar header[DSF_HEADER_LENGTH];
    gsize bytes_read;
    GFileInfo *info;
    guint64 file_size_header;
    goffset file_size;
    guint32 bps;
    guint64 n_samples;

    g_return_val_if_fail (file != NULL && ETFileInfo != NULL, FALSE);
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
        g_object_unref (file_istream);
        goto err;
    }

    info = g_file_input_stream_query_info (file_istream,
                                           G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                           NULL, error);
    g_object_unref (file_istream);

    if (info == NULL)
    {
        goto err;
    }

    file_size = g_file_info_get_size (info);
    g_object_unref (info);

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
    file_size_header = guint64_from_le_bytes (&header[12]);

    if (file_size_header != file_size)
    {
        g_debug ("DSF file is %" G_GUINT64_FORMAT
                 " bytes, but the file size stored in the header is %"
                 G_GOFFSET_FORMAT " bytes", file_size, file_size_header);
    }

    ETFileInfo->size = file_size;

    /* 40 (4 bytes) format version, normally 1. */

    /* 44 (4 bytes) format ID, normally 0. */
    ETFileInfo->version = guint32_from_le_bytes (&header[44]);

    /* 48 (4 bytes) channel type, normally 1-7. */
    ETFileInfo->mode = guint32_from_le_bytes (&header[52]);

    /* 52 (4 bytes) number of channels. */

    /* 56 (4 bytes) sampling frequency, 2822400 or 5644800 Hz. */
    ETFileInfo->samplerate = guint32_from_le_bytes (&header[56]);

    /* 60 (4 bytes) bits per sample, 1 or 8. */
    bps = guint32_from_le_bytes (&header[60]);

    /* 64 (8 bytes) sample count (per channel). */
    n_samples = guint64_from_le_bytes (&header[64]);

    /* 72 (4 bytes) block size per channel, 4096. */

    /* 76 (4 bytes) zero padded. */

    ETFileInfo->duration = n_samples / ETFileInfo->samplerate;
    ETFileInfo->bitrate = bps * (n_samples / ETFileInfo->duration / 1000);

    return TRUE;

err:
    return FALSE;
}

EtFileHeaderFields *
et_dsf_header_display_file_info_to_ui (const ET_File *ETFile)
{
    EtFileHeaderFields *fields;
    ET_File_Info *info;
    gchar *time = NULL;
    gchar *time1 = NULL;
    gchar *size = NULL;
    gchar *size1 = NULL;

    info = ETFile->ETFileInfo;
    fields = g_slice_new (EtFileHeaderFields);

    fields->description = _("DSF File");

    /* Encoder version */
    fields->version_label = _("Encoder:");

    switch (info->version)
    {
        case 0:
            fields->version = g_strdup (_("DSD raw"));
            break;
        default:
            fields->version = g_strdup_printf ("%d", info->version);
            break;
    }

    /* Bitrate */
    fields->bitrate = g_strdup_printf (_("%d kb/s"), info->bitrate);

    /* Samplerate */
    fields->samplerate = g_strdup_printf (_("%d Hz"), info->samplerate);

    /* Mode */
    fields->mode_label = _("Channels:");

    switch (info->mode)
    {
        case 1:
            fields->mode = g_strdup (_("Mono"));
            break;
        case 2:
            fields->mode = g_strdup (_("Stereo"));
            break;
        case 3:
            fields->mode = g_strdup_printf ("%d", info->mode);
            break;
        case 4:
            fields->mode = g_strdup (_("Quadrophonic"));
            break;
        case 5:
        case 6:
            fields->mode = g_strdup_printf ("%d", info->mode + 1);
            break;
        case 7:
            fields->mode = g_strdup (_("5.1"));
            break;
        default:
            fields->mode = g_strdup (_("Unknown"));
            break;
    }

    /* Size */
    size = g_format_size (info->size);
    size1 = g_format_size (ETCore->ETFileDisplayedList_TotalSize);
    fields->size = g_strdup_printf ("%s (%s)", size, size1);
    g_free (size);
    g_free (size1);

    /* Duration */
    time = Convert_Duration (info->duration);
    time1 = Convert_Duration (ETCore->ETFileDisplayedList_TotalDuration);
    fields->duration = g_strdup_printf ("%s (%s)", time, time1);
    g_free (time);
    g_free (time1);

    return fields;
}

void
et_dsf_file_header_fields_free (EtFileHeaderFields *fields)
{
    g_return_if_fail (fields != NULL);

    g_free (fields->version);
    g_free (fields->bitrate);
    g_free (fields->samplerate);
    g_free (fields->mode);
    g_free (fields->size);
    g_free (fields->duration);
    g_slice_free (EtFileHeaderFields, fields);
}
