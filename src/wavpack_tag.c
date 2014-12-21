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
    WavpackContext *wpc;
    gchar field[MAXLEN] = { 0, };
    gchar *field2;
    guint length;

    int open_flags = OPEN_TAGS;

    g_return_val_if_fail (filename != NULL && FileTag != NULL, FALSE);

    wpc = WavpackOpenFileInput(filename, NULL, open_flags, 0);

    if ( wpc == NULL ) {
        return FALSE;
    }

    /*
     * Title
     */
    length = WavpackGetTagItem(wpc, "title", field, MAXLEN);

    if ( length > 0 && FileTag->title == NULL ) {
        FileTag->title = Try_To_Validate_Utf8_String(field);
    }

    memset (field, '\0', MAXLEN);

    /*
     * Artist
     */
    length = WavpackGetTagItem(wpc, "artist", field, MAXLEN);

    if ( length > 0 && FileTag->artist == NULL) {
        FileTag->artist = Try_To_Validate_Utf8_String(field);
    }

    memset (field, '\0', MAXLEN);

    /* Album artist. */
    length = WavpackGetTagItem (wpc, "album artist", field, MAXLEN);

    if (length > 0 && FileTag->album_artist == NULL)
    {
        FileTag->album_artist = Try_To_Validate_Utf8_String (field);
    }

    memset (field, '\0', MAXLEN);

    /*
     * Album
     */
    length = WavpackGetTagItem(wpc, "album", field, MAXLEN);

    if ( length > 0 && FileTag->album == NULL ) {
        FileTag->album = Try_To_Validate_Utf8_String(field);
    }

    memset (field, '\0', MAXLEN);

    /*
     * Discnumber + Disctotal.
     */
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
        gchar *tmp;

        tmp = Try_To_Validate_Utf8_String (field2);
        FileTag->disc_total = et_disc_number_to_string (atoi (tmp));
        g_free (tmp);
    }

    if (length > 0 && FileTag->disc_number == NULL)
    {
        gchar *tmp;

        tmp = Try_To_Validate_Utf8_String (field);
        FileTag->disc_number = et_disc_number_to_string (atoi (tmp));
        g_free (tmp);
    }

    memset (field, '\0', MAXLEN);

    /*
     * Year
     */
    length = WavpackGetTagItem(wpc, "year", field, MAXLEN);

    if ( length > 0 && FileTag->year == NULL ) {
        FileTag->year = Try_To_Validate_Utf8_String(field);
    }

    memset (field, '\0', MAXLEN);

    /*
     * Tracknumber + tracktotal
     */
    length = WavpackGetTagItem(wpc, "track", field, MAXLEN);
    field2 = g_utf8_strchr(field, -1, '/');

    /* Need to cut off the total tracks if present */
    if (field2) {
        *field2 = 0;
        field2++;
    }

    if (field2 && FileTag->track_total == NULL)
    {
        gchar *tmp;

        tmp = Try_To_Validate_Utf8_String (field2);
        FileTag->track_total = et_track_number_to_string (atoi (tmp));
        g_free (tmp);
    }

    if (length > 0 && FileTag->track == NULL)
    {
        gchar *tmp;

        tmp = Try_To_Validate_Utf8_String (field);
        FileTag->track = et_track_number_to_string (atoi (tmp));
        g_free (tmp);
    }

    memset (field, '\0', MAXLEN);

    /*
     * Genre
     */
    length = WavpackGetTagItem(wpc, "genre", field, MAXLEN);

    if ( length > 0 && FileTag->genre == NULL ) {
        FileTag->genre = Try_To_Validate_Utf8_String(field);
    }

    memset (field, '\0', MAXLEN);

    /*
     * Comment
     */
    length = WavpackGetTagItem(wpc, "comment", field, MAXLEN);

    if ( length > 0 && FileTag->comment == NULL ) {
        FileTag->comment = Try_To_Validate_Utf8_String(field);
    }

    memset (field, '\0', MAXLEN);

    /*
     * Composer
     */
    length = WavpackGetTagItem(wpc, "composer", field, MAXLEN);

    if ( length > 0 && FileTag->composer == NULL ) {
        FileTag->composer = Try_To_Validate_Utf8_String(field);
    }

    memset (field, '\0', MAXLEN);

    /*
     * Original artist
     */
    length = WavpackGetTagItem(wpc, "original artist", field, MAXLEN);

    if ( length > 0 && FileTag->orig_artist == NULL ) {
        FileTag->orig_artist  = Try_To_Validate_Utf8_String(field);
    }

    memset (field, '\0', MAXLEN);

    /*
     * Copyright
     */
    length = WavpackGetTagItem(wpc, "copyright", field, MAXLEN);

    if ( length > 0 && FileTag->copyright == NULL ) {
        FileTag->copyright = Try_To_Validate_Utf8_String(field);
    }

    memset (field, '\0', MAXLEN);

    /*
     * URL
     */
    length = WavpackGetTagItem(wpc, "copyright url", field, MAXLEN);

    if ( length > 0 && FileTag->url == NULL ) {
        FileTag->url = Try_To_Validate_Utf8_String(field);
    }

    memset (field, '\0', MAXLEN);

    /*
     * Encoded by
     */
    length = WavpackGetTagItem(wpc, "encoded by", field, MAXLEN);

    if ( length > 0 && FileTag->encoded_by == NULL ) {
        FileTag->encoded_by = Try_To_Validate_Utf8_String(field);
    }

    WavpackCloseFile (wpc);

    return TRUE;
}

/*
 * et_wavpack_append_or_delete_tag_item:
 * @wpc: the #WavpackContext of which to modify tags
 * @tag: the tag item name
 * @value: the tag value to write, or %NULL to delete
 *
 * Appends @value to the @tag item of @wpc, or removes the tag item if @value
 * is %NULL.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
static gboolean
et_wavpack_append_or_delete_tag_item (WavpackContext *wpc,
                                      const gchar *tag,
                                      const gchar *value)
{
    if (value)
    {
        return WavpackAppendTagItem (wpc, tag, value, strlen (value));
    }
    else
    {
        WavpackDeleteTagItem (wpc, tag);

        /* It is not an error if there was no tag item to delete. */
        return TRUE;
    }
}

gboolean
Wavpack_Tag_Write_File_Tag (ET_File *ETFile)
{
    WavpackContext *wpc;

    gchar    *filename = ((File_Name *)((GList *)ETFile->FileNameCur)->data)->value;
    File_Tag *FileTag  = (File_Tag *)ETFile->FileTag->data;
    gchar    *buffer;

    int open_flags = OPEN_EDIT_TAGS;

    g_return_val_if_fail (ETFile != NULL && ETFile->FileTag != NULL, FALSE);

    wpc = WavpackOpenFileInput(filename, NULL, open_flags, 0);

    if ( wpc == NULL ) {
        return FALSE;
    }

    /* Title. */
    if (!et_wavpack_append_or_delete_tag_item (wpc, "title", FileTag->title))
    {
        return FALSE;
    }

    /* Artist. */
    if (!et_wavpack_append_or_delete_tag_item (wpc, "artist", FileTag->artist))
    {
        return FALSE;
    }

    /* Album artist. */
    if (!et_wavpack_append_or_delete_tag_item (wpc, "album artist",
                                               FileTag->album_artist))
    {
        return FALSE;
    }

    /* Album. */
    if (!et_wavpack_append_or_delete_tag_item (wpc, "album", FileTag->album))
    {
        return FALSE;
    }

    /* Discnumber. */
    if (FileTag->disc_number && FileTag->disc_total)
    {
        buffer = g_strdup_printf ("%s/%s", FileTag->disc_number,
                                  FileTag->disc_total);

        if (!et_wavpack_append_or_delete_tag_item (wpc, "part", buffer))
        {
            g_free (buffer);
            return FALSE;
        }
        else
        {
            g_free (buffer);
        }
    }
    else
    {
        if (!et_wavpack_append_or_delete_tag_item (wpc, "part",
                                                   FileTag->disc_number))
        {
            return FALSE;
        }
    }

    /* Year. */
    if (!et_wavpack_append_or_delete_tag_item (wpc, "year", FileTag->year))
    {
        return FALSE;
    }

    /* Tracknumber + tracktotal. */
    if (FileTag->track_total)
    {
        buffer = g_strdup_printf ("%s/%s", FileTag->track,
                                  FileTag->track_total);

        if (!et_wavpack_append_or_delete_tag_item (wpc, "track", buffer))
        {
            g_free (buffer);
            return FALSE;
        }
        else
        {
            g_free (buffer);
        }
    }
    else
    {
        if (!et_wavpack_append_or_delete_tag_item (wpc, "track",
                                                   FileTag->track))
        {
            return FALSE;
        }
    }

    /* Genre. */
    if (!et_wavpack_append_or_delete_tag_item (wpc, "genre", FileTag->genre))
    {
        return FALSE;
    }

    /* Comment. */
    if (!et_wavpack_append_or_delete_tag_item (wpc, "comment", FileTag->comment))
    {
        return FALSE;
    }

    /* Composer. */
    if (!et_wavpack_append_or_delete_tag_item (wpc, "composer",
                                               FileTag->composer))
    {
        return FALSE;
    }

    /* Original artist. */
    if (!et_wavpack_append_or_delete_tag_item (wpc, "original artist",
                                               FileTag->orig_artist))
    {
        return FALSE;
    }

    /* Copyright. */
    if (!et_wavpack_append_or_delete_tag_item (wpc, "copyright",
                                               FileTag->copyright))
    {
        return FALSE;
    }

    /* URL. */
    if (!et_wavpack_append_or_delete_tag_item (wpc, "copyright url",
                                               FileTag->url))
    {
        return FALSE;
    }

    /* Encoded by. */
    if (!et_wavpack_append_or_delete_tag_item (wpc, "encoded by",
                                               FileTag->encoded_by))
    {
        return FALSE;
    }

    if (WavpackWriteTag (wpc) == 0)
    {
        return FALSE;
    }

    WavpackCloseFile (wpc);

    return TRUE;
}


#endif /* ENABLE_WAVPACK */
