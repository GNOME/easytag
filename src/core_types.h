/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
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

#ifndef ET_CORE_TYPES_H_
#define ET_CORE_TYPES_H_

#include <glib.h>

G_BEGIN_DECLS

/*
 * ET_File_Type:
 * @MP2_FILE: MPEG audio Layer 2: .mp2 (.mpg) (.mpga)
 * @MP3_FILE: MPEG audio Layer 3: .mp3 (.mpg) (.mpga)
 * @MP4_FILE: MPEG audio Layer 4 / AAC: .mp4 (.m4a) (.m4p) (.m4v)
 * @OGG_FILE: Ogg Vorbis audio: .ogg (.ogm)
 * @FLAC_FILE: FLAC (lossless): .flac .fla
 * @MPC_FILE: MusePack: .mpc .mp+ .mpp
 * @MAC_FILE: Monkey's Audio (lossless): .ape (.mac)
 * @SPEEX_FILE: Speech audio files: .spx
 * @OFR_FILE: OptimFROG (lossless): .ofr .ofs
 * @WAVPACK_FILE: Wavpack (lossless): .wv
 * @OPUS_FILE: Ogg Opus audio: .opus
 * @UNKNOWN_FILE: not a recognized file
 * Types of files
 */
typedef enum
{ /* (.ext) is not so popular. */
    MP2_FILE = 0,
    MP3_FILE,
    MP4_FILE,
    OGG_FILE,
    FLAC_FILE,
    MPC_FILE,
    MAC_FILE,
    SPEEX_FILE,
    OFR_FILE,
    WAVPACK_FILE,
    OPUS_FILE,
    UNKNOWN_FILE
} ET_File_Type;

/*
 * Types of tags
 */
typedef enum
{
    ID3_TAG = 0,
    OGG_TAG,
    APE_TAG,
    FLAC_TAG,
    MP4_TAG,
    WAVPACK_TAG,
    OPUS_TAG,
    UNKNOWN_TAG
} ET_Tag_Type;

/*
 * EtFileHeaderFields:
 * @description: a description of the file type, such as MP3 File
 * @version_label: the label for the encoder version, such as MPEG
 * @version: the encoder version (such as 2, Layer III)
 * @bitrate: the bitrate of the file (not the bit depth of the samples)
 * @samplerate: the sample rate of the primary audio track, generally in Hz
 * @mode_label: the label for the audio mode, for example Mode
 * @mode: the audio mode (stereo, mono, and so on)
 * @size: the size of the audio file
 * @duration: the length of the primary audio track
 *
 * UI-visible strings, populated by the tagging support code to be displayed in
 * the EtFileArea.
 */
typedef struct
{
    /*< public >*/
    gchar *description;
    gchar *version_label;
    gchar *version;
    gchar *bitrate;
    gchar *samplerate;
    gchar *mode_label;
    gchar *mode;
    gchar *size;
    gchar *duration;
} EtFileHeaderFields;

G_END_DECLS

#endif /* ET_TYPES_H_ */
