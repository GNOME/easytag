/* EasyTAG - tag editor for audio files
 * Copyright (C) 2013-2015  David King <amigadave@amigadave.com>
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

#ifndef ET_APPLICATION_WINDOW_H_
#define ET_APPLICATION_WINDOW_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#include "et_core.h"

#define ET_TYPE_APPLICATION_WINDOW (et_application_window_get_type ())
#define ET_APPLICATION_WINDOW(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), ET_TYPE_APPLICATION_WINDOW, EtApplicationWindow))

typedef struct _EtApplicationWindow EtApplicationWindow;
typedef struct _EtApplicationWindowClass EtApplicationWindowClass;

struct _EtApplicationWindow
{
    /*< private >*/
    GtkApplicationWindow parent_instance;
};

struct _EtApplicationWindowClass
{
    /*< private >*/
    GtkApplicationWindowClass parent_class;
};

GType et_application_window_get_type (void);
EtApplicationWindow *et_application_window_new (GtkApplication *application);
File_Tag * et_application_window_tag_area_create_file_tag (EtApplicationWindow *self);
void et_application_window_tag_area_clear (EtApplicationWindow *self);
void et_application_window_tag_area_set_sensitive (EtApplicationWindow *self, gboolean sensitive);
const gchar * et_application_window_file_area_get_filename (EtApplicationWindow *self);
void et_application_window_file_area_set_file_fields (EtApplicationWindow *self, const ET_File *ETFile);
void et_application_window_file_area_set_header_fields (EtApplicationWindow *self, EtFileHeaderFields *fields);
void et_application_window_file_area_clear (EtApplicationWindow *self);
void et_application_window_file_area_set_sensitive (EtApplicationWindow *self, gboolean sensitive);
void et_application_window_disable_command_actions (EtApplicationWindow *self);
void et_application_window_update_actions (EtApplicationWindow *self);
void et_application_window_set_busy_cursor (EtApplicationWindow *self);
void et_application_window_set_normal_cursor (EtApplicationWindow *self);
void et_application_window_tag_area_display_controls (EtApplicationWindow *self, const ET_File *ETFile);
GtkWidget * et_application_window_get_log_area (EtApplicationWindow *self);
void et_application_window_show_preferences_dialog_scanner (EtApplicationWindow *self);
void et_application_window_browser_toggle_display_mode (EtApplicationWindow *self);
void et_application_window_browser_set_sensitive (EtApplicationWindow *self, gboolean sensitive);
void et_application_window_browser_clear (EtApplicationWindow *self);
void et_application_window_browser_clear_album_model (EtApplicationWindow *self);
void et_application_window_browser_clear_artist_model (EtApplicationWindow *self);
void et_application_window_select_dir (EtApplicationWindow *self, GFile *file);
void et_application_window_select_file_by_et_file (EtApplicationWindow *self, ET_File *ETFile);
GFile * et_application_window_get_current_path (EtApplicationWindow *self);
GtkWidget * et_application_window_get_scan_dialog (EtApplicationWindow *self);
void et_application_window_apply_changes (EtApplicationWindow *self);
void et_application_window_browser_entry_set_text (EtApplicationWindow *self, const gchar *text);
void et_application_window_browser_label_set_text (EtApplicationWindow *self, const gchar *text);
ET_File * et_application_window_browser_get_et_file_from_path (EtApplicationWindow *self, GtkTreePath *path);
ET_File * et_application_window_browser_get_et_file_from_iter (EtApplicationWindow *self, GtkTreeIter *iter);
GList * et_application_window_browser_get_selected_files (EtApplicationWindow *self);
GtkTreeSelection * et_application_window_browser_get_selection (EtApplicationWindow *self);
GtkTreeViewColumn *et_application_window_browser_get_column_for_column_id (EtApplicationWindow *self, gint column_id);
GtkSortType et_application_window_browser_get_sort_order_for_column_id (EtApplicationWindow *self, gint column_id);
void et_application_window_browser_select_file_by_iter_string (EtApplicationWindow *self, const gchar *iter_string, gboolean select);
void et_application_window_update_et_file_from_ui (EtApplicationWindow *self);
void et_application_window_display_et_file (EtApplicationWindow *self, ET_File *ETFile);
void et_application_window_browser_select_file_by_et_file (EtApplicationWindow *self, const ET_File *file, gboolean select);
GtkTreePath * et_application_window_browser_select_file_by_et_file2 (EtApplicationWindow *self, const ET_File *file, gboolean select, GtkTreePath *start_path);
ET_File * et_application_window_browser_select_file_by_dlm (EtApplicationWindow *self, const gchar *string, gboolean select);
void et_application_window_browser_unselect_all (EtApplicationWindow *self);
void et_application_window_browser_refresh_list (EtApplicationWindow *self);
void et_application_window_browser_refresh_file_in_list (EtApplicationWindow *self, const ET_File *file);
void et_application_window_scan_dialog_update_previews (EtApplicationWindow *self);
void et_application_window_progress_set_fraction (EtApplicationWindow *self, gdouble fraction);
void et_application_window_progress_set_text (EtApplicationWindow *self, const gchar *text);
void et_application_window_status_bar_message (EtApplicationWindow *self, const gchar *message, gboolean with_timer);
void et_application_window_quit (EtApplicationWindow *self);

G_END_DECLS

#endif /* !ET_APPLICATION_WINDOW_H_ */
