/* mpeg_header.c - 2000/05/12 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
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

#include "config.h"

#if defined ENABLE_MP3 && defined ENABLE_ID3LIB

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "mpeg_header.h"
#include "easytag.h"
#include "misc.h"

#include <id3.h>
#include "id3lib/id3_bugfix.h"



/****************
 * Declarations *
 ****************/
static const gchar *layer_names[3] =
{
    "I",    /* Layer 1 */
    "II",   /* Layer 2 */
    "III"   /* Layer 3 */
};



/**************
 * Prototypes *
 **************/
static gchar* channel_mode_name(int mode);



/*************
 * Functions *
 *************/

static gchar* channel_mode_name(int mode)
{
    static const gchar *channel_mode[] =
    {
        N_("Stereo"),
        N_("Joint stereo"),
        N_("Dual channel"),
        N_("Single channel")
    };
    if (mode < 0 || mode > 3)
        return "";
    return _(channel_mode[mode]);
}



/*
 * Read infos into header of first frame
 */
gboolean
Mpeg_Header_Read_File_Info (gchar *filename, ET_File_Info *ETFileInfo)
{
    /*
     * With id3lib, the header frame couldn't be read if the file contains an ID3v2 tag with an APIC frame
     */
    ID3Tag *id3_tag = NULL;    /* Tag defined by the id3lib */
    const Mp3_Headerinfo* headerInfo = NULL;

    g_return_val_if_fail (filename != NULL || ETFileInfo != NULL, FALSE);

    /* Get size of file */
    ETFileInfo->size = et_get_file_size (filename);

    /* Get data from tag */
    if ( (id3_tag = ID3Tag_New()) == NULL )
        return FALSE;

    /* Link the file to the tag (uses ID3TT_ID3V2 to get header if APIC is present in Tag) */
    ID3Tag_LinkWithFlags(id3_tag,filename,ID3TT_ID3V2);

    if ( (headerInfo = ID3Tag_GetMp3HeaderInfo(id3_tag)) )
    {
        switch (headerInfo->version)
        {
            case MPEGVERSION_1:
                ETFileInfo->version = 1;
                ETFileInfo->mpeg25 = FALSE;
                break;
            case MPEGVERSION_2:
                ETFileInfo->version = 2;
                ETFileInfo->mpeg25 = FALSE;
                break;
            case MPEGVERSION_2_5:
                ETFileInfo->version = 2;
                ETFileInfo->mpeg25 = TRUE;
                break;
            default:
                break;
        }

        switch (headerInfo->layer)
        {
            case MPEGLAYER_I:
                ETFileInfo->layer = 1;
                break;
            case MPEGLAYER_II:
                ETFileInfo->layer = 2;
                break;
            case MPEGLAYER_III:
                ETFileInfo->layer = 3;
                break;
            default:
                break;
        }

        // Samplerate
        ETFileInfo->samplerate = headerInfo->frequency;

        // Mode -> Seems to be detected but incorrect?!
        switch (headerInfo->modeext)
        {
            case MP3CHANNELMODE_STEREO:
                ETFileInfo->mode = 0;
                break;
            case MP3CHANNELMODE_JOINT_STEREO:
                ETFileInfo->mode = 1;
                break;
            case MP3CHANNELMODE_DUAL_CHANNEL:
                ETFileInfo->mode = 2;
                break;
            case MP3CHANNELMODE_SINGLE_CHANNEL:
                ETFileInfo->mode = 3;
                break;
            default:
                break;
        }

        // Bitrate
        if (headerInfo->vbr_bitrate <= 0)
        {
            ETFileInfo->variable_bitrate = FALSE;
            ETFileInfo->bitrate = headerInfo->bitrate/1000;
        }else
        {
            ETFileInfo->variable_bitrate = TRUE;
            ETFileInfo->bitrate = headerInfo->vbr_bitrate/1000;
        }

        // Duration
        ETFileInfo->duration = headerInfo->time;
    }

    /* Free allocated data */
    ID3Tag_Delete(id3_tag);

    return TRUE;
}



/*
 * Display header infos in the main window
 */
gboolean Mpeg_Header_Display_File_Info_To_UI(gchar *filename_utf8, ET_File_Info *ETFileInfo)
{
    gchar *text;
    gchar *time = NULL;
    gchar *time1 = NULL;
    gchar *size = NULL;
    gchar *size1 = NULL;
    gint ln_num = sizeof(layer_names)/sizeof(layer_names[0]);


    /* MPEG, Layer versions */
    gtk_label_set_text(GTK_LABEL(VersionLabel),_("MPEG"));
    if (ETFileInfo->mpeg25)
        text = g_strdup_printf("2.5, Layer %s",(ETFileInfo->layer>=1 && ETFileInfo->layer<=ln_num)?layer_names[ETFileInfo->layer-1]:"?");
    else
        text = g_strdup_printf("%d, Layer %s",ETFileInfo->version,(ETFileInfo->layer>=1 && ETFileInfo->layer<=ln_num)?layer_names[ETFileInfo->layer-1]:"?");
    gtk_label_set_text(GTK_LABEL(VersionValueLabel),text);
    g_free(text);

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
    gtk_label_set_text(GTK_LABEL(ModeLabel),_("Mode:"));
    text = g_strdup_printf("%s",_(channel_mode_name(ETFileInfo->mode)));
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

#endif /* defined ENABLE_MP3 && defined ENABLE_ID3LIB */
