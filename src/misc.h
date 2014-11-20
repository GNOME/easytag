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

#ifndef ET_MISC_H_
#define ET_MISC_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * Combobox misc functions
 */
gboolean Add_String_To_Combo_List(GtkListStore *liststore, const gchar *string);
gchar   *Get_Active_Combo_Box_Item(GtkComboBox *combo);


/*
 * Other
 */
void Init_Character_Translation_Table (void);

// Mouse cursor
void Init_Mouse_Cursor    (void);
void Set_Busy_Cursor      (void);
void Set_Unbusy_Cursor    (void);


gchar *Convert_Duration (gulong duration);

goffset et_get_file_size (const gchar *filename);

gint Combo_Alphabetic_Sort (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer data);

gboolean et_run_audio_player (GList *files, GError **error);
gboolean et_run_program (const gchar *program_name, GList *args_list, GError **error);

void File_Selection_Window_For_File      (GtkWidget *entry);
void File_Selection_Window_For_Directory (GtkWidget *entry);

gchar * et_disc_number_to_string (const guint disc_number);
gchar * et_track_number_to_string (const guint track_number);

void et_on_child_exited (GPid pid, gint status, gpointer user_data);

G_END_DECLS

#endif /* ET_MISC_H_ */
