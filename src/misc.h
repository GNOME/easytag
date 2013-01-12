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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __MISC_H__
#define __MISC_H__


#include <gtk/gtk.h>

/***************
 * Declaration *
 ***************/

GtkWidget *WritePlaylistWindow;
GtkWidget *playlist_use_mask_name;
GtkWidget *PlayListNameMaskCombo;
GtkWidget *playlist_use_dir_name;
GtkWidget *playlist_only_selected_files;
GtkWidget *playlist_full_path;
GtkWidget *playlist_relative_path;
GtkWidget *playlist_create_in_parent_dir;
GtkWidget *playlist_use_dos_separator;
GtkWidget *playlist_content_none;
GtkWidget *playlist_content_filename;
GtkWidget *playlist_content_mask;
GtkWidget *PlayListContentMaskCombo;
GtkListStore *PlayListNameMaskModel;
GtkListStore *PlayListContentMaskModel;


/**************
 * Prototypes *
 **************/

/*
 * Create Pixmaps, buttons...
 */
GtkWidget *Create_Pixmap_Icon_With_Event_Box (const gchar *pixmap_name);
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
void Entry_Changed_Disable_Object (GtkWidget *widget_to_disable, GtkEditable *source_widget);
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

gchar *Convert_Size     (gfloat size);
gchar *Convert_Size_1   (gfloat size);
gchar *Convert_Duration (gulong duration);

gulong Get_File_Size (const gchar *filename);

void Strip_String (gchar *string);
gint Combo_Alphabetic_Sort (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer data);

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


#endif /* __MISC_H__ */
