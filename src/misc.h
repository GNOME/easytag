/* misc.h - 2000/06/28 */
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __MISC_H__
#define __MISC_H__


#include <gtk/gtk.h>

/***************
 * Declaration *
 ***************/

GtkWidget *playlist_use_mask_name;
GtkWidget *playlist_use_dir_name;
GtkWidget *playlist_only_selected_files;
GtkWidget *playlist_full_path;
GtkWidget *playlist_relative_path;
GtkWidget *playlist_create_in_parent_dir;
GtkWidget *playlist_use_dos_separator;
GtkWidget *playlist_content_none;
GtkWidget *playlist_content_filename;
GtkWidget *playlist_content_mask;


/**************
 * Prototypes *
 **************/

/*
 * Create Pixmaps, buttons...
 */
GtkWidget *Create_Button_With_Icon_And_Label (const gchar *pixmap_name, gchar *label);
GtkWidget *Create_Xpm_Image                  (const char **xpm_name);


/*
 * Combobox misc functions
 */
gboolean Add_String_To_Combo_List(GtkListStore *liststore, const gchar *string);
gchar   *Get_Active_Combo_Box_Item(GtkComboBox *combo);


/*
 * Other
 */
void Insert_Only_Digit (GtkEditable *editable,const gchar *text,gint length,gint *position,gpointer data);
gboolean Parse_Date (void);
void Load_Genres_List_To_UI (void);
void Load_Track_List_To_UI  (void);
void Init_Character_Translation_Table (void);
void Init_Custom_Icons (void);
gchar *Check_If_Executable_Exists (const gchar *program);

// Mouse cursor
void Init_Mouse_Cursor    (void);
void Set_Busy_Cursor      (void);
void Set_Unbusy_Cursor    (void);

// Run Audio Player
void Run_Audio_Player_Using_Directory (void);
void Run_Audio_Player_Using_Selection (void);
void Run_Audio_Player_Using_Browser_Artist_List (void);
void Run_Audio_Player_Using_Browser_Album_List  (void);

gchar *Convert_Duration (gulong duration);

void et_show_help (void);

goffset et_get_file_size (const gchar *filename);

gint Combo_Alphabetic_Sort (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer data);

gboolean et_run_program (const gchar *program_name, GList *args_list);

void File_Selection_Window_For_File      (GtkWidget *entry);
void File_Selection_Window_For_Directory (GtkWidget *entry);

// Playlist window
void Open_Write_Playlist_Window          (void);
void Write_Playlist_Window_Apply_Changes (void);

// Search file window
void Open_Search_File_Window (void);
void Search_File_Window_Apply_Changes (void);

// Load filenames window
void Open_Load_Filename_Window          (void);
void Load_Filename_Window_Apply_Changes (void);

gchar * et_disc_number_to_string (const guint disc_number);
gchar * et_track_number_to_string (const guint track_number);

void et_on_child_exited (GPid pid, gint status, gpointer user_data);

#endif /* __MISC_H__ */
