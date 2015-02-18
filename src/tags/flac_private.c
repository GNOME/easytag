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

#ifdef ENABLE_FLAC

#include <errno.h>
#include <unistd.h>

size_t
et_flac_read_func (void *ptr,
                   size_t size,
                   size_t nmemb,
                   FLAC__IOHandle handle)
{
    EtFlacReadState *state;
    gssize bytes_read;

    state = (EtFlacReadState *)handle;

    state->eof = FALSE;

    bytes_read = g_input_stream_read (G_INPUT_STREAM (state->istream), ptr,
                                      size * nmemb, NULL, &state->error);

    if (bytes_read == -1)
    {
        errno = EIO;
        return 0;
    }
    else if (bytes_read == 0)
    {
        state->eof = TRUE;
    }

    return bytes_read;
}

size_t
et_flac_write_func (const void *ptr,
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

int
et_flac_seek_func (FLAC__IOHandle handle,
                   FLAC__int64 offset,
                   int whence)
{
    GSeekType seektype;
    EtFlacReadState *state;

    state = (EtFlacReadState *)handle;

    if (!g_seekable_can_seek (state->seekable))
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

        if (g_seekable_seek (state->seekable, offset, seektype, NULL,
                             &state->error))
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

FLAC__int64
et_flac_tell_func (FLAC__IOHandle handle)
{
    EtFlacReadState *state;

    state = (EtFlacReadState *)handle;

    if (!g_seekable_can_seek (state->seekable))
    {
        errno = EBADF;
        return -1;
    }
    else
    {
        return g_seekable_tell (state->seekable);
    }
}

int
et_flac_eof_func (FLAC__IOHandle handle)
{
    EtFlacReadState *state;

    state = (EtFlacReadState *)handle;

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

#endif /* ENABLE_FLAC */
