/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
 * Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
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

/*
 * Code taken from :
 * FLAC - Free Lossless Audio Codec - v1.0.3
 * Copyright (C) 2001  Josh Coalson
 *
 */

#include "config.h" /* For definition of ENABLE_FLAC. */

#ifdef ENABLE_FLAC

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <FLAC/all.h>
#include <errno.h>

#include "easytag.h"
#include "et_core.h"
#include "flac_header.h"
#include "misc.h"


/****************
 * Declarations *
 ****************/
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

gboolean
flac_header_read_file_info (const gchar *filename,
                            ET_File_Info *ETFileInfo,
                            GError **error)
{
    gint duration = 0;
    gulong filesize;
    FLAC__StreamDecoder *tmp_decoder;

    file_info_struct tmp_file_info;

    g_return_val_if_fail (filename != NULL && ETFileInfo != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    /* Decoding FLAC file */
    tmp_decoder = FLAC__stream_decoder_new ();

    if (tmp_decoder == NULL)
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_NOMEM, "%s",
                     g_strerror (ENOMEM));
        return FALSE;
    }

    tmp_file_info.abort_flag = false;

    FLAC__stream_decoder_set_md5_checking     (tmp_decoder, false);
    if(FLAC__stream_decoder_init_file(tmp_decoder, filename, write_callback_, metadata_callback_, error_callback_, &tmp_file_info) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
    {
        /* TODO: Set error message according to FLAC__StreamDecoderInitStatus.
         */
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "%s",
                     _("Error opening FLAC file"));
        FLAC__stream_decoder_finish (tmp_decoder);
        FLAC__stream_decoder_delete (tmp_decoder);
        return FALSE;
    }

    if(!FLAC__stream_decoder_process_until_end_of_metadata(tmp_decoder))
    {
        /* TODO: Set error message according to state fetched from
         * FLAC__stream_decoder_get_state(). */
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "%s",
                     _("Error opening FLAC file"));
        FLAC__stream_decoder_finish (tmp_decoder);
        FLAC__stream_decoder_delete (tmp_decoder);
        return FALSE;
    }

    FLAC__stream_decoder_finish(tmp_decoder);
    FLAC__stream_decoder_delete(tmp_decoder);
    /* End of decoding FLAC file */


    filesize = et_get_file_size (filename);
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

EtFileHeaderFields *
Flac_Header_Display_File_Info_To_UI (const gchar *filename_utf8,
                                     const ET_File *ETFile)
{
    EtFileHeaderFields *fields;
    ET_File_Info *info;
    gchar *time = NULL;
    gchar *time1 = NULL;
    gchar *size = NULL;
    gchar *size1 = NULL;

    info = ETFile->ETFileInfo;
    fields = g_slice_new (EtFileHeaderFields);

    fields->description = _("FLAC File");

    /* Nothing to display */
    fields->version_label = _("Encoder:");
    fields->version = g_strdup ("flac");

    /* Bitrate */
    fields->bitrate = g_strdup_printf (_("%d kb/s"), info->bitrate);

    /* Samplerate */
    fields->samplerate = g_strdup_printf (_("%d Hz"), info->samplerate);

    /* Mode */
    fields->mode_label = _("Channels:");
    fields->mode = g_strdup_printf ("%d", info->mode);

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
et_flac_file_header_fields_free (EtFileHeaderFields *fields)
{
    g_return_if_fail (fields != NULL);

    g_free (fields->version);
    g_free (fields->bitrate);
    g_free (fields->samplerate);
    g_free (fields->mode);
    g_free (fields->size);
    g_free (fields->duration);
    g_slice_free (EtFileHeaderFields, fields);
}

#endif /* ENABLE_FLAC */
