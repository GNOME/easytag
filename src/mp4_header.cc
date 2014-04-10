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
 * Mp4_Header_Read_File_Info:
 *
 * Get header info into the ETFileInfo structure
 */
gboolean Mp4_Header_Read_File_Info (gchar *filename, ET_File_Info *ETFileInfo)
{
    TagLib::MP4::Tag *tag;
    const TagLib::MP4::Properties *properties;

    g_return_val_if_fail (filename != NULL && ETFileInfo != NULL, FALSE);

    /* Get size of file */
    ETFileInfo->size = et_get_file_size (filename);

    TagLib::MP4::File mp4file (filename);

    if (!mp4file.isOpen ())
    {
        gchar *filename_utf8 = filename_to_display (filename);
        Log_Print (LOG_ERROR, _("Error while opening file: '%s' (%s)."),
                   filename_utf8,_("MP4 format invalid"));
        g_free (filename_utf8);
        return FALSE;
    }

    if (!(tag = mp4file.tag ()))
    {
        gchar *filename_utf8 = filename_to_display (filename);
        Log_Print (LOG_ERROR, _("File contains no audio track: '%s'"),
                   filename_utf8);
        g_free (filename_utf8);
        return FALSE;
    }

    properties = mp4file.audioProperties ();

    if (properties == NULL)
    {
        gchar *filename_utf8 = filename_to_display (filename);
        Log_Print (LOG_ERROR, _("Error reading properties from file: '%s'"),
                   filename_utf8);
        g_free (filename_utf8);
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
 * Mp4_Header_Display_File_Info_To_UI:
 *
 * Display header info in the main window
 */
gboolean Mp4_Header_Display_File_Info_To_UI(gchar *filename, ET_File_Info *ETFileInfo)
{
    gchar *text;
    gchar *time = NULL;
    gchar *time1 = NULL;
    gchar *size = NULL;
    gchar *size1 = NULL;

    /* MPEG, Layer versions */
    gtk_label_set_text(GTK_LABEL(VersionLabel),ETFileInfo->mpc_version);
    //text = g_strdup_printf("%d",ETFileInfo->version);
    gtk_label_set_text(GTK_LABEL(VersionValueLabel),ETFileInfo->mpc_profile);
    //g_free(text);

    /* Bitrate */
    if (ETFileInfo->variable_bitrate)
        text = g_strdup_printf(_("~%d kb/s"),ETFileInfo->bitrate);
    else
        text = g_strdup_printf(_("%d kb/s"),ETFileInfo->bitrate);
    gtk_label_set_text(GTK_LABEL(BitrateValueLabel),text);
    g_free(text);

    /* Samplerate */
    text = g_strdup_printf(_("%d Hz"),ETFileInfo->samplerate);
    gtk_label_set_text(GTK_LABEL(SampleRateValueLabel),text);
    g_free(text);

    /* Mode */
    /* mpeg4ip library seems to always return -1 */
    gtk_label_set_text(GTK_LABEL(ModeLabel),_("Channels:"));
    if( ETFileInfo->mode == -1 )
        text = g_strdup_printf("Unknown");
    else
        text = g_strdup_printf("%d",ETFileInfo->mode);
    gtk_label_set_text(GTK_LABEL(ModeValueLabel),text);
    g_free(text);

    /* Size */
    size  = g_format_size (ETFileInfo->size);
    size1 = g_format_size (ETCore->ETFileDisplayedList_TotalSize);
    text  = g_strdup_printf("%s (%s)",size,size1);
    gtk_label_set_text(GTK_LABEL(SizeValueLabel),text);
    if (size)  g_free(size);
    if (size1) g_free(size1);
    g_free(text);

    /* Duration */
    time  = Convert_Duration(ETFileInfo->duration);
    time1 = Convert_Duration(ETCore->ETFileDisplayedList_TotalDuration);
    text  = g_strdup_printf("%s (%s)",time,time1);
    gtk_label_set_text(GTK_LABEL(DurationValueLabel),text);
    if (time)  g_free(time);
    if (time1) g_free(time1);
    g_free(text);

    return TRUE;
}
