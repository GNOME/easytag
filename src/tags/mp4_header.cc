/*
 *  EasyTAG - Tag editor for audio files
 *  Copyright (C) 2012-1014  David King <amigadave@amigadave.com>
 *  Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
 *  Copyright (C) 2005  Stewart Whitman <swhitman@cox.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/* This file is intended to be included directly in mp4_tag.cc */

/*
 * et_mp4_header_read_file_info:
 *
 * Get header info into the ETFileInfo structure
 */
gboolean
et_mp4_header_read_file_info (GFile *file,
                              ET_File_Info *ETFileInfo,
                              GError **error)
{
    GFileInfo *info;
    const TagLib::MP4::Properties *properties;

    g_return_val_if_fail (file != NULL && ETFileInfo != NULL, FALSE);

    /* Get size of file */
    info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_SIZE,
                              G_FILE_QUERY_INFO_NONE, NULL, error);

    if (!info)
    {
        return FALSE;
    }

    ETFileInfo->size = g_file_info_get_size (info);
    g_object_unref (info);

    GIO_InputStream stream (file);

    if (!stream.isOpen ())
    {
        const GError *tmp_error = stream.getError ();

        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Error while opening file: %s"), tmp_error->message);
        return FALSE;
    }

    TagLib::MP4::File mp4file (&stream);

    if (!mp4file.isOpen ())
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Error while opening file: %s"),
                     _("MP4 format invalid"));
        return FALSE;
    }

    properties = mp4file.audioProperties ();

    if (properties == NULL)
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "%s",
                     _("Error reading properties from file"));
        return FALSE;
    }

    /* Get format/subformat */
    {
        ETFileInfo->mpc_version = g_strdup ("MPEG");

        switch (properties->codec ())
        {
            case TagLib::MP4::Properties::AAC:
                ETFileInfo->mpc_profile = g_strdup ("4, AAC");
                break;
            case TagLib::MP4::Properties::ALAC:
                ETFileInfo->mpc_profile = g_strdup ("4, ALAC");
                break;
            case TagLib::MP4::Properties::Unknown:
            default:
                ETFileInfo->mpc_profile = g_strdup ("4, Unknown");
                break;
        };
    }

    ETFileInfo->version = 4;
    ETFileInfo->mpeg25 = 0;
    ETFileInfo->layer = 14;

    ETFileInfo->variable_bitrate = TRUE;
    ETFileInfo->bitrate = properties->bitrate ();
    ETFileInfo->samplerate = properties->sampleRate ();
    ETFileInfo->mode = properties->channels ();
    ETFileInfo->duration = properties->length ();

    return TRUE;
}

/*
 * et_mp4_header_display_file_info_to_ui:
 *
 * Display header info in the main window
 */
EtFileHeaderFields *
et_mp4_header_display_file_info_to_ui (const ET_File *ETFile)
{
    EtFileHeaderFields *fields;
    ET_File_Info *info;
    gchar *time = NULL;
    gchar *time1 = NULL;
    gchar *size = NULL;
    gchar *size1 = NULL;

    info = ETFile->ETFileInfo;
    fields = g_slice_new (EtFileHeaderFields);

    fields->description = _("MP4/AAC File");

    /* MPEG, Layer versions */
    fields->version_label = info->mpc_version;
    fields->version = info->mpc_profile;

    /* Bitrate */
    if (info->variable_bitrate)
    {
        fields->bitrate = g_strdup_printf (_("~%d kb/s"), info->bitrate);
    }
    else
    {
        fields->bitrate = g_strdup_printf (_("%d kb/s"), info->bitrate);
    }

    /* Samplerate */
    fields->samplerate = g_strdup_printf (_("%d Hz"), info->samplerate);

    /* Mode */
    /* mpeg4ip library seems to always return -1 */
    fields->mode_label = _("Channels:");

    if (info->mode == -1)
    {
        fields->mode = g_strdup ("Unknown");
    }
    else
    {
        fields->mode = g_strdup_printf ("%d", info->mode);
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
et_mp4_file_header_fields_free (EtFileHeaderFields *fields)
{
    g_return_if_fail (fields != NULL);

    g_free (fields->bitrate);
    g_free (fields->samplerate);
    g_free (fields->mode);
    g_free (fields->size);
    g_free (fields->duration);
    g_slice_free (EtFileHeaderFields, fields);
}
