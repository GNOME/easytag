/* wavpack_tag.c - 2007/02/15 */
/*
 *  EasyTAG - Tag editor for many file types
 *  Copyright (C) 2007 Maarten Maathuis (madman2003@gmail.com)
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

#include "config.h"

#ifdef ENABLE_WAVPACK

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <wavpack/wavpack.h>

#include "easytag.h"
#include "et_core.h"
#include "picture.h"
#include "charset.h"
#include "misc.h"
#include "wavpack_tag.h"

#define MAXLEN 1024

/*
 * For the APEv2 tags, the following field names are officially supported and
 * recommended by WavPack (although there are no restrictions on what field names
 * may be used):
 * 
 *     Artist
 *     Title
 *     Album
 *     Track
 *     Year
 *     Genre
 *     Comment
 *     Cuesheet (note: may include replay gain info as remarks)
 *     Replay_Track_Gain
 *     Replay_Track_Peak
 *     Replay_Album_Gain
 *     Replay_Album_Peak
 *     Cover Art (Front)
 *     Cover Art (Back)
 */

/*
 * Read tag data from a Wavpack file.
 */
gboolean
wavpack_tag_read_file_tag (const gchar *filename,
                           File_Tag *FileTag,
                           GError **error)
{
    WavpackContext *wpc;
    gchar message[80];
    gchar *field, *field2;
    guint length;

    int open_flags = OPEN_TAGS;

    g_return_val_if_fail (filename != NULL && FileTag != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    /* TODO: Use WavpackOpenFileInputEx() instead. */
    wpc = WavpackOpenFileInput (filename, message, open_flags, 0);

    if (wpc == NULL)
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "%s", message);
        return FALSE;
    }

    /*
     * Title
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "title", field, MAXLEN);

    if ( length > 0 && FileTag->title == NULL ) {
        FileTag->title = Try_To_Validate_Utf8_String(field);
    }

    g_free (field);

    /*
     * Artist
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "artist", field, MAXLEN);

    if ( length > 0 && FileTag->artist == NULL) {
        FileTag->artist = Try_To_Validate_Utf8_String(field);
    }

    g_free (field);

    /*
     * Album
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "album", field, MAXLEN);

    if ( length > 0 && FileTag->album == NULL ) {
        FileTag->album = Try_To_Validate_Utf8_String(field);
    }

    g_free (field);

    /*
     * Discnumber + Disctotal.
     */
    field = g_malloc0 (sizeof (char) * MAXLEN);
    length = WavpackGetTagItem (wpc, "part", field, MAXLEN);
    field2 = g_utf8_strchr (field, -1, '/');

    /* Need to cut off the total tracks if present */
    if (field2)
    {
        *field2 = 0;
        field2++;
    }

    if (field2 && FileTag->disc_total == NULL)
    {
        FileTag->disc_total = et_disc_number_to_string (atoi (Try_To_Validate_Utf8_String (field2)));
    }
    if (length > 0 && FileTag->disc_number == NULL)
    {
        FileTag->disc_number = et_disc_number_to_string (atoi (Try_To_Validate_Utf8_String (field)));
    }

    g_free (field);

    /*
     * Year
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "year", field, MAXLEN);

    if ( length > 0 && FileTag->year == NULL ) {
        FileTag->year = Try_To_Validate_Utf8_String(field);
    }

    g_free (field);

    /*
     * Tracknumber + tracktotal
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "track", field, MAXLEN);
    field2 = g_utf8_strchr(field, -1, '/');

    /* Need to cut off the total tracks if present */
    if (field2) {
        *field2 = 0;
        field2++;
    }

    if (field2 && FileTag->track_total == NULL)
    {
        FileTag->track_total = et_track_number_to_string (atoi (Try_To_Validate_Utf8_String (field2)));
    }
    if (length > 0 && FileTag->track == NULL)
    {
        FileTag->track = et_track_number_to_string (atoi (Try_To_Validate_Utf8_String (field)));
    }

    g_free (field);

    /*
     * Genre
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "genre", field, MAXLEN);

    if ( length > 0 && FileTag->genre == NULL ) {
        FileTag->genre = Try_To_Validate_Utf8_String(field);
    }

    g_free (field);

    /*
     * Comment
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "comment", field, MAXLEN);

    if ( length > 0 && FileTag->comment == NULL ) {
        FileTag->comment = Try_To_Validate_Utf8_String(field);
    }

    g_free (field);

    /*
     * Composer
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "composer", field, MAXLEN);

    if ( length > 0 && FileTag->composer == NULL ) {
        FileTag->composer = Try_To_Validate_Utf8_String(field);
    }

    g_free (field);

    /*
     * Original artist
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "original artist", field, MAXLEN);

    if ( length > 0 && FileTag->orig_artist == NULL ) {
        FileTag->orig_artist  = Try_To_Validate_Utf8_String(field);
    }

    g_free (field);

    /*
     * Copyright
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "copyright", field, MAXLEN);

    if ( length > 0 && FileTag->copyright == NULL ) {
        FileTag->copyright = Try_To_Validate_Utf8_String(field);
    }

    g_free (field);

    /*
     * URL
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "copyright url", field, MAXLEN);

    if ( length > 0 && FileTag->url == NULL ) {
        FileTag->url = Try_To_Validate_Utf8_String(field);
    }

    g_free (field);

    /*
     * Encoded by
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "encoded by", field, MAXLEN);

    if ( length > 0 && FileTag->encoded_by == NULL ) {
        FileTag->encoded_by = Try_To_Validate_Utf8_String(field);
    }

    g_free (field);

    WavpackCloseFile(wpc);

    return TRUE;
}

gboolean
wavpack_tag_write_file_tag (const ET_File *ETFile,
                            GError **error)
{
    const gchar *filename;
    const File_Tag *FileTag;
    WavpackContext *wpc;
    gchar message[80];
    gchar *buffer;

    g_return_val_if_fail (ETFile != NULL && ETFile->FileTag != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    filename = ((File_Name *)((GList *)ETFile->FileNameCur)->data)->value;
    FileTag = (File_Tag *)ETFile->FileTag->data;

    /* TODO: Use WavpackOpenFileInputEx() instead. */
    wpc = WavpackOpenFileInput (filename, message, OPEN_EDIT_TAGS, 0);

    if (wpc == NULL)
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "%s", message);
        return FALSE;
    }

    /*
     * Title
     */
    if (FileTag->title && WavpackAppendTagItem(wpc, "title", FileTag->title, strlen(FileTag->title)) == 0) {
        goto err;
    }

    /*
     * Artist
     */
    if (FileTag->artist && WavpackAppendTagItem(wpc, "artist", FileTag->artist, strlen(FileTag->artist)) == 0) {
        goto err;
    }

    /*
     * Album
     */
    if (FileTag->album && WavpackAppendTagItem(wpc, "album", FileTag->album, strlen(FileTag->album)) == 0) {
        goto err;
    }

    /*
     * Discnumber
    */
    if (FileTag->disc_number && FileTag->disc_total)
    {
        buffer = g_strdup_printf ("%s/%s", FileTag->disc_number,
                                  FileTag->disc_total);

        if (WavpackAppendTagItem (wpc, "part", buffer, strlen (buffer)) == 0)
        {
            g_free (buffer);
            goto err;
        }
        else
        {
            g_free (buffer);
        }
    }
    else
    {
        if (FileTag->disc_number && WavpackAppendTagItem (wpc, "part",
                                                          FileTag->disc_number,
                                                          strlen (FileTag->disc_number)) == 0)
        {
            goto err;
        }
    }

    /*
     * Year
     */
    if (FileTag->year && WavpackAppendTagItem(wpc, "year", FileTag->year, strlen(FileTag->year)) == 0) {
        goto err;
    }

    /*
     * Tracknumber + tracktotal
     */
    if (FileTag->track_total) {
        buffer = g_strdup_printf("%s/%s", FileTag->track, FileTag->track_total);
        if (FileTag->track && WavpackAppendTagItem(wpc, "track", buffer, strlen(buffer)) == 0) {
            g_free(buffer);
            goto err;
        } else {
            g_free(buffer);
        }
    } else {
        if (FileTag->track && WavpackAppendTagItem(wpc, "track", FileTag->track, strlen(FileTag->track)) == 0) {
            goto err;
        }
    }

    /*
     * Genre
     */
    if (FileTag->genre && WavpackAppendTagItem(wpc, "genre", FileTag->genre, strlen(FileTag->genre)) == 0) {
        goto err;
    }

    /*
     * Comment
     */
    if (FileTag->comment && WavpackAppendTagItem(wpc, "comment", FileTag->comment, strlen(FileTag->comment)) == 0) {
        goto err;
    }

    /*
     * Composer
     */
    if (FileTag->composer && WavpackAppendTagItem(wpc, "composer", FileTag->composer, strlen(FileTag->composer)) == 0) {
        goto err;
    }

    /*
     * Original artist
     */
    if (FileTag->orig_artist && WavpackAppendTagItem(wpc, "original artist", FileTag->orig_artist, strlen(FileTag->orig_artist)) == 0) {
        goto err;
    }

    /*
     * Copyright
     */
    if (FileTag->copyright && WavpackAppendTagItem(wpc, "copyright", FileTag->copyright, strlen(FileTag->copyright)) == 0) {
        goto err;
    }

    /*
     * URL
     */
    if (FileTag->url && WavpackAppendTagItem(wpc, "copyright url", FileTag->url, strlen(FileTag->url)) == 0) {
        goto err;
    }

    /*
     * Encoded by
     */
    if (FileTag->encoded_by && WavpackAppendTagItem(wpc, "encoded by", FileTag->encoded_by, strlen(FileTag->encoded_by)) == 0) {
        goto err;
    }

    if (WavpackWriteTag (wpc) != 0)
    {
        goto err;
    }

    WavpackCloseFile (wpc);

    return TRUE;

err:
    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "%s",
                 WavpackGetErrorMessage (wpc));
    WavpackCloseFile (wpc);
    return FALSE;
}

#endif /* ENABLE_WAVPACK */
