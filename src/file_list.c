/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014-2016  David King <amigadave@amigadave.com>
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

#include "config.h"

#include "file_list.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "application_window.h"
#include "charset.h"
#include "easytag.h"
#include "log.h"
#include "misc.h"
#include "mpeg_header.h"
#include "monkeyaudio_header.h"
#include "musepack_header.h"
#include "picture.h"
#include "ape_tag.h"
#ifdef ENABLE_MP3
#include "id3_tag.h"
#endif
#ifdef ENABLE_OGG
#include "ogg_header.h"
#include "ogg_tag.h"
#endif
#ifdef ENABLE_FLAC
#include "flac_header.h"
#include "flac_tag.h"
#endif
#ifdef ENABLE_MP4
#include "mp4_header.h"
#include "mp4_tag.h"
#endif
#ifdef ENABLE_WAVPACK
#include "wavpack_header.h"
#include "wavpack_tag.h"
#endif
#ifdef ENABLE_OPUS
#include "opus_header.h"
#include "opus_tag.h"
#endif

/*
 * et_file_list_free:
 * @file_list: (element-type ET_File) (allow-none): a list of files
 *
 * Frees the full list of files.
 */
void
et_file_list_free (GList *file_list)
{
    g_return_if_fail (file_list != NULL);

    g_list_free_full (file_list, (GDestroyNotify)ET_Free_File_List_Item);
}

static void
et_history_file_free (ET_History_File *file)
{
    g_slice_free (ET_History_File, file);
}

/*
 * History list contains only pointers, so no data to free except the history structure.
 */
void
et_history_file_list_free (GList *file_list)
{
    g_return_if_fail (file_list != NULL);

    /* et_history_list_add() sets the list to the final element, so explicitly
     * go back to the start. */
    g_list_free_full (g_list_first (file_list),
                      (GDestroyNotify)et_history_file_free);
}

/*
 * "Display" list contains only pointers, so NOTHING to free
 */
void
et_displayed_file_list_free (GList *file_list)
{
}

/*
 * ArtistAlbum list contains 3 levels of lists
 */
void
et_artist_album_file_list_free (GList *file_list)
{
    GList *l;

    g_return_if_fail (file_list != NULL);

    /* Pointers are stored inside the artist/album list-stores, so free them
     * first. */
    et_application_window_browser_clear_artist_model (ET_APPLICATION_WINDOW (MainWindow));
    et_application_window_browser_clear_album_model (ET_APPLICATION_WINDOW (MainWindow));

    for (l = file_list; l != NULL; l = g_list_next (l))
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

    g_list_free (file_list);
}

/* Key for each item of ETFileList */
static guint
ET_File_Key_New (void)
{
    static guint ETFileKey = 0;
    return ++ETFileKey;
}

/*
 * et_core_read_file_info:
 * @file: a file from which to read information
 * @ETFileInfo: (out caller-allocates): a file information structure
 * @error: a #GError to provide information on erros, or %NULL to ignore
 *
 * Fille @ETFileInfo with information about the file. Currently, this only
 * fills the file size.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
static gboolean
et_core_read_file_info (GFile *file,
                        ET_File_Info *ETFileInfo,
                        GError **error)
{
    GFileInfo *info;

    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
    g_return_val_if_fail (file != NULL && ETFileInfo != NULL, FALSE);

    info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_SIZE,
                              G_FILE_QUERY_INFO_NONE, NULL, error);

    if (!info)
    {
        g_assert (error == NULL || *error != NULL);
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

    return TRUE;
}

/*
 * et_file_list_add:
 * Add a file to the "main" list. And get all information of the file.
 * The filename passed in should be in raw format, only convert it to UTF8 when
 * displaying it.
 */
GList *
et_file_list_add (GList *file_list,
                  GFile *file)
{
    GList *result;
    const ET_File_Description *description;
    ET_File      *ETFile;
    File_Name    *FileName;
    File_Tag     *FileTag;
    ET_File_Info *ETFileInfo;
    gchar        *ETFileExtension;
    guint         ETFileKey;
    guint         undo_key;
    GFileInfo *fileinfo;
    gchar *filename;
    gchar *display_path;
    GError *error = NULL;
    gboolean success;

    g_return_val_if_fail (file != NULL, file_list);

    /* Primary Key for this file */
    ETFileKey = ET_File_Key_New();

    /* Get description of the file */
    filename = g_file_get_path (file);
    display_path = g_filename_display_name (filename);
    description = ET_Get_File_Description (filename);

    /* Get real extension of the file (keeping the case) */
    ETFileExtension = g_strdup(ET_Get_File_Extension(filename));

    /* Fill the File_Name structure for FileNameList */
    FileName = et_file_name_new ();
    FileName->saved      = TRUE;    /* The file hasn't been changed, so it's saved */
    ET_Set_Filename_File_Name_Item (FileName, display_path, filename);

    /* Fill the File_Tag structure for FileTagList */
    FileTag = et_file_tag_new ();
    FileTag->saved = TRUE;    /* The file hasn't been changed, so it's saved */

    switch (description->TagType)
    {
#ifdef ENABLE_MP3
        case ID3_TAG:
            if (!id3tag_read_file_tag (file, FileTag, &error))
            {
                Log_Print (LOG_ERROR,
                           _("Error reading ID3 tag from file ‘%s’: %s"),
                           display_path, error->message);
                g_clear_error (&error);
            }
            break;
#endif
#ifdef ENABLE_OGG
        case OGG_TAG:
            if (!ogg_tag_read_file_tag (file, FileTag, &error))
            {
                Log_Print (LOG_ERROR,
                           _("Error reading tag from Ogg file ‘%s’: %s"),
                           display_path, error->message);
                g_clear_error (&error);
            }
            break;
#endif
#ifdef ENABLE_FLAC
        case FLAC_TAG:
            if (!flac_tag_read_file_tag (file, FileTag, &error))
            {
                Log_Print (LOG_ERROR,
                           _("Error reading tag from FLAC file ‘%s’: %s"),
                           display_path, error->message);
                g_clear_error (&error);
            }
            break;
#endif
        case APE_TAG:
            if (!ape_tag_read_file_tag (file, FileTag, &error))
            {
                Log_Print (LOG_ERROR,
                           _("Error reading APE tag from file ‘%s’: %s"),
                           display_path, error->message);
                g_clear_error (&error);
            }
            break;
#ifdef ENABLE_MP4
        case MP4_TAG:
            if (!mp4tag_read_file_tag (file, FileTag, &error))
            {
                Log_Print (LOG_ERROR,
                           _("Error reading tag from MP4 file ‘%s’: %s"),
                           display_path, error->message);
                g_clear_error (&error);
            }
            break;
#endif
#ifdef ENABLE_WAVPACK
        case WAVPACK_TAG:
            if (!wavpack_tag_read_file_tag (file, FileTag, &error))
            {
                Log_Print (LOG_ERROR,
                           _("Error reading tag from WavPack file ‘%s’: %s"),
                           display_path, error->message);
                g_clear_error (&error);
            }
        break;
#endif
#ifdef ENABLE_OPUS
        case OPUS_TAG:
            if (!et_opus_tag_read_file_tag (file, FileTag, &error))
            {
                Log_Print (LOG_ERROR,
                           _("Error reading tag from Opus file ‘%s’: %s"),
                           display_path, error->message);
                g_clear_error (&error);
            }
            break;
#endif
#ifndef ENABLE_MP3
        case ID3_TAG:
#endif
#ifndef ENABLE_OGG
        case OGG_TAG:
#endif
#ifndef ENABLE_FLAC
        case FLAC_TAG:
#endif
#ifndef ENABLE_MP4
        case MP4_TAG:
#endif
#ifndef ENABLE_WAVPACK
        case WAVPACK_TAG:
#endif
#ifndef ENABLE_OPUS
        case OPUS_TAG:
#endif
        case UNKNOWN_TAG:
        default:
            /* FIXME: Translatable string. */
            Log_Print (LOG_ERROR,
                       "FileTag: Undefined tag type (%d) for file %s",
                       (gint)description->TagType, display_path);
            break;
    }

    if (FileTag->year && g_utf8_strlen (FileTag->year, -1) > 4)
    {
        Log_Print (LOG_WARNING,
                   _("The year value ‘%s’ seems to be invalid in file ‘%s’. The information will be lost when saving"),
                   FileTag->year, display_path);
    }

    /* Fill the ET_File_Info structure */
    ETFileInfo = et_file_info_new ();

    switch (description->FileType)
    {
#if defined ENABLE_MP3 && defined ENABLE_ID3LIB
        case MP3_FILE:
        case MP2_FILE:
            success = et_mpeg_header_read_file_info (file, ETFileInfo, &error);
            break;
#endif
#ifdef ENABLE_OGG
        case OGG_FILE:
            success = et_ogg_header_read_file_info (file, ETFileInfo, &error);
            break;
#endif
#ifdef ENABLE_SPEEX
        case SPEEX_FILE:
            success = et_speex_header_read_file_info (file, ETFileInfo,
                                                      &error);
            break;
#endif
#ifdef ENABLE_FLAC
        case FLAC_FILE:
            success = et_flac_header_read_file_info (file, ETFileInfo, &error);
            break;
#endif
        case MPC_FILE:
            success = et_mpc_header_read_file_info (file, ETFileInfo, &error);
            break;
        case MAC_FILE:
            success = et_mac_header_read_file_info (file, ETFileInfo, &error);
            break;
#ifdef ENABLE_WAVPACK
        case WAVPACK_FILE:
            success = et_wavpack_header_read_file_info (file, ETFileInfo,
                                                        &error);
            break;
#endif
#ifdef ENABLE_MP4
        case MP4_FILE:
            success = et_mp4_header_read_file_info (file, ETFileInfo, &error);
            break;
#endif
#ifdef ENABLE_OPUS
        case OPUS_FILE:
            success = et_opus_read_file_info (file, ETFileInfo, &error);
            break;
#endif
        case OFR_FILE:
#if !defined ENABLE_MP3 && !defined ENABLE_ID3LIB
        case MP3_FILE:
        case MP2_FILE:
#endif
#ifndef ENABLE_OGG
        case OGG_FILE:
#endif
#ifndef ENABLE_SPEEX
        case SPEEX_FILE:
#endif
#ifndef ENABLE_FLAC
        case FLAC_FILE:
#endif
#ifndef ENABLE_MP4
        case MP4_FILE:
#endif
#ifndef ENABLE_WAVPACK
        case WAVPACK_FILE:
#endif
#ifndef ENABLE_OPUS
        case OPUS_FILE:
#endif
        case UNKNOWN_FILE:
        default:
            /* FIXME: Translatable string. */
            Log_Print (LOG_ERROR,
                       "ETFileInfo: Undefined file type (%d) for file %s",
                       (gint)description->FileType, display_path);
            /* To get at least the file size. */
            success = et_core_read_file_info (file, ETFileInfo, &error);
            break;
    }

    if (!success)
    {
        Log_Print (LOG_ERROR,
                   _("Error while querying information for file ‘%s’: %s"),
                   display_path, error->message);
        g_error_free (error);
    }

    /* Store the modification time of the file to check if the file was changed
     * before saving */
    fileinfo = g_file_query_info (file, G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                  G_FILE_QUERY_INFO_NONE, NULL, NULL);

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
    ETFile->ETFileDescription    = description;
    ETFile->ETFileExtension      = ETFileExtension;
    ETFile->FileNameList         = g_list_append(NULL,FileName);
    ETFile->FileNameCur          = ETFile->FileNameList;
    ETFile->FileNameNew          = ETFile->FileNameList;
    ETFile->FileTagList          = g_list_append(NULL,FileTag);
    ETFile->FileTag              = ETFile->FileTagList;
    ETFile->ETFileInfo           = ETFileInfo;

    /* Add the item to the "main list" */
    result = g_list_append (file_list, ETFile);


    /*
     * Process the filename and tag to generate undo if needed...
     * The undo key must be the same for FileName and FileTag => changed in the same time
     */
    undo_key = et_undo_key_new ();

    FileName = et_file_name_new ();
    FileName->key = undo_key;
    ET_Save_File_Name_Internal(ETFile,FileName);

    FileTag = et_file_tag_new ();
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
        Log_Print (LOG_INFO, _("Automatic corrections applied for file ‘%s’"),
                   display_path);
    }

    /* Add the item to the ArtistAlbum list (placed here to take advantage of previous changes) */
    //ET_Add_File_To_Artist_Album_File_List(ETFile);

    //ET_Debug_Print_File_List(ETCore->ETFileList,__FILE__,__LINE__,__FUNCTION__);

    g_free (filename);
    g_free (display_path);

    return result;
}

/*
 * Comparison function for sorting by ascending artist in the ArtistAlbumList.
 */
static gint
ET_Comp_Func_Sort_Artist_Item_By_Ascending_Artist (const GList *AlbumList1,
                                                   const GList *AlbumList2)
{
    const GList *etfilelist1 = NULL;
    const GList *etfilelist2 = NULL;
    const ET_File *etfile1 = NULL;
    const ET_File *etfile2 = NULL;
    const gchar *etfile1_artist;
    const gchar *etfile2_artist;

    if (!AlbumList1 || !(etfilelist1 = (GList *)AlbumList1->data)
        || !(etfile1 = (ET_File *)etfilelist1->data))
    {
        return -1;
    }

    if (!AlbumList2 || !(etfilelist2 = (GList *)AlbumList2->data)
        || !(etfile2 = (ET_File *)etfilelist2->data))
    {
        return 1;
    }

    etfile1_artist = ((File_Tag *)etfile1->FileTag->data)->artist;
    etfile2_artist = ((File_Tag *)etfile2->FileTag->data)->artist;

    if (g_settings_get_boolean (MainSettings, "sort-case-sensitive"))
    {
        return et_normalized_strcmp0 (etfile1_artist, etfile2_artist);
    }
    else
    {
        return et_normalized_strcasecmp0 (etfile1_artist, etfile2_artist);
    }
}

/*
 * Comparison function for sorting by ascending album in the ArtistAlbumList.
 */
static gint
ET_Comp_Func_Sort_Album_Item_By_Ascending_Album (const GList *etfilelist1,
                                                 const GList *etfilelist2)
{
    const ET_File *etfile1;
    const ET_File *etfile2;
    const gchar *etfile1_album;
    const gchar *etfile2_album;

    if (!etfilelist1 || !(etfile1 = (ET_File *)etfilelist1->data))
    {
        return -1;
    }

    if (!etfilelist2 || !(etfile2 = (ET_File *)etfilelist2->data))
    {
        return 1;
    }

    etfile1_album  = ((File_Tag *)etfile1->FileTag->data)->album;
    etfile2_album  = ((File_Tag *)etfile2->FileTag->data)->album;

    if (g_settings_get_boolean (MainSettings, "sort-case-sensitive"))
    {
        return et_normalized_strcmp0 (etfile1_album, etfile2_album);
    }
    else
    {
        return et_normalized_strcasecmp0 (etfile1_album, etfile2_album);
    }
}

/*
 * Comparison function for sorting etfile in the ArtistAlbumList.
 * FIX ME : should use the default sorting!
 */
static gint
ET_Comp_Func_Sort_Etfile_Item_By_Ascending_Filename (const ET_File *ETFile1,
                                                     const ET_File *ETFile2)
{

    if (!ETFile1) return -1;
    if (!ETFile2) return  1;

    return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1,ETFile2);
}

/*
 * The ETArtistAlbumFileList contains 3 levels of lists to sort the ETFile by artist then by album :
 *  - "ETArtistAlbumFileList" list is a list of "ArtistList" items,
 *  - "ArtistList" list is a list of "AlbumList" items,
 *  - "AlbumList" list is a list of ETFile items.
 * Note : use the function ET_Debug_Print_Artist_Album_List(...) to understand how it works, it needed...
 */
static GList *
et_artist_album_list_add_file (GList *file_list,
                               ET_File *ETFile)
{
    const gchar *ETFile_Artist;
    const gchar *ETFile_Album;
    const gchar *etfile_artist;
    const gchar *etfile_album;
    GList *ArtistList;
    GList *AlbumList = NULL;
    GList *etfilelist = NULL;
    ET_File *etfile = NULL;

    g_return_val_if_fail (ETFile != NULL, NULL);

    /* Album value of the ETFile passed in parameter. */
    ETFile_Album = ((File_Tag *)ETFile->FileTag->data)->album;
    /* Artist value of the ETFile passed in parameter. */
    ETFile_Artist = ((File_Tag *)ETFile->FileTag->data)->artist;

    for (ArtistList = file_list; ArtistList != NULL;
         ArtistList = g_list_next (ArtistList))
    {
        AlbumList = (GList *)ArtistList->data;  /* Take the first item */
        /* Take the first item, and the first etfile item. */
        if (AlbumList && (etfilelist = (GList *)AlbumList->data)
            && (etfile = (ET_File *)etfilelist->data)
            && ((File_Tag *)etfile->FileTag->data) != NULL)
        {
            etfile_artist = ((File_Tag *)etfile->FileTag->data)->artist;
        }
        else
        {
            etfile_artist = NULL;
        }

        if ((etfile_artist &&  ETFile_Artist && strcmp (etfile_artist,
                                                        ETFile_Artist) == 0)
            || (!etfile_artist && !ETFile_Artist)) /* The "artist" values correspond? */
        {
            /* The "ArtistList" item was found! */
            while (AlbumList)
            {
                if ((etfilelist = (GList *)AlbumList->data)
                    && (etfile = (ET_File *)etfilelist->data)
                    && ((File_Tag *)etfile->FileTag->data) != NULL)
                {
                    etfile_album = ((File_Tag *)etfile->FileTag->data)->album;
                }
                else
                {
                    etfile_album = NULL;
                }

                if ((etfile_album && ETFile_Album && strcmp (etfile_album,
                                                             ETFile_Album) == 0)
                    || (!etfile_album && !ETFile_Album))
                    /* The "album" values correspond? */
                {
                    /* The "AlbumList" item was found!
                     * Add the ETFile to this AlbumList item */
                    AlbumList->data = g_list_insert_sorted ((GList *)AlbumList->data,
                                                            ETFile,
                                                            (GCompareFunc)ET_Comp_Func_Sort_Etfile_Item_By_Ascending_Filename);
                    return file_list;
                }

                AlbumList = g_list_next (AlbumList);
            }

            /* The "AlbumList" item was NOT found! => Add a new "AlbumList"
             * item (+...) item to the "ArtistList" list. */
            etfilelist = g_list_append (NULL, ETFile);
            ArtistList->data = g_list_insert_sorted ((GList *)ArtistList->data,
                                                     etfilelist,
                                                     (GCompareFunc)ET_Comp_Func_Sort_Album_Item_By_Ascending_Album);
            return file_list;
        }
    }

    /* The "ArtistList" item was NOT found! => Add a new "ArtistList" to the
     * main list (=ETArtistAlbumFileList). */
    etfilelist = g_list_append (NULL, ETFile);
    AlbumList  = g_list_append (NULL, etfilelist);

    /* Sort the list by ascending Artist. */
    return g_list_insert_sorted (file_list, AlbumList,
                                 (GCompareFunc)ET_Comp_Func_Sort_Artist_Item_By_Ascending_Artist);
}

GList *
et_artist_album_list_new_from_file_list (GList *file_list)
{
    GList *result = NULL;
    GList *l;

    for (l = g_list_first (file_list); l != NULL; l = g_list_next (l))
    {
        ET_File *ETFile = (ET_File *)l->data;
        result = et_artist_album_list_add_file (result, ETFile);
    }

    return result;
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
                            ArtistList->data = AlbumList;
                        }
                        else
                        {
                            /* Delete from the main list. */
                            ETCore->ETArtistAlbumFileList = g_list_remove (ArtistList,ArtistList->data);

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

/*
 * Returns the length of the list of displayed files
 */
static guint
et_displayed_file_list_length (GList *displayed_list)
{
    GList *list;

    list = g_list_first (displayed_list);
    return g_list_length (list);
}

/*
 * Renumber the list of displayed files (IndexKey) from 1 to n
 */
static void
et_displayed_file_list_renumber (GList *displayed_list)
{
    GList *l = NULL;
    guint i = 1;

    for (l = g_list_first (displayed_list); l != NULL; l = g_list_next (l))
    {
        ((ET_File *)l->data)->IndexKey = i++;
    }
}

/*
 * Delete the corresponding file and free the allocated data. Return TRUE if deleted.
 */
void
ET_Remove_File_From_File_List (ET_File *ETFile)
{
    GList *ETFileList = NULL;          // Item containing the ETFile to delete... (in ETCore->ETFileList)
    GList *ETFileDisplayedList = NULL; // Item containing the ETFile to delete... (in ETCore->ETFileDisplayedList)

    // Remove infos of the file
    ETCore->ETFileDisplayedList_TotalSize     -= ((ET_File_Info *)ETFile->ETFileInfo)->size;
    ETCore->ETFileDisplayedList_TotalDuration -= ((ET_File_Info *)ETFile->ETFileInfo)->duration;

    // Find the ETFileList containing the ETFile item
    ETFileDisplayedList = g_list_find(g_list_first(ETCore->ETFileDisplayedList),ETFile);
    ETFileList = g_list_find (ETCore->ETFileList, ETFile);

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

    /* Remove the file from the ETFileList list. */
    ETCore->ETFileList = g_list_remove (ETCore->ETFileList, ETFile);

    // Remove the file from the ETArtistAlbumList list
    ET_Remove_File_From_Artist_Album_List(ETFile);

    /* Remove the file from the ETFileDisplayedList list (if not already). */
    ETCore->ETFileDisplayedList = g_list_remove (g_list_first (ETCore->ETFileDisplayedList),
                                                 ETFile);

    // Free data of the file
    ET_Free_File_List_Item(ETFile);

    /* Recalculate length of ETFileDisplayedList list. */
    ETCore->ETFileDisplayedList_Length = et_displayed_file_list_length (ETCore->ETFileDisplayedList);

    /* To number the ETFile in the list. */
    et_displayed_file_list_renumber (ETCore->ETFileDisplayedList);

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
}

/**************************
 * File sorting functions *
 **************************/

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

/*
 * Sort an 'ETFileList'
 */
GList *
ET_Sort_File_List (GList *ETFileList,
                   EtSortMode Sorting_Type)
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
        default:
            g_assert_not_reached ();
            break;
    }
    /* Save sorting mode (note: needed when called from UI). */
    g_settings_set_enum (MainSettings, "sort-mode", Sorting_Type);

    //ETFileList = g_list_first(etfilelist);
    return etfilelist;
}

/*
 * Returns the first item of the "displayed list"
 */
GList *
ET_Displayed_File_List_First (void)
{
    ETCore->ETFileDisplayedList = g_list_first(ETCore->ETFileDisplayedList);
    return ETCore->ETFileDisplayedList;
}

/*
 * Returns the previous item of the "displayed list". When no more item, it returns NULL.
 */
GList *
ET_Displayed_File_List_Previous (void)
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
GList *
ET_Displayed_File_List_Next (void)
{
    if (ETCore->ETFileDisplayedList && ETCore->ETFileDisplayedList->next)
        return ETCore->ETFileDisplayedList = ETCore->ETFileDisplayedList->next;
    else
        return NULL;
}

/*
 * Returns the last item of the "displayed list"
 */
GList *
ET_Displayed_File_List_Last (void)
{
    ETCore->ETFileDisplayedList = g_list_last(ETCore->ETFileDisplayedList);
    return ETCore->ETFileDisplayedList;
}

/*
 * Returns the item of the "displayed list" which correspond to the given 'ETFile' (used into browser list).
 */
GList *
ET_Displayed_File_List_By_Etfile (const ET_File *ETFile)
{
    GList *etfilelist;

    etfilelist = g_list_find (g_list_first (ETCore->ETFileDisplayedList),
                              ETFile);

    if (etfilelist)
    {
        /* To "save" the position like in ET_File_List_Next... (not very good -
         * FIXME) */
        ETCore->ETFileDisplayedList = etfilelist;
    }

    return etfilelist;
}

/*
 * Load the list of displayed files (calculate length, size, ...)
 * It contains part (filtrated : view by artists and albums) or full ETCore->ETFileList list
 */
void
et_displayed_file_list_set (GList *ETFileList)
{
    GList *l = NULL;

    ETCore->ETFileDisplayedList = g_list_first(ETFileList);

    ETCore->ETFileDisplayedList_Length = et_displayed_file_list_length (ETCore->ETFileDisplayedList);
    ETCore->ETFileDisplayedList_TotalSize     = 0;
    ETCore->ETFileDisplayedList_TotalDuration = 0;

    // Get size and duration of files in the list
    for (l = ETCore->ETFileDisplayedList; l != NULL; l = g_list_next (l))
    {
        ETCore->ETFileDisplayedList_TotalSize += ((ET_File_Info *)((ET_File *)l->data)->ETFileInfo)->size;
        ETCore->ETFileDisplayedList_TotalDuration += ((ET_File_Info *)((ET_File *)l->data)->ETFileInfo)->duration;
    }

    /* Sort the file list. */
    ET_Sort_File_List (ETCore->ETFileDisplayedList,
                       g_settings_get_enum (MainSettings,
                                            "sort-mode"));

    /* Synchronize, so that the core file list pointer always points to the
     * head of the list. */
    ETCore->ETFileList = g_list_first (ETCore->ETFileList);

    /* Should renums ETCore->ETFileDisplayedList only! */
    et_displayed_file_list_renumber (ETCore->ETFileDisplayedList);
}

/*
 * Function used to update path of filenames into list after renaming a parent directory
 * (for ex: "/mp3/old_path/file.mp3" to "/mp3/new_path/file.mp3"
 */
void
et_file_list_update_directory_name (GList *file_list,
                                    const gchar *old_path,
                                    const gchar *new_path)
{
    GList *filelist;
    ET_File *file;
    GList *filenamelist;
    gchar *filename;
    gchar *old_path_tmp;

    g_return_if_fail (file_list != NULL);
    g_return_if_fail (!et_str_empty (old_path));
    g_return_if_fail (!et_str_empty (new_path));

    /* Add '/' to end of path to avoid ambiguity between a directory and a
     * filename... */
    if (old_path[strlen (old_path) - 1] == G_DIR_SEPARATOR)
    {
        old_path_tmp = g_strdup (old_path);
    }
    else
    {
        old_path_tmp = g_strconcat (old_path, G_DIR_SEPARATOR_S, NULL);
    }

    for (filelist = g_list_first (file_list); filelist != NULL;
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
                    /* Replace path of filename. */
                    if (strncmp (filename, old_path_tmp, strlen (old_path_tmp))
                        == 0)
                    {
                        gchar *filename_tmp;

                        // Create the new filename
                        filename_tmp = g_strconcat (new_path,
                                                    (new_path[strlen (new_path) - 1] == G_DIR_SEPARATOR) ? "" : G_DIR_SEPARATOR_S,
                                                    &filename[strlen (old_path_tmp)],NULL);

                        ET_Set_Filename_File_Name_Item (FileName, NULL,
                                                        filename_tmp);
                        g_free (filename_tmp);
                    }
                }
             }
        }
    }

    g_free (old_path_tmp);
}

/*
 * Execute one 'undo' in the main undo list (it selects the last ETFile changed,
 * before to apply an undo action)
 */
ET_File *
ET_Undo_History_File_Data (void)
{
    ET_File *ETFile;
    const ET_History_File *ETHistoryFile;

    g_return_val_if_fail (ETCore->ETHistoryFileList != NULL, NULL);
    g_return_val_if_fail (et_history_list_has_undo (ETCore->ETHistoryFileList), NULL);

    ETHistoryFile = (ET_History_File *)ETCore->ETHistoryFileList->data;
    ETFile        = (ET_File *)ETHistoryFile->ETFile;
    ET_Displayed_File_List_By_Etfile(ETFile);
    ET_Undo_File_Data(ETFile);

    if (ETCore->ETHistoryFileList->prev)
        ETCore->ETHistoryFileList = ETCore->ETHistoryFileList->prev;
    return ETFile;
}

/*
 * et_history_list_has_undo:
 * @history_list: the end of a history list
 *
 * Returns: %TRUE if undo file list contains undo data, %FALSE otherwise
 */
gboolean
et_history_list_has_undo (GList *history_list)
{
    return history_list && history_list->prev;
}


/*
 * Execute one 'redo' in the main undo list
 */
ET_File *
ET_Redo_History_File_Data (void)
{
    ET_File *ETFile;
    ET_History_File *ETHistoryFile;

    if (!ETCore->ETHistoryFileList
        || !et_history_list_has_redo (ETCore->ETHistoryFileList))
    {
        return NULL;
    }

    ETHistoryFile = (ET_History_File *)ETCore->ETHistoryFileList->next->data;
    ETFile        = (ET_File *)ETHistoryFile->ETFile;
    ET_Displayed_File_List_By_Etfile(ETFile);
    ET_Redo_File_Data(ETFile);

    if (ETCore->ETHistoryFileList->next)
        ETCore->ETHistoryFileList = ETCore->ETHistoryFileList->next;
    return ETFile;
}

/*
 * et_history_list_has_redo:
 * @history_list: the end of a history list
 *
 * Returns: %TRUE if undo file list contains redo data, %FALSE otherwise
 */
gboolean
et_history_list_has_redo (GList *history_list)
{
    return history_list && history_list->next;
}

/*
 * Add a ETFile item to the main undo list of files
 */
GList *
et_history_list_add (GList *history_list,
                     ET_File *ETFile)
{
    ET_History_File *ETHistoryFile;
    GList *result;

    g_return_val_if_fail (ETFile != NULL, FALSE);

    ETHistoryFile = g_slice_new0 (ET_History_File);
    ETHistoryFile->ETFile = ETFile;

    /* The undo list must contains one item before the 'first undo' data */
    if (!history_list)
    {
        result = g_list_append (NULL,
                                g_slice_new0 (ET_History_File));
    }
    else
    {
        result = history_list;
    }

    /* Add the item to the list (cut end of list from the current element) */
    result = g_list_append (result, ETHistoryFile);
    /* TODO: Investigate whether this is sensible. */
    result = g_list_last (result);

    return result;
}

/*
 * et_file_list_check_all_saved:
 * @etfilelist: (element-type ET_File) (allow-none): a list of files
 *
 * Checks if some files, in the list, have been changed but not saved.
 *
 * Returns: %TRUE if all files have been saved, %FALSE otherwise
 */
gboolean
et_file_list_check_all_saved (GList *etfilelist)
{
    if (!etfilelist)
    {
        return TRUE;
    }
    else
    {
        GList *l;

        for (l = g_list_first (etfilelist); l != NULL; l = g_list_next (l))
        {
            if (!et_file_check_saved ((ET_File *)l->data))
            {
                return FALSE;
            }
        }

        return TRUE;
    }
}

/*
 * Returns the number of file in the directory of the selected file.
 * Parameter "path" should be in UTF-8
 */
guint
et_file_list_get_n_files_in_path (GList *file_list,
                                  const gchar *path_utf8)
{
    gchar *path_key;
    GList *l;
    guint  count = 0;

    g_return_val_if_fail (path_utf8 != NULL, count);

    path_key = g_utf8_collate_key (path_utf8, -1);

    for (l = g_list_first (file_list); l != NULL; l = g_list_next (l))
    {
        ET_File *ETFile = (ET_File *)l->data;
        const gchar *cur_filename_utf8 = ((File_Name *)((GList *)ETFile->FileNameCur)->data)->value_utf8;
        gchar *dirname_utf8      = g_path_get_dirname(cur_filename_utf8);
        gchar *dirname_key = g_utf8_collate_key (dirname_utf8, -1);

        if (strcmp (dirname_utf8, path_utf8) == 0)
        {
            count++;
        }

        g_free (dirname_utf8);
        g_free (dirname_key);
    }

    g_free (path_key);

    return count;
}
