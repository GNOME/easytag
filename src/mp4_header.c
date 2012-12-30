/* mp4_header.c - 2005/02/05 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h" // For definition of ENABLE_MP4

#ifdef ENABLE_MP4

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "mp4_header.h"
#include "easytag.h"
#include "et_core.h"
#include "log.h"
#include "misc.h"
#include "charset.h"

#include <tag_c.h>


/*
 * Mp4_Header_Read_File_Info:
 *
 * Get header info into the ETFileInfo structure
 */
gboolean Mp4_Header_Read_File_Info (gchar *filename, ET_File_Info *ETFileInfo)
{
    TagLib_File *file;
    const TagLib_AudioProperties *properties;

    if (!filename || !ETFileInfo)
        return FALSE;

    /* Get size of file */
    ETFileInfo->size = Get_File_Size(filename);

    if ((file = taglib_file_new_type(filename, TagLib_File_MP4)) == NULL )
    {
        gchar *filename_utf8 = filename_to_display(filename);
        //g_print(_("Error while opening file: '%s' (%s)."),filename_utf8,g_strerror(errno));
        Log_Print(LOG_ERROR,_("Error while opening file: '%s' (%s)."),filename_utf8,_("MP4 format invalid"));
        g_free(filename_utf8);
        return FALSE;
    }

    /* Check for audio track */
    if( !taglib_file_is_valid(file) )
    {
        gchar *filename_utf8 = filename_to_display(filename);
        Log_Print(LOG_ERROR,_("Error while opening file: '%s' (%s)."),filename_utf8,("Contains no audio track"));
        g_free(filename_utf8);
        return FALSE;
    }

    properties = taglib_file_audioproperties(file);

    /* Get format/subformat */
    {
        ETFileInfo->mpc_version = g_strdup("MPEG");
	ETFileInfo->mpc_profile = g_strdup("4, Unknown");
    }

    ETFileInfo->version = 4;
    ETFileInfo->mpeg25 = 0;
    ETFileInfo->layer = 14;

    ETFileInfo->variable_bitrate = TRUE;
    ETFileInfo->bitrate = taglib_audioproperties_bitrate(properties);
    ETFileInfo->samplerate = taglib_audioproperties_samplerate(properties);
    ETFileInfo->mode = taglib_audioproperties_channels(properties);
    ETFileInfo->duration = taglib_audioproperties_length(properties);

    taglib_file_free(file);
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
    size  = Convert_Size(ETFileInfo->size);
    size1 = Convert_Size(ETCore->ETFileDisplayedList_TotalSize);
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

#endif
