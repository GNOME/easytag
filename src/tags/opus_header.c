/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014 Abhinav Jangda (abhijangda@hotmail.com)
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

#include "opus_header.h"
#include "et_core.h"
#include "charset.h"
#include "misc.h"

/*
 * et_opus_error_quark:
 *
 * To get EtOpusError domain.
 *
 * Returns: GQuark for EtOpusError domain
 */
GQuark
et_opus_error_quark (void)
{
    return g_quark_from_static_string ("et-opus-error-quark");
}

/*
 * et_opus_open_file:
 * @filename: Filepath to open
 * @error: GError or %NULL
 *
 * Opens an Opus file.
 *
 * Returns: a OggOpusFile on success or %NULL on error.
 */
OggOpusFile *
et_opus_open_file (GFile *gfile, GError **error)
{
    OggOpusFile *file;
    gchar *path;
    int error_val;

    g_return_val_if_fail (error == NULL || *error == NULL, NULL);
    g_return_val_if_fail (gfile != NULL, NULL);

    path = g_file_get_path (gfile);
    /* Opusfile does the UTF-8 to UTF-16 translation on Windows
     * automatically. */
    file = op_open_file (path, &error_val);
    g_free (path);

    if (!file)
    {
        /* Got error while opening opus file */
        switch (error_val)
        {
            case OP_EREAD:
                g_set_error (error, ET_OPUS_ERROR, ET_OPUS_ERROR_READ,
                             "Error reading file");
                g_assert (error == NULL || *error != NULL);
                return NULL;

            case OP_EFAULT:
                g_set_error (error, ET_OPUS_ERROR, ET_OPUS_ERROR_FAULT,
                             "Memory allocation failure or internal library error");
                g_assert (error == NULL || *error != NULL);
                return NULL;

            case OP_EIMPL:
                g_set_error (error, ET_OPUS_ERROR, ET_OPUS_ERROR_IMPL,
                             "Stream used an unimplemented feature");
                g_assert (error == NULL || *error != NULL);
                return NULL;

            case OP_EINVAL:
                g_set_error (error, ET_OPUS_ERROR, ET_OPUS_ERROR_INVAL,
                             "seek () succeeded on this source but tell () did not");
                g_assert (error == NULL || *error != NULL);
                return NULL;

            case OP_ENOTFORMAT:
                g_set_error (error, ET_OPUS_ERROR, ET_OPUS_ERROR_NOTFORMAT,
                             "No logical stream found in a link");
                g_assert (error == NULL || *error != NULL);
                return NULL;

            case OP_EBADHEADER:
                g_set_error (error, ET_OPUS_ERROR, ET_OPUS_ERROR_BADHEADER,
                             "Corrupted header packet");
                g_assert (error == NULL || *error != NULL);
                return NULL;

            case OP_EVERSION:
                g_set_error (error, ET_OPUS_ERROR, ET_OPUS_ERROR_VERSION,
                             "ID header contained an unrecognized version number");
                g_assert (error == NULL || *error != NULL);
                return NULL;

            case OP_EBADLINK:
                g_set_error (error, ET_OPUS_ERROR, ET_OPUS_ERROR_BADLINK,
                             "Corrupted link found");
                g_assert (error == NULL || *error != NULL);
                return NULL;

            case OP_EBADTIMESTAMP:
                g_set_error (error, ET_OPUS_ERROR, ET_OPUS_ERROR_BADTIMESTAMP,
                             "First/last timestamp in a link failed checks");
                g_assert (error == NULL || *error != NULL);
                return NULL;
            default:
                g_assert_not_reached ();
                break;
        }
    }

    return file;
}

/*
 * et_opus_read_file_info:
 * @file: file to read info from
 * @ETFileInfo: ET_File_Info to put information into
 * @error: a GError or %NULL
 *
 * Read header information of an Opus file.
 *
 * Returns: %TRUE if successful otherwise %FALSE
 */
gboolean
et_opus_read_file_info (GFile *gfile, ET_File_Info *ETFileInfo,
                        GError **error)
{
    OggOpusFile *file;
    const OpusHead* head;
    GFileInfo *info;

    g_return_val_if_fail (gfile != NULL && ETFileInfo != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    file = et_opus_open_file (gfile, error);

    if (!file)
    {
        g_assert (error == NULL || *error != NULL);
        return FALSE;
    }

    /* FIXME: Improve error-checking. */
    head = op_head (file, -1);
    /* TODO: Read the vendor string from the Vorbis comment? */
    ETFileInfo->version = head->version;
    ETFileInfo->bitrate = op_bitrate (file, -1) / 1000;
    ETFileInfo->mode = head->channel_count;

    /* All Opus audio is encoded at 48 kHz, but the input sample rate can
     * differ, and then input_sample_rate will be set. */
    if (head->input_sample_rate != 0)
    {
        ETFileInfo->samplerate = head->input_sample_rate;
    }
    else
    {
        ETFileInfo->samplerate = 48000;
    }

    ETFileInfo->duration = op_pcm_total (file, -1) / 48000;
    op_free (file);

    info = g_file_query_info (gfile, G_FILE_ATTRIBUTE_STANDARD_SIZE,
                              G_FILE_QUERY_INFO_NONE, NULL, NULL);

    if (info)
    {
        ETFileInfo->size = g_file_info_get_size (info);
        g_object_unref (info);
    }
    else
    {
        ETFileInfo->size = 0;
    }

    g_assert (error == NULL || *error == NULL);
    return TRUE;
}

/*
 * et_opus_header_display_file_info_to_ui:
 * @ETFile: ET_File to display information
 *
 * Display header info from ET_File.
 *
 * Returns: a new #EtFileHeaderFields, free with
 * et_opus_file_header_fields_free()
 */
EtFileHeaderFields *
et_opus_header_display_file_info_to_ui (const ET_File *ETFile)
{
    EtFileHeaderFields *fields;
    ET_File_Info *info;
    gchar *time = NULL;
    gchar *time1 = NULL;
    gchar *size = NULL;
    gchar *size1 = NULL;

    info = ETFile->ETFileInfo;
    fields = g_slice_new (EtFileHeaderFields);

    fields->description = _("Opus File");

    /* Encoder version */
    fields->version_label = _("Encoder:");
    fields->version = g_strdup_printf ("%d", info->version);

    /* Bitrate */
    fields->bitrate = g_strdup_printf (_("%d kb/s"), info->bitrate);

    /* Samplerate */
    fields->samplerate = g_strdup_printf (_("%d Hz"), info->samplerate);

    /* Mode */
    fields->mode_label = _("Channels:");
    fields->mode = g_strdup_printf ("%d", info->mode);

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
et_opus_file_header_fields_free (EtFileHeaderFields *fields)
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

#endif /* ENABLE_OPUS */
