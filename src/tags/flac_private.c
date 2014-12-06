/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014 David King <amigadave@amigadave.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "flac_private.h"

#include <errno.h>

static size_t
read_func (GInputStream *istream,
           void *buffer,
           gsize count,
           gboolean *eof,
           GError **error)
{
    gssize bytes_read;

    *eof = FALSE;

    bytes_read = g_input_stream_read (istream, buffer, count, NULL, error);

    if (bytes_read == -1)
    {
        errno = EIO;
        return 0;
    }
    else if (bytes_read == 0)
    {
        *eof = TRUE;
    }

    return bytes_read;
}

size_t
et_flac_read_read_func (void *ptr,
                        size_t size,
                        size_t nmemb,
                        FLAC__IOHandle handle)
{
    EtFlacReadState *state;

    state = (EtFlacReadState *)handle;

    return read_func (G_INPUT_STREAM (state->istream), ptr, size * nmemb,
                      &state->eof, &state->error);
}

size_t
et_flac_write_read_func (void *ptr,
                         size_t size,
                         size_t nmemb,
                         FLAC__IOHandle handle)
{
    EtFlacWriteState *state;

    state = (EtFlacWriteState *)handle;

    return read_func (G_INPUT_STREAM (state->istream), ptr, size * nmemb,
                      &state->eof, &state->error);
}

size_t
et_flac_write_write_func (const void *ptr,
                          size_t size,
                          size_t nmemb,
                          FLAC__IOHandle handle)
{
    EtFlacWriteState *state;
    gsize bytes_written;

    state = (EtFlacWriteState *)handle;

    if (!g_output_stream_write_all (G_OUTPUT_STREAM (state->ostream), ptr,
                                    size * nmemb, &bytes_written, NULL,
                                    &state->error))
    {
        g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %" G_GSIZE_FORMAT
                 " bytes of data were written", bytes_written,
                 size);
        errno = EIO;
    }

    return bytes_written;
}

static gint
seek_func (GSeekable *seekable,
           goffset offset,
           gint whence,
           GError **error)
{
    GSeekType seektype;

    if (!g_seekable_can_seek (seekable))
    {
        errno = EBADF;
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

        if (g_seekable_seek (seekable, offset, seektype, NULL, error))
        {
            return 0;
        }
        else
        {
            /* TODO: More suitable error. */
            errno = EINVAL;
            return -1;
        }
    }
}

int
et_flac_read_seek_func (FLAC__IOHandle handle,
                        FLAC__int64 offset,
                        int whence)
{
    EtFlacReadState *state;

    state = (EtFlacReadState *)handle;

    return seek_func (G_SEEKABLE (state->istream), offset, whence,
                      &state->error);
}

int
et_flac_write_seek_func (FLAC__IOHandle handle,
                         FLAC__int64 offset,
                         int whence)
{
    EtFlacWriteState *state;

    state = (EtFlacWriteState *)handle;

    /* Seeking on the IOStream causes both the input and output stream to have
     * the same offset. */
    return seek_func (G_SEEKABLE (state->iostream), offset, whence,
                      &state->error);
}

static FLAC__int64
tell_func (GSeekable *seekable)
{
    if (!g_seekable_can_seek (seekable))
    {
        errno = EBADF;
        return -1;
    }
    else
    {
        return g_seekable_tell (seekable);
    }
}

FLAC__int64
et_flac_read_tell_func (FLAC__IOHandle handle)
{
    EtFlacReadState *state;

    state = (EtFlacReadState *)handle;

    return tell_func (G_SEEKABLE (state->istream));
}

FLAC__int64
et_flac_write_tell_func (FLAC__IOHandle handle)
{
    EtFlacWriteState *state;

    state = (EtFlacWriteState *)handle;

    return tell_func (G_SEEKABLE (state->iostream));
}

int
et_flac_read_eof_func (FLAC__IOHandle handle)
{
    EtFlacReadState *state;

    state = (EtFlacReadState *)handle;

    /* EOF is not directly supported by GFileInputStream. */
    return state->eof ? 1 : 0;
}

int
et_flac_write_eof_func (FLAC__IOHandle handle)
{
    EtFlacWriteState *state;

    state = (EtFlacWriteState *)handle;

    /* EOF is not directly supported by GFileInputStream. */
    return state->eof ? 1 : 0;
}

int
et_flac_read_close_func (FLAC__IOHandle handle)
{
    EtFlacReadState *state;

    state = (EtFlacReadState *)handle;

    g_clear_object (&state->istream);
    g_clear_error (&state->error);

    /* Always return success. */
    return 0;
}

int
et_flac_write_close_func (FLAC__IOHandle handle)
{
    EtFlacWriteState *state;

    state = (EtFlacWriteState *)handle;

    g_clear_object (&state->file);
    g_clear_object (&state->iostream);
    g_clear_error (&state->error);

    /* Always return success. */
    return 0;
}
