/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014-2015  David King <amigadave@amigadave.com>
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

#include <glib/gi18n.h>
#include <stdlib.h>
#include <string.h>
#include <vorbis/codec.h>

#include "ogg_tag.h"
#include "vcedit.h"
#include "et_core.h"
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
add_to_guchar_str (guchar *ustr,
                   gsize *ustr_len,
                   const guchar *str,
                   gsize str_len)
{
    gsize i;

    for (i = *ustr_len; i < *ustr_len + str_len; i++)
    {
        ustr[i] = str[i - *ustr_len];
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
 * set_or_append_field:
 * @field: (inout): pointer to a location in which to store the field value
 * @field_value: (transfer full): the string to store in @field
 *
 * Set @field to @field_value if @field is empty, otherwise append to it.
 * Ownership of @field_value is transferred to this function.
 */
static void
set_or_append_field (gchar **field,
                     gchar *field_value)
{
    if (*field == NULL)
    {
        *field = field_value;
    }
    else
    {
        gchar *field_tmp = g_strconcat (*field, MULTIFIELD_SEPARATOR,
                                        field_value, NULL);
        g_free (*field);
        *field = field_tmp;
        g_free (field_value);
    }
}

/*
 * @field: a tag field to search
 * @str: a search term to compare against @field
 *
 * Call strncasecmp() on @str and @field, comparing only up to the length of
 * the field.
 */
static gint
field_strncasecmp (const gchar *field,
                   const gchar *str)
{
    return strncasecmp (field, str, strlen (str));
}

/*
 * et_add_file_tags_from_vorbis_comments:
 * @vc: Vorbis comment from which to fill @FileTag
 * @FileTag: tag to populate from @vc
 *
 * Reads Vorbis comments and copies them to file tag.
 */
void
et_add_file_tags_from_vorbis_comments (vorbis_comment *vc,
                                       File_Tag *FileTag)
{
    gchar *string = NULL;
    gchar *string1 = NULL;
    gchar *string2 = NULL;
    guint field_num, i;
    EtPicture *prev_pic = NULL;

    /*********
     * Title *
     *********/
    /* Note : don't forget to add any new field to 'Save unsupported fields' */
    field_num = 0;
    while ((string = vorbis_comment_query (vc, ET_VORBIS_COMMENT_FIELD_TITLE,
                                           field_num++)) != NULL)
    {
        if (!et_str_empty (string))
        {
            string = Try_To_Validate_Utf8_String (string);
            set_or_append_field (&FileTag->title, string);
        }
    }

    /**********
     * Artist *
     **********/
    field_num = 0;
    while ((string = vorbis_comment_query (vc, ET_VORBIS_COMMENT_FIELD_ARTIST,
                                           field_num++)) != NULL)
    {
        if (!et_str_empty (string))
        {
            string = Try_To_Validate_Utf8_String (string);
            set_or_append_field (&FileTag->artist, string);
        }
    }

    /****************
     * Album Artist *
     ****************/
    field_num = 0;
    while ((string = vorbis_comment_query (vc,
                                           ET_VORBIS_COMMENT_FIELD_ALBUM_ARTIST,
                                           field_num++)) != NULL)
    {
        if (!et_str_empty (string))
        {
            string = Try_To_Validate_Utf8_String (string);
            set_or_append_field (&FileTag->album_artist, string);
        }
    }

    /*********
     * Album *
     *********/
    field_num = 0;
    while ((string = vorbis_comment_query (vc, ET_VORBIS_COMMENT_FIELD_ALBUM,
                                           field_num++)) != NULL)
    {
        if (!et_str_empty (string))
        {
            string = Try_To_Validate_Utf8_String (string);
            set_or_append_field (&FileTag->album, string);
        }
    }

    /**********************************************
     * Disc Number (Part of a Set) and Disc Total *
     **********************************************/
    if ((string = vorbis_comment_query (vc, ET_VORBIS_COMMENT_FIELD_DISC_NUMBER, 0)) != NULL
        && !et_str_empty (string))
    {
        /* Check if DISCTOTAL used, else takes it in DISCNUMBER. */
        if ((string1 = vorbis_comment_query (vc,
                                             ET_VORBIS_COMMENT_FIELD_DISC_TOTAL,
                                             0)) != NULL
            && !et_str_empty (string1))
        {
            FileTag->disc_total = et_disc_number_to_string (atoi (string1));
        }
        else if ((string1 = strchr (string, '/')))
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
    string = vorbis_comment_query (vc, ET_VORBIS_COMMENT_FIELD_DATE, 0);

    if (!et_str_empty (string))
    {
        FileTag->year = g_strdup(string);
    }

    /*************************
     * Track and Total Track *
     *************************/
    string = vorbis_comment_query (vc, ET_VORBIS_COMMENT_FIELD_TRACK_NUMBER,
                                   0);

    if (!et_str_empty (string))
    {
        /* Check if TRACKTOTAL used, else takes it in TRACKNUMBER. */
        if ((string1 = vorbis_comment_query (vc,
                                             ET_VORBIS_COMMENT_FIELD_TRACK_TOTAL,
                                             0)) != NULL
            && !et_str_empty (string1))
        {
            FileTag->track_total = et_track_number_to_string (atoi (string1));
        }
        else if ((string1 = strchr (string, '/')))
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
    while ((string = vorbis_comment_query (vc, ET_VORBIS_COMMENT_FIELD_GENRE,
                                           field_num++)) != NULL)
    {
        if (!et_str_empty (string))
        {
            string = Try_To_Validate_Utf8_String (string);
            set_or_append_field (&FileTag->genre, string);
        }
    }

    /***********
     * Comment *
     ***********/
    field_num = 0;
    string1 = NULL; // Cause it may be not updated into the 'while' condition
    while (((string2 = vorbis_comment_query (vc,
                                             ET_VORBIS_COMMENT_FIELD_DESCRIPTION,
                                             field_num)) != NULL) /* New specification */
           || ((string  = vorbis_comment_query (vc,
                                                ET_VORBIS_COMMENT_FIELD_COMMENT,
                                                field_num)) != NULL) /* Old : Winamp format (for EasyTAG 1.99.11 and older) */
           || ((string1 = vorbis_comment_query (vc, "", field_num)) != NULL)) /* Old : Xmms format (for EasyTAG 1.99.11 and older). */
    {
        /* Contains comment in new specifications format and we prefer this
         * format (field name defined). */
        if (!et_str_empty (string2))
        {
            string2 = Try_To_Validate_Utf8_String (string2);
            set_or_append_field (&FileTag->comment, string2);
        }
        /* Contains comment to Winamp format and we prefer this format (field
         * name defined). */
        else if (!et_str_empty (string))
        {
            string = Try_To_Validate_Utf8_String (string);
            set_or_append_field (&FileTag->comment, string);
        }
        /* Contains comment in XMMS format only. */
        else if (!et_str_empty (string1))
        {
            string1 = Try_To_Validate_Utf8_String (string1);
            set_or_append_field (&FileTag->comment, string1);
        }

        field_num++;
    }

    /************
     * Composer *
     ************/
    field_num = 0;
    while ((string = vorbis_comment_query (vc,
                                           ET_VORBIS_COMMENT_FIELD_COMPOSER,
                                           field_num++)) != NULL)
    {
        if (!et_str_empty (string))
        {
            string = Try_To_Validate_Utf8_String (string);
            set_or_append_field (&FileTag->composer, string);
        }
    }

    /*******************
     * Original artist *
     *******************/
    field_num = 0;
    while ((string = vorbis_comment_query (vc,
                                           ET_VORBIS_COMMENT_FIELD_PERFORMER,
                                           field_num++)) != NULL)
    {
        if (!et_str_empty (string))
        {
            string = Try_To_Validate_Utf8_String (string);
            set_or_append_field (&FileTag->orig_artist, string);
        }
    }

    /*************
     * Copyright *
     *************/
    field_num = 0;
    while ((string = vorbis_comment_query (vc,
                                           ET_VORBIS_COMMENT_FIELD_COPYRIGHT,
                                           field_num++)) != NULL)
    {
        if (!et_str_empty (string))
        {
            string = Try_To_Validate_Utf8_String (string);
            set_or_append_field (&FileTag->copyright, string);
        }
    }

    /*******
     * URL *
     *******/
    field_num = 0;
    while ((string = vorbis_comment_query (vc,
                                           ET_VORBIS_COMMENT_FIELD_CONTACT,
                                           field_num++)) != NULL)
    {
        if (!et_str_empty (string))
        {
            string = Try_To_Validate_Utf8_String (string);
            set_or_append_field (&FileTag->url, string);
        }
    }

    /**************
     * Encoded by *
     **************/
    field_num = 0;
    while ((string = vorbis_comment_query (vc,
                                           ET_VORBIS_COMMENT_FIELD_ENCODED_BY,
                                           field_num++)) != NULL)
    {
        if (!et_str_empty (string))
        {
            string = Try_To_Validate_Utf8_String (string);
            set_or_append_field (&FileTag->encoded_by, string);
        }
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
    while ((string = vorbis_comment_query (vc, ET_VORBIS_COMMENT_FIELD_COVER_ART,
                                           field_num++)) != NULL)
    {
        EtPicture *pic;
        guchar *data;
        gsize data_size;
        GBytes *bytes;
        EtPictureType type;
        const gchar *description;
            
        /* Force marking the file as modified, so that the deprecated cover art
         * field in converted in a METADATA_PICTURE_BLOCK field. */
        FileTag->saved = FALSE;

        /* Decode picture data. */
        data = g_base64_decode (string, &data_size);
        bytes = g_bytes_new_take (data, data_size);

        if ((string = vorbis_comment_query (vc,
                                            ET_VORBIS_COMMENT_FIELD_COVER_ART_TYPE,
                                            field_num - 1))
            != NULL)
        {
            type = atoi (string);
        }
        else
        {
            type = ET_PICTURE_TYPE_FRONT_COVER;
        }

        if ((string = vorbis_comment_query (vc,
                                            ET_VORBIS_COMMENT_FIELD_COVER_ART_DESCRIPTION,
                                            field_num - 1)) != NULL)
        {
            description = string;
        }
        else
        {
            description = "";
        }

        pic = et_picture_new (type, description, 0, 0, bytes);
        g_bytes_unref (bytes);

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

    /* METADATA_BLOCK_PICTURE tag used for picture information */
    field_num = 0;
    while ((string = vorbis_comment_query (vc,
                                           ET_VORBIS_COMMENT_FIELD_METADATA_BLOCK_PICTURE,
                                           field_num++)) != NULL)
    {
        EtPicture *pic;
        gsize bytes_pos, mimelen, desclen;
        guchar *decoded_ustr;
        GBytes *bytes = NULL;
        EtPictureType type;
        gchar *description;
        GBytes *pic_bytes;
        gsize decoded_size;
        gsize data_size;

        /* Decode picture data. */
        decoded_ustr = g_base64_decode (string, &decoded_size);

        /* Check that the comment decoded to a long enough string to hold the
         * whole structure (8 fields of 4 bytes each). */
        if (decoded_size < 8 * 4)
        {
            g_free (decoded_ustr);
            goto invalid_picture;
        }

        bytes = g_bytes_new_take (decoded_ustr, decoded_size);

        /* Reading picture type. */
        type = read_guint32_from_byte (decoded_ustr, 0);
        bytes_pos = 4;

        /* TODO: Check that there is a maximum of 1 of each of
         * ET_PICTURE_TYPE_FILE_ICON and ET_PICTURE_TYPE_OTHER_FILE_ICON types
         * in the file. */
        if (type >= ET_PICTURE_TYPE_UNDEFINED)
        {
            goto invalid_picture;
        }

        /* Reading MIME data. */
        mimelen = read_guint32_from_byte (decoded_ustr, bytes_pos);
        bytes_pos += 4;

        if (mimelen > decoded_size - bytes_pos - (6 * 4))
        {
            goto invalid_picture;
        }

        /* Check for a valid MIME type. */
        if (mimelen > 0)
        {
            const gchar *mime;

            mime = (const gchar *)&decoded_ustr[bytes_pos];

            /* TODO: Check for "-->" when adding linked image support. */
            if (strncmp (mime, "image/", mimelen) != 0
                && strncmp (mime, "image/png", mimelen) != 0
                && strncmp (mime, "image/jpeg", mimelen) != 0)
            {
                gchar *mime_str;

                mime_str = g_strndup (mime, mimelen);
                g_debug ("Invalid Vorbis comment image MIME type: %s",
                         mime_str);

                g_free (mime_str);
                goto invalid_picture;
            }
        }

        /* Skip over the MIME type, as gdk-pixbuf does not use it. */
        bytes_pos += mimelen;

        /* Reading description */
        desclen = read_guint32_from_byte (decoded_ustr, bytes_pos);
        bytes_pos += 4;

        if (desclen > decoded_size - bytes_pos - (5 * 4))
        {
            goto invalid_picture;
        }

        description = g_strndup ((const gchar *)&decoded_ustr[bytes_pos],
                                 desclen);

        /* Skip the width, height, color depth and number-of-colors fields. */
        bytes_pos += desclen + 16;

        /* Reading picture size */
        data_size = read_guint32_from_byte (decoded_ustr, bytes_pos);
        bytes_pos += 4;

        if (data_size > decoded_size - bytes_pos)
        {
            g_free (description);
            goto invalid_picture;
        }

        /* Read only the image data into a new GBytes. */
        pic_bytes = g_bytes_new_from_bytes (bytes, bytes_pos, data_size);

        pic = et_picture_new (type, description, 0, 0, pic_bytes);

        g_free (description);
        g_bytes_unref (pic_bytes);

        if (!prev_pic)
        {
            FileTag->picture = pic;
        }
        else
        {
            prev_pic->next = pic;
        }

        prev_pic = pic;

        /* pic->bytes still holds a ref on the decoded data. */
        g_bytes_unref (bytes);
        continue;

invalid_picture:
        /* Mark the file as modified, so that the invalid field is removed upon
         * saving. */
        FileTag->saved = FALSE;

        g_bytes_unref (bytes);
    }

    /***************************
     * Save unsupported fields *
     ***************************/
    for (i=0;i<(guint)vc->comments;i++)
    {
        if (field_strncasecmp (vc->user_comments[i],
                               ET_VORBIS_COMMENT_FIELD_TITLE "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_ARTIST "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_ALBUM_ARTIST "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_ALBUM "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_DISC_NUMBER "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_DISC_TOTAL "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_DATE "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_TRACK_NUMBER) != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_TRACK_TOTAL) != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_GENRE "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_DESCRIPTION "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_COMMENT "=") != 0
            && field_strncasecmp (vc->user_comments[i], "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_COMPOSER "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_PERFORMER "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_COPYRIGHT "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_CONTACT "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_ENCODED_BY "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_COVER_ART "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_COVER_ART_TYPE "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_COVER_ART_MIME "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_COVER_ART_DESCRIPTION "=") != 0
            && field_strncasecmp (vc->user_comments[i],
                                  ET_VORBIS_COMMENT_FIELD_METADATA_BLOCK_PICTURE "=") != 0
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
ogg_tag_read_file_tag (GFile *file,
                       File_Tag *FileTag,
                       GError **error)
{
    GFileInputStream *istream;
    EtOggState *state;

    g_return_val_if_fail (file != NULL && FileTag != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    istream = g_file_read (file, NULL, error);

    if (!istream)
    {
        g_assert (error == NULL || *error != NULL);
        return FALSE;
    }

    {
    /* Check for an unsupported ID3v2 tag. */
    guchar tmp_id3[4];

    if (g_input_stream_read (G_INPUT_STREAM (istream), tmp_id3, 4, NULL,
                             error) == 4)
    {
        /* Calculate ID3v2 length. */
        if (tmp_id3[0] == 'I' && tmp_id3[1] == 'D' && tmp_id3[2] == '3'
            && tmp_id3[3] < 0xFF)
        {
            /* ID3v2 tag skipper $49 44 33 yy yy xx zz zz zz zz [zz size]. */
            /* Size is 6-9 position */
            if (!g_seekable_seek (G_SEEKABLE (istream), 2, G_SEEK_CUR,
                                  NULL, error))
            {
                goto err;
            }

            if (g_input_stream_read (G_INPUT_STREAM (istream), tmp_id3, 4,
                                     NULL, error) == 4)
            {
                gchar *path;

                path = g_file_get_path (file);
                g_debug ("Ogg file '%s' contains an ID3v2 tag", path);
                g_free (path);

                /* Mark the file as modified, so that the ID3 tag is removed
                 * upon saving. */
                FileTag->saved = FALSE;
            }
        }
    }

    if (error && *error != NULL)
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
        vcedit_clear(state);
        return FALSE;
    }

    g_assert (error == NULL || *error == NULL);

    /* Get data from tag */
    /*{
        gint i; 
        for (i=0;i<vc->comments;i++) 
            g_print("%s -> Ogg vc:'%s'\n",g_path_get_basename(filename),vc->user_comments[i]);
    }*/

    et_add_file_tags_from_vorbis_comments (vcedit_comments(state), FileTag);
    vcedit_clear(state);

    return TRUE;

err:
    g_assert (error == NULL || *error != NULL);
    g_object_unref (istream);
    return FALSE;
}

/*
 * Save field value in separated tags if it contains multifields
 */
static void
et_ogg_write_delimited_tag (vorbis_comment *vc,
                            const gchar *tag_name,
                            const gchar *values)
{
    gchar **strings;
    gsize i;

    strings = g_strsplit (values, MULTIFIELD_SEPARATOR, 255);
    
    for (i = 0; strings[i] != NULL; i++)
    {
        if (*strings[i])
        {
            vorbis_comment_add_tag (vc, tag_name, strings[i]);
        }
    }

    g_strfreev (strings);
}

static void
et_ogg_set_tag (vorbis_comment *vc,
                const gchar *tag_name,
                const gchar *value,
                gboolean split)
{
    if (value)
    {
        if (split)
        {
            et_ogg_write_delimited_tag (vc, tag_name, value);
        }
        else
        {
            vorbis_comment_add_tag (vc, tag_name, value);
        }
    }
}

gboolean
ogg_tag_write_file_tag (const ET_File *ETFile,
                        GError **error)
{
    const File_Tag *FileTag;
    const gchar *filename;
    GFile           *file;
    EtOggState *state;
    vorbis_comment *vc;
    GList *l;
    EtPicture *pic;

    g_return_val_if_fail (ETFile != NULL && ETFile->FileTag != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    FileTag       = (File_Tag *)ETFile->FileTag->data;
    filename      = ((File_Name *)ETFile->FileNameCur->data)->value;

    file = g_file_new_for_path (filename);

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
    et_ogg_set_tag (vc, ET_VORBIS_COMMENT_FIELD_TITLE, FileTag->title,
                    g_settings_get_boolean (MainSettings, "ogg-split-title"));

    /**********
     * Artist *
     **********/
    et_ogg_set_tag (vc, ET_VORBIS_COMMENT_FIELD_ARTIST, FileTag->artist,
                    g_settings_get_boolean (MainSettings, "ogg-split-artist"));

    /****************
     * Album Artist *
     ****************/
    et_ogg_set_tag (vc, ET_VORBIS_COMMENT_FIELD_ALBUM_ARTIST,
                    FileTag->album_artist,
                    g_settings_get_boolean (MainSettings, "ogg-split-artist"));

    /*********
     * Album *
     *********/
    et_ogg_set_tag (vc, ET_VORBIS_COMMENT_FIELD_ALBUM, FileTag->album,
                    g_settings_get_boolean (MainSettings, "ogg-split-album"));

    /***************
     * Disc Number *
     ***************/
    et_ogg_set_tag (vc, ET_VORBIS_COMMENT_FIELD_DISC_NUMBER,
                    FileTag->disc_number, FALSE);
    et_ogg_set_tag (vc, ET_VORBIS_COMMENT_FIELD_DISC_TOTAL,
                    FileTag->disc_total, FALSE);

    /********
     * Year *
     ********/
    et_ogg_set_tag (vc, ET_VORBIS_COMMENT_FIELD_DATE, FileTag->year, FALSE);

    /*************************
     * Track and Total Track *
     *************************/
    et_ogg_set_tag (vc, ET_VORBIS_COMMENT_FIELD_TRACK_NUMBER, FileTag->track,
                    FALSE);
    et_ogg_set_tag (vc, ET_VORBIS_COMMENT_FIELD_TRACK_TOTAL,
                    FileTag->track_total, FALSE);

    /*********
     * Genre *
     *********/
    et_ogg_set_tag (vc, ET_VORBIS_COMMENT_FIELD_GENRE, FileTag->genre,
                    g_settings_get_boolean (MainSettings, "ogg-split-genre"));

    /***********
     * Comment *
     ***********/
    /* Format of new specification. */
    et_ogg_set_tag (vc, ET_VORBIS_COMMENT_FIELD_DESCRIPTION, FileTag->comment,
                    g_settings_get_boolean (MainSettings,
                                            "ogg-split-comment"));

    /************
     * Composer *
     ************/
    et_ogg_set_tag (vc, ET_VORBIS_COMMENT_FIELD_COMPOSER, FileTag->composer,
                    g_settings_get_boolean (MainSettings,
                                            "ogg-split-composer"));

    /*******************
     * Original artist *
     *******************/
    et_ogg_set_tag (vc, ET_VORBIS_COMMENT_FIELD_PERFORMER,
                    FileTag->orig_artist,
                    g_settings_get_boolean (MainSettings,
                                            "ogg-split-original-artist"));

    /*************
     * Copyright *
     *************/
    et_ogg_set_tag (vc, ET_VORBIS_COMMENT_FIELD_COPYRIGHT, FileTag->copyright,
                    FALSE);

    /*******
     * URL *
     *******/
    et_ogg_set_tag (vc, ET_VORBIS_COMMENT_FIELD_CONTACT, FileTag->url, FALSE);

    /**************
     * Encoded by *
     **************/
    et_ogg_set_tag (vc, ET_VORBIS_COMMENT_FIELD_ENCODED_BY,
                    FileTag->encoded_by, FALSE);
    
    /***********
     * Picture *
     ***********/
    for (pic = FileTag->picture; pic != NULL; pic = pic->next)
    {
        const gchar *mime;
        guchar array[4];
        guchar *ustring = NULL;
        gsize ustring_len = 0;
        gchar *base64_string;
        gsize desclen;
        gconstpointer data;
        gsize data_size;
        Picture_Format format = Picture_Format_From_Data (pic);

        /* According to the specification, only PNG and JPEG images should
         * be added to Vorbis comments. */
        if (format != PICTURE_FORMAT_PNG && format != PICTURE_FORMAT_JPEG)
        {
            GdkPixbufLoader *loader;
            GError *loader_error = NULL;

            loader = gdk_pixbuf_loader_new ();

            if (!gdk_pixbuf_loader_write_bytes (loader, pic->bytes,
                                                &loader_error))
            {
                g_debug ("Error parsing image data: %s",
                         loader_error->message);
                g_error_free (loader_error);
                g_object_unref (loader);
                continue;
            }
            else
            {
                GdkPixbuf *pixbuf;
                gchar *buffer;
                gsize buffer_size;

                if (!gdk_pixbuf_loader_close (loader, &loader_error))
                {
                    g_debug ("Error parsing image data: %s",
                             loader_error->message);
                    g_error_free (loader_error);
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
                                                &loader_error, NULL))
                {
                    g_debug ("Error while converting image to PNG: %s",
                             loader_error->message);
                    g_error_free (loader_error);
                    g_object_unref (pixbuf);
                    continue;
                }

                g_object_unref (pixbuf);

                g_bytes_unref (pic->bytes);
                pic->bytes = g_bytes_new_take (buffer, buffer_size);

                /* Set the picture format to reflect the new data. */
                format = Picture_Format_From_Data (pic);
            }
        }

        mime = Picture_Mime_Type_String (format);

        data = g_bytes_get_data (pic->bytes, &data_size);

        /* Calculating full length of byte string and allocating. */
        desclen = pic->description ? strlen (pic->description) : 0;
        ustring = g_malloc (4 * 8 + strlen (mime) + desclen + data_size);

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
        convert_to_byte_array (data_size, array);
        add_to_guchar_str (ustring, &ustring_len, array, 4);

        add_to_guchar_str (ustring, &ustring_len, data, data_size);

        base64_string = g_base64_encode (ustring, ustring_len);
        vorbis_comment_add_tag (vc,
                                ET_VORBIS_COMMENT_FIELD_METADATA_BLOCK_PICTURE,
                                base64_string);

        g_free (base64_string);
        g_free (ustring);
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

    g_object_unref (file);

    g_assert (error == NULL || *error == NULL);
    return TRUE;
}
#endif /* ENABLE_OGG */
