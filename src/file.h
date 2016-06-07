/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014,2015  David King <amigadave@amigadave.com>
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

#ifndef ET_FILE_H_
#define ET_FILE_H_

#include <glib.h>

G_BEGIN_DECLS

#include "core_types.h"
#include "file_description.h"
#include "file_info.h"
#include "file_name.h"
#include "file_tag.h"

/*
 * Description of each item of the ETFileList list
 */
typedef struct
{
    guint IndexKey;           /* Value used to display the position in the list (and in the BrowserList) - Must be renumered after resorting the list - This value varies when resorting list */

    guint ETFileKey;          /* Primary key to identify each item of the list (no longer used?) */

    guint64 FileModificationTime; /* Save modification time of the file */

    const ET_File_Description *ETFileDescription;
    gchar               *ETFileExtension;   /* Real extension of the file (keeping the case) (should be placed in ETFileDescription?) */
    ET_File_Info        *ETFileInfo;        /* Header infos: bitrate, duration, ... */

    GList *FileNameCur;       /* Points to item of FileNameList that represents the current value of filename state (i.e. file on hard disk) */
    GList *FileNameNew;       /* Points to item of FileNameList that represents the new value of filename state */
    GList *FileNameList; /* Contains the history of changes about the filename. */
    GList *FileNameListBak;   /* Contains items of FileNameList removed by 'undo' procedure but have data currently saved (for example, when you save your last changes, make some 'undo', then make new changes) */

    GList *FileTag;           /* Points to the current item used of FileTagList */
    GList *FileTagList;       /* Contains the history of changes about file tag data */
    GList *FileTagListBak;    /* Contains items of FileTagList removed by 'undo' procedure but have data currently saved */
} ET_File;

/*
 * Description of each item of the ETHistoryFileList list
 */
typedef struct
{
    ET_File *ETFile;           /* Pointer to item of ETFileList changed */
} ET_History_File;

gboolean et_file_check_saved (const ET_File *ETFile);

ET_File * ET_File_Item_New (void);
void ET_Free_File_List_Item (ET_File *ETFile);

void ET_Save_File_Data_From_UI (ET_File *ETFile);
gboolean ET_Save_File_Name_Internal (const ET_File *ETFile, File_Name *FileName);
gboolean ET_Save_File_Tag_To_HD (ET_File *ETFile, GError **error);
gboolean ET_Save_File_Tag_Internal (ET_File *ETFile, File_Tag *FileTag);

gboolean ET_Undo_File_Data (ET_File *ETFile);
gboolean ET_Redo_File_Data (ET_File *ETFile);
gboolean ET_File_Data_Has_Undo_Data (const ET_File *ETFile);
gboolean ET_File_Data_Has_Redo_Data (const ET_File *ETFile);

gboolean ET_Manage_Changes_Of_File_Data (ET_File *ETFile, File_Name *FileName, File_Tag *FileTag);
void ET_Mark_File_Name_As_Saved (ET_File *ETFile);
gchar *et_file_generate_name (const ET_File *ETFile, const gchar *new_file_name);
gchar * ET_File_Format_File_Extension (const ET_File *ETFile);

gint ET_Comp_Func_Sort_File_By_Ascending_Filename (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Filename (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Creation_Date (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Creation_Date (const ET_File *ETFile1, const ET_File *ETFile2);
gint et_comp_func_sort_file_by_ascending_disc_number (const ET_File *ETFile1,
                                                      const ET_File *ETFile2);
gint et_comp_func_sort_file_by_descending_disc_number (const ET_File *ETFile1,
                                                       const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Track_Number (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Track_Number (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Title (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Title (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Artist (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Artist (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Album_Artist (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Album_Artist (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Album (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Album (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Year (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Year (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Genre (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Genre (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Comment (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Comment (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Composer (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Composer (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Orig_Artist (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Orig_Artist (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Copyright (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Copyright (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Url (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Url (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Encoded_By (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Encoded_By (const ET_File *ETFile1, const ET_File *ETFile2);

gint ET_Comp_Func_Sort_File_By_Ascending_File_Type (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_File_Type (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_File_Size (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_File_Size (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_File_Duration (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_File_Duration (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_File_Bitrate (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_File_Bitrate (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_File_Samplerate (const ET_File *ETFile1, const ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_File_Samplerate (const ET_File *ETFile1, const ET_File *ETFile2);

G_END_DECLS

#endif /* !ET_FILE_H_ */
