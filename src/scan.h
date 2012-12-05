/* scan.h - 2000/06/16 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifndef __SCAN_H__
#define __SCAN_H__


#include "et_core.h"

/****************
 * Declarations *
 ****************/
GtkWidget *ScannerWindow;
GtkWidget *SWScanButton;    // To enable/disable it in easytag.c


enum
{
    SCANNER_FILL_TAG = 0,
    SCANNER_RENAME_FILE,
    SCANNER_PROCESS_FIELDS
}; // Add a new item : Min and Max values used in Open_ScannerWindow

enum {
    MASK_EDITOR_TEXT,
    MASK_EDITOR_COUNT
};



/**************
 * Prototypes *
 **************/

void   Scan_Select_Mode_And_Run_Scanner     (ET_File *ETFile);
gchar *Scan_Generate_New_Filename_From_Mask       (ET_File *ETFile, gchar *mask, gboolean no_dir_check_or_conversion);
gchar *Scan_Generate_New_Directory_Name_From_Mask (ET_File *ETFile, gchar *mask, gboolean no_dir_check_or_conversion);
void   Scan_Rename_File_Generate_Preview      (void);
void   Scan_Fill_Tag_Generate_Preview         (void);
void   Scan_Rename_Directory_Generate_Preview (void);

void   Scan_Use_Fill_Tag_Scanner            (void);
void   Scan_Use_Rename_File_Scanner         (void);
void   Scan_Use_Process_Fields_Scanner      (void);

gboolean Scan_Check_Rename_File_Mask (GtkWidget *widget_to_show_hide, GtkEntry *widget_source);

void Scan_Process_Fields_All_Uppercase           (gchar *string);
void Scan_Process_Fields_All_Downcase            (gchar *string);
void Scan_Process_Fields_Letter_Uppercase        (gchar *string);
void Scan_Process_Fields_First_Letters_Uppercase (gchar *string);
void Scan_Process_Fields_Remove_Space            (gchar *string);
void Scan_Process_Fields_Insert_Space            (gchar **string);
void Scan_Process_Fields_Keep_One_Space          (gchar *string);

void Scan_Convert_Underscore_Into_Space (gchar *string);
void Scan_Convert_P20_Into_Space        (gchar *string);
void Scan_Convert_Space_Into_Undescore  (gchar *string);
void Scan_Remove_Spaces                 (gchar *string);

void Init_ScannerWindow (void);
void Open_ScannerWindow (gint scanner_type);
void ScannerWindow_Apply_Changes (void);

#endif /* __SCAN_H__ */
