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

#include <config.h> // For definition of ENABLE_MP4

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

/* These undefs are because the mpeg4ip library contains a gnu config file in it's .h file */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#include <mp4.h>


/****************
 * Declarations *
 ****************/

static const struct
{
    uint8_t profile;
    const char *format;
    const char *subformat;
} MP4AudioProfileToName[] = {
    { MP4_MPEG4_AAC_MAIN_AUDIO_TYPE,       "MPEG", "4, AAC main", },
    { MP4_MPEG4_AAC_LC_AUDIO_TYPE,         "MPEG", "4, AAC LC", },
    { MP4_MPEG4_AAC_SSR_AUDIO_TYPE,        "MPEG", "4, AAC SSR", },
    { MP4_MPEG4_AAC_LTP_AUDIO_TYPE,        "MPEG", "4, AAC LTP", },
    { MP4_MPEG4_AAC_HE_AUDIO_TYPE,         "MPEG", "4, AAC HE", },
    { MP4_MPEG4_AAC_SCALABLE_AUDIO_TYPE,   "MPEG", "4, AAC Scalable", },
    { 7,                                   "MPEG", "4, TwinVQ", },
    { MP4_MPEG4_CELP_AUDIO_TYPE,           "MPEG", "4, CELP", },
    { MP4_MPEG4_HVXC_AUDIO_TYPE,           "MPEG", "4, HVXC", },
    //  10, 11 unused
    { MP4_MPEG4_TTSI_AUDIO_TYPE,           "MPEG", "4, TTSI", },
    { MP4_MPEG4_MAIN_SYNTHETIC_AUDIO_TYPE, "MPEG", "4, Main Synthetic", },
    { MP4_MPEG4_WAVETABLE_AUDIO_TYPE,      "MPEG", "4, Wavetable Syn", },
    { MP4_MPEG4_MIDI_AUDIO_TYPE,           "MPEG", "4, General MIDI", },
    { MP4_MPEG4_ALGORITHMIC_FX_AUDIO_TYPE, "MPEG", "4, Algo Syn and Audio FX", },
    { 17,                                  "MPEG", "4, ER AAC LC", },
    // 18 unused
    { 19,                                  "MPEG", "4, ER AAC LTP", },
    { 20,                                  "MPEG", "4, ER AAC Scalable", },
    { 21,                                  "MPEG", "4, ER TwinVQ", },
    { 22,                                  "MPEG", "4, ER BSAC", },
    { 23,                                  "MPEG", "4, ER ACC LD", },
    { 24,                                  "MPEG", "4, ER CELP", },
    { 25,                                  "MPEG", "4, ER HVXC", },
    { 26,                                  "MPEG", "4, ER HILN", },
    { 27,                                  "MPEG", "4, ER Parametric", },
};

static const struct
{
    uint8_t profile;
    const char *format;
    const char *subformat;
} AudioProfileToName[] = {
    { MP4_MPEG2_AAC_MAIN_AUDIO_TYPE,       "MPEG",   "2, AAC Main" },
    { MP4_MPEG2_AAC_LC_AUDIO_TYPE,         "MPEG",   "2, AAC LC" },
    { MP4_MPEG2_AAC_SSR_AUDIO_TYPE,        "MPEG",   "2, AAC SSR" },
    { MP4_MPEG2_AUDIO_TYPE,                "MPEG",   "2, Audio (13818-3)" },
    { MP4_MPEG1_AUDIO_TYPE,                "MPEG",   "1, Audio (11172-3)" },
    // mpeg4ip's private definitions
    { MP4_PCM16_LITTLE_ENDIAN_AUDIO_TYPE,  "PCM16",   "Little Endian" },
    { MP4_VORBIS_AUDIO_TYPE,               "Vorbis",  "" },
    { MP4_ALAW_AUDIO_TYPE,                 "G.711",   "aLaw" },
    { MP4_ULAW_AUDIO_TYPE,                 "G.711",   "uLaw" },
    { MP4_G723_AUDIO_TYPE,                 "G.723.1", "" },
    { MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE,     "PCM16",   "Big Endian" },
};

#define NUMBER_OF(A) (sizeof(A) / sizeof(A[0]))


/**************
 * Prototypes *
 **************/


/*************
 * Functions *
 *************/

/*
 * getType:
 *
 * Returns a format/sub-format information. Taken from mp4.h/mp4info.
 */
static void getType(MP4FileHandle file, MP4TrackId trackId, const char **format, const char **subformat )
{
    unsigned i;
    const char *media_data_name = MP4GetTrackMediaDataName(file, trackId);

    *format = _("Audio");
    *subformat = _("Unknown");

    if (media_data_name == NULL)
    {
        ;
    } else if (strcasecmp(media_data_name, "samr") == 0)
    {
        *subformat = "AMR";
    } else if (strcasecmp(media_data_name, "sawb") == 0)
    {
        *subformat = "AMR-WB";
    } else if (strcasecmp(media_data_name, "mp4a") == 0)
    {
        u_int8_t type = MP4GetTrackEsdsObjectTypeId(file, trackId);

        if( type == MP4_MPEG4_AUDIO_TYPE )
        {
            u_int8_t* pAacConfig = NULL;
            u_int32_t aacConfigLength;

            MP4GetTrackESConfiguration(file, trackId, &pAacConfig, &aacConfigLength);

            if (pAacConfig != NULL)
            {
                type = aacConfigLength >= 2 ? ((pAacConfig[0] >> 3) & 0x1f) : 0;
                free(pAacConfig);

                for (i = 0; i < NUMBER_OF(MP4AudioProfileToName); i++)
                {
                    if (type == MP4AudioProfileToName[i].profile)
                    {
                        *format = MP4AudioProfileToName[i].format;
                        *subformat = MP4AudioProfileToName[i].subformat;
                        return;
                    }
                }
            }
            *format = "MPEG";
            *subformat = "4, Unknown";
        } else
        {
            for (i = 0; i < NUMBER_OF(AudioProfileToName); i++)
            {
                if (type == AudioProfileToName[i].profile)
                {
                    *format = AudioProfileToName[i].format;
                    *subformat = AudioProfileToName[i].subformat;
                    return;
                }
            }
        }
    } else
    {
        *subformat = media_data_name;
    }
}


/*
 * Mp4_Header_Read_File_Info:
 *
 * Get header info into the ETFileInfo structure
 */
gboolean Mp4_Header_Read_File_Info (gchar *filename, ET_File_Info *ETFileInfo)
{
    MP4FileHandle file;
    MP4TrackId trackId = 1;
    //const char* trackType;
    const char *format, *subformat;

    if (!filename || !ETFileInfo)
        return FALSE;

    /* Get size of file */
    ETFileInfo->size = Get_File_Size(filename);

    if ((file = MP4Read(filename, 0)) == MP4_INVALID_FILE_HANDLE )
    {
        gchar *filename_utf8 = filename_to_display(filename);
        //g_print(_("ERROR while opening file: '%s' (%s)."),filename_utf8,g_strerror(errno));
        Log_Print(LOG_ERROR,_("ERROR while opening file: '%s' (%s)."),filename_utf8,_("MP4 format invalid"));
        g_free(filename_utf8);
        return FALSE;
    }

    /* Check for audio track */
    if( MP4GetNumberOfTracks(file,MP4_AUDIO_TRACK_TYPE,0) < 1 )
    {
        gchar *filename_utf8 = filename_to_display(filename);
        Log_Print(LOG_ERROR,_("ERROR while opening file: '%s' (%s)."),filename_utf8,("Contains no audio track"));
        MP4Close(file);
        g_free(filename_utf8);
        return FALSE;
    }

    /* Get the first track id (index 0) */
    trackId = MP4FindTrackId(file, 0, MP4_AUDIO_TRACK_TYPE, 0);

    /* Get format/subformat */
    {
        getType( file, trackId, &format, &subformat );
        ETFileInfo->mpc_version = g_strdup( format );
        ETFileInfo->mpc_profile = g_strdup( subformat );
    }

    ETFileInfo->version = 4;
    ETFileInfo->mpeg25 = 0;
    ETFileInfo->layer = 14;

    ETFileInfo->variable_bitrate = TRUE;
    ETFileInfo->bitrate = MP4GetTrackBitRate(file, trackId) / 1000;
    ETFileInfo->samplerate = MP4GetTrackTimeScale(file, trackId);
    ETFileInfo->mode = MP4GetTrackAudioChannels(file, trackId);
    ETFileInfo->duration = MP4ConvertFromTrackDuration(file, trackId, MP4GetTrackDuration(file, trackId), MP4_SECS_TIME_SCALE);

    MP4Close(file);
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
