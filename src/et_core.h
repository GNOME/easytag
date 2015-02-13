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

#ifndef ET_CORE_H_
#define ET_CORE_H_

#include <glib.h>

G_BEGIN_DECLS

#include <gdk/gdk.h>

#include "file.h"

/*
 * Colors Used (see declaration into et_core.c)
 */
extern GdkRGBA RED;

/*
 * Description of all variables, lists needed by EasyTAG
 */
typedef struct
{
    // The main list of files
    GList *ETFileList;                  // List of ALL FILES (ET_File) loaded in the directory and sub-directories (List of ET_File) (This list musn't be altered, and points always to the first item)

    // The list of files organized by artist then album
    GList *ETArtistAlbumFileList;

    // Displayed list (part of the main list of files displayed in BrowserList) (used when displaying by Artist & Album) 
    GList *ETFileDisplayedList;                 // List of files displayed (List of ET_File from ETFileList / ATArtistAlbumFileList) | !! May not point to the first item!!
    guint  ETFileDisplayedList_Length;          // Contains the length of the displayed list
    gfloat ETFileDisplayedList_TotalSize;       // Total of the size of files in displayed list (in bytes)
    gulong ETFileDisplayedList_TotalDuration;   // Total of duration of files in displayed list (in seconds)

    // Displayed item
    ET_File *ETFileDisplayed;           // Pointer to the current ETFile displayed in EasyTAG (may be NULL)


    // History list
    GList *ETHistoryFileList;           // History list of files changes for undo/redo actions
} ET_Core;

extern ET_Core *ETCore; /* Main pointer to structure needed by EasyTAG. */

void ET_Core_Create (void);
void ET_Core_Free (void);

G_END_DECLS

#endif /* ET_CORE_H_ */
