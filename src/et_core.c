/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
 * Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "et_core.h"

#include "application_window.h"
#include "file.h"
#include "file_list.h"

ET_Core *ETCore = NULL;

/*
 * Colors Used
 */
GdkRGBA RED = {1.0, 0.0, 0.0, 1.0 };

void
ET_Core_Create (void)
{
    /* Allocate. */
    if (ETCore == NULL)
    {
        ETCore = g_slice_new0 (ET_Core);
    }
}

void
ET_Core_Free (void)
{
    g_return_if_fail (ETCore != NULL);

    /* First frees lists. */
    if (ETCore->ETFileList)
        ET_Free_File_List();

    if (ETCore->ETFileDisplayedList)
        ET_Free_Displayed_File_List();

    if (ETCore->ETHistoryFileList)
        ET_Free_History_File_List();

    if (ETCore->ETArtistAlbumFileList)
        ET_Free_Artist_Album_File_List();

    if (ETCore)
    {
        g_slice_free (ET_Core, ETCore);
        ETCore = NULL;
    }
}
