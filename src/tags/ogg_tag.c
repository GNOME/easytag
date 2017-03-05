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
 * validate_field_utf8:
 * @field_value: the string to validate
 * @field_len: the length of the string
 *
 * Validate a Vorbis comment field to ensure that it is UTF-8. Either return a
 * duplicate of the original (valid) string, or a converted equivalent (of an
 * invalid UTF-8 string).
 *
 * Returns: a valid UTF-8 represenation of @field_value
 */
static gchar *
validate_field_utf8 (const gchar *field_value,
                     gint field_len)
{
    gchar *result;

    if (g_utf8_validate (field_value, field_len, NULL))
    {
        result = g_strndup (field_value, field_len);
    }
    else
    {
        gchar *field_value_tmp = g_strndup (field_value,
                                            field_len);
        /* Unnecessarily validates the field again, but this should not be the
         * common case. */
        result = Try_To_Validate_Utf8_String (field_value_tmp);
        g_free (field_value_tmp);
    }

    return result;
}

/*
 * populate_tag_hash_table:
 * @vc: a Vorbis comment block from which to read fields
 *
 * Add comments from the supplied @vc block to a newly-allocated hash table.
 * Normalise the field names to upper-case ASCII, taking care to ignore the
 * current locale. Validate the field values are UTF-8 before inserting them in
 * the hash table. Add the values as strings in a GSList.
 *
 * Returns: (transfer full): a newly-allocated hash table of tags
 */
static GHashTable *
populate_tag_hash_table (const vorbis_comment *vc)
{
    GHashTable *ret;
    gint i;

    /* Free the string lists manually, to avoid having to duplicate them each
     * time that an existing key is inserted. */
    ret = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

    for (i = 0; i < vc->comments; i++)
    {
        const gchar *comment = vc->user_comments[i];
        const gint len = vc->comment_lengths[i];
        const gchar *separator;
        gchar *field;
        gchar *field_up;
        gchar *value;
        /* TODO: Use a GPtrArray instead? */
        GSList *l;

        separator = memchr (comment, '=', len);

        if (!separator)
        {
            g_warning ("Field separator not found when reading Vorbis tag: %s",
                       comment);
            continue;
        }

        field = g_strndup (comment, separator - comment);
        field_up = g_ascii_strup (field, -1);
        g_free (field);

        l = g_hash_table_lookup (ret, field_up);

        /* If the lookup failed, a new list is created. The list takes
         * ownership of the field value. */
        value = validate_field_utf8 (separator + 1, len - ((separator + 1) -
                                                           comment));

        /* Appending is slower, but much easier here (and the lists should be
         * short). */
        l = g_slist_append (l, value);

        g_hash_table_insert (ret, field_up, l);
    }

    return ret;
}

/*
 * values_list_foreach
 * @data: (transfer full): the tag value
 * @user_data: (inout); a tag location to fill
 *
 * Called on each element in a GSList of tag values, to set or append the
 * string to the tag.
 */
static void
values_list_foreach (gpointer data,
                     gpointer user_data)
{
    gchar *value = (gchar *)data;
    gchar **tag = (gchar **)user_data;

    if (!et_str_empty (value))
    {
        set_or_append_field (tag, value);
    }
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
    GHashTable *tags;
    GSList *strings;
    GHashTableIter tags_iter;
    gchar *key;
    EtPicture *prev_pic = NULL;

    tags = populate_tag_hash_table (vc);

    /* Note : don't forget to add any new field to 'Save unsupported fields' */

    /* Title. */
    if ((strings = g_hash_table_lookup (tags, ET_VORBIS_COMMENT_FIELD_TITLE)))
    {
        g_slist_foreach (strings, values_list_foreach, &FileTag->title);
        g_slist_free (strings);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_TITLE);
    }

    /* Artist. */
    if ((strings = g_hash_table_lookup (tags, ET_VORBIS_COMMENT_FIELD_ARTIST)))
    {
        g_slist_foreach (strings, values_list_foreach, &FileTag->artist);
        g_slist_free (strings);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_ARTIST);
    }

    /* Album artist. */
    if ((strings = g_hash_table_lookup (tags,
                                        ET_VORBIS_COMMENT_FIELD_ALBUM_ARTIST)))
    {
        g_slist_foreach (strings, values_list_foreach, &FileTag->album_artist);
        g_slist_free (strings);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_ALBUM_ARTIST);
    }

    /* Album */
    if ((strings = g_hash_table_lookup (tags, ET_VORBIS_COMMENT_FIELD_ALBUM)))
    {
        g_slist_foreach (strings, values_list_foreach, &FileTag->album);
        g_slist_free (strings);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_ALBUM);
    }

    /* Disc number and total discs. */
    if ((strings = g_hash_table_lookup (tags,
                                        ET_VORBIS_COMMENT_FIELD_DISC_TOTAL)))
    {
        /* Only take values from the first total discs field. */
        if (!et_str_empty (strings->data))
        {
            FileTag->disc_total = et_disc_number_to_string (atoi (strings->data));
        }

        g_slist_free_full (strings, g_free);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_DISC_TOTAL);
    }

    if ((strings = g_hash_table_lookup (tags,
                                        ET_VORBIS_COMMENT_FIELD_DISC_NUMBER)))
    {
        /* Only take values from the first disc number field. */
        if (!et_str_empty (strings->data))
        {
            gchar *separator;

            separator = strchr (strings->data, '/');

            if (separator && !FileTag->disc_total)
            {
                FileTag->disc_total = et_disc_number_to_string (atoi (separator + 1));
                *separator = '\0';
            }

            FileTag->disc_number = et_disc_number_to_string (atoi (strings->data));
        }

        g_slist_free_full (strings, g_free);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_DISC_NUMBER);
    }

    /* Year. */
    if ((strings = g_hash_table_lookup (tags, ET_VORBIS_COMMENT_FIELD_DATE)))
    {
        g_slist_foreach (strings, values_list_foreach, &FileTag->year);
        g_slist_free (strings);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_DATE);
    }

    /* Track number and total tracks. */
    if ((strings = g_hash_table_lookup (tags,
                                        ET_VORBIS_COMMENT_FIELD_TRACK_TOTAL)))
    {
        /* Only take values from the first total tracks field. */
        if (!et_str_empty (strings->data))
        {
            FileTag->track_total = et_track_number_to_string (atoi (strings->data));
        }

        g_slist_free_full (strings, g_free);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_TRACK_TOTAL);
    }

    if ((strings = g_hash_table_lookup (tags,
                                        ET_VORBIS_COMMENT_FIELD_TRACK_NUMBER)))
    {
        /* Only take values from the first track number field. */
        if (!et_str_empty (strings->data))
        {
            gchar *separator;

            separator = strchr (strings->data, '/');

            if (separator && !FileTag->track_total)
            {
                FileTag->track_total = et_track_number_to_string (atoi (separator + 1));
                *separator = '\0';
            }

            FileTag->track = et_track_number_to_string (atoi (strings->data));
        }

        g_slist_free_full (strings, g_free);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_TRACK_NUMBER);
    }

    /* Genre. */
    if ((strings = g_hash_table_lookup (tags, ET_VORBIS_COMMENT_FIELD_GENRE)))
    {
        g_slist_foreach (strings, values_list_foreach, &FileTag->genre);
        g_slist_free (strings);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_GENRE);
    }

    /* Comment */
    {
        GSList *descs;
        GSList *comments;

        descs = g_hash_table_lookup (tags,
                                     ET_VORBIS_COMMENT_FIELD_DESCRIPTION);
        comments = g_hash_table_lookup (tags,
                                        ET_VORBIS_COMMENT_FIELD_COMMENT);

        if (descs && !comments)
        {
            g_slist_foreach (descs, values_list_foreach, &FileTag->comment);
        }
        else if (descs && comments)
        {
            /* Mark the file as modified, so that comments are written to the
             * DESCRIPTION field on saving. */
            FileTag->saved = FALSE;

            g_slist_foreach (descs, values_list_foreach, &FileTag->comment);
            g_slist_foreach (comments, values_list_foreach, &FileTag->comment);
        }
        else if (comments)
        {
            FileTag->saved = FALSE;

            g_slist_foreach (comments, values_list_foreach, &FileTag->comment);
        }

        g_slist_free (descs);
        g_slist_free (comments);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_DESCRIPTION);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_COMMENT);
    }

    /* Composer. */
    if ((strings = g_hash_table_lookup (tags,
                                        ET_VORBIS_COMMENT_FIELD_COMPOSER)))
    {
        g_slist_foreach (strings, values_list_foreach, &FileTag->composer);
        g_slist_free (strings);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_COMPOSER);
    }

    /* Original artist. */
    if ((strings = g_hash_table_lookup (tags,
                                        ET_VORBIS_COMMENT_FIELD_PERFORMER)))
    {
        g_slist_foreach (strings, values_list_foreach, &FileTag->orig_artist);
        g_slist_free (strings);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_PERFORMER);
    }

    /* Copyright. */
    if ((strings = g_hash_table_lookup (tags,
                                        ET_VORBIS_COMMENT_FIELD_COPYRIGHT)))
    {
        g_slist_foreach (strings, values_list_foreach, &FileTag->copyright);
        g_slist_free (strings);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_COPYRIGHT);
    }

    /* URL. */
    if ((strings = g_hash_table_lookup (tags,
                                        ET_VORBIS_COMMENT_FIELD_CONTACT)))
    {
        g_slist_foreach (strings, values_list_foreach, &FileTag->url);
        g_slist_free (strings);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_CONTACT);
    }

    /* Encoded by. */
    if ((strings = g_hash_table_lookup (tags,
                                        ET_VORBIS_COMMENT_FIELD_ENCODED_BY)))
    {
        g_slist_foreach (strings, values_list_foreach, &FileTag->encoded_by);
        g_slist_free (strings);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_ENCODED_BY);
    }

    /* Cover art. */
    if ((strings = g_hash_table_lookup (tags,
                                        ET_VORBIS_COMMENT_FIELD_COVER_ART)))
    {
        GSList *l;
        GSList *m;
        GSList *n;
        GSList *types;
        GSList *descs;

        /* Force marking the file as modified, so that the deprecated cover art
         * field is converted to a METADATA_PICTURE_BLOCK field. */
        FileTag->saved = FALSE;

        types = g_hash_table_lookup (tags,
                                     ET_VORBIS_COMMENT_FIELD_COVER_ART_TYPE);
        descs = g_hash_table_lookup (tags,
                                     ET_VORBIS_COMMENT_FIELD_COVER_ART_DESCRIPTION);

        l = strings;
        m = types;
        n = descs;

        while (l && !et_str_empty (l->data))
        {
            EtPicture *pic;
            guchar *data;
            gsize data_size;
            GBytes *bytes;
            EtPictureType type;
            const gchar *description;

            /* Decode picture data. */
            data = g_base64_decode (l->data, &data_size);
            bytes = g_bytes_new_take (data, data_size);

            /* It is only necessary for there to be image data, but the type
             * and description are optional. */
            if (m)
            {
                type = !et_str_empty (m->data) ? atoi (m->data)
                                               : ET_PICTURE_TYPE_FRONT_COVER;

                m = g_slist_next (m);
            }
            else
            {
                type = ET_PICTURE_TYPE_FRONT_COVER;
            }

            if (n)
            {
                description = !et_str_empty (n->data) ? n->data : "";

                n = g_slist_next (n);
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

            l = g_slist_next (l);
        }

        g_slist_free_full (strings, g_free);
        g_slist_free_full (types, g_free);
        g_slist_free_full (descs, g_free);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_COVER_ART);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_COVER_ART_DESCRIPTION);
        g_hash_table_remove (tags, ET_VORBIS_COMMENT_FIELD_COVER_ART_TYPE);
    }

    /* METADATA_BLOCK_PICTURE tag used for picture information. */
    if ((strings = g_hash_table_lookup (tags,
                                        ET_VORBIS_COMMENT_FIELD_METADATA_BLOCK_PICTURE)))
    {
        GSList *l;

        for (l = strings; l != NULL; l = g_slist_next (l))
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
            decoded_ustr = g_base64_decode (l->data, &decoded_size);

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

        g_slist_free_full (strings, g_free);
        g_hash_table_remove (tags,
                             ET_VORBIS_COMMENT_FIELD_METADATA_BLOCK_PICTURE);
    }

    /* Save unsupported fields. */
    g_hash_table_iter_init (&tags_iter, tags);

    while (g_hash_table_iter_next (&tags_iter, (gpointer *)&key,
                                   (gpointer *)&strings))
    {
        GSList *l;

        for (l = strings; l != NULL; l = g_slist_next (l))
        {
            FileTag->other = g_list_prepend (FileTag->other,
                                             g_strconcat (key, "=", l->data,
                                                          NULL));
        }

        g_slist_free_full (strings, g_free);
        g_hash_table_iter_remove (&tags_iter);
    }

    if (FileTag->other)
    {
        FileTag->other = g_list_reverse (FileTag->other);
    }

    /* The hash table should now only contain keys. */
    g_hash_table_unref (tags);
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
        add_to_guchar_str (ustring, &ustring_len, (const guchar *)mime,
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

        /* TODO: Determine the depth per pixel by querying the pixbuf to see
         * whether an alpha channel is present. */
        convert_to_byte_array (0, array);
        add_to_guchar_str (ustring, &ustring_len, array, 4);

        /* Non-indexed images should set this to zero. */
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
