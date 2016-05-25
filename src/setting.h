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


#ifndef ET_SETTINGS_H_
#define ET_SETTINGS_H_

#include <gtk/gtk.h>

/* Categories to search in CDDB manual search. */
typedef enum
{
    ET_CDDB_SEARCH_CATEGORY_BLUES = 1 << 0,
    ET_CDDB_SEARCH_CATEGORY_CLASSICAL = 1 << 1,
    ET_CDDB_SEARCH_CATEGORY_COUNTRY = 1 << 2,
    ET_CDDB_SEARCH_CATEGORY_FOLK = 1 << 3,
    ET_CDDB_SEARCH_CATEGORY_JAZZ = 1 << 4,
    ET_CDDB_SEARCH_CATEGORY_MISC = 1 << 5,
    ET_CDDB_SEARCH_CATEGORY_NEWAGE = 1 << 6,
    ET_CDDB_SEARCH_CATEGORY_REGGAE = 1 << 7,
    ET_CDDB_SEARCH_CATEGORY_ROCK = 1 << 8,
    ET_CDDB_SEARCH_CATEGORY_SOUNDTRACK = 1 << 9
} EtCddbSearchCategory;

/* Fields to use in CDDB manual search. */
typedef enum
{
    ET_CDDB_SEARCH_FIELD_ARTIST = 1 << 0,
    ET_CDDB_SEARCH_FIELD_TITLE = 1 << 1,
    ET_CDDB_SEARCH_FIELD_TRACK = 1 << 2,
    ET_CDDB_SEARCH_FIELD_OTHER = 1 << 3
} EtCddbSearchField;

/* Fields to set from CDDB search results. */
typedef enum
{
    ET_CDDB_SET_FIELD_TITLE = 1 << 0,
    ET_CDDB_SET_FIELD_ARTIST = 1 << 1,
    ET_CDDB_SET_FIELD_ALBUM = 1 << 2,
    ET_CDDB_SET_FIELD_YEAR = 1 << 3,
    ET_CDDB_SET_FIELD_TRACK = 1 << 4,
    ET_CDDB_SET_FIELD_TRACK_TOTAL = 1 << 5,
    ET_CDDB_SET_FIELD_GENRE = 1 << 6,
    ET_CDDB_SET_FIELD_FILENAME = 1 << 7
} EtCddbSetField;

/* Character set for passing to iconv. */
typedef enum
{
    ET_CHARSET_IBM864,
    ET_CHARSET_ISO_8859_6,
    ET_CHARSET_WINDOWS_1256,
    ET_CHARSET_ISO_8859_13,
    ET_CHARSET_ISO_8859_4,
    ET_CHARSET_WINDOWS_1257,
    ET_CHARSET_ISO_8859_14,
    ET_CHARSET_IBM852,
    ET_CHARSET_ISO_8859_2,
    ET_CHARSET_WINDOWS_1250,
    ET_CHARSET_GB18030,
    ET_CHARSET_GB2312,
    ET_CHARSET_BIG5,
    ET_CHARSET_BIG5_HKSCS,
    ET_CHARSET_IBM855,
    ET_CHARSET_ISO_8859_5,
    ET_CHARSET_ISO_IR_111,
    ET_CHARSET_ISO_KOI8_R,
    ET_CHARSET_WINDOWS_1251,
    ET_CHARSET_IBM866,
    ET_CHARSET_KOI8_U,
    ET_CHARSET_US_ASCII,
    ET_CHARSET_ISO_8859_7,
    ET_CHARSET_WINDOWS_1253,
    ET_CHARSET_IBM862,
    ET_CHARSET_WINDOWS_1255,
    ET_CHARSET_EUC_JP,
    ET_CHARSET_ISO_2022_JP,
    ET_CHARSET_SHIFT_JIS,
    ET_CHARSET_EUC_KR,
    ET_CHARSET_ISO_8859_10,
    ET_CHARSET_ISO_8859_3,
    ET_CHARSET_TIS_620,
    ET_CHARSET_IBM857,
    ET_CHARSET_ISO_8859_9,
    ET_CHARSET_WINDOWS_1254,
    ET_CHARSET_UTF_8,
    ET_CHARSET_VISCII,
    ET_CHARSET_WINDOWS_1258,
    ET_CHARSET_ISO_8859_8,
    ET_CHARSET_IBM850,
    ET_CHARSET_ISO_8859_1,
    ET_CHARSET_ISO_8859_15,
    ET_CHARSET_WINDOWS_1252
} EtCharset;

/* Method for processing spaces when updating tags. */
typedef enum
{
    ET_CONVERT_SPACES_SPACES,
    ET_CONVERT_SPACES_UNDERSCORES,
    ET_CONVERT_SPACES_REMOVE,
    ET_CONVERT_SPACES_NO_CHANGE
} EtConvertSpaces;

typedef enum
{
    ET_FILENAME_EXTENSION_LOWER_CASE,
    ET_FILENAME_EXTENSION_UPPER_CASE,
    ET_FILENAME_EXTENSION_NO_CHANGE
} EtFilenameExtensionMode;

/* Scanner dialog process fields capitalization options. */
typedef enum
{
    ET_PROCESS_CAPITALIZE_ALL_UP,
    ET_PROCESS_CAPITALIZE_ALL_DOWN,
    ET_PROCESS_CAPITALIZE_FIRST_LETTER_UP,
    ET_PROCESS_CAPITALIZE_FIRST_WORDS_UP,
    ET_PROCESS_CAPITALIZE_NO_CHANGE
} EtProcessCapitalize;

/* Tag fields to process in the scanner. */
typedef enum
{
    ET_PROCESS_FIELD_FILENAME = 1 << 0,
    ET_PROCESS_FIELD_TITLE = 1 << 1,
    ET_PROCESS_FIELD_ARTIST = 1 << 2,
    ET_PROCESS_FIELD_ALBUM_ARTIST = 1 << 3,
    ET_PROCESS_FIELD_ALBUM = 1 << 4,
    ET_PROCESS_FIELD_GENRE = 1 << 5,
    ET_PROCESS_FIELD_COMMENT = 1 << 6,
    ET_PROCESS_FIELD_COMPOSER = 1 << 7,
    ET_PROCESS_FIELD_ORIGINAL_ARTIST = 1 << 8,
    ET_PROCESS_FIELD_COPYRIGHT = 1 << 9,
    ET_PROCESS_FIELD_URL = 1 << 10,
    ET_PROCESS_FIELD_ENCODED_BY = 1 << 11
} EtProcessField;

typedef enum
{
    ET_PROCESS_FIELDS_CONVERT_SPACES,
    ET_PROCESS_FIELDS_CONVERT_UNDERSCORES,
    ET_PROCESS_FIELDS_CONVERT_CHARACTERS,
    ET_PROCESS_FIELDS_CONVERT_NO_CHANGE
} EtProcessFieldsConvert;

/* Content of generated playlists. */
typedef enum
{
    ET_PLAYLIST_CONTENT_FILENAMES,
    ET_PLAYLIST_CONTENT_EXTENDED,
    ET_PLAYLIST_CONTENT_EXTENDED_MASK
} EtPlaylistContent;

/* Encoding options when renaming files. */
typedef enum
{
    ET_RENAME_ENCODING_TRY_ALTERNATIVE,
    ET_RENAME_ENCODING_TRANSLITERATE,
    ET_RENAME_ENCODING_IGNORE
} EtRenameEncoding;

/*
 * The mode for the scanner window.
 */
typedef enum
{
    ET_SCAN_MODE_FILL_TAG,
    ET_SCAN_MODE_RENAME_FILE,
    ET_SCAN_MODE_PROCESS_FIELDS
} EtScanMode;

/* Types of sorting. See the GSettings key "sort-mode". */
typedef enum
{
    ET_SORT_MODE_ASCENDING_FILENAME,
    ET_SORT_MODE_DESCENDING_FILENAME,
    ET_SORT_MODE_ASCENDING_TITLE,
    ET_SORT_MODE_DESCENDING_TITLE,
    ET_SORT_MODE_ASCENDING_ARTIST,
    ET_SORT_MODE_DESCENDING_ARTIST,
    ET_SORT_MODE_ASCENDING_ALBUM_ARTIST,
    ET_SORT_MODE_DESCENDING_ALBUM_ARTIST,
    ET_SORT_MODE_ASCENDING_ALBUM,
    ET_SORT_MODE_DESCENDING_ALBUM,
    ET_SORT_MODE_ASCENDING_YEAR,
    ET_SORT_MODE_DESCENDING_YEAR,
    ET_SORT_MODE_ASCENDING_DISC_NUMBER,
    ET_SORT_MODE_DESCENDING_DISC_NUMBER,
    ET_SORT_MODE_ASCENDING_TRACK_NUMBER,
    ET_SORT_MODE_DESCENDING_TRACK_NUMBER,
    ET_SORT_MODE_ASCENDING_GENRE,
    ET_SORT_MODE_DESCENDING_GENRE,
    ET_SORT_MODE_ASCENDING_COMMENT,
    ET_SORT_MODE_DESCENDING_COMMENT,
    ET_SORT_MODE_ASCENDING_COMPOSER,
    ET_SORT_MODE_DESCENDING_COMPOSER,
    ET_SORT_MODE_ASCENDING_ORIG_ARTIST,
    ET_SORT_MODE_DESCENDING_ORIG_ARTIST,
    ET_SORT_MODE_ASCENDING_COPYRIGHT,
    ET_SORT_MODE_DESCENDING_COPYRIGHT,
    ET_SORT_MODE_ASCENDING_URL,
    ET_SORT_MODE_DESCENDING_URL,
    ET_SORT_MODE_ASCENDING_ENCODED_BY,
    ET_SORT_MODE_DESCENDING_ENCODED_BY,
    ET_SORT_MODE_ASCENDING_CREATION_DATE,
    ET_SORT_MODE_DESCENDING_CREATION_DATE,
    ET_SORT_MODE_ASCENDING_FILE_TYPE,
    ET_SORT_MODE_DESCENDING_FILE_TYPE,
    ET_SORT_MODE_ASCENDING_FILE_SIZE,
    ET_SORT_MODE_DESCENDING_FILE_SIZE,
    ET_SORT_MODE_ASCENDING_FILE_DURATION,
    ET_SORT_MODE_DESCENDING_FILE_DURATION,
    ET_SORT_MODE_ASCENDING_FILE_BITRATE,
    ET_SORT_MODE_DESCENDING_FILE_BITRATE,
    ET_SORT_MODE_ASCENDING_FILE_SAMPLERATE,
    ET_SORT_MODE_DESCENDING_FILE_SAMPLERATE
} EtSortMode;

/* Additional options to be passed to iconv(). */
typedef enum
{
    ET_TAG_ENCODING_NONE,
    ET_TAG_ENCODING_TRANSLITERATE,
    ET_TAG_ENCODING_IGNORE
} EtTagEncoding;

/*
 * Config variables
 */

extern GSettings *MainSettings;

void Init_Config_Variables (void);

gboolean Setting_Create_Files     (void);


/* MasksList */
void Load_Scan_Tag_Masks_List (GtkListStore *liststore, gint colnum,
                               const gchar * const *fallback);
void Save_Scan_Tag_Masks_List (GtkListStore *liststore, gint colnum);

/* RenameFileMasksList */
void Load_Rename_File_Masks_List (GtkListStore *liststore, gint colnum,
                                  const gchar * const *fallback);
void Save_Rename_File_Masks_List (GtkListStore *liststore, gint colnum);

/* 'BrowserEntry' combobox */
void Load_Path_Entry_List (GtkListStore *liststore, gint colnum);
void Save_Path_Entry_List (GtkListStore *liststore, gint colnum);

/* Run Program combobox (tree browser) */
void Load_Run_Program_With_Directory_List (GtkListStore *liststore, gint colnum);
void Save_Run_Program_With_Directory_List (GtkListStore *liststore, gint colnum);

/* Run Program combobox (file browser) */
void Load_Run_Program_With_File_List (GtkListStore *liststore, gint colnum);
void Save_Run_Program_With_File_List (GtkListStore *liststore, gint colnum);

/* 'SearchStringEntry' combobox */
void Load_Search_File_List (GtkListStore *liststore, gint colnum);
void Save_Search_File_List (GtkListStore *liststore, gint colnum);

gboolean et_settings_enum_get (GValue *value, GVariant *variant,
                               gpointer user_data);
GVariant *et_settings_enum_set (const GValue *value,
                                const GVariantType *expected_type,
                                gpointer user_data);
gboolean et_settings_enum_radio_get (GValue *value, GVariant *variant,
                                     gpointer user_data);
GVariant *et_settings_enum_radio_set (const GValue *value,
                                      const GVariantType *expected_type,
                                      gpointer user_data);
gboolean et_settings_flags_toggle_get (GValue *value, GVariant *variant,
                                       gpointer user_data);
GVariant *et_settings_flags_toggle_set (const GValue *value,
                                        const GVariantType *expected_type,
                                        gpointer user_data);


#endif /* ET_SETTINGS_H_ */
