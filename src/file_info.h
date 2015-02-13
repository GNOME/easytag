/* EasyTAG - tag editor for audio files
 * Copyright (C) 2015  David King <amigadave@amigadave.com>
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
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef ET_FILE_INFO_H_
#define ET_FILE_INFO_H_

#include <glib.h>

G_BEGIN_DECLS

/*
 * Structure containing informations of the header of file
 * Nota: This struct was copied from an "MP3 structure", and will change later.
 */
typedef struct
{
    gint version;               /* Version of bitstream (mpeg version for mp3, encoder version for ogg) */
    gint mpeg25;                /* Version is MPEG 2.5? */
    gsize layer; /* "MP3 data" */
    gint bitrate;               /* Bitrate (kb/s) */
    gboolean variable_bitrate;  /* Is a VBR file? */
    gint samplerate;            /* Samplerate (Hz) */
    gint mode;                  /* Stereo, ... or channels for ogg */
    goffset size;               /* The size of file (in bytes) */
    gint duration;              /* The duration of file (in seconds) */
    gchar *mpc_profile;         /* MPC data */
    gchar *mpc_version;         /* MPC data : encoder version  (also for Speex) */
} ET_File_Info;

ET_File_Info * et_file_info_new (void);
void et_file_info_free (ET_File_Info *file_info);

G_END_DECLS

#endif /* !ET_FILE_INFO_H_ */
