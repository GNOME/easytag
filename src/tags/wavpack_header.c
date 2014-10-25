/* EasyTAG - Tag editor for audio files.
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
 * Copyright (C) 2007 Maarten Maathuis (madman2003@gmail.com)
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

#include <config.h>

#ifdef ENABLE_WAVPACK

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <wavpack/wavpack.h>

#include "easytag.h"
#include "et_core.h"
#include "misc.h"
#include "charset.h"
#include "wavpack_header.h"


gboolean
wavpack_header_read_file_info (const gchar *filename,
                               ET_File_Info *ETFileInfo,
                               GError **error)
{
    WavpackContext *wpc;
    gchar message[80];

    g_return_val_if_fail (filename != NULL && ETFileInfo != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    /* TODO: Use WavpackOpenFileInputEx() instead. */
    wpc = WavpackOpenFileInput (filename, message, 0, 0);

    if (wpc == NULL)
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "%s", message);
        return FALSE;
    }

    ETFileInfo->version     = WavpackGetVersion(wpc);
    /* .wvc correction file not counted */
    ETFileInfo->bitrate     = WavpackGetAverageBitrate(wpc, 0)/1000;
    ETFileInfo->samplerate  = WavpackGetSampleRate(wpc);
    ETFileInfo->mode        = WavpackGetNumChannels(wpc);
    ETFileInfo->size        = WavpackGetFileSize(wpc);
    ETFileInfo->duration    = WavpackGetNumSamples(wpc)/ETFileInfo->samplerate;

    WavpackCloseFile(wpc);

    return TRUE;
}

EtFileHeaderFields *
et_wavpack_header_display_file_info_to_ui (const ET_File *ETFile)
{
    EtFileHeaderFields *fields;
    ET_File_Info *info;
    gchar *time = NULL;
    gchar *time1 = NULL;
    gchar *size = NULL;
    gchar *size1 = NULL;

    info = ETFile->ETFileInfo;
    fields = g_slice_new (EtFileHeaderFields);

    fields->description = _("Wavpack File");

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
    g_free (fields->mode);

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
et_wavpack_file_header_fields_free (EtFileHeaderFields *fields)
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

#endif /* ENABLE_WAVPACK */
