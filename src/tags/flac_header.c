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

typedef struct
{
    GFileInputStream *istream;
    GError *error;
    gboolean abort;

    /* *2 for max bytes-per-sample, *2 for max channels, another *2 for
     * overflow */
    FLAC__byte reservoir[FLAC__MAX_BLOCK_SIZE * 2 * 2 * 2];
    guint reservoir_samples;
    guint total_samples;
    guint bits_per_sample;
    guint channels;
    guint sample_rate;
    guint length_in_msec;
} EtFlacState;


/* FLAC__stream_decoder_init_stream() IO callbacks. */
static FLAC__StreamDecoderReadStatus
et_flac_read_func (const FLAC__StreamDecoder *decoder,
                   FLAC__byte buffer[],
                   size_t *bytes,
                   void *client_data)
{
    EtFlacState *state = (EtFlacState *)client_data;
    gssize bytes_read;

    bytes_read = g_input_stream_read (G_INPUT_STREAM (state->istream), buffer,
                                      *bytes, NULL, &state->error);

    if (bytes_read == -1)
    {
        *bytes = 0;
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    }
    else if (bytes_read == 0)
    {
        *bytes = 0;
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }
    else
    {
        *bytes = bytes_read;
        return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
    }
}

static FLAC__StreamDecoderSeekStatus
et_flac_seek_func (const FLAC__StreamDecoder *decoder,
                   FLAC__uint64 absolute_byte_offset,
                   void *client_data)
{
    EtFlacState *state = (EtFlacState *)client_data;

    if (!g_seekable_can_seek (G_SEEKABLE (state->istream)))
    {
        return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
    }
    else
    {
        if (!g_seekable_seek (G_SEEKABLE (state->istream),
                              absolute_byte_offset, G_SEEK_SET, NULL,
                              &state->error))
        {
            return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
        }
        else
        {
            return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
        }
    }
}

static FLAC__StreamDecoderTellStatus
et_flac_tell_func (const FLAC__StreamDecoder *decoder,
                   FLAC__uint64 *absolute_byte_offset,
                   void *client_data)
{
    EtFlacState *state = (EtFlacState *)client_data;

    if (!g_seekable_can_seek (G_SEEKABLE (state->istream)))
    {
        return FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;
    }

    *absolute_byte_offset = g_seekable_tell (G_SEEKABLE (state->istream));

    /* TODO: Investigate whether it makes sense to return an error code. */
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

static FLAC__StreamDecoderLengthStatus
et_flac_length_func (const FLAC__StreamDecoder *decoder,
                     FLAC__uint64 *stream_length,
                     void *client_data)
{
    EtFlacState *state = (EtFlacState *)client_data;
    GFileInfo *info;

    info = g_file_input_stream_query_info (state->istream,
                                           G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                           NULL, &state->error);

    if (!info)
    {
        g_debug ("Error getting length of FLAC stream: %s",
                 state->error->message);
        return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
    }
    else
    {
        *stream_length = g_file_info_get_size (info);
        g_object_unref (info);
        return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
    }
}

static FLAC__bool
et_flac_eof_func (const FLAC__StreamDecoder *decoder,
                  void *client_data)
{
    /* GFileInputStream does not directly support checking to tell if the
     * stream is at EOF. */
    return false;
}

static FLAC__StreamDecoderWriteStatus
et_flac_write_func (const FLAC__StreamDecoder *decoder,
                    const FLAC__Frame *frame,
                    const FLAC__int32 * const buffer[],
                    void *client_data)
{
    EtFlacState *state = (EtFlacState *)client_data;
    const guint bps = state->bits_per_sample;
    const guint channels = state->channels;
    const guint wide_samples = frame->header.blocksize;
    guint wide_sample;
    guint sample;
    guint channel;
    FLAC__int8 *scbuffer = (FLAC__int8*)state->reservoir;
    FLAC__int16 *ssbuffer = (FLAC__int16*)state->reservoir;

    if (state->abort)
    {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    if (bps == 8)
    {
        for (sample = state->reservoir_samples * channels, wide_sample = 0;
             wide_sample < wide_samples; wide_sample++)
        {
            for (channel = 0; channel < channels; channel++, sample++)
            {
                scbuffer[sample] = (FLAC__int8)buffer[channel][wide_sample];
            }
        }
    }
    else if (bps == 16)
    {
        for (sample = state->reservoir_samples * channels, wide_sample = 0;
             wide_sample < wide_samples; wide_sample++)
        {
            for (channel = 0; channel < channels; channel++, sample++)
            {
                ssbuffer[sample] = (FLAC__int16)buffer[channel][wide_sample];
            }
        }
    }
    else
    {
        state->abort = TRUE;
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    state->reservoir_samples += wide_samples;

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void
et_flac_metadata_func (const FLAC__StreamDecoder *decoder,
                       const FLAC__StreamMetadata *metadata,
                       void *client_data)
{
    EtFlacState *state = (EtFlacState *)client_data;

    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
    {
        FLAC__ASSERT(metadata->data.stream_info.total_samples < 0x100000000); /* this plugin can only handle < 4 gigasamples */
        state->total_samples = (unsigned)(metadata->data.stream_info.total_samples&0xffffffff);
        state->bits_per_sample = metadata->data.stream_info.bits_per_sample;
        state->channels = metadata->data.stream_info.channels;
        state->sample_rate = metadata->data.stream_info.sample_rate;

        /* To prevent crash... */
        if (state->sample_rate != 0 && (state->sample_rate / 100) != 0)
        {
            state->length_in_msec = state->total_samples * 10
                                    / (state->sample_rate / 100);
        }
    }
}

static void
et_flac_error_func (const FLAC__StreamDecoder *decoder,
                    FLAC__StreamDecoderErrorStatus status,
                    void *client_data)
{
    EtFlacState *state = (EtFlacState *)client_data;

    if (status != FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC)
    {
        state->abort = TRUE;
    }
}

/* Not a callback for FLAC__stream_decoder_init_stream(). */
static void
et_flac_close (EtFlacState *state)
{
    g_clear_object (&state->istream);
    g_clear_error (&state->error);
}

/* Header info of FLAC file */

gboolean
flac_header_read_file_info (GFile *file,
                            ET_File_Info *ETFileInfo,
                            GError **error)
{
    gint duration = 0;
    GFileInfo *info;
    FLAC__StreamDecoder *tmp_decoder;
    EtFlacState state;

    g_return_val_if_fail (file != NULL && ETFileInfo != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    /* Decoding FLAC file */
    tmp_decoder = FLAC__stream_decoder_new ();

    if (tmp_decoder == NULL)
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_NOMEM, "%s",
                     g_strerror (ENOMEM));
        return FALSE;
    }


    FLAC__stream_decoder_set_md5_checking (tmp_decoder, false);

    state.abort = FALSE;
    state.error = NULL;
    state.istream = g_file_read (file, NULL, &state.error);
    state.reservoir_samples = 0;

    if (FLAC__stream_decoder_init_stream (tmp_decoder, et_flac_read_func,
                                          et_flac_seek_func, et_flac_tell_func,
                                          et_flac_length_func,
                                          et_flac_eof_func, et_flac_write_func,
                                          et_flac_metadata_func,
                                          et_flac_error_func, &state)
        != FLAC__STREAM_DECODER_INIT_STATUS_OK)
    {
        /* TODO: Set error message according to FLAC__StreamDecoderInitStatus.
         */
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "%s",
                     _("Error opening FLAC file"));
        g_debug ("Error opening FLAC stream: %s", state.error->message);
        et_flac_close (&state);
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
        g_debug ("Error fetching metadata from FLAC stream: %s",
                 state.error->message);
        et_flac_close (&state);
        FLAC__stream_decoder_finish (tmp_decoder);
        FLAC__stream_decoder_delete (tmp_decoder);
        return FALSE;
    }

    et_flac_close (&state);
    FLAC__stream_decoder_finish(tmp_decoder);
    FLAC__stream_decoder_delete(tmp_decoder);
    /* End of decoding FLAC file */

    info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_SIZE,
                              G_FILE_QUERY_INFO_NONE, NULL, NULL);

    if (info)
    {
        ETFileInfo->size = g_file_info_get_size (info);
        g_object_unref (info);
    }
    else
    {
        ETFileInfo->size = 0;
    }

    duration = (gint)state.length_in_msec / 1000;

    if (duration > 0 && ETFileInfo->size > 0)
    {
        /* FIXME: Approximation! Needs to remove tag size. */
        ETFileInfo->bitrate = ETFileInfo->size * 8 / duration / 1000;
    }

    ETFileInfo->version = 0; /* Not defined in FLAC file. */
    ETFileInfo->samplerate = state.sample_rate;
    ETFileInfo->mode = state.channels;
    ETFileInfo->duration = duration;

    return TRUE;
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
