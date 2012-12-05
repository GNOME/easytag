/* config.h - 2000/06/21 */
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


#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <gtk/gtk.h>

/***************
 * Declaration *
 ***************/

typedef enum
{
    CV_TYPE_STRING=0,
    CV_TYPE_INT,
    CV_TYPE_BOOL
} Config_Variable_Type;


typedef struct _tConfigVariable tConfigVariable;
struct _tConfigVariable
{
    char *name;                 /* Variable name written in config file */
    Config_Variable_Type type;  /* Variable type: Integer, Alphabetic, ... */
    void *pointer;              /* Pointer to our variable */
};


/*
 * Config varariables
 */
/* Common */
gint    LOAD_ON_STARTUP;
gchar  *DEFAULT_PATH_TO_MP3;
gint    BROWSER_LINE_STYLE;
gint    BROWSER_EXPANDER_STYLE;
gint    BROWSE_SUBDIR;
gint    BROWSE_HIDDEN_DIR;
gint    OPEN_SELECTED_BROWSER_NODE;

/* Misc */
// User Interface
gint    SET_MAIN_WINDOW_POSITION;
gint    MAIN_WINDOW_X;
gint    MAIN_WINDOW_Y;
gint    MAIN_WINDOW_HEIGHT;
gint    MAIN_WINDOW_WIDTH;
gint    PANE_HANDLE_POSITION1;
gint    PANE_HANDLE_POSITION2;
gint    PANE_HANDLE_POSITION3;
gint    PANE_HANDLE_POSITION4;
gint    SHOW_HEADER_INFO;

gint    CHANGED_FILES_DISPLAYED_TO_RED;
gint    CHANGED_FILES_DISPLAYED_TO_BOLD;

gint    SORTING_FILE_MODE;
gint    SORTING_FILE_CASE_SENSITIVE;

gint    MESSAGE_BOX_POSITION_NONE;
gint    MESSAGE_BOX_POSITION_CENTER;
gint    MESSAGE_BOX_POSITION_MOUSE;
gint    MESSAGE_BOX_POSITION_CENTER_ON_PARENT;

gchar  *AUDIO_FILE_PLAYER;

/* File Settings */
gint    REPLACE_ILLEGAL_CHARACTERS_IN_FILENAME;
gint    FILENAME_EXTENSION_LOWER_CASE;
gint    FILENAME_EXTENSION_UPPER_CASE;
gint    FILENAME_EXTENSION_NO_CHANGE;
gint    PRESERVE_MODIFICATION_TIME;
gint    UPDATE_PARENT_DIRECTORY_MODIFICATION_TIME;

gint    FILENAME_CHARACTER_SET_OTHER;
gint    FILENAME_CHARACTER_SET_APPROXIMATE;
gint    FILENAME_CHARACTER_SET_DISCARD;

/* Tag Settings */
gint    WRITE_ID3_TAGS_IN_FLAC_FILE;
gint    STRIP_TAG_WHEN_EMPTY_FIELDS;
gint    CONVERT_OLD_ID3V2_TAG_VERSION;

gint    FILE_WRITING_ID3V2_VERSION_4;
gint    USE_NON_STANDARD_ID3_READING_CHARACTER_SET;
gchar  *FILE_READING_ID3V1V2_CHARACTER_SET;

gint    FILE_WRITING_ID3V2_WRITE_TAG;
gint    FILE_WRITING_ID3V2_USE_CRC32;
gint    FILE_WRITING_ID3V2_USE_COMPRESSION;
gint    FILE_WRITING_ID3V2_TEXT_ONLY_GENRE;
gint    FILE_WRITING_ID3V2_USE_UNICODE_CHARACTER_SET;
gchar  *FILE_WRITING_ID3V2_UNICODE_CHARACTER_SET;
gchar  *FILE_WRITING_ID3V2_NO_UNICODE_CHARACTER_SET;
gint    FILE_WRITING_ID3V2_ICONV_OPTIONS_NO;
gint    FILE_WRITING_ID3V2_ICONV_OPTIONS_TRANSLIT;
gint    FILE_WRITING_ID3V2_ICONV_OPTIONS_IGNORE;

gint    FILE_WRITING_ID3V1_WRITE_TAG;
gchar  *FILE_WRITING_ID3V1_CHARACTER_SET;
gint    FILE_WRITING_ID3V1_ICONV_OPTIONS_NO;
gint    FILE_WRITING_ID3V1_ICONV_OPTIONS_TRANSLIT;
gint    FILE_WRITING_ID3V1_ICONV_OPTIONS_IGNORE;

gint    VORBIS_SPLIT_FIELD_TITLE;
gint    VORBIS_SPLIT_FIELD_ARTIST;
gint    VORBIS_SPLIT_FIELD_ALBUM;
gint    VORBIS_SPLIT_FIELD_GENRE;
gint    VORBIS_SPLIT_FIELD_COMMENT;
gint    VORBIS_SPLIT_FIELD_COMPOSER;
gint    VORBIS_SPLIT_FIELD_ORIG_ARTIST;

gint    DATE_AUTO_COMPLETION;
gint    NUMBER_TRACK_FORMATED;
gint    NUMBER_TRACK_FORMATED_SPIN_BUTTON;
gint    OGG_TAG_WRITE_XMMS_COMMENT;
gint    SET_FOCUS_TO_SAME_TAG_FIELD;
gint    SET_FOCUS_TO_FIRST_TAG_FIELD;
gint    LOG_MAX_LINES;
gint    SHOW_LOG_VIEW;


/* Scanner */
gint    SCANNER_TYPE;
gint    SCAN_MASK_EDITOR_BUTTON;
gint    SCAN_LEGEND_BUTTON;
gint    FTS_CONVERT_UNDERSCORE_AND_P20_INTO_SPACE;
gint    FTS_CONVERT_SPACE_INTO_UNDERSCORE;
gint    RFS_CONVERT_UNDERSCORE_AND_P20_INTO_SPACE;
gint    RFS_CONVERT_SPACE_INTO_UNDERSCORE;
gint    RFS_REMOVE_SPACES;
gint    PFS_DONT_UPPER_SOME_WORDS;
gint    OVERWRITE_TAG_FIELD;
gint    SET_DEFAULT_COMMENT;
gchar  *DEFAULT_COMMENT;
gint    SET_CRC32_COMMENT;
gint    OPEN_SCANNER_WINDOW_ON_STARTUP;
gint    SCANNER_WINDOW_ON_TOP;
gint    SET_SCANNER_WINDOW_POSITION;
gint    SCANNER_WINDOW_X;
gint    SCANNER_WINDOW_Y;

/* Confirmation */
gint    CONFIRM_BEFORE_EXIT;
gint    CONFIRM_WRITE_TAG;
gint    CONFIRM_RENAME_FILE;
gint    CONFIRM_WRITE_PLAYLIST;
gint    CONFIRM_DELETE_FILE;
gint    CONFIRM_WHEN_UNSAVED_FILES;

/* Scanner window */
gint    PROCESS_FILENAME_FIELD;
gint    PROCESS_TITLE_FIELD;
gint    PROCESS_ARTIST_FIELD;
gint    PROCESS_ALBUM_ARTIST_FIELD;
gint    PROCESS_ALBUM_FIELD;
gint    PROCESS_GENRE_FIELD;
gint    PROCESS_COMMENT_FIELD;
gint    PROCESS_COMPOSER_FIELD;
gint    PROCESS_ORIG_ARTIST_FIELD;
gint    PROCESS_COPYRIGHT_FIELD;
gint    PROCESS_URL_FIELD;
gint    PROCESS_ENCODED_BY_FIELD;
gchar  *PROCESS_FIELDS_CONVERT_FROM;
gchar  *PROCESS_FIELDS_CONVERT_TO;
gint    PF_CONVERT_INTO_SPACE;
gint    PF_CONVERT_SPACE;
gint    PF_CONVERT;
gint    PF_CONVERT_ALL_UPPERCASE;
gint    PF_CONVERT_ALL_DOWNCASE;
gint    PF_CONVERT_FIRST_LETTER_UPPERCASE;
gint    PF_CONVERT_FIRST_LETTERS_UPPERCASE;
gint    PF_DETECT_ROMAN_NUMERALS;
gint    PF_REMOVE_SPACE;
gint    PF_INSERT_SPACE;
gint    PF_ONLY_ONE_SPACE;

/* Playlist window */
gchar  *PLAYLIST_NAME;
gint    PLAYLIST_USE_MASK_NAME;
gint    PLAYLIST_USE_DIR_NAME;
gint    PLAYLIST_ONLY_SELECTED_FILES;
gint    PLAYLIST_FULL_PATH;
gint    PLAYLIST_RELATIVE_PATH;
gint    PLAYLIST_CREATE_IN_PARENT_DIR;
gint    PLAYLIST_USE_DOS_SEPARATOR;
gint    PLAYLIST_CONTENT_NONE;
gint    PLAYLIST_CONTENT_FILENAME;
gint    PLAYLIST_CONTENT_MASK;
gchar  *PLAYLIST_CONTENT_MASK_VALUE;

gint    PLAYLIST_WINDOW_X;
gint    PLAYLIST_WINDOW_Y;
gint    PLAYLIST_WINDOW_WIDTH;
gint    PLAYLIST_WINDOW_HEIGHT;

/* "Load filenames from txt" window */
gint    LOAD_FILE_RUN_SCANNER;

gint    LOAD_FILE_WINDOW_X;
gint    LOAD_FILE_WINDOW_Y;
gint    LOAD_FILE_WINDOW_WIDTH;
gint    LOAD_FILE_WINDOW_HEIGHT;

/* CDDB in preferences window */
gchar  *CDDB_SERVER_NAME_AUTOMATIC_SEARCH;
gint    CDDB_SERVER_PORT_AUTOMATIC_SEARCH;
gchar  *CDDB_SERVER_CGI_PATH_AUTOMATIC_SEARCH;
gchar  *CDDB_SERVER_NAME_AUTOMATIC_SEARCH2;
gint    CDDB_SERVER_PORT_AUTOMATIC_SEARCH2;
gchar  *CDDB_SERVER_CGI_PATH_AUTOMATIC_SEARCH2;
gchar  *CDDB_SERVER_NAME_MANUAL_SEARCH;
gint    CDDB_SERVER_PORT_MANUAL_SEARCH;
gchar  *CDDB_SERVER_CGI_PATH_MANUAL_SEARCH;
gchar  *CDDB_LOCAL_PATH;
gint    CDDB_USE_PROXY;
gchar  *CDDB_PROXY_NAME;
gint    CDDB_PROXY_PORT;
gchar  *CDDB_PROXY_USER_NAME;
gchar  *CDDB_PROXY_USER_PASSWORD;

gint    SET_CDDB_WINDOW_POSITION;
gint    CDDB_WINDOW_X;
gint    CDDB_WINDOW_Y;
gint    CDDB_WINDOW_HEIGHT;
gint    CDDB_WINDOW_WIDTH;
gint    CDDB_PANE_HANDLE_POSITION;

gint    CDDB_FOLLOW_FILE;
gint    CDDB_USE_DLM;
gint    CDDB_USE_LOCAL_ACCESS;

/* CDDB window */
gint    CDDB_SEARCH_IN_ALL_FIELDS;
gint    CDDB_SEARCH_IN_ARTIST_FIELD;
gint    CDDB_SEARCH_IN_TITLE_FIELD;
gint    CDDB_SEARCH_IN_TRACK_NAME_FIELD;
gint    CDDB_SEARCH_IN_OTHER_FIELD;

gint    CDDB_SHOW_CATEGORIES;

gint    CDDB_SEARCH_IN_ALL_CATEGORIES;
gint    CDDB_SEARCH_IN_BLUES_CATEGORY;
gint    CDDB_SEARCH_IN_CLASSICAL_CATEGORY;
gint    CDDB_SEARCH_IN_COUNTRY_CATEGORY;
gint    CDDB_SEARCH_IN_FOLK_CATEGORY;
gint    CDDB_SEARCH_IN_JAZZ_CATEGORY;
gint    CDDB_SEARCH_IN_MISC_CATEGORY;
gint    CDDB_SEARCH_IN_NEWAGE_CATEGORY;
gint    CDDB_SEARCH_IN_REGGAE_CATEGORY;
gint    CDDB_SEARCH_IN_ROCK_CATEGORY;
gint    CDDB_SEARCH_IN_SOUNDTRACK_CATEGORY;

gint    CDDB_SET_TO_ALL_FIELDS;
gint    CDDB_SET_TO_TITLE;
gint    CDDB_SET_TO_ARTIST;
gint    CDDB_SET_TO_ALBUM;
gint    CDDB_SET_TO_YEAR;
gint    CDDB_SET_TO_TRACK;
gint    CDDB_SET_TO_TRACK_TOTAL;
gint    CDDB_SET_TO_GENRE;
gint    CDDB_SET_TO_FILE_NAME;

gint    CDDB_RUN_SCANNER;

/* Search Window */
gint    SET_SEARCH_WINDOW_POSITION;
gint    SEARCH_WINDOW_X;
gint    SEARCH_WINDOW_Y;
gint    SEARCH_WINDOW_HEIGHT;
gint    SEARCH_WINDOW_WIDTH;
gint    SEARCH_IN_FILENAME;
gint    SEARCH_IN_TAG;
gint    SEARCH_CASE_SENSITIVE;

/* Default mask */
gchar  *SCAN_TAG_DEFAULT_MASK;
gchar  *RENAME_FILE_DEFAULT_MASK;
gchar  *RENAME_DIRECTORY_DEFAULT_MASK;
gint    RENAME_DIRECTORY_WITH_MASK;


/* Other parameters */
gint    OPTIONS_NOTEBOOK_PAGE;
gint    OPTIONS_WINDOW_HEIGHT;
gint    OPTIONS_WINDOW_WIDTH;



/**************
 * Prototypes *
 **************/

void Init_Config_Variables (void);
void Read_Config           (void);

void Save_Changes_Of_Preferences_Window  (void);
void Save_Changes_Of_UI                  (void);

gboolean Setting_Create_Files     (void);


/* MasksList */
void Load_Scan_Tag_Masks_List (GtkListStore *liststore, gint colnum, gchar **fallback);
void Save_Scan_Tag_Masks_List (GtkListStore *liststore, gint colnum);

/* RenameFileMasksList */
void Load_Rename_File_Masks_List (GtkListStore *liststore, gint colnum, gchar **fallback);
void Save_Rename_File_Masks_List (GtkListStore *liststore, gint colnum);

/* RenameDirectoryMasksList 'RenameDirectoryMaskCombo' combobox */
void Load_Rename_Directory_Masks_List (GtkListStore *liststore, gint colnum, gchar **fallback);
void Save_Rename_Directory_Masks_List (GtkListStore *liststore, gint colnum);

/* 'DefaultPathToMp3' combobox */
void Load_Default_Path_To_MP3_List (GtkListStore *liststore, gint colnum);
void Save_Default_Path_To_MP3_List (GtkListStore *liststore, gint colnum);

/* 'DefaultComment' combobox */
void Load_Default_Tag_Comment_Text_List (GtkListStore *liststore, gint colnum);
void Save_Default_Tag_Comment_Text_List (GtkListStore *liststore, gint colnum);

/* 'BrowserEntry' combobox */
void Load_Path_Entry_List (GtkListStore *liststore, gint colnum);
void Save_Path_Entry_List (GtkListStore *liststore, gint colnum);

/* 'PlayListNameEntry' combobox */
void Load_Play_List_Name_List (GtkListStore *liststore, gint colnum);
void Save_Play_List_Name_List (GtkListStore *liststore, gint colnum);

/* Run Program combobox (tree browser) */
void Load_Run_Program_With_Directory_List (GtkListStore *liststore, gint colnum);
void Save_Run_Program_With_Directory_List (GtkListStore *liststore, gint colnum);

/* Run Program combobox (file browser) */
void Load_Run_Program_With_File_List (GtkListStore *liststore, gint colnum);
void Save_Run_Program_With_File_List (GtkListStore *liststore, gint colnum);

/* 'FilePlayerEntry' combobox */
void Load_Audio_File_Player_List (GtkListStore *liststore, gint colnum);
void Save_Audio_File_Player_List (GtkListStore *liststore, gint colnum);

/* 'SearchStringEntry' combobox */
void Load_Search_File_List (GtkListStore *liststore, gint colnum);
void Save_Search_File_List (GtkListStore *liststore, gint colnum);

/* 'FileToLoad' combobox */
void Load_File_To_Load_List (GtkListStore *liststore, gint colnum);
void Save_File_To_Load_List (GtkListStore *liststore, gint colnum);

/* 'PlayListContentMaskEntry' combobox */
void Load_Playlist_Content_Mask_List (GtkListStore *liststore, gint colnum);
void Save_Playlist_Content_Mask_List (GtkListStore *liststore, gint colnum);

/* 'CddbSearchStringEntry' combobox */
void Load_Cddb_Search_String_List (GtkListStore *liststore, gint colnum);
void Save_Cddb_Search_String_List (GtkListStore *liststore, gint colnum);

/* 'CddbSearchStringInResultEntry' combobox */
void Load_Cddb_Search_String_In_Result_List (GtkListStore *liststore, gint colnum);
void Save_Cddb_Search_String_In_Result_List (GtkListStore *liststore, gint colnum);

/* 'CddbLocalPath' combobox */
void Load_Cddb_Local_Path_List (GtkListStore *liststore, gint colnum);
void Save_Cddb_Local_Path_List (GtkListStore *liststore, gint colnum);



#endif /* __CONFIG_H__ */
