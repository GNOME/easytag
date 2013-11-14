/* ogg_header.c - 2003/12/29 */
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
Ogg_Header_Read_File_Info (const gchar *filename, ET_File_Info *ETFileInfo)
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

    state.file = g_file_new_for_path (filename);
    state.error = NULL;
    state.istream = G_INPUT_STREAM (g_file_read (state.file, NULL,
                                                 &state.error));

    filename_utf8 = filename_to_display (filename);

    if (!state.istream)
    {
        /* FIXME: Pass error back to calling function. */
        Log_Print (LOG_ERROR, _("Error while opening file: '%s' (%s)"),
                   filename_utf8, state.error->message);
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
        }else
        {
            Log_Print(LOG_ERROR,_("Ogg Vorbis: The specified bitstream does not exist or the "
                        "file has been initialized improperly (file: '%s')."),filename_utf8);
        }

        duration        = ov_time_total(&vf,-1); // (s) Total time.
        //g_print("play time: %ld s\n",(long)ov_time_total(&vf,-1));
        //g_print("serialnumber: %ld\n",(long)ov_serialnumber(&vf,-1));
        //g_print("compressed length: %ld bytes\n",(long)(ov_raw_total(&vf,-1)));

        /***{
            // Test for displaying comments
            vorbis_comment *vc = ov_comment(&vf,-1);
            Log_Print(LOG_OK,">>> %s",filename_utf8);
            Log_Print(LOG_OK,"Nbr comments : %d",vc->comments);
            Log_Print(LOG_OK,"Vendor : %s",vc->vendor);
            char **ptr = vc->user_comments;
            while(*ptr){
              Log_Print(LOG_OK,"> %s",*ptr);
              ++ptr;
            }
        }***/
        ov_clear(&vf); // This close also the file
    }else
    {
        /* On error. */
        if (state.error)
        {
            g_debug ("Ogg Vorbis: error reading header information (%s)",
                     state.error->message);
        }

        et_ogg_close_func (&state);

        switch (res)
        {
            case OV_EREAD:
                Log_Print(LOG_ERROR,_("Ogg Vorbis: Read from media returned an error (file: '%s')."),filename_utf8);
                break;
            case OV_ENOTVORBIS:
                Log_Print(LOG_ERROR,_("Ogg Vorbis: Bitstream is not Vorbis data (file: '%s')."),filename_utf8);
                break;
            case OV_EVERSION:
                Log_Print(LOG_ERROR,_("Ogg Vorbis: Vorbis version mismatch (file: '%s')."),filename_utf8);
                break;
            case OV_EBADHEADER:
                Log_Print(LOG_ERROR,_("Ogg Vorbis: Invalid Vorbis bitstream header (file: '%s')."),filename_utf8);
                break;
            case OV_EFAULT:
                Log_Print(LOG_ERROR,_("Ogg Vorbis: Internal logic fault, indicates a bug or heap/stack corruption (file: '%s')."),filename_utf8);
                break;
            default:
                break;
        }
    }

    filesize = Get_File_Size(filename);

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

gboolean Speex_Header_Read_File_Info (gchar *filename, ET_File_Info *ETFileInfo)
{
    vcedit_state *state;
    SpeexHeader  *si;
    gchar *encoder_version = NULL;
    gint channels = 0;
    glong rate = 0;
    glong bitrate = 0;
    gdouble duration = 0;
    gulong filesize;
    gchar *filename_utf8;
    GFile *gfile;
    GError *error = NULL;

    g_return_val_if_fail (filename != NULL && ETFileInfo != NULL, FALSE);

    filename_utf8 = filename_to_display(filename);

    state = vcedit_new_state();    // Allocate memory for 'state'
    gfile = g_file_new_for_path (filename);
    if (!vcedit_open (state, gfile, &error))
    {
        Log_Print (LOG_ERROR,
                   _("Error: Failed to open file: '%s' as Vorbis (%s)."),
                   filename_utf8, error->message);
        g_error_free (error);
        g_object_unref (gfile);
        g_free(filename_utf8);
        vcedit_clear(state);
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

    filesize = Get_File_Size(filename);

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
    g_free(filename_utf8);
    return TRUE;
}
#endif

gboolean Ogg_Header_Display_File_Info_To_UI (gchar *filename, ET_File_Info *ETFileInfo)
{
    gchar *text;
    gchar *time = NULL;
    gchar *time1 = NULL;
    gchar *size = NULL;
    gchar *size1 = NULL;

    /* Encoder version */
    gtk_label_set_text(GTK_LABEL(VersionLabel),_("Encoder:"));
    if (!ETFileInfo->mpc_version)
    {
        text = g_strdup_printf("%d",ETFileInfo->version);
        gtk_label_set_text(GTK_LABEL(VersionValueLabel),text);
        g_free(text);
    }else
    {
        gtk_label_set_text(GTK_LABEL(VersionValueLabel),ETFileInfo->mpc_version);
    }

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

#endif /* ENABLE_OGG */
