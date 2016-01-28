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

#include <errno.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "info_mac.h"
#include "is_tag.h"

#define MAC_FORMAT_FLAG_8_BIT                 1    // 8-bit wave
#define MAC_FORMAT_FLAG_CRC                   2    // new CRC32 error detection
#define MAC_FORMAT_FLAG_HAS_PEAK_LEVEL        4    // u-long Peak_Level after the header
#define MAC_FORMAT_FLAG_24_BIT                8    // 24-bit wave
#define MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS    16    // number of seek elements after the peak level
#define MAC_FORMAT_FLAG_CREATE_WAV_HEADER    32    // wave header not stored

#define MAC_FORMAT_HEADER_LENGTH 16

struct macHeader {
    char             id[4];               // should equal 'MAC '
    unsigned short   ver;                 // version number * 1000 (3.81 = 3810)
    unsigned short   compLevel;           // the compression level
    unsigned short   formatFlags;         // any format flags (for future use)
    unsigned short   channels;            // the number of channels (1 or 2)
    unsigned long    sampleRate;          // the sample rate (typically 44100)
    unsigned long    headerBytesWAV;      // the bytes after the MAC header that compose the WAV header
    unsigned long    terminatingBytesWAV; // the bytes after that raw data (for extended info)
    unsigned long    totalFrames;         // the number of frames in the file
    unsigned long    finalFrameBlocks;    // the number of samples in the final frame
    unsigned long    peakLevel;
    unsigned short   seekElements;
};
    

// local prototypes
static int 
monkey_samples_per_frame(unsigned int versionid, unsigned int compressionlevel);
static const char *
monkey_stringify(unsigned int profile);

static const char *
monkey_stringify(unsigned int profile)
{
        static const char na[] = "unknown";
        static const char *Names[] = {
                na, "Fast", "Normal", "High", "Extra-High", "Insane"
        };
        unsigned int profile2 = profile/1000;

        return (profile2 >= sizeof (Names) / sizeof (*Names)) ? na : Names[(profile2)];
}


static int 
monkey_samples_per_frame(unsigned int versionid, unsigned int compressionlevel) 
{
    if (versionid >= 3950) {
        return 294912; // 73728 * 4
    } else if (versionid >= 3900) {
        return 73728;
    } else if ((versionid >= 3800) && (compressionlevel == COMPRESSION_LEVEL_EXTRA_HIGH)) {
        return 73728;
    } else {
        return 9216;
    }
}    
    
/*
 * info_mac_read:
 * @file: file from which to read a header
 * @stream_info: stream information to fill
 * @error: a #GError, or NULL
 *
 * Read the header information from a Monkey's Audio file.
 *
 * Returns: %TRUE on success, or %FALSE and with @error set on failure
*/
gboolean
info_mac_read (GFile *file,
               StreamInfoMac *stream_info,
               GError **error)
{
    GFileInfo *info;
    GFileInputStream *istream;
    guint8 header_buffer[MAC_FORMAT_HEADER_LENGTH];
    gsize bytes_read;
    gsize size_id3;
    struct macHeader *header;
    
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_SIZE,
                              G_FILE_QUERY_INFO_NONE, NULL, error);

    if (!info)
    {
        return FALSE;
    }

    stream_info->FileSize = g_file_info_get_size (info);
    g_object_unref (info);

    istream = g_file_read (file, NULL, error);

    if (!istream)
    {
        return FALSE;
    }

    /* FIXME: is_id3v2() should accept an istream or a GFile. */
    {
        gchar *path;
        FILE *fp;

        path = g_file_get_path (file);
        fp = g_fopen (path, "rb");

        if (!fp)
        {
            /* TODO: Add specific error domain and message. */
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "%s",
                         g_strerror (EINVAL));
            g_free (path);
            return FALSE;
        }

        size_id3 = is_id3v2 (fp);
        fclose (fp);
        g_free (path);
    }

    if (!g_seekable_seek (G_SEEKABLE (istream), size_id3, G_SEEK_SET, NULL,
                          error))
    {
        return FALSE;
    }

    if (!g_input_stream_read_all (G_INPUT_STREAM (istream), header_buffer,
                                  MAC_FORMAT_HEADER_LENGTH, &bytes_read, NULL,
                                  error))
    {
        g_debug ("Only %" G_GSIZE_FORMAT " bytes out of 16 bytes of data were "
                 "read", bytes_read);
        return FALSE;
    }

    if (memcmp (header_buffer, "MAC", 3) != 0)
    {
        /* TODO: Add specific error domain and message. */
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "%s",
                     g_strerror (EINVAL));
        return FALSE; // no monkeyAudio file
    }
    
    header = (struct macHeader *) header_buffer;
    
    stream_info->Version = stream_info->EncoderVersion = header->ver;
    stream_info->Channels = header->channels;
    stream_info->SampleFreq = header->sampleRate;
    stream_info->Flags = header->formatFlags;
    stream_info->SamplesPerFrame = monkey_samples_per_frame(header->ver, header->compLevel);
    stream_info->BitsPerSample = (header->formatFlags & MAC_FORMAT_FLAG_8_BIT)
                                  ? 8 : ((header->formatFlags & MAC_FORMAT_FLAG_24_BIT) ? 24 : 16);
    
    stream_info->PeakLevel = header->peakLevel;
//    Info->PeakRatio       = Info->PakLevel / pow(2, Info->bitsPerSample - 1);
    stream_info->Frames = header->totalFrames;
    stream_info->Samples = (stream_info->Frames - 1)
                           * stream_info->SamplesPerFrame
                           + header->finalFrameBlocks;
    
    stream_info->Duration = stream_info -> SampleFreq > 0 ?
                            ((float)stream_info->Samples
                             / stream_info->SampleFreq) * 1000 : 0;
    
    stream_info->Compresion = header->compLevel;
    stream_info->CompresionName = monkey_stringify (stream_info->Compresion);
    
    stream_info->UncompresedSize = stream_info->Samples
                                   * stream_info->Channels
                                   * (stream_info->BitsPerSample / 8);
    
    stream_info->CompresionRatio = (stream_info->UncompresedSize
                                    + header->headerBytesWAV) > 0
                                    ? stream_info->FileSize
                                      / (float) (stream_info->UncompresedSize
                                                 + header->headerBytesWAV) : 0.;
    
    stream_info->Bitrate = stream_info->Duration > 0
                           ? (((stream_info->Samples * stream_info->Channels
                                * stream_info->BitsPerSample)
                               / (float) stream_info->Duration)
                              * stream_info->CompresionRatio) * 1000 : 0;
    
    stream_info->PeakRatio = stream_info->ByteLength = 0;

    return TRUE;
}
