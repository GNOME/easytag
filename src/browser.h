/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014-2015  David King <amigadave@amigadave.com>
 * Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
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

#ifndef ET_BROWSER_H_
#define ET_BROWSER_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#include "et_core.h"

#define ET_TYPE_BROWSER (et_browser_get_type ())
#define ET_BROWSER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), ET_TYPE_BROWSER, EtBrowser))

typedef struct _EtBrowser EtBrowser;
typedef struct _EtBrowserClass EtBrowserClass;

struct _EtBrowser
{
    /*< private >*/
    GtkBin parent_instance;
};

struct _EtBrowserClass
{
    /*< private >*/
    GtkBinClass parent_class;
};

GType et_browser_get_type (void);
EtBrowser *et_browser_new (void);
void et_browser_show_open_directory_with_dialog (EtBrowser *self);
void et_browser_show_open_files_with_dialog (EtBrowser *self);
void et_browser_show_rename_directory_dialog (EtBrowser *self);

typedef enum
{
    ET_BROWSER_MODE_FILE,
    ET_BROWSER_MODE_ARTIST
} EtBrowserMode;

/*
 * To number columns of ComboBox
 */
enum
{
    MISC_COMBO_TEXT, // = 0 (First column)
    MISC_COMBO_COUNT // = 1 (Number of columns in ComboBox)
};

void et_browser_clear_album_model (EtBrowser *self);
void et_browser_clear_artist_model (EtBrowser *self);

void et_browser_select_dir (EtBrowser *self, GFile *file);
void et_browser_reload (EtBrowser *self);
void et_browser_collapse (EtBrowser *self);
void et_browser_set_sensitive (EtBrowser *self, gboolean sensitive);

void et_browser_load_file_list (EtBrowser *self, GList *etfilelist, const ET_File *etfile_to_select);
void et_browser_refresh_list (EtBrowser *self);
void et_browser_refresh_file_in_list (EtBrowser *self, const ET_File *ETFile);
void et_browser_clear (EtBrowser *self);
void et_browser_select_file_by_et_file (EtBrowser *self, const ET_File *ETFile, gboolean select_it);
GtkTreePath * et_browser_select_file_by_et_file2 (EtBrowser *self, const ET_File *searchETFile, gboolean select_it, GtkTreePath *startPath);
void et_browser_select_file_by_iter_string (EtBrowser *self, const gchar* stringiter, gboolean select_it);
ET_File *et_browser_select_file_by_dlm (EtBrowser *self, const gchar* string, gboolean select_it);
void et_browser_refresh_sort (EtBrowser *self);
void et_browser_select_all (EtBrowser *self);
void et_browser_unselect_all (EtBrowser *self);
void et_browser_invert_selection (EtBrowser *self);
void et_browser_remove_file (EtBrowser *self, const ET_File *ETFile);
ET_File * et_browser_get_et_file_from_path (EtBrowser *self, GtkTreePath *path);
ET_File * et_browser_get_et_file_from_iter (EtBrowser *self, GtkTreeIter *iter);

void et_browser_entry_set_text (EtBrowser *self, const gchar *text);
void et_browser_label_set_text (EtBrowser *self, const gchar *text);

void et_browser_set_display_mode (EtBrowser *self, EtBrowserMode mode);

void et_browser_go_home (EtBrowser *self);
void et_browser_go_desktop (EtBrowser *self);
void et_browser_go_documents (EtBrowser *self);
void et_browser_go_downloads (EtBrowser *self);
void et_browser_go_music (EtBrowser *self);
void et_browser_go_parent (EtBrowser *self);

void et_browser_run_player_for_album_list (EtBrowser *self);
void et_browser_run_player_for_artist_list (EtBrowser *self);
void et_browser_run_player_for_selection (EtBrowser *self);

void et_browser_load_default_dir (EtBrowser *self);
void et_browser_reload_directory (EtBrowser *self);
void et_browser_set_current_path_default (EtBrowser *self);
GFile * et_browser_get_current_path (EtBrowser *self);

GList * et_browser_get_selected_files (EtBrowser *self);
GtkTreeSelection * et_browser_get_selection (EtBrowser *self);

GtkTreeViewColumn * et_browser_get_column_for_column_id (EtBrowser *self, gint column_id);
GtkSortType et_browser_get_sort_order_for_column_id (EtBrowser *self, gint column_id);

G_END_DECLS

#endif /* ET_BROWSER_H_ */
