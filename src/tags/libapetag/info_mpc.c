/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014 David King <amigadave@amigadave.com>
 * Copyright (C) 2002 Artur Polaczynski (Ar't) <artii@o2.pl>
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

/*
    Some portions of code or/and ideas come from 
    winamp plugins, xmms plugins, mppdec decoder
    thanks:
    -Frank Klemm <Frank.Klemm@uni-jena.de> 
    -Andree Buschmann <Andree.Buschmann@web.de> 
    -Thomas Juerges <thomas.juerges@astro.ruhr-uni-bochum.de> 
*/

#include <errno.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "info_mpc.h"
#include "is_tag.h"

#define MPC_HEADER_LENGTH 16

/*
*.MPC,*.MP+,*.MPP
*/
/* Profile is 0...15, where 7...13 is used. */
static const char *
profile_stringify (unsigned int profile)
{
    static const char na[] = "n.a.";
    static const char *Names[] = {
        na, "Experimental", na, na,
        na, na, na, "Telephone",
        "Thumb", "Radio", "Standard", "Xtreme",
        "Insane", "BrainDead", "BrainDead+", "BrainDead++"
    };

    return profile >=
        sizeof (Names) / sizeof (*Names) ? na : Names[profile];
}

/*
* info_mpc_read:
* @file: file from which to read a header
* @stream_info: stream information to fill
* @error: a #Gerror, or %NULL
*
* Read header from the given MusePack @file.
*
* Returns: %TRUE on success, %FALSE and with @error set on failure
*/
gboolean
info_mpc_read (GFile *file,
               StreamInfoMpc *stream_info,
               GError **error)
{
    GFileInfo *info;
    GFileInputStream *istream;
    guint32 header_buffer[MPC_HEADER_LENGTH];
    gsize bytes_read;
    gsize id3_size;
    
    info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_SIZE,
                              G_FILE_QUERY_INFO_NONE, NULL, error);

    if (!info)
    {
        return FALSE;
    }

    stream_info->FileSize = g_file_info_get_size (info);
    g_object_unref (info);

    {
        gchar *path;
        FILE *fp;

        path = g_file_get_path (file);
        fp = g_fopen (path, "rb");
        g_free (path);

        if (!fp)
        {
            /* TODO: Add specific error domain and message. */
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "%s",
                         g_strerror (EINVAL));
            return FALSE;
        }

        /* Skip id3v2. */
        /* FIXME: is_id3v2 (and is_id3v1 and is_ape) should accept an istream
         * or GFile. */
        id3_size = is_id3v2 (fp);

        fseek (fp, 0, SEEK_END);
        stream_info->FileSize = ftell (fp);

        /* Stream size. */
        stream_info->ByteLength = stream_info->FileSize - is_id3v1 (fp)
                                  - is_ape (fp) - id3_size;

        fclose (fp);
    }
        
    istream = g_file_read (file, NULL, error);

    if (!istream)
    {
        return FALSE;
    }

    if (!g_seekable_seek (G_SEEKABLE (istream), id3_size, G_SEEK_SET, NULL,
                          error))
    {
        return FALSE;
    }

    /* Read 16 guint32. */
    if (!g_input_stream_read_all (G_INPUT_STREAM (istream), header_buffer,
                                  MPC_HEADER_LENGTH * 4, &bytes_read, NULL,
                                  error))
    {
        g_debug ("Only %" G_GSIZE_FORMAT "bytes out of 16 bytes of data were "
                 "read", bytes_read);
        return FALSE;
    }

    /* FIXME: Read 4 bytes, take as a uint32, then byteswap if necessary. (The
     * official Musepack decoder expects the user(!) to request the
     * byteswap.) */
    if (memcmp (header_buffer, "MP+", 3) != 0)
    {
        /* TODO: Add specific error domain and message. */
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "%s",
                     g_strerror (EINVAL));
        return FALSE;
    }
    
    stream_info->StreamVersion = header_buffer[0] >> 24;

    if (stream_info->StreamVersion >= 7)
    {
        const long samplefreqs[4] = { 44100, 48000, 37800, 32000 };
        
        // read the file-header (SV7 and above)
        stream_info->Bitrate = 0;
        stream_info->Frames = header_buffer[1];
        stream_info->SampleFreq = samplefreqs[(header_buffer[2] >> 16) & 0x0003];
        stream_info->MaxBand = (header_buffer[2] >> 24) & 0x003F;
        stream_info->MS = (header_buffer[2] >> 30) & 0x0001;
        stream_info->Profile = (header_buffer[2] << 8) >> 28;
        stream_info->IS = (header_buffer[2] >> 31) & 0x0001;
        stream_info->BlockSize = 1;
        
        stream_info->EncoderVersion = (header_buffer[6] >> 24) & 0x00FF;
        stream_info->Channels = 2;
        // gain
        stream_info->EstPeakTitle = header_buffer[2] & 0xFFFF;    // read the ReplayGain data
        stream_info->GainTitle = (header_buffer[3] >> 16) & 0xFFFF;
        stream_info->PeakTitle = header_buffer[3] & 0xFFFF;
        stream_info->GainAlbum = (header_buffer[4] >> 16) & 0xFFFF;
        stream_info->PeakAlbum = header_buffer[4] & 0xFFFF;
        // gaples
        stream_info->IsTrueGapless = (header_buffer[5] >> 31) & 0x0001;    // true gapless: used?
        stream_info->LastFrameSamples = (header_buffer[5] >> 20) & 0x07FF;    // true gapless: valid samples for last frame
        
        if (stream_info->EncoderVersion == 0)
        {
            sprintf (stream_info->Encoder, "<= 1.05"); // Buschmann 1.7.x, Klemm <= 1.05
        }
        else
        {
            switch (stream_info->EncoderVersion % 10)
            {
            case 0:
                sprintf (stream_info->Encoder, "%u.%u",
                         stream_info->EncoderVersion / 100,
                         stream_info->EncoderVersion / 10 % 10);
                break;
            case 2:
            case 4:
            case 6:
            case 8:
                sprintf (stream_info->Encoder, "%u.%02u Beta",
                         stream_info->EncoderVersion / 100,
                         stream_info->EncoderVersion % 100);
                break;
            default:
                sprintf (stream_info->Encoder, "%u.%02u Alpha",
                         stream_info->EncoderVersion / 100,
                         stream_info->EncoderVersion % 100);
                break;
            }
        }
        // estimation, exact value needs too much time
        stream_info->Bitrate = (long) (stream_info->ByteLength) * 8. * stream_info->SampleFreq / (1152 * stream_info->Frames - 576);
        
    }
    else
    {
        // read the file-header (SV6 and below)
        stream_info->Bitrate = ((header_buffer[0] >> 23) & 0x01FF) * 1000;    // read the file-header (SV6 and below)
        stream_info->MS = (header_buffer[0] >> 21) & 0x0001;
        stream_info->IS = (header_buffer[0] >> 22) & 0x0001;
        stream_info->StreamVersion = (header_buffer[0] >> 11) & 0x03FF;
        stream_info->MaxBand = (header_buffer[0] >> 6) & 0x001F;
        stream_info->BlockSize = (header_buffer[0]) & 0x003F;
        
        stream_info->Profile = 0;
        //gain
        stream_info->GainTitle = 0;    // not supported
        stream_info->PeakTitle = 0;
        stream_info->GainAlbum = 0;
        stream_info->PeakAlbum = 0;
        //gaples
        stream_info->LastFrameSamples = 0;
        stream_info->IsTrueGapless = 0;
        
        if (stream_info->StreamVersion >= 5)
        {
            stream_info->Frames = header_buffer[1];    // 32 bit
        }
        else
        {
            stream_info->Frames = (header_buffer[1] >> 16);    // 16 bit
        }
        
        stream_info->EncoderVersion = 0;
        stream_info->Encoder[0] = '\0';
#if 0
        if (Info->StreamVersion == 7)
            return ERROR_CODE_SV7BETA;    // are there any unsupported parameters used?
        if (Info->Bitrate != 0)
            return ERROR_CODE_CBR;
        if (Info->IS != 0)
            return ERROR_CODE_IS;
        if (Info->BlockSize != 1)
            return ERROR_CODE_BLOCKSIZE;
#endif
        if (stream_info->StreamVersion < 6)    // Bugfix: last frame was invalid for up to SV5
        {
            stream_info->Frames -= 1;
        }
        
        stream_info->SampleFreq = 44100;    // AB: used by all files up to SV7
        stream_info->Channels = 2;
    }
    
    stream_info->ProfileName = profile_stringify (stream_info->Profile);
    
    stream_info->Duration = (int) (stream_info->Frames * 1152
                                   / (stream_info->SampleFreq / 1000.0));

    return TRUE;
}
