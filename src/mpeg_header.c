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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "mpeg_header.h"
#include "easytag.h"
#include "et_core.h"
#include "log.h"
#include "misc.h"
#include "charset.h"

// Set to :
// - 1 to use ID3lib for reading headers
// - 0 to use mpeg123 for reading headers
#define USE_ID3LIB_4_HEADER 0

#if USE_ID3LIB_4_HEADER
#   include <id3.h>
#   include "id3lib/id3_bugfix.h"
#else
#   include "libmpg123/mpg123.h"
#endif




/****************
 * Declarations *
 ****************/
gchar *layer_names[3] =
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
gboolean Mpeg_Header_Read_File_Info (gchar *filename, ET_File_Info *ETFileInfo)
{
#if (!USE_ID3LIB_4_HEADER)
    FILE *file;
    gulong filesize;


    if (!filename || !ETFileInfo)
        return FALSE;

    /* Get size of file */
    filesize         = Get_File_Size(filename);
    ETFileInfo->size = filesize;


    /*
     * This part was taken from XMMS
     */
    if ((file = fopen(filename, "rb")) != NULL)
    {
        guint32 head;
        unsigned char tmp[4];
        struct frame frm;

        if (fread(tmp, 1, 4, file) != 4)
        {
            fclose(file);
            return FALSE;
        }

        // Skip data of the ID3v2.x tag (It may contain data similar to mpeg frame 
        // as, for example, in the id3 APIC frame) (patch from Artur Polaczynski)
        if (tmp[0] == 'I' && tmp[1] == 'D' && tmp[2] == '3' && tmp[3] < 0xFF)
        {
            // ID3v2 tag skipeer $49 44 33 yy yy xx zz zz zz zz [zz size]
            long id3v2size;
            fseek(file, 2, SEEK_CUR); // Size is 6-9 position
            if (fread(tmp, 1, 4, file) != 4) // Read bytes of tag size
            {
                fclose(file);
                return FALSE;
            }
            id3v2size = 10 + ( (long)(tmp[3]) | ((long)(tmp[2]) << 7) | ((long)(tmp[1]) << 14) | ((long)(tmp[0]) << 21) );
            fseek(file, id3v2size, SEEK_SET);
            if (fread(tmp, 1, 4, file) != 4) // Read mpeg header
            {
                fclose(file);
                return FALSE;
            }
        }

        head = ((guint32) tmp[0] << 24) | ((guint32) tmp[1] << 16) | ((guint32) tmp[2] << 8) | (guint32) tmp[3];
        while (!mpg123_head_check(head))
        {
            head <<= 8;
            if (fread(tmp, 1, 1, file) != 1)
            {
                fclose(file);
                return FALSE;
            }
            head |= tmp[0];
        }
        if (mpg123_decode_header(&frm, head))
        {
            guchar *buf;
            gdouble tpf;
            XHEADDATA xing_header;

            buf = g_malloc(frm.framesize + 4);
            fseek(file, -4, SEEK_CUR);
            fread(buf, 1, frm.framesize + 4, file);
            xing_header.toc = NULL;
            tpf = mpg123_compute_tpf(&frm);
            // MPEG and Layer version
            ETFileInfo->mpeg25 = frm.mpeg25;
            if (!ETFileInfo->mpeg25)
                ETFileInfo->version = frm.lsf+1;
            ETFileInfo->layer = frm.lay;
            //if (ETFileInfo->mpeg25) g_print("mpeg_level: MPEG 2.5, layer %d\n",ETFileInfo->layer);
            //else                    g_print("mpeg_level: MPEG %d, layer %d\n",ETFileInfo->version,ETFileInfo->layer);

            fseek(file, 0, SEEK_END);
            // Variable bitrate? + bitrate
            if ( (ETFileInfo->variable_bitrate=mpg123_get_xing_header(&xing_header,buf)) )
            {
                ETFileInfo->bitrate = (gint) ((xing_header.bytes * 8) / (tpf * xing_header.frames * 1000));
                /* g_print ("Bitrate: Variable,\navg. bitrate: %d kb/s\n",ETFileInfo->bitrate); */
            } else
            {
                ETFileInfo->bitrate = tabsel_123[frm.lsf][frm.lay - 1][frm.bitrate_index];
                /* g_print ("Bitrate: %d kb/s\n",ETFileInfo->bitrate); */
            }
            /* Samplerate. */
            ETFileInfo->samplerate = mpg123_freqs[frm.sampling_frequency];
            /* Mode. */
            ETFileInfo->mode = frm.mode;
            /* g_print ("Samplerate: %ld Hz\n", mpg123_freqs[frm.sampling_frequency]);
            g_print ("%s\nError protection: %s\nCopyright: %s\nOriginal: %s\nEmphasis: %s\n", channel_mode_name(frm.mode), bool_label[frm.error_protection], bool_label[frm.copyright], bool_label[frm.original], emphasis[frm.emphasis]);
            g_print ("Filesize: %lu B\n", ftell(file)); */
            g_free (buf);
        }

        // Duration
        ETFileInfo->duration = mpg123_get_song_time(file)/1000;
        //g_print("time %s\n",Convert_Duration(ETFileInfo->duration));

        fclose(file);
    }else
    {
        gchar *filename_utf8 = filename_to_display(filename);
        Log_Print(LOG_ERROR,_("Error while opening file: '%s' (%s)."),filename_utf8,g_strerror(errno));
        g_free(filename_utf8);
        return FALSE;
    }

#else
    // Needs to uncomment some #include at the beginning

    /*
     * With id3lib, the header frame couldn't be read if the file contains an ID3v2 tag with an APIC frame
     */
    gulong filesize;
    ID3Tag *id3_tag = NULL;    /* Tag defined by the id3lib */
    const Mp3_Headerinfo* headerInfo = NULL;


    if (!filename || !ETFileInfo)
        return FALSE;

    /* Get size of file */
    filesize         = Get_File_Size(filename);
    ETFileInfo->size = filesize;

    /* Get data from tag */
    if ( (id3_tag = ID3Tag_New()) == NULL )
        return FALSE;

    /* Link the file to the tag (uses ID3TT_ID3V2 to get header if APIC is present in Tag) */
    ID3Tag_LinkWithFlags(id3_tag,filename,ID3TT_ID3V2);

    /*ID3_STRUCT(Mp3_Headerinfo)
    {
      Mpeg_Layers layer;
      Mpeg_Version version;
      MP3_BitRates bitrate;
      Mp3_ChannelMode channelmode;
      Mp3_ModeExt modeext;
      Mp3_Emphasis emphasis;
      Mp3_Crc crc;
      uint32 vbr_bitrate;           // avg bitrate from xing header
      uint32 frequency;             // samplerate
      uint32 framesize;
      uint32 frames;                // nr of frames
      uint32 time;                  // nr of seconds in song
      bool privatebit;
      bool copyrighted;
      bool original;
    };*/

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
                ETFileInfo->mpeg25 = TRUE;
                ETFileInfo->mpeg25 = FALSE;
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

#endif

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
    ln_num = sizeof(layer_names)/sizeof(layer_names[0]); // Used to avoid problem with layer_names[]
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
