/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
 * Copyright (C) 2001-2003  Jerome Couderc <easytag@gmail.com>
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

#include "config.h" // For definition of ENABLE_OGG

#ifdef ENABLE_OGG

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <unistd.h>

#include "easytag.h"
#include "ogg_tag.h"
#include "vcedit.h"
#include "et_core.h"
#include "log.h"
#include "misc.h"
#include "picture.h"
#include "setting.h"
#include "charset.h"

/* for mkstemp. */
#include "win32/win32dep.h"


/***************
 * Declaration *
 ***************/

#define MULTIFIELD_SEPARATOR " - "

/* Ogg Vorbis fields names in UTF-8 : http://www.xiph.org/vorbis/doc/v-comment.html
 *
 * Field names :
 *
 * Below is a proposed, minimal list of standard field names with a description of intended use. No single or group of field names is mandatory; a comment header may contain one, all or none of the names in this list.
 *
 * TITLE        : Track/Work name
 * VERSION      : The version field may be used to differentiate multiple versions of the same track title in a single collection. (e.g. remix info)
 * ALBUM        : The collection name to which this track belongs
 * TRACKNUMBER  : The track number of this piece if part of a specific larger collection or album
 * ARTIST       : The artist generally considered responsible for the work. In popular music this is usually the performing band or singer. For classical music it would be the composer. For an audio book it would be the author of the original text.
 * ALBUMARTIST  : The compilation artist or overall artist of an album
 * PERFORMER    : The artist(s) who performed the work. In classical music this would be the conductor, orchestra, soloists. In an audio book it would be the actor who did the reading. In popular music this is typically the same as the ARTIST and is omitted.
 * COPYRIGHT    : Copyright attribution, e.g., '2001 Nobody's Band' or '1999 Jack Moffitt'
 * LICENSE      : License information, eg, 'All Rights Reserved', 'Any Use Permitted', a URL to a license such as a Creative Commons license ("www.creativecommons.org/blahblah/license.html") or the EFF Open Audio License ('distributed under the terms of the Open Audio License. see http://www.eff.org/IP/Open_licenses/eff_oal.html for details'), etc.
 * ORGANIZATION : Name of the organization producing the track (i.e. the 'record label')
 * DESCRIPTION  : A short text description of the contents
 * GENRE        : A short text indication of music genre
 * DATE         : Date the track was recorded
 * LOCATION     : Location where track was recorded
 * CONTACT      : Contact information for the creators or distributors of the track. This could be a URL, an email address, the physical address of the producing label.
 * ISRC         : ISRC number for the track; see the ISRC intro page for more information on ISRC numbers.
 *
 * The remaining tags are multiples; if they are present more than once, all their occurances are considered significant.
 *
 * PUBLISHER   : who publishes the disc the track came from
 * DISCNUMBER  : if part of a multi-disc album, put the disc number here
 * EAN/UPN     : there may be a barcode on the CD; it is most likely an EAN or UPN (Universal Product Number).
 * LABEL       : the record label or imprint on the disc
 * LABELNO     : record labels often put the catalog number of the source media somewhere on the packaging. use this tag to record it.
 * OPUS        : the number of the work; eg, Opus 10, BVW 81, K6
 * SOURCEMEDIA : the recording media the track came from. eg, CD, Cassette, Radio Broadcast, LP, CD Single
 * TRACKTOTAL  :
 * ENCODED-BY  : The person who encoded the Ogg file. May include contact information such as email address and phone number.
 * ENCODING    : Put the settings you used to encode the Ogg file here. This tag is NOT expected to be stored or returned by cddb type databases. It includes information about the quality setting, bit rate, and bitrate management settings used to encode the Ogg. It also is used for information about which encoding software was used to do the encoding.
 * COMPOSER    : composer of the work. eg, Gustav Mahler
 * ARRANGER    : the person who arranged the piece, eg, Ravel
 * LYRICIST    : the person who wrote the lyrics, eg Donizetti
 * AUTHOR      : for text that is spoken, or was originally meant to be spoken, the author, eg JRR Tolkien
 * CONDUCTOR   : conductor of the work; eg Herbert von Karajan. choir directors would also use this tag.
 * PERFORMER   : individual performers singled out for mention; eg, Yoyo Ma (violinist)
 * ENSEMBLE    : the group playing the piece, whether orchestra, singing duo, or rock and roll band.
 * PART        : a division within a work; eg, a movement of a symphony. Some tracks contain several parts. Use a single PART tag for each part contained in a track. ie, PART="Oh sole mio"
 * PARTNUMBER  : The part number goes in here. You can use any format you like, such as Roman numerals, regular numbers, or whatever. The numbers should be entered in such a way that an alphabetical sort on this tag will correctly show the proper ordering of all the oggs that contain the contain the piece of music.
 * LOCATION    : location of recording, or other location of interest
 * COMMENT     : additional comments of any nature.
 */



/**************
 * Prototypes *
 **************/

static gboolean Ogg_Write_Delimetered_Tag (vorbis_comment *vc, const gchar *tag_name, gchar *values);
static gboolean Ogg_Write_Tag (vorbis_comment *vc, const gchar *tag_name, gchar *value);
static void Ogg_Set_Tag (vorbis_comment *vc, const gchar *tag_name, gchar *value, gboolean split);

/*************
 * Functions *
 *************/

/*
 * convert_to_byte_array:
 * @n: Integer to convert
 * @array: Destination byte array
 *
 * Converts an integer to byte array.
 */
static void
convert_to_byte_array (guint32 n, guchar *array)
{
    array [0] = (n >> 24) & 0xFF;
    array [1] = (n >> 16) & 0xFF;
    array [2] = (n >> 8) & 0xFF;
    array [3] = n & 0xFF;
}

/*
 * add_to_guchar_str:
 * @ustr: Destination string
 * @ustr_len: Pointer to length of destination string
 * @str: String to append
 * @str_len: Length of str
 *
 * Append a guchar string to given guchar string.
 */

static void
add_to_guchar_str (guchar *ustr, gsize *ustr_len, guchar *str, gsize str_len)
{
    gsize i;

    for (i = *ustr_len; i < *ustr_len + str_len; i++)
    {
        ustr [i] = str [i - *ustr_len];
    }

    *ustr_len += str_len;
}

/*
 * read_guint_from_byte:
 * @str: the byte string
 * @start: position to start with
 *
 * Reads and returns an integer from given byte string starting from start.
 * Returns: Integer which is read
 */
static guint32
read_guint32_from_byte (guchar *str, gsize start)
{
    gsize i;
    guint32 read = 0;

    for (i = start; i < start + 4; i++)
    {
        read = (read << 8) + str[i];
    }

    return read;
}

/*
 * et_add_file_tags_from_vorbis_comments:
 * @vc: the byte string
 * @FileTag: position to start with
 * @filename_utf8: display filename
 *
 * Reads vorbis comments and copies them to file tag.
 */

void
et_add_file_tags_from_vorbis_comments (vorbis_comment *vc, File_Tag *FileTag,
                                       const gchar *filename_utf8)
{
    gchar *string = NULL;
    gchar *string1 = NULL;
    gchar *string2 = NULL;
    guint field_num, i;
    Picture *prev_pic = NULL;

    /*********
     * Title *
     *********/
    /* Note : don't forget to add any new field to 'Save unsupported fields' */
    field_num = 0;
    while ( (string = vorbis_comment_query(vc,"TITLE",field_num++)) != NULL )
    {
        string = Try_To_Validate_Utf8_String(string);

        if ( g_utf8_strlen(string, -1) > 0 )
        {
            if (FileTag->title==NULL)
                FileTag->title = g_strdup(string);
            else
                FileTag->title = g_strconcat(FileTag->title,MULTIFIELD_SEPARATOR,string,NULL);
            // If strlen = 0, then no allocated data!
        }

        g_free(string);
    }

    /**********
     * Artist *
     **********/
    field_num = 0;
    while ( (string = vorbis_comment_query(vc,"ARTIST",field_num++)) != NULL )
    {
        string = Try_To_Validate_Utf8_String(string);

        if ( g_utf8_strlen(string, -1) > 0 )
        {
            if (FileTag->artist==NULL)
                FileTag->artist = g_strdup(string);
            else
                FileTag->artist = g_strconcat(FileTag->artist,MULTIFIELD_SEPARATOR,string,NULL);
        }

        g_free(string);
    }

    /****************
     * Album Artist *
     ****************/
    field_num = 0;
    while ( (string = vorbis_comment_query(vc,"ALBUMARTIST",field_num++)) != NULL )
    {
        string = Try_To_Validate_Utf8_String(string);

        if ( g_utf8_strlen(string, -1) > 0 )
        {
            if (FileTag->album_artist==NULL)
                FileTag->album_artist = g_strdup(string);
            else
                FileTag->album_artist = g_strconcat(FileTag->album_artist,MULTIFIELD_SEPARATOR,string,NULL);
        }

        g_free(string);
    }

    /*********
     * Album *
     *********/
    field_num = 0;
    while ( (string = vorbis_comment_query(vc,"ALBUM",field_num++)) != NULL )
    {
        string = Try_To_Validate_Utf8_String(string);

        if ( g_utf8_strlen(string, -1) > 0 )
        {
            if (FileTag->album==NULL)
                FileTag->album = g_strdup(string);
            else
                FileTag->album = g_strconcat(FileTag->album,MULTIFIELD_SEPARATOR,string,NULL);
        }

        g_free(string);
    }

    /**********************************************
     * Disc Number (Part of a Set) and Disc Total *
     **********************************************/
    if ((string = vorbis_comment_query (vc, "DISCNUMBER", 0)) != NULL
        && g_utf8_strlen (string, -1) > 0)
    {
        /* Check if DISCTOTAL used, else takes it in DISCNUMBER. */
        if ((string1 = vorbis_comment_query (vc, "DISCTOTAL", 0)) != NULL
            && g_utf8_strlen (string1, -1) > 0)
        {
            FileTag->disc_total = et_disc_number_to_string (atoi (string1));
        }
        else if ((string1 = g_utf8_strchr (string, -1, '/')))
        {
            FileTag->disc_total = et_disc_number_to_string (atoi (string1
                                                                  + 1));
            *string1 = '\0';
        }

        FileTag->disc_number = et_disc_number_to_string (atoi (string));
    }

    /********
     * Year *
     ********/
    if ( (string = vorbis_comment_query(vc,"DATE",0)) != NULL && g_utf8_strlen(string, -1) > 0 )
    {
        FileTag->year = g_strdup(string);

        if (g_utf8_strlen (FileTag->year, -1) > 4)
        {
            Log_Print (LOG_WARNING,
                       _("The year value ‘%s’ seems to be invalid in file ‘%s’. The information will be lost when saving"),
                       FileTag->year, filename_utf8);
        }
    }

    /*************************
     * Track and Total Track *
     *************************/
    if ( (string = vorbis_comment_query(vc,"TRACKNUMBER",0)) != NULL && g_utf8_strlen(string, -1) > 0 )
    {
        /* Check if TRACKTOTAL used, else takes it in TRACKNUMBER. */
        if ((string1 = vorbis_comment_query (vc, "TRACKTOTAL", 0)) != NULL
            && g_utf8_strlen (string1, -1) > 0)
        {
            FileTag->track_total = et_track_number_to_string (atoi (string1));
        }
        else if ((string1 = g_utf8_strchr (string, -1, '/')))
        {
            FileTag->track_total = et_track_number_to_string (atoi (string1
                                                                    + 1));
            *string1 = '\0';
        }
        FileTag->track = g_strdup (string);
    }

    /*********
     * Genre *
     *********/
    field_num = 0;
    while ( (string = vorbis_comment_query(vc,"GENRE",field_num++)) != NULL )
    {
        string = Try_To_Validate_Utf8_String(string);

        if ( g_utf8_strlen(string, -1) > 0 )
        {
            if (FileTag->genre==NULL)
                FileTag->genre = g_strdup(string);
            else
                FileTag->genre = g_strconcat(FileTag->genre,MULTIFIELD_SEPARATOR,string,NULL);
        }

        g_free(string);
    }

    /***********
     * Comment *
     ***********/
    field_num = 0;
    string1 = NULL; // Cause it may be not updated into the 'while' condition
    while ( ((string2 = vorbis_comment_query(vc,"DESCRIPTION",field_num)) != NULL )   // New specifications
         || ((string  = vorbis_comment_query(vc,"COMMENT",    field_num)) != NULL )   // Old : Winamp format (for EasyTAG 1.99.11 and older)
         || ((string1 = vorbis_comment_query(vc,"",           field_num)) != NULL ) ) // Old : Xmms format   (for EasyTAG 1.99.11 and older)
    {
        string  = Try_To_Validate_Utf8_String(string);
        string1 = Try_To_Validate_Utf8_String(string1);
        string2 = Try_To_Validate_Utf8_String(string2);

        if ( string2 && g_utf8_strlen(string2, -1) > 0 ) // Contains comment to new specifications format and we prefer this format (field name defined)
        {
            if (FileTag->comment==NULL)
                FileTag->comment = g_strdup(string2);
            else
                FileTag->comment = g_strconcat(FileTag->comment,MULTIFIELD_SEPARATOR,string2,NULL);

            // Frees allocated data
            if (string && g_utf8_strlen(string, -1) > 0)
                g_free(string);
            if (string1 && g_utf8_strlen(string1, -1) > 0)
                g_free(string1);
        }else if ( string && g_utf8_strlen(string, -1) > 0 ) // Contains comment to Winamp format and we prefer this format (field name defined)
        {
            if (FileTag->comment==NULL)
                FileTag->comment = g_strdup(string);
            else
                FileTag->comment = g_strconcat(FileTag->comment,MULTIFIELD_SEPARATOR,string,NULL);

            // Frees allocated data
            if (string1 && g_utf8_strlen(string1, -1) > 0)
                g_free(string1);
        }else if ( string1 && g_utf8_strlen(string1, -1) > 0 ) // Contains comment to Xmms format only
        {
            if (FileTag->comment==NULL)
                FileTag->comment = g_strdup(string1);
            else
                FileTag->comment = g_strconcat(FileTag->comment,MULTIFIELD_SEPARATOR,string1,NULL);
        }

        g_free(string);
        g_free(string1);
        g_free(string2);

        string  = NULL;
        string1 = NULL;
        field_num++;
    }

    /************
     * Composer *
     ************/
    field_num = 0;
    while ( (string = vorbis_comment_query(vc,"COMPOSER",field_num++)) != NULL )
    {
        string = Try_To_Validate_Utf8_String(string);

        if ( g_utf8_strlen(string, -1) > 0 )
        {
            if (FileTag->composer==NULL)
                FileTag->composer = g_strdup(string);
            else
                FileTag->composer = g_strconcat(FileTag->composer,MULTIFIELD_SEPARATOR,string,NULL);
        }

        g_free(string);
    }

    /*******************
     * Original artist *
     *******************/
    field_num = 0;
    while ( (string = vorbis_comment_query(vc,"PERFORMER",field_num++)) != NULL )
    {
        string = Try_To_Validate_Utf8_String(string);

        if ( g_utf8_strlen(string, -1) > 0 )
        {
            if (FileTag->orig_artist==NULL)
                FileTag->orig_artist = g_strdup(string);
            else
                FileTag->orig_artist = g_strconcat(FileTag->orig_artist,MULTIFIELD_SEPARATOR,string,NULL);
        }

        g_free(string);
    }

    /*************
     * Copyright *
     *************/
    field_num = 0;
    while ( (string = vorbis_comment_query(vc,"COPYRIGHT",field_num++)) != NULL )
    {
        string = Try_To_Validate_Utf8_String(string);

        if ( g_utf8_strlen(string, -1) > 0 )
        {
            if (FileTag->copyright==NULL)
                FileTag->copyright = g_strdup(string);
            else
                FileTag->copyright = g_strconcat(FileTag->copyright,MULTIFIELD_SEPARATOR,string,NULL);
        }

        g_free(string);
    }

    /*******
     * URL *
     *******/
    field_num = 0;
    while ((string = vorbis_comment_query (vc, "CONTACT", field_num++)) != NULL)
    {
        string = Try_To_Validate_Utf8_String(string);

        if ( g_utf8_strlen(string, -1) > 0 )
        {
            if (FileTag->url==NULL)
                FileTag->url = g_strdup(string);
            else
                FileTag->url = g_strconcat(FileTag->url,MULTIFIELD_SEPARATOR,string,NULL);
        }

        g_free(string);
    }

    /**************
     * Encoded by *
     **************/
    field_num = 0;
    while ( (string = vorbis_comment_query(vc,"ENCODED-BY",field_num++)) != NULL )
    {
        string = Try_To_Validate_Utf8_String(string);

        if ( g_utf8_strlen(string, -1) > 0 )
        {
            if (FileTag->encoded_by==NULL)
                FileTag->encoded_by = g_strdup(string);
            else
                FileTag->encoded_by = g_strconcat(FileTag->encoded_by,MULTIFIELD_SEPARATOR,string,NULL);
        }

        g_free(string);
    }


    /**************
     * Picture    *
     **************/
    /* Non officials tags used for picture information:
     *  - COVERART            : contains the picture data
     *  - COVERARTTYPE        : cover front, ...
     *  - COVERARTDESCRIPTION : information set by user
     *  - COVERARTMIME        : image/jpeg or image/png (written only)
     */
    field_num = 0;
    while ( (string = vorbis_comment_query(vc,"COVERART",field_num++)) != NULL )
    {
        Picture *pic;
            
        /* Force marking the file as modified, so that the deprecated cover art
         * field in converted in a METADATA_PICTURE_BLOCK field. */
        FileTag->saved = FALSE;

        pic = Picture_Allocate();
        if (!prev_pic)
            FileTag->picture = pic;
        else
            prev_pic->next = pic;
        prev_pic = pic;

        pic->data = NULL;
        
        // Decode picture data
        pic->data = g_base64_decode (string, &pic->size);

        if ( (string = vorbis_comment_query(vc,"COVERARTTYPE",field_num-1)) != NULL )
        {
            pic->type = atoi(string);
        }

        if ( (string = vorbis_comment_query(vc,"COVERARTDESCRIPTION",field_num-1)) != NULL )
        {
            pic->description = g_strdup(string);
        }

        //if ((string = vorbis_comment_query(vc,"COVERTARTMIME",field_num-1)) != NULL )
        //{
        //    pic->description = g_strdup(string);
        //}

    }

    /* METADATA_BLOCK_PICTURE tag used for picture information */
    field_num = 0;
    while ((string = vorbis_comment_query (vc, "METADATA_BLOCK_PICTURE",
                                           field_num++)) != NULL)
    {
        Picture *pic;
        gsize bytes_pos, mimelen, desclen;
        guchar *decoded_ustr;
        gsize decoded_len;

        pic = Picture_Allocate();

        if (!prev_pic)
        {
            FileTag->picture = pic;
        }
        else
        {
            prev_pic->next = pic;
        }

        prev_pic = pic;

        pic->data = NULL;

        /* Decode picture data. */
        decoded_ustr = g_base64_decode (string, &decoded_len);

        /* Reading picture type. */
        pic->type = read_guint32_from_byte (decoded_ustr, 0);
        bytes_pos = 4;

        /* Reading MIME data. */
        mimelen = read_guint32_from_byte (decoded_ustr, bytes_pos);
        bytes_pos = 8 + mimelen;

        /* Reading description */
        desclen = read_guint32_from_byte (decoded_ustr, bytes_pos);
        bytes_pos += 4;

        pic->description = g_malloc (desclen + 1);

        for (i = 0; i < desclen; i++)
        {
            pic->description [i] = decoded_ustr [i + bytes_pos];
        }

        pic->description [desclen] = '\0';
        bytes_pos += desclen + 16;

        /* Reading picture size */
        pic->size = read_guint32_from_byte (decoded_ustr, bytes_pos);
        bytes_pos += 4;

        /* Reading decoded picture */
        pic->data = g_malloc (pic->size);

        for (i = 0; i < pic->size; i++)
        {
            pic->data [i] = decoded_ustr [i + bytes_pos];
        }

        g_free (decoded_ustr);
    }

    /***************************
     * Save unsupported fields *
     ***************************/
    for (i=0;i<(guint)vc->comments;i++)
    {
        if ( strncasecmp(vc->user_comments[i],"TITLE=",            6) != 0
          && strncasecmp(vc->user_comments[i],"ARTIST=",           7) != 0
          && strncasecmp(vc->user_comments[i],"ALBUMARTIST=",     12) != 0
          && strncasecmp(vc->user_comments[i],"ALBUM=",            6) != 0
          && strncasecmp(vc->user_comments[i],"DISCNUMBER=",      11) != 0
          && strncasecmp(vc->user_comments[i],"DATE=",             5) != 0
          && strncasecmp(vc->user_comments[i],"TRACKNUMBER=",     12) != 0
          && strncasecmp(vc->user_comments[i],"TRACKTOTAL=",      11) != 0
          && strncasecmp(vc->user_comments[i],"GENRE=",            6) != 0
          && strncasecmp(vc->user_comments[i],"DESCRIPTION=",     12) != 0
          && strncasecmp(vc->user_comments[i],"COMMENT=",          8) != 0
          && strncasecmp(vc->user_comments[i],"=",                 1) != 0
          && strncasecmp(vc->user_comments[i],"COMPOSER=",         9) != 0
          && strncasecmp(vc->user_comments[i],"PERFORMER=",       10) != 0
          && strncasecmp(vc->user_comments[i],"COPYRIGHT=",       10) != 0
          && strncasecmp(vc->user_comments[i],"CONTACT=",          8) != 0
          && strncasecmp(vc->user_comments[i],"ENCODED-BY=",      11) != 0
          && strncasecmp(vc->user_comments[i],"COVERART=",         9) != 0
          && strncasecmp(vc->user_comments[i],"COVERARTTYPE=",       13) != 0
          && strncasecmp(vc->user_comments[i],"COVERARTMIME=",       13) != 0
          && strncasecmp(vc->user_comments[i],"COVERARTDESCRIPTION=",20) != 0
          && strncasecmp (vc->user_comments[i], "METADATA_BLOCK_PICTURE=", 23) != 0
           )
        {
            FileTag->other = g_list_append(FileTag->other,
                                           Try_To_Validate_Utf8_String(vc->user_comments[i]));
        }
    }
}

/*
 * Read tag data into an Ogg Vorbis file.
 * Note:
 *  - if field is found but contains no info (strlen(str)==0), we don't read it
 */
gboolean
ogg_tag_read_file_tag (const gchar *filename, File_Tag *FileTag, GError **error)
{
    GFile *file;
    GFileInputStream *istream;
    vcedit_state   *state;
    gchar *filename_utf8;

    g_return_val_if_fail (filename != NULL && FileTag != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    file = g_file_new_for_path (filename);
    istream = g_file_read (file, NULL, error);

    if (!istream)
    {
        g_object_unref (file);
        g_assert (error == NULL || *error != NULL);
        return FALSE;
    }

    filename_utf8 = filename_to_display (filename);

    {
    // Skip the id3v2 tag
    guchar tmp_id3[4];
    gulong id3v2size;

    // Check if there is an ID3v2 tag...
    if (!g_seekable_seek (G_SEEKABLE (istream), 0L, G_SEEK_SET, NULL, error))
    {
        goto err;
    }

    if (g_input_stream_read (G_INPUT_STREAM (istream), tmp_id3, 4, NULL,
                             error) == 4)
    {
        // Calculate ID3v2 length
        if (tmp_id3[0] == 'I' && tmp_id3[1] == 'D' && tmp_id3[2] == '3' && tmp_id3[3] < 0xFF)
        {
            // id3v2 tag skipeer $49 44 33 yy yy xx zz zz zz zz [zz size]
            /* Size is 6-9 position */
            if (!g_seekable_seek (G_SEEKABLE (istream), 2, G_SEEK_CUR,
                                  NULL, error))
            {
                goto err;
            }

            if (g_input_stream_read (G_INPUT_STREAM (istream), tmp_id3, 4,
                                     NULL, error) == 4)
            {
                id3v2size = 10 + ( (long)(tmp_id3[3])        | ((long)(tmp_id3[2]) << 7)
                                | ((long)(tmp_id3[1]) << 14) | ((long)(tmp_id3[0]) << 21) );

                if (!g_seekable_seek (G_SEEKABLE (istream), id3v2size,
                                      G_SEEK_SET, NULL, error))
                {
                    goto err;
                }

                Log_Print (LOG_ERROR,
                           _("The Ogg Vorbis file ‘%s’ contains an ID3v2 tag"),
                           filename_utf8);
            }
            else if (!g_seekable_seek (G_SEEKABLE (istream), 0L, G_SEEK_SET,
                                       NULL, error))
            {
                goto err;
            }

        }
        else if (!g_seekable_seek (G_SEEKABLE (istream), 0L, G_SEEK_SET,
                                   NULL, error))
        {
            goto err;
        }

    }
    else if (!g_seekable_seek (G_SEEKABLE (istream), 0L, G_SEEK_SET,
                               NULL, error))
    {
        goto err;
    }

    }

    g_assert (error == NULL || *error == NULL);

    g_object_unref (istream);

    state = vcedit_new_state();    // Allocate memory for 'state'

    if (!vcedit_open (state, file, error))
    {
        g_assert (error == NULL || *error != NULL);
        g_object_unref (file);
        vcedit_clear(state);
        g_free (filename_utf8);
        return FALSE;
    }

    g_assert (error == NULL || *error == NULL);

    /* Get data from tag */
    /*{
        gint i; 
        for (i=0;i<vc->comments;i++) 
            g_print("%s -> Ogg vc:'%s'\n",g_path_get_basename(filename),vc->user_comments[i]);
    }*/

    et_add_file_tags_from_vorbis_comments (vcedit_comments(state), FileTag,
                                           filename_utf8);
    vcedit_clear(state);
    g_object_unref (file);
    g_free(filename_utf8);

    return TRUE;

err:
    g_assert (error == NULL || *error != NULL);
    g_object_unref (istream);
    g_object_unref (istream);
    g_object_unref (file);
    g_free(filename_utf8);
    return FALSE;
}


/*
 * Save field value in separated tags if it contains multifields
 */
static gboolean Ogg_Write_Delimetered_Tag (vorbis_comment *vc, const gchar *tag_name, gchar *values)
{
    gchar **strings = g_strsplit(values,MULTIFIELD_SEPARATOR,255);
    unsigned int i=0;
    
    for (i=0;i<g_strv_length(strings);i++)
    {
        if (strlen(strings[i])>0)
        {
            Ogg_Write_Tag(vc,tag_name,strings[i]);
        }
    }
    g_strfreev(strings);
    return TRUE;
}

/*
 * Save field value in a single tag
 */
static gboolean Ogg_Write_Tag (vorbis_comment *vc, const gchar *tag_name, gchar *value)
{
    char *string = g_strconcat(tag_name,value,NULL);

    vorbis_comment_add(vc,string);
    g_free(string);
    return TRUE;
}

static void Ogg_Set_Tag (vorbis_comment *vc, const gchar *tag_name, gchar *value, gboolean split)
{
    if ( value && split ) {
        Ogg_Write_Delimetered_Tag(vc,tag_name,value);
    } else if ( value ) {
        Ogg_Write_Tag(vc,tag_name,value);
    }
}

gboolean
ogg_tag_write_file_tag (const ET_File *ETFile,
                        GError **error)
{
    const File_Tag *FileTag;
    const gchar *filename;
    const gchar *filename_utf8;
    GFile           *file;
    GFileInputStream *istream;
    vcedit_state   *state;
    vorbis_comment *vc;
    gchar          *string;
    GList *l;
    Picture        *pic;

    g_return_val_if_fail (ETFile != NULL && ETFile->FileTag != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    FileTag       = (File_Tag *)ETFile->FileTag->data;
    filename      = ((File_Name *)ETFile->FileNameCur->data)->value;
    filename_utf8 = ((File_Name *)ETFile->FileNameCur->data)->value_utf8;

    file = g_file_new_for_path (filename);
    istream = g_file_read (file, NULL, error);

    if (!istream)
    {
        g_object_unref (file);
        return FALSE;
    }

    {
    // Skip the id3v2 tag
    gsize bytes_read;
    guchar tmp_id3[4];
    gulong id3v2size;

    // Check if there is an ID3v2 tag...
    if (!g_seekable_seek (G_SEEKABLE (istream), 0L, G_SEEK_SET, NULL, error))
    {
        goto err;
    }

    if (g_input_stream_read_all (G_INPUT_STREAM (istream), tmp_id3, 4,
                                 &bytes_read, NULL, error))
    {
        // Calculate ID3v2 length
        if (tmp_id3[0] == 'I' && tmp_id3[1] == 'D' && tmp_id3[2] == '3' && tmp_id3[3] < 0xFF)
        {
            // id3v2 tag skipeer $49 44 33 yy yy xx zz zz zz zz [zz size]
            /* Size is 6-9 position */
            if (!g_seekable_seek (G_SEEKABLE (istream), 2, G_SEEK_CUR,
                                  NULL, error))
            {
                goto err;
            }

            if (g_input_stream_read_all (G_INPUT_STREAM (istream), tmp_id3, 4,
                                         &bytes_read, NULL, error))
            {
                id3v2size = 10 + ( (long)(tmp_id3[3])        | ((long)(tmp_id3[2]) << 7)
                                | ((long)(tmp_id3[1]) << 14) | ((long)(tmp_id3[0]) << 21) );

                if (!g_seekable_seek (G_SEEKABLE (istream), id3v2size,
                                      G_SEEK_SET, NULL, error))
                {
                    goto err;
                }

                Log_Print (LOG_ERROR,
                           _("The Ogg Vorbis file ‘%s’ contains an ID3v2 tag"),
                           filename_utf8);
            }
            else
            {
                g_debug ("Only %" G_GSIZE_FORMAT " bytes out of 4 bytes of "
                         "data were read", bytes_read);
                goto err;
            }

        }
        else if (!g_seekable_seek (G_SEEKABLE (istream), 0L, G_SEEK_SET,
                                   NULL, error))
        {
            goto err;
        }

    }
    else
    {
        g_debug ("Only %" G_GSIZE_FORMAT " bytes out of 4 bytes of data were "
                 "read", bytes_read);
        goto err;
    }

    }

    g_assert (error == NULL || *error == NULL);

    g_object_unref (istream);

    state = vcedit_new_state();    // Allocate memory for 'state'

    if (!vcedit_open (state, file, error))
    {
        g_assert (error == NULL || *error != NULL);
        g_object_unref (file);
        vcedit_clear(state);
        return FALSE;
    }

    g_assert (error == NULL || *error == NULL);

    /* Get data from tag */
    vc = vcedit_comments(state);
    vorbis_comment_clear(vc);
    vorbis_comment_init(vc);

    /*********
     * Title *
     *********/
    Ogg_Set_Tag (vc, "TITLE=", FileTag->title,
                 g_settings_get_boolean (MainSettings, "ogg-split-title"));

    /**********
     * Artist *
     **********/
    Ogg_Set_Tag (vc, "ARTIST=", FileTag->artist,
                 g_settings_get_boolean (MainSettings, "ogg-split-artist"));

    /****************
     * Album Artist *
     ****************/
    Ogg_Set_Tag (vc, "ALBUMARTIST=", FileTag->album_artist,
                 g_settings_get_boolean (MainSettings, "ogg-split-artist"));

    /*********
     * Album *
     *********/
    Ogg_Set_Tag (vc, "ALBUM=", FileTag->album,
                 g_settings_get_boolean (MainSettings, "ogg-split-album"));

    /***************
     * Disc Number *
     ***************/
    Ogg_Set_Tag(vc,"DISCNUMBER=",FileTag->disc_number,FALSE);
    Ogg_Set_Tag (vc, "DISCTOTAL=", FileTag->disc_total, FALSE);

    /********
     * Year *
     ********/
    Ogg_Set_Tag(vc,"DATE=",FileTag->year,FALSE);

    /*************************
     * Track and Total Track *
     *************************/
    Ogg_Set_Tag(vc,"TRACKNUMBER=",FileTag->track,FALSE);

    Ogg_Set_Tag(vc,"TRACKTOTAL=",FileTag->track_total,FALSE);

    /*********
     * Genre *
     *********/
    Ogg_Set_Tag (vc, "GENRE=", FileTag->genre,
                 g_settings_get_boolean (MainSettings, "ogg-split-genre"));

    /***********
     * Comment *
     ***********/
    /* Format of new specification. */
    Ogg_Set_Tag (vc, "DESCRIPTION=", FileTag->comment,
                 g_settings_get_boolean (MainSettings, "ogg-split-comment"));

    /************
     * Composer *
     ************/
    Ogg_Set_Tag (vc ,"COMPOSER=", FileTag->composer,
                 g_settings_get_boolean (MainSettings, "ogg-split-composer"));

    /*******************
     * Original artist *
     *******************/
    Ogg_Set_Tag (vc, "PERFORMER=", FileTag->orig_artist,
                 g_settings_get_boolean (MainSettings,
                                         "ogg-split-original-artist"));

    /*************
     * Copyright *
     *************/
    Ogg_Set_Tag(vc,"COPYRIGHT=",FileTag->copyright,FALSE);

    /*******
     * URL *
     *******/
    Ogg_Set_Tag (vc, "CONTACT=", FileTag->url, FALSE);

    /**************
     * Encoded by *
     **************/
    Ogg_Set_Tag(vc,"ENCODED-BY=",FileTag->encoded_by,FALSE);
    
    
    /***********
     * Picture *
     ***********/
    for (pic = FileTag->picture; pic != NULL; pic = pic->next)
    {
        if (pic->data)
        {
            const gchar *mime;
            guchar array[4];
            guchar *ustring = NULL;
            gsize ustring_len = 0;
            gchar *base64_string;
            gsize desclen;
            Picture_Format format = Picture_Format_From_Data (pic);

            /* According to the specification, only PNG and JPEG images should
             * be added to Vorbis comments. */
            if (format != PICTURE_FORMAT_PNG && format != PICTURE_FORMAT_JPEG)
            {
                GdkPixbufLoader *loader;
                GError *error = NULL;

                loader = gdk_pixbuf_loader_new ();

                if (!gdk_pixbuf_loader_write (loader, pic->data, pic->size,
                                              &error))
                {
                    Log_Print (LOG_ERROR, _("Error parsing image data ‘%s’"),
                               error->message);
                    g_error_free (error);
                    g_object_unref (loader);
                    continue;
                }
                else
                {
                    GdkPixbuf *pixbuf;
                    gchar *buffer;
                    gsize buffer_size;

                    if (!gdk_pixbuf_loader_close (loader, &error))
                    {
                        Log_Print (LOG_ERROR,
                                   _("Error parsing image data ‘%s’"),
                                   error->message);
                        g_error_free (error);
                        g_object_unref (loader);
                        continue;
                    }

                    pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

                    if (!pixbuf)
                    {
                        g_object_unref (loader);
                        continue;
                    }

                    g_object_ref (pixbuf);
                    g_object_unref (loader);

                    /* Always convert to PNG. */
                    if (!gdk_pixbuf_save_to_buffer (pixbuf, &buffer,
                                                    &buffer_size, "png",
                                                    &error, NULL))
                    {
                        g_debug ("Error while converting image to PNG: %s",
                                 error->message);
                        g_error_free (error);
                        g_object_unref (pixbuf);
                        continue;
                    }

                    g_object_unref (pixbuf);

                    g_free (pic->data);
                    pic->data = (guchar *)buffer;
                    pic->size = buffer_size;

                    /* Set the picture format to reflect the new data. */
                    format = Picture_Format_From_Data (pic);
                }
            }

            mime = Picture_Mime_Type_String (format);

            /* Calculating full length of byte string and allocating. */
            desclen = pic->description ? strlen (pic->description) : 0;
            ustring = g_malloc (4 * 8 + strlen (mime) + desclen + pic->size);

            /* Adding picture type. */
            convert_to_byte_array (pic->type, array);
            add_to_guchar_str (ustring, &ustring_len, array, 4);

            /* Adding MIME string and its length. */
            convert_to_byte_array (strlen (mime), array);
            add_to_guchar_str (ustring, &ustring_len, array, 4);
            add_to_guchar_str (ustring, &ustring_len, (guchar *)mime,
                               strlen (mime));

            /* Adding picture description. */
            convert_to_byte_array (desclen, array);
            add_to_guchar_str (ustring, &ustring_len, array, 4);
            add_to_guchar_str (ustring, &ustring_len,
                               (guchar *)pic->description,
                               desclen);

            /* Adding width, height, color depth, indexed colors. */
            convert_to_byte_array (pic->width, array);
            add_to_guchar_str (ustring, &ustring_len, array, 4);

            convert_to_byte_array (pic->height, array);
            add_to_guchar_str (ustring, &ustring_len, array, 4);

            convert_to_byte_array (0, array);
            add_to_guchar_str (ustring, &ustring_len, array, 4);

            convert_to_byte_array (0, array);
            add_to_guchar_str (ustring, &ustring_len, array, 4);

            /* Adding picture data and its size. */
            convert_to_byte_array (pic->size, array);
            add_to_guchar_str (ustring, &ustring_len, array, 4);

            add_to_guchar_str (ustring, &ustring_len, pic->data, pic->size);

            base64_string = g_base64_encode (ustring, ustring_len);
            string = g_strconcat ("METADATA_BLOCK_PICTURE=", base64_string,
                                  NULL);
            vorbis_comment_add (vc, string);

            g_free (base64_string);
            g_free (ustring);
            g_free (string);
        }
    }

    /**************************
     * Set unsupported fields *
     **************************/
    for (l = FileTag->other; l != NULL; l = g_list_next (l))
    {
        if (l->data)
        {
            vorbis_comment_add (vc, (gchar *)l->data);
        }
    }

    /* Write tag to 'file' in all cases */
    if (!vcedit_write (state, file, error))
    {
        g_assert (error == NULL || *error != NULL);
        g_object_unref (file);
        vcedit_clear(state);
        return FALSE;
    }
    else
    {
        vcedit_clear (state);
    }

    g_assert (error == NULL || *error == NULL);
    return TRUE;
    
err:
    g_assert (error == NULL || *error != NULL);
    g_object_unref (istream);
    g_object_unref (file);
    return FALSE;
}
#endif /* ENABLE_OGG */
