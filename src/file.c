/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014,2015  David King <amigadave@amigadave.com>
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

#include "file.h"

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <utime.h>

#include "application_window.h"
#include "easytag.h"
#include "file_tag.h"
#include "file_list.h"
#ifdef ENABLE_MP3
#   include "id3_tag.h"
#endif
#include "picture.h"
#include "ape_tag.h"
#ifdef ENABLE_OGG
#include "ogg_tag.h"
#endif
#ifdef ENABLE_FLAC
#include "flac_tag.h"
#endif
#ifdef ENABLE_MP4
#include "mp4_tag.h"
#endif
#ifdef ENABLE_WAVPACK
#include "wavpack_tag.h"
#endif
#ifdef ENABLE_OPUS
#include "opus_tag.h"
#endif
#include "browser.h"
#include "log.h"
#include "misc.h"
#include "setting.h"
#include "charset.h"

#include "win32/win32dep.h"

static gboolean ET_Free_File_Name_List            (GList *FileNameList);
static gboolean ET_Free_File_Tag_List (GList *FileTagList);

static void ET_Mark_File_Tag_As_Saved (ET_File *ETFile);

static gboolean ET_Add_File_Name_To_List (ET_File *ETFile,
                                          File_Name *FileName);
static gboolean ET_Add_File_Tag_To_List (ET_File *ETFile, File_Tag  *FileTag);

/*
 * Create a new ET_File structure
 */
ET_File *
ET_File_Item_New (void)
{
    ET_File *ETFile;

    ETFile = g_slice_new0 (ET_File);

    return ETFile;
}

/*
 * Comparison function for sorting by ascending filename.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_Filename (const ET_File *ETFile1,
                                              const ET_File *ETFile2)
{
    const gchar *file1_ck = ((File_Name *)((GList *)ETFile1->FileNameCur)->data)->value_ck;
    const gchar *file2_ck = ((File_Name *)((GList *)ETFile2->FileNameCur)->data)->value_ck;
    // !!!! : Must be the same rules as "Cddb_Track_List_Sort_Func" to be
    // able to sort in the same order files in cddb and in the file list.
    return g_settings_get_boolean (MainSettings,
                                   "sort-case-sensitive") ? strcmp (file1_ck, file2_ck)
                                                          : strcasecmp (file1_ck, file2_ck);
}

/*
 * Comparison function for sorting by descending filename.
 */
gint
ET_Comp_Func_Sort_File_By_Descending_Filename (const ET_File *ETFile1,
                                               const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending disc number.
 */
/* FIXME: Handle non-numeric disc number. */
gint
et_comp_func_sort_file_by_ascending_disc_number (const ET_File *ETFile1,
                                                 const ET_File *ETFile2)
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
et_comp_func_sort_file_by_descending_disc_number (const ET_File *ETFile1,
                                                  const ET_File *ETFile2)
{
    return et_comp_func_sort_file_by_ascending_disc_number (ETFile2, ETFile1);
}


/*
 * Comparison function for sorting by ascending track number.
 */
/* FIXME: Handle non-numeric track number. */
gint
ET_Comp_Func_Sort_File_By_Ascending_Track_Number (const ET_File *ETFile1,
                                                  const ET_File *ETFile2)
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
gint
ET_Comp_Func_Sort_File_By_Descending_Track_Number (const ET_File *ETFile1,
                                                   const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Track_Number(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending creation date.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_Creation_Date (const ET_File *ETFile1,
                                                   const ET_File *ETFile2)
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
gint
ET_Comp_Func_Sort_File_By_Descending_Creation_Date (const ET_File *ETFile1,
                                                    const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Creation_Date(ETFile2,ETFile1);
}

/*
 * et_file_list_sort_string:
 * @str1: a UTF-8 string, or %NULL
 * @str2: a UTF-8 string to compare against, or %NULL
 * @file1: an #ET_File corresponding to @str1
 * @file2: an #ET_File corresponding to @str2
 * @case_sensitive: whether the sorting should obey case
 *
 * Compare two UTF-8 strings, normalizing them before doing so, falling back to
 * the filenames if the strings are otherwise identical, and obeying the
 * requested case-sensitivity.
 *
 * Returns: an integer less than, equal to, or greater than zero, if str1 is
 * less than, equal to or greater than str2
 */
static gint
et_file_list_sort_string (const gchar *str1,
                          const gchar *str2,
                          const ET_File *file1,
                          const ET_File *file2,
                          gboolean case_sensitive)
{
    gint result;

    if (case_sensitive)
    {
        result = et_normalized_strcmp0 (str1, str2);
    }
    else
    {
        result = et_normalized_strcasecmp0 (str1, str2);
    }

    if (result == 0)
    {
        /* Secondary criterion. */
        return ET_Comp_Func_Sort_File_By_Ascending_Filename (file1, file2);
    }
    else
    {
        /* Primary criterion. */
        return result;
    }
}

/*
 * Comparison function for sorting by ascending title.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_Title (const ET_File *ETFile1,
                                           const ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->title == ((File_Tag *)ETFile2->FileTag->data)->title))
        return 0;

    if (!ETFile1->FileTag->data)
    {
        return -1;
    }

    if (!ETFile2->FileTag->data)
    {
        return 1;
    }

    return et_file_list_sort_string (((File_Tag *)ETFile1->FileTag->data)->title,
                                     ((File_Tag *)ETFile2->FileTag->data)->title,
                                     ETFile1, ETFile2,
                                     g_settings_get_boolean (MainSettings,
                                                             "sort-case-sensitive"));
}

/*
 * Comparison function for sorting by descending title.
 */
gint
ET_Comp_Func_Sort_File_By_Descending_Title (const ET_File *ETFile1,
                                            const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Title(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending artist.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_Artist (const ET_File *ETFile1,
                                            const ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->artist == ((File_Tag *)ETFile2->FileTag->data)->artist))
        return 0;

    if (!ETFile1->FileTag->data)
    {
        return -1;
    }

    if (!ETFile2->FileTag->data)
    {
        return 1;
    }

    return et_file_list_sort_string (((File_Tag *)ETFile1->FileTag->data)->artist,
                                     ((File_Tag *)ETFile2->FileTag->data)->artist,
                                     ETFile1, ETFile2,
                                     g_settings_get_boolean (MainSettings,
                                                             "sort-case-sensitive"));
}

/*
 * Comparison function for sorting by descending artist.
 */
gint
ET_Comp_Func_Sort_File_By_Descending_Artist (const ET_File *ETFile1,
                                             const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Artist(ETFile2,ETFile1);
}

/*
 * Comparison function for sorting by ascending album artist.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_Album_Artist (const ET_File *ETFile1,
                                                  const ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->album_artist == ((File_Tag *)ETFile2->FileTag->data)->album_artist))
        return 0;

    if (!ETFile1->FileTag->data)
    {
        return -1;
    }

    if (!ETFile2->FileTag->data)
    {
        return 1;
    }

    return et_file_list_sort_string (((File_Tag *)ETFile1->FileTag->data)->album_artist,
                                     ((File_Tag *)ETFile2->FileTag->data)->album_artist,
                                     ETFile1, ETFile2,
                                     g_settings_get_boolean (MainSettings,
                                                             "sort-case-sensitive"));
}

/*
 * Comparison function for sorting by descending album artist.
 */
gint
ET_Comp_Func_Sort_File_By_Descending_Album_Artist (const ET_File *ETFile1,
                                                   const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Album_Artist(ETFile2,ETFile1);
}

/*
 * Comparison function for sorting by ascending album.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_Album (const ET_File *ETFile1,
                                           const ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->album == ((File_Tag *)ETFile2->FileTag->data)->album))
        return 0;

    if (!ETFile1->FileTag->data)
    {
        return -1;
    }

    if (!ETFile2->FileTag->data)
    {
        return 1;
    }

    return et_file_list_sort_string (((File_Tag *)ETFile1->FileTag->data)->album,
                                     ((File_Tag *)ETFile2->FileTag->data)->album,
                                     ETFile1, ETFile2,
                                     g_settings_get_boolean (MainSettings,
                                                             "sort-case-sensitive"));
}

/*
 * Comparison function for sorting by descending album.
 */
gint
ET_Comp_Func_Sort_File_By_Descending_Album (const ET_File *ETFile1,
                                            const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Album(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending year.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_Year (const ET_File *ETFile1,
                                          const ET_File *ETFile2)
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
gint
ET_Comp_Func_Sort_File_By_Descending_Year (const ET_File *ETFile1,
                                           const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Year(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending genre.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_Genre (const ET_File *ETFile1,
                                           const ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->genre == ((File_Tag *)ETFile2->FileTag->data)->genre))
        return 0;

    if (!ETFile1->FileTag->data)
    {
        return -1;
    }

    if (!ETFile2->FileTag->data)
    {
        return 1;
    }

    return et_file_list_sort_string (((File_Tag *)ETFile1->FileTag->data)->genre,
                                     ((File_Tag *)ETFile2->FileTag->data)->genre,
                                     ETFile1, ETFile2,
                                     g_settings_get_boolean (MainSettings,
                                                             "sort-case-sensitive"));
}

/*
 * Comparison function for sorting by descending genre.
 */
gint
ET_Comp_Func_Sort_File_By_Descending_Genre (const ET_File *ETFile1,
                                            const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Genre(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending comment.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_Comment (const ET_File *ETFile1,
                                             const ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->comment == ((File_Tag *)ETFile2->FileTag->data)->comment))
        return 0;

    if (!ETFile1->FileTag->data)
    {
        return -1;
    }

    if (!ETFile2->FileTag->data)
    {
        return 1;
    }

    return et_file_list_sort_string (((File_Tag *)ETFile1->FileTag->data)->comment,
                                     ((File_Tag *)ETFile2->FileTag->data)->comment,
                                     ETFile1, ETFile2,
                                     g_settings_get_boolean (MainSettings,
                                                             "sort-case-sensitive"));
}

/*
 * Comparison function for sorting by descending comment.
 */
gint
ET_Comp_Func_Sort_File_By_Descending_Comment (const ET_File *ETFile1,
                                              const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Comment(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending composer.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_Composer (const ET_File *ETFile1,
                                              const ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->composer == ((File_Tag *)ETFile2->FileTag->data)->composer))
        return 0;

    if (!ETFile1->FileTag->data)
    {
        return -1;
    }

    if (!ETFile2->FileTag->data)
    {
        return 1;
    }

    return et_file_list_sort_string (((File_Tag *)ETFile1->FileTag->data)->composer,
                                     ((File_Tag *)ETFile2->FileTag->data)->composer,
                                     ETFile1, ETFile2,
                                     g_settings_get_boolean (MainSettings,
                                                             "sort-case-sensitive"));
}

/*
 * Comparison function for sorting by descending composer.
 */
gint
ET_Comp_Func_Sort_File_By_Descending_Composer (const ET_File *ETFile1,
                                               const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Composer(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending original artist.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_Orig_Artist (const ET_File *ETFile1,
                                                 const ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->orig_artist == ((File_Tag *)ETFile2->FileTag->data)->orig_artist))
        return 0;

    if (!ETFile1->FileTag->data)
    {
        return -1;
    }

    if (!ETFile2->FileTag->data)
    {
        return 1;
    }

    return et_file_list_sort_string (((File_Tag *)ETFile1->FileTag->data)->orig_artist,
                                     ((File_Tag *)ETFile2->FileTag->data)->orig_artist,
                                     ETFile1, ETFile2,
                                     g_settings_get_boolean (MainSettings,
                                                             "sort-case-sensitive"));
}

/*
 * Comparison function for sorting by descending original artist.
 */
gint
ET_Comp_Func_Sort_File_By_Descending_Orig_Artist (const ET_File *ETFile1,
                                                  const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Orig_Artist(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending copyright.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_Copyright (const ET_File *ETFile1,
                                               const ET_File *ETFile2)
{
   // Compare pointers just in case they are the same (e.g. both are NULL)
   if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
   ||  (((File_Tag *)ETFile1->FileTag->data)->copyright == ((File_Tag *)ETFile2->FileTag->data)->copyright))
        return 0;

    if (!ETFile1->FileTag->data)
    {
        return -1;
    }

    if (!ETFile2->FileTag->data)
    {
        return 1;
    }

    return et_file_list_sort_string (((File_Tag *)ETFile1->FileTag->data)->copyright,
                                     ((File_Tag *)ETFile2->FileTag->data)->copyright,
                                     ETFile1, ETFile2,
                                     g_settings_get_boolean (MainSettings,
                                                             "sort-case-sensitive"));
}

/*
 * Comparison function for sorting by descending copyright.
 */
gint
ET_Comp_Func_Sort_File_By_Descending_Copyright (const ET_File *ETFile1,
                                                const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Copyright(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending URL.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_Url (const ET_File *ETFile1,
                                         const ET_File *ETFile2)
{
    /* Compare pointers just in case they are the same (e.g. both are NULL). */
    if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
        || (((File_Tag *)ETFile1->FileTag->data)->url
            == ((File_Tag *)ETFile2->FileTag->data)->url))
    {
        return 0;
    }

    if (!ETFile1->FileTag->data)
    {
        return -1;
    }

    if (!ETFile2->FileTag->data)
    {
        return 1;
    }

    return et_file_list_sort_string (((File_Tag *)ETFile1->FileTag->data)->url,
                                     ((File_Tag *)ETFile2->FileTag->data)->url,
                                     ETFile1, ETFile2,
                                     g_settings_get_boolean (MainSettings,
                                                             "sort-case-sensitive"));
}

/*
 * Comparison function for sorting by descending URL.
 */
gint
ET_Comp_Func_Sort_File_By_Descending_Url (const ET_File *ETFile1,
                                          const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Url(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending encoded by.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_Encoded_By (const ET_File *ETFile1,
                                                const ET_File *ETFile2)
{
    /* Compare pointers just in case they are the same (e.g. both are NULL). */
    if ((ETFile1->FileTag->data == ETFile2->FileTag->data)
        || (((File_Tag *)ETFile1->FileTag->data)->encoded_by
            == ((File_Tag *)ETFile2->FileTag->data)->encoded_by))
    {
        return 0;
    }

    if (!ETFile1->FileTag->data)
    {
        return -1;
    }

    if (!ETFile2->FileTag->data)
    {
        return 1;
    }

    return et_file_list_sort_string (((File_Tag *)ETFile1->FileTag->data)->encoded_by,
                                     ((File_Tag *)ETFile2->FileTag->data)->encoded_by,
                                     ETFile1, ETFile2,
                                     g_settings_get_boolean (MainSettings,
                                                             "sort-case-sensitive"));
}

/*
 * Comparison function for sorting by descendingencoded by.
 */
gint
ET_Comp_Func_Sort_File_By_Descending_Encoded_By (const ET_File *ETFile1,
                                                 const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_Encoded_By(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending file type (mp3, ogg, ...).
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_File_Type (const ET_File *ETFile1,
                                               const ET_File *ETFile2)
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
gint
ET_Comp_Func_Sort_File_By_Descending_File_Type (const ET_File *ETFile1,
                                                const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_File_Type(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending file size.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_File_Size (const ET_File *ETFile1,
                                               const ET_File *ETFile2)
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
gint
ET_Comp_Func_Sort_File_By_Descending_File_Size (const ET_File *ETFile1,
                                                const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_File_Size(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending file duration.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_File_Duration (const ET_File *ETFile1,
                                                   const ET_File *ETFile2)
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
gint
ET_Comp_Func_Sort_File_By_Descending_File_Duration (const ET_File *ETFile1,
                                                    const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_File_Duration(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending file bitrate.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_File_Bitrate (const ET_File *ETFile1,
                                                  const ET_File *ETFile2)
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
gint
ET_Comp_Func_Sort_File_By_Descending_File_Bitrate (const ET_File *ETFile1,
                                                   const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_File_Bitrate(ETFile2,ETFile1);
}


/*
 * Comparison function for sorting by ascending file samplerate.
 */
gint
ET_Comp_Func_Sort_File_By_Ascending_File_Samplerate (const ET_File *ETFile1,
                                                     const ET_File *ETFile2)
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
gint
ET_Comp_Func_Sort_File_By_Descending_File_Samplerate (const ET_File *ETFile1,
                                                      const ET_File *ETFile2)
{
    return ET_Comp_Func_Sort_File_By_Ascending_File_Samplerate(ETFile2,ETFile1);
}

/*********************
 * Freeing functions *
 *********************/

/*
 * Frees one item of the full main list of files.
 */
void
ET_Free_File_List_Item (ET_File *ETFile)
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
            et_file_info_free (ETFile->ETFileInfo);
        }

        g_free(ETFile->ETFileExtension);
        g_slice_free (ET_File, ETFile);
    }
}


/*
 * Frees the full list: GList *FileNameList.
 */
static gboolean
ET_Free_File_Name_List (GList *FileNameList)
{
    g_return_val_if_fail (FileNameList != NULL, FALSE);

    FileNameList = g_list_first (FileNameList);

    g_list_free_full (FileNameList, (GDestroyNotify)et_file_name_free);

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
        {
            et_file_tag_free ((File_Tag *)l->data);
        }
    }

    g_list_free (FileTagList);

    return TRUE;
}


/*********************
 * Copying functions *
 *********************/

/********************
 * Saving functions *
 ********************/

/*
 * Do the same thing of ET_Save_File_Name_From_UI, but without getting the
 * data from the UI.
 */
gboolean
ET_Save_File_Name_Internal (const ET_File *ETFile,
                            File_Name *FileName)
{
    gchar *filename_new = NULL;
    gchar *dirname = NULL;
    gchar *filename;
    gchar *extension;
    gchar *pos;
    gboolean success;

    g_return_val_if_fail (ETFile != NULL && FileName != NULL, FALSE);

    // Get the current path to the file
    dirname = g_path_get_dirname( ((File_Name *)ETFile->FileNameNew->data)->value );

    // Get the name of file (and rebuild it with extension with a 'correct' case)
    filename = g_path_get_basename( ((File_Name *)ETFile->FileNameNew->data)->value );

    // Remove the extension
    if ((pos=strrchr(filename, '.'))!=NULL)
        *pos = 0;

    /* Convert filename extension (lower/upper). */
    extension = ET_File_Format_File_Extension (ETFile);

    // Check length of filename
    //ET_File_Name_Check_Length(ETFile,filename);

    // Regenerate the new filename (without path)
    filename_new = g_strconcat(filename,extension,NULL);
    g_free(extension);
    g_free(filename);

    success = et_file_name_set_from_components (FileName, filename_new,
                                                dirname,
                                                g_settings_get_boolean (MainSettings,
                                                                        "rename-replace-illegal-chars"));

    g_free (filename_new);
    g_free (dirname);
    return success;
}

/*
 * Do the same thing of et_tag_area_create_file_tag without getting the data from the UI.
 */
gboolean
ET_Save_File_Tag_Internal (ET_File *ETFile, File_Tag *FileTag)
{
    File_Tag *FileTagCur;


    g_return_val_if_fail (ETFile != NULL && ETFile->FileTag != NULL
                          && FileTag != NULL, FALSE);

    FileTagCur = (File_Tag *)ETFile->FileTag->data;

    /* Title */
    if (!et_str_empty (FileTagCur->title))
    {
        FileTag->title = g_strdup(FileTagCur->title);
        g_strstrip (FileTag->title);
    } else
    {
        FileTag->title = NULL;
    }

    /* Artist */
    if (!et_str_empty (FileTagCur->artist))
    {
        FileTag->artist = g_strdup(FileTagCur->artist);
        g_strstrip (FileTag->artist);
    } else
    {
        FileTag->artist = NULL;
    }

    /* Album Artist */
    if (!et_str_empty (FileTagCur->album_artist))
    {
        FileTag->album_artist = g_strdup(FileTagCur->album_artist);
        g_strstrip (FileTag->album_artist);
    } else
    {
        FileTag->album_artist = NULL;
    }


    /* Album */
    if (!et_str_empty (FileTagCur->album))
    {
        FileTag->album = g_strdup(FileTagCur->album);
        g_strstrip (FileTag->album);
    } else
    {
        FileTag->album = NULL;
    }


    /* Disc Number */
    if (!et_str_empty (FileTagCur->disc_number))
    {
        FileTag->disc_number = g_strdup(FileTagCur->disc_number);
        g_strstrip (FileTag->disc_number);
    } else
    {
        FileTag->disc_number = NULL;
    }


    /* Discs Total */
    if (!et_str_empty (FileTagCur->disc_total))
    {
        FileTag->disc_total = et_disc_number_to_string (atoi (FileTagCur->disc_total));
        g_strstrip (FileTag->disc_total);
    }
    else
    {
        FileTag->disc_total = NULL;
    }


    /* Year */
    if (!et_str_empty (FileTagCur->year))
    {
        FileTag->year = g_strdup(FileTagCur->year);
        g_strstrip (FileTag->year);
    } else
    {
        FileTag->year = NULL;
    }


    /* Track */
    if (!et_str_empty (FileTagCur->track))
    {
        gchar *tmp_str;

        FileTag->track = et_track_number_to_string (atoi (FileTagCur->track));

        /* This field must contain only digits. */
        tmp_str = FileTag->track;
        while (g_ascii_isdigit (*tmp_str)) tmp_str++;
            *tmp_str = 0;
        g_strstrip (FileTag->track);
    } else
    {
        FileTag->track = NULL;
    }


    /* Track Total */
    if (!et_str_empty (FileTagCur->track_total))
    {
        FileTag->track_total = et_track_number_to_string (atoi (FileTagCur->track_total));
        g_strstrip (FileTag->track_total);
    } else
    {
        FileTag->track_total = NULL;
    }


    /* Genre */
    if (!et_str_empty (FileTagCur->genre))
    {
        FileTag->genre = g_strdup(FileTagCur->genre);
        g_strstrip (FileTag->genre);
    } else
    {
        FileTag->genre = NULL;
    }


    /* Comment */
    if (!et_str_empty (FileTagCur->comment))
    {
        FileTag->comment = g_strdup(FileTagCur->comment);
        g_strstrip (FileTag->comment);
    } else
    {
        FileTag->comment = NULL;
    }


    /* Composer */
    if (!et_str_empty (FileTagCur->composer))
    {
        FileTag->composer = g_strdup(FileTagCur->composer);
        g_strstrip (FileTag->composer);
    } else
    {
        FileTag->composer = NULL;
    }


    /* Original Artist */
    if (!et_str_empty (FileTagCur->orig_artist))
    {
        FileTag->orig_artist = g_strdup(FileTagCur->orig_artist);
        g_strstrip (FileTag->orig_artist);
    } else
    {
        FileTag->orig_artist = NULL;
    }


    /* Copyright */
    if (!et_str_empty (FileTagCur->copyright))
    {
        FileTag->copyright = g_strdup(FileTagCur->copyright);
        g_strstrip (FileTag->copyright);
    } else
    {
        FileTag->copyright = NULL;
    }


    /* URL */
    if (!et_str_empty (FileTagCur->url))
    {
        FileTag->url = g_strdup(FileTagCur->url);
        g_strstrip (FileTag->url);
    } else
    {
        FileTag->url = NULL;
    }


    /* Encoded by */
    if (!et_str_empty (FileTagCur->encoded_by))
    {
        FileTag->encoded_by = g_strdup(FileTagCur->encoded_by);
        g_strstrip (FileTag->encoded_by);
    } else
    {
        FileTag->encoded_by = NULL;
    }


    /* Picture */
    et_file_tag_set_picture (FileTag, FileTagCur->picture);

    return TRUE;
}


/*
 * Save data contained into File_Tag structure to the file on hard disk.
 */
gboolean
ET_Save_File_Tag_To_HD (ET_File *ETFile, GError **error)
{
    const ET_File_Description *description;
    const gchar *cur_filename;
    const gchar *cur_filename_utf8;
    gboolean state;
    GFile *file;
    GFileInfo *fileinfo;

    g_return_val_if_fail (ETFile != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    cur_filename = ((File_Name *)(ETFile->FileNameCur)->data)->value;
    cur_filename_utf8 = ((File_Name *)(ETFile->FileNameCur)->data)->value_utf8;

    description = ETFile->ETFileDescription;

    /* Store the file timestamps (in case they are to be preserved) */
    file = g_file_new_for_path (cur_filename);
    fileinfo = g_file_query_info (file, "time::*", G_FILE_QUERY_INFO_NONE,
                                  NULL, NULL);

    switch (description->TagType)
    {
#ifdef ENABLE_MP3
        case ID3_TAG:
            state = id3tag_write_file_tag (ETFile, error);
            break;
#endif
#ifdef ENABLE_OGG
        case OGG_TAG:
            state = ogg_tag_write_file_tag (ETFile, error);
            break;
#endif
#ifdef ENABLE_FLAC
        case FLAC_TAG:
            state = flac_tag_write_file_tag (ETFile, error);
            break;
#endif
        case APE_TAG:
            state = ape_tag_write_file_tag (ETFile, error);
            break;
#ifdef ENABLE_MP4
        case MP4_TAG:
            state = mp4tag_write_file_tag (ETFile, error);
            break;
#endif
#ifdef ENABLE_WAVPACK
        case WAVPACK_TAG:
            state = wavpack_tag_write_file_tag (ETFile, error);
            break;
#endif
#ifdef ENABLE_OPUS
        case OPUS_TAG:
            state = ogg_tag_write_file_tag (ETFile, error);
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
            Log_Print (LOG_ERROR,
                       "Saving to HD: Undefined function for tag type '%d' (file %s).",
                       (gint)description->TagType, cur_filename_utf8);
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
            g_utime (path, NULL);
            g_free (path);
        }

        ET_Mark_File_Tag_As_Saved(ETFile);
        return TRUE;
    }
    else
    {
        g_assert (error == NULL || *error != NULL);

        return FALSE;
    }
}

/*
 * Check if 'FileName' and 'FileTag' differ with those of 'ETFile'.
 * Manage undo feature for the ETFile and the main undo list.
 */
gboolean
ET_Manage_Changes_Of_File_Data (ET_File *ETFile,
                                File_Name *FileName,
                                File_Tag *FileTag)
{
    gboolean undo_added = FALSE;

    g_return_val_if_fail (ETFile != NULL, FALSE);

    /*
     * Detect changes of filename and generate the filename undo list
     */
    if (FileName)
    {
        if (ETFile->FileNameNew
            && et_file_name_detect_difference ((File_Name *)(ETFile->FileNameNew)->data,
                                               FileName) == TRUE)
        {
            ET_Add_File_Name_To_List(ETFile,FileName);
            undo_added |= TRUE;
        }else
        {
            et_file_name_free (FileName);
        }
    }

    /*
     * Detect changes in tag data and generate the tag undo list
     */
    if (FileTag)
    {
        if (ETFile->FileTag
            && et_file_tag_detect_difference ((File_Tag *)(ETFile->FileTag)->data,
                                              FileTag) == TRUE)
        {
            ET_Add_File_Tag_To_List(ETFile,FileTag);
            undo_added |= TRUE;
        }
        else
        {
            et_file_tag_free (FileTag);
        }
    }

    /*
     * Generate main undo (file history of modifications)
     */
    if (undo_added)
    {
        ETCore->ETHistoryFileList = et_history_list_add (ETCore->ETHistoryFileList,
                                                         ETFile);
    }

    //return TRUE;
    return undo_added;
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
gboolean
ET_File_Data_Has_Undo_Data (const ET_File *ETFile)
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
gboolean
ET_File_Data_Has_Redo_Data (const ET_File *ETFile)
{
    gboolean has_filename_redo_data = FALSE;
    gboolean has_filetag_redo_data  = FALSE;

    g_return_val_if_fail (ETFile != NULL, FALSE);

    if (ETFile->FileNameNew && ETFile->FileNameNew->next) has_filename_redo_data = TRUE;
    if (ETFile->FileTag && ETFile->FileTag->next)         has_filetag_redo_data  = TRUE;

    return has_filename_redo_data | has_filetag_redo_data;
}

/*
 * Checks if the current files had been changed but not saved.
 * Returns TRUE if the file has been saved.
 * Returns FALSE if some changes haven't been saved.
 */
gboolean
et_file_check_saved (const ET_File *ETFile)
{
    const File_Tag *FileTag = NULL;
    const File_Name *FileNameNew = NULL;

    g_return_val_if_fail (ETFile != NULL, TRUE);

    if (ETFile->FileTag)
    {
        FileTag = ETFile->FileTag->data;
    }

    if (ETFile->FileNameNew)
    {
        FileNameNew = ETFile->FileNameNew->data;
    }

    /* Check if the tag has been changed. */
    if (FileTag && !FileTag->saved)
    {
        return FALSE;
    }

    /* Check if name of file has been changed. */
    if (FileNameNew && !FileNameNew->saved)
    {
        return FALSE;
    }

    /* No changes. */
    return TRUE;
}

/*******************
 * Extra functions *
 *******************/

/*
 * Set to TRUE the value of 'FileTag->saved' for the File_Tag item passed in parameter.
 * And set ALL other values of the list to FALSE.
 */
static void
Set_Saved_Value_Of_File_Tag (File_Tag *FileTag, gpointer user_data)
{
    const gboolean saved = (gboolean)GPOINTER_TO_INT(user_data);

    if (FileTag) FileTag->saved = saved;
}

static void
ET_Mark_File_Tag_As_Saved (ET_File *ETFile)
{
    File_Tag *FileTag;
    GList *FileTagList;

    FileTag     = (File_Tag *)ETFile->FileTag->data;    // The current FileTag, to set to TRUE
    FileTagList = ETFile->FileTagList;
    /* All other FileTag set to FALSE. */
    g_list_foreach (FileTagList, (GFunc)Set_Saved_Value_Of_File_Tag,
                    GINT_TO_POINTER((gint)FALSE));
    FileTag->saved = TRUE; // The current FileTag set to TRUE
}


void ET_Mark_File_Name_As_Saved (ET_File *ETFile)
{
    File_Name *FileNameNew;
    GList *FileNameList;

    FileNameNew  = (File_Name *)ETFile->FileNameNew->data;    // The current FileName, to set to TRUE
    FileNameList = ETFile->FileNameList;
    g_list_foreach (FileNameList, (GFunc)Set_Saved_Value_Of_File_Tag,
                    GINT_TO_POINTER((gint)FALSE));
    FileNameNew->saved = TRUE;
}

/*
 * et_file_generate_name:
 * @ETFile: the file from which to read the existing name
 * @new_file_name_utf8: UTF-8 encoded new filename
 *
 * This function generates a new filename using path of the old file and the
 * new name.
 *
 * Returns: a newly-allocated filename, in UTF-8
 */
#if 1
gchar *
et_file_generate_name (const ET_File *ETFile,
                       const gchar *new_file_name_utf8)
{
    gchar *dirname_utf8;

    g_return_val_if_fail (ETFile && ETFile->FileNameNew->data, NULL);
    g_return_val_if_fail (new_file_name_utf8, NULL);

    if ((dirname_utf8 = g_path_get_dirname (((File_Name *)ETFile->FileNameNew->data)->value_utf8)))
    {
        gchar *extension;
        gchar *new_file_name_path_utf8;

        /* Convert filename extension (lower/upper). */
        extension = ET_File_Format_File_Extension (ETFile);

        // Check length of filename (limit ~255 characters)
        //ET_File_Name_Check_Length(ETFile,new_file_name_utf8);

        if (g_path_is_absolute (new_file_name_utf8))
        {
            /* Just add the extension. */
            new_file_name_path_utf8 = g_strconcat (new_file_name_utf8,
                                                   extension, NULL);
        }
        else
        {
            /* New path (with filename). */
            if (strcmp (dirname_utf8, G_DIR_SEPARATOR_S) == 0)
            {
                /* Root directory. */
                new_file_name_path_utf8 = g_strconcat (dirname_utf8,
                                                       new_file_name_utf8,
                                                       extension, NULL);
            }
            else
            {
                new_file_name_path_utf8 = g_strconcat (dirname_utf8,
                                                       G_DIR_SEPARATOR_S,
                                                       new_file_name_utf8,
                                                       extension, NULL);
            }
        }

        g_free (dirname_utf8);
        g_free (extension);

        return new_file_name_path_utf8;
    }

    return NULL;
}
#else
/* FOR TESTING */
/* Returns filename in file system encoding */
gchar *et_file_generate_name (ET_File *ETFile, gchar *new_file_name_utf8)
{
    gchar *dirname;

    if (ETFile && ETFile->FileNameNew->data && new_file_name_utf8
    && (dirname=g_path_get_dirname(((File_Name *)ETFile->FileNameNew->data)->value)) )
    {
        gchar *extension;
        gchar *new_file_name_path;
        gchar *new_file_name;

        new_file_name = filename_from_display(new_file_name_utf8);

        /* Convert filename extension (lower/upper). */
        extension = ET_File_Format_File_Extension (ETFile);

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
gchar *
ET_File_Format_File_Extension (const ET_File *ETFile)
{
    EtFilenameExtensionMode mode;

    mode = g_settings_get_enum (MainSettings, "rename-extension-mode");

    switch (mode)
    {
        case ET_FILENAME_EXTENSION_LOWER_CASE:
            return g_ascii_strdown (ETFile->ETFileDescription->Extension, -1);
        case ET_FILENAME_EXTENSION_UPPER_CASE:
            return g_ascii_strup (ETFile->ETFileDescription->Extension, -1);
        case ET_FILENAME_EXTENSION_NO_CHANGE:
        default:
            return g_strdup (ETFile->ETFileExtension);
    };
}
