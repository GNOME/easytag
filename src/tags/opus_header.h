/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014 Abhinav Jangda <abhijangda@hotmail.com>
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

#ifndef ET_OPUS_HEADER_H_
#define ET_OPUS_HEADER_H_

#include <glib.h>
#include <opus/opusfile.h>

#include "et_core.h"

/*
 * Error domain and codes for errors while reading/writing Opus files
 */
GQuark et_opus_error_quark (void);

#define ET_OPUS_ERROR et_opus_error_quark ()

/*
 * EtOpusError:
 * @ET_OPUS_ERROR_READ: Error reading file
 * @ET_OPUS_ERROR_FAULT: Memory allocation failure or internal library error
 * @ET_OPUS_ERROR_IMPL: Stream used an unimplemented feature
 * @ET_OPUS_ERROR_INVAL: seek () succeeded on this source but tell () didn't
 * @ET_OPUS_ERROR_NOTFORMAT: No logical stream found in a link
 * @ET_OPUS_ERROR_BADHEADER: Corrputed header packet
 * @ET_OPUS_ERROR_VERSION: ID header contained an unrecognized version number
 * @ET_OPUS_ERROR_BADLINK: Corrupted link found
 * @ET_OPUS_ERROR_BADTIMESTAMP: First/last timestamp in a link failed checks
 *
 * Errors that can occur when reading Opus files.
 */
typedef enum
{
    ET_OPUS_ERROR_READ,
    ET_OPUS_ERROR_FAULT,
    ET_OPUS_ERROR_IMPL,
    ET_OPUS_ERROR_INVAL,
    ET_OPUS_ERROR_NOTFORMAT,
    ET_OPUS_ERROR_BADHEADER,
    ET_OPUS_ERROR_VERSION,
    ET_OPUS_ERROR_BADLINK,
    ET_OPUS_ERROR_BADTIMESTAMP,
} EtOpusError;

gboolean et_opus_read_file_info (GFile *gfile, ET_File_Info *ETFileInfo, GError **error);
OggOpusFile * et_opus_open_file (GFile *gfile, GError **error);
EtFileHeaderFields * et_opus_header_display_file_info_to_ui (const ET_File *ETFile);
void et_opus_file_header_fields_free (EtFileHeaderFields *fields);

#endif /* ET_OPUS_HEADER_H_ */
