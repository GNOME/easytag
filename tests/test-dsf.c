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

#include "dsf_header.h"

ET_Core *ETCore;

#if 0
static void
dsf_header (void)
{
    ET_File_Info *info;
    GError *error = NULL;

    info = ET_File_Info_Item_New ();
    et_dsf_header_read_file_info (file, info, &error);

    g_assert_no_error (error);
}
#endif

static void
dsf_header_to_fields (void)
{
    ET_File file;
    ET_File_Info info;
    ET_Core core;
    EtFileHeaderFields *fields;

    file.ETFileInfo = &info;

    info.version = 0;
    info.bitrate = 2824;
    info.samplerate = 2822400;
    info.mode = 4;
    info.size = 10;
    info.duration = 10;

    /* FIXME: Remove ETCore dependence from tagging code. */
    ETCore = &core;
    ETCore->ETFileDisplayedList_TotalSize = 20;
    ETCore->ETFileDisplayedList_TotalDuration = 20;

    fields = et_dsf_header_display_file_info_to_ui (&file);

    g_assert_cmpstr (fields->description, ==, "DSF File");
    g_assert_cmpstr (fields->version, ==, "DSD raw");
    g_assert_cmpstr (fields->bitrate, ==, "2824 kb/s");
    g_assert_cmpstr (fields->samplerate, ==, "2822400 Hz");
    g_assert_cmpstr (fields->mode, ==, "Quadrophonic");
    g_assert_cmpstr (fields->size, ==, "10 bytes (20 bytes)");
    g_assert_cmpstr (fields->duration, ==, "0:10 (0:20)");

    et_dsf_file_header_fields_free (fields);
}

int
main (int argc, char** argv)
{
    g_test_init (&argc, &argv, NULL);

#if 0
    g_test_add_func ("/dsf/header", dsf_header);
#endif
    g_test_add_func ("/dsf/header-to-fields", dsf_header_to_fields);

    return g_test_run ();
}
