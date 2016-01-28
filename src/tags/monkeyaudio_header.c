/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
 * Copyright (C) 2001-2003  Jerome Couderc <easytag@gmail.com>
 * Copyright (C) 2002-2003  Artur Polaczyñski <artii@o2.pl>
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

#include "et_core.h"
#include "misc.h"
#include "monkeyaudio_header.h"
#include "libapetag/info_mac.h"

gboolean
et_mac_header_read_file_info (GFile *file,
                              ET_File_Info *ETFileInfo,
                              GError **error)
{
    StreamInfoMac Info;

    g_return_val_if_fail (file != NULL && ETFileInfo != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    if (!info_mac_read (file, &Info, error))
    {
        return FALSE;
    }

    ETFileInfo->mpc_profile   = g_strdup(Info.CompresionName);
    ETFileInfo->version       = Info.Version;
    ETFileInfo->bitrate       = Info.Bitrate/1000.0;
    ETFileInfo->samplerate    = Info.SampleFreq;
    ETFileInfo->mode          = Info.Channels;
    ETFileInfo->size          = Info.FileSize;
    ETFileInfo->duration      = Info.Duration/1000;

    return TRUE;
}

EtFileHeaderFields *
et_mac_header_display_file_info_to_ui (const ET_File *ETFile)
{
    EtFileHeaderFields *fields;
    ET_File_Info *info;
    gchar *time  = NULL;
    gchar *time1 = NULL;
    gchar *size  = NULL;
    gchar *size1 = NULL;

    info = ETFile->ETFileInfo;
    fields = g_slice_new (EtFileHeaderFields);

    fields->description = _("Monkey's Audio File");

    /* Mode changed to profile name  */
    fields->mode_label = _("Profile:");
    fields->mode = info->mpc_profile;

    /* Bitrate */
    fields->bitrate = g_strdup_printf (_("%d kb/s"), info->bitrate);

    /* Samplerate */
    fields->samplerate = g_strdup_printf (_("%d Hz"), info->samplerate);

    /* Version changed to encoder version */
    fields->version_label = _("Encoder:");
    fields->version = g_strdup_printf ("%i.%i", info->version / 1000,
                                       info->version % 1000);

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
et_mac_file_header_fields_free (EtFileHeaderFields *fields)
{
    g_return_if_fail (fields != NULL);

    g_free (fields->bitrate);
    g_free (fields->samplerate);
    g_free (fields->version);
    g_free (fields->size);
    g_free (fields->duration);
    g_slice_free (EtFileHeaderFields, fields);
}
