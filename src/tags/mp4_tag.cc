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

#include "mp4_header.h"
#include "mp4_tag.h"
#include "picture.h"
#include "easytag.h"
#include "setting.h"
#include "log.h"
#include "misc.h"
#include "et_core.h"
#include "charset.h"
#include "gio_wrapper.h"

#include <mp4file.h>
#include <mp4tag.h>
#include <tpropertymap.h>

/* Include mp4_header.cc directly. */
#include "mp4_header.cc"

/*
 * Mp4_Tag_Read_File_Tag:
 *
 * Read tag data into an Mp4 file.
 */
gboolean
Mp4tag_Read_File_Tag (const gchar *filename, File_Tag *FileTag)
{
    TagLib::MP4::Tag *tag;
    guint year;

    g_return_val_if_fail (filename != NULL && FileTag != NULL, FALSE);

    /* Get data from tag. */
    GFile *file = g_file_new_for_path (filename);
    GIO_InputStream stream (file);

    if (!stream.isOpen ())
    {
        gchar *filename_utf8 = filename_to_display (filename);
        const GError *error = stream.getError ();
        Log_Print (LOG_ERROR, _("Error while opening file ‘%s’: %s"),
                   filename_utf8, error->message);
        g_free (filename_utf8);
        return FALSE;
    }

    TagLib::MP4::File mp4file (&stream);
    g_object_unref (file);

    if (!mp4file.isOpen ())
    {
        gchar *filename_utf8 = filename_to_display (filename);
        const GError *error = stream.getError ();

        if (error)
        {
            Log_Print (LOG_ERROR, _("Error while opening file ‘%s’: %s"),
                       filename_utf8, error->message);
        }
        else
        {
            Log_Print (LOG_ERROR, _("Error while opening file ‘%s’: %s"),
                       filename_utf8, _("MP4 format invalid"));
        }

        g_free (filename_utf8);
        return FALSE;
    }

    if (!(tag = mp4file.tag ()))
    {
        gchar *filename_utf8 = filename_to_display (filename);
        Log_Print (LOG_ERROR, _("Error reading tags from file ‘%s’"),
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

    const TagLib::PropertyMap extra_tag = tag->properties ();

    /* Disc number. */
    /* Total disc number support in TagLib reads multiple disc numbers and
     * joins them with a "/". */
    if (extra_tag.contains ("DISCNUMBER"))
    {
        const TagLib::StringList disc_numbers = extra_tag["DISCNUMBER"];
        int offset = disc_numbers.front ().find ("/");

        if (offset != -1)
        {
            FileTag->disc_total = et_disc_number_to_string (disc_numbers.front ().substr (offset + 1).toInt ());
        }

        FileTag->disc_number = et_disc_number_to_string (disc_numbers.front ().toInt ());
    }

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
    if (extra_tag.contains ("TRACKNUMBER"))
    {
        const TagLib::StringList track_numbers = extra_tag["TRACKNUMBER"];
        int offset = track_numbers.front ().find ("/");

        if (offset != -1)
        {
            FileTag->track_total = et_track_number_to_string (track_numbers.front ().substr (offset + 1).toInt ());
        }

        FileTag->track = et_track_number_to_string (track_numbers.front ().toInt ());
    }

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
    if (extra_tag.contains ("COMPOSER"))
    {
        const TagLib::StringList composers = extra_tag["COMPOSER"];
        FileTag->composer = g_strdup (composers.front ().toCString (true));
    }

    /* Copyright. */
    if (extra_tag.contains ("COPYRIGHT"))
    {
        const TagLib::StringList copyrights = extra_tag["COPYRIGHT"];
        FileTag->copyright = g_strdup (copyrights.front ().toCString (true));
    }

    /*****************
     * Encoding Tool *
     *****************/
    if (extra_tag.contains ("ENCODEDBY"))
    {
        const TagLib::StringList encodedbys = extra_tag["ENCODEDBY"];
        FileTag->encoded_by = g_strdup (encodedbys.front ().toCString (true));
    }

    const TagLib::MP4::ItemListMap &extra_items = tag->itemListMap ();

    /****************
     * Album Artist *
     ****************/
    if (extra_items.contains ("aART"))
    {
        const TagLib::MP4::Item album_artists = extra_items["aART"];
        FileTag->album_artist = g_strdup (album_artists.toStringList ().front ().toCString (true));
    }

    /***********
     * Picture *
     ***********/
    if (extra_items.contains ("covr"))
    {
        const TagLib::MP4::Item cover = extra_items["covr"];
        const TagLib::MP4::CoverArt &art = cover.toCoverArtList ().front ();

        FileTag->picture = Picture_Allocate ();

        FileTag->picture->size = art.data ().size ();
        FileTag->picture->data = (guchar *)g_memdup (art.data ().data (),
                                                     art.data ().size ());
    }
    else
    {
        FileTag->picture = NULL;
    }

    return TRUE;
}


/*
 * Mp4_Tag_Write_File_Tag:
 *
 * Write tag data into an Mp4 file.
 */
gboolean Mp4tag_Write_File_Tag (ET_File *ETFile)
{
    File_Tag *FileTag;
    gchar    *filename;
    gchar    *filename_utf8;
    TagLib::MP4::Tag *tag;
    gboolean success;

    g_return_val_if_fail (ETFile != NULL && ETFile->FileTag != NULL, FALSE);

    FileTag = (File_Tag *)ETFile->FileTag->data;
    filename      = ((File_Name *)ETFile->FileNameCur->data)->value;
    filename_utf8 = ((File_Name *)ETFile->FileNameCur->data)->value_utf8;

    /* Open file for writing */
    GFile *file = g_file_new_for_path (filename);
    GIO_IOStream stream (file);

    if (!stream.isOpen ())
    {
        gchar *filename_utf8 = filename_to_display (filename);
        const GError *error = stream.getError ();
        Log_Print (LOG_ERROR, _("Error while opening file ‘%s’: %s"),
                   filename_utf8, error->message);
        g_free (filename_utf8);
        return FALSE;
    }

    TagLib::MP4::File mp4file (&stream);

    g_object_unref (file);

    if (!mp4file.isOpen ())
    {
        const GError *error = stream.getError ();

        if (error)
        {
            Log_Print (LOG_ERROR, _("Error while opening file ‘%s’: %s"),
                       filename_utf8, error->message);
        }
        else
        {
            Log_Print (LOG_ERROR, _("Error while opening file ‘%s’: %s"),
                       filename_utf8, _("MP4 format invalid"));
        }

        return FALSE;
    }

    if (!(tag = mp4file.tag ()))
    {
        gchar *filename_utf8 = filename_to_display (filename);
        Log_Print (LOG_ERROR, _("Error reading tags from file ‘%s’"),
                   filename_utf8);
        g_free (filename_utf8);
        return FALSE;
    }

    TagLib::PropertyMap fields;

    /*********
     * Title *
     *********/
    if (FileTag->title && *(FileTag->title))
    {
        TagLib::String string (FileTag->title, TagLib::String::UTF8);
        fields.insert ("TITLE", string);
    }

    /**********
     * Artist *
     **********/
    if (FileTag->artist && *(FileTag->artist))
    {
        TagLib::String string (FileTag->artist, TagLib::String::UTF8);
        fields.insert ("ARTIST", string);
    }

    /*********
     * Album *
     *********/
    if (FileTag->album && *(FileTag->album))
    {
        TagLib::String string (FileTag->album, TagLib::String::UTF8);
        fields.insert ("ALBUM", string);
    }

    /* Disc number. */
    if (FileTag->disc_number && *(FileTag->disc_number))
    {
        if (FileTag->disc_total && *(FileTag->disc_total))
        {
            gchar *str;

            str = g_strconcat (FileTag->disc_number, "/", FileTag->disc_total,
                               NULL);
            TagLib::String string (str, TagLib::String::UTF8);
            fields.insert ("DISCNUMBER", string);
            g_free (str);
        }
        else
        {
            TagLib::String string (FileTag->disc_number, TagLib::String::UTF8);
            fields.insert ("DISCNUMBER", string);
        }
    }

    /********
     * Year *
     ********/
    if (FileTag->year && *(FileTag->year))
    {
        TagLib::String string (FileTag->year, TagLib::String::UTF8);
        fields.insert ("DATE", string);
    }

    /*************************
     * Track and Total Track *
     *************************/
    if (FileTag->track && *(FileTag->track))
    {
        if (FileTag->track_total && *(FileTag->track_total))
        {
            gchar *str;

            str = g_strconcat (FileTag->track, "/", FileTag->track_total,
                               NULL);
            TagLib::String string (str, TagLib::String::UTF8);
            fields.insert ("TRACKNUMBER", string);
            g_free (str);
        }
        else
        {
            TagLib::String string (FileTag->track, TagLib::String::UTF8);
            fields.insert ("TRACKNUMBER", string);
        }
    }

    /*********
     * Genre *
     *********/
    if (FileTag->genre && *(FileTag->genre))
    {
        TagLib::String string (FileTag->genre, TagLib::String::UTF8);
        fields.insert ("GENRE", string);
    }

    /***********
     * Comment *
     ***********/
    if (FileTag->comment && *(FileTag->comment))
    {
        TagLib::String string (FileTag->comment, TagLib::String::UTF8);
        fields.insert ("COMMENT", string);
    }

    /**********************
     * Composer or Writer *
     **********************/
    if (FileTag->composer && *(FileTag->composer))
    {
        TagLib::String string (FileTag->composer, TagLib::String::UTF8);
        fields.insert ("COMPOSER", string);
    }

    /* Copyright. */
    if (FileTag->copyright && *(FileTag->copyright))
    {
        TagLib::String string (FileTag->copyright, TagLib::String::UTF8);
        fields.insert ("COPYRIGHT", string);
    }

    /*****************
     * Encoding Tool *
     *****************/
    if (FileTag->encoded_by && *(FileTag->encoded_by))
    {
        TagLib::String string (FileTag->encoded_by, TagLib::String::UTF8);
        fields.insert ("ENCODEDBY", string);
    }

    TagLib::MP4::ItemListMap &extra_items = tag->itemListMap ();

    /* Album artist. */
    /* FIXME: No "ALBUMARTIST" support in TagLib, use atom directly. */
    if (FileTag->album_artist && *(FileTag->album_artist))
    {
        TagLib::String string (FileTag->album_artist, TagLib::String::UTF8);
        extra_items.insert ("aART", TagLib::MP4::Item (string));
    }

    /***********
     * Picture *
     ***********/
    if (FileTag->picture)
    {
        Picture_Format pf;
        TagLib::MP4::CoverArt::Format f;

        pf = Picture_Format_From_Data (FileTag->picture);

        switch (pf)
        {
            case PICTURE_FORMAT_JPEG:
                f = TagLib::MP4::CoverArt::JPEG;
                break;
            case PICTURE_FORMAT_PNG:
                f = TagLib::MP4::CoverArt::PNG;
                break;
            case PICTURE_FORMAT_GIF:
                f = TagLib::MP4::CoverArt::GIF;
                break;
            default:
                g_critical ("Unknown format");
                f = TagLib::MP4::CoverArt::JPEG;
                break;
        }

        TagLib::MP4::CoverArt art (f, TagLib::ByteVector((char *)FileTag->picture->data,
                                                         FileTag->picture->size));

        extra_items.insert ("covr",
                            TagLib::MP4::Item (TagLib::MP4::CoverArtList ().append (art)));
    }

    tag->setProperties (fields);
    success = mp4file.save () ? TRUE : FALSE;

    return success;
}

#endif /* ENABLE_MP4 */
