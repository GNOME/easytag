/* flac_header.c - 2002/07/03 */
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

/*
 * Code taken from :
 * FLAC - Free Lossless Audio Codec - v1.0.3
 * Copyright (C) 2001  Josh Coalson
 *
 */

#include <config.h> // For definition of ENABLE_FLAC

#ifdef ENABLE_FLAC

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>    // 20030519 <tmancill@debian.org> for compiliation on ia64 arch
#include <FLAC/all.h>

#include "easytag.h"
#include "et_core.h"
#include "flac_header.h"
#include "log.h"
#include "misc.h"
#include "charset.h"


/****************
 * Declarations *
 ****************/
/* Taken in "xmms/plugin.h" */
typedef enum
{
    FMT_U8, FMT_S8, FMT_U16_LE, FMT_U16_BE, FMT_U16_NE, FMT_S16_LE, FMT_S16_BE, FMT_S16_NE
}
AFormat;

typedef struct {
    FLAC__bool abort_flag;
    FLAC__bool is_playing;
    FLAC__bool eof;
    FLAC__bool play_thread_open; /* if true, is_playing must also be true */
    unsigned total_samples;
    unsigned bits_per_sample;
    unsigned channels;
    unsigned sample_rate;
    unsigned length_in_msec;
    AFormat sample_format; // Note : defined in XMMS devel
    int seek_to_in_sec;
} file_info_struct;


static FLAC__byte reservoir_[FLAC__MAX_BLOCK_SIZE * 2 * 2 * 2]; /* *2 for max bytes-per-sample, *2 for max channels, another *2 for overflow */
static unsigned reservoir_samples_ = 0;


/**************
 * Prototypes *
 **************/
static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback_   (const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);



/*************
 * Functions *
 *************/

/****************************
 * Header info of FLAC file *
 ****************************/

gboolean Flac_Header_Read_File_Info (gchar *filename, ET_File_Info *ETFileInfo)
{
    FILE *file;
    gdouble duration = 0;
    gulong filesize;
    FLAC__StreamDecoder *tmp_decoder;

    file_info_struct tmp_file_info;

    if (!filename || !ETFileInfo)
        return FALSE;

    if ( (file=fopen(filename,"r"))==NULL )
    {
        gchar *filename_utf8 = filename_to_display(filename);
        Log_Print(LOG_ERROR,_("Error while opening file: '%s' (%s)."),filename_utf8,g_strerror(errno));
        g_free(filename_utf8);
        return FALSE;
    }
    fclose(file);

    /* Decoding FLAC file */
    tmp_decoder = FLAC__stream_decoder_new();
    if (tmp_decoder == 0)
    {
        return FALSE;
    }

    tmp_file_info.abort_flag = false;

    FLAC__stream_decoder_set_md5_checking     (tmp_decoder, false);
    if(FLAC__stream_decoder_init_file(tmp_decoder, filename, write_callback_, metadata_callback_, error_callback_, &tmp_file_info) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
        return FALSE;

    if(!FLAC__stream_decoder_process_until_end_of_metadata(tmp_decoder))
    {
        return FALSE;
    }

    FLAC__stream_decoder_finish(tmp_decoder);
    FLAC__stream_decoder_delete(tmp_decoder);
    /* End of decoding FLAC file */


    filesize = Get_File_Size(filename);
    duration = (gint)tmp_file_info.length_in_msec/1000;

    ETFileInfo->version     = 0; // Not defined in FLAC file
    if (duration > 0)
        ETFileInfo->bitrate = filesize*8/duration/1000; // FIXME : Approximation!! Needs to remove tag size!
    ETFileInfo->samplerate  = tmp_file_info.sample_rate;
    ETFileInfo->mode        = tmp_file_info.channels;
    ETFileInfo->size        = filesize;
    ETFileInfo->duration    = duration;

    return TRUE;
}



FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
    file_info_struct *file_info = (file_info_struct *)client_data;
    const unsigned bps = file_info->bits_per_sample, channels = file_info->channels, wide_samples = frame->header.blocksize;
    unsigned wide_sample, sample, channel;
    FLAC__int8 *scbuffer = (FLAC__int8*)reservoir_;
    FLAC__int16 *ssbuffer = (FLAC__int16*)reservoir_;

    (void)decoder;

    if (file_info->abort_flag)
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

    if (bps == 8) {
        for(sample = reservoir_samples_*channels, wide_sample = 0; wide_sample < wide_samples; wide_sample++)
            for(channel = 0; channel < channels; channel++, sample++)
                scbuffer[sample] = (FLAC__int8)buffer[channel][wide_sample];
    }
    else if (bps == 16) {
        for(sample = reservoir_samples_*channels, wide_sample = 0; wide_sample < wide_samples; wide_sample++)
            for(channel = 0; channel < channels; channel++, sample++)
                ssbuffer[sample] = (FLAC__int16)buffer[channel][wide_sample];
    }
    else {
        file_info->abort_flag = true;
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    reservoir_samples_ += wide_samples;

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
    file_info_struct *file_info = (file_info_struct *)client_data;
    (void)decoder;
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
    {
        FLAC__ASSERT(metadata->data.stream_info.total_samples < 0x100000000); /* this plugin can only handle < 4 gigasamples */
        file_info->total_samples = (unsigned)(metadata->data.stream_info.total_samples&0xffffffff);
        file_info->bits_per_sample = metadata->data.stream_info.bits_per_sample;
        file_info->channels = metadata->data.stream_info.channels;
        file_info->sample_rate = metadata->data.stream_info.sample_rate;

        if (file_info->bits_per_sample == 8)
        {
            file_info->sample_format = FMT_S8;
        } else if (file_info->bits_per_sample == 16)
        {
            file_info->sample_format = FMT_S16_NE;
        } else
        {
            file_info->abort_flag = true;
            return;
        }
        if (file_info->sample_rate != 0 && (file_info->sample_rate / 100) != 0) // To prevent crash...
            file_info->length_in_msec = file_info->total_samples * 10 / (file_info->sample_rate / 100);
    }
}

void error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
    file_info_struct *file_info = (file_info_struct *)client_data;
    (void)decoder;
    if (status != FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC)
        file_info->abort_flag = true;
}




gboolean Flac_Header_Display_File_Info_To_UI (gchar *filename_utf8, ET_File_Info *ETFileInfo)
{
    gchar *text;
    gchar *time = NULL;
    gchar *time1 = NULL;
    gchar *size = NULL;
    gchar *size1 = NULL;

    /* Nothing to display */
    //gtk_label_set_text(GTK_LABEL(VersionLabel),"");
    //gtk_label_set_text(GTK_LABEL(VersionValueLabel),"");
    gtk_label_set_text(GTK_LABEL(VersionLabel),_("Encoder:"));
    gtk_label_set_text(GTK_LABEL(VersionValueLabel),"flac");

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
