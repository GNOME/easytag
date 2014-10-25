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

#include "config.h" /* For definition of ENABLE_OGG. */

#ifdef ENABLE_OGG

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <errno.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#ifdef ENABLE_SPEEX
#include <speex/speex_header.h>
#include "vcedit.h"
#endif

#include "easytag.h"
#include "ogg_header.h"
#include "et_core.h"
#include "charset.h"
#include "log.h"
#include "misc.h"


/*************
 * Functions *
 *************/

/*
 * et_ogg_error_quark:
 *
 * To get EtOGGError domain.
 *
 * Returns: GQuark for EtOGGError domain
 */
GQuark
et_ogg_error_quark (void)
{
    return g_quark_from_static_string ("et-ogg-error-quark");
}

/*
 * EtOggState:
 * @file: the Ogg file which is currently being parsed
 * @istream: an input stream for the current Ogg file
 * @error: either the most recent error, or %NULL
 *
 * The current state of the Ogg parser, for passing between the callbacks used
 * in ov_open_callbacks().
 */
typedef struct
{
    GFile *file;
    GInputStream *istream;
    GError *error;
} EtOggState;

/*
 * et_ogg_read_func:
 * @ptr: the buffer to fill with data
 * @size: the size of individual reads
 * @nmemb: the number of members to read
 * @datasource: the Ogg parser state
 *
 * Read a number of bytes from the Ogg file.
 *
 * Returns: the number of bytes read from the stream. Returns 0 on end-of-file.
 * Sets errno and returns 0 on error
 */
static size_t
et_ogg_read_func (void *ptr, size_t size, size_t nmemb, void *datasource)
{
    EtOggState *state = (EtOggState *)datasource;
    gssize bytes_read;

    bytes_read = g_input_stream_read (state->istream, ptr, size * nmemb, NULL,
                                      &state->error);

    if (bytes_read == -1)
    {
        /* FIXME: Convert state->error to errno. */
        errno = EIO;
        return 0;
    }
    else
    {
        return bytes_read;
    }
}

/*
 * et_ogg_seek_func:
 * @datasource: the Ogg parser state
 * @offset: the number of bytes to seek
 * @whence: either %SEEK_SET, %SEEK_CUR or %SEEK_END
 *
 * Seek in the currently-open Ogg file.
 *
 * Returns: 0 on success, -1 and sets errno on error
 */
static int
et_ogg_seek_func (void *datasource, ogg_int64_t offset, int whence)
{
    EtOggState *state = (EtOggState *)datasource;
    GSeekType seektype;

    if (!g_seekable_can_seek (G_SEEKABLE (state->istream)))
    {
        return -1;
    }
    else
    {
        switch (whence)
        {
            case SEEK_SET:
                seektype = G_SEEK_SET;
                break;
            case SEEK_CUR:
                seektype = G_SEEK_CUR;
                break;
            case SEEK_END:
                seektype = G_SEEK_END;
                break;
            default:
                errno = EINVAL;
                return -1;
        }

        if (g_seekable_seek (G_SEEKABLE (state->istream), offset, seektype,
                             NULL, &state->error))
        {
            return 0;
        }
        else
        {
            errno = EBADF;
            return -1;
        }
    }
}

/*
 * et_ogg_close_func:
 * @datasource: the Ogg parser state
 *
 * Close the Ogg stream and invalidate the parser state given by @datasource.
 * Be sure to check the error field before invaidating the state.
 *
 * Returns: 0
 */
static int
et_ogg_close_func (void *datasource)
{
    EtOggState *state = (EtOggState *)datasource;

    g_clear_object (&state->file);
    g_clear_object (&state->istream);
    g_clear_error (&state->error);

    return 0;
}

/*
 * et_ogg_tell_func:
 * @datasource: the Ogg parser state
 *
 * Tell the current position of the stream from the beginning of the Ogg file.
 *
 * Returns: the current position in the Ogg file
 */
static long
et_ogg_tell_func (void *datasource)
{
    EtOggState *state = (EtOggState *)datasource;

    return g_seekable_tell (G_SEEKABLE (state->istream));
}

gboolean
ogg_header_read_file_info (const gchar *filename,
                           ET_File_Info *ETFileInfo,
                           GError **error)
{
    OggVorbis_File vf;
    vorbis_info *vi;
    gint encoder_version = 0;
    gint channels = 0;
    glong rate = 0;
    glong bitrate_nominal = 0;
    gdouble duration = 0;
    gulong filesize;
    gint res;
    ov_callbacks callbacks = { et_ogg_read_func, et_ogg_seek_func,
                               et_ogg_close_func, et_ogg_tell_func };
    EtOggState state;
    gchar *filename_utf8;

    g_return_val_if_fail (filename != NULL && ETFileInfo != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    state.file = g_file_new_for_path (filename);
    state.error = NULL;
    state.istream = G_INPUT_STREAM (g_file_read (state.file, NULL,
                                                 &state.error));

    filename_utf8 = filename_to_display (filename);

    if (!state.istream)
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Error while opening file ‘%s’: %s"), filename_utf8,
                     state.error->message);
        g_free (filename_utf8);
        return FALSE;
    }

    if ((res = ov_open_callbacks (&state, &vf, NULL, 0, callbacks)) == 0)
    {
        if ( (vi=ov_info(&vf,0)) != NULL )
        {
            encoder_version = vi->version;         // Vorbis encoder version used to create this bitstream.
            channels        = vi->channels;        // Number of channels in bitstream.
            rate            = vi->rate;            // (Hz) Sampling rate of the bitstream.
            bitrate_nominal = vi->bitrate_nominal; // (b/s) Specifies the average bitrate for a VBR bitstream.
        }
        else
        {
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("The specified bitstream does not exist or the "
                         "file has been initialized improperly (file: ‘%s’)"),
                       filename_utf8);
            g_free (filename_utf8);
            et_ogg_close_func (&state);
            return FALSE;
        }

        duration        = ov_time_total(&vf,-1); // (s) Total time.

        ov_clear(&vf); // This close also the file
    }else
    {
        /* On error. */
        if (state.error)
        {
            gchar *message;

            switch (res)
            {
                case OV_EREAD:
                    message = _("Read from media returned an error");
                    break;
                case OV_ENOTVORBIS:
                    message = _("Bitstream is not Vorbis data");
                    break;
                case OV_EVERSION:
                    message = _("Vorbis version mismatch");
                    break;
                case OV_EBADHEADER:
                    message = _("Invalid Vorbis bitstream header");
                    break;
                case OV_EFAULT:
                    message = _("Internal logic fault, indicates a bug or heap/stack corruption");
                    break;
                default:
                    message = "";
                    break;
            }

            g_set_error (error, state.error->domain, state.error->code,
                         "%s", message);
            et_ogg_close_func (&state);
            g_free (filename_utf8);
            return FALSE;
        }

        et_ogg_close_func (&state);
    }

    filesize = et_get_file_size (filename);

    ETFileInfo->version    = encoder_version;
    ETFileInfo->bitrate    = bitrate_nominal/1000;
    ETFileInfo->samplerate = rate;
    ETFileInfo->mode       = channels;
    ETFileInfo->size       = filesize;
    ETFileInfo->duration   = duration;

    g_free(filename_utf8);
    return TRUE;
}


#ifdef ENABLE_SPEEX

gboolean
speex_header_read_file_info (const gchar *filename,
                             ET_File_Info *ETFileInfo,
                             GError **error)
{
    vcedit_state *state;
    SpeexHeader  *si;
    gchar *encoder_version = NULL;
    gint channels = 0;
    glong rate = 0;
    glong bitrate = 0;
    gdouble duration = 0;
    gulong filesize;
    GFile *gfile;
    GError *tmp_error = NULL;

    g_return_val_if_fail (filename != NULL && ETFileInfo != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    state = vcedit_new_state();    // Allocate memory for 'state'
    gfile = g_file_new_for_path (filename);

    if (!vcedit_open (state, gfile, &tmp_error))
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Failed to open file as Vorbis: %s"),
                     tmp_error->message);
        g_error_free (tmp_error);
        g_object_unref (gfile);
        vcedit_clear (state);
        return FALSE;
    }

    // Get Speex information
    if ( (si=state->si) != NULL )
    {
        encoder_version = si->speex_version;
        channels        = si->nb_channels;        // Number of channels in bitstream.
        rate            = si->rate;               // (Hz) Sampling rate of the bitstream.
        bitrate         = si->bitrate;            // (b/s) Specifies the bitrate

        duration        = 0;//ov_time_total(&vf,-1); // (s) Total time.

        //g_print("play time: %ld s\n",(long)ov_time_total(&vf,-1));
        //g_print("serialnumber: %ld\n",(long)ov_serialnumber(&vf,-1));
        //g_print("compressed length: %ld bytes\n",(long)(ov_raw_total(&vf,-1)));
    }

    filesize = et_get_file_size (filename);

    ETFileInfo->mpc_version = g_strdup(encoder_version);
    ETFileInfo->bitrate     = bitrate/1000;
    ETFileInfo->samplerate  = rate;
    ETFileInfo->mode        = channels;
    ETFileInfo->size        = filesize;
    //if (bitrate > 0)
    //    ETFileInfo->duration = filesize*8/bitrate/1000; // FIXME : Approximation!! Needs to remove tag size!
    //else
        ETFileInfo->duration   = duration;

    vcedit_clear(state);
    g_object_unref (gfile);
    return TRUE;
}
#endif

EtFileHeaderFields *
et_ogg_header_display_file_info_to_ui (const ET_File *ETFile)
{
    EtFileHeaderFields *fields;
    ET_File_Info *info;
    gchar *time = NULL;
    gchar *time1 = NULL;
    gchar *size = NULL;
    gchar *size1 = NULL;

    info = ETFile->ETFileInfo;
    fields = g_slice_new (EtFileHeaderFields);

    switch (ETFile->ETFileDescription->FileType)
    {
        case OGG_FILE:
            fields->description = _("Ogg Vorbis File");
            break;
        case SPEEX_FILE:
            fields->description = _("Speex File");
            break;
        default:
            g_assert_not_reached ();
    }

    /* Encoder version */
    fields->version_label = _("Encoder:");

    if (!info->mpc_version)
    {
        fields->version = g_strdup_printf ("%d", info->version);
    }
    else
    {
        fields->version = g_strdup (info->mpc_version);
    }

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
et_ogg_file_header_fields_free (EtFileHeaderFields *fields)
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

#endif /* ENABLE_OGG */
