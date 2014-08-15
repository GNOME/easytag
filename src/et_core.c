/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2000-2003  David King <amigadave@amigadave.com>
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

#include <config.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#include <ctype.h>
#include <locale.h>
#include "errno.h"

#include "application_window.h"
#include "easytag.h"
#include "et_core.h"
#include "mpeg_header.h"
#include "monkeyaudio_header.h"
#include "musepack_header.h"
#ifdef ENABLE_MP3
#   include "id3_tag.h"
#endif
#include "picture.h"
#include "ape_tag.h"
#ifdef ENABLE_OGG
#   include "ogg_header.h"
#   include "ogg_tag.h"
#endif
#ifdef ENABLE_FLAC
#   include "flac_header.h"
#   include "flac_tag.h"
#endif
#ifdef ENABLE_MP4
#   include "mp4_header.h"
#   include "mp4_tag.h"
#endif
#ifdef ENABLE_WAVPACK
#   include "wavpack_header.h"
#   include "wavpack_tag.h"
#endif
#ifdef ENABLE_OPUS
#include "opus_tag.h"
#include "opus_header.h"
#endif
#include "browser.h"
#include "log.h"
#include "misc.h"
#include "setting.h"
#include "charset.h"

#include "win32/win32dep.h"


/***************
 * Declaration *
 ***************/

ET_Core *ETCore = NULL;

const ET_File_Description ETFileDescription[] =
{
#ifdef ENABLE_MP3
    { MP3_FILE, ".mp3", ID3_TAG},
    { MP2_FILE, ".mp2", ID3_TAG},
#endif
#ifdef ENABLE_OPUS
    { OPUS_FILE, ".opus", OPUS_TAG},
#endif
#ifdef ENABLE_OGG
    { OGG_FILE, ".ogg", OGG_TAG},
    { OGG_FILE, ".oga", OGG_TAG},
#endif
#ifdef ENABLE_SPEEX
    { SPEEX_FILE, ".spx", OGG_TAG}, /* Implemented by Pierre Dumuid. */
#endif
#ifdef ENABLE_FLAC
    { FLAC_FILE, ".flac", FLAC_TAG},
    { FLAC_FILE, ".fla",  FLAC_TAG},
#endif
    { MPC_FILE, ".mpc", APE_TAG}, /* Implemented by Artur Polaczynski. */
    { MPC_FILE, ".mp+", APE_TAG}, /* Implemented by Artur Polaczynski. */
    { MPC_FILE, ".mpp", APE_TAG}, /* Implemented by Artur Polaczynski. */
    { MAC_FILE, ".ape", APE_TAG}, /* Implemented by Artur Polaczynski. */
    { MAC_FILE, ".mac", APE_TAG}, /* Implemented by Artur Polaczynski. */
    { OFR_FILE, ".ofr", APE_TAG},
    { OFR_FILE, ".ofs", APE_TAG},
#ifdef ENABLE_MP4
    { MP4_FILE, ".mp4", MP4_TAG}, /* Implemented by Michael Ihde. */
    { MP4_FILE, ".m4a", MP4_TAG}, /* Implemented by Michael Ihde. */
    { MP4_FILE, ".m4p", MP4_TAG}, /* Implemented by Michael Ihde. */
    { MP4_FILE, ".m4v", MP4_TAG},
#endif
#ifdef ENABLE_WAVPACK
    { WAVPACK_FILE, ".wv", WAVPACK_TAG}, /* Implemented by Maarten Maathuis. */
#endif
    { UNKNOWN_FILE, "", UNKNOWN_TAG } /* This item must be placed at the end! */
};

const gsize ET_FILE_DESCRIPTION_SIZE = G_N_ELEMENTS (ETFileDescription) - 1;

/*
 * Colors Used
 */
GdkRGBA RED = {1.0, 0.0, 0.0, 1.0 };


/**************
 * Prototypes *
 **************/

//gboolean ET_File_Is_Supported (gchar *filename);
static gchar *ET_Get_File_Extension (const gchar *filename);
static const ET_File_Description *ET_Get_File_Description (const gchar *filename);
static const ET_File_Description *ET_Get_File_Description_From_Extension (const gchar *extension);

static gboolean ET_Free_File_List                 (void);
static gboolean ET_Free_File_Name_List            (GList *FileNameList);
static gboolean ET_Free_File_Tag_List (GList *FileTagList);
static gboolean ET_Free_File_Name_Item            (File_Name *FileName);
static gboolean ET_Free_File_Tag_Item_Other_Field (File_Tag *FileTag);
static gboolean ET_Free_File_Info_Item (ET_File_Info *ETFileInfo);
static gboolean ET_Free_History_File_List (void);
static gboolean ET_Free_Displayed_File_List (void);
static gboolean ET_Free_Artist_Album_File_List (void);

static void ET_Initialize_File_Item (ET_File *ETFile);
static void ET_Initialize_File_Tag_Item (File_Tag *FileTag);
static void ET_Initialize_File_Name_Item (File_Name *FileName);
static void ET_Initialize_File_Info_Item (ET_File_Info *ETFileInfo);

//gboolean ET_Copy_File_Tag_Item       (ET_File *ETFile, File_Tag *FileTag);
static gboolean ET_Copy_File_Tag_Item_Other_Field (ET_File *ETFile, File_Tag *FileTag);
//gboolean ET_Set_Field_File_Tag_Picture (gchar **FileTagField, Picture *pic);

static guint ET_File_Key_New (void);
static guint ET_Undo_Key_New (void);

static gboolean ET_Remove_File_From_Artist_Album_List (ET_File *ETFile);

static void ET_Display_Filename_To_UI (ET_File *ETFile);
static EtFileHeaderFields * ET_Display_File_Info_To_UI (ET_File *ETFile);

static gboolean ET_Save_File_Name_From_UI (ET_File *ETFile,
                                           File_Name *FileName);
static gboolean ET_Save_File_Name_Internal (ET_File *ETFile, File_Name *FileName);
static gboolean ET_Save_File_Tag_Internal (ET_File *ETFile, File_Tag *FileTag);

static void ET_Mark_File_Tag_As_Saved (ET_File *ETFile);

static gboolean ET_Detect_Changes_Of_File_Name (File_Name *FileName1,
                                                File_Name *FileName2);
static gboolean ET_Add_File_Name_To_List (ET_File *ETFile,
                                          File_Name *FileName);
static gboolean ET_Add_File_Tag_To_List (ET_File *ETFile, File_Tag  *FileTag);
static gboolean ET_Add_File_To_History_List (ET_File *ETFile);
static gboolean ET_Add_File_To_Artist_Album_File_List (ET_File *ETFile);

static guint ET_Displayed_File_List_Get_Length      (void);
static void ET_Displayed_File_List_Number (void);

static gboolean et_core_read_file_info (const gchar *filename,
                                        ET_File_Info *ETFileInfo,
                                        GError **error);

static gint ET_Comp_Func_Sort_Artist_Item_By_Ascending_Artist (GList *AlbumList1,
                                                               GList *AlbumList2);
static gint ET_Comp_Func_Sort_Album_Item_By_Ascending_Album (GList *etfilelist1,
                                                             GList *etfilelist2);
static gint ET_Comp_Func_Sort_Etfile_Item_By_Ascending_Filename (ET_File *ETFile1,
                                                                 ET_File *ETFile2);
static gchar *ET_File_Name_Format_Extension (ET_File *ETFile);

static void set_sort_order_for_column_id (gint column_id,
                                          GtkTreeViewColumn *column,
                                          EtSortMode sort_type);


/*******************
 * Basic functions *
 *******************/

/*
 * Returns the extension of the file
 */
static gchar *
ET_Get_File_Extension (const gchar *filename)
{
    if (filename)
        return strrchr(filename, '.');
    else
        return NULL;
}


/*
 * Determine description of file using his extension.
 * If extension is NULL or not found into the tab, it returns the last entry for UNKNOWN_FILE.
 */
static const ET_File_Description *
ET_Get_File_Description_From_Extension (const gchar *extension)
{
    guint i;

    if (!extension) // Unknown file
        return &ETFileDescription[ET_FILE_DESCRIPTION_SIZE];

    for (i=0; i<ET_FILE_DESCRIPTION_SIZE; i++)  // Use of '<' instead of '<=' to avoid to test for Unknown file
        if ( strcasecmp(extension,ETFileDescription[i].Extension)==0 )
            return &ETFileDescription[i];

    // If not found in the list
    return &ETFileDescription[ET_FILE_DESCRIPTION_SIZE];
}


/*
 * Determines description of file.
 * Determines first the extension. If extension is NULL or not found into the tab,
 * it returns the last entry for UNKNOWN_FILE.
 */
static const ET_File_Description *
ET_Get_File_Description (const gchar *filename)
{
    return ET_Get_File_Description_From_Extension(ET_Get_File_Extension(filename));
}


/*
 * Returns TRUE if the file is supported, else returns FALSE
 */
gboolean ET_File_Is_Supported (const gchar *filename)
{
    if (ET_Get_File_Description(filename)->FileType != UNKNOWN_FILE)
        return TRUE;
    else
        return FALSE;
}




/*****************************************************************************
 * Manipulation of ET_Core functions (main functions needed for the program) *
 *****************************************************************************/
void ET_Core_Create (void)
{
    // Allocate
    if (ETCore == NULL)
        ETCore = g_malloc0(sizeof(ET_Core));

    // Initialize
    ET_Core_Initialize();
}

void ET_Core_Initialize (void)
{
    ETCore->ETFileList                        = NULL;
    ETCore->ETFileDisplayedList               = NULL;
    ETCore->ETFileDisplayedListPtr            = NULL;
    ETCore->ETFileDisplayedList_Length        = 0;
    ETCore->ETFileDisplayedList_TotalSize     = 0;
    ETCore->ETFileDisplayedList_TotalDuration = 0;
    ETCore->ETFileDisplayed                   = NULL;
    ETCore->ETHistoryFileList                 = NULL;
    ETCore->ETArtistAlbumFileList             = NULL;
}

void ET_Core_Free (void)
{
    // Frees first lists, then initialize
    if (ETCore->ETFileList)
        ET_Free_File_List();

    if (ETCore->ETFileDisplayedList)
        ET_Free_Displayed_File_List();

    if (ETCore->ETHistoryFileList)
        ET_Free_History_File_List();

    if (ETCore->ETArtistAlbumFileList)
        ET_Free_Artist_Album_File_List();

    // Initialize by security
    ET_Core_Initialize();
}

void
ET_Core_Destroy (void)
{
    if (ETCore)
    {
        ET_Core_Free ();
        g_free (ETCore);
        ETCore = NULL;
    }
}



/**************************
 * Initializing functions *
 **************************/

/*
 * Create a new File_Name structure
 */
File_Name *ET_File_Name_Item_New (void)
{
    File_Name *FileName;

    FileName = g_malloc0(sizeof(File_Name));
    ET_Initialize_File_Name_Item(FileName);

    return FileName;
}


/*
 * Create a new File_Tag structure
 */
File_Tag *ET_File_Tag_Item_New (void)
{
    File_Tag *FileTag;

    FileTag = g_malloc0(sizeof(File_Tag));
    ET_Initialize_File_Tag_Item(FileTag);

    return FileTag;
}


/*
 * Create a new File_Info structure
 */
static ET_File_Info *
ET_File_Info_Item_New (void)
{
    ET_File_Info *ETFileInfo;

    ETFileInfo = g_malloc0(sizeof(ET_File_Info));
    ET_Initialize_File_Info_Item(ETFileInfo);

    return ETFileInfo;
}


/*
 * Create a new ET_File structure
 */
ET_File *ET_File_Item_New (void)
{
    ET_File *ETFile;

    ETFile = g_malloc0(sizeof(ET_File));
    ET_Initialize_File_Item(ETFile);

    return ETFile;
}


static void
ET_Initialize_File_Name_Item (File_Name *FileName)
{
    if (FileName)
    {
        FileName->key        = ET_Undo_Key_New();
        FileName->saved      = FALSE;
        FileName->value      = NULL;
        FileName->value_utf8 = NULL;
        FileName->value_ck   = NULL;
    }
}


static void
ET_Initialize_File_Tag_Item (File_Tag *FileTag)
{
    if (FileTag)
    {
        FileTag->key         = ET_Undo_Key_New();
        FileTag->saved       = FALSE;
        FileTag->title       = NULL;
        FileTag->artist      = NULL;
        FileTag->album_artist= NULL;
        FileTag->album       = NULL;
        FileTag->disc_number = NULL;
        FileTag->disc_total = NULL;
        FileTag->track       = NULL;
        FileTag->track_total = NULL;
        FileTag->year        = NULL;
        FileTag->genre       = NULL;
        FileTag->comment     = NULL;
        FileTag->composer    = NULL;
        FileTag->orig_artist = NULL;
        FileTag->copyright   = NULL;
        FileTag->url         = NULL;
        FileTag->encoded_by  = NULL;
        FileTag->picture     = NULL;
        FileTag->other       = NULL;
    }
}


static void
ET_Initialize_File_Info_Item (ET_File_Info *ETFileInfo)
{
    if (ETFileInfo)
    {
        ETFileInfo->mpc_profile = NULL;
        ETFileInfo->mpc_version = NULL;
    }
}


static void
ET_Initialize_File_Item (ET_File *ETFile)
{
    if (ETFile)
    {
        ETFile->IndexKey          = 0;
        ETFile->ETFileKey         = 0;
        ETFile->ETFileDescription = NULL;
        ETFile->ETFileInfo        = NULL;
        ETFile->FileNameCur       = NULL;
        ETFile->FileNameNew       = NULL;
        ETFile->FileNameList      = NULL;
        ETFile->FileNameListBak   = NULL;
        ETFile->FileTag           = NULL;
        ETFile->FileTagList       = NULL;
        ETFile->FileTagListBak    = NULL;
    }
}


/* Key for each item of ETFileList */
static guint
ET_File_Key_New (void)
{
    static guint ETFileKey = 0;
    return ++ETFileKey;
}

/* Key for Undo */
static guint
ET_Undo_Key_New (void)
{
    static guint ETUndoKey = 0;
    return ++ETUndoKey;
}




/**********************************
 * File adding/removing functions *
 **********************************/

/*
 * ET_Add_File_To_File_List: Add a file to the "main" list. And get all information of the file.
 * The filename passed in should be in raw format, only convert it to UTF8 when displaying it.
 */
GList *ET_Add_File_To_File_List (gchar *filename)
{
    const ET_File_Description *ETFileDescription;
    ET_File      *ETFile;
    File_Name    *FileName;
    File_Tag     *FileTag;
    ET_File_Info *ETFileInfo;
    gchar        *ETFileExtension;
    guint         ETFileKey;
    guint         undo_key;
    GFile *file;
    GFileInfo *fileinfo;
    gchar        *filename_utf8 = filename_to_display(filename);
    const gchar  *locale_lc_ctype = getenv("LC_CTYPE");
    GError *error = NULL;

    if (!filename)
        return ETCore->ETFileList;

    file = g_file_new_for_path (filename);

    /* Primary Key for this file */
    ETFileKey = ET_File_Key_New();

    /* Get description of the file */
    ETFileDescription = ET_Get_File_Description(filename);

    /* Get real extension of the file (keeping the case) */
    ETFileExtension = g_strdup(ET_Get_File_Extension(filename));

    /* Fill the File_Name structure for FileNameList */
    FileName = ET_File_Name_Item_New();
    FileName->saved      = TRUE;    /* The file hasn't been changed, so it's saved */
    FileName->value      = filename;
    FileName->value_utf8 = filename_utf8;
    FileName->value_ck   = g_utf8_collate_key_for_filename(filename_utf8, -1);

    /* Fill the File_Tag structure for FileTagList */
    FileTag = ET_File_Tag_Item_New();
    FileTag->saved = TRUE;    /* The file hasn't been changed, so it's saved */

    /* Patch from Doruk Fisek and Onur Kucuk: avoid upper/lower conversion bugs
     * (like I->i conversion in some locales) in tag parsing. The problem occurs
     * for example with Turkish language where it can't read 'TITLE=' field if
     * it is written as 'Title=' in the file */
    setlocale(LC_CTYPE, "C");

    switch (ETFileDescription->TagType)
    {
#ifdef ENABLE_MP3
        case ID3_TAG:
            Id3tag_Read_File_Tag(filename,FileTag);
            break;
#endif
#ifdef ENABLE_OGG
        case OGG_TAG:
            if (!ogg_tag_read_file_tag (filename, FileTag, &error))
            {
                Log_Print (LOG_ERROR,
                           _("Error reading tag from ogg file (%s)"),
                           error->message);
                g_clear_error (&error);
            }
            break;
#endif
#ifdef ENABLE_FLAC
        case FLAC_TAG:
            Flac_Tag_Read_File_Tag(filename,FileTag);
            break;
#endif
        case APE_TAG:
            Ape_Tag_Read_File_Tag(filename,FileTag);
            break;
#ifdef ENABLE_MP4
        case MP4_TAG:
            Mp4tag_Read_File_Tag(filename,FileTag);
            break;
#endif
#ifdef ENABLE_WAVPACK
        case WAVPACK_TAG:
            Wavpack_Tag_Read_File_Tag(filename, FileTag);
        break;
#endif
#ifdef ENABLE_OPUS
        case OPUS_TAG:
            if (!et_opus_tag_read_file_tag (file, FileTag, &error))
            {
                Log_Print (LOG_ERROR,
                           _("Error reading tag from Opus file (%s)"),
                           error->message);
                g_clear_error (&error);
            }
            break;
#endif
        case UNKNOWN_TAG:
        default:
            Log_Print(LOG_ERROR,"FileTag: Undefined tag type (%d) for file %s",ETFileDescription->TagType,filename_utf8);
            break;
    }

    /* Fill the ET_File_Info structure */
    ETFileInfo = ET_File_Info_Item_New ();

    switch (ETFileDescription->FileType)
    {
#if defined ENABLE_MP3 && defined ENABLE_ID3LIB
        case MP3_FILE:
        case MP2_FILE:
            Mpeg_Header_Read_File_Info(filename,ETFileInfo);
            break;
#endif
#ifdef ENABLE_OGG
        case OGG_FILE:
            Ogg_Header_Read_File_Info(filename,ETFileInfo);
            break;
#endif
#ifdef ENABLE_SPEEX
        case SPEEX_FILE:
            Speex_Header_Read_File_Info(filename,ETFileInfo);
            break;
#endif
#ifdef ENABLE_FLAC
        case FLAC_FILE:
            Flac_Header_Read_File_Info(filename,ETFileInfo);
            break;
#endif
        case MPC_FILE:
            Mpc_Header_Read_File_Info(filename,ETFileInfo);
            break;
        case MAC_FILE:
            Mac_Header_Read_File_Info(filename,ETFileInfo);
            break;
#ifdef ENABLE_WAVPACK
        case WAVPACK_FILE:
            Wavpack_Header_Read_File_Info(filename, ETFileInfo);
            break;
#endif
#ifdef ENABLE_MP4
        case MP4_FILE:
            Mp4_Header_Read_File_Info(filename,ETFileInfo);
            break;
#endif
#ifdef ENABLE_OPUS
        case OPUS_FILE:
            if (!et_opus_read_file_info (file, ETFileInfo, &error))
            {
                Log_Print (LOG_ERROR, _("Error while querying information for file '%s': %s"),
                           filename_utf8, error->message);
                g_error_free (error);
            }
            break;
#endif
        case UNKNOWN_FILE:
        default:
            Log_Print(LOG_ERROR,"ETFileInfo: Undefined file type (%d) for file %s",ETFileDescription->FileType,filename_utf8);
            /* To get at least the file size. */
            if (!et_core_read_file_info (filename, ETFileInfo, &error))
            {
                Log_Print (LOG_ERROR, _("Error while querying information for file '%s': %s"),
                           filename_utf8, error->message);
                g_error_free (error);
            }

            break;
    }

    /* Restore previous value */
    setlocale(LC_CTYPE, locale_lc_ctype ? locale_lc_ctype : "");

    /* Store the modification time of the file to check if the file was changed
     * before saving */
    fileinfo = g_file_query_info (file, G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                  G_FILE_QUERY_INFO_NONE, NULL, NULL);
    g_object_unref (file);

    /* Attach all data defined above to this ETFile item */
    ETFile = ET_File_Item_New();

    if (fileinfo)
    {
        ETFile->FileModificationTime = g_file_info_get_attribute_uint64 (fileinfo,
                                                                         G_FILE_ATTRIBUTE_TIME_MODIFIED);
        g_object_unref (fileinfo);
    }
    else
    {
        ETFile->FileModificationTime = 0;
    }

    ETFile->IndexKey             = 0; // Will be renumered after...
    ETFile->ETFileKey            = ETFileKey;
    ETFile->ETFileDescription    = ETFileDescription;
    ETFile->ETFileExtension      = ETFileExtension;
    ETFile->FileNameList         = g_list_append(NULL,FileName);
    ETFile->FileNameCur          = ETFile->FileNameList;
    ETFile->FileNameNew          = ETFile->FileNameList;
    ETFile->FileTagList          = g_list_append(NULL,FileTag);
    ETFile->FileTag              = ETFile->FileTagList;
    ETFile->ETFileInfo           = ETFileInfo;

    /* Add the item to the "main list" */
    ETCore->ETFileList = g_list_append(ETCore->ETFileList,ETFile);


    /*
     * Process the filename and tag to generate undo if needed...
     * The undo key must be the same for FileName and FileTag => changed in the same time
     */
    undo_key = ET_Undo_Key_New();

    FileName = ET_File_Name_Item_New();
    FileName->key = undo_key;
    ET_Save_File_Name_Internal(ETFile,FileName);

    FileTag = ET_File_Tag_Item_New();
    FileTag->key = undo_key;
    ET_Save_File_Tag_Internal(ETFile,FileTag);

    /*
     * Generate undo for the file and the main undo list.
     * If no changes detected, FileName and FileTag item are deleted.
     */
    ET_Manage_Changes_Of_File_Data(ETFile,FileName,FileTag);

    /*
     * Display a message if the file was changed at start
     */
    FileTag  = (File_Tag *)ETFile->FileTag->data;
    FileName = (File_Name *)ETFile->FileNameNew->data;
    if ( (FileName && FileName->saved==FALSE) || (FileTag && FileTag->saved==FALSE) )
    {
        Log_Print(LOG_INFO,_("Automatic corrections applied for file '%s'."), filename_utf8);
    }

    /* Add the item to the ArtistAlbum list (placed here to take advantage of previous changes) */
    //ET_Add_File_To_Artist_Album_File_List(ETFile);

    //ET_Debug_Print_File_List(ETCore->ETFileList,__FILE__,__LINE__,__FUNCTION__);
    return ETCore->ETFileList;
}

gboolean ET_Create_Artist_Album_File_List (void)
{
    GList *l;

    if (ETCore->ETArtistAlbumFileList)
        ET_Free_Artist_Album_File_List();

    for (l = g_list_first (ETCore->ETFileList); l != NULL; l = g_list_next (l))
    {
        ET_File *ETFile = (ET_File *)l->data;
        ET_Add_File_To_Artist_Album_File_List(ETFile);
    }
    //ET_Debug_Print_Artist_Album_List(__FILE__,__LINE__,__FUNCTION__);
    return TRUE;
}
/*
 * The ETArtistAlbumFileList contains 3 levels of lists to sort the ETFile by artist then by album :
 *  - "ETArtistAlbumFileList" list is a list of "ArtistList" items,
 *  - "ArtistList" list is a list of "AlbumList" items,
 *  - "AlbumList" list is a list of ETFile items.
 * Note : use the function ET_Debug_Print_Artist_Album_List(...) to understand how it works, it needed...
 */
static gboolean
ET_Add_File_To_Artist_Album_File_List (ET_File *ETFile)
{
    if (ETFile)
    {
        gchar *ETFile_Artist = ((File_Tag *)ETFile->FileTag->data)->artist; // Artist value of the ETFile passed in parameter
        gchar *ETFile_Album  = ((File_Tag *)ETFile->FileTag->data)->album;  // Album  value of the ETFile passed in parameter
        gchar *etfile_artist = NULL;
        gchar *etfile_album  = NULL;
        GList *ArtistList;
        GList *AlbumList     = NULL;
        GList *etfilelist    = NULL;
        ET_File *etfile      = NULL;

        for (ArtistList = ETCore->ETArtistAlbumFileList; ArtistList != NULL;
             ArtistList = g_list_next (ArtistList))
        {
            AlbumList = (GList *)ArtistList->data;  /* Take the first item */
            if (AlbumList
            && (etfilelist = (GList *)AlbumList->data)       /* Take the first item */
            && (etfile     = (ET_File *)etfilelist->data)    /* Take the first etfile item */
            && ((File_Tag *)etfile->FileTag->data) != NULL )
            {
                etfile_artist = ((File_Tag *)etfile->FileTag->data)->artist;
            }else
            {
                etfile_artist = NULL;
            }

            if ( (etfile_artist &&  ETFile_Artist && strcmp(etfile_artist,ETFile_Artist)==0)
            ||   (!etfile_artist && !ETFile_Artist) ) // The "artist" values correspond?
            {
                // The "ArtistList" item was found!
                while (AlbumList)
                {
                    if ( (etfilelist = (GList *)AlbumList->data)
                    &&   (etfile     = (ET_File *)etfilelist->data)
                    &&   ((File_Tag *)etfile->FileTag->data) != NULL )
                    {
                        etfile_album  = ((File_Tag *)etfile->FileTag->data)->album;
                    }else
                    {
                        etfile_album  = NULL;
                    }

                    if ( (etfile_album &&  ETFile_Album && strcmp(etfile_album,ETFile_Album)==0)
                    ||   (!etfile_album && !ETFile_Album) ) // The "album" values correspond?
                    {
                        // The "AlbumList" item was found!
                        // Add the ETFile to this AlbumList item
                        //g_print(">>>  add to etfile list (%s)\n",g_path_get_basename(((File_Name *)ETFile->FileNameCur->data)->value));
                        AlbumList->data = (gpointer) g_list_append((GList *)AlbumList->data,ETFile);
                        AlbumList->data = (gpointer) g_list_sort((GList *)AlbumList->data,(GCompareFunc)ET_Comp_Func_Sort_Etfile_Item_By_Ascending_Filename);
                        return TRUE;
                    }
                    AlbumList = g_list_next (AlbumList);
                }
                // The "AlbumList" item was NOT found! => Add a new "AlbumList" item (+...) item to the "ArtistList" list
                etfilelist = g_list_append(NULL,ETFile);
                //g_print(">>>  add new album (%s)\n",g_path_get_basename(((File_Name *)ETFile->FileNameCur->data)->value));
                ArtistList->data = (gpointer) g_list_append((GList *)ArtistList->data,etfilelist);
                ArtistList->data = (gpointer) g_list_sort((GList *)ArtistList->data,(GCompareFunc)ET_Comp_Func_Sort_Album_Item_By_Ascending_Album);
                return TRUE;
            }
        }
        // The "ArtistList" item was NOT found! => Add a new "ArtistList" to the main list (=ETArtistAlbumFileList)
        etfilelist = g_list_append(NULL,ETFile);
        AlbumList  = g_list_append(NULL,etfilelist);
        //g_print(">>>  add new artist (%s)(etfile:%x AlbumList:%x)\n",g_path_get_basename(((File_Name *)ETFile->FileNameCur->data)->value),etfilelist,AlbumList);
        ETCore->ETArtistAlbumFileList = g_list_append(ETCore->ETArtistAlbumFileList,AlbumList);
        // Sort the list by ascending Artist
        ETCore->ETArtistAlbumFileList = g_list_sort(ETCore->ETArtistAlbumFileList,(GCompareFunc)ET_Comp_Func_Sort_Artist_Item_By_Ascending_Artist);

        return TRUE;
    }else
    {
        return FALSE;
    }
}



/*
 * Delete the corresponding file and free the allocated data. Return TRUE if deleted.
 */
gboolean ET_Remove_File_From_File_List (ET_File *ETFile)
{
    GList *ETFileList = NULL;          // Item containing the ETFile to delete... (in ETCore->ETFileList)
    GList *ETFileDisplayedList = NULL; // Item containing the ETFile to delete... (in ETCore->ETFileDisplayedList)

    // Remove infos of the file
    ETCore->ETFileDisplayedList_TotalSize     -= ((ET_File_Info *)ETFile->ETFileInfo)->size;
    ETCore->ETFileDisplayedList_TotalDuration -= ((ET_File_Info *)ETFile->ETFileInfo)->duration;

    // Find the ETFileList containing the ETFile item
    ETFileDisplayedList = g_list_find(g_list_first(ETCore->ETFileDisplayedList),ETFile);
    ETFileList          = g_list_find(g_list_first(ETCore->ETFileList),ETFile);

    // Note : this ETFileList must be used only for ETCore->ETFileDisplayedList, and not ETCore->ETFileDisplayed
    if (ETCore->ETFileDisplayedList == ETFileDisplayedList)
    {
        if (ETFileList->next)
            ETCore->ETFileDisplayedList = ETFileDisplayedList->next;
        else if (ETFileList->prev)
            ETCore->ETFileDisplayedList = ETFileDisplayedList->prev;
        else
            ETCore->ETFileDisplayedList = NULL;
    }
    // If the current displayed file is just removing, it will be unable to display it again!
    if (ETCore->ETFileDisplayed == ETFile)
    {
        if (ETCore->ETFileDisplayedList)
            ETCore->ETFileDisplayed = (ET_File *)ETCore->ETFileDisplayedList->data;
        else
            ETCore->ETFileDisplayed = (ET_File *)NULL;
    }

    // Remove the file from the ETFileList list
    ETCore->ETFileList = g_list_first(g_list_remove_link(g_list_first(ETCore->ETFileList),ETFileList));

    // Remove the file from the ETArtistAlbumList list
    ET_Remove_File_From_Artist_Album_List(ETFile);

    // Remove the file from the ETFileDisplayedList list (if not already done)
    if ( (ETFileDisplayedList = g_list_find(ETCore->ETFileDisplayedList,ETFile)) )
    {
        ETCore->ETFileDisplayedList = g_list_first(g_list_remove_link(g_list_first(ETCore->ETFileDisplayedList),ETFileDisplayedList));
    }

    // Free data of the file
    ET_Free_File_List_Item(ETFile);
    if (ETFileList)
        g_list_free(ETFileList);
    if (ETFileDisplayedList)
        g_list_free(ETFileDisplayedList);

    // Recalculate length of ETFileDisplayedList list
    ET_Displayed_File_List_Get_Length();

    // To number the ETFile in the list
    ET_Displayed_File_List_Number();

    // Displaying...
    if (ETCore->ETFileDisplayedList)
    {
        if (ETCore->ETFileDisplayed)
        {
            ET_Displayed_File_List_By_Etfile(ETCore->ETFileDisplayed);
        }else if (ETCore->ETFileDisplayedList->data)
        {
            // Select the new file (synchronize index,...)
            ET_Displayed_File_List_By_Etfile((ET_File *)ETCore->ETFileDisplayedList->data);
        }
    }else
    {
        // Reinit the tag and file area
        et_application_window_file_area_clear (ET_APPLICATION_WINDOW (MainWindow));
        et_application_window_tag_area_clear (ET_APPLICATION_WINDOW (MainWindow));
        et_application_window_update_actions (ET_APPLICATION_WINDOW (MainWindow));
    }

    return TRUE;
}


/*
 * Delete the corresponding file (allocated data was previously freed!). Return TRUE if deleted.
 */
static gboolean
ET_Remove_File_From_Artist_Album_List (ET_File *ETFile)
{
    GList *ArtistList;
    GList *AlbumList;
    GList *etfilelist;

    g_return_val_if_fail (ETFile != NULL, FALSE);

    /* Search for the ETFile in the list. */
    for (ArtistList = ETCore->ETArtistAlbumFileList; ArtistList != NULL;
         ArtistList = g_list_next (ArtistList))
    {
        for (AlbumList = g_list_first ((GList *)ArtistList->data);
             AlbumList != NULL; AlbumList = g_list_next (AlbumList))
        {
            for (etfilelist = g_list_first ((GList *)AlbumList->data);
                 etfilelist != NULL; etfilelist = g_list_next (etfilelist))
            {
                ET_File *etfile = (ET_File *)etfilelist->data;

                if (ETFile == etfile) /* The ETFile to delete was found! */
                {
                    etfilelist = g_list_remove (etfilelist, ETFile);

                    if (etfilelist) /* Delete from AlbumList. */
                    {
                        AlbumList->data = (gpointer) g_list_first (etfilelist);
                    }
                    else
                    {
                        AlbumList = g_list_remove (AlbumList, AlbumList->data);

                        if (AlbumList) /* Delete from ArtistList. */
                        {
                            ArtistList->data = (gpointer) g_list_first (AlbumList);
                        }
                        else
                        {
                            /* Delete from the main list. */
                            ETCore->ETArtistAlbumFileList = g_list_remove (ArtistList,ArtistList->data);

                            if (ETCore->ETArtistAlbumFileList)
                            {
                                ETCore->ETArtistAlbumFileList = g_list_first (ETCore->ETArtistAlbumFileList);
                            }

                            return TRUE;
                        }

                        return TRUE;
                    }

                    return TRUE;
                }
            }
        }
    }

    return FALSE; /* ETFile is NUL, or not found in the list. */
}

/**************************
 * File sorting functions *
 **************************/

/*
 * Sort the 'ETFileDisplayedList' following the 'Sorting_Type'
 * Note : Add also new sorting in 'Browser_List_Sort_Func'
 */
static void
ET_Sort_Displayed_File_List (EtSortMode Sorting_Type)
{
    ETCore->ETFileDisplayedList = ET_Sort_File_List(ETCore->ETFileDisplayedList,Sorting_Type);
}

/*
 * Sort an 'ETFileList'
 */
GList *
ET_Sort_File_List (GList *ETFileList, EtSortMode Sorting_Type)
{
    EtApplicationWindow *window;
    GtkTreeViewColumn *column;
    GList *etfilelist;
    gint column_id = Sorting_Type / 2;

    window = ET_APPLICATION_WINDOW (MainWindow);
    column = et_application_window_browser_get_column_for_column_id (window,
                                                                     column_id);

    /* Important to rewind before. */
    etfilelist = g_list_first (ETFileList);

    /* FIXME: Port to sort-mode? */
    set_sort_order_for_column_id (column_id, column, Sorting_Type);

    /* Sort... */
    switch (Sorting_Type)
    {
        case ET_SORT_MODE_ASCENDING_FILENAME:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_Filename);
            break;
        case ET_SORT_MODE_DESCENDING_FILENAME:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_Filename);
            break;
        case ET_SORT_MODE_ASCENDING_TITLE:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_Title);
            break;
        case ET_SORT_MODE_DESCENDING_TITLE:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_Title);
            break;
        case ET_SORT_MODE_ASCENDING_ARTIST:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_Artist);
            break;
        case ET_SORT_MODE_DESCENDING_ARTIST:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_Artist);
            break;
        case ET_SORT_MODE_ASCENDING_ALBUM_ARTIST:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_Album_Artist);
            break;
        case ET_SORT_MODE_DESCENDING_ALBUM_ARTIST:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_Album_Artist);
            break;
		case ET_SORT_MODE_ASCENDING_ALBUM:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_Album);
            break;
        case ET_SORT_MODE_DESCENDING_ALBUM:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_Album);
            break;
        case ET_SORT_MODE_ASCENDING_YEAR:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_Year);
            break;
        case ET_SORT_MODE_DESCENDING_YEAR:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_Year);
            break;
        case ET_SORT_MODE_ASCENDING_DISC_NUMBER:
            etfilelist = g_list_sort (etfilelist,
                                      (GCompareFunc)et_comp_func_sort_file_by_ascending_disc_number);
            break;
        case ET_SORT_MODE_DESCENDING_DISC_NUMBER:
            etfilelist = g_list_sort (etfilelist,
                                      (GCompareFunc)et_comp_func_sort_file_by_descending_disc_number);
            break;
        case ET_SORT_MODE_ASCENDING_TRACK_NUMBER:
            etfilelist = g_list_sort (etfilelist,
                                      (GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_Track_Number);
            break;
        case ET_SORT_MODE_DESCENDING_TRACK_NUMBER:
            etfilelist = g_list_sort (etfilelist,
                                      (GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_Track_Number);
            break;
        case ET_SORT_MODE_ASCENDING_GENRE:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_Genre);
            break;
        case ET_SORT_MODE_DESCENDING_GENRE:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_Genre);
            break;
        case ET_SORT_MODE_ASCENDING_COMMENT:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_Comment);
            break;
        case ET_SORT_MODE_DESCENDING_COMMENT:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_Comment);
            break;
        case ET_SORT_MODE_ASCENDING_COMPOSER:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_Composer);
            break;
        case ET_SORT_MODE_DESCENDING_COMPOSER:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_Composer);
            break;
        case ET_SORT_MODE_ASCENDING_ORIG_ARTIST:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_Orig_Artist);
            break;
        case ET_SORT_MODE_DESCENDING_ORIG_ARTIST:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_Orig_Artist);
            break;
        case ET_SORT_MODE_ASCENDING_COPYRIGHT:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_Copyright);
            break;
        case ET_SORT_MODE_DESCENDING_COPYRIGHT:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_Copyright);
            break;
        case ET_SORT_MODE_ASCENDING_URL:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_Url);
            break;
        case ET_SORT_MODE_DESCENDING_URL:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_Url);
            break;
        case ET_SORT_MODE_ASCENDING_ENCODED_BY:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_Encoded_By);
            break;
        case ET_SORT_MODE_DESCENDING_ENCODED_BY:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_Encoded_By);
            break;
        case ET_SORT_MODE_ASCENDING_CREATION_DATE:
            etfilelist = g_list_sort (etfilelist,
                                      (GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_Creation_Date);
            break;
        case ET_SORT_MODE_DESCENDING_CREATION_DATE:
            etfilelist = g_list_sort (etfilelist,
                                      (GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_Creation_Date);
            break;
        case ET_SORT_MODE_ASCENDING_FILE_TYPE:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_File_Type);
            break;
        case ET_SORT_MODE_DESCENDING_FILE_TYPE:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_File_Type);
            break;
        case ET_SORT_MODE_ASCENDING_FILE_SIZE:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_File_Size);
            break;
        case ET_SORT_MODE_DESCENDING_FILE_SIZE:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_File_Size);
            break;
        case ET_SORT_MODE_ASCENDING_FILE_DURATION:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_File_Duration);
            break;
        case ET_SORT_MODE_DESCENDING_FILE_DURATION:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_File_Duration);
            break;
        case ET_SORT_MODE_ASCENDING_FILE_BITRATE:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_File_Bitrate);
            break;
        case ET_SORT_MODE_DESCENDING_FILE_BITRATE:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_File_Bitrate);
            break;
        case ET_SORT_MODE_ASCENDING_FILE_SAMPLERATE:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Ascending_File_Samplerate);
            break;
        case ET_SORT_MODE_DESCENDING_FILE_SAMPLERATE:
            etfilelist = g_list_sort(etfilelist,(GCompareFunc)ET_Comp_Func_Sort_File_By_Descending_File_Samplerate);
            break;
    }
    /* Save sorting mode (note: needed when called from UI). */
    g_settings_set_enum (MainSettings, "sort-mode", Sorting_Type);

    //ETFileList = g_list_first(etfilelist);
    return g_list_first(etfilelist);
}


/*
 * Sort the list of files following the 'Sorting_Type' value. The new sorting is displayed in the UI.
 */
void
ET_Sort_Displayed_File_List_And_Update_UI (EtSortMode Sorting_Type)
{
    g_return_if_fail (ETCore->ETFileList != NULL);

    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    /* Sort the list */
    ET_Sort_Displayed_File_List(Sorting_Type);

    /* To number the ETFile in the list */
    ET_Displayed_File_List_Number();

    /* Reload files in browser list */
    ET_Displayed_File_List_By_Etfile(ETCore->ETFileDisplayed);  // Just to update 'ETFileDisplayedList'
    et_application_window_browser_select_file_by_et_file (ET_APPLICATION_WINDOW (MainWindow),
                                                          ETCore->ETFileDisplayed,
                                                          TRUE);
    ET_Display_File_Data_To_UI(ETCore->ETFileDisplayed);

    et_application_window_browser_refresh_sort (ET_APPLICATION_WINDOW (MainWindow));
    et_application_window_update_actions (ET_APPLICATION_WINDOW (MainWindow));
}


/*
 * Comparison function for sorting by ascending filename.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_Filename (ET_File *ETFile1, ET_File *ETFile2)
{
    gchar *file1_ck   = ((File_Name *)((GList *)ETFile1->FileNameCur)->data)->value_ck;
    gchar *file2_ck   = ((File_Name *)((GList *)ETFile2->FileNameCur)->data)->value_ck;
    // !!!! : Must be the same rules as "Cddb_Track_List_Sort_Func" to be
    // able to sort in the same order files in cddb and in the file list.
    return g_settings_get_boolean (MainSettings,
                                   "sort-case-sensitive") ? strcmp (file1_ck, file2_ck)
                                                          : strcasecmp (file1_ck, file2_ck);
}

/*
 * Comparison function for sorting by descending filename.
 */
gint ET_Comp_Func_Sort_File_By_Descending_Filename (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending disc number.
 */
gint
et_comp_func_sort_file_by_ascending_disc_number (ET_File *ETFile1,
                                                 ET_File *ETFile2)
{
    gint track1, track2;

    if (!ETFile1->FileTag->data
        || !((File_Tag *)ETFile1->FileTag->data)->disc_number)
    {
        track1 = 0;
    }
    else
    {
        track1 = atoi (((File_Tag *)ETFile1->FileTag->data)->disc_number);
    }

    if (!ETFile2->FileTag->data
        || !((File_Tag *)ETFile2->FileTag->data)->disc_number)
    {
        track2 = 0;
    }
    else
    {
        track2 = atoi (((File_Tag *)ETFile2->FileTag->data)->disc_number);
    }

    /* Second criterion. */
    if (track1 == track2)
    {
        return ET_Comp_Func_Sort_File_By_Ascending_Filename (ETFile1, ETFile2);
    }

    /* First criterion. */
    return (track1 - track2);
}

/*
 * Comparison function for sorting by descending disc number.
 */
gint
et_comp_func_sort_file_by_descending_disc_number (ET_File *ETFile1,
                                                  ET_File *ETFile2)
{
    return et_comp_func_sort_file_by_ascending_disc_number (ETFile2, ETFile1);
}


/*
 * Comparison function for sorting by ascending track number.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_Track_Number (ET_File *ETFile1, ET_File *ETFile2)
{
    gint track1, track2;

    if ( !ETFile1->FileTag->data || !((File_Tag *)ETFile1->FileTag->data)->track )
        track1 = 0;
    else
        track1 = atoi( ((File_Tag *)ETFile1->FileTag->data)->track );

    if ( !ETFile2->FileTag->data || !((File_Tag *)ETFile2->FileTag->data)->track )
        track2 = 0;
    else
        track2 = atoi( ((File_Tag *)ETFile2->FileTag->data)->track );

    // Second criterion
    if (track1 == track2)
        return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);

    // First criterion
    return (track1 - track2);
}

/*
 * Comparison function for sorting by descending track number.
 */
gint ET_Comp_Func_Sort_File_By_Descending_Track_Number (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Track_Number(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending creation date.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_Creation_Date (ET_File *ETFile1, ET_File *ETFile2)
{
    GFile *file;
    GFileInfo *info;
    guint64 time1 = 0;
    guint64 time2 = 0;

    /* TODO: Report errors? */
    file = g_file_new_for_path (((File_Name *)ETFile1->FileNameCur->data)->value);
    info = g_file_query_info (file, G_FILE_ATTRIBUTE_TIME_CHANGED,
                              G_FILE_QUERY_INFO_NONE, NULL, NULL);

    g_object_unref (file);

    if (info)
    {
        time1 = g_file_info_get_attribute_uint64 (info,
                                                  G_FILE_ATTRIBUTE_TIME_CHANGED);
        g_object_unref (info);
    }

    file = g_file_new_for_path (((File_Name *)ETFile2->FileNameCur->data)->value);
    info = g_file_query_info (file, G_FILE_ATTRIBUTE_TIME_CHANGED,
                              G_FILE_QUERY_INFO_NONE, NULL, NULL);

    g_object_unref (file);

    if (info)
    {
        time2 = g_file_info_get_attribute_uint64 (info,
                                                  G_FILE_ATTRIBUTE_TIME_CHANGED);
        g_object_unref (info);
    }

    /* Second criterion. */
    if (time1 == time2)
    {
        return ET_Comp_Func_Sort_File_By_Ascending_Filename (ETFile1, ETFile2);
    }

    /* First criterion. */
    return (gint64)(time1 - time2);
}

/*
 * Comparison function for sorting by descending creation date.
 */
gint ET_Comp_Func_Sort_File_By_Descending_Creation_Date (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Creation_Date(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending title.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_Title (ET_File *ETFile1, ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->title == ((File_Tag *)ETFile2->FileTag->data)->title))
        return 0;

    if ( !ETFile1->FileTag->data || !((File_Tag *)ETFile1->FileTag->data)->title )
        return -1;
    if ( !ETFile2->FileTag->data || !((File_Tag *)ETFile2->FileTag->data)->title )
        return 1;

    if (g_settings_get_boolean (MainSettings, "sort-case-sensitive"))
    {
        if ( strcmp(((File_Tag *)ETFile1->FileTag->data)->title,((File_Tag *)ETFile2->FileTag->data)->title) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcmp(((File_Tag *)ETFile1->FileTag->data)->title,((File_Tag *)ETFile2->FileTag->data)->title);
    }else
    {
        if ( strcasecmp(((File_Tag *)ETFile1->FileTag->data)->title,((File_Tag *)ETFile2->FileTag->data)->title) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
      else
            // First criterion
            return strcasecmp(((File_Tag *)ETFile1->FileTag->data)->title,((File_Tag *)ETFile2->FileTag->data)->title);
    }
}

/*
 * Comparison function for sorting by descending title.
 */
gint ET_Comp_Func_Sort_File_By_Descending_Title (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Title(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending artist.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_Artist (ET_File *ETFile1, ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->artist == ((File_Tag *)ETFile2->FileTag->data)->artist))
        return 0;

    if ( !ETFile1->FileTag->data || !((File_Tag *)ETFile1->FileTag->data)->artist )
        return -1;
    if ( !ETFile2->FileTag->data || !((File_Tag *)ETFile2->FileTag->data)->artist )
        return 1;

    if (g_settings_get_boolean (MainSettings, "sort-case-sensitive"))
    {
        if ( strcmp(((File_Tag *)ETFile1->FileTag->data)->artist,((File_Tag *)ETFile2->FileTag->data)->artist) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcmp(((File_Tag *)ETFile1->FileTag->data)->artist,((File_Tag *)ETFile2->FileTag->data)->artist);
    }else
    {
        if ( strcasecmp(((File_Tag *)ETFile1->FileTag->data)->artist,((File_Tag *)ETFile2->FileTag->data)->artist) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcasecmp(((File_Tag *)ETFile1->FileTag->data)->artist,((File_Tag *)ETFile2->FileTag->data)->artist);
    }
}

/*
 * Comparison function for sorting by descending artist.
 */
gint ET_Comp_Func_Sort_File_By_Descending_Artist (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Artist(ETFile2,ETFile1);
}

/*
 * Comparison function for sorting by ascending album artist.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_Album_Artist (ET_File *ETFile1, ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->album_artist == ((File_Tag *)ETFile2->FileTag->data)->album_artist))
        return 0;

    if ( !ETFile1->FileTag->data || !((File_Tag *)ETFile1->FileTag->data)->album_artist )
        return -1;
    if ( !ETFile2->FileTag->data || !((File_Tag *)ETFile2->FileTag->data)->album_artist )
        return 1;

    if (g_settings_get_boolean (MainSettings, "sort-case-sensitive"))
    {
        if ( strcmp(((File_Tag *)ETFile1->FileTag->data)->album_artist,((File_Tag *)ETFile2->FileTag->data)->album_artist) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Artist(ETFile1,ETFile2);
        else
            // First criterion
            return strcmp(((File_Tag *)ETFile1->FileTag->data)->album_artist,((File_Tag *)ETFile2->FileTag->data)->album_artist);
    }else
    {
        if ( strcasecmp(((File_Tag *)ETFile1->FileTag->data)->album_artist,((File_Tag *)ETFile2->FileTag->data)->album_artist) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Artist(ETFile1,ETFile2);
        else
            // First criterion
            return strcasecmp(((File_Tag *)ETFile1->FileTag->data)->album_artist,((File_Tag *)ETFile2->FileTag->data)->album_artist);
    }
}

/*
 * Comparison function for sorting by descending album artist.
 */
gint ET_Comp_Func_Sort_File_By_Descending_Album_Artist (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Album_Artist(ETFile2,ETFile1);
}

/*
 * Comparison function for sorting by ascending album.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_Album (ET_File *ETFile1, ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->album == ((File_Tag *)ETFile2->FileTag->data)->album))
        return 0;

    if ( !ETFile1->FileTag->data || !((File_Tag *)ETFile1->FileTag->data)->album )
        return -1;
    if ( !ETFile2->FileTag->data || !((File_Tag *)ETFile2->FileTag->data)->album )
        return 1;

    if (g_settings_get_boolean (MainSettings, "sort-case-sensitive"))
    {
        if ( strcmp(((File_Tag *)ETFile1->FileTag->data)->album,((File_Tag *)ETFile2->FileTag->data)->album) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcmp(((File_Tag *)ETFile1->FileTag->data)->album,((File_Tag *)ETFile2->FileTag->data)->album);
    }else
    {
        if ( strcasecmp(((File_Tag *)ETFile1->FileTag->data)->album,((File_Tag *)ETFile2->FileTag->data)->album) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcasecmp(((File_Tag *)ETFile1->FileTag->data)->album,((File_Tag *)ETFile2->FileTag->data)->album);
    }
}

/*
 * Comparison function for sorting by descending album.
 */
gint ET_Comp_Func_Sort_File_By_Descending_Album (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Album(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending year.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_Year (ET_File *ETFile1, ET_File *ETFile2)
{
    gint year1, year2;

    if ( !ETFile1->FileTag->data || !((File_Tag *)ETFile1->FileTag->data)->year )
        year1 = 0;
    else
        year1 = atoi( ((File_Tag *)ETFile1->FileTag->data)->year );

    if ( !ETFile2->FileTag->data || !((File_Tag *)ETFile2->FileTag->data)->year )
        year2 = 0;
    else
        year2 = atoi( ((File_Tag *)ETFile2->FileTag->data)->year );

    // Second criterion
    if (year1 == year2)
        return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);

    // First criterion
    return (year1 - year2);
}

/*
 * Comparison function for sorting by descending year.
 */
gint ET_Comp_Func_Sort_File_By_Descending_Year (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Year(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending genre.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_Genre (ET_File *ETFile1, ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->genre == ((File_Tag *)ETFile2->FileTag->data)->genre))
        return 0;

    if ( !ETFile1->FileTag->data || !((File_Tag *)ETFile1->FileTag->data)->genre ) return -1;
    if ( !ETFile2->FileTag->data || !((File_Tag *)ETFile2->FileTag->data)->genre ) return 1;

    if (g_settings_get_boolean (MainSettings, "sort-case-sensitive"))
    {
        if ( strcmp(((File_Tag *)ETFile1->FileTag->data)->genre,((File_Tag *)ETFile2->FileTag->data)->genre) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcmp(((File_Tag *)ETFile1->FileTag->data)->genre,((File_Tag *)ETFile2->FileTag->data)->genre);
    }else
    {
        if ( strcasecmp(((File_Tag *)ETFile1->FileTag->data)->genre,((File_Tag *)ETFile2->FileTag->data)->genre) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcasecmp(((File_Tag *)ETFile1->FileTag->data)->genre,((File_Tag *)ETFile2->FileTag->data)->genre);
    }
}

/*
 * Comparison function for sorting by descending genre.
 */
gint ET_Comp_Func_Sort_File_By_Descending_Genre (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Genre(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending comment.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_Comment (ET_File *ETFile1, ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->comment == ((File_Tag *)ETFile2->FileTag->data)->comment))
        return 0;

    if ( !ETFile1->FileTag->data || !((File_Tag *)ETFile1->FileTag->data)->comment )
        return -1;
    if ( !ETFile2->FileTag->data || !((File_Tag *)ETFile2->FileTag->data)->comment )
        return 1;

    if (g_settings_get_boolean (MainSettings, "sort-case-sensitive"))
    {
        if ( strcmp(((File_Tag *)ETFile1->FileTag->data)->comment,((File_Tag *)ETFile2->FileTag->data)->comment) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcmp(((File_Tag *)ETFile1->FileTag->data)->comment,((File_Tag *)ETFile2->FileTag->data)->comment);
    }else
    {
        if ( strcasecmp(((File_Tag *)ETFile1->FileTag->data)->comment,((File_Tag *)ETFile2->FileTag->data)->comment) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcasecmp(((File_Tag *)ETFile1->FileTag->data)->comment,((File_Tag *)ETFile2->FileTag->data)->comment);
    }
}

/*
 * Comparison function for sorting by descending comment.
 */
gint ET_Comp_Func_Sort_File_By_Descending_Comment (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Comment(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending composer.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_Composer (ET_File *ETFile1, ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->composer == ((File_Tag *)ETFile2->FileTag->data)->composer))
        return 0;

    if ( !ETFile1->FileTag->data || !((File_Tag *)ETFile1->FileTag->data)->composer )
        return -1;
    if ( !ETFile2->FileTag->data || !((File_Tag *)ETFile2->FileTag->data)->composer )
        return 1;

    if (g_settings_get_boolean (MainSettings, "sort-case-sensitive"))
    {
        if ( strcmp(((File_Tag *)ETFile1->FileTag->data)->composer,((File_Tag *)ETFile2->FileTag->data)->composer) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcmp(((File_Tag *)ETFile1->FileTag->data)->composer,((File_Tag *)ETFile2->FileTag->data)->composer);
    }else
    {
        if ( strcasecmp(((File_Tag *)ETFile1->FileTag->data)->composer,((File_Tag *)ETFile2->FileTag->data)->composer) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcasecmp(((File_Tag *)ETFile1->FileTag->data)->composer,((File_Tag *)ETFile2->FileTag->data)->composer);
    }
}

/*
 * Comparison function for sorting by descending composer.
 */
gint ET_Comp_Func_Sort_File_By_Descending_Composer (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Composer(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending original artist.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_Orig_Artist (ET_File *ETFile1, ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->orig_artist == ((File_Tag *)ETFile2->FileTag->data)->orig_artist))
        return 0;

    if ( !ETFile1->FileTag->data || !((File_Tag *)ETFile1->FileTag->data)->orig_artist )
        return -1;
    if ( !ETFile2->FileTag->data || !((File_Tag *)ETFile2->FileTag->data)->orig_artist )
        return 1;

    if (g_settings_get_boolean (MainSettings, "sort-case-sensitive"))
    {
        if ( strcmp(((File_Tag *)ETFile1->FileTag->data)->orig_artist,((File_Tag *)ETFile2->FileTag->data)->orig_artist) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcmp(((File_Tag *)ETFile1->FileTag->data)->orig_artist,((File_Tag *)ETFile2->FileTag->data)->orig_artist);
    }else
    {
        if ( strcasecmp(((File_Tag *)ETFile1->FileTag->data)->orig_artist,((File_Tag *)ETFile2->FileTag->data)->orig_artist) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcasecmp(((File_Tag *)ETFile1->FileTag->data)->orig_artist,((File_Tag *)ETFile2->FileTag->data)->orig_artist);
    }
}

/*
 * Comparison function for sorting by descending original artist.
 */
gint ET_Comp_Func_Sort_File_By_Descending_Orig_Artist (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Orig_Artist(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending copyright.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_Copyright (ET_File *ETFile1, ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->copyright == ((File_Tag *)ETFile2->FileTag->data)->copyright))
        return 0;

    if ( !ETFile1->FileTag->data || !((File_Tag *)ETFile1->FileTag->data)->copyright )
        return -1;
    if ( !ETFile2->FileTag->data || !((File_Tag *)ETFile2->FileTag->data)->copyright )
        return 1;

    if (g_settings_get_boolean (MainSettings, "sort-case-sensitive"))
    {
        if ( strcmp(((File_Tag *)ETFile1->FileTag->data)->copyright,((File_Tag *)ETFile2->FileTag->data)->copyright) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcmp(((File_Tag *)ETFile1->FileTag->data)->copyright,((File_Tag *)ETFile2->FileTag->data)->copyright);
    }else
    {
        if ( strcasecmp(((File_Tag *)ETFile1->FileTag->data)->copyright,((File_Tag *)ETFile2->FileTag->data)->copyright) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcasecmp(((File_Tag *)ETFile1->FileTag->data)->copyright,((File_Tag *)ETFile2->FileTag->data)->copyright);
    }
}

/*
 * Comparison function for sorting by descending copyright.
 */
gint ET_Comp_Func_Sort_File_By_Descending_Copyright (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Copyright(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending URL.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_Url (ET_File *ETFile1, ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->url == ((File_Tag *)ETFile2->FileTag->data)->url))
        return 0;

    if ( !ETFile1->FileTag->data || !((File_Tag *)ETFile1->FileTag->data)->url )
        return -1;
    if ( !ETFile2->FileTag->data || !((File_Tag *)ETFile2->FileTag->data)->url )
        return 1;

    if (g_settings_get_boolean (MainSettings, "sort-case-sensitive"))
    {
        if ( strcmp(((File_Tag *)ETFile1->FileTag->data)->url,((File_Tag *)ETFile2->FileTag->data)->url) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcmp(((File_Tag *)ETFile1->FileTag->data)->url,((File_Tag *)ETFile2->FileTag->data)->url);
    }else
    {
        if ( strcasecmp(((File_Tag *)ETFile1->FileTag->data)->url,((File_Tag *)ETFile2->FileTag->data)->url) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcasecmp(((File_Tag *)ETFile1->FileTag->data)->url,((File_Tag *)ETFile2->FileTag->data)->url);
    }
}

/*
 * Comparison function for sorting by descending URL.
 */
gint ET_Comp_Func_Sort_File_By_Descending_Url (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Url(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending encoded by.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_Encoded_By (ET_File *ETFile1, ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->encoded_by == ((File_Tag *)ETFile2->FileTag->data)->encoded_by))
        return 0;

    if ( !ETFile1->FileTag->data || !((File_Tag *)ETFile1->FileTag->data)->encoded_by )
        return -1;
    if ( !ETFile2->FileTag->data || !((File_Tag *)ETFile2->FileTag->data)->encoded_by )
        return 1;

    if (g_settings_get_boolean (MainSettings, "sort-case-sensitive"))
    {
        if ( strcmp(((File_Tag *)ETFile1->FileTag->data)->encoded_by,((File_Tag *)ETFile2->FileTag->data)->encoded_by) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcmp(((File_Tag *)ETFile1->FileTag->data)->encoded_by,((File_Tag *)ETFile2->FileTag->data)->encoded_by);
    }else
    {
        if ( strcasecmp(((File_Tag *)ETFile1->FileTag->data)->encoded_by,((File_Tag *)ETFile2->FileTag->data)->encoded_by) == 0 )
            // Second criterion
            return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
        else
            // First criterion
            return strcasecmp(((File_Tag *)ETFile1->FileTag->data)->encoded_by,((File_Tag *)ETFile2->FileTag->data)->encoded_by);
    }
}

/*
 * Comparison function for sorting by descendingencoded by.
 */
gint ET_Comp_Func_Sort_File_By_Descending_Encoded_By (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Encoded_By(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending file type (mp3, ogg, ...).
 */
gint ET_Comp_Func_Sort_File_By_Ascending_File_Type (ET_File *ETFile1, ET_File *ETFile2)
{
    if ( !ETFile1->ETFileDescription ) return -1;
    if ( !ETFile2->ETFileDescription ) return 1;

    // Second criterion
    if (ETFile1->ETFileDescription->FileType == ETFile2->ETFileDescription->FileType)
        return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);

    // First criterion
    return (ETFile1->ETFileDescription->FileType - ETFile2->ETFileDescription->FileType);
}

/*
 * Comparison function for sorting by descending file type (mp3, ogg, ...).
 */
gint ET_Comp_Func_Sort_File_By_Descending_File_Type (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_File_Type(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending file size.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_File_Size (ET_File *ETFile1, ET_File *ETFile2)
{
    if ( !ETFile1->ETFileInfo ) return -1;
    if ( !ETFile2->ETFileInfo ) return 1;

    // Second criterion
    if (ETFile1->ETFileInfo->size == ETFile2->ETFileInfo->size)
        return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);

    // First criterion
    return (ETFile1->ETFileInfo->size - ETFile2->ETFileInfo->size);
}

/*
 * Comparison function for sorting by descending file size.
 */
gint ET_Comp_Func_Sort_File_By_Descending_File_Size (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_File_Size(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending file duration.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_File_Duration (ET_File *ETFile1, ET_File *ETFile2)
{
    if ( !ETFile1->ETFileInfo ) return -1;
    if ( !ETFile2->ETFileInfo ) return 1;

    // Second criterion
    if (ETFile1->ETFileInfo->duration == ETFile2->ETFileInfo->duration)
        return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);

    // First criterion
    return (ETFile1->ETFileInfo->duration - ETFile2->ETFileInfo->duration);
}

/*
 * Comparison function for sorting by descending file duration.
 */
gint ET_Comp_Func_Sort_File_By_Descending_File_Duration (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_File_Duration(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending file bitrate.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_File_Bitrate (ET_File *ETFile1, ET_File *ETFile2)
{
    if ( !ETFile1->ETFileInfo ) return -1;
    if ( !ETFile2->ETFileInfo ) return 1;

    // Second criterion
    if (ETFile1->ETFileInfo->bitrate == ETFile2->ETFileInfo->bitrate)
        return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);

    // First criterion
    return (ETFile1->ETFileInfo->bitrate - ETFile2->ETFileInfo->bitrate);
}

/*
 * Comparison function for sorting by descending file bitrate.
 */
gint ET_Comp_Func_Sort_File_By_Descending_File_Bitrate (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_File_Bitrate(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending file samplerate.
 */
gint ET_Comp_Func_Sort_File_By_Ascending_File_Samplerate (ET_File *ETFile1, ET_File *ETFile2)
{
    if ( !ETFile1->ETFileInfo ) return -1;
    if ( !ETFile2->ETFileInfo ) return 1;

    // Second criterion
    if (ETFile1->ETFileInfo->samplerate == ETFile2->ETFileInfo->samplerate)
        return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);

    // First criterion
    return (ETFile1->ETFileInfo->samplerate - ETFile2->ETFileInfo->samplerate);
}

/*
 * Comparison function for sorting by descending file samplerate.
 */
gint ET_Comp_Func_Sort_File_By_Descending_File_Samplerate (ET_File *ETFile1, ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_File_Samplerate(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending artist in the ArtistAlbumList.
 */
static gint ET_Comp_Func_Sort_Artist_Item_By_Ascending_Artist (GList *AlbumList1,
                                                               GList *AlbumList2)
{
    GList   *etfilelist1 = NULL,    *etfilelist2 = NULL;
    ET_File *etfile1 = NULL,        *etfile2 = NULL;
    gchar   *etfile1_artist, *etfile2_artist;

    if (!AlbumList1
    || !(etfilelist1    = (GList *)AlbumList1->data)
    || !(etfile1        = (ET_File *)etfilelist1->data)
    || !(etfile1_artist = ((File_Tag *)etfile1->FileTag->data)->artist) )
        return -1;

    if (!AlbumList2
    || !(etfilelist2    = (GList *)AlbumList2->data)
    || !(etfile2        = (ET_File *)etfilelist2->data)
    || !(etfile2_artist = ((File_Tag *)etfile2->FileTag->data)->artist) )
        return 1;

    /*if (g_settings_get_boolean (MainSettings, "sort-case-sensitive"))
     *    return strcmp(etfile1_artist,etfile2_artist); */
    //else
        return strcasecmp(etfile1_artist,etfile2_artist);
}

/*
 * Comparison function for sorting by ascending album in the ArtistAlbumList.
 */
static gint
ET_Comp_Func_Sort_Album_Item_By_Ascending_Album (GList *etfilelist1,
                                                 GList *etfilelist2)
{
    ET_File *etfile1,       *etfile2;
    gchar   *etfile1_album, *etfile2_album;

    if (!etfilelist1
    || !(etfile1        = (ET_File *)etfilelist1->data)
    || !(etfile1_album  = ((File_Tag *)etfile1->FileTag->data)->album) )
        return -1;

    if (!etfilelist2
    || !(etfile2        = (ET_File *)etfilelist2->data)
    || !(etfile2_album  = ((File_Tag *)etfile2->FileTag->data)->album) )
        return 1;

    /*if (g_settings_get_boolean (MainSettings, "sort-case-sensitive"))
     *    return strcmp(etfile1_album,etfile2_album); */
    //else
        return strcasecmp(etfile1_album,etfile2_album);
}

/*
 * Comparison function for sorting etfile in the ArtistAlbumList.
 * FIX ME : should use the default sorting!
 */
static gint
ET_Comp_Func_Sort_Etfile_Item_By_Ascending_Filename (ET_File *ETFile1,
                                                     ET_File *ETFile2)
{

    if (!ETFile1) return -1;
    if (!ETFile2) return  1;

    return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
}




/***************************
 * List handling functions *
 ***************************/

/*
 * Returns the first item of the "displayed list"
 */
GList *ET_Displayed_File_List_First (void)
{
    ETCore->ETFileDisplayedList = g_list_first(ETCore->ETFileDisplayedList);
    return ETCore->ETFileDisplayedList;
}

/*
 * Returns the previous item of the "displayed list". When no more item, it returns NULL.
 */
GList *ET_Displayed_File_List_Previous (void)
{
    if (ETCore->ETFileDisplayedList && ETCore->ETFileDisplayedList->prev)
        return ETCore->ETFileDisplayedList = ETCore->ETFileDisplayedList->prev;
    else
        return NULL;
}

/*
 * Returns the next item of the "displayed list".
 * When no more item, it returns NULL to don't "overwrite" the list.
 */
GList *ET_Displayed_File_List_Next (void)
{
    if (ETCore->ETFileDisplayedList && ETCore->ETFileDisplayedList->next)
        return ETCore->ETFileDisplayedList = ETCore->ETFileDisplayedList->next;
    else
        return NULL;
}

/*
 * Returns the last item of the "displayed list"
 */
GList *ET_Displayed_File_List_Last (void)
{
    ETCore->ETFileDisplayedList = g_list_last(ETCore->ETFileDisplayedList);
    return ETCore->ETFileDisplayedList;
}

/*
 * Returns the item of the "displayed list" which correspond to the given 'ETFile' (used into browser list).
 */
GList *ET_Displayed_File_List_By_Etfile (ET_File *ETFile)
{
    GList *etfilelist;

    for (etfilelist = ET_Displayed_File_List_First (); etfilelist != NULL;
         etfilelist = ET_Displayed_File_List_Next ())
    {
        if (ETFile == (ET_File *)etfilelist->data)
            break;
    }

    if (etfilelist)
    {
        /* To "save" the position like in ET_File_List_Next... (not very good -
         * FIXME) */
        ETCore->ETFileDisplayedList = etfilelist;
    }
    return etfilelist;
}

/*
 * Just returns the current item of the "main list"
 */
/*GList *ET_Displayed_File_List_Current (void)
{
    return ETCore->ETFileDisplayedList;
    //return ETCore->ETFileDisplayedListPtr;
}*/

/*
 * Renumber the list of displayed files (IndexKey) from 1 to n
 */
static void
ET_Displayed_File_List_Number (void)
{
    GList *l = NULL;
    guint i = 1;

    for (l = g_list_first (ETCore->ETFileDisplayedList); l != NULL;
         l = g_list_next (l))
    {
        ((ET_File *)l->data)->IndexKey = i++;
    }
}

/*
 * Returns the length of the list of displayed files
 */
static guint
ET_Displayed_File_List_Get_Length (void)
{
    GList *list = NULL;

    list = g_list_first(ETCore->ETFileDisplayedList);
    ETCore->ETFileDisplayedList_Length = g_list_length(list);
    return ETCore->ETFileDisplayedList_Length;
}

/*
 * Load the list of displayed files (calculate length, size, ...)
 * It contains part (filtrated : view by artists and albums) or full ETCore->ETFileList list
 */
gboolean ET_Set_Displayed_File_List (GList *ETFileList)
{
    GList *l = NULL;

    ETCore->ETFileDisplayedList = g_list_first(ETFileList);

    //ETCore->ETFileDisplayedListPtr = ETCore->ETFileDisplayedList;
    ETCore->ETFileDisplayedList_Length = ET_Displayed_File_List_Get_Length();
    ETCore->ETFileDisplayedList_TotalSize     = 0;
    ETCore->ETFileDisplayedList_TotalDuration = 0;

    // Get size and duration of files in the list
    for (l = ETCore->ETFileDisplayedList; l != NULL; l = g_list_next (l))
    {
        ETCore->ETFileDisplayedList_TotalSize += ((ET_File_Info *)((ET_File *)l->data)->ETFileInfo)->size;
        ETCore->ETFileDisplayedList_TotalDuration += ((ET_File_Info *)((ET_File *)l->data)->ETFileInfo)->duration;
    }

    /* Sort the file list. */
    ET_Sort_Displayed_File_List (g_settings_get_enum (MainSettings,
                                 "sort-mode"));

    // Should renums ETCore->ETFileDisplayedList only!
    ET_Displayed_File_List_Number();

    return TRUE;
}



/*********************
 * Freeing functions *
 *********************/

/*
 * Frees the full main list of files: GList *ETFileList and reinitialize it.
 */
gboolean ET_Free_File_List (void)
{
    g_return_val_if_fail (ETCore != NULL && ETCore->ETFileList != NULL, FALSE);

    g_list_free_full (ETCore->ETFileList,
                      (GDestroyNotify)ET_Free_File_List_Item);
    ETCore->ETFileList = NULL;

    return TRUE;
}


/*
 * Frees one item of the full main list of files.
 */
gboolean ET_Free_File_List_Item (ET_File *ETFile)
{
    if (ETFile)
    {
        /* Frees the lists */
        if (ETFile->FileNameList)
        {
            ET_Free_File_Name_List(ETFile->FileNameList);
        }
        if (ETFile->FileNameListBak)
        {
            ET_Free_File_Name_List(ETFile->FileNameListBak);
        }
        if (ETFile->FileTagList)
        {
            ET_Free_File_Tag_List (ETFile->FileTagList);
        }
        if (ETFile->FileTagListBak)
        {
            ET_Free_File_Tag_List (ETFile->FileTagListBak);
        }
        /* Frees infos of ETFileInfo */
        if (ETFile->ETFileInfo)
        {
            ET_Free_File_Info_Item (ETFile->ETFileInfo);
        }
        g_free(ETFile->ETFileExtension);
        g_free(ETFile);
    }

    return TRUE;
}


/*
 * Frees the full list: GList *FileNameList.
 */
gboolean ET_Free_File_Name_List (GList *FileNameList)
{
    g_return_val_if_fail (FileNameList != NULL, FALSE);

    FileNameList = g_list_first (FileNameList);

    g_list_free_full (FileNameList, (GDestroyNotify)ET_Free_File_Name_Item);

    return TRUE;
}


/*
 * Frees a File_Name item.
 */
gboolean ET_Free_File_Name_Item (File_Name *FileName)
{
    g_return_val_if_fail (FileName != NULL, FALSE);

    g_free(FileName->value);
    g_free(FileName->value_utf8);
    g_free(FileName->value_ck);
    g_free(FileName);

    return TRUE;
}


/*
 * Frees the full list: GList *TagList.
 */
static gboolean
ET_Free_File_Tag_List (GList *FileTagList)
{
    GList *l;

    g_return_val_if_fail (FileTagList != NULL, FALSE);

    FileTagList = g_list_first (FileTagList);

    for (l = FileTagList; l != NULL; l = g_list_next (l))
    {
        if ((File_Tag *)l->data)
            ET_Free_File_Tag_Item ((File_Tag *)l->data);
    }

    g_list_free (FileTagList);

    return TRUE;
}


/*
 * Frees the list of 'other' field in a File_Tag item (contains attached gchar data).
 */
static gboolean
ET_Free_File_Tag_Item_Other_Field (File_Tag *FileTag)
{
    g_list_free_full (FileTag->other, g_free);

    return TRUE;
}


/*
 * Frees a File_Tag item.
 */
gboolean ET_Free_File_Tag_Item (File_Tag *FileTag)
{
    g_return_val_if_fail (FileTag != NULL, FALSE);

    g_free(FileTag->title);
    g_free(FileTag->artist);
    g_free(FileTag->album_artist);
    g_free(FileTag->album);
    g_free(FileTag->disc_number);
    g_free (FileTag->disc_total);
    g_free(FileTag->year);
    g_free(FileTag->track);
    g_free(FileTag->track_total);
    g_free(FileTag->genre);
    g_free(FileTag->comment);
    g_free(FileTag->composer);
    g_free(FileTag->orig_artist);
    g_free(FileTag->copyright);
    g_free(FileTag->url);
    g_free(FileTag->encoded_by);
    Picture_Free(FileTag->picture);
    // Free list of other fields
    ET_Free_File_Tag_Item_Other_Field(FileTag);

    g_free(FileTag);
    return TRUE;
}


/*
 * Frees a File_Info item.
 */
static gboolean
ET_Free_File_Info_Item (ET_File_Info *ETFileInfo)
{
    g_return_val_if_fail (ETFileInfo != NULL, FALSE);

    g_free(ETFileInfo->mpc_profile);
    g_free(ETFileInfo->mpc_version);

    g_free(ETFileInfo);
    return TRUE;
}


/*
 * History list contains only pointers, so no data to free except the history structure.
 */
static gboolean
ET_Free_History_File_List (void)
{
    g_return_val_if_fail (ETCore != NULL && ETCore->ETHistoryFileList != NULL,
                          FALSE);

    ETCore->ETHistoryFileList = g_list_first (ETCore->ETHistoryFileList);

    g_list_free_full (ETCore->ETHistoryFileList, g_free);

    ETCore->ETHistoryFileList = NULL;

    return TRUE;
}

/*
 * "Display" list contains only pointers, so NOTHING to free
 */
static gboolean
ET_Free_Displayed_File_List (void)
{
    g_return_val_if_fail (ETCore != NULL
                          && ETCore->ETFileDisplayedList != NULL, FALSE);

    ETCore->ETFileDisplayedList = NULL;

    return TRUE;
}


/*
 * ArtistAlbum list contains 3 levels of lists
 */
static gboolean
ET_Free_Artist_Album_File_List (void)
{
    GList *l;

    g_return_val_if_fail (ETCore != NULL
                          && ETCore->ETArtistAlbumFileList != NULL, FALSE);

    /* Pointers are stored inside the artist/album list-stores, so free them
     * first. */
    et_application_window_browser_clear_artist_model (ET_APPLICATION_WINDOW (MainWindow));
    et_application_window_browser_clear_album_model (ET_APPLICATION_WINDOW (MainWindow));

    for (l = ETCore->ETArtistAlbumFileList; l != NULL; l = g_list_next (l))
    {
        GList *m;

	for (m = (GList *)l->data; m != NULL; m = g_list_next (m))
        {
            GList *n = (GList *)m->data;
            if (n)
                g_list_free (n);
        }

        if (l->data) /* Free AlbumList list. */
            g_list_free ((GList *)l->data);
    }

    if (ETCore->ETArtistAlbumFileList)
        g_list_free(ETCore->ETArtistAlbumFileList);

    ETCore->ETArtistAlbumFileList = NULL;

    return TRUE;
}




/*********************
 * Copying functions *
 *********************/

/*
 * Duplicate the 'other' list
 */
static gboolean
ET_Copy_File_Tag_Item_Other_Field (ET_File *ETFile, File_Tag *FileTag)
{
    File_Tag *FileTagCur;
    GList *l;

    FileTagCur = (File_Tag *)(ETFile->FileTag)->data;

    for (l = FileTagCur->other; l != NULL; l = g_list_next (l))
    {
        FileTag->other = g_list_append (FileTag->other,
                                        g_strdup ((gchar *)l->data));
    }

    return TRUE;
}


/*
 * Copy data of the File_Tag structure (of ETFile) to the FileTag item.
 * Reallocate data if not null.
 */
gboolean ET_Copy_File_Tag_Item (ET_File *ETFile, File_Tag *FileTag)
{
    File_Tag *FileTagCur;

    g_return_val_if_fail (ETFile != NULL && ETFile->FileTag != NULL &&
                          (File_Tag *)(ETFile->FileTag)->data != NULL, FALSE);
    g_return_val_if_fail (FileTag != NULL, FALSE);

    /* The data to duplicate to FileTag */
    FileTagCur = (File_Tag *)(ETFile->FileTag)->data;
    // Key for the item, may be overwritten
    FileTag->key = ET_Undo_Key_New();

    if (FileTagCur->title)
    {
        FileTag->title = g_strdup(FileTagCur->title);
    }else
    {
        g_free(FileTag->title);
        FileTag->title = NULL;
    }

    if (FileTagCur->artist)
    {
        FileTag->artist = g_strdup(FileTagCur->artist);
    }else
    {
        g_free(FileTag->artist);
        FileTag->artist = NULL;
    }

    if (FileTagCur->album_artist)
    {
        FileTag->album_artist = g_strdup(FileTagCur->album_artist);
    }else
    {
        g_free(FileTag->album_artist);
        FileTag->album_artist = NULL;
    }

    if (FileTagCur->album)
    {
        FileTag->album = g_strdup(FileTagCur->album);
    }else
    {
        g_free(FileTag->album);
        FileTag->album = NULL;
    }

    if (FileTagCur->disc_number)
    {
        FileTag->disc_number = g_strdup(FileTagCur->disc_number);
    }else
    {
        g_free(FileTag->disc_number);
        FileTag->disc_number = NULL;
    }

    if (FileTagCur->disc_total)
    {
        FileTag->disc_total = g_strdup (FileTagCur->disc_total);
    }
    else
    {
        g_free (FileTag->disc_total);
        FileTag->disc_total = NULL;
    }

    if (FileTagCur->year)
    {
        FileTag->year = g_strdup(FileTagCur->year);
    }else
    {
        g_free(FileTag->year);
        FileTag->year = NULL;
    }

    if (FileTagCur->track)
    {
        FileTag->track = g_strdup(FileTagCur->track);
    }else
    {
        g_free(FileTag->track);
        FileTag->track = NULL;
    }

    if (FileTagCur->track_total)
    {
        FileTag->track_total = g_strdup(FileTagCur->track_total);
    }else
    {
        g_free(FileTag->track_total);
        FileTag->track_total = NULL;
    }

    if (FileTagCur->genre)
    {
        FileTag->genre = g_strdup(FileTagCur->genre);
    }else
    {
        g_free(FileTag->genre);
        FileTag->genre = NULL;
    }

    if (FileTagCur->comment)
    {
        FileTag->comment = g_strdup(FileTagCur->comment);
    }else
    {
        g_free(FileTag->comment);
        FileTag->comment = NULL;
    }

    if (FileTagCur->composer)
    {
        FileTag->composer = g_strdup(FileTagCur->composer);
    }else
    {
        g_free(FileTag->composer);
        FileTag->composer = NULL;
    }

    if (FileTagCur->orig_artist)
    {
        FileTag->orig_artist = g_strdup(FileTagCur->orig_artist);
    }else
    {
        g_free(FileTag->orig_artist);
        FileTag->orig_artist = NULL;
    }

    if (FileTagCur->copyright)
    {
        FileTag->copyright = g_strdup(FileTagCur->copyright);
    }else
    {
        g_free(FileTag->copyright);
        FileTag->copyright = NULL;
    }

    if (FileTagCur->url)
    {
        FileTag->url = g_strdup(FileTagCur->url);
    }else
    {
        g_free(FileTag->url);
        FileTag->url = NULL;
    }

    if (FileTagCur->encoded_by)
    {
        FileTag->encoded_by = g_strdup(FileTagCur->encoded_by);
    }else
    {
        g_free(FileTag->encoded_by);
        FileTag->encoded_by = NULL;
    }

    if (FileTagCur->picture)
    {
        if (FileTag->picture)
            Picture_Free(FileTag->picture);
        FileTag->picture = Picture_Copy(FileTagCur->picture);
    }else if (FileTag->picture)
    {
        Picture_Free(FileTag->picture);
        FileTag->picture = NULL;
    }

    if (FileTagCur->other)
    {
        ET_Copy_File_Tag_Item_Other_Field(ETFile,FileTag);
    }else
    {
        ET_Free_File_Tag_Item_Other_Field (FileTag);
    }

    return TRUE;
}


/*
 * Fill content of a FileName item according to the filename passed in argument (UTF-8 filename or not)
 * Calculate also the collate key.
 * It treats numbers intelligently so that "file1" "file10" "file5" is sorted as "file1" "file5" "file10"
 */
gboolean ET_Set_Filename_File_Name_Item (File_Name *FileName, gchar *filename_utf8, gchar *filename)
{
    g_return_val_if_fail (FileName != NULL, FALSE);

    if (filename_utf8 && filename)
    {
        FileName->value_utf8 = g_strdup(filename_utf8);
        FileName->value      = g_strdup(filename);
        FileName->value_ck   = g_utf8_collate_key_for_filename(FileName->value_utf8, -1);
    }else if (filename_utf8)
    {
        FileName->value_utf8 = g_strdup(filename_utf8);
        FileName->value      = filename_from_display(filename_utf8);
        FileName->value_ck   = g_utf8_collate_key_for_filename(FileName->value_utf8, -1);
    }else if (filename)
    {
        FileName->value_utf8 = filename_to_display(filename);;
        FileName->value      = g_strdup(filename);
        FileName->value_ck   = g_utf8_collate_key_for_filename(FileName->value_utf8, -1);
    }else
    {
        return FALSE;
    }

    return TRUE;
}


/*
 * Set the value of a field of a FileTag item (for ex, value of FileTag->title)
 * Must be used only for the 'gchar *' components
 */
gboolean ET_Set_Field_File_Tag_Item (gchar **FileTagField, const gchar *value)
{
    g_return_val_if_fail (FileTagField != NULL, FALSE);

    if (*FileTagField != NULL)
    {
        g_free(*FileTagField);
        *FileTagField = NULL;
    }

    if (value != NULL)
    {
        if (g_utf8_strlen(value, -1) > 0)
            *FileTagField = g_strdup(value);
        else
            *FileTagField = NULL;
    }

    return TRUE;
}


/*
 * Set the value of a field of a FileTag Picture item.
 */
gboolean ET_Set_Field_File_Tag_Picture (Picture **FileTagField, Picture *pic)
{
    g_return_val_if_fail (FileTagField != NULL, FALSE);

    if (*FileTagField != NULL)
    {
        Picture_Free((Picture *)*FileTagField);
        *FileTagField = NULL;
    }

    if (pic)
        *FileTagField = Picture_Copy(pic);

    return TRUE;
}


/************************
 * Displaying functions *
 ************************/

static void
et_file_header_fields_free (EtFileHeaderFields *fields)
{
    g_free (fields->version);
    g_free (fields->bitrate);
    g_free (fields->samplerate);
    g_free (fields->mode);
    g_free (fields->size);
    g_free (fields->duration);
    g_slice_free (EtFileHeaderFields, fields);
}

/*
 * Display information of the file (Position + Header + Tag) to the user interface.
 * Before doing it, it saves data of the file currently displayed
 */
void ET_Display_File_Data_To_UI (ET_File *ETFile)
{
    EtApplicationWindow *window;
    const ET_File_Description *ETFileDescription;
    gchar *cur_filename;
    gchar *cur_filename_utf8;
    gchar *msg;
    EtFileHeaderFields *fields;
#ifdef ENABLE_OPUS
    GFile *file;
#endif

    g_return_if_fail (ETFile != NULL &&
                      ((GList *)ETFile->FileNameCur)->data != NULL);
                      /* For the case where ETFile is an "empty" structure. */

    cur_filename      = ((File_Name *)((GList *)ETFile->FileNameCur)->data)->value;
    cur_filename_utf8 = ((File_Name *)((GList *)ETFile->FileNameCur)->data)->value_utf8;
    ETFileDescription = ETFile->ETFileDescription;

    /* Save the current displayed file */
    ETCore->ETFileDisplayed = ETFile;

    window = ET_APPLICATION_WINDOW (MainWindow);

    /* Display position in list + show/hide icon if file writable/read_only (cur_filename) */
    et_application_window_file_area_set_file_fields (window, ETFile);

    /* Display filename (and his path) (value in FileNameNew) */
    ET_Display_Filename_To_UI(ETFile);

    /* Display tag data */
    et_application_window_tag_area_display_et_file (window, ETFile);

    /* Display controls in tag area */
    et_application_window_tag_area_display_controls (window, ETFile);

    /* Display file data, header data and file type */
    switch (ETFileDescription->FileType)
    {
#if defined ENABLE_MP3 && defined ENABLE_ID3LIB
        case MP3_FILE:
        case MP2_FILE:
            fields = Mpeg_Header_Display_File_Info_To_UI (cur_filename,
                                                          ETFile);
            et_application_window_file_area_set_header_fields (window, fields);
            et_mpeg_file_header_fields_free (fields);
            break;
#endif
#ifdef ENABLE_OGG
        case OGG_FILE:
            fields = Ogg_Header_Display_File_Info_To_UI (cur_filename, ETFile);
            et_application_window_file_area_set_header_fields (window, fields);
            et_ogg_file_header_fields_free (fields);
            break;
#endif
#ifdef ENABLE_SPEEX
        case SPEEX_FILE:
            fields = Ogg_Header_Display_File_Info_To_UI (cur_filename, ETFile);
            et_application_window_file_area_set_header_fields (window, fields);
            et_ogg_file_header_fields_free (fields);
            break;
#endif
#ifdef ENABLE_FLAC
        case FLAC_FILE:
            fields = Flac_Header_Display_File_Info_To_UI (cur_filename,
                                                          ETFile);
            et_application_window_file_area_set_header_fields (window, fields);
            et_flac_file_header_fields_free (fields);
            break;
#endif
        case MPC_FILE:
            fields = Mpc_Header_Display_File_Info_To_UI (cur_filename, ETFile);
            et_application_window_file_area_set_header_fields (window, fields);
            et_mpc_file_header_fields_free (fields);
            break;
        case MAC_FILE:
            fields = Mac_Header_Display_File_Info_To_UI (cur_filename,
                                                         ETFile);
            et_application_window_file_area_set_header_fields (window, fields);
            et_mac_file_header_fields_free (fields);
            break;
#ifdef ENABLE_MP4
        case MP4_FILE:
            fields = Mp4_Header_Display_File_Info_To_UI (cur_filename, ETFile);
            et_application_window_file_area_set_header_fields (window, fields);
            et_mp4_file_header_fields_free (fields);
            break;
#endif
#ifdef ENABLE_WAVPACK
        case WAVPACK_FILE:
            fields = Wavpack_Header_Display_File_Info_To_UI (cur_filename,
                                                             ETFile);
            et_application_window_file_area_set_header_fields (window, fields);
            et_wavpack_file_header_fields_free (fields);
            break;
#endif
#ifdef ENABLE_OPUS
        case OPUS_FILE:
            file = g_file_new_for_path (cur_filename);
            fields = et_opus_header_display_file_info_to_ui (file, ETFile);
            et_application_window_file_area_set_header_fields (window, fields);
            et_opus_file_header_fields_free (fields);
            g_object_unref (file);
            break;
#endif
        case UNKNOWN_FILE:
        default:
            /* Default displaying. */
            fields = ET_Display_File_Info_To_UI (ETFile);
            et_application_window_file_area_set_header_fields (window, fields);
            et_file_header_fields_free (fields);
            Log_Print (LOG_ERROR,
                       "ETFileInfo: Undefined file type %d for file %s.",
                       ETFileDescription->FileType, cur_filename_utf8);
            break;
    }

    msg = g_strdup_printf(_("File: '%s'"), cur_filename_utf8);
    et_application_window_status_bar_message (window, msg, FALSE);
    g_free (msg);
}

static void
ET_Display_Filename_To_UI (ET_File *ETFile)
{
    gchar *new_filename_utf8;
    gchar *dirname_utf8;
    gchar *text;

    g_return_if_fail (ETFile != NULL);

    new_filename_utf8 = ((File_Name *)((GList *)ETFile->FileNameNew)->data)->value_utf8;

    /*
     * Set the path to the file into BrowserEntry (dirbrowser)
     */
    dirname_utf8 = g_path_get_dirname(new_filename_utf8);
    et_application_window_browser_entry_set_text (ET_APPLICATION_WINDOW (MainWindow),
                                                  dirname_utf8);

    // And refresh the number of files in this directory
    text = g_strdup_printf(ngettext("One file","%u files",ET_Get_Number_Of_Files_In_Directory(dirname_utf8)),ET_Get_Number_Of_Files_In_Directory(dirname_utf8));
    et_application_window_browser_label_set_text (ET_APPLICATION_WINDOW (MainWindow),
                                                  text);
    g_free(dirname_utf8);
    g_free(text);
}


/*
 * "Default" way to display File Info to the user interface.
 */
static EtFileHeaderFields *
ET_Display_File_Info_To_UI (ET_File *ETFile)
{
    EtFileHeaderFields *fields;
    ET_File_Info *info;
    gchar *time  = NULL;
    gchar *time1 = NULL;
    gchar *size  = NULL;
    gchar *size1 = NULL;

    info = ETFile->ETFileInfo;
    fields = g_slice_new (EtFileHeaderFields);

    fields->description = _("File");

    /* MPEG, Layer versions */
    fields->version = g_strdup_printf ("%d, Layer %d", info->version,
                                       info->layer);

    /* Bitrate */
    fields->bitrate = g_strdup_printf (_("%d kb/s"), info->bitrate);

    /* Samplerate */
    fields->samplerate = g_strdup_printf (_("%d Hz"), info->samplerate);

    /* Mode */
    fields->mode = g_strdup_printf ("%d", info->mode);

    /* Size */
    size = g_format_size (info->size);
    size1 = g_format_size (ETCore->ETFileDisplayedList_TotalSize);
    fields->size = g_strdup_printf ("%s (%s)", size, size1);
    g_free (size);
    g_free (size1);

    /* Duration */
    time = Convert_Duration (info->duration);
    time1 = Convert_Duration (ETCore->ETFileDisplayedList_TotalDuration);
    fields->duration = g_strdup_printf ("%s (%s)", time, time1);
    g_free (time);
    g_free (time1);

    return fields;
}

/********************
 * Saving functions *
 ********************/

/*
 * Save information of the file, contained into the entries of the user interface, in the list.
 * An undo key is generated to be used for filename and tag if there are changed is the same time.
 * Filename and Tag.
 */
void ET_Save_File_Data_From_UI (ET_File *ETFile)
{
    const ET_File_Description *ETFileDescription;
    File_Name *FileName;
    File_Tag  *FileTag;
    guint      undo_key;
    gchar     *cur_filename_utf8;

    if (!ETFile ||
        !ETFile->FileNameCur ||
        !ETFile->FileNameCur->data)
        return;

    cur_filename_utf8 = ((File_Name *)((GList *)ETFile->FileNameCur)->data)->value_utf8;
    ETFileDescription = ETFile->ETFileDescription;
    undo_key = ET_Undo_Key_New();


    /*
     * Save filename and generate undo for filename
     */
    FileName = g_malloc0(sizeof(File_Name));
    ET_Initialize_File_Name_Item(FileName);
    FileName->key = undo_key;
    ET_Save_File_Name_From_UI(ETFile,FileName); // Used for all files!

    switch (ETFileDescription->TagType)
    {
#ifdef ENABLE_MP3
        case ID3_TAG:
#endif
#ifdef ENABLE_OGG
        case OGG_TAG:
#endif
#ifdef ENABLE_FLAC
        case FLAC_TAG:
#endif
#ifdef ENABLE_MP4
        case MP4_TAG:
#endif
#ifdef ENABLE_WAVPACK
        case WAVPACK_TAG:
#endif
#ifdef ENABLE_OPUS
        case OPUS_TAG:
#endif
        case APE_TAG:
            FileTag = et_application_window_tag_area_create_file_tag (ET_APPLICATION_WINDOW (MainWindow));
            ET_Copy_File_Tag_Item_Other_Field (ETFile, FileTag);
            break;
        case UNKNOWN_TAG:
        default:
            FileTag = ET_File_Tag_Item_New ();
            Log_Print(LOG_ERROR,"FileTag: Undefined tag type %d for file %s.",ETFileDescription->TagType,cur_filename_utf8);
            break;
    }

    /*
     * Generate undo for the file and the main undo list.
     * If no changes detected, FileName and FileTag item are deleted.
     */
    ET_Manage_Changes_Of_File_Data(ETFile,FileName,FileTag);

    /* Refresh file into browser list */
    et_application_window_browser_refresh_file_in_list (ET_APPLICATION_WINDOW (MainWindow),
                                                        ETFile);
}


/*
 * Save displayed filename into list if it had been changed. Generates also an history list for undo/redo.
 *  - ETFile : the current etfile that we want to save,
 *  - FileName : where is 'temporary' saved the new filename.
 *
 * Note : it builds new filename (with path) from strings encoded into file system
 *        encoding, not UTF-8 (it preserves file system encoding of parent directories).
 */
static gboolean
ET_Save_File_Name_From_UI (ET_File *ETFile, File_Name *FileName)
{
    gchar *filename_new = NULL;
    gchar *dirname = NULL;
    gchar *filename;
    const gchar *filename_utf8;
    gchar *extension;

    g_return_val_if_fail (ETFile != NULL && FileName != NULL, FALSE);

    filename_utf8 = et_application_window_file_area_get_filename (ET_APPLICATION_WINDOW (MainWindow));
    filename = filename_from_display (filename_utf8);

    if (!filename)
    {
        // If translation fails...
        GtkWidget *msgdialog;
        gchar *filename_escaped_utf8 = g_strescape(filename_utf8, NULL);
        msgdialog = gtk_message_dialog_new(GTK_WINDOW(MainWindow),
                                           GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_CLOSE,
                                           _("Could not convert filename '%s' into system filename encoding"),
                                           filename_escaped_utf8);
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msgdialog),_("Try setting the environment variable G_FILENAME_ENCODING."));
        gtk_window_set_title(GTK_WINDOW(msgdialog), _("Filename translation"));

        gtk_dialog_run(GTK_DIALOG(msgdialog));
        gtk_widget_destroy(msgdialog);
        g_free(filename_escaped_utf8);
        return FALSE;
    }

    // Get the current path to the file
    dirname = g_path_get_dirname( ((File_Name *)ETFile->FileNameNew->data)->value );

    // Convert filename extension (lower or upper)
    extension = ET_File_Name_Format_Extension(ETFile);

    // Check length of filename (limit ~255 characters)
    //ET_File_Name_Check_Length(ETFile,filename);

    // Filename (in file system encoding!)
    if (filename && strlen(filename)>0)
    {
        // Regenerate the new filename (without path)
        filename_new = g_strconcat(filename,extension,NULL);
    }else
    {
        // Keep the 'last' filename (if a 'blank' filename was entered in the fileentry for ex)...
        filename_new = g_path_get_basename( ((File_Name *)ETFile->FileNameNew->data)->value );
    }
    g_free (filename);
    g_free (extension);

    // Check if new filename seems to be correct
    if ( !filename_new || strlen(filename_new) <= strlen(ETFile->ETFileDescription->Extension) )
    {
        FileName->value      = NULL;
        FileName->value_utf8 = NULL;
        FileName->value_ck   = NULL;

        g_free(filename_new);
        g_free(dirname);
        return FALSE;
    }

    // Convert the illegal characters
    ET_File_Name_Convert_Character(filename_new); // FIX ME : should be in UTF8?

    /* Set the new filename (in file system encoding). */
    FileName->value = g_strconcat(dirname,G_DIR_SEPARATOR_S,filename_new,NULL);
    /* Set the new filename (in UTF-8 encoding). */
    FileName->value_utf8 = filename_to_display(FileName->value);
    // Calculates collate key
    FileName->value_ck = g_utf8_collate_key_for_filename(FileName->value_utf8, -1);

    g_free(filename_new);
    g_free(dirname);
    return TRUE;
}


/*
 * Do the same thing of ET_Save_File_Name_From_UI, but without getting the
 * data from the UI.
 */
static gboolean
ET_Save_File_Name_Internal (ET_File *ETFile, File_Name *FileName)
{
    gchar *filename_new = NULL;
    gchar *dirname = NULL;
    gchar *filename;
    gchar *extension;
    gchar *pos;

    g_return_val_if_fail (ETFile != NULL && FileName != NULL, FALSE);

    // Get the current path to the file
    dirname = g_path_get_dirname( ((File_Name *)ETFile->FileNameNew->data)->value );

    // Get the name of file (and rebuild it with extension with a 'correct' case)
    filename = g_path_get_basename( ((File_Name *)ETFile->FileNameNew->data)->value );

    // Remove the extension
    if ((pos=strrchr(filename, '.'))!=NULL)
        *pos = 0;

    // Convert filename extension (lower/upper)
    extension = ET_File_Name_Format_Extension(ETFile);

    // Check length of filename
    //ET_File_Name_Check_Length(ETFile,filename);

    // Regenerate the new filename (without path)
    filename_new = g_strconcat(filename,extension,NULL);
    g_free(extension);
    g_free(filename);

    // Check if new filename seems to be correct
    if (filename_new)
    {
        // Convert the illegal characters
        ET_File_Name_Convert_Character(filename_new);

        /* Set the new filename (in file system encoding). */
        FileName->value = g_strconcat(dirname,G_DIR_SEPARATOR_S,filename_new,NULL);
        /* Set the new filename (in UTF-8 encoding). */
        FileName->value_utf8 = filename_to_display(FileName->value);
        // Calculate collate key
        FileName->value_ck = g_utf8_collate_key_for_filename(FileName->value_utf8, -1);

        g_free(filename_new);
        g_free(dirname);
        return TRUE;
    }else
    {
        FileName->value = NULL;
        FileName->value_utf8 = NULL;
        FileName->value_ck = NULL;

        g_free(filename_new);
        g_free(dirname);
        return FALSE;
    }
}

/*
 * Do the same thing of et_tag_area_create_file_tag without getting the data from the UI.
 */
static gboolean
ET_Save_File_Tag_Internal (ET_File *ETFile, File_Tag *FileTag)
{
    File_Tag *FileTagCur;


    g_return_val_if_fail (ETFile != NULL && ETFile->FileTag != NULL
                          && FileTag != NULL, FALSE);

    FileTagCur = (File_Tag *)ETFile->FileTag->data;

    /* Title */
    if ( FileTagCur->title && g_utf8_strlen(FileTagCur->title, -1)>0 )
    {
        FileTag->title = g_strdup(FileTagCur->title);
        g_strstrip (FileTag->title);
    } else
    {
        FileTag->title = NULL;
    }

    /* Artist */
    if ( FileTagCur->artist && g_utf8_strlen(FileTagCur->artist, -1)>0 )
    {
        FileTag->artist = g_strdup(FileTagCur->artist);
        g_strstrip (FileTag->artist);
    } else
    {
        FileTag->artist = NULL;
    }

	/* Album Artist */
    if ( FileTagCur->album_artist && g_utf8_strlen(FileTagCur->album_artist, -1)>0 )
    {
        FileTag->album_artist = g_strdup(FileTagCur->album_artist);
        g_strstrip (FileTag->album_artist);
    } else
    {
        FileTag->album_artist = NULL;
    }


    /* Album */
    if ( FileTagCur->album && g_utf8_strlen(FileTagCur->album, -1)>0 )
    {
        FileTag->album = g_strdup(FileTagCur->album);
        g_strstrip (FileTag->album);
    } else
    {
        FileTag->album = NULL;
    }


    /* Disc Number */
    if ( FileTagCur->disc_number && g_utf8_strlen(FileTagCur->disc_number, -1)>0 )
    {
        FileTag->disc_number = g_strdup(FileTagCur->disc_number);
        g_strstrip (FileTag->disc_number);
    } else
    {
        FileTag->disc_number = NULL;
    }


    /* Discs Total */
    if (FileTagCur->disc_total
        && g_utf8_strlen (FileTagCur->disc_total, -1) > 0)
    {
        FileTag->disc_total = et_disc_number_to_string (atoi (FileTagCur->disc_total));
        g_strstrip (FileTag->disc_total);
    }
    else
    {
        FileTag->disc_total = NULL;
    }


    /* Year */
    if ( FileTagCur->year && g_utf8_strlen(FileTagCur->year, -1)>0 )
    {
        FileTag->year = g_strdup(FileTagCur->year);
        g_strstrip (FileTag->year);
    } else
    {
        FileTag->year = NULL;
    }


    /* Track */
    if ( FileTagCur->track && g_utf8_strlen(FileTagCur->track, -1)>0 )
    {
        gchar *tmp_str;

        FileTag->track = et_track_number_to_string (atoi (FileTagCur->track));

        // This field must contain only digits
        tmp_str = FileTag->track;
        while (isdigit((guchar)*tmp_str)) tmp_str++;
            *tmp_str = 0;
        g_strstrip (FileTag->track);
    } else
    {
        FileTag->track = NULL;
    }


    /* Track Total */
    if ( FileTagCur->track_total && g_utf8_strlen(FileTagCur->track_total, -1)>0 )
    {
        FileTag->track_total = et_track_number_to_string (atoi (FileTagCur->track_total));
        g_strstrip (FileTag->track_total);
    } else
    {
        FileTag->track_total = NULL;
    }


    /* Genre */
    if ( FileTagCur->genre && g_utf8_strlen(FileTagCur->genre, -1)>0 )
    {
        FileTag->genre = g_strdup(FileTagCur->genre);
        g_strstrip (FileTag->genre);
    } else
    {
        FileTag->genre = NULL;
    }


    /* Comment */
    if ( FileTagCur->comment && g_utf8_strlen(FileTagCur->comment, -1)>0 )
    {
        FileTag->comment = g_strdup(FileTagCur->comment);
        g_strstrip (FileTag->comment);
    } else
    {
        FileTag->comment = NULL;
    }


    /* Composer */
    if ( FileTagCur->composer && g_utf8_strlen(FileTagCur->composer, -1)>0 )
    {
        FileTag->composer = g_strdup(FileTagCur->composer);
        g_strstrip (FileTag->composer);
    } else
    {
        FileTag->composer = NULL;
    }


    /* Original Artist */
    if ( FileTagCur->orig_artist && g_utf8_strlen(FileTagCur->orig_artist, -1)>0 )
    {
        FileTag->orig_artist = g_strdup(FileTagCur->orig_artist);
        g_strstrip (FileTag->orig_artist);
    } else
    {
        FileTag->orig_artist = NULL;
    }


    /* Copyright */
    if ( FileTagCur->copyright && g_utf8_strlen(FileTagCur->copyright, -1)>0 )
    {
        FileTag->copyright = g_strdup(FileTagCur->copyright);
        g_strstrip (FileTag->copyright);
    } else
    {
        FileTag->copyright = NULL;
    }


    /* URL */
    if ( FileTagCur->url && g_utf8_strlen(FileTagCur->url, -1)>0 )
    {
        FileTag->url = g_strdup(FileTagCur->url);
        g_strstrip (FileTag->url);
    } else
    {
        FileTag->url = NULL;
    }


    /* Encoded by */
    if ( FileTagCur->encoded_by && g_utf8_strlen(FileTagCur->encoded_by, -1)>0 )
    {
        FileTag->encoded_by = g_strdup(FileTagCur->encoded_by);
        g_strstrip (FileTag->encoded_by);
    } else
    {
        FileTag->encoded_by = NULL;
    }


    /* Picture */
    if(FileTagCur->picture)
    {
        if (FileTag->picture)
            Picture_Free(FileTag->picture);
        FileTag->picture = Picture_Copy(FileTagCur->picture);
    }else if (FileTag->picture)
    {
        Picture_Free(FileTag->picture);
        FileTag->picture = NULL;
    }

    return TRUE;
}


/*
 * Save data contained into File_Tag structure to the file on hard disk.
 */
gboolean
ET_Save_File_Tag_To_HD (ET_File *ETFile, GError **error)
{
    const ET_File_Description *ETFileDescription;
    gchar *cur_filename;
    gchar *cur_filename_utf8;
    gboolean state;
    GFile *file;
    GFileInfo *fileinfo;

    g_return_val_if_fail (ETFile != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    cur_filename      = ((File_Name *)(ETFile->FileNameCur)->data)->value;
    cur_filename_utf8 = ((File_Name *)(ETFile->FileNameCur)->data)->value_utf8;

    ETFileDescription = ETFile->ETFileDescription;

    /* Store the file timestamps (in case they are to be preserved) */
    file = g_file_new_for_path (cur_filename);
    fileinfo = g_file_query_info (file, "time::*", G_FILE_QUERY_INFO_NONE,
                                  NULL, NULL);

    switch (ETFileDescription->TagType)
    {
#ifdef ENABLE_MP3
        case ID3_TAG:
            state = Id3tag_Write_File_Tag(ETFile);
            break;
#endif
#ifdef ENABLE_OGG
        case OGG_TAG:
            state = ogg_tag_write_file_tag (ETFile, error);
            break;
#endif
#ifdef ENABLE_FLAC
        case FLAC_TAG:
            state = Flac_Tag_Write_File_Tag(ETFile);
            break;
#endif
        case APE_TAG:
            state = Ape_Tag_Write_File_Tag(ETFile);
            break;
#ifdef ENABLE_MP4
        case MP4_TAG:
            state = Mp4tag_Write_File_Tag(ETFile);
            break;
#endif
#ifdef ENABLE_WAVPACK
        case WAVPACK_TAG:
            state = Wavpack_Tag_Write_File_Tag(ETFile);
            break;
#endif
#ifdef ENABLE_OPUS
        case OPUS_TAG:
            state = ogg_tag_write_file_tag (ETFile, error);
            break;
#endif
        case UNKNOWN_TAG:
        default:
            Log_Print(LOG_ERROR,"Saving to HD: Undefined function for tag type '%d' (file %s).",
                ETFileDescription->TagType,cur_filename_utf8);
            state = FALSE;
            break;
    }

    /* Update properties for the file. */
    if (fileinfo)
    {
        if (g_settings_get_boolean (MainSettings,
                                    "file-preserve-modification-time"))
        {
            g_file_set_attributes_from_info (file, fileinfo,
                                             G_FILE_QUERY_INFO_NONE,
                                             NULL, NULL);
        }

        g_object_unref (fileinfo);
    }

    /* Update the stored file modification time to prevent EasyTAG from warning
     * that an external program has changed the file. */
    fileinfo = g_file_query_info (file,
                                  G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                  G_FILE_QUERY_INFO_NONE, NULL, NULL);

    if (fileinfo)
    {
        ETFile->FileModificationTime = g_file_info_get_attribute_uint64 (fileinfo,
                                                                         G_FILE_ATTRIBUTE_TIME_MODIFIED);
        g_object_unref (fileinfo);
    }

    g_object_unref (file);

    if (state==TRUE)
    {

        /* Update date and time of the parent directory of the file after
         * changing the tag value (ex: needed for Amarok for refreshing). Note
         * that when renaming a file the parent directory is automatically
         * updated.
         */
        if (g_settings_get_boolean (MainSettings,
                                    "file-update-parent-modification-time"))
        {
            gchar *path = g_path_get_dirname (cur_filename);
            utime (path, NULL);
            g_free (path);
        }

        ET_Mark_File_Tag_As_Saved(ETFile);
        return TRUE;
    }
    else
    {
        if (*error == NULL)
        {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_UNKNOWN,
                         _("Error writing tag type %d to file %s (%s)"),
                         ETFileDescription->TagType, cur_filename_utf8,
                         g_strerror (EIO));
        }

        return FALSE;
    }
}

/*
 * Function used to update path of filenames into list after renaming a parent directory
 * (for ex: "/mp3/old_path/file.mp3" to "/mp3/new_path/file.mp3"
 */
void ET_Update_Directory_Name_Into_File_List (gchar* last_path, gchar *new_path)
{
    GList *filelist;
    ET_File *file;
    GList *filenamelist;
    gchar *filename;
    gchar *last_path_tmp;

    if (!ETCore->ETFileList || !last_path || !new_path ||
        strlen(last_path) < 1 || strlen(new_path) < 1 )
        return;

    // Add '/' to end of path to avoid ambiguity between a directory and a filename...
    if (last_path[strlen(last_path)-1]==G_DIR_SEPARATOR)
        last_path_tmp = g_strdup(last_path);
    else
        last_path_tmp = g_strconcat(last_path,G_DIR_SEPARATOR_S,NULL);

    for (filelist = g_list_first (ETCore->ETFileList); filelist != NULL;
         filelist = g_list_next (filelist))
    {
        if ((file = filelist->data))
        {
            for (filenamelist = file->FileNameList; filenamelist != NULL;
                 filenamelist = g_list_next (filenamelist))
            {
                File_Name *FileName = (File_Name *)filenamelist->data;

                if ( FileName && (filename=FileName->value) )
                {
                    // Replace path of filename
                    if ( strncmp(filename,last_path_tmp,strlen(last_path_tmp))==0 )
                    {
                        gchar *filename_tmp;

                        // Create the new filename
                        filename_tmp = g_strconcat(new_path,
                                                   (new_path[strlen(new_path)-1]==G_DIR_SEPARATOR) ? "" : G_DIR_SEPARATOR_S,
                                                   &filename[strlen(last_path_tmp)],NULL);

                        /* Replace the filename (in file system encoding). */
                        g_free(FileName->value);
                        FileName->value = filename_tmp;
                        /* Replace the filename (in file system encoding). */
                        g_free(FileName->value_utf8);
                        FileName->value_utf8 = filename_to_display(FileName->value);
                        // Recalculate the collate key
                        g_free(FileName->value_ck);
                        FileName->value_ck = g_utf8_collate_key_for_filename(FileName->value_utf8, -1);
                    }
                }
             }
        }
    }

    g_free(last_path_tmp);
}



/***********************
 * Undo/Redo functions *
 ***********************/

/*
 * Check if 'FileName' and 'FileTag' differ with those of 'ETFile'.
 * Manage undo feature for the ETFile and the main undo list.
 */
gboolean ET_Manage_Changes_Of_File_Data (ET_File *ETFile, File_Name *FileName, File_Tag *FileTag)
{
    gboolean undo_added = FALSE;

    g_return_val_if_fail (ETFile != NULL, FALSE);

    /*
     * Detect changes of filename and generate the filename undo list
     */
    if (FileName)
    {
        if ( ETFile->FileNameNew && ET_Detect_Changes_Of_File_Name( (File_Name *)(ETFile->FileNameNew)->data,FileName )==TRUE )
        {
            ET_Add_File_Name_To_List(ETFile,FileName);
            undo_added |= TRUE;
        }else
        {
            ET_Free_File_Name_Item(FileName);
        }
    }

    /*
     * Detect changes in tag data and generate the tag undo list
     */
    if (FileTag)
    {
        if ( ETFile->FileTag && ET_Detect_Changes_Of_File_Tag( (File_Tag *)(ETFile->FileTag)->data,FileTag )==TRUE )
        {
            ET_Add_File_Tag_To_List(ETFile,FileTag);
            undo_added |= TRUE;
        }else
        {
            ET_Free_File_Tag_Item(FileTag);
        }
    }

    /*
     * Generate main undo (file history of modifications)
     */
    if (undo_added)
        ET_Add_File_To_History_List(ETFile);

    //return TRUE;
    return undo_added;
}


/*
 * Compares two File_Name items :
 *  - returns TRUE if there aren't the same
 *  - else returns FALSE
 */
static gboolean
ET_Detect_Changes_Of_File_Name (File_Name *FileName1, File_Name *FileName2)
{
    const gchar *filename1_ck;
    const gchar *filename2_ck;

    if (!FileName1 && !FileName2) return FALSE;
    if (FileName1 && !FileName2) return TRUE;
    if (!FileName1 && FileName2) return TRUE;

    /* Both FileName1 and FileName2 are != NULL. */
    if (!FileName1->value && !FileName2->value) return FALSE;
    if (FileName1->value && !FileName2->value) return TRUE;
    if (!FileName1->value && FileName2->value) return TRUE;

    /* Compare collate keys (with FileName->value converted to UTF-8 as it
     * contains raw data). */
    filename1_ck = FileName1->value_ck;
    filename2_ck = FileName2->value_ck;

    /* Filename changed ? (we check path + file). */
    if (strcmp (filename1_ck, filename2_ck) != 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*
 * Compares two File_Tag items and returns TRUE if there aren't the same.
 * Notes:
 *  - if field is '' or NULL => will be removed
 */
gboolean ET_Detect_Changes_Of_File_Tag (File_Tag *FileTag1, File_Tag *FileTag2)
{
    Picture *pic1;
    Picture *pic2;

    g_return_val_if_fail (FileTag1 != NULL && FileTag2 != NULL, FALSE);

    if ( ( FileTag1 && !FileTag2)
      || (!FileTag1 &&  FileTag2) )
        return TRUE;

    /* Title */
    if ( FileTag1->title && !FileTag2->title && g_utf8_strlen(FileTag1->title, -1)>0 ) return TRUE;
    if (!FileTag1->title &&  FileTag2->title && g_utf8_strlen(FileTag2->title, -1)>0 ) return TRUE;
    if ( FileTag1->title &&  FileTag2->title && g_utf8_collate(FileTag1->title,FileTag2->title)!=0 ) return TRUE;

    /* Artist */
    if ( FileTag1->artist && !FileTag2->artist && g_utf8_strlen(FileTag1->artist, -1)>0 ) return TRUE;
    if (!FileTag1->artist &&  FileTag2->artist && g_utf8_strlen(FileTag2->artist, -1)>0 ) return TRUE;
    if ( FileTag1->artist &&  FileTag2->artist && g_utf8_collate(FileTag1->artist,FileTag2->artist)!=0 ) return TRUE;

	/* Album Artist */
    if ( FileTag1->album_artist && !FileTag2->album_artist && g_utf8_strlen(FileTag1->album_artist, -1)>0 ) return TRUE;
    if (!FileTag1->album_artist &&  FileTag2->album_artist && g_utf8_strlen(FileTag2->album_artist, -1)>0 ) return TRUE;
    if ( FileTag1->album_artist &&  FileTag2->album_artist && g_utf8_collate(FileTag1->album_artist,FileTag2->album_artist)!=0 ) return TRUE;

    /* Album */
    if ( FileTag1->album && !FileTag2->album && g_utf8_strlen(FileTag1->album, -1)>0 ) return TRUE;
    if (!FileTag1->album &&  FileTag2->album && g_utf8_strlen(FileTag2->album, -1)>0 ) return TRUE;
    if ( FileTag1->album &&  FileTag2->album && g_utf8_collate(FileTag1->album,FileTag2->album)!=0 ) return TRUE;

    /* Disc Number */
    if ( FileTag1->disc_number && !FileTag2->disc_number && g_utf8_strlen(FileTag1->disc_number, -1)>0 ) return TRUE;
    if (!FileTag1->disc_number &&  FileTag2->disc_number && g_utf8_strlen(FileTag2->disc_number, -1)>0 ) return TRUE;
    if ( FileTag1->disc_number &&  FileTag2->disc_number && g_utf8_collate(FileTag1->disc_number,FileTag2->disc_number)!=0 ) return TRUE;

    /* Discs Total */
    if (FileTag1->disc_total && !FileTag2->disc_total
        && g_utf8_strlen (FileTag1->disc_total, -1) > 0)
    {
        return TRUE;
    }

    if (!FileTag1->disc_total &&  FileTag2->disc_total
        && g_utf8_strlen (FileTag2->disc_total, -1) > 0)
    {
        return TRUE;
    }

    if (FileTag1->disc_total &&  FileTag2->disc_total
        && g_utf8_collate (FileTag1->disc_total, FileTag2->disc_total) != 0)
    {
        return TRUE;
    }

    /* Year */
    if ( FileTag1->year && !FileTag2->year && g_utf8_strlen(FileTag1->year, -1)>0 ) return TRUE;
    if (!FileTag1->year &&  FileTag2->year && g_utf8_strlen(FileTag2->year, -1)>0 ) return TRUE;
    if ( FileTag1->year &&  FileTag2->year && g_utf8_collate(FileTag1->year,FileTag2->year)!=0 ) return TRUE;

    /* Track */
    if ( FileTag1->track && !FileTag2->track && g_utf8_strlen(FileTag1->track, -1)>0 ) return TRUE;
    if (!FileTag1->track &&  FileTag2->track && g_utf8_strlen(FileTag2->track, -1)>0 ) return TRUE;
    if ( FileTag1->track &&  FileTag2->track && g_utf8_collate(FileTag1->track,FileTag2->track)!=0 ) return TRUE;

    /* Track Total */
    if ( FileTag1->track_total && !FileTag2->track_total && g_utf8_strlen(FileTag1->track_total, -1)>0 ) return TRUE;
    if (!FileTag1->track_total &&  FileTag2->track_total && g_utf8_strlen(FileTag2->track_total, -1)>0 ) return TRUE;
    if ( FileTag1->track_total &&  FileTag2->track_total && g_utf8_collate(FileTag1->track_total,FileTag2->track_total)!=0 ) return TRUE;

    /* Genre */
    if ( FileTag1->genre && !FileTag2->genre && g_utf8_strlen(FileTag1->genre, -1)>0 ) return TRUE;
    if (!FileTag1->genre &&  FileTag2->genre && g_utf8_strlen(FileTag2->genre, -1)>0 ) return TRUE;
    if ( FileTag1->genre &&  FileTag2->genre && g_utf8_collate(FileTag1->genre,FileTag2->genre)!=0 ) return TRUE;

    /* Comment */
    if ( FileTag1->comment && !FileTag2->comment && g_utf8_strlen(FileTag1->comment, -1)>0 ) return TRUE;
    if (!FileTag1->comment &&  FileTag2->comment && g_utf8_strlen(FileTag2->comment, -1)>0 ) return TRUE;
    if ( FileTag1->comment &&  FileTag2->comment && g_utf8_collate(FileTag1->comment,FileTag2->comment)!=0 ) return TRUE;

    /* Composer */
    if ( FileTag1->composer && !FileTag2->composer && g_utf8_strlen(FileTag1->composer, -1)>0 ) return TRUE;
    if (!FileTag1->composer &&  FileTag2->composer && g_utf8_strlen(FileTag2->composer, -1)>0 ) return TRUE;
    if ( FileTag1->composer &&  FileTag2->composer && g_utf8_collate(FileTag1->composer,FileTag2->composer)!=0 ) return TRUE;

    /* Original artist */
    if ( FileTag1->orig_artist && !FileTag2->orig_artist && g_utf8_strlen(FileTag1->orig_artist, -1)>0 ) return TRUE;
    if (!FileTag1->orig_artist &&  FileTag2->orig_artist && g_utf8_strlen(FileTag2->orig_artist, -1)>0 ) return TRUE;
    if ( FileTag1->orig_artist &&  FileTag2->orig_artist && g_utf8_collate(FileTag1->orig_artist,FileTag2->orig_artist)!=0 ) return TRUE;

    /* Copyright */
    if ( FileTag1->copyright && !FileTag2->copyright && g_utf8_strlen(FileTag1->copyright, -1)>0 ) return TRUE;
    if (!FileTag1->copyright &&  FileTag2->copyright && g_utf8_strlen(FileTag2->copyright, -1)>0 ) return TRUE;
    if ( FileTag1->copyright &&  FileTag2->copyright && g_utf8_collate(FileTag1->copyright,FileTag2->copyright)!=0 ) return TRUE;

    /* URL */
    if ( FileTag1->url && !FileTag2->url && g_utf8_strlen(FileTag1->url, -1)>0 ) return TRUE;
    if (!FileTag1->url &&  FileTag2->url && g_utf8_strlen(FileTag2->url, -1)>0 ) return TRUE;
    if ( FileTag1->url &&  FileTag2->url && g_utf8_collate(FileTag1->url,FileTag2->url)!=0 ) return TRUE;

    /* Encoded by */
    if ( FileTag1->encoded_by && !FileTag2->encoded_by && g_utf8_strlen(FileTag1->encoded_by, -1)>0 ) return TRUE;
    if (!FileTag1->encoded_by &&  FileTag2->encoded_by && g_utf8_strlen(FileTag2->encoded_by, -1)>0 ) return TRUE;
    if ( FileTag1->encoded_by &&  FileTag2->encoded_by && g_utf8_collate(FileTag1->encoded_by,FileTag2->encoded_by)!=0 ) return TRUE;

    /* Picture */
    for (pic1 = FileTag1->picture, pic2 = FileTag2->picture; ;
         pic1 = pic1->next, pic2 = pic2->next)
    {
        if( (pic1 && !pic2) || (!pic1 && pic2) )
            return TRUE;
        if (!pic1 || !pic2)
            break; // => no changes
        //if (!pic1->data || !pic2->data)
        //    break; // => no changes

        if (!pic1->data && !pic2->data)
            return FALSE;
        if (pic1->data && !pic2->data)
            return TRUE;
        if (!pic1->data && pic2->data)
            return TRUE;
        if (pic1->size != pic2->size
        ||  memcmp(pic1->data, pic2->data, pic1->size))
            return TRUE;
        if (pic1->type != pic2->type)
            return TRUE;
        if (pic1->description && !pic2->description
        &&  g_utf8_strlen(pic1->description, -1)>0 )
            return TRUE;
        if (!pic1->description && pic2->description
        &&  g_utf8_strlen(pic2->description, -1)>0 )
            return TRUE;
        if (pic1->description && pic2->description
        &&  g_utf8_collate(pic1->description, pic2->description)!=0 )
            return TRUE;
    }

    return FALSE; /* No changes */
}


/*
 * Add a FileName item to the history list of ETFile
 */
static gboolean
ET_Add_File_Name_To_List (ET_File *ETFile, File_Name *FileName)
{
    GList *cut_list = NULL;

    g_return_val_if_fail (ETFile != NULL && FileName != NULL, FALSE);

    /* How it works : Cut the FileNameList list after the current item,
     * and appends it to the FileNameListBak list for saving the data.
     * Then appends the new item to the FileNameList list */
    if (ETFile->FileNameList)
    {
        cut_list = ETFile->FileNameNew->next; // Cut after the current item...
        ETFile->FileNameNew->next = NULL;
    }
    if (cut_list)
        cut_list->prev = NULL;

    /* Add the new item to the list */
    ETFile->FileNameList = g_list_append(ETFile->FileNameList,FileName);
    /* Set the current item to use */
    ETFile->FileNameNew  = g_list_last(ETFile->FileNameList);
    /* Backup list */
    /* FIX ME! Keep only the saved item */
    ETFile->FileNameListBak = g_list_concat(ETFile->FileNameListBak,cut_list);

    return TRUE;
}

/*
 * Add a FileTag item to the history list of ETFile
 */
static gboolean
ET_Add_File_Tag_To_List (ET_File *ETFile, File_Tag *FileTag)
{
    GList *cut_list = NULL;

    g_return_val_if_fail (ETFile != NULL && FileTag != NULL, FALSE);

    if (ETFile->FileTag)
    {
        cut_list = ETFile->FileTag->next; // Cut after the current item...
        ETFile->FileTag->next = NULL;
    }
    if (cut_list)
        cut_list->prev = NULL;

    /* Add the new item to the list */
    ETFile->FileTagList = g_list_append(ETFile->FileTagList,FileTag);
    /* Set the current item to use */
    ETFile->FileTag     = g_list_last(ETFile->FileTagList);
    /* Backup list */
    ETFile->FileTagListBak = g_list_concat(ETFile->FileTagListBak,cut_list);

    return TRUE;
}

/*
 * Add a ETFile item to the main undo list of files
 */
static gboolean
ET_Add_File_To_History_List (ET_File *ETFile)
{
    ET_History_File *ETHistoryFile;

    g_return_val_if_fail (ETFile != NULL, FALSE);

    ETHistoryFile = g_malloc0(sizeof(ET_History_File));
    ETHistoryFile->ETFile = ETFile;

    /* The undo list must contains one item before the 'first undo' data */
    if (!ETCore->ETHistoryFileList)
        ETCore->ETHistoryFileList = g_list_append(ETCore->ETHistoryFileList,g_malloc0(sizeof(ET_History_File)));

    /* Add the item to the list (cut end of list from the current element) */
    ETCore->ETHistoryFileList = g_list_append(ETCore->ETHistoryFileList,ETHistoryFile);
    ETCore->ETHistoryFileList = g_list_last(ETCore->ETHistoryFileList);

    return TRUE;
}


/*
 * Applies one undo to the ETFile data (to reload the previous data).
 * Returns TRUE if an undo had been applied.
 */
gboolean ET_Undo_File_Data (ET_File *ETFile)
{
    gboolean has_filename_undo_data = FALSE;
    gboolean has_filetag_undo_data  = FALSE;
    guint    filename_key, filetag_key, undo_key;

    g_return_val_if_fail (ETFile != NULL, FALSE);

    /* Find the valid key */
    if (ETFile->FileNameNew->prev && ETFile->FileNameNew->data)
        filename_key = ((File_Name *)ETFile->FileNameNew->data)->key;
    else
        filename_key = 0;
    if (ETFile->FileTag->prev && ETFile->FileTag->data)
        filetag_key = ((File_Tag *)ETFile->FileTag->data)->key;
    else
        filetag_key = 0;
    // The key to use
    undo_key = MAX(filename_key,filetag_key);

    /* Undo filename */
    if (ETFile->FileNameNew->prev && ETFile->FileNameNew->data
    && (undo_key==((File_Name *)ETFile->FileNameNew->data)->key))
    {
        ETFile->FileNameNew = ETFile->FileNameNew->prev;
        has_filename_undo_data = TRUE; // To indicate that an undo has been applied
    }

    /* Undo tag data */
    if (ETFile->FileTag->prev && ETFile->FileTag->data
    && (undo_key==((File_Tag *)ETFile->FileTag->data)->key))
    {
        ETFile->FileTag = ETFile->FileTag->prev;
        has_filetag_undo_data  = TRUE;
    }

    return has_filename_undo_data | has_filetag_undo_data;
}


/*
 * Returns TRUE if file contains undo data (filename or tag)
 */
gboolean ET_File_Data_Has_Undo_Data (ET_File *ETFile)
{
    gboolean has_filename_undo_data = FALSE;
    gboolean has_filetag_undo_data  = FALSE;

    g_return_val_if_fail (ETFile != NULL, FALSE);

    if (ETFile->FileNameNew && ETFile->FileNameNew->prev) has_filename_undo_data = TRUE;
    if (ETFile->FileTag && ETFile->FileTag->prev)         has_filetag_undo_data  = TRUE;

    return has_filename_undo_data | has_filetag_undo_data;
}


/*
 * Applies one redo to the ETFile data. Returns TRUE if a redo had been applied.
 */
gboolean ET_Redo_File_Data (ET_File *ETFile)
{
    gboolean has_filename_redo_data = FALSE;
    gboolean has_filetag_redo_data  = FALSE;
    guint    filename_key, filetag_key, undo_key;

    g_return_val_if_fail (ETFile != NULL, FALSE);

    /* Find the valid key */
    if (ETFile->FileNameNew->next && ETFile->FileNameNew->next->data)
        filename_key = ((File_Name *)ETFile->FileNameNew->next->data)->key;
    else
        filename_key = (guint)~0; // To have the max value for guint
    if (ETFile->FileTag->next && ETFile->FileTag->next->data)
        filetag_key = ((File_Tag *)ETFile->FileTag->next->data)->key;
    else
        filetag_key = (guint)~0; // To have the max value for guint
    // The key to use
    undo_key = MIN(filename_key,filetag_key);

    /* Redo filename */
    if (ETFile->FileNameNew->next && ETFile->FileNameNew->next->data
    && (undo_key==((File_Name *)ETFile->FileNameNew->next->data)->key))
    {
        ETFile->FileNameNew = ETFile->FileNameNew->next;
        has_filename_redo_data = TRUE; // To indicate that a redo has been applied
    }

    /* Redo tag data */
    if (ETFile->FileTag->next && ETFile->FileTag->next->data
    && (undo_key==((File_Tag *)ETFile->FileTag->next->data)->key))
    {
        ETFile->FileTag = ETFile->FileTag->next;
        has_filetag_redo_data  = TRUE;
    }

    return has_filename_redo_data | has_filetag_redo_data;
}


/*
 * Returns TRUE if file contains redo data (filename or tag)
 */
gboolean ET_File_Data_Has_Redo_Data (ET_File *ETFile)
{
    gboolean has_filename_redo_data = FALSE;
    gboolean has_filetag_redo_data  = FALSE;

    g_return_val_if_fail (ETFile != NULL, FALSE);

    if (ETFile->FileNameNew && ETFile->FileNameNew->next) has_filename_redo_data = TRUE;
    if (ETFile->FileTag && ETFile->FileTag->next)         has_filetag_redo_data  = TRUE;

    return has_filename_redo_data | has_filetag_redo_data;
}


/*
 * Execute one 'undo' in the main undo list (it selects the last ETFile changed,
 * before to apply an undo action)
 */
ET_File *ET_Undo_History_File_Data (void)
{
    ET_File *ETFile;
    ET_History_File *ETHistoryFile;

    g_return_val_if_fail (ETCore->ETHistoryFileList != NULL ||
                          ET_History_File_List_Has_Undo_Data (), NULL);

    ETHistoryFile = (ET_History_File *)ETCore->ETHistoryFileList->data;
    ETFile        = (ET_File *)ETHistoryFile->ETFile;
    ET_Displayed_File_List_By_Etfile(ETFile);
    ET_Undo_File_Data(ETFile);

    if (ETCore->ETHistoryFileList->prev)
        ETCore->ETHistoryFileList = ETCore->ETHistoryFileList->prev;
    return ETFile;
}


/*
 * Returns TRUE if undo file list contains undo data
 */
gboolean ET_History_File_List_Has_Undo_Data (void)
{
    if (ETCore->ETHistoryFileList && ETCore->ETHistoryFileList->prev)
        return TRUE;
    else
        return FALSE;
}


/*
 * Execute one 'redo' in the main undo list
 */
ET_File *ET_Redo_History_File_Data (void)
{
    ET_File *ETFile;
    ET_History_File *ETHistoryFile;

    if (!ETCore->ETHistoryFileList || !ET_History_File_List_Has_Redo_Data()) return NULL;

    ETHistoryFile = (ET_History_File *)ETCore->ETHistoryFileList->next->data;
    ETFile        = (ET_File *)ETHistoryFile->ETFile;
    ET_Displayed_File_List_By_Etfile(ETFile);
    ET_Redo_File_Data(ETFile);

    if (ETCore->ETHistoryFileList->next)
        ETCore->ETHistoryFileList = ETCore->ETHistoryFileList->next;
    return ETFile;
}


/*
 * Returns TRUE if undo file list contains redo data
 */
gboolean ET_History_File_List_Has_Redo_Data (void)
{
    if (ETCore->ETHistoryFileList && ETCore->ETHistoryFileList->next)
        return TRUE;
    else
        return FALSE;
}




/**********************
 * Checking functions *
 **********************/


/*
 * Ckecks if the current files had been changed but not saved.
 * Returns TRUE if the file has been saved.
 * Returns FALSE if some changes haven't been saved.
 */
gboolean ET_Check_If_File_Is_Saved (ET_File *ETFile)
{
    File_Tag  *FileTag     = NULL;
    File_Name *FileNameNew = NULL;

    g_return_val_if_fail (ETFile != NULL, TRUE);

    if (ETFile->FileTag)
        FileTag     = ETFile->FileTag->data;
    if (ETFile->FileNameNew)
        FileNameNew = ETFile->FileNameNew->data;

    // Check if the tag has been changed
    if ( FileTag && FileTag->saved != TRUE )
        return FALSE;

    // Check if name of file has been changed
    if ( FileNameNew && FileNameNew->saved != TRUE )
        return FALSE;

    // No changes
    return TRUE;
}


/*
 * Ckecks if some files, in the list, had been changed but not saved.
 * Returns TRUE if all files have been saved.
 * Returns FALSE if some changes haven't been saved.
 */
gboolean ET_Check_If_All_Files_Are_Saved (void)
{
    /* Check if some files haven't been saved, if didn't nochange=0 */
    if (!ETCore->ETFileList)
    {
        return TRUE;
    }else
    {
        GList *l;

        for (l = g_list_first (ETCore->ETFileList); l != NULL;
             l = g_list_next (l))
        {
            if (ET_Check_If_File_Is_Saved ((ET_File *)l->data) == FALSE)
                return FALSE;
        }
        return TRUE;
    }
}




/*******************
 * Extra functions *
 *******************/

/*
 * Set to TRUE the value of 'FileTag->saved' for the File_Tag item passed in parameter.
 * And set ALL other values of the list to FALSE.
 */
static void
Set_Saved_Value_Of_File_Tag (File_Tag *FileTag, gboolean saved)
{
    if (FileTag) FileTag->saved = saved;
}

static void
ET_Mark_File_Tag_As_Saved (ET_File *ETFile)
{
    File_Tag *FileTag;
    GList *FileTagList;

    FileTag     = (File_Tag *)ETFile->FileTag->data;    // The current FileTag, to set to TRUE
    FileTagList = ETFile->FileTagList;
    g_list_foreach(FileTagList,(GFunc)Set_Saved_Value_Of_File_Tag,FALSE); // All other FileTag set to FALSE
    FileTag->saved = TRUE; // The current FileTag set to TRUE
}


void ET_Mark_File_Name_As_Saved (ET_File *ETFile)
{
    File_Name *FileNameNew;
    GList *FileNameList;

    FileNameNew  = (File_Name *)ETFile->FileNameNew->data;    // The current FileName, to set to TRUE
    FileNameList = ETFile->FileNameList;
    g_list_foreach(FileNameList,(GFunc)Set_Saved_Value_Of_File_Tag,FALSE);
    FileNameNew->saved = TRUE;
}


/*
 * et_core_read_file_info:
 * @filename: (type filename): a file from which to read information
 * @ETFileInfo: (out caller-allocates): a file information structure
 * @error: a #GError to provide information on erros, or %NULL to ignore
 *
 * Fille @ETFileInfo with information about the file. Currently, this only
 * fills the file size.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
static gboolean
et_core_read_file_info (const gchar *filename, ET_File_Info *ETFileInfo,
                        GError **error)
{
    GFile *file;
    GFileInfo *info;

    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
    g_return_val_if_fail (filename != NULL && ETFileInfo != NULL, FALSE);

    file = g_file_new_for_path (filename);
    info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_SIZE,
                              G_FILE_QUERY_INFO_NONE, NULL, error);

    if (!info)
    {
        g_assert (error == NULL || *error != NULL);
        g_object_unref (file);
        return FALSE;
    }

    ETFileInfo->version    = 0;
    ETFileInfo->bitrate    = 0;
    ETFileInfo->samplerate = 0;
    ETFileInfo->mode       = 0;
    ETFileInfo->size = g_file_info_get_size (info);
    ETFileInfo->duration   = 0;

    g_assert (error == NULL || *error == NULL);
    g_object_unref (info);
    g_object_unref (file);
    return TRUE;
}

/*
 * This function generates a new filename using path of the old file and the
 * new name.
 * - ETFile -> old_file_name : "/path_to_file/old_name.ext"
 * - new_file_name_utf8      : "new_name.ext"
 * Returns "/path_to_file/new_name.ext" into allocated data (in UTF-8)!
 * Notes :
 *   - filenames (basemane) musn't exceed 255 characters (i.e. : "new_name.ext")
 *   - ogg filename musn't exceed 255-6 characters as we use mkstemp
 */
#if 1
gchar *ET_File_Name_Generate (ET_File *ETFile, gchar *new_file_name_utf8)
{
    gchar *dirname_utf8;

    if (ETFile && ETFile->FileNameNew->data && new_file_name_utf8
    && (dirname_utf8=g_path_get_dirname(((File_Name *)ETFile->FileNameNew->data)->value_utf8)) )
    {
        gchar *extension;
        gchar *new_file_name_path_utf8;

        // Convert filename extension (lower/upper)
        extension = ET_File_Name_Format_Extension(ETFile);

        // Check length of filename (limit ~255 characters)
        //ET_File_Name_Check_Length(ETFile,new_file_name_utf8);

        // If filemame starts with /, it's a full filename with path but without extension
        if (g_path_is_absolute(new_file_name_utf8))
        {
            // We just add the extension
            new_file_name_path_utf8 = g_strconcat(new_file_name_utf8,extension,NULL);
        }else
        {
            // New path (with filename)
            if ( strcmp(dirname_utf8,G_DIR_SEPARATOR_S)==0 ) // Root directory?
                new_file_name_path_utf8 = g_strconcat(dirname_utf8,new_file_name_utf8,extension,NULL);
            else
                new_file_name_path_utf8 = g_strconcat(dirname_utf8,G_DIR_SEPARATOR_S,new_file_name_utf8,extension,NULL);
        }

        g_free (dirname_utf8);
        g_free(extension);
        return new_file_name_path_utf8; // in UTF-8
    }else
    {
        return NULL;
    }
}
#else
/* FOR TESTING */
/* Returns filename in file system encoding */
gchar *ET_File_Name_Generate (ET_File *ETFile, gchar *new_file_name_utf8)
{
    gchar *dirname;

    if (ETFile && ETFile->FileNameNew->data && new_file_name_utf8
    && (dirname=g_path_get_dirname(((File_Name *)ETFile->FileNameNew->data)->value)) )
    {
        gchar *extension;
        gchar *new_file_name_path;
        gchar *new_file_name;

        new_file_name = filename_from_display(new_file_name_utf8);

        // Convert filename extension (lower/upper)
        extension = ET_File_Name_Format_Extension(ETFile);

        // Check length of filename (limit ~255 characters)
        //ET_File_Name_Check_Length(ETFile,new_file_name_utf8);

        // If filemame starts with /, it's a full filename with path but without extension
        if (g_path_is_absolute(new_file_name))
        {
            // We just add the extension
            new_file_name_path = g_strconcat(new_file_name,extension,NULL);
        }else
        {
            // New path (with filename)
            if ( strcmp(dirname,G_DIR_SEPARATOR_S)==0 ) // Root directory?
                new_file_name_path = g_strconcat(dirname,new_file_name,extension,NULL);
            else
                new_file_name_path = g_strconcat(dirname,G_DIR_SEPARATOR_S,new_file_name,extension,NULL);
        }

        g_free(dirname);
        g_free(new_file_name);
        g_free(extension);
        return new_file_name_path; // in file system encoding
    }else
    {
        return NULL;
    }
}
#endif


/* Convert filename extension (lower/upper/no change). */
static gchar *
ET_File_Name_Format_Extension (ET_File *ETFile)
{
    EtFilenameExtensionMode mode;

    mode = g_settings_get_enum (MainSettings, "rename-extension-mode");

    switch (mode)
    {
        /* FIXME: Should use filename encoding, not UTF-8! */
        case ET_FILENAME_EXTENSION_LOWER_CASE:
            return g_utf8_strdown (ETFile->ETFileDescription->Extension, -1);
        case ET_FILENAME_EXTENSION_UPPER_CASE:
            return g_utf8_strup (ETFile->ETFileDescription->Extension, -1);
        case ET_FILENAME_EXTENSION_NO_CHANGE:
        default:
            return g_strdup (ETFile->ETFileExtension);
    };
}


/*
 * Used to replace the illegal characters in the filename
 * Paremeter 'filename' musn't contain the path, else directories separators would be replaced!
 */
gboolean ET_File_Name_Convert_Character (gchar *filename_utf8)
{
    gchar *character;

    g_return_val_if_fail (filename_utf8 != NULL, FALSE);

    // Convert automatically the directory separator ('/' on LINUX and '\' on WIN32) to '-'.
    while ( (character=g_utf8_strchr(filename_utf8, -1, G_DIR_SEPARATOR))!=NULL )
        *character = '-';

#ifdef G_OS_WIN32
    /* Convert character '\' on WIN32 to '-'. */
    while ( (character=g_utf8_strchr(filename_utf8, -1, '\\'))!=NULL )
        *character = '-';
    /* Convert character '/' on WIN32 to '-'. May be converted to '\' after. */
    while ( (character=g_utf8_strchr(filename_utf8, -1, '/'))!=NULL )
        *character = '-';
#endif /* G_OS_WIN32 */

    /* Convert other illegal characters on FAT32/16 filesystems and ISO9660 and
     * Joliet (CD-ROM filesystems). */
    if (g_settings_get_boolean (MainSettings, "rename-replace-illegal-chars"))
    {
        // Commented as we display unicode values as "\351" for "é"
        //while ( (character=g_utf8_strchr(filename_utf8, -1, '\\'))!=NULL )
        //    *character = ',';
        while ( (character=g_utf8_strchr(filename_utf8, -1, ':'))!=NULL )
            *character = '-';
        //while ( (character=g_utf8_strchr(filename_utf8, -1, ';'))!=NULL )
        //    *character = '-';
        while ( (character=g_utf8_strchr(filename_utf8, -1, '*'))!=NULL )
            *character = '+';
        while ( (character=g_utf8_strchr(filename_utf8, -1, '?'))!=NULL )
            *character = '_';
        while ( (character=g_utf8_strchr(filename_utf8, -1, '\"'))!=NULL )
            *character = '\'';
        while ( (character=g_utf8_strchr(filename_utf8, -1, '<'))!=NULL )
            *character = '(';
        while ( (character=g_utf8_strchr(filename_utf8, -1, '>'))!=NULL )
            *character = ')';
        while ( (character=g_utf8_strchr(filename_utf8, -1, '|'))!=NULL )
            *character = '-';
    }
    return TRUE;
}


/*
 * Returns the number of file in the directory of the selected file.
 * Parameter "path" should be in UTF-8
 */
guint
ET_Get_Number_Of_Files_In_Directory (const gchar *path_utf8)
{
    GList *l;
    guint  count = 0;

    g_return_val_if_fail (path_utf8 != NULL, count);

    for (l = g_list_first (ETCore->ETFileList); l != NULL; l = g_list_next (l))
    {
        ET_File *ETFile = (ET_File *)l->data;
        gchar *cur_filename_utf8 = ((File_Name *)((GList *)ETFile->FileNameCur)->data)->value_utf8;
        gchar *dirname_utf8      = g_path_get_dirname(cur_filename_utf8);

        if (g_utf8_collate(dirname_utf8, path_utf8) == 0)
            count++;

        g_free(dirname_utf8 );
    }

    return count;
}

/*
 * Set appropriate sort order for the given column_id
 */
static void
set_sort_order_for_column_id (gint column_id, GtkTreeViewColumn *column,
                              EtSortMode sort_type)
{
    EtApplicationWindow *window;
    EtSortMode current_mode;

    window = ET_APPLICATION_WINDOW (MainWindow);

    /* Removing the sort indicator for the currently selected treeview
     * column. */
    current_mode = g_settings_get_enum (MainSettings, "sort-mode");

    if (current_mode < ET_SORT_MODE_ASCENDING_CREATION_DATE)
    {
        gtk_tree_view_column_set_sort_indicator (et_application_window_browser_get_column_for_column_id (window, current_mode / 2),
                                                 FALSE);
    }

    if (sort_type < ET_SORT_MODE_ASCENDING_CREATION_DATE)
    {
        gtk_tree_view_column_clicked (et_application_window_browser_get_column_for_column_id (window, column_id));

        if (sort_type % 2 == 0)
        {
            /* GTK_SORT_ASCENDING */
            if (et_application_window_browser_get_sort_order_for_column_id (window, column_id)
                == GTK_SORT_DESCENDING)
            {
                gtk_tree_view_column_set_sort_order (column,
                                                     GTK_SORT_ASCENDING);
            }
        }
        else
        {
            /* GTK_SORT_DESCENDING */
            if (et_application_window_browser_get_sort_order_for_column_id (window, column_id)
                == GTK_SORT_ASCENDING)
            {
                gtk_tree_view_column_set_sort_order (column,
                                                     GTK_SORT_DESCENDING);
            }
        }
    }
}
