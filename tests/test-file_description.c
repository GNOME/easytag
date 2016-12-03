/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014-2015 David King <amigadave@amigadave.com>
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

#include "config.h"

#include "file_description.h"

static void
file_description_get_extension (void)
{
    gsize i;
    static const struct
    {
        const gchar *filename;
        const gchar *extension;
    } filenames[] =
    {
        { "test.mp3", ".mp3" },
        { "test.mp4.mp3", ".mp3" },
        { "test.mp4mp3", ".mp4mp3" },
        { "test.", "." },
        { "test", NULL },
        { "test.Mp4", ".Mp4" }
    };

    for (i = 0; i < G_N_ELEMENTS (filenames); i++)
    {
        const gchar *extension;
        extension = ET_Get_File_Extension (filenames[i].filename);
        g_assert_cmpstr (extension, ==, filenames[i].extension);
    }
}

static void
file_description_get_file_description (void)
{
    gsize i;
    static const struct
    {
        const gchar *filename;
        ET_File_Type file_type;
        ET_Tag_Type tag_type;
    } filenames[] =
    {
#ifdef ENABLE_MP3
        { "test.mp3", MP3_FILE, ID3_TAG },
        { "test.mp4.mp3", MP3_FILE, ID3_TAG },
        { "test.mp2", MP2_FILE, ID3_TAG },
#else /* !ENABLE_MP3 */
        { "test.mp3", UNKNOWN_FILE, UNKNOWN_TAG },
        { "test.mp4.mp3", UNKNOWN_FILE, UNKNOWN_TAG },
        { "test.mp2", UNKNOWN_FILE, UNKNOWN_TAG },
#endif /* !ENABLE_MP3 */
        { "test.mp4mp3", UNKNOWN_FILE, UNKNOWN_TAG },
        { "test.", UNKNOWN_FILE, UNKNOWN_TAG },
        { "test", UNKNOWN_FILE, UNKNOWN_TAG },
#ifdef ENABLE_MP4
        { "test.Mp4", MP4_FILE, MP4_TAG },
        { "test.mp4", MP4_FILE, MP4_TAG },
        { "test.m4a", MP4_FILE, MP4_TAG },
        { "test.m4p", MP4_FILE, MP4_TAG },
        { "test.m4v", MP4_FILE, MP4_TAG },
        { "test.aac", MP4_FILE, MP4_TAG },
#else /* !ENABLE_MP4 */
        { "test.Mp4", UNKNOWN_FILE, UNKNOWN_TAG },
        { "test.mp4", UNKNOWN_FILE, UNKNOWN_TAG },
        { "test.m4a", UNKNOWN_FILE, UNKNOWN_TAG },
        { "test.m4p", UNKNOWN_FILE, UNKNOWN_TAG },
        { "test.m4v", UNKNOWN_FILE, UNKNOWN_TAG },
#endif /* ENABLE_MP4 */
        { "test.mpeg", UNKNOWN_FILE, UNKNOWN_TAG },
#ifdef ENABLE_OPUS
        { "test.opus", OPUS_FILE, OPUS_TAG },
#else /* !ENABLE_OPUS */
        { "test.opus", UNKNOWN_FILE, UNKNOWN_TAG },
#endif /* !ENABLE_OPUS */
#ifdef ENABLE_OGG
        { "test.ogg", OGG_FILE, OGG_TAG },
        { "test.oga", OGG_FILE, OGG_TAG },
#else /* !ENABLE_OGG */
        { "test.ogg", UNKNOWN_FILE, UNKNOWN_TAG },
        { "test.oga", UNKNOWN_FILE, UNKNOWN_TAG },
#endif /* !ENABLE_OGG */
#ifdef ENABLE_SPEEX
        { "test.spx", SPEEX_FILE, OGG_TAG },
#else /* !ENABLE_SPEEX */
        { "test.spx", UNKNOWN_FILE, UNKNOWN_TAG },
#endif
        { "test.dsf", UNKNOWN_FILE, UNKNOWN_TAG },
#ifdef ENABLE_FLAC
        { "test.flac", FLAC_FILE, FLAC_TAG },
        { "test.fla", FLAC_FILE, FLAC_TAG },
#else /* !ENABLE_FLAC */
        { "test.flac", UNKNOWN_FILE, UNKNOWN_TAG },
        { "test.fla", UNKNOWN_FILE, UNKNOWN_TAG },
#endif /* !ENABLE_FLAC */
        { "test.mpc", MPC_FILE, APE_TAG },
        { "test.mp+", MPC_FILE, APE_TAG },
        { "test.mpp", MPC_FILE, APE_TAG },
        { "test.ape", MAC_FILE, APE_TAG },
        { "test.mac", MAC_FILE, APE_TAG },
        { "test.ofr", OFR_FILE, APE_TAG },
        { "test.ofs", OFR_FILE, APE_TAG },
#ifdef ENABLE_WAVPACK
        { "test.wv", WAVPACK_FILE, WAVPACK_TAG },
#else /* !ENABLE_WAVPACK */
        { "test.wv", UNKNOWN_FILE, UNKNOWN_TAG },
#endif /* !ENABLE_WAVPACK */
        { "test.wvc", UNKNOWN_FILE, UNKNOWN_TAG }
    };

    for (i = 0; i < G_N_ELEMENTS (filenames); i++)
    {
        const ET_File_Description *description;
        description = ET_Get_File_Description (filenames[i].filename);
        g_assert_cmpint (description->FileType, ==, filenames[i].file_type);
        g_assert_cmpint (description->TagType, ==, filenames[i].tag_type);
    }
}

static void
file_description_is_supported (void)
{
    gsize i;
    static const struct
    {
        const gchar *filename;
        gboolean supported;
    } filenames[] =
    {
#ifdef ENABLE_MP3
        { "test.mp3", TRUE },
        { "test.mp4.mp3", TRUE },
        { "test.mp2", TRUE },
#else /* !ENABLE_MP3 */
        { "test.mp3", FALSE },
        { "test.mp4.mp3", FALSE },
        { "test.mp2", FALSE },
#endif /* !ENABLE_MP3 */
        { "test.mp4mp3", FALSE },
        { "test.", FALSE },
        { "test", FALSE },
        { "test.mpeg", FALSE },
#ifdef ENABLE_OPUS
        { "test.opus", TRUE },
#else /* !ENABLE_OPUS */
        { "test.opus", FALSE },
#endif /* !ENABLE_OPUS */
#ifdef ENABLE_OGG
        { "test.ogg", TRUE },
        { "test.oga", TRUE },
#else /* !ENABLE_OGG */
        { "test.ogg", FALSE },
        { "test.oga", FALSE },
#endif /* !ENABLE_OGG */
#ifdef ENABLE_SPEEX
        { "test.spx", TRUE },
#else /* !ENABLE_SPEEX */
        { "test.spx", FALSE },
#endif /* !ENABLE_SPEEX */
#ifdef ENABLE_FLAC
        { "test.flac", TRUE },
        { "test.fla", TRUE },
#else /* !ENABLE_FLAC */
        { "test.flac", FALSE },
        { "test.fla", FALSE },
#endif /* !ENABLE_FLAC */
        { "test.mpc", TRUE },
        { "test.mp+", TRUE },
        { "test.mpp", TRUE },
        { "test.ape", TRUE },
        { "test.mac", TRUE },
        { "test.ofr", TRUE },
        { "test.ofs", TRUE },
#ifdef ENABLE_MP4
        { "test.mp4", TRUE },
        { "test.m4a", TRUE },
        { "test.m4p", TRUE },
        { "test.m4v", TRUE },
#else /* !ENABLE_MP4 */
        { "test.mp4", FALSE },
        { "test.m4a", FALSE },
        { "test.m4p", FALSE },
        { "test.m4v", FALSE },
#endif /* !ENABLE_MP4 */
#ifdef ENABLE_WAVPACK
        { "test.wv", TRUE },
#else /* !ENABLE_WAVPACK */
        { "test.wv", FALSE },
#endif /* !ENABLE_WAVPACK */
        { "test.wvc", FALSE }
    };

    for (i = 0; i < G_N_ELEMENTS (filenames); i++)
    {
        gboolean supported;
        supported = et_file_is_supported (filenames[i].filename);
        g_assert (supported == filenames[i].supported);
    }
}

int
main (int argc, char** argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/file_description/get_extension",
                     file_description_get_extension);
    g_test_add_func ("/file_description/get_file_description",
                     file_description_get_file_description);
    g_test_add_func ("/file_description/is_supported",
                     file_description_is_supported);

    return g_test_run ();
}
