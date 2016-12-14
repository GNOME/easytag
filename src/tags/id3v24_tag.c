/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014-2016  David King <amigadave@amigadave.com>
 * Copyright (C) 2001-2003  Jerome Couderc <easytag@gmail.com>
 * Copyright (C) 2006-2007  Alexey Illarionov <littlesavage@rambler.ru>
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

#include "config.h"

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#include "id3_tag.h"
#include "picture.h"
#include "browser.h"
#include "setting.h"
#include "misc.h"
#include "et_core.h"
#include "charset.h"

#include "win32/win32dep.h"


#ifdef ENABLE_MP3

#include <id3tag.h>
#include "genres.h"


/****************
 * Declarations *
 ****************/
#define MULTIFIELD_SEPARATOR " - "
#define EASYTAG_STRING_ENCODEDBY "Encoded by"

enum {
    EASYTAG_ID3_FIELD_LATIN1        = 0x0001,
    EASYTAG_ID3_FIELD_LATIN1FULL    = 0x0002,
    EASYTAG_ID3_FIELD_LATIN1LIST    = 0x0004,
    EASYTAG_ID3_FIELD_STRING        = 0x0008,
    EASYTAG_ID3_FIELD_STRINGFULL    = 0x0010,
    EASYTAG_ID3_FIELD_STRINGLIST    = 0x0020,
    EASYTAG_ID3_FIELD_LANGUAGE      = 0x0040
};

/**************
 * Prototypes *
 **************/
static int    etag_guess_byteorder      (const id3_ucs4_t *ustr, gchar **ret);
static int    etag_ucs42gchar           (const id3_ucs4_t *usrc, unsigned is_latin, unsigned is_utf16, gchar **res);
static int    libid3tag_Get_Frame_Str   (const struct id3_frame *frame, unsigned etag_field_type, gchar **retstr);

static void   Id3tag_delete_frames      (struct id3_tag *tag, const gchar *name, int start);
static void   Id3tag_delete_txxframes   (struct id3_tag *tag, const gchar *param1, int start);
static struct id3_frame *Id3tag_find_and_create_frame    (struct id3_tag *tag, const gchar *name);
static int    id3taglib_set_field       (struct id3_frame *frame, const gchar *str, enum id3_field_type type, int num, int clear, int id3v1);
static int    etag_set_tags             (const gchar *str, const char *frame_name, enum id3_field_type field_type, struct id3_tag *v1tag, struct id3_tag *v2tag, gboolean *strip_tags);
static gboolean etag_write_tags (const gchar *filename, struct id3_tag const *v1tag,
                            struct id3_tag const *v2tag, gboolean strip_tags, GError **error);

/*************
 * Functions *
 *************/

/*
 * Read id3v1.x / id3v2 tag and load data into the File_Tag structure.
 * Returns TRUE on success, else FALSE.
 * If a tag entry exists (ex: title), we allocate memory, else value stays to NULL
 */
gboolean
id3tag_read_file_tag (GFile *gfile,
                      File_Tag *FileTag,
                      GError **error)
{
    GInputStream *istream;
    gsize bytes_read;
    GSeekable *seekable;
    gchar *filename;
    int fd;
    struct id3_file *file;
    struct id3_tag *tag;
    struct id3_frame *frame;
    union id3_field *field;
    gchar *string1, *string2;
    EtPicture *prev_pic = NULL;
    int i, j;
    unsigned tmpupdate, update = 0;
    long tagsize;

    g_return_val_if_fail (gfile != NULL && FileTag != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    istream = G_INPUT_STREAM (g_file_read (gfile, NULL, error));

    if (!istream)
    {
        return FALSE;
    }

    string1 = g_malloc0 (ID3_TAG_QUERYSIZE);

    /* Check if the file has an ID3v2 tag or/and an ID3v1 tags.
     * 1) ID3v2 tag. */
    if (!g_input_stream_read_all (istream, string1, ID3_TAG_QUERYSIZE,
                                  &bytes_read, NULL, error))
    {
        g_object_unref (istream);
        g_free (string1);
        return FALSE;
    }
    else if (bytes_read != ID3_TAG_QUERYSIZE)
    {
        g_object_unref (istream);
        g_free (string1);
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_PARTIAL_INPUT, "%s",
                     _("Error reading tags from file"));
        return FALSE;
    }

    if ((tagsize = id3_tag_query((id3_byte_t const *)string1, ID3_TAG_QUERYSIZE)) <= ID3_TAG_QUERYSIZE)
    {
        /* ID3v2 tag not found! */
        update = g_settings_get_boolean (MainSettings, "id3v2-enabled");
    }else
    {
        /* ID3v2 tag found */
        if (!g_settings_get_boolean (MainSettings, "id3v2-enabled"))
        {
            /* To delete the tag. */
            update = 1;
        }else
        {
            /* Determine version if user want to upgrade old tags */
            if (g_settings_get_boolean (MainSettings, "id3v2-convert-old")
            && (string1 = g_realloc (string1, tagsize))
                && g_input_stream_read_all (istream,
                                            &string1[ID3_TAG_QUERYSIZE],
                                            tagsize - ID3_TAG_QUERYSIZE,
                                            &bytes_read, NULL, error)
                && bytes_read == (gsize)(tagsize - ID3_TAG_QUERYSIZE)
            && (tag = id3_tag_parse((id3_byte_t const *)string1, tagsize))
               )
            {
                unsigned version = id3_tag_version(tag);
#ifdef ENABLE_ID3LIB
                /* Besides upgrade old tags we will downgrade id3v2.4 to id3v2.3 */
                if (g_settings_get_boolean (MainSettings, "id3v2-version-4"))
                {
                    update = (ID3_TAG_VERSION_MAJOR(version) < 4);
                }else
                {
                    update = ((ID3_TAG_VERSION_MAJOR(version) < 3)
                            | (ID3_TAG_VERSION_MAJOR(version) == 4));
                }
#else
                update = (ID3_TAG_VERSION_MAJOR(version) < 4);
#endif
                id3_tag_delete(tag);
            }
        }
    }

    /* 2) ID3v1 tag. */
    seekable = G_SEEKABLE (istream);

    if (!g_seekable_can_seek (seekable))
    {
        g_object_unref (istream);
        g_free (string1);
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_PARTIAL_INPUT, "%s",
                     _("Error reading tags from file"));
        return FALSE;
    }

    /* Go to the beginning of ID3v1 tag. */
    if (g_seekable_seek (seekable, -ID3V1_TAG_SIZE, G_SEEK_END, NULL, error)
    && (string1)
        && g_input_stream_read_all (istream, string1, 3, &bytes_read, NULL,
                                    NULL /* Ignore errors. */)
        && bytes_read == 3
    && (string1[0] == 'T')
    && (string1[1] == 'A')
    && (string1[2] == 'G')
       )
    {
        /* ID3v1 tag found! */
        if (!g_settings_get_boolean (MainSettings, "id3v1-enabled"))
        {
            update = 1;
        }
    }else
    {
        /* ID3v1 tag not found! */
        if (g_settings_get_boolean (MainSettings, "id3v1-enabled"))
        {
            update = 1;
        }
    }

    g_free (string1);
    g_object_unref (istream);

    filename = g_file_get_path (gfile);

    if ((fd = g_open (filename, O_RDONLY, 0)) == -1)
    {
        g_free (filename);
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                     _("Error reading tags from file"));
        return FALSE;
    }

    g_free (filename);

    /* The fd ownership is transferred to id3tag. */
    if ((file = id3_file_fdopen (fd, ID3_FILE_MODE_READONLY)) == NULL)
    {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                     _("Error reading tags from file"));
        return FALSE;
    }

    if ( ((tag = id3_file_tag(file)) == NULL)
    ||   (tag->nframes == 0))
    {
        id3_file_close(file);
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                     _("Error reading tags from file"));
        return FALSE;
    }


    /****************
     * Title (TIT2) *
     ****************/
    if ( (frame = id3_tag_findframe(tag, ID3_FRAME_TITLE, 0)) )
        update |= libid3tag_Get_Frame_Str(frame, EASYTAG_ID3_FIELD_STRINGLIST, &FileTag->title);

    /*****************
     * Artist (TPE1) *
     *****************/
    if ( (frame = id3_tag_findframe(tag, ID3_FRAME_ARTIST, 0)) )
        update |= libid3tag_Get_Frame_Str(frame, EASYTAG_ID3_FIELD_STRINGLIST, &FileTag->artist);

    /*****************
     * Album Artist (TPE2) *
     *****************/
    if ( (frame = id3_tag_findframe(tag, "TPE2", 0)) )
        update |= libid3tag_Get_Frame_Str(frame, EASYTAG_ID3_FIELD_STRINGLIST, &FileTag->album_artist);

    /****************
     * Album (TALB) *
     ****************/
    if ( (frame = id3_tag_findframe(tag, ID3_FRAME_ALBUM, 0)) )
        update |= libid3tag_Get_Frame_Str(frame, ~0, &FileTag->album);

    /************************
     * Part of a set (TPOS) *
     ************************/
    if ((frame = id3_tag_findframe (tag, "TPOS", 0)))
    {
        update |= libid3tag_Get_Frame_Str (frame, ~0, &string1);

        if (string1)
        {
            string2 = strchr (string1, '/');

            if (string2)
            {
                FileTag->disc_total = et_disc_number_to_string (atoi (string2
                                                                      + 1));
                *string2 = '\0';
            }

            FileTag->disc_number = et_disc_number_to_string (atoi (string1));
            g_free (string1);
        }
    }

    /********************
     * Year (TYER/TDRC) *
     ********************/
    if ( (frame = id3_tag_findframe(tag, ID3_FRAME_YEAR, 0)) )
    {
        update |= libid3tag_Get_Frame_Str(frame, ~0, &string1);
        if ( string1 )
        {
            g_strstrip (string1);
            FileTag->year = string1;
        }
    }

    /********************************
     * Track and Total Track (TRCK) *
     ********************************/
    if ( (frame = id3_tag_findframe(tag, ID3_FRAME_TRACK, 0)) )
    {
        update |= libid3tag_Get_Frame_Str(frame, ~0, &string1);
        if ( string1 )
        {
            string2 = strchr (string1, '/');

            if (string2)
            {
                FileTag->track_total = et_track_number_to_string (atoi (string2 + 1));
                *string2 = '\0'; // To cut string1
            }
            FileTag->track = et_track_number_to_string (atoi (string1));

            g_free(string1);

        }
    }

    /****************
     * Genre (TCON) *
     ****************/
    if ( (frame = id3_tag_findframe(tag, ID3_FRAME_GENRE, 0)) )
    {
        update |= libid3tag_Get_Frame_Str(frame, ~0, &string1);
        if ( string1 )
        {
            /*
             * We manipulate only the name of the genre
             * Genre is written like this :
             *    - "(<genre_id>)"              -> "(3)"
             *    - "<genre_name>"              -> "Dance"
             *    - "(<genre_id>)<refinement>"  -> "(3)EuroDance"
             */
            gchar *tmp;
            unsigned genre = 0;
            FileTag->genre = NULL;

            if ((string1[0] == '(') && g_ascii_isdigit (string1[1])
                && (tmp = strchr (string1 + 1, ')')) && *(tmp+1))
                /* Convert a genre written as '(3)EuroDance' into 'EuroDance' */
            {
                FileTag->genre = g_strdup (tmp + 1);
            }
            else if ((string1[0] == '(') && g_ascii_isdigit (string1[1])
                     && strchr (string1, ')'))
            {
                /* Convert a genre written as '(3)' into 'Dance' */
                genre = strtol (string1 +1 , &tmp, 10);
                if (*tmp != ')')
                {
                    FileTag->genre = g_strdup (string1);
                }
            }
            else
            {
                genre = strtol (string1, &tmp, 10);
                if (tmp == string1)
                {
                    FileTag->genre = g_strdup (string1);
                }
            }

            if (!FileTag->genre)
            {
                if (id3_genre_index (genre))
                {
                    FileTag->genre = (gchar *)id3_ucs4_utf8duplicate (id3_genre_index (genre));
                }
                else if (strcmp (genre_no (genre),
                                 genre_no (ID3_INVALID_GENRE)) != 0)
                {
                    /* If the integer genre is not found in the (outdated)
                     * libid3tag index, try the EasyTAG index instead. */
                    FileTag->genre = g_strdup (genre_no (genre));
                }
            }

            g_free(string1);
        }
    }

    /******************
     * Comment (COMM) *
     ******************/
    if ( (frame = id3_tag_findframe(tag, ID3_FRAME_COMMENT, 0)) )
    {
        update |= libid3tag_Get_Frame_Str(frame, /* EASYTAG_ID3_FIELD_STRING | */ EASYTAG_ID3_FIELD_STRINGFULL,
            &FileTag->comment);
        /*{
            gchar *comment1 = Id3tag_Get_Field(frame,ID3FN_DESCRIPTION)
            gchar *comment2 = Id3tag_Get_Field(id3_frame,ID3FN_LANGUAGE)
        }*/
    }

    /*******************
     * Composer (TCOM) *
     *******************/
    if ( (frame = id3_tag_findframe(tag, "TCOM", 0)) )
        update |= libid3tag_Get_Frame_Str(frame, ~0, &FileTag->composer);

    /**************************
     * Original artist (TOPE) *
     **************************/
    if ( (frame = id3_tag_findframe(tag, "TOPE", 0)) )
        update |= libid3tag_Get_Frame_Str(frame, ~0, &FileTag->orig_artist);

    /*******************
     * Copyright (TCOP)*
     *******************/
    if ( (frame = id3_tag_findframe(tag, "TCOP", 0)) )
        update |= libid3tag_Get_Frame_Str(frame, ~0, &FileTag->copyright);

    /**************
     * URL (WXXX) *
     **************/
    if ( (frame = id3_tag_findframe(tag, "WXXX", 0)) )
        update |= libid3tag_Get_Frame_Str(frame, EASYTAG_ID3_FIELD_LATIN1, &FileTag->url);

    /*******************************
     * Encoded by (TENC) or (TXXX) *
     *******************************/
    if ( (frame = id3_tag_findframe(tag, "TENC", 0)) )
        update |= libid3tag_Get_Frame_Str(frame, ~0, &FileTag->encoded_by);
    
    /* Encoded by in TXXX frames */
    string1 = NULL;
    for (i = 0; (frame = id3_tag_findframe(tag, "TXX", i)); i++)
    {
        // Do nothing if already read...
        if (FileTag->encoded_by)
            break;
        
        tmpupdate = libid3tag_Get_Frame_Str(frame, ~0, &string1);
        if (string1)
        {
            if (strncmp (string1,
                         EASYTAG_STRING_ENCODEDBY MULTIFIELD_SEPARATOR,
                         strlen (EASYTAG_STRING_ENCODEDBY MULTIFIELD_SEPARATOR)) == 0)
            {
                FileTag->encoded_by = g_strdup(&string1[sizeof(EASYTAG_STRING_ENCODEDBY) + sizeof(MULTIFIELD_SEPARATOR) - 2]);
                g_free(string1);
                update |= tmpupdate;
            }else
                g_free(string1);
        }
    }

    /******************
     * Picture (APIC) *
     ******************/
    for (i = 0; (frame = id3_tag_findframe(tag, "APIC", i)); i++)
    {
        GBytes *bytes = NULL;
        EtPictureType type = ET_PICTURE_TYPE_FRONT_COVER;
        gchar *description;
        EtPicture *pic;

        /* Picture file data. */
        for (j = 0; (field = id3_frame_field(frame, j)); j++)
        {
            enum id3_field_type field_type;

            field_type = id3_field_type (field);

            if (field_type == ID3_FIELD_TYPE_BINARYDATA)
            {
                id3_length_t size;
                id3_byte_t const *data;

                data = id3_field_getbinarydata (field, &size);

                if (data)
                {
                    if (bytes)
                    {
                        g_bytes_unref (bytes);
                    }

                    bytes = g_bytes_new (data, size);
                }
            }
            else if (field_type == ID3_FIELD_TYPE_INT8)
            {
                type = id3_field_getint (field);
            }
        }

        /* Picture description. The accepted fields are restricted to those
         * of string type, as the description field is the only one of string
         * type in the APIC tag (the MIME type is Latin1 type). */
        update |= libid3tag_Get_Frame_Str (frame, EASYTAG_ID3_FIELD_STRING,
                                           &description);

        pic = et_picture_new (type, description ? description : "", 0, 0,
                              bytes);
        g_bytes_unref (bytes);
        g_free (description);

        if (!prev_pic)
        {
            FileTag->picture = pic;
        }
        else
        {
            prev_pic->next = pic;
        }

        prev_pic = pic;
    }

    /**********************
     * Lyrics (SYLC/USLT) *
     **********************/
    /** see also id3/misc_support.h  **
    if ( (id3_frame = ID3Tag_FindFrameWithID(id3_tag,ID3FID_SYNCEDLYRICS)) )
    {
        gulong  size = 0;
        guchar *data = NULL;
        gchar  *description = NULL;
        gchar  *language = NULL;
        gint timestamp_format = 0;
        gint sync_type = 0;

        // SyncLyrics data
        if ( (id3_field = ID3Frame_GetField(id3_frame, ID3FN_DATA)) )
        {
            size = ID3Field_Size(id3_field);
            data = g_malloc(size);
            ID3Field_GetBINARY(id3_field, data, size);
        }

        // SyncLyrics description
        description = Id3tag_Get_Field(id3_frame, ID3FN_DESCRIPTION);

        // SyncLyrics language
        language = Id3tag_Get_Field(id3_frame, ID3FN_LANGUAGE);

        // SyncLyrics timestamp field
        if ( (id3_field = ID3Frame_GetField(id3_frame, ID3FN_TIMESTAMPFORMAT)) )
        {
            timestamp_format = ID3Field_GetINT(id3_field);
        }

        // SyncLyrics content type
        if ( (id3_field = ID3Frame_GetField(id3_frame, ID3FN_CONTENTTYPE)) )
        {
            sync_type = ID3Field_GetINT(id3_field);
        }

        // Print data
        // j.a. Pouwelse - pouwelse :
        //      http://sourceforge.net/tracker/index.php?func=detail&aid=401873&group_id=979&atid=300979
        {
            char tag[255];
            unsigned int time;
            luint pos = 0;

            g_print("SyncLyrics/description      : %s\n",description);
            g_print("SyncLyrics/language         : %s\n",language);
            g_print("SyncLyrics/timestamp format : %s (%d)\n",timestamp_format==ID3TSF_FRAME ? "ID3TSF_FRAME" : timestamp_format==ID3TSF_MS ? "ID3TSF_MS" : "" ,timestamp_format);
            g_print("SyncLyrics/sync type        : %s (%d)\n",sync_type==ID3CT_LYRICS ? "ID3CT_LYRICS" : "",sync_type);


            g_print("SyncLyrics size             : %d\n", size);
            g_print("Lyrics/data :\n");
            while (pos < size)
            {
                strcpy(tag,data+pos);
                //g_print("txt start=%d ",pos);
                pos+=strlen(tag)+1;             // shift string and terminating \0
                //g_print("txt end=%d ",pos);
                memcpy(&time,data+pos,4);
                pos+=4;
                //g_print("%d -> %s\n",time,tag);
                g_print("%s",tag);
            }
        }

    } **/

    if (update)
        FileTag->saved = FALSE;

    /* Free allocated data */
    id3_file_close(file);

    return TRUE;
}


/* Guess byteorder of UTF-16 string that was converted to 'ustr' (some ID3
 * tags contain UTF-16 string without BOM and in fact can be UTF16BE and
 * UTF-16LE). Function correct byteorder, if it is needed, and return new
 * corrected utf-8 string in 'ret'.
 * Return value of function is 0 if byteorder was not changed
 */
static int
etag_guess_byteorder(const id3_ucs4_t *ustr, gchar **ret) /* XXX */
{
    unsigned i, len;
    gunichar *gstr;
    gchar *tmp, *str, *str2;
    gchar *charset;

    if (!ustr || !*ustr)
    {
        if (ret)
            *ret = NULL;
        return 0;
    }

    if (g_settings_get_boolean (MainSettings, "id3-override-read-encoding"))
    {
        charset = g_settings_get_string (MainSettings,
                                         "id3v1v2-charset");
    }
    else if (!g_settings_get_boolean (MainSettings, "id3v2-enable-unicode"))
    {
        charset = g_settings_get_string (MainSettings,
                                         "id3v2-no-unicode-charset"); /* XXX */
    }
    else
    {
        const gchar *temp;
        g_get_charset (&temp);
        charset = g_strdup (temp);
    }

    if (!charset)
    {
        charset = g_strdup ("ISO-8859-1");
    }

    tmp = (gchar *)id3_ucs4_utf8duplicate(ustr);
    str = g_convert(tmp, -1, charset, "UTF-8", NULL, NULL, NULL);
    if (str)
    {
        g_free(str);
        if (ret)
            *ret = tmp;
        else
            free (tmp);
        g_free (charset);
        return 0; /* byteorder not changed */
    }

    for (len = 0; ustr[len]; len++);

    gstr = g_try_malloc(sizeof(gunichar) * (len + 1));
    if ( gstr == NULL )
    {
        if (ret)
            *ret = tmp;
        else
            free(tmp);
        g_free (charset);
        return 0;
    }

    for (i = 0; i <= len; i++)
        gstr[i] = ((ustr[i] & 0xff00) >> 8) | ((ustr[i] & 0xff) << 8);
    str = g_ucs4_to_utf8(gstr, len, NULL, NULL, NULL);
    g_free(gstr);

    if (str == NULL)
    {
        if (ret)
            *ret = tmp;
        else
            free(tmp);
        g_free (charset);
        return 0;
    }

    str2 = g_convert(str, -1, charset, "UTF-8", NULL, NULL, NULL);

    if (str2 && *str2)
    {
        g_free(str2);
        free(tmp);
        if (ret)
            *ret = str;
        else
            free(str);
        g_free (charset);
        return 1;
    }

    g_free(str);

    if (ret)
        *ret = tmp;
    else
        free(tmp);

    g_free (charset);
    return 0;
}


/* convert ucs4 string to utf-8 gchar in 'res' according to easytag charset
 * conversion settings and field type.
 * function return 0 if byteorder of utf-16 string was changed
 */
static int
etag_ucs42gchar(const id3_ucs4_t *usrc, unsigned is_latin,
                unsigned is_utf16, gchar **res)
{
    gchar *latinstr, *retstr;
    int retval;

    if (!usrc || !*usrc)
    {
        if (res)
            *res = NULL;
        return 0;
    }

    retval = 0, retstr = NULL;

    if (is_latin && g_settings_get_boolean (MainSettings,
                                            "id3-override-read-encoding"))
    {
        if ((latinstr = (gchar *)id3_ucs4_latin1duplicate(usrc)))
        {
            gint id3v1v2_charset;
            const gchar * charset;

            id3v1v2_charset = g_settings_get_enum (MainSettings,
                                                   "id3v1v2-charset");
            charset = et_charset_get_name_from_index (id3v1v2_charset);
            retstr = convert_string (latinstr, charset, "UTF-8", FALSE);
            free(latinstr);
        }
    }else
    {
        if (is_utf16)
        {
            retval |= etag_guess_byteorder(usrc, &retstr);
        }else
        {
            retstr = (gchar *)id3_ucs4_utf8duplicate(usrc);
        }
    }

    if (res)
        *res = retstr;
    else
        free (retstr);

    return retval;
}


static int
libid3tag_Get_Frame_Str (const struct id3_frame *frame,
                         unsigned etag_field_type,
                         gchar **retstr)
{
    const union id3_field *field;
    unsigned i;
    gchar *ret;
    unsigned is_latin, is_utf16;
    unsigned retval;

    ret = NULL;
    retval = 0;
    is_latin = 1, is_utf16 = 0;

    /* Find the encoding used for the field. */
    for (i = 0; (field = id3_frame_field (frame, i)); i++)
    {
        if (id3_field_type(field) == ID3_FIELD_TYPE_TEXTENCODING)
        {
            is_latin = (id3_field_gettextencoding(field) == ID3_FIELD_TEXTENCODING_ISO_8859_1);
            is_utf16 = (id3_field_gettextencoding(field) == ID3_FIELD_TEXTENCODING_UTF_16);
            break;
        }
    }

    for (i = 0; (field = id3_frame_field (frame, i)); i++)
    {
        unsigned field_type;
        gchar *tmpstr = NULL;

        switch (field_type = id3_field_type(field))
        {
            case ID3_FIELD_TYPE_LATIN1:
            case ID3_FIELD_TYPE_LATIN1FULL:
            {
                gchar *latinstr;

                if (field_type == ID3_FIELD_TYPE_LATIN1)
                {
                    if (!(etag_field_type & EASYTAG_ID3_FIELD_LATIN1))
                        continue;
                }
                else
                {
                    if (!(etag_field_type & EASYTAG_ID3_FIELD_LATIN1FULL))
                        continue;
                }

                latinstr = g_strdup (field_type == ID3_FIELD_TYPE_LATIN1 ? (const gchar *)id3_field_getlatin1 (field)
                                                                         : (const gchar *)id3_field_getfulllatin1 (field));

                if (g_settings_get_boolean (MainSettings,
                                            "id3-override-read-encoding"))
                {
                    gint id3v1v2_charset;
                    const gchar * charset;

                    id3v1v2_charset = g_settings_get_enum (MainSettings,
                                                           "id3v1v2-charset");
                    charset = et_charset_get_name_from_index (id3v1v2_charset);
                    tmpstr = convert_string (latinstr, charset, "UTF-8", FALSE);
                    g_free (latinstr);
                }
                else
                {
                    tmpstr = latinstr;
                }

                break;
            }

            case ID3_FIELD_TYPE_STRING:
            case ID3_FIELD_TYPE_STRINGFULL:
            {
                const id3_ucs4_t *usrc;

                if (field_type == ID3_FIELD_TYPE_STRING)
                {
                    if (!(etag_field_type & EASYTAG_ID3_FIELD_STRING))
                        continue;
                }
                else
                {
                    if (!(etag_field_type & EASYTAG_ID3_FIELD_STRINGFULL))
                        continue;
                }

                usrc = (field_type == ID3_FIELD_TYPE_STRING) ? id3_field_getstring (field)
                                                             : id3_field_getfullstring (field);
                retval |= etag_ucs42gchar (usrc, is_latin, is_utf16, &tmpstr);
                break;
            }

            case ID3_FIELD_TYPE_STRINGLIST:
            {
                unsigned strcnt, j;

                if (!(etag_field_type & EASYTAG_ID3_FIELD_STRINGLIST))
                    continue;

                strcnt = id3_field_getnstrings (field);

                for (j = 0; j < strcnt; j++)
                {
                    gchar *tmpstr2 = NULL;

                    retval |= etag_ucs42gchar (id3_field_getstrings (field, j),
                                               is_latin, is_utf16, &tmpstr2);

                    if (tmpstr2 && *tmpstr2 && g_utf8_validate(tmpstr2, -1, NULL))
                    {
                        if (tmpstr)
                        {
                            gchar *to_free = tmpstr;
                            tmpstr = g_strconcat (tmpstr, " ", tmpstr2, NULL);

                            g_free (to_free);
                        }
                        else
                        {
                            tmpstr = g_strdup (tmpstr2);
                        }
                    }

                    free (tmpstr2);
                }

                break;
            }

            default:
                break;
        }

        if (!et_str_empty (tmpstr) && g_utf8_validate (tmpstr, -1, NULL))
        {
            if (ret)
            {
                gchar *to_free = ret;
                ret = g_strconcat (ret, MULTIFIELD_SEPARATOR, tmpstr, NULL);
                g_free (to_free);
            }
            else
            {
                ret = g_strdup (tmpstr);
            }
        }

        g_free (tmpstr);
    }

    if (retstr)
    {
        *retstr = ret;
    }
    else
    {
        g_free (ret);
    }

    return retval;
}


/*
 * Write the ID3 tags to the file. Returns TRUE on success, else 0.
 */
gboolean
id3tag_write_file_v24tag (const ET_File *ETFile,
                          GError **error)
{
    const File_Tag *FileTag;
    const gchar *filename;
    struct id3_tag   *v1tag, *v2tag;
    struct id3_frame *frame;
    union id3_field  *field;
    gchar            *string1;
    EtPicture          *pic;
    gboolean strip_tags = TRUE;
    guchar genre_value = ID3_INVALID_GENRE;
    gboolean success;

    g_return_val_if_fail (ETFile != NULL && ETFile->FileTag != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    FileTag       = (File_Tag *)ETFile->FileTag->data;
    filename      = ((File_Name *)ETFile->FileNameCur->data)->value;

    v1tag = v2tag = NULL;

    /* Write ID3v2 tag. */
    if (g_settings_get_boolean (MainSettings, "id3v2-enabled"))
    {
        int fd;
        struct id3_file *file;
        struct id3_tag *tmptag;
        unsigned tagsize;
        id3_byte_t *tmpbuf = NULL;

        if ((fd = g_open (filename, O_RDONLY, 0)) == -1)
        {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                         _("Error reading tags from file"));
            return FALSE;
        }

        /* Read old v2 tag */
        if ((file = id3_file_fdopen (fd, ID3_FILE_MODE_READONLY)) == NULL)
        {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                         _("Error reading tags from file"));
            return FALSE;
        }

        if ((tmptag = id3_file_tag(file)) == NULL)
        {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                         _("Error reading tags from file"));
            id3_file_close(file);
            return FALSE;
        }

        id3_tag_options(tmptag, ID3_TAG_OPTION_UNSYNCHRONISATION
                              | ID3_TAG_OPTION_ID3V1 
                              | ID3_TAG_OPTION_COMPRESSION 
                              | ID3_TAG_OPTION_APPENDEDTAG,
                        //ID3_TAG_OPTION_UNSYNCHRONISATION); // Taglib doesn't support frames with unsynchronisation (patch from Alexey Illarionov) http://bugs.kde.org/show_bug.cgi?id=138829
                        0);

        /* XXX Create new tag and copy all frames*/
        tagsize = id3_tag_render(tmptag, NULL);
        if ((tagsize > 10)
        && (tmpbuf = g_try_malloc(tagsize))
        && (id3_tag_render(tmptag, tmpbuf) != 0)
        )
            v2tag = id3_tag_parse(tmpbuf, tagsize);
        g_free(tmpbuf);

        if (v2tag == NULL)
        {
            if ((v2tag = id3_tag_new()) == NULL)
            {
                g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                             _("Error reading tags from file"));
                id3_file_close(file);
                return FALSE;
            }
        }

        id3_file_close(file);

        /* Set padding  XXX */
        if ((v2tag->paddedsize < 1024)
        || ((v2tag->paddedsize > 4096) && (tagsize < 1024))
        )
            v2tag->paddedsize = 1024;

        /* Set options */
        id3_tag_options(v2tag, ID3_TAG_OPTION_UNSYNCHRONISATION
                             | ID3_TAG_OPTION_APPENDEDTAG
                             | ID3_TAG_OPTION_ID3V1
                             | ID3_TAG_OPTION_CRC
                             | ID3_TAG_OPTION_COMPRESSION,
                        //ID3_TAG_OPTION_UNSYNCHRONISATION); // Taglib doesn't support frames with unsynchronisation (patch from Alexey Illarionov) http://bugs.kde.org/show_bug.cgi?id=138829
                        0);
        
        if (g_settings_get_boolean (MainSettings, "id3v2-crc32"))
        {
            id3_tag_options (v2tag, ID3_TAG_OPTION_CRC, ~0);
        }
        if (g_settings_get_boolean (MainSettings, "id3v2-compression"))
        {
            id3_tag_options (v2tag, ID3_TAG_OPTION_COMPRESSION, ~0);
        }
    }

    /* Write ID3v1 tag. */
    if (g_settings_get_boolean (MainSettings, "id3v1-enabled"))
    {
        v1tag = id3_tag_new();
        if (!v1tag)
        {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s",
                         _("Error reading tags from file"));
            return FALSE;
        }
        
        id3_tag_options(v1tag, ID3_TAG_OPTION_ID3V1, ~0);
    }


    /*********
     * Title *
     *********/
    etag_set_tags(FileTag->title, ID3_FRAME_TITLE, ID3_FIELD_TYPE_STRINGLIST, v1tag, v2tag, &strip_tags);

    /**********
     * Artist *
     **********/
    etag_set_tags(FileTag->artist, ID3_FRAME_ARTIST, ID3_FIELD_TYPE_STRINGLIST, v1tag, v2tag, &strip_tags);

    /**********
     * Album Artist *
     **********/
    etag_set_tags(FileTag->album_artist, "TPE2", ID3_FIELD_TYPE_STRINGLIST, NULL, v2tag, &strip_tags);

    /*********
     * Album *
     *********/
    etag_set_tags(FileTag->album, ID3_FRAME_ALBUM, ID3_FIELD_TYPE_STRINGLIST, v1tag, v2tag, &strip_tags);

    /***************
     * Part of set *
     ***************/
    string1 = et_id3tag_get_tpos_from_file_tag (FileTag);
    etag_set_tags (string1, "TPOS", ID3_FIELD_TYPE_STRINGLIST, NULL, v2tag,
                   &strip_tags);
    g_free (string1);

    /********
     * Year *
     ********/
    etag_set_tags(FileTag->year, ID3_FRAME_YEAR, ID3_FIELD_TYPE_STRINGLIST, v1tag, v2tag, &strip_tags);

    /*************************
     * Track and Total Track *
     *************************/
    if ( FileTag->track 
    &&   FileTag->track_total 
    &&  *FileTag->track_total )
        string1 = g_strconcat(FileTag->track,"/",FileTag->track_total,NULL);
    else
        string1 = NULL;

    etag_set_tags(string1 ? string1 : FileTag->track, ID3_FRAME_TRACK, ID3_FIELD_TYPE_STRINGLIST, NULL, v2tag, &strip_tags);
    etag_set_tags(FileTag->track, ID3_FRAME_TRACK, ID3_FIELD_TYPE_STRINGLIST, v1tag, NULL, &strip_tags);
    g_free(string1);

    /*********
     * Genre *
     *********/
    /* Genre is written like this :
     *    - "<genre_id>"    -> "3"
     *    - "<genre_name>"  -> "EuroDance"
     */
    if (FileTag->genre)
        genre_value = Id3tag_String_To_Genre(FileTag->genre);

    if ((genre_value == ID3_INVALID_GENRE)
        || g_settings_get_boolean (MainSettings, "id3v2-text-only-genre"))
    {
        etag_set_tags (FileTag->genre, ID3_FRAME_GENRE,
                       ID3_FIELD_TYPE_STRINGLIST, v1tag, v2tag, &strip_tags);
    }
    else
    {
        /* The ID3v1 genre must always be given as a plain string, and
         * libid3tag does the appropriate conversion. */
        etag_set_tags (FileTag->genre, ID3_FRAME_GENRE,
                       ID3_FIELD_TYPE_STRINGLIST, v1tag, NULL, &strip_tags);

        /* Only the ID3v2 tag is converted to the bracketed form. */
        string1 = g_strdup_printf ("(%d)",genre_value);
        etag_set_tags (string1, ID3_FRAME_GENRE,
                       ID3_FIELD_TYPE_STRINGLIST, NULL, v2tag, &strip_tags);
        g_free (string1);
    }

    /***********
     * Comment *
     ***********/
    etag_set_tags(FileTag->comment, ID3_FRAME_COMMENT, ID3_FIELD_TYPE_STRINGFULL, v1tag, v2tag, &strip_tags);

    /************
     * Composer *
     ************/
    etag_set_tags(FileTag->composer, "TCOM", ID3_FIELD_TYPE_STRINGLIST, NULL, v2tag, &strip_tags);

    /*******************
     * Original artist *
     *******************/
    etag_set_tags(FileTag->orig_artist, "TOPE", ID3_FIELD_TYPE_STRINGLIST, NULL, v2tag, &strip_tags);

    /*************
     * Copyright *
     *************/
    etag_set_tags(FileTag->copyright, "TCOP", ID3_FIELD_TYPE_STRINGLIST, NULL, v2tag, &strip_tags);

    /*******
     * URL *
     *******/
    etag_set_tags(FileTag->url, "WXXX", ID3_FIELD_TYPE_LATIN1, NULL, v2tag, &strip_tags);

    /***************
     * Encoded by  *
     ***************/
    //if ( v2tag && FileTag->encoded_by && *FileTag->encoded_by
    //&& (frame = Id3tag_find_and_create_txxframe(v2tag, EASYTAG_STRING_ENCODEDBY)))
    //{
    //    id3taglib_set_field(frame, EASYTAG_STRING_ENCODEDBY, ID3_FIELD_TYPE_STRING, 0, 1, 0);
    //    id3taglib_set_field(frame, FileTag->encoded_by, ID3_FIELD_TYPE_STRING, 1, 0, 0);
    //    strip_tags = FALSE;
    //}else
    // Save encoder name in TENC frame instead of the TXX frame
    etag_set_tags(FileTag->encoded_by, "TENC", ID3_FIELD_TYPE_STRINGLIST, NULL, v2tag, &strip_tags);
    if (v2tag)
        Id3tag_delete_txxframes(v2tag, EASYTAG_STRING_ENCODEDBY, 0);

    /***********
     * Picture *
     ***********/
    Id3tag_delete_frames(v2tag, "APIC", 0);

    if (v2tag)
    {
        for (pic = FileTag->picture; pic != NULL; pic = pic->next)
        {
            gint i;

            if ((frame = id3_frame_new("APIC")) == NULL)
                continue;

            id3_tag_attachframe(v2tag, frame);
            for (i = 0; (field = id3_frame_field(frame, i)); i++)
            {
                Picture_Format format;
                enum id3_field_type field_type;

                field_type = id3_field_type (field);
                
                if (field_type == ID3_FIELD_TYPE_LATIN1)
                {
                    format = Picture_Format_From_Data(pic);
                    id3_field_setlatin1(field, (id3_latin1_t const *)Picture_Mime_Type_String(format));
                }
                else if (field_type == ID3_FIELD_TYPE_INT8)
                {
                    id3_field_setint (field, pic->type);
                }
                else if (field_type == ID3_FIELD_TYPE_BINARYDATA)
                {
                    gconstpointer data;
                    gsize data_size;

                    data = g_bytes_get_data (pic->bytes, &data_size);
                    id3_field_setbinarydata (field, data, data_size);
                }
            }

            if (pic->description)
                id3taglib_set_field(frame, pic->description, ID3_FIELD_TYPE_STRING, 0, 0, 0);

            strip_tags = FALSE;
        }
    }

    /****************************************
     * File length (in milliseconds) DISCARD*
     ****************************************/

    /*********************************
     * Update id3v1.x and id3v2 tags *
     *********************************/
    success = etag_write_tags (filename, v1tag, v2tag, strip_tags, error);

    // Free data
    if (v1tag)
        id3_tag_delete(v1tag);
    if (v2tag)
        id3_tag_delete(v2tag);

    return success;
}

/* Dele all frames with 'name'
 * begining with frame num 'start' (0-based)
 * from tag 'tag'
 */
static void
Id3tag_delete_frames(struct id3_tag *tag, const gchar *name, int start)
{
    struct id3_frame *frame;

    if (!tag || !name || !*name)
    return;

    while ((frame = id3_tag_findframe(tag, name, start)))
    {
        id3_tag_detachframe(tag, frame);
        id3_frame_delete(frame);
    }

}

static void
Id3tag_delete_txxframes(struct id3_tag *tag, const gchar *param1, int start)
{
    int i;
    struct id3_frame *frame;
    union id3_field *field;
    const id3_ucs4_t *ucs4string;
    gchar *str;

    if (!tag || !param1 || !*param1)
    return;

    for (i = start; (frame = id3_tag_findframe(tag, "TXXX", i)); )
        if ( (field = id3_frame_field(frame, 1))
        && (ucs4string = id3_field_getstring(field)) )
        {
            str = NULL;
            if ((str = (gchar *)id3_ucs4_latin1duplicate(ucs4string))
            && (g_ascii_strncasecmp (str, param1, strlen (param1)) == 0))
            {
                g_free(str);
                id3_tag_detachframe(tag, frame);
                id3_frame_delete(frame);
            }else
            {
                i++;
                g_free(str);
            }
        }else
            i++;
}

/* Find first frame with name 'name' in tag 'tag'
 * create new if not found
 */
static struct id3_frame *
Id3tag_find_and_create_frame (struct id3_tag *tag, const gchar *name)
{
    struct id3_frame *frame;

    g_return_val_if_fail (tag != NULL && name != NULL && *name != 0, NULL);

    frame = id3_tag_findframe(tag, name, 0);
    if (!frame)
    {
        if ((frame = id3_frame_new(name)) == NULL)
            return NULL;
        id3_tag_attachframe(tag, frame);
    }

    return frame;
}

static int
id3taglib_set_field(struct id3_frame *frame,
                    const gchar *str,
                    enum id3_field_type type,
                    int num, int clear, int id3v1)
{
    union id3_field    *field;
    enum id3_field_type    curtype;
    id3_ucs4_t        *buf;
    gchar        *latinstr, *encname;
    enum id3_field_textencoding enc_field;
    unsigned    i;
    unsigned    is_set;

    latinstr = NULL, buf = NULL;
    is_set = 0;
    enc_field = ID3_FIELD_TEXTENCODING_ISO_8859_1;

    if (str)
    {
        /* Prepare str according to encoding conversion settings. */
        if ((!g_settings_get_boolean (MainSettings, "id3v2-enable-unicode"))
            || (type == ID3_FIELD_TYPE_LATIN1)
            || (type == ID3_FIELD_TYPE_LATIN1FULL)
            || id3v1)
        {
            encname = NULL;
            /* id3v1 fields converted using its own character set and iconv options */
            if ( id3v1 )
            {
                gint id3v1_charset;
                const gchar *charset;
                EtTagEncoding iconv_option;

                id3v1_charset = g_settings_get_enum (MainSettings,
                                                     "id3v1-charset");
                charset = et_charset_get_name_from_index (id3v1_charset);
                iconv_option = g_settings_get_enum (MainSettings,
                                                    "id3v1-encoding-option");

                if (iconv_option != ET_TAG_ENCODING_NONE)
                {
                    encname = g_strconcat (charset,
                                           iconv_option == ET_TAG_ENCODING_TRANSLITERATE ? "//TRANSLIT" : "//IGNORE",
                                           NULL);
                }
                else
                {
                    encname = g_strdup (charset);
                }
            }
            else
            {
                /* latin1 fields (such as URL) always converted with ISO-8859-1*/
                if ((type != ID3_FIELD_TYPE_LATIN1) && (type != ID3_FIELD_TYPE_LATIN1FULL))
                {
                    gint id3v2_charset;
                    const gchar *charset;
                    EtTagEncoding iconv_option;

                    id3v2_charset = g_settings_get_enum (MainSettings,
                                                         "id3v2-no-unicode-charset");
                    charset = et_charset_get_name_from_index (id3v2_charset);
                    iconv_option = g_settings_get_enum (MainSettings,
                                                        "id3v2-encoding-option");
                    if (iconv_option != ET_TAG_ENCODING_NONE)
                    {
                        encname = g_strconcat (charset,
                                               iconv_option == ET_TAG_ENCODING_TRANSLITERATE ? "//TRANSLIT" : "//IGNORE",
                                               NULL);
                    }
                    else
                    {
                        encname = g_strdup (charset);
                    }
                }
            }

            latinstr = convert_string(str, "UTF-8", encname ? encname : "ISO-8859-1//IGNORE", TRUE);
            g_free (encname);
            buf = id3_latin1_ucs4duplicate((id3_latin1_t const *)latinstr);
        }
        else
        {
            gchar *charset;

            charset = g_settings_get_string (MainSettings,
                                             "id3v2-unicode-charset");

            if (!strcmp (charset, "UTF-16"))
            {
                enc_field = ID3_FIELD_TEXTENCODING_UTF_16;
                buf = id3_utf8_ucs4duplicate((id3_utf8_t const *)str);
            }
            else
            {
                enc_field = ID3_FIELD_TEXTENCODING_UTF_8;
                buf = id3_utf8_ucs4duplicate((id3_utf8_t const *)str);
            }

            g_free (charset);
        }
    }

    if (frame)
        frame->flags &= ~ID3_FRAME_FLAG_FORMATFLAGS;

    for (i = 0; (field = id3_frame_field(frame, i)); i++)
    {
        if (is_set && !clear)
            break;

        curtype = id3_field_type (field);

        if (curtype == ID3_FIELD_TYPE_TEXTENCODING)
        {
            id3_field_settextencoding (field, enc_field);
        }
        else if (curtype == ID3_FIELD_TYPE_LATIN1)
        {
            if (clear)
            {
                id3_field_setlatin1 (field, NULL);
            }

            if ((type == curtype) && !is_set)
            {
                if (num == 0)
                {
                    id3_field_setlatin1 (field,
                                         (id3_latin1_t const *)latinstr);
                    is_set = 1;
                }
                else
                {
                    num--;
                }
            }
        }
        else if (curtype == ID3_FIELD_TYPE_LATIN1FULL)
        {
                if (clear)
                {
                    id3_field_setfulllatin1 (field, NULL);
                }
                if ((type == curtype) && !is_set)
                {
                    if (num == 0)
                    {
                        id3_field_setfulllatin1 (field,
                                                 (id3_latin1_t const *)latinstr);
                        is_set = 1;
                    }
                    else
                    {
                        num--;
                    }
                }
        }
        else if (curtype == ID3_FIELD_TYPE_STRING)
        {
                if (clear)
                {
                    id3_field_setstring (field, NULL);
                }

                if ((type == curtype) && !is_set)
                {
                    if (num == 0)
                    {
                        id3_field_setstring(field, buf);
                        is_set = 1;
                    }
                    else
                    {
                        num--;
                    }
                }
        }
        else if (curtype == ID3_FIELD_TYPE_STRINGFULL)
        {
            if (clear)
            {
                id3_field_setfullstring (field, NULL);
            }

            if ((type == curtype) && !is_set)
            {
                if (num == 0)
                {
                    id3_field_setfullstring (field, buf);
                    is_set = 1;
                }
                else
                {
                    num--;
                }
            }
        }
        else if (curtype == ID3_FIELD_TYPE_STRINGLIST)
        {
            if (clear)
            {
                id3_field_setstrings (field, 0, NULL);
            }

            if ((type == curtype) && !is_set)
            {
                if ((num == 0) && (buf))
                {
                    id3_field_addstring (field, buf);
                    is_set = 1;
                }
                else
                {
                    num--;
                }
            }
        }

        if (is_set)
        {
            free(latinstr);
            free(buf);
            latinstr = NULL, buf = NULL;
        }
    }

    if (latinstr || buf)
    {
        free(latinstr);
        free(buf);
        return 1;
    } else
        return 0;
}


static int
etag_set_tags (const gchar *str,
               const char *frame_name,
               enum id3_field_type field_type,
               struct id3_tag *v1tag,
               struct id3_tag *v2tag,
              gboolean *strip_tags)
{
    struct id3_frame *ftmp;

    if ( str && *str )
    {
        *strip_tags = FALSE;

        if (v2tag
        && (ftmp = Id3tag_find_and_create_frame(v2tag, frame_name)))
            id3taglib_set_field(ftmp, str, field_type, 0, 1, 0);
        if (v1tag
        && (ftmp = Id3tag_find_and_create_frame(v1tag, frame_name)))
            id3taglib_set_field(ftmp, str, field_type, 0, 1, 1);
    }else
    {
        if (v2tag)
            Id3tag_delete_frames(v2tag, frame_name, 0);
    }

    return 0;
}

static gboolean
etag_write_tags (const gchar *filename, 
                 struct id3_tag const *v1tag,
                 struct id3_tag const *v2tag,
                 gboolean strip_tags,
                 GError **error)
{
    id3_byte_t *v1buf, *v2buf;
    id3_length_t v1size = 0, v2size = 0;
    gchar tmp[ID3_TAG_QUERYSIZE];
    GFile *file;
    GFileIOStream *iostream;
    GSeekable *seekable;
    GInputStream *istream;
    GOutputStream *ostream;
    long filev2size;
    gchar *audio_buffer = NULL;
    gboolean success = TRUE;
    gsize bytes_read;
    gsize bytes_written;

    v1buf = v2buf = NULL;

    if (!strip_tags)
    {
        /* Render v1 tag */
        if (v1tag)
        {
            v1size = id3_tag_render (v1tag, NULL);

            if (v1size == ID3V1_TAG_SIZE)
            {
                v1buf = g_malloc (v1size);

                if (id3_tag_render (v1tag, v1buf) != v1size)
                {
                    /* NOTREACHED */
                    g_free (v1buf);
                    v1buf = NULL;
                }
            }
        }

        /* Render v2 tag */
        if (v2tag)
        {
            v2size = id3_tag_render (v2tag, NULL);

            if (v2size > 10)
            {
                v2buf = g_malloc0 (v2size);

                if ((v2size = id3_tag_render (v2tag, v2buf)) == 0)
                {
                    /* NOTREACHED */
                    g_free (v2buf);
                    v2buf = NULL;
                }
            }
        }
    }
    
    if (v1buf == NULL)
    {
        v1size = 0;
    }
    if (v2buf == NULL)
    {
        v2size = 0;
    }

    file = g_file_new_for_path (filename);
    iostream = g_file_open_readwrite (file, NULL, error);

    if (!iostream)
    {
        goto err;
    }

    /* Seeking on the IOStream seeks to the same position on both the input and
     * output streams. */
    seekable = G_SEEKABLE (iostream);

    if (!g_seekable_can_seek (seekable))
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_BADF, "%s",
                     g_strerror (EBADF));
        goto err;
    }

    success = FALSE;

    /* Handle ID3v1 tag */
    if (!g_seekable_seek (seekable, -ID3V1_TAG_SIZE, G_SEEK_END, NULL, error))
    {
        goto err;
    }

    istream = g_io_stream_get_input_stream (G_IO_STREAM (iostream));

    if (!g_input_stream_read_all (istream, tmp, ID3_TAG_QUERYSIZE, &bytes_read,
                                  NULL, error))
    {
        goto err;
    }

    /* Seek to the beginning of the ID3v1 tag, if it exists. */
    if ((tmp[0] == 'T') && (tmp[1] == 'A') && (tmp[2] == 'G'))
    {
        if (!g_seekable_seek (seekable, -ID3V1_TAG_SIZE, G_SEEK_END, NULL,
                              error))
        {
            goto err;
        }
    }
    else
    {
        if (!g_seekable_seek (seekable, 0, G_SEEK_END, NULL, error))
        {
            goto err;
        }
    }

    /* Search ID3v2 tags at the end of the file (before any ID3v1 tag) */
    /* XXX: Unsafe */
    if (g_seekable_seek (seekable, -ID3_TAG_QUERYSIZE, G_SEEK_CUR, NULL,
                         error))
    {
        if (!g_input_stream_read_all (istream, tmp, ID3_TAG_QUERYSIZE,
                                      &bytes_read, NULL, error))
        {
            goto err;
        }

        filev2size = id3_tag_query ((id3_byte_t const *)tmp,
                                    ID3_TAG_QUERYSIZE);

        if (filev2size > 10)
        {
            if (!g_seekable_seek (seekable, -filev2size, G_SEEK_CUR, NULL,
                                  error))
            {
                goto err;
            }

            if (!g_input_stream_read_all (istream, tmp, ID3_TAG_QUERYSIZE,
                                          &bytes_read, NULL, error))
            {
                goto err;
            }

            if (id3_tag_query ((id3_byte_t const *)tmp, ID3_TAG_QUERYSIZE)
                != filev2size)
            {
                if (!g_seekable_seek (seekable,
                                      -ID3_TAG_QUERYSIZE - filev2size,
                                      G_SEEK_CUR, NULL, error))
                {
                    goto err;
                }
            }
            else
            {
                if (!g_seekable_seek (seekable, -ID3_TAG_QUERYSIZE,
                                      G_SEEK_CUR, NULL, error))
                {
                    goto err;
                }
            }
        }
    }

    ostream = g_io_stream_get_output_stream (G_IO_STREAM (iostream));

    /* Write id3v1 tag */
    if (v1buf)
    {
        if (!g_output_stream_write_all (ostream, v1buf, v1size, &bytes_written,
                                        NULL, error))
        {
            goto err;
        }
    }

    /* Truncate file (strip tags at the end of file) */
    if (!g_seekable_can_truncate (seekable))
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_BADF, "%s",
                     g_strerror (EBADF));
        goto err;
    }

    if (!g_seekable_truncate (seekable, g_seekable_tell (seekable), NULL,
                              error))
    {
        goto err;
    }

    /* Handle Id3v2 tag */
    if (!g_seekable_seek (seekable, 0, G_SEEK_SET, NULL, error))
    {
        goto err;
    }

    if (!g_input_stream_read_all (istream, tmp, ID3_TAG_QUERYSIZE, &bytes_read,
                                  NULL, error))
    {
        goto err;
    }

    filev2size = id3_tag_query ((id3_byte_t const *)tmp, ID3_TAG_QUERYSIZE);

    /* No ID3v2 tag in the file, and no new tag. */
    if ((filev2size == 0) && (v2size == 0))
    {
        /* TODO: Improve error description. */
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_BADF, "%s",
                     g_strerror (EBADF));
        goto err;
    }

    if (filev2size == (long)v2size)
    {
        /* New and old tag are the same length, so no need to handle audio. */
        if (!g_seekable_seek (seekable, 0, G_SEEK_SET, NULL, error))
        {
            goto err;
        }

        if (!g_output_stream_write_all (ostream, v2buf, v2size, &bytes_written,
                                        NULL, error))
        {
            goto err;
        }
    }
    else
    {
        gsize audio_length;

        /* New and old tag differ in length, so copy the audio data to after
         * the new tag. */
        if (!g_seekable_seek (seekable, 0, G_SEEK_END, NULL, error))
        {
            goto err;
        }

        audio_length = g_seekable_tell (seekable) - filev2size;
        audio_buffer = g_malloc (audio_length);

        if (!g_seekable_seek (seekable, filev2size, G_SEEK_SET, NULL, error))
        {
            goto err;
        }

        if (audio_length != 0)
        {
            if (!g_input_stream_read_all (istream, audio_buffer, audio_length,
                                          &bytes_read, NULL, error))
            {
                goto err;
            }
        }
        
        /* Return to the beginning of the file. */
        if (!g_seekable_seek (seekable, 0, G_SEEK_SET, NULL, error))
        {
            goto err;
        }

        /* Write the ID3v2 tag. */
        if (v2buf)
        {
            if (!g_output_stream_write_all (ostream, v2buf, v2size,
                                            &bytes_written, NULL, error))
            {
                goto err;
            }
        }

        /* Write audio data. */
        if (audio_length != 0)
        {
            if (!g_output_stream_write_all (ostream, audio_buffer,
                                            audio_length, &bytes_written, NULL,
                                            error))
            {
                goto err;
            }
        }

        if (!g_seekable_truncate (seekable, g_seekable_tell (seekable), NULL,
                                  error))
        {
            goto err;
        }
    }

    success = TRUE;

err:
    g_free (audio_buffer);
    g_object_unref (file);
    g_clear_object (&iostream);
    g_free (v1buf);
    g_free (v2buf);
    return success;
}

#endif /* ENABLE_MP3 */
