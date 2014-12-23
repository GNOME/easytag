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

#ifndef ET_WAVPACK_PRIVATE_H_
#define ET_WAVPACK_PRIVATE_H_

#include "config.h"

#ifdef ENABLE_WAVPACK

#include <gio/gio.h>
#include <wavpack/wavpack.h>

G_BEGIN_DECLS

typedef struct
{
    GFileInputStream *istream;
    GSeekable *seekable;
    GError *error;
} EtWavpackState;

typedef struct
{
    /* Start fields copied from EtWavpackState. */
    GFileInputStream *istream;
    GSeekable *seekable;
    GError *error;
    /* End fields copied from EtWavpackState. */
    GFileIOStream *iostream;
    GFileOutputStream *ostream;
} EtWavpackWriteState;

int32_t wavpack_read_bytes (void *id, void *data, int32_t bcount);
uint32_t wavpack_get_pos (void *id);
int wavpack_set_pos_abs (void *id, uint32_t pos);
int wavpack_set_pos_rel (void *id, int32_t delta, int mode);
int wavpack_push_back_byte (void *id, int c);
uint32_t wavpack_get_length (void *id);
int wavpack_can_seek (void *id);

/* Only use this with EtWavpackWriteState. */
int32_t wavpack_write_bytes (void *id, void *data, int32_t bcount);

G_END_DECLS

#endif /* ENABLE_WAVPACK */

#endif /* ET_WAVPACK_PRIVATE_H_ */
