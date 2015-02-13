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

#include "file_info.h"

/*
 * Create a new File_Info structure
 */
ET_File_Info *
et_file_info_new (void)
{
    return g_slice_new0 (ET_File_Info);
}

/*
 * Frees a File_Info item.
 */
void
et_file_info_free (ET_File_Info *file_info)
{
    g_return_if_fail (file_info != NULL);

    g_free (file_info->mpc_profile);
    g_free (file_info->mpc_version);

    g_slice_free (ET_File_Info, file_info);
}
