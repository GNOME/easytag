/* monkeyaudio_header.c */
/*
 *  EasyTAG - Tag editor for MP3, Ogg Vorbis and MPC files
 *  Copyright (C) 2001-2003  Jerome Couderc <easytag@gmail.com>
 *  Copyright (C) 2002-2003  Artur Polaczyñski <artii@o2.pl>
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

#include <config.h>

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "easytag.h"
#include "et_core.h"
#include "misc.h"
#include "setting.h"
#include "charset.h"
#include "monkeyaudio_header.h"
#include "libapetag/info_mac.h"


/***************
 * Header info *
 ***************/

gboolean Mac_Header_Read_File_Info(gchar *filename, ET_File_Info *ETFileInfo)
{
    StreamInfoMac Info;

    if (info_mac_read(filename, &Info))
    {
        gchar *filename_utf8 = filename_to_display(filename);
        fprintf(stderr,"MAC header not found for file '%s'\n", filename_utf8);
        g_free(filename_utf8);
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



gboolean Mac_Header_Display_File_Info_To_UI (gchar *filename_utf8, ET_File_Info *ETFileInfo)
{
    gchar *text;
    gchar *time  = NULL;
    gchar *time1 = NULL;
    gchar *size  = NULL;
    gchar *size1 = NULL;

    /* Mode changed to profile name  */
    text = g_strdup_printf(_("Profile:"));
    gtk_label_set_text(GTK_LABEL(ModeLabel),text);
    g_free(text);
    text = g_strdup_printf("%s",ETFileInfo->mpc_profile);
    gtk_label_set_text(GTK_LABEL(ModeValueLabel),text);
    g_free(text);

    /* Bitrate */
    text = g_strdup_printf(_("%d kb/s"),ETFileInfo->bitrate);
    gtk_label_set_text(GTK_LABEL(BitrateValueLabel),text);
    g_free(text);

    /* Samplerate */
    text = g_strdup_printf(_("%d Hz"),ETFileInfo->samplerate);
    gtk_label_set_text(GTK_LABEL(SampleRateValueLabel),text);
    g_free(text);

    /* Version changed to encoder version */
    text = g_strdup_printf(_("Encoder:"));
    gtk_label_set_text(GTK_LABEL(VersionLabel),text);
    g_free(text);
    text = g_strdup_printf("%i.%i",ETFileInfo->version/1000,ETFileInfo->version%1000);
    gtk_label_set_text(GTK_LABEL(VersionValueLabel),text);
    g_free(text);

    /* Size */
    size  = Convert_Size(ETFileInfo->size);
    size1 = Convert_Size(ETCore->ETFileDisplayedList_TotalSize);
    text  = g_strdup_printf("%s (%s)",size,size1);
    gtk_label_set_text(GTK_LABEL(SizeValueLabel),text);
    g_free(size);
    g_free(size1);
    g_free(text);

    /* Duration */
    time  = Convert_Duration(ETFileInfo->duration);
    time1 = Convert_Duration(ETCore->ETFileDisplayedList_TotalDuration);
    text  = g_strdup_printf("%s (%s)",time,time1);
    gtk_label_set_text(GTK_LABEL(DurationValueLabel),text);
    g_free(time);
    g_free(time1);
    g_free(text);

    return TRUE;
}
