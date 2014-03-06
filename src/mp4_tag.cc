/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2012-1014  David King <amigadave@amigadave.com>
 *  Copyright (C) 2001-2005  Jerome Couderc <easytag@gmail.com>
 *  Copyright (C) 2005  Michael Ihde <mike.ihde@randomwalking.com>
 *  Copyright (C) 2005  Stewart Whitman <swhitman@cox.net>
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

#include "config.h" // For definition of ENABLE_MP4

#ifdef ENABLE_MP4

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>

#include "mp4_tag.h"
#include "picture.h"
#include "easytag.h"
#include "setting.h"
#include "log.h"
#include "misc.h"
#include "et_core.h"
#include "charset.h"

#include <mp4file.h>

/*
 * Mp4_Tag_Read_File_Tag:
 *
 * Read tag data into an Mp4 file.
 *
 * Note:
 *  - for string fields, //if field is found but contains no info (strlen(str)==0), we don't read it
 *  - for track numbers, if they are zero, then we don't read it
 */
gboolean Mp4tag_Read_File_Tag (gchar *filename, File_Tag *FileTag)
{
    TagLib::MP4::Tag *tag;
    guint year;
    guint track;

    g_return_val_if_fail (filename != NULL && FileTag != NULL, FALSE);

    /* Get data from tag. */
    TagLib::MP4::File mp4file (filename);

    if (!mp4file.isOpen ())
    {
        gchar *filename_utf8 = filename_to_display (filename);
        Log_Print (LOG_ERROR, _("Error while opening file: '%s' (%s)."),
                   filename_utf8, _("MP4 format invalid"));
        g_free (filename_utf8);
        return FALSE;
    }

    if (!(tag = mp4file.tag ()))
    {
        gchar *filename_utf8 = filename_to_display (filename);
        Log_Print (LOG_ERROR, _("Error reading tags from file: '%s'"),
                   filename_utf8);
        g_free (filename_utf8);
        return FALSE;
    }

    /*********
     * Title *
     *********/
    FileTag->title = g_strdup (tag->title ().toCString (true));

    /**********
     * Artist *
     **********/
    FileTag->artist = g_strdup (tag->artist ().toCString (true));

    /*********
     * Album *
     *********/
    FileTag->album = g_strdup (tag->album ().toCString (true));

    /****************
     * Album Artist *
     ****************/
    /* TODO: No album artist or disc number support in the TagLib C API! */

    /********
     * Year *
     ********/
    year = tag->year ();

    if (year != 0)
    {
        FileTag->year = g_strdup_printf ("%u", year);
    }

    /*************************
     * Track and Total Track *
     *************************/
    track = tag->track ();

    if (track != 0)
        FileTag->track = et_track_number_to_string (track);
    /* TODO: No total track support in the TagLib C API! */

    /*********
     * Genre *
     *********/
    FileTag->genre = g_strdup (tag->genre ().toCString (true));

    /***********
     * Comment *
     ***********/
    FileTag->comment = g_strdup (tag->comment ().toCString (true));

    /**********************
     * Composer or Writer *
     **********************/
    /* TODO: No composer support in the TagLib C API! */

    /*****************
     * Encoding Tool *
     *****************/
    /* TODO: No encode_by support in the TagLib C API! */

    /***********
     * Picture *
     ***********/
    /* TODO: No encode_by support in the TagLib C API! */

    return TRUE;
}


/*
 * Mp4_Tag_Write_File_Tag:
 *
 * Write tag data into an Mp4 file.
 *
 * Note:
 *  - for track numbers, we write 0's if one or the other is blank
 */
gboolean Mp4tag_Write_File_Tag (ET_File *ETFile)
{
    File_Tag *FileTag;
    gchar    *filename;
    gchar    *filename_utf8;
    TagLib::Tag *tag;
    gboolean success;

    g_return_val_if_fail (ETFile != NULL && ETFile->FileTag != NULL, FALSE);

    FileTag = (File_Tag *)ETFile->FileTag->data;
    filename      = ((File_Name *)ETFile->FileNameCur->data)->value;
    filename_utf8 = ((File_Name *)ETFile->FileNameCur->data)->value_utf8;

    /* Open file for writing */
    TagLib::MP4::File mp4file (filename);

    if (!mp4file.isOpen ())
    {
        Log_Print (LOG_ERROR, _("Error while opening file: '%s' (%s)."),
                   filename_utf8, _("MP4 format invalid"));
        return FALSE;
    }

    if (!(tag = mp4file.tag ()))
    {
        gchar *filename_utf8 = filename_to_display (filename);
        Log_Print (LOG_ERROR, _("Error reading tags from file: '%s'"),
                   filename_utf8);
        g_free (filename_utf8);
        return FALSE;
    }

    /*********
     * Title *
     *********/
    if (FileTag->title && *(FileTag->title))
    {
        TagLib::String string (FileTag->title, TagLib::String::UTF8);
        tag->setTitle (string);
    }

    /**********
     * Artist *
     **********/
    if (FileTag->artist && *(FileTag->artist))
    {
        TagLib::String string (FileTag->artist, TagLib::String::UTF8);
        tag->setArtist (string);
    }

    /*********
     * Album *
     *********/
    if (FileTag->album && *(FileTag->album))
    {
        TagLib::String string (FileTag->album, TagLib::String::UTF8);
        tag->setAlbum (string);
    }

    /********
     * Year *
     ********/
    if (FileTag->year)
    {
        tag->setYear (atoi (FileTag->year));
    }

    /*************************
     * Track and Total Track *
     *************************/
    if (FileTag->track)
    {
        tag->setTrack (atoi (FileTag->track));
    }

    /*********
     * Genre *
     *********/
    if (FileTag->genre && *(FileTag->genre))
    {
        TagLib::String string (FileTag->genre, TagLib::String::UTF8);
        tag->setGenre (string);
    }

    /***********
     * Comment *
     ***********/
    if (FileTag->comment && *(FileTag->comment))
    {
        TagLib::String string (FileTag->comment, TagLib::String::UTF8);
        tag->setComment (string);
    }

    /**********************
     * Composer or Writer *
     **********************/

    /*****************
     * Encoding Tool *
     *****************/

    /***********
     * Picture *
     ***********/

    success = mp4file.save () ? TRUE : FALSE;

    return success;
}

#endif /* ENABLE_MP4 */
