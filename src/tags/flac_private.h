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

#ifndef ET_FLAC_PRIVATE_H_
#define ET_FLAC_PRIVATE_H_

#include "config.h"

#ifdef ENABLE_FLAC

#include <gio/gio.h>
#include <FLAC/metadata.h>

G_BEGIN_DECLS

/*
 * EtFlacReadState:
 *
 * Used as a FLAC__IOHandle for FLAC__metadata_chain_read_with_callbacks().
 */
typedef struct
{
    GFileInputStream *istream;
    gboolean eof;
    GError *error;
} EtFlacReadState;

/*
 * EtFlacWriteState:
 *
 * Used as a FLAC__IOHandle for FLAC__metadata_chain_write_with_callbacks() and
 * FLAC__metadata_chain_write_with_callbacks_and_tempfile().
 */
typedef struct
{
    GFile *file;
    GFileInputStream *istream;
    GFileOutputStream *ostream;
    GFileIOStream *iostream;
    gboolean eof;
    GError *error;
} EtFlacWriteState;

size_t et_flac_read_read_func (void *ptr, size_t size, size_t nmemb, FLAC__IOHandle handle);
int et_flac_read_seek_func (FLAC__IOHandle handle, FLAC__int64 offset, int whence);
FLAC__int64 et_flac_read_tell_func (FLAC__IOHandle handle);
int et_flac_read_eof_func (FLAC__IOHandle handle);
int et_flac_read_close_func (FLAC__IOHandle handle);

size_t et_flac_write_read_func (void *ptr, size_t size, size_t nmemb, FLAC__IOHandle handle);
size_t et_flac_write_write_func (const void *ptr, size_t size, size_t nmemb, FLAC__IOHandle handle);
int et_flac_write_seek_func (FLAC__IOHandle handle, FLAC__int64 offset, int whence);
FLAC__int64 et_flac_write_tell_func (FLAC__IOHandle handle);
int et_flac_write_eof_func (FLAC__IOHandle handle);
int et_flac_write_close_func (FLAC__IOHandle handle);

G_END_DECLS

#endif /* ENABLE_FLAC */

#endif /* ET_FLAC_PRIVATE_H_ */
