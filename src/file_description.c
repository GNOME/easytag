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

#include "config.h"
#include "file_description.h"

#include <string.h>

const ET_File_Description ETFileDescription[] =
{
#ifdef ENABLE_MP3
    { MP3_FILE, ".mp3", ID3_TAG},
    { MP2_FILE, ".mp2", ID3_TAG},
#endif
#ifdef ENABLE_OPUS
    { OPUS_FILE, ".opus", OPUS_TAG},
#endif
#ifdef ENABLE_OGG
    { OGG_FILE, ".ogg", OGG_TAG},
    { OGG_FILE, ".oga", OGG_TAG},
#endif
#ifdef ENABLE_SPEEX
    { SPEEX_FILE, ".spx", OGG_TAG}, /* Implemented by Pierre Dumuid. */
#endif
#ifdef ENABLE_FLAC
    { FLAC_FILE, ".flac", FLAC_TAG},
    { FLAC_FILE, ".fla",  FLAC_TAG},
#endif
    { MPC_FILE, ".mpc", APE_TAG}, /* Implemented by Artur Polaczynski. */
    { MPC_FILE, ".mp+", APE_TAG}, /* Implemented by Artur Polaczynski. */
    { MPC_FILE, ".mpp", APE_TAG}, /* Implemented by Artur Polaczynski. */
    { MAC_FILE, ".ape", APE_TAG}, /* Implemented by Artur Polaczynski. */
    { MAC_FILE, ".mac", APE_TAG}, /* Implemented by Artur Polaczynski. */
    { OFR_FILE, ".ofr", APE_TAG},
    { OFR_FILE, ".ofs", APE_TAG},
#ifdef ENABLE_MP4
    { MP4_FILE, ".mp4", MP4_TAG}, /* Implemented by Michael Ihde. */
    { MP4_FILE, ".m4a", MP4_TAG}, /* Implemented by Michael Ihde. */
    { MP4_FILE, ".m4p", MP4_TAG}, /* Implemented by Michael Ihde. */
    { MP4_FILE, ".m4v", MP4_TAG},
    { MP4_FILE, ".aac", MP4_TAG},
#endif
#ifdef ENABLE_WAVPACK
    { WAVPACK_FILE, ".wv", WAVPACK_TAG}, /* Implemented by Maarten Maathuis. */
#endif
    { UNKNOWN_FILE, "", UNKNOWN_TAG } /* This item must be placed at the end! */
};

const gsize ET_FILE_DESCRIPTION_SIZE = G_N_ELEMENTS (ETFileDescription) - 1;

/*
 * Returns the extension of the file
 */
const gchar *
ET_Get_File_Extension (const gchar *filename)
{
    if (filename)
    {
        return strrchr (filename, '.');
    }
    else
    {
        return NULL;
    }
}


/*
 * Determine description of file using his extension.
 * If extension is NULL or not found into the tab, it returns the last entry for UNKNOWN_FILE.
 */
static const ET_File_Description *
ET_Get_File_Description_From_Extension (const gchar *extension)
{
    guint i;

    if (!extension) // Unknown file
        return &ETFileDescription[ET_FILE_DESCRIPTION_SIZE];

    for (i=0; i<ET_FILE_DESCRIPTION_SIZE; i++)  // Use of '<' instead of '<=' to avoid to test for Unknown file
        if ( strcasecmp(extension,ETFileDescription[i].Extension)==0 )
            return &ETFileDescription[i];

    // If not found in the list
    return &ETFileDescription[ET_FILE_DESCRIPTION_SIZE];
}


/*
 * Determines description of file.
 * Determines first the extension. If extension is NULL or not found into the tab,
 * it returns the last entry for UNKNOWN_FILE.
 */
const ET_File_Description *
ET_Get_File_Description (const gchar *filename)
{
    return ET_Get_File_Description_From_Extension(ET_Get_File_Extension(filename));
}


/*
 * Returns TRUE if the file is supported, else returns FALSE
 */
gboolean
et_file_is_supported (const gchar *filename)
{
    if (ET_Get_File_Description(filename)->FileType != UNKNOWN_FILE)
        return TRUE;
    else
        return FALSE;
}
