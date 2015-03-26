/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
 * Copyright (C) 2001-2003  Jerome Couderc <easytag@gmail.com>
 * Copyright (C) 2003       Pavel Minayev <thalion@front.ru>
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

#ifdef ENABLE_FLAC

#include <glib/gi18n.h>
#include <errno.h>

#include "easytag.h"
#include "flac_private.h"
#include "flac_tag.h"
#include "vcedit.h"
#include "et_core.h"
#include "id3_tag.h"
#include "misc.h"
#include "setting.h"
#include "picture.h"
#include "charset.h"

#define MULTIFIELD_SEPARATOR " - "

/* FLAC uses Ogg Vorbis comments
 * Ogg Vorbis fields names :
 *  - TITLE        : Track name
 *  - VERSION      : The version field may be used to differentiate multiple version of the same track title in a single collection. (e.g. remix info)
 *  - ALBUM        : The collection name to which this track belongs
 *  - ALBUMARTIST  : Compilation Artist or overall artist of an album
 *  - TRACKNUMBER  : The track number of this piece if part of a specific larger collection or album
 *  - TRACKTOTAL   :
 *  - ARTIST       : Track performer
 *  - ORGANIZATION : Name of the organization producing the track (i.e. the 'record label')
 *  - DESCRIPTION  : A short text description of the contents
 *  - COMMENT      : same as DESCRIPTION
 *  - GENRE        : A short text indication of music genre
 *  - DATE         : Date the track was recorded
 *  - LOCATION     : Location where track was recorded
 *  - CONTACT      : Contact information for the creators or distributors of
 *                   the track. This could be a URL, an email address, the
 *                   physical address of the producing label.
 *  - COPYRIGHT    : Copyright information
 *  - ISRC         : ISRC number for the track; see the ISRC intro page for more information on ISRC numbers.
 *
 * Field names should not be 'internationalized'; this is a concession to simplicity
 * not an attempt to exclude the majority of the world that doesn't speak English.
 * Field *contents*, however, are represented in UTF-8 to allow easy representation
 * of any language.
 */

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
                     guint32 field_len)
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
 * Read tag data from a FLAC file using the level 2 flac interface,
 * Note:
 *  - if field is found but contains no info (strlen(str)==0), we don't read it
 */
gboolean
flac_tag_read_file_tag (GFile *file,
                        File_Tag *FileTag,
                        GError **error)
{
    FLAC__Metadata_Chain *chain;
    EtFlacReadState state;
    FLAC__IOCallbacks callbacks = { et_flac_read_func,
                                    NULL, /* Do not set a write callback. */
                                    et_flac_seek_func, et_flac_tell_func,
                                    et_flac_eof_func,
                                    et_flac_read_close_func };
    FLAC__Metadata_Iterator *iter;

    gchar *string = NULL;
    guint i;
    EtPicture *prev_pic = NULL;
    //gint j = 1;

    g_return_val_if_fail (file != NULL && FileTag != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    chain = FLAC__metadata_chain_new ();

    if (chain == NULL)
    {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_NOMEM, "%s",
                     g_strerror (ENOMEM));
        return FALSE;
    }

    state.error = NULL;
    state.istream = g_file_read (file, NULL, &state.error);
    state.seekable = G_SEEKABLE (state.istream);

    if (!FLAC__metadata_chain_read_with_callbacks (chain, &state, callbacks))
    {
        /* TODO: Provide a dedicated error enum corresponding to status. */
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "%s",
                     _("Error opening FLAC file"));
        et_flac_read_close_func (&state);

        return FALSE;
    }

    iter = FLAC__metadata_iterator_new ();

    if (iter == NULL)
    {
        et_flac_read_close_func (&state);
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_NOMEM, "%s",
                     g_strerror (ENOMEM));
        return FALSE;
    }

    FLAC__metadata_iterator_init (iter, chain);

    while (FLAC__metadata_iterator_next (iter))
    {
        FLAC__StreamMetadata *block;

        block = FLAC__metadata_iterator_get_block (iter);

        if (block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
        {
                FLAC__StreamMetadata_VorbisComment       *vc;
                FLAC__StreamMetadata_VorbisComment_Entry *field;
                gint   field_num;
                guint32 field_len;
                gchar *field_value;
                
                // Get comments from block
                vc = &block->data.vorbis_comment;

                /*********
                 * Title *
                 *********/
                field_num = 0;
                while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block,field_num,"TITLE")) >= 0 )
                {
                    /* Extract field value */
                    field = &vc->comments[field_num++];
                    field_value = memchr(field->entry, '=', field->length);

                    if (field_value)
                    {
                        field_value++;

                        if (!et_str_empty (field_value))
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value = validate_field_utf8 (field_value,
                                                               field_len);

                            set_or_append_field (&FileTag->title, field_value);
                        }
                    }
                }

                /**********
                 * Artist *
                 **********/
                field_num = 0;
                while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block,field_num,"ARTIST")) >= 0 )
                {
                    /* Extract field value */
                    field = &vc->comments[field_num++];
                    field_value = memchr(field->entry, '=', field->length);

                    if (field_value)
                    {
                        field_value++;

                        if (!et_str_empty (field_value))
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value = validate_field_utf8 (field_value,
                                                               field_len);

                            set_or_append_field (&FileTag->artist,
                                                 field_value);
                        }
                    }
                }

                /****************
                 * Album Artist *
                 ****************/
                field_num = 0;
                while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block,field_num,"ALBUMARTIST")) >= 0 )
                {
                    /* Extract field value */
                    field = &vc->comments[field_num++];
                    field_value = memchr(field->entry, '=', field->length);

                    if (field_value)
                    {
                        field_value++;

                        if (!et_str_empty (field_value))
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value = validate_field_utf8 (field_value,
                                                               field_len);

                            set_or_append_field (&FileTag->album_artist,
                                                 field_value);
                        }
                    }
                }

                /*********
                 * Album *
                 *********/
                field_num = 0;
                while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block,field_num,"ALBUM")) >= 0 )
                {
                    /* Extract field value */
                    field = &vc->comments[field_num++];
                    field_value = memchr(field->entry, '=', field->length);

                    if (field_value)
                    {
                        field_value++;

                        if (!et_str_empty (field_value))
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value = validate_field_utf8 (field_value,
                                                               field_len);

                            set_or_append_field (&FileTag->album, field_value);
                        }
                    }
                }

                /******************************
                 * Disc Number and Disc Total *
                 ******************************/
                if ((field_num = FLAC__metadata_object_vorbiscomment_find_entry_from (block, 0, "DISCTOTAL")) >= 0)
                {
                    /* Extract field value. */
                    field = &vc->comments[field_num];
                    field_value = memchr (field->entry, '=', field->length);

                    if (field_value)
                    {
                        field_value++;

                        if (!et_str_empty (field_value))
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value = validate_field_utf8 (field_value,
                                                               field_len);
                            FileTag->disc_total = et_disc_number_to_string (atoi (field_value));
                            g_free (field_value);
                        }
                    }
                    /* Discs is also filled below, if not done here. */
                }

                if ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block,0,"DISCNUMBER")) >= 0 )
                {
                    /* Extract field value */
                    field = &vc->comments[field_num];
                    field_value = memchr(field->entry, '=', field->length);

                    if (field_value)
                    {
                        field_value++;

                        if (!et_str_empty (field_value))
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value = validate_field_utf8 (field_value,
                                                               field_len);
                            string = g_utf8_strchr (field_value, -1, '/');

                            if (string && !FileTag->disc_total)
                            {
                                FileTag->disc_total = et_disc_number_to_string (atoi (string + 1));
                                *string = '\0';
                            }

                            FileTag->disc_number = et_disc_number_to_string (atoi (field_value));

                            g_free (field_value);
                        }
                    }
                }

                /********
                 * Year *
                 ********/
                if ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block,0,"DATE")) >= 0 )
                {
                    /* Extract field value */
                    field = &vc->comments[field_num];
                    field_value = memchr(field->entry, '=', field->length);

                    if (field_value)
                    {
                        field_value++;

                        if (!et_str_empty (field_value))
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value = validate_field_utf8 (field_value,
                                                               field_len);
                            FileTag->year = field_value;
                        }
                    }
                }

                /*************************
                 * Track and Total Track *
                 *************************/
                if ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block,0,"TRACKTOTAL")) >= 0 )
                {
                    /* Extract field value */
                    field = &vc->comments[field_num];
                    field_value = memchr(field->entry, '=', field->length);

                    if (field_value)
                    {
                        field_value++;

                        if (!et_str_empty (field_value))
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value = validate_field_utf8 (field_value,
                                                               field_len);
                            FileTag->track_total = et_track_number_to_string (atoi (field_value));
                            g_free (field_value);
                        }
                    }
                    // Below is also filled track_total if not done here
                }

                if ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block,0,"TRACKNUMBER")) >= 0 )
                {
                    /* Extract field value */
                    field = &vc->comments[field_num];
                    field_value = memchr(field->entry, '=', field->length);

                    if (field_value)
                    {
                        field_value++;

                        if (!et_str_empty (field_value))
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value = validate_field_utf8 (field_value,
                                                               field_len);
                            string = g_utf8_strchr(field_value, -1, '/');

                            if (string && !FileTag->track_total)
                            {
                                FileTag->track_total = et_track_number_to_string (atoi (string + 1));
                                *string = '\0';
                            }
                            FileTag->track = et_track_number_to_string (atoi (field_value));

                            g_free (field_value);
                        }
                    }
                }

                /*********
                 * Genre *
                 *********/
                field_num = 0;
                while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block,field_num,"GENRE")) >= 0 )
                {
                    /* Extract field value */
                    field = &vc->comments[field_num++];
                    field_value = memchr(field->entry, '=', field->length);

                    if (field_value)
                    {
                        field_value++;

                        if (!et_str_empty (field_value))
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value = validate_field_utf8 (field_value,
                                                               field_len);

                            set_or_append_field (&FileTag->genre, field_value);
                        }
                    }
                }

                /***********
                 * Comment *
                 ***********/
                field_num = 0;
                while ( 1 )
                {
                    gint field_num1, field_num2;

                    // The comment field can take two forms...
                    field_num1 = FLAC__metadata_object_vorbiscomment_find_entry_from(block,field_num,"DESCRIPTION");
                    field_num2 = FLAC__metadata_object_vorbiscomment_find_entry_from(block,field_num,"COMMENT");

                    if (field_num1 >= 0 && field_num2 >= 0)
                        // Note : We set field_num to the last "comment" field to avoid to concatenate 
                        // the DESCRIPTION and COMMENT field if there are both present (EasyTAG writes the both...)
                        if (field_num1 < field_num2)
                            field_num = field_num2;
                        else
                            field_num = field_num1;
                    else if (field_num1 >= 0)
                        field_num = field_num1;
                    else if (field_num2 >= 0)
                        field_num = field_num2;
                    else
                        break;

                    /* Extract field value */
                    field = &vc->comments[field_num++];
                    field_value = memchr(field->entry, '=', field->length);

                    if (field_value)
                    {
                        field_value++;

                        if (!et_str_empty (field_value))
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value = validate_field_utf8 (field_value,
                                                               field_len);

                            set_or_append_field (&FileTag->comment,
                                                 field_value);
                        }
                    }
                }

                /************
                 * Composer *
                 ************/
                field_num = 0;
                while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block,field_num,"COMPOSER")) >= 0 )
                {
                    /* Extract field value */
                    field = &vc->comments[field_num++];
                    field_value = memchr(field->entry, '=', field->length);

                    if (field_value)
                    {
                        field_value++;

                        if (!et_str_empty (field_value))
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value = validate_field_utf8 (field_value,
                                                               field_len);

                            set_or_append_field (&FileTag->composer,
                                                 field_value);
                        }
                    }
                }

                /*******************
                 * Original artist *
                 *******************/
                field_num = 0;
                while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block,field_num,"PERFORMER")) >= 0 )
                {
                    /* Extract field value */
                    field = &vc->comments[field_num++];
                    field_value = memchr(field->entry, '=', field->length);

                    if (field_value)
                    {
                        field_value++;

                        if (!et_str_empty (field_value))
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value = validate_field_utf8 (field_value,
                                                               field_len);

                            set_or_append_field (&FileTag->orig_artist,
                                                 field_value);
                        }
                    }
                }

                /*************
                 * Copyright *
                 *************/
                field_num = 0;
                while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block,field_num,"COPYRIGHT")) >= 0 )
                {
                    /* Extract field value */
                    field = &vc->comments[field_num++];
                    field_value = memchr(field->entry, '=', field->length);

                    if (field_value)
                    {
                        field_value++;

                        if (!et_str_empty (field_value))
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value = validate_field_utf8 (field_value,
                                                               field_len);

                            set_or_append_field (&FileTag->copyright,
                                                 field_value);
                        }
                    }
                }

                /*******
                 * URL *
                 *******/
                field_num = 0;
                while ((field_num = FLAC__metadata_object_vorbiscomment_find_entry_from (block, field_num, "CONTACT")) >= 0)
                {
                    /* Extract field value */
                    field = &vc->comments[field_num++];
                    field_value = memchr(field->entry, '=', field->length);

                    if (field_value)
                    {
                        field_value++;

                        if (!et_str_empty (field_value))
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value = validate_field_utf8 (field_value,
                                                               field_len);

                            set_or_append_field (&FileTag->url, field_value);
                        }
                    }
                }

                /**************
                 * Encoded by *
                 **************/
                field_num = 0;
                while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block,field_num,"ENCODED-BY")) >= 0 )
                {
                    /* Extract field value */
                    field = &vc->comments[field_num++];
                    field_value = memchr(field->entry, '=', field->length);

                    if (field_value)
                    {
                        field_value++;

                        if (!et_str_empty (field_value))
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value = validate_field_utf8 (field_value,
                                                               field_len);

                            set_or_append_field (&FileTag->encoded_by,
                                                 field_value);
                        }
                    }
                }

                /***************************
                 * Save unsupported fields *
                 ***************************/
                for (i=0;i<(guint)vc->num_comments;i++)
                {
                    field = &vc->comments[i];
                    if ( strncasecmp((gchar *)field->entry,"TITLE=",       MIN(6,  field->length)) != 0
                      && strncasecmp((gchar *)field->entry,"ARTIST=",      MIN(7,  field->length)) != 0
                      && strncasecmp((gchar *)field->entry,"ALBUMARTIST=", MIN(12, field->length)) != 0
                      && strncasecmp((gchar *)field->entry,"ALBUM=",       MIN(6,  field->length)) != 0
                      && strncasecmp((gchar *)field->entry,"DISCNUMBER=",  MIN(11, field->length)) != 0
                      && strncasecmp((gchar *)field->entry,"DISCTOTAL=",   MIN(10, field->length)) != 0
                      && strncasecmp((gchar *)field->entry,"DATE=",        MIN(5,  field->length)) != 0
                      && strncasecmp((gchar *)field->entry,"TRACKNUMBER=", MIN(12, field->length)) != 0
                      && strncasecmp((gchar *)field->entry,"TRACKTOTAL=",  MIN(11, field->length)) != 0
                      && strncasecmp((gchar *)field->entry,"GENRE=",       MIN(6,  field->length)) != 0
                      && strncasecmp((gchar *)field->entry,"DESCRIPTION=", MIN(12, field->length)) != 0
                      && strncasecmp((gchar *)field->entry,"COMMENT=",     MIN(8,  field->length)) != 0
                      && strncasecmp((gchar *)field->entry,"COMPOSER=",    MIN(9,  field->length)) != 0
                      && strncasecmp((gchar *)field->entry,"PERFORMER=",   MIN(10, field->length)) != 0
                      && strncasecmp((gchar *)field->entry,"COPYRIGHT=",   MIN(10, field->length)) != 0
                      && strncasecmp((gchar *)field->entry,"CONTACT=",     MIN(8,  field->length)) != 0
                      && strncasecmp((gchar *)field->entry,"ENCODED-BY=",  MIN(11, field->length)) != 0 )
                    {
                        //g_print("custom %*s\n", field->length, field->entry);
                        FileTag->other = g_list_append(FileTag->other,g_strndup((const gchar *)field->entry, field->length));
                    }
                }
        }
        else if (block->type == FLAC__METADATA_TYPE_PICTURE)
        {
            /* Picture. */
            FLAC__StreamMetadata_Picture *p;
            GBytes *bytes;
            EtPicture *pic;
        
            /* Get picture data from block. */
            p = &block->data.picture;

            bytes = g_bytes_new (p->data, p->data_length);
        
            pic = et_picture_new (p->type, (const gchar *)p->description,
                                  0, 0, bytes);
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
    }

    FLAC__metadata_iterator_delete (iter);
    FLAC__metadata_chain_delete (chain);
    et_flac_read_close_func (&state);

#ifdef ENABLE_MP3
    /* If no FLAC vorbis tag found : we try to get the ID3 tag if it exists
     * (but it will be deleted when rewriting the tag) */
    if ( FileTag->title       == NULL
      && FileTag->artist      == NULL
      && FileTag->album_artist == NULL
      && FileTag->album       == NULL
      && FileTag->disc_number == NULL
      && FileTag->disc_total == NULL
      && FileTag->year        == NULL
      && FileTag->track       == NULL
      && FileTag->track_total == NULL
      && FileTag->genre       == NULL
      && FileTag->comment     == NULL
      && FileTag->composer    == NULL
      && FileTag->orig_artist == NULL
      && FileTag->copyright   == NULL
      && FileTag->url         == NULL
      && FileTag->encoded_by  == NULL
      && FileTag->picture     == NULL)
    {
        id3tag_read_file_tag (file, FileTag, NULL);

        // If an ID3 tag has been found (and no FLAC tag), we mark the file as
        // unsaved to rewrite a flac tag.
        if ( FileTag->title       != NULL
          || FileTag->artist      != NULL
          || FileTag->album_artist != NULL
          || FileTag->album       != NULL
          || FileTag->disc_number != NULL
          || FileTag->disc_total != NULL
          || FileTag->year        != NULL
          || FileTag->track       != NULL
          || FileTag->track_total != NULL
          || FileTag->genre       != NULL
          || FileTag->comment     != NULL
          || FileTag->composer    != NULL
          || FileTag->orig_artist != NULL
          || FileTag->copyright   != NULL
          || FileTag->url         != NULL
          || FileTag->encoded_by  != NULL
          || FileTag->picture     != NULL)
        {
            FileTag->saved = FALSE;
        }
    }
#endif

    return TRUE;
}

/*
 * vc_block_append_single_tag:
 * @vc_block: the Vorbis comment in which to add the tag
 * @tag_name: the name of the tag
 * @value: the value of the tag
 *
 * Save field value in a single tag.
 */
static void
vc_block_append_single_tag (FLAC__StreamMetadata *vc_block,
                            const gchar *tag_name,
                            const gchar *value)
{
    FLAC__StreamMetadata_VorbisComment_Entry field;
    char *string = g_strconcat (tag_name, value, NULL);
    FLAC__bool result;

    field.entry = (FLAC__byte *)string;
    field.length = strlen (string);
    result = FLAC__metadata_object_vorbiscomment_append_comment (vc_block,
                                                                 field, true);
    g_free (string);

    if (result != true)
    {
        g_critical ("Invalid Vorbis comment, or memory allocation failed, when writing FLAC tag '%s' with value '%s'",
                    tag_name, value);
    }
}

/*
 * vc_block_append_multiple_tags:
 * @vc_block: the Vorbis comment block to which tags should be appended
 * @tag_name: the name of the tag
 * @values: the values of the tag
 *
 * Append multiple copies of the supplied @tag_name to @vc_block, splitting
 * @values at %MULTIFIELD_SEPARATOR.
 */
static void
vc_block_append_multiple_tags (FLAC__StreamMetadata *vc_block,
                               const gchar *tag_name,
                               const gchar *values)
{
    gchar **strings = g_strsplit (values, MULTIFIELD_SEPARATOR, 255);
    guint i;
    guint len;
    
    for (i = 0, len = g_strv_length (strings); i < len; i++)
    {
        if (!et_str_empty (strings[i]))
        {
            vc_block_append_single_tag (vc_block, tag_name, strings[i]);
        }
    }

    g_strfreev (strings);
}

/*
 * vc_block_append_tag:
 * @vc_block: the Vorbis comment block to which a tag should be appended
 * @tag_name: the name of the tag, including the trailing '=', or the empty
 *            string (if @value is to be taken as the combination of the tag
 *            name and value)
 * @value: the value of the tag
 * @split: %TRUE to split @value into multiple tags at %MULTIFIELD_SEPARATOR,
 *         %FALSE to keep the tag value unchanged
 *
 * Append the supplied @tag_name and @value to @vc_block, optionally splitting
 * @value if @split is %TRUE.
 */
static void
vc_block_append_tag (FLAC__StreamMetadata *vc_block,
                     const gchar *tag_name,
                     const gchar *value,
                     gboolean split)
{
    if (value && split)
    {
        vc_block_append_multiple_tags (vc_block, tag_name, value);
    }
    else if (value)
    {
        vc_block_append_single_tag (vc_block, tag_name, value);
    }
}

/*
 * Write Flac tag, using the level 2 flac interface
 */
gboolean
flac_tag_write_file_tag (const ET_File *ETFile,
                         GError **error)
{
    const File_Tag *FileTag;
    GFile *file;
    GFileIOStream *iostream;
    EtFlacWriteState state;
    FLAC__IOCallbacks callbacks = { et_flac_read_func, et_flac_write_func,
                                    et_flac_seek_func, et_flac_tell_func,
                                    et_flac_eof_func,
                                    et_flac_write_close_func };
    const gchar *filename;
    const gchar *filename_utf8;
    const gchar *flac_error_msg;
    FLAC__Metadata_Chain *chain;
    FLAC__Metadata_Iterator *iter;
    FLAC__StreamMetadata_VorbisComment_Entry vce_field_vendor_string; // To save vendor string
    gboolean vce_field_vendor_string_found = FALSE;

    g_return_val_if_fail (ETFile != NULL && ETFile->FileTag != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    FileTag       = (File_Tag *)ETFile->FileTag->data;
    filename      = ((File_Name *)ETFile->FileNameCur->data)->value;
    filename_utf8 = ((File_Name *)ETFile->FileNameCur->data)->value_utf8;

    /* libFLAC is able to detect (and skip) ID3v2 tags by itself */
    
    /* Create a new chain instance to get all blocks in one time. */
    chain = FLAC__metadata_chain_new ();

    if (chain == NULL)
    {
        flac_error_msg = FLAC__Metadata_ChainStatusString[FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR];
        
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Error while opening file ‘%s’ as FLAC: %s"),
                     filename_utf8, flac_error_msg);
        return FALSE;
    }

    file = g_file_new_for_path (filename);

    state.file = file;
    state.error = NULL;
    /* TODO: Fallback to an in-memory copy of the file for non-local files,
     * where creation of the GFileIOStream may fail. */
    iostream = g_file_open_readwrite (file, NULL, &state.error);

    if (iostream == NULL)
    {
        FLAC__metadata_chain_delete (chain);
        g_propagate_error (error, state.error);
        g_object_unref (file);
        return FALSE;
    }

    state.istream = G_FILE_INPUT_STREAM (g_io_stream_get_input_stream (G_IO_STREAM (iostream)));
    state.ostream = G_FILE_OUTPUT_STREAM (g_io_stream_get_output_stream (G_IO_STREAM (iostream)));
    state.seekable = G_SEEKABLE (iostream);
    state.iostream = iostream;

    if (!FLAC__metadata_chain_read_with_callbacks (chain, &state, callbacks))
    {
        const FLAC__Metadata_ChainStatus status = FLAC__metadata_chain_status (chain);
        flac_error_msg = FLAC__Metadata_ChainStatusString[status];
        FLAC__metadata_chain_delete (chain);

        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Error while opening file ‘%s’ as FLAC: %s"),
                     filename_utf8, flac_error_msg);
        et_flac_write_close_func (&state);
        return FALSE;
    }
    
    /* Create a new iterator instance for the chain. */
    iter = FLAC__metadata_iterator_new ();

    if (iter == NULL)
    {
        flac_error_msg = FLAC__Metadata_ChainStatusString[FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR];

        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Error while opening file ‘%s’ as FLAC: %s"),
                     filename_utf8, flac_error_msg);
        FLAC__metadata_chain_delete (chain);
        et_flac_write_close_func (&state);
        return FALSE;
    }
    
    FLAC__metadata_iterator_init (iter, chain);
    
    while (FLAC__metadata_iterator_next (iter))
    {
        const FLAC__MetadataType block_type = FLAC__metadata_iterator_get_block_type (iter);

        /* TODO: Modify the blocks directly, rather than deleting and
         * recreating. */
        if (block_type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
        {
            // Delete the VORBIS_COMMENT block and convert to padding. But before, save the original vendor string.
            /* Get block data. */
            FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(iter);
            FLAC__StreamMetadata_VorbisComment *vc = &block->data.vorbis_comment;
            
            if (vc->vendor_string.entry != NULL)
            {
                // Get initial vendor string, to don't alterate it by FLAC__VENDOR_STRING when saving file
                vce_field_vendor_string.entry = (FLAC__byte *)g_strdup ((gchar *)vc->vendor_string.entry);
                vce_field_vendor_string.length = strlen ((gchar *)vce_field_vendor_string.entry);
                vce_field_vendor_string_found = TRUE;
            }
            
            /* Free block data. */
            FLAC__metadata_iterator_delete_block (iter, true);
        }
        else if (block_type == FLAC__METADATA_TYPE_PICTURE)
        {
            /* Delete all the PICTURE blocks, and convert to padding. */
            FLAC__metadata_iterator_delete_block (iter, true);
        }
    }
    
    
    //
    // Create and insert a new VORBISCOMMENT block
    //
    {
        FLAC__StreamMetadata *vc_block; // For vorbis comments
        GList *l;
        
        // Allocate a block for Vorbis comments
        vc_block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);

        // Set the original vendor string, else will be use the version of library
        if (vce_field_vendor_string_found)
        {
            // must set 'copy' param to false, because the API will reuse the  pointer of an empty
            // string (yet still return 'true', indicating it was copied); the string is free'd during
            // metadata_chain_delete routine
            FLAC__metadata_object_vorbiscomment_set_vendor_string(vc_block, vce_field_vendor_string, /*copy=*/false);
        }


        /*********
         * Title *
         *********/
        vc_block_append_tag (vc_block, "TITLE=", FileTag->title,
                             g_settings_get_boolean (MainSettings,
                                                     "ogg-split-title"));

        /**********
         * Artist *
         **********/
        vc_block_append_tag (vc_block, "ARTIST=", FileTag->artist,
                             g_settings_get_boolean (MainSettings,
                                                     "ogg-split-artist"));

        /****************
         * Album Artist *
         ****************/
        vc_block_append_tag (vc_block, "ALBUMARTIST=", FileTag->album_artist,
                             g_settings_get_boolean (MainSettings,
                                                     "ogg-split-artist"));

        /*********
         * Album *
         *********/
        vc_block_append_tag (vc_block, "ALBUM=", FileTag->album,
                             g_settings_get_boolean (MainSettings,
                                                     "ogg-split-album"));

        /******************************
         * Disc Number and Disc Total *
         ******************************/
        vc_block_append_tag (vc_block, "DISCNUMBER=", FileTag->disc_number,
                             FALSE);
        vc_block_append_tag (vc_block, "DISCTOTAL=", FileTag->disc_total,
                             FALSE);

        /********
         * Year *
         ********/
        vc_block_append_tag (vc_block, "DATE=", FileTag->year, FALSE);

        /*************************
         * Track and Total Track *
         *************************/
        vc_block_append_tag (vc_block, "TRACKNUMBER=", FileTag->track, FALSE);
        vc_block_append_tag (vc_block, "TRACKTOTAL=", FileTag->track_total,
                             FALSE);

        /*********
         * Genre *
         *********/
        vc_block_append_tag (vc_block, "GENRE=", FileTag->genre,
                             g_settings_get_boolean (MainSettings,
                                                     "ogg-split-genre"));

        /***********
         * Comment *
         ***********/
        vc_block_append_tag (vc_block, "DESCRIPTION=", FileTag->comment,
                             g_settings_get_boolean (MainSettings,
                                                     "ogg-split-comment"));

        /************
         * Composer *
         ************/
        vc_block_append_tag (vc_block, "COMPOSER=", FileTag->composer,
                             g_settings_get_boolean (MainSettings,
                                                     "ogg-split-composer"));

        /*******************
         * Original artist *
         *******************/
        vc_block_append_tag (vc_block, "PERFORMER=", FileTag->orig_artist,
                             g_settings_get_boolean (MainSettings,
                                                     "ogg-split-original-artist"));

        /*************
         * Copyright *
         *************/
        vc_block_append_tag (vc_block, "COPYRIGHT=", FileTag->copyright,
                             FALSE);

        /*******
         * URL *
         *******/
        vc_block_append_tag (vc_block, "CONTACT=", FileTag->url, FALSE);

        /**************
         * Encoded by *
         **************/
        vc_block_append_tag (vc_block, "ENCODED-BY=", FileTag->encoded_by,
                             FALSE);


        /**************************
         * Set unsupported fields *
         **************************/
        for (l = FileTag->other; l != NULL; l = g_list_next (l))
        {
            vc_block_append_tag (vc_block, "", (gchar *)l->data, FALSE);
        }

        // Add the block to the the chain (so we don't need to free the block)
        FLAC__metadata_iterator_insert_block_after(iter, vc_block);
    }
    
    
    
    //
    // Create and insert PICTURE blocks
    //
    
    /***********
     * Picture *
     ***********/
    {
        EtPicture *pic = FileTag->picture;

        while (pic)
        {
            /* TODO: Can this ever be NULL? */
            if (pic->bytes)
            {
                const gchar *violation;
                FLAC__StreamMetadata *picture_block; // For picture data
                Picture_Format format;
                gconstpointer data;
                gsize data_size;
                
                // Allocate block for picture data
                picture_block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PICTURE);
                
                // Type
                picture_block->data.picture.type = pic->type;
                
                // Mime type
                format = Picture_Format_From_Data(pic);
                /* Safe to pass a const string, according to the FLAC API
                 * reference. */
                FLAC__metadata_object_picture_set_mime_type(picture_block, (gchar *)Picture_Mime_Type_String(format), TRUE);

                // Description
                if (pic->description)
                {
                    FLAC__metadata_object_picture_set_description(picture_block, (FLAC__byte *)pic->description, TRUE);
                }
                
                // Resolution
                picture_block->data.picture.width  = pic->width;
                picture_block->data.picture.height = pic->height;
                picture_block->data.picture.depth  = 0;

                /* Picture data. */
                data = g_bytes_get_data (pic->bytes, &data_size);
                FLAC__metadata_object_picture_set_data (picture_block,
                                                        (FLAC__byte *)data,
                                                        (FLAC__uint32)data_size,
                                                        TRUE);
                
                if (!FLAC__metadata_object_picture_is_legal (picture_block,
                                                             &violation))
                {
                    g_critical ("Created an invalid picture block: ‘%s’",
                                violation);
                    FLAC__metadata_object_delete (picture_block);
                }
                else
                {
                    // Add the block to the the chain (so we don't need to free the block)
                    FLAC__metadata_iterator_insert_block_after(iter, picture_block);
                }
            }
            
            pic = pic->next;
        }
    }
    
    FLAC__metadata_iterator_delete (iter);
    
    //
    // Prepare for writing tag
    //
    
    FLAC__metadata_chain_sort_padding (chain);
 
    /* Write tag. */
    if (FLAC__metadata_chain_check_if_tempfile_needed (chain, true))
    {
        EtFlacWriteState temp_state;
        GFile *temp_file;
        GFileIOStream *temp_iostream;
        GError *temp_error = NULL;

        temp_file = g_file_new_tmp ("easytag-XXXXXX", &temp_iostream,
                                    &temp_error);

        if (temp_file == NULL)
        {
            FLAC__metadata_chain_delete (chain);
            g_propagate_error (error, temp_error);
            et_flac_write_close_func (&state);
            return FALSE;
        }

        temp_state.file = temp_file;
        temp_state.error = NULL;
        temp_state.istream = G_FILE_INPUT_STREAM (g_io_stream_get_input_stream (G_IO_STREAM (temp_iostream)));
        temp_state.ostream = G_FILE_OUTPUT_STREAM (g_io_stream_get_output_stream (G_IO_STREAM (temp_iostream)));
        temp_state.seekable = G_SEEKABLE (temp_iostream);
        temp_state.iostream = temp_iostream;

        if (!FLAC__metadata_chain_write_with_callbacks_and_tempfile (chain,
                                                                     true,
                                                                     &state,
                                                                     callbacks,
                                                                     &temp_state,
                                                                     callbacks))
        {
            const FLAC__Metadata_ChainStatus status = FLAC__metadata_chain_status (chain);
            flac_error_msg = FLAC__Metadata_ChainStatusString[status];

            FLAC__metadata_chain_delete (chain);
            et_flac_write_close_func (&temp_state);
            et_flac_write_close_func (&state);

            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                         _("Failed to write comments to file ‘%s’: %s"),
                         filename_utf8, flac_error_msg);
            return FALSE;
        }

        if (!g_file_move (temp_file, file, G_FILE_COPY_OVERWRITE, NULL, NULL,
                          NULL, &state.error))
        {
            FLAC__metadata_chain_delete (chain);
            et_flac_write_close_func (&temp_state);

            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                         _("Failed to write comments to file ‘%s’: %s"),
                         filename_utf8, state.error->message);
            et_flac_write_close_func (&state);
            return FALSE;
        }

        et_flac_write_close_func (&temp_state);
    }
    else
    {
        if (!FLAC__metadata_chain_write_with_callbacks (chain, true, &state,
                                                        callbacks))
        {
            const FLAC__Metadata_ChainStatus status = FLAC__metadata_chain_status (chain);
            flac_error_msg = FLAC__Metadata_ChainStatusString[status];

            FLAC__metadata_chain_delete (chain);
            et_flac_write_close_func (&state);

            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                         _("Failed to write comments to file ‘%s’: %s"),
                         filename_utf8, flac_error_msg);
            return FALSE;
        }
    }

    FLAC__metadata_chain_delete (chain);
    et_flac_write_close_func (&state);

#ifdef ENABLE_MP3
    {
        // Delete the ID3 tags (create a dummy ETFile for the Id3tag_... function)
        ET_File   *ETFile_tmp    = ET_File_Item_New();
        File_Name *FileName_tmp = et_file_name_new ();
        File_Tag  *FileTag_tmp = et_file_tag_new ();
        // Same file...
        FileName_tmp->value      = g_strdup(filename);
        FileName_tmp->value_utf8 = g_strdup(filename_utf8); // Not necessary to fill 'value_ck'
        ETFile_tmp->FileNameList = g_list_append(NULL,FileName_tmp);
        ETFile_tmp->FileNameCur  = ETFile_tmp->FileNameList;
        // With empty tag...
        ETFile_tmp->FileTagList  = g_list_append(NULL,FileTag_tmp);
        ETFile_tmp->FileTag      = ETFile_tmp->FileTagList;
        id3tag_write_file_tag (ETFile_tmp, NULL);
        ET_Free_File_List_Item(ETFile_tmp);
    }
#endif
    
    return TRUE;
}


#endif /* ENABLE_FLAC */
