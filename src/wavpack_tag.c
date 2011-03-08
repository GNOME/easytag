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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>

#ifdef ENABLE_WAVPACK

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <wavpack/wavpack.h>

#include "easytag.h"
#include "vcedit.h"
#include "et_core.h"
#include "picture.h"
//#include "setting.h"
#include "charset.h"
#include "wavpack_tag.h"


/***************
 * Declaration *
 ***************/

#define MULTIFIELD_SEPARATOR " - "

/**************
 * Prototypes *
 **************/
gboolean Wavpack_Tag_Write_File (FILE *file_in, gchar *filename_in, vcedit_state *state);


/*************
 * Functions *
 *************/

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
gboolean Wavpack_Tag_Read_File_Tag (gchar *filename, File_Tag *FileTag)
{
    if (!filename || !FileTag)
        return FALSE;

    WavpackContext *wpc;
    gchar *field, *field2;
    guint length;

    int open_flags = OPEN_TAGS;

    wpc = WavpackOpenFileInput(filename, NULL, open_flags, 0);

    if ( wpc == NULL ) {
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

    free(field);

    /*
     * Artist
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "artist", field, MAXLEN);

    if ( length > 0 && FileTag->artist == NULL) {
        FileTag->artist = Try_To_Validate_Utf8_String(field);
    }

    free(field);

    /*
     * Album
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "album", field, MAXLEN);

    if ( length > 0 && FileTag->album == NULL ) {
        FileTag->album = Try_To_Validate_Utf8_String(field);
    }

    free(field);

    /*
     * Discnumber
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "part", field, MAXLEN);

    if ( length > 0 && FileTag->disc_number == NULL ) {
        FileTag->disc_number = Try_To_Validate_Utf8_String(field);
    }

    free(field);

    /*
     * Year
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "year", field, MAXLEN);

    if ( length > 0 && FileTag->year == NULL ) {
        FileTag->year = Try_To_Validate_Utf8_String(field);
    }

    free(field);

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

    if ( field2 && FileTag->track_total == NULL ) {
        FileTag->track_total = Try_To_Validate_Utf8_String(field2);
    }
    if ( length > 0 && FileTag->track == NULL ) {
        FileTag->track = Try_To_Validate_Utf8_String(field);
    }

    free(field);

    /*
     * Genre
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "genre", field, MAXLEN);

    if ( length > 0 && FileTag->genre == NULL ) {
        FileTag->genre = Try_To_Validate_Utf8_String(field);
    }

    free(field);

    /*
     * Comment
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "comment", field, MAXLEN);

    if ( length > 0 && FileTag->comment == NULL ) {
        FileTag->comment = Try_To_Validate_Utf8_String(field);
    }

    free(field);

    /*
     * Composer
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "composer", field, MAXLEN);

    if ( length > 0 && FileTag->composer == NULL ) {
        FileTag->composer = Try_To_Validate_Utf8_String(field);
    }

    free(field);

    /*
     * Original artist
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "original artist", field, MAXLEN);

    if ( length > 0 && FileTag->orig_artist == NULL ) {
        FileTag->orig_artist  = Try_To_Validate_Utf8_String(field);
    }

    free(field);

    /*
     * Copyright
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "copyright", field, MAXLEN);

    if ( length > 0 && FileTag->copyright == NULL ) {
        FileTag->copyright = Try_To_Validate_Utf8_String(field);
    }

    free(field);

    /*
     * URL
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "copyright url", field, MAXLEN);

    if ( length > 0 && FileTag->url == NULL ) {
        FileTag->url = Try_To_Validate_Utf8_String(field);
    }

    free(field);

    /*
     * Encoded by
     */
    field = g_malloc0(sizeof(char) * MAXLEN);
    length = WavpackGetTagItem(wpc, "encoded by", field, MAXLEN);

    if ( length > 0 && FileTag->encoded_by == NULL ) {
        FileTag->encoded_by = Try_To_Validate_Utf8_String(field);
    }

    free(field);

    WavpackCloseFile(wpc);

    return TRUE;
}


gboolean Wavpack_Tag_Write_File_Tag (ET_File *ETFile)
{
    if (!ETFile || !ETFile->FileTag)
        return FALSE;

    WavpackContext *wpc;

    gchar    *filename = ((File_Name *)((GList *)ETFile->FileNameCur)->data)->value;
    File_Tag *FileTag  = (File_Tag *)ETFile->FileTag->data;
    gchar    *buffer;

    int open_flags = OPEN_EDIT_TAGS;

    wpc = WavpackOpenFileInput(filename, NULL, open_flags, 0);

    if ( wpc == NULL ) {
        return FALSE;
    }

    /*
     * Title
     */
    if (FileTag->title && WavpackAppendTagItem(wpc, "title", FileTag->title, strlen(FileTag->title)) == 0) {
        return FALSE;
    }

    /*
     * Artist
     */
    if (FileTag->artist && WavpackAppendTagItem(wpc, "artist", FileTag->artist, strlen(FileTag->artist)) == 0) {
        return FALSE;
    }

    /*
     * Album
     */
    if (FileTag->album && WavpackAppendTagItem(wpc, "album", FileTag->album, strlen(FileTag->album)) == 0) {
        return FALSE;
    }

    /*
     * Discnumber
    */
    if (FileTag->disc_number && WavpackAppendTagItem(wpc, "part", FileTag->disc_number, strlen(FileTag->disc_number)) == 0) {
        return FALSE;
    }

    /*
     * Year
     */
    if (FileTag->year && WavpackAppendTagItem(wpc, "year", FileTag->year, strlen(FileTag->year)) == 0) {
        return FALSE;
    }

    /*
     * Tracknumber + tracktotal
     */
    if (FileTag->track_total) {
        buffer = g_strdup_printf("%s/%s", FileTag->track, FileTag->track_total);
        if (FileTag->track && WavpackAppendTagItem(wpc, "track", buffer, strlen(buffer)) == 0) {
            g_free(buffer);
            return FALSE;
        } else {
            g_free(buffer);
        }
    } else {
        if (FileTag->track && WavpackAppendTagItem(wpc, "track", FileTag->track, strlen(FileTag->track)) == 0) {
            return FALSE;
        }
    }

    /*
     * Genre
     */
    if (FileTag->genre && WavpackAppendTagItem(wpc, "genre", FileTag->genre, strlen(FileTag->genre)) == 0) {
        return FALSE;
    }

    /*
     * Comment
     */
    if (FileTag->comment && WavpackAppendTagItem(wpc, "comment", FileTag->comment, strlen(FileTag->comment)) == 0) {
        return FALSE;
    }

    /*
     * Composer
     */
    if (FileTag->composer && WavpackAppendTagItem(wpc, "composer", FileTag->composer, strlen(FileTag->composer)) == 0) {
        return FALSE;
    }

    /*
     * Original artist
     */
    if (FileTag->orig_artist && WavpackAppendTagItem(wpc, "original artist", FileTag->orig_artist, strlen(FileTag->orig_artist)) == 0) {
        return FALSE;
    }

    /*
     * Copyright
     */
    if (FileTag->copyright && WavpackAppendTagItem(wpc, "copyright", FileTag->copyright, strlen(FileTag->copyright)) == 0) {
        return FALSE;
    }

    /*
     * URL
     */
    if (FileTag->url && WavpackAppendTagItem(wpc, "copyright url", FileTag->url, strlen(FileTag->url)) == 0) {
        return FALSE;
    }

    /*
     * Encoded by
     */
    if (FileTag->encoded_by && WavpackAppendTagItem(wpc, "encoded by", FileTag->encoded_by, strlen(FileTag->encoded_by)) == 0) {
        return FALSE;
    }

    WavpackWriteTag(wpc);

    WavpackCloseFile(wpc);

    return TRUE;
}


#endif /* ENABLE_WAVPACK */
