/* browser.h - 2000/04/28 */
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


#ifndef __BROWSER_H__
#define __BROWSER_H__

#include "et_core.h"


/****************
 * Declarations *
 ****************/

/*
 * Data attached to each row of the artist list
 */
#if 0
typedef struct _ArtistRow ArtistRow;
struct _ArtistRow
{
    GList *AlbumList; // It's a list of AlbumList items...
};
#endif

/*
 * Data attached to each row of the artist list
 */
#if 0
typedef struct _AlbumRow AlbumRow;
struct _AlbumRow
{
    GList *ETFileList; // It's a list of ETFile items...
};
#endif

/*
 * To number columns of ComboBox
 */
enum
{
    MISC_COMBO_TEXT, // = 0 (First column)
    MISC_COMBO_COUNT // = 1 (Number of columns in ComboBox)
};


enum
{
    TREE_COLUMN_DIR_NAME,
    TREE_COLUMN_FULL_PATH,
    TREE_COLUMN_SCANNED,
    TREE_COLUMN_HAS_SUBDIR,
    TREE_COLUMN_PIXBUF,
    TREE_COLUMN_COUNT
};

enum
{
    LIST_FILE_NAME,
    LIST_FILE_POINTER,
    LIST_FILE_KEY,
    LIST_FILE_OTHERDIR, // To change color for other directories
    LIST_FONT_WEIGHT,
    LIST_ROW_BACKGROUND,
    LIST_ROW_FOREGROUND,
    // Tag fields
    LIST_FILE_TITLE,
    LIST_FILE_ARTIST,
    LIST_FILE_ALBUM_ARTIST,
    LIST_FILE_ALBUM,
    LIST_FILE_YEAR,
    LIST_FILE_DISCNO,
    LIST_FILE_TRACK,
    LIST_FILE_GENRE,
    LIST_FILE_COMMENT,
    LIST_FILE_COMPOSER,
    LIST_FILE_ORIG_ARTIST,
    LIST_FILE_COPYRIGHT,
    LIST_FILE_URL,
    LIST_FILE_ENCODED_BY,
    LIST_COLUMN_COUNT
};

enum
{
    ARTIST_PIXBUF,
    ARTIST_NAME,
    ARTIST_NUM_ALBUMS,
    ARTIST_NUM_FILES,
    ARTIST_ALBUM_LIST_POINTER,
    ARTIST_FONT_STYLE,
    ARTIST_FONT_WEIGHT,
    ARTIST_ROW_FOREGROUND,
    ARTIST_COLUMN_COUNT
};

enum
{
    ALBUM_PIXBUF,
    ALBUM_NAME,
    ALBUM_NUM_FILES,
    ALBUM_ETFILE_LIST_POINTER,
    ALBUM_FONT_STYLE,
    ALBUM_FONT_WEIGHT,
    ALBUM_ROW_FOREGROUND,
    ALBUM_COLUMN_COUNT
};


GtkWidget *BrowserList;
GtkWidget *BrowserAlbumList;
GtkWidget *BrowserArtistList;
GtkWidget *BrowserEntryCombo;
GtkListStore *BrowserEntryModel;
GtkWidget *BrowserHPaned;
GtkWidget *ArtistAlbumVPaned;

GtkWidget *RenameDirectoryWindow;
GtkWidget *RenameDirectoryMaskCombo;
GtkWidget *RenameDirectoryPreviewLabel;


/**************
 * Prototypes *
 **************/

GtkWidget   *Create_Browser_Items    (GtkWidget *parent);
gboolean     Browser_Tree_Select_Dir (const gchar *current_path);
void         Browser_Tree_Rebuild    (gchar *path_to_load);
void         Browser_Tree_Collapse   (void);

void         Browser_List_Load_File_List            (GList *etfilelist, ET_File *etfile_to_select);
void         Browser_List_Refresh_Whole_List        (void);
void         Browser_List_Refresh_File_In_List      (ET_File *ETFile);
void         Browser_List_Clear                     (void);
void         Browser_List_Select_File_By_Etfile     (ET_File *ETFile, gboolean select_it);
GtkTreePath *Browser_List_Select_File_By_Etfile2    (ET_File *searchETFile, gboolean select_it, GtkTreePath *startPath);
ET_File     *Browser_List_Select_File_By_Iter_String(const gchar* stringiter, gboolean select_it);
ET_File     *Browser_List_Select_File_By_DLM        (const gchar* string, gboolean select_it);
void         Browser_List_Refresh_Sort            (void);
void         Browser_List_Select_All_Files        (void);
void         Browser_List_Unselect_All_Files      (void);
void         Browser_List_Invert_File_Selection   (void);
void         Browser_List_Remove_File             (ET_File *ETFile);
ET_File     *Browser_List_Get_ETFile_From_Path    (GtkTreePath *path);
ET_File     *Browser_List_Get_ETFile_From_Iter    (GtkTreeIter *iter);

void         Browser_Entry_Set_Text      (gchar *text);
void         Browser_Label_Set_Text      (gchar *text);

void         Browser_Display_Tree_Or_Artist_Album_List (void);

void         Browser_Area_Set_Sensitive  (gboolean activate);

void         Browser_Load_Home_Directory            (void);
void		 Browser_Load_Desktop_Directory 		(void);
void		 Browser_Load_Documents_Directory 		(void);
void		 Browser_Load_Downloads_Directory 		(void);
void 		 Browser_Load_Music_Directory 			(void);

void         Browser_Load_Default_Directory         (void);
void         Browser_Reload_Directory               (void);
void         Set_Current_Path_As_Default            (void);
gchar       *Browser_Get_Current_Path               (void);

void         Browser_Open_Rename_Directory_Window (void);
void         Browser_Open_Run_Program_Tree_Window (void);
void         Browser_Open_Run_Program_List_Window (void);


#endif /* __BROWSER_H__ */
