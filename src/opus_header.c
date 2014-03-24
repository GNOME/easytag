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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <errno.h>

#include "easytag.h"
#include "opus_header.h"
#include "et_core.h"
#include "charset.h"
#include "log.h"
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

    info = g_file_query_info (gfile, G_FILE_ATTRIBUTE_STANDARD_SIZE,
                              G_FILE_QUERY_INFO_NONE, NULL, NULL);

    head = op_head (file, -1);
    ETFileInfo->version = head->version;
    ETFileInfo->bitrate = op_bitrate (file, -1);
    ETFileInfo->samplerate = head->input_sample_rate;
    ETFileInfo->mode = head->channel_count;

    if (info)
    {
        ETFileInfo->size = g_file_info_get_size (info);
        g_object_unref (info);
    }
    else
    {
        ETFileInfo->size = 0;
    }
    
    ETFileInfo->duration = op_pcm_total (file, -1);
    op_free (file);

    g_assert (error == NULL || *error == NULL);
    return TRUE;
}

/*
 * et_opus_header_display_file_info_to_ui:
 * @filename: file to display info of
 * @ETFileInfo: ET_File_Info to display information
 *
 * Display header info from ET_File_Info.
 *
 * Returns: %TRUE if successful, otherwise %FALSE
 */
gboolean
et_opus_header_display_file_info_to_ui (GFile *file,
                                        ET_File_Info *ETFileInfo)
{
    gchar *text;
    gchar *time = NULL;
    gchar *time1 = NULL;
    gchar *size = NULL;
    gchar *size1 = NULL;

    /* Encoder version */
    gtk_label_set_text (GTK_LABEL (VersionLabel), _("Encoder:"));
    text = g_strdup_printf ("%d", ETFileInfo->version);
    gtk_label_set_text (GTK_LABEL (VersionValueLabel), text);
    g_free (text);

    /* Bitrate */
    text = g_strdup_printf (_("%d kb/s"), ETFileInfo->bitrate);
    gtk_label_set_text (GTK_LABEL (BitrateValueLabel), text);
    g_free (text);

    /* Samplerate */
    text = g_strdup_printf (_("%d Hz"), ETFileInfo->samplerate);
    gtk_label_set_text (GTK_LABEL (SampleRateValueLabel), text);
    g_free (text);

    /* Mode */
    gtk_label_set_text (GTK_LABEL (ModeLabel), _("Channels:"));
    text = g_strdup_printf ("%d", ETFileInfo->mode);
    gtk_label_set_text (GTK_LABEL (ModeValueLabel), text);
    g_free (text);

    /* Size */
    size  = g_format_size (ETFileInfo->size);
    size1 = g_format_size (ETCore->ETFileDisplayedList_TotalSize);
    text  = g_strdup_printf ("%s (%s)", size, size1);
    gtk_label_set_text (GTK_LABEL (SizeValueLabel), text);
    g_free (size);
    g_free (size1);
    g_free (text);

    /* Duration */
    time  = Convert_Duration (ETFileInfo->duration);
    time1 = Convert_Duration (ETCore->ETFileDisplayedList_TotalDuration);
    text  = g_strdup_printf ("%s (%s)", time, time1);
    gtk_label_set_text (GTK_LABEL (DurationValueLabel), text);
    g_free (time);
    g_free (time1);
    g_free (text);

    return TRUE;
}

#endif /* ENABLE_OPUS */
