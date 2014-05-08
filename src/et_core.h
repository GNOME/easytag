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

#ifndef ET_CORE_H_
#define ET_CORE_H_

#include "config.h" /* For definition of ENABLE_OGG and so on. */

#include "setting.h"

#include <glib.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

/*
 * Colors Used (see declaration into et_core.c)
 */
extern GdkRGBA RED;

/*
 * Types of files
 */
typedef enum
{                    //                                             (.ext) is not so popular 
    MP2_FILE = 0,    // Mpeg audio Layer 2        : .mp2            (.mpg) (.mpga)
    MP3_FILE,        // Mpeg audio Layer 3        : .mp3            (.mpg) (.mpga)
    MP4_FILE, /* MPEG audio Layer 4 / AAC: .mp4 (.m4a) (.m4p) (.m4v) */
    OGG_FILE,        // Ogg Vorbis audio          : .ogg            (.ogm)
    FLAC_FILE,       // FLAC (lossless)           : .flac .fla
    MPC_FILE,        // MusePack                  : .mpc .mp+ .mpp
    MAC_FILE,        // Monkey's Audio (lossless) : .ape            (.mac)
    SPEEX_FILE,      // Speech audio files        : .spx
    OFR_FILE,        // OptimFROG (lossless)      : .ofr .ofs
    WAVPACK_FILE,    // Wavpack (lossless)        : .wv
    OPUS_FILE, /* Ogg Opus File: .opus */
    UNKNOWN_FILE
} ET_File_Type;


/*
 * Types of tags
 */
typedef enum
{
    ID3_TAG = 0,
    OGG_TAG,
    APE_TAG,
    FLAC_TAG,
    MP4_TAG,
    WAVPACK_TAG,
    OPUS_TAG,
    UNKNOWN_TAG
} ET_Tag_Type;


/*
 * Description of each item of the FileNameList list
 */
typedef struct _File_Name File_Name;
struct _File_Name
{
    guint key;
    gboolean saved;        /* Set to TRUE if this filename had been saved */
    gchar *value;          /* The filename containing the full path and the extension of the file */
    gchar *value_utf8;     /* Same than "value", but converted to UTF-8 to avoid multiple call to the convertion function */
    gchar *value_ck;       /* Collate key of "value_utf8" to speed up comparaison */
};


/*
 * Description of Picture item (see picture.h)
 */
typedef struct _Picture Picture;
struct _Picture
{
    gint     type;
    gchar   *description;
    gint     width;         /* Original width of the picture */
    gint     height;        /* Original height of the picture */
    gsize size; /* Picture size in bytes (like stat) */
    guchar  *data;
    Picture *next;
};


/*
 * Description of each item of the TagList list
 */
typedef struct _File_Tag File_Tag;
struct _File_Tag
{
    guint key;             /* Incremented value */
    gboolean saved;        /* Set to TRUE if this tag had been saved */

    gchar *title;          /* Title of track */
    gchar *artist;         /* Artist name */
    gchar *album_artist;   /* Album Artist */
    gchar *album;          /* Album name */
    gchar *disc_number;    /* Disc number */
    gchar *disc_total; /* The total number of discs of the album (ex: 1/2). */
    gchar *year;           /* Year of track */
    gchar *track;          /* Position of track in the album */
    gchar *track_total;    /* The number of tracks for the album (ex: 12/20) */
    gchar *genre;          /* Genre of song */
    gchar *comment;        /* Comment */
    gchar *composer;       /* Composer */
    gchar *orig_artist;    /* Original artist / Performer */
    gchar *copyright;      /* Copyright */
    gchar *url;            /* URL */
    gchar *encoded_by;     /* Encoded by */
    Picture *picture;      /* Picture */
    GList *other;          /* List of unsupported fields (used for ogg only) */
};


/*
 * Structure containing informations of the header of file
 * Nota: This struct was copied from an "MP3 structure", and will change later.
 */
typedef struct _ET_File_Info ET_File_Info;
struct _ET_File_Info
{
    gint version;               /* Version of bitstream (mpeg version for mp3, encoder version for ogg) */
    gint mpeg25;                /* Version is MPEG 2.5? */
    gint layer;                 /* "MP3 data" */
    gint bitrate;               /* Bitrate (kb/s) */
    gboolean variable_bitrate;  /* Is a VBR file? */
    gint samplerate;            /* Samplerate (Hz) */
    gint mode;                  /* Stereo, ... or channels for ogg */
    goffset size;               /* The size of file (in bytes) */
    gint duration;              /* The duration of file (in seconds) */
    gchar *mpc_profile;         /* MPC data */
    gchar *mpc_version;         /* MPC data : encoder version  (also for Speex) */
};


/*
 * Structure for descripting supported files
 */
typedef struct _ET_File_Description ET_File_Description;
struct _ET_File_Description
{
    ET_File_Type FileType;    /* Type of file (ex: MP3) */
    const gchar *Extension; /* Extension (ex: ".mp3") */
    ET_Tag_Type  TagType;     /* Type of tag (ex: ID3) */
};


/*
 * Description of supported files
 */
extern const ET_File_Description ETFileDescription[];

/* Calculate the last index of the previous tab */
extern const gsize ET_FILE_DESCRIPTION_SIZE;


/*
 * Description of each item of the ETFileList list
 */
typedef struct _ET_File ET_File;
struct _ET_File
{
    guint IndexKey;           /* Value used to display the position in the list (and in the BrowserList) - Must be renumered after resorting the list - This value varies when resorting list */

    guint ETFileKey;          /* Primary key to identify each item of the list (no longer used?) */

    time_t FileModificationTime;            /* Save modification time of the file */

    ET_File_Description *ETFileDescription;
    gchar               *ETFileExtension;   /* Real extension of the file (keeping the case) (should be placed in ETFileDescription?) */
    ET_File_Info        *ETFileInfo;        /* Header infos: bitrate, duration, ... */

    GList *FileNameCur;       /* Points to item of FileNameList that represents the current value of filename state (i.e. file on hard disk) */
    GList *FileNameNew;       /* Points to item of FileNameList that represents the new value of filename state */
    GList *FileNameList; /* Contains the history of changes about the filename. */
    GList *FileNameListBak;   /* Contains items of FileNameList removed by 'undo' procedure but have data currently saved (for example, when you save your last changes, make some 'undo', then make new changes) */
    
    GList *FileTag;           /* Points to the current item used of FileTagList */
    GList *FileTagList;       /* Contains the history of changes about file tag data */
    GList *FileTagListBak;    /* Contains items of FileTagList removed by 'undo' procedure but have data currently saved */
    
};



/*
 * Description of each item of the ETHistoryFileList list
 */
typedef struct _ET_History_File ET_History_File;
struct _ET_History_File
{
    ET_File *ETFile;           /* Pointer to item of ETFileList changed */
};



/*
 * Description of all variables, lists needed by EasyTAG
 */
typedef struct _ET_Core ET_Core;
struct _ET_Core
{
    // The main list of files
    GList *ETFileList;                  // List of ALL FILES (ET_File) loaded in the directory and sub-directories (List of ET_File) (This list musn't be altered, and points always to the first item)

    // The list of files organized by artist then album
    GList *ETArtistAlbumFileList;


    // Displayed list (part of the main list of files displayed in BrowserList) (used when displaying by Artist & Album) 
    GList *ETFileDisplayedList;                 // List of files displayed (List of ET_File from ETFileList / ATArtistAlbumFileList) | !! May not point to the first item!!
    GList *ETFileDisplayedListPtr;              // Pointer to the current item of ETFileDisplayedList used with ET_Displayed_File_List_First, ET_Displayed_File_List_Previous
    guint  ETFileDisplayedList_Length;          // Contains the length of the displayed list
    gfloat ETFileDisplayedList_TotalSize;       // Total of the size of files in displayed list (in bytes)
    gulong ETFileDisplayedList_TotalDuration;   // Total of duration of files in displayed list (in seconds)
    
    // Displayed item
    ET_File *ETFileDisplayed;           // Pointer to the current ETFile displayed in EasyTAG (may be NULL)


    // History list
    GList *ETHistoryFileList;           // History list of files changes for undo/redo actions
};

extern ET_Core *ETCore; /* Main pointer to structure needed by EasyTAG. */



/**************
 * Prototypes *
 **************/

void ET_Core_Create     (void);
void ET_Core_Initialize (void);
void ET_Core_Free       (void);
void ET_Core_Destroy    (void);

gboolean ET_File_Is_Supported (const gchar *filename);
GList   *ET_Add_File_To_File_List        (gchar *filename);
gboolean ET_Remove_File_From_File_List   (ET_File *ETFile);

gboolean ET_Create_Artist_Album_File_List      (void);
gboolean ET_Remove_File_From_File_List         (ET_File *ETFile);

gboolean ET_Check_If_File_Is_Saved       (ET_File *ETFile);
gboolean ET_Check_If_All_Files_Are_Saved (void);

ET_File      *ET_File_Item_New       (void);
File_Name    *ET_File_Name_Item_New  (void);
File_Tag     *ET_File_Tag_Item_New   (void);
gboolean      ET_Free_File_Tag_Item  (File_Tag *FileTag);
gboolean      ET_Free_File_List_Item (ET_File *ETFile);

gboolean ET_Copy_File_Tag_Item          (ET_File *ETFile, File_Tag *FileTag);
gboolean ET_Set_Field_File_Name_Item    (gchar **FileNameField, gchar *value);
gboolean ET_Set_Filename_File_Name_Item (File_Name *FileName, gchar *filename_utf8, gchar *filename);
gboolean ET_Set_Field_File_Tag_Item (gchar **FileTagField, const gchar *value);
gboolean ET_Set_Field_File_Tag_Picture  (Picture **FileTagField, Picture *pic);

GList   *ET_Displayed_File_List_First       (void);
GList   *ET_Displayed_File_List_Previous    (void);
GList   *ET_Displayed_File_List_Next        (void);
GList   *ET_Displayed_File_List_Last        (void);
GList   *ET_Displayed_File_List_By_Etfile   (ET_File *ETFile);

gboolean ET_Set_Displayed_File_List         (GList *ETFileList);

void     ET_Display_File_Data_To_UI (ET_File *ETFile);
void     ET_Save_File_Data_From_UI  (ET_File *ETFile);
gboolean ET_Save_File_Tag_To_HD (ET_File *ETFile, GError **error);

gboolean ET_Undo_File_Data          (ET_File *ETFile);
gboolean ET_Redo_File_Data          (ET_File *ETFile);
gboolean ET_File_Data_Has_Undo_Data (ET_File *ETFile);
gboolean ET_File_Data_Has_Redo_Data (ET_File *ETFile);

ET_File *ET_Undo_History_File_Data          (void);
ET_File *ET_Redo_History_File_Data          (void);
gboolean ET_History_File_List_Has_Undo_Data (void);
gboolean ET_History_File_List_Has_Redo_Data (void);

gboolean ET_Manage_Changes_Of_File_Data          (ET_File *ETFile, File_Name *FileName, File_Tag *FileTag);
void     ET_Mark_File_Name_As_Saved              (ET_File *ETFile);
void     ET_Update_Directory_Name_Into_File_List (gchar* last_path, gchar *new_path);
gboolean ET_File_Name_Convert_Character          (gchar *filename_utf8);
gchar   *ET_File_Name_Generate                   (ET_File *ETFile, gchar *new_file_name);
guint ET_Get_Number_Of_Files_In_Directory (const gchar *path_utf8);

gboolean ET_Detect_Changes_Of_File_Tag          (File_Tag  *FileTag1,  File_Tag  *FileTag2);

GList *ET_Sort_File_List (GList *ETFileList, EtSortMode Sorting_Type);
void ET_Sort_Displayed_File_List_And_Update_UI (EtSortMode Sorting_Type);
gint ET_Comp_Func_Sort_File_By_Ascending_Filename         (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Filename        (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Creation_Date    (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Creation_Date   (ET_File *ETFile1, ET_File *ETFile2);
gint et_comp_func_sort_file_by_ascending_disc_number (ET_File *ETFile1,
                                                      ET_File *ETFile2);
gint et_comp_func_sort_file_by_descending_disc_number (ET_File *ETFile1,
                                                       ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Track_Number     (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Track_Number    (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Title            (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Title           (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Artist           (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Artist          (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Album_Artist     (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Album_Artist    (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Album            (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Album           (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Year             (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Year            (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Genre            (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Genre           (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Comment          (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Comment         (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Composer         (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Composer        (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Orig_Artist      (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Orig_Artist     (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Copyright        (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Copyright       (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Url              (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Url             (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_Encoded_By       (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_Encoded_By      (ET_File *ETFile1, ET_File *ETFile2);

gint ET_Comp_Func_Sort_File_By_Ascending_File_Type        (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_File_Type       (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_File_Size        (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_File_Size       (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_File_Duration    (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_File_Duration   (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_File_Bitrate     (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_File_Bitrate    (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Ascending_File_Samplerate  (ET_File *ETFile1, ET_File *ETFile2);
gint ET_Comp_Func_Sort_File_By_Descending_File_Samplerate (ET_File *ETFile1, ET_File *ETFile2);

G_END_DECLS

#endif /* ET_CORE_H_ */
