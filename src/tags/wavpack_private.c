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

#include "wavpack_private.h"

#ifdef ENABLE_WAVPACK

/* For EOF. */
#include <stdio.h>

int32_t
wavpack_read_bytes (void *id,
                    void *data,
                    int32_t bcount)
{
    EtWavpackState *state;
    gssize bytes_written;

    state = (EtWavpackState *)id;

    bytes_written = g_input_stream_read (G_INPUT_STREAM (state->istream), data,
                                         bcount, NULL, &state->error);

    if (bytes_written == -1)
    {
        return 0;
    }

    return bytes_written;
}

uint32_t
wavpack_get_pos (void *id)
{
    EtWavpackState *state;

    state = (EtWavpackState *)id;

    return g_seekable_tell (state->seekable);
}

int
wavpack_set_pos_abs (void *id,
                     uint32_t pos)
{
    EtWavpackState *state;

    state = (EtWavpackState *)id;

    if (!g_seekable_seek (state->seekable, pos, G_SEEK_SET, NULL,
                          &state->error))
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int
wavpack_set_pos_rel (void *id,
                     int32_t delta,
                     int mode)
{
    EtWavpackState *state;
    GSeekType seek_type;

    state = (EtWavpackState *)id;

    switch (mode)
    {
        case SEEK_SET:
            seek_type = G_SEEK_SET;
            break;
        case SEEK_CUR:
            seek_type = G_SEEK_CUR;
            break;
        case SEEK_END:
            seek_type = G_SEEK_END;
            break;
        default:
            g_assert_not_reached ();
            break;
    }

    if (!g_seekable_seek (state->seekable, delta, seek_type, NULL,
                          &state->error))
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int
wavpack_push_back_byte (void *id,
                        int c)
{
    EtWavpackState *state;

    state = (EtWavpackState *)id;

    if (!g_seekable_seek (state->seekable, -1, G_SEEK_CUR, NULL,
                          &state->error))
    {
        return EOF;
    }

    /* TODO: Check if c is present in the input stream. */
    return c;
}

uint32_t
wavpack_get_length (void *id)
{
    EtWavpackState *state;
    GFileInfo *info;
    goffset size;

    state = (EtWavpackState *)id;

    info = g_file_input_stream_query_info (state->istream,
                                           G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                           NULL, &state->error);

    if (!info)
    {
        return 0;
    }

    size = g_file_info_get_size (info);
    g_object_unref (info);

    return size;
}

int
wavpack_can_seek (void *id)
{
    EtWavpackState *state;

    state = (EtWavpackState *)id;

    return g_seekable_can_seek (state->seekable);
}

int32_t
wavpack_write_bytes (void *id,
                     void *data,
                     int32_t bcount)
{
    EtWavpackWriteState *state;
    gssize bytes_written;

    state = (EtWavpackWriteState *)id;

    bytes_written = g_output_stream_write (G_OUTPUT_STREAM (state->ostream),
                                           data, bcount, NULL, &state->error);

    if (bytes_written == -1)
    {
        return 0;
    }

    return bytes_written;
}

#endif /* ENABLE_WAVPACK */
