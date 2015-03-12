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

#ifndef ET_FILE_LIST_H_
#define ET_FILE_LIST_H_

#include <glib.h>

G_BEGIN_DECLS

#include "file.h"
#include "file_tag.h"
#include "setting.h"

GList * et_file_list_add (GList *file_list, GFile *file);
void ET_Remove_File_From_File_List (ET_File *ETFile);
gboolean et_file_list_check_all_saved (GList *etfilelist);
void et_file_list_update_directory_name (GList *file_list, const gchar *old_path, const gchar *new_path);
guint et_file_list_get_n_files_in_path (GList *file_list, const gchar *path_utf8);
void et_file_list_free (GList *file_list);

GList * et_artist_album_list_new_from_file_list (GList *file_list);
void et_artist_album_file_list_free (GList *file_list);

GList * ET_Displayed_File_List_First (void);
GList * ET_Displayed_File_List_Previous (void);
GList * ET_Displayed_File_List_Next (void);
GList * ET_Displayed_File_List_Last (void);
GList * ET_Displayed_File_List_By_Etfile (const ET_File *ETFile);

void et_displayed_file_list_set (GList *ETFileList);
void et_displayed_file_list_free (GList *file_list);

GList * et_history_list_add (GList *history_list, ET_File *ETFile);
gboolean ET_Add_File_To_History_List (ET_File *ETFile);
ET_File * ET_Undo_History_File_Data (void);
ET_File * ET_Redo_History_File_Data (void);
gboolean et_history_list_has_undo (GList *history_list);
gboolean et_history_list_has_redo (GList *history_list);
void et_history_file_list_free (GList *file_list);

GList *ET_Sort_File_List (GList *ETFileList, EtSortMode Sorting_Type);

G_END_DECLS

#endif /* !ET_FILE_H_ */
