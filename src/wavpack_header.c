/* wavpack_header.c - 2007/02/15 */
/*
 *  EasyTAG - Tag editor for many file types
 *  Copyright (C) 2007 Maarten Maathuis (madman2003@gmail.com)
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

#ifdef ENABLE_WAVPACK

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <wavpack/wavpack.h>

#include "easytag.h"
#include "et_core.h"
#include "misc.h"
#include "charset.h"
#include "wavpack_header.h"


gboolean Wavpack_Header_Read_File_Info(gchar *filename, ET_File_Info *ETFileInfo)
{
    WavpackContext *wpc;

    wpc = WavpackOpenFileInput(filename, NULL, 0, 0);

    if ( wpc == NULL ) {
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


gboolean Wavpack_Header_Display_File_Info_To_UI(gchar *filename_utf8, ET_File_Info *ETFileInfo)
{
    gchar *text;
    gchar *time = NULL;
    gchar *time1 = NULL;
    gchar *size = NULL;
    gchar *size1 = NULL;

    /* Encoder version */
    gtk_label_set_text(GTK_LABEL(VersionLabel),_("Encoder:"));
    text = g_strdup_printf("%d",ETFileInfo->version);
    gtk_label_set_text(GTK_LABEL(VersionValueLabel),text);
    g_free(text);

    /* Bitrate */
    text = g_strdup_printf(_("%d kb/s"),ETFileInfo->bitrate);
    gtk_label_set_text(GTK_LABEL(BitrateValueLabel),text);
    g_free(text);

    /* Samplerate */
    text = g_strdup_printf(_("%d Hz"),ETFileInfo->samplerate);
    gtk_label_set_text(GTK_LABEL(SampleRateValueLabel),text);
    g_free(text);

    /* Mode */
    gtk_label_set_text(GTK_LABEL(ModeLabel),_("Channels:"));
    text = g_strdup_printf("%d",ETFileInfo->mode);
    gtk_label_set_text(GTK_LABEL(ModeValueLabel),text);
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

#endif /* ENABLE_FLAC */
