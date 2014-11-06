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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <FLAC/metadata.h>
#include <unistd.h>

#include "easytag.h"
#include "flac_tag.h"
#include "vcedit.h"
#include "et_core.h"
#include "id3_tag.h"
#include "log.h"
#include "misc.h"
#include "setting.h"
#include "picture.h"
#include "charset.h"


/***************
 * Declaration *
 ***************/

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



/**************
 * Prototypes *
 **************/

static gboolean Flac_Write_Delimetered_Tag (FLAC__StreamMetadata *vc_block, const gchar *tag_name, gchar *values);
static gboolean Flac_Write_Tag (FLAC__StreamMetadata *vc_block, const gchar *tag_name, gchar *value);
static gboolean Flac_Set_Tag (FLAC__StreamMetadata *vc_block, const gchar *tag_name, gchar *value, gboolean split);


/*************
 * Functions *
 *************/

/*
 * Read tag data from a FLAC file using the level 1 flac interface,
 * Note:
 *  - if field is found but contains no info (strlen(str)==0), we don't read it
 */
gboolean
flac_tag_read_file_tag (GFile *file,
                        File_Tag *FileTag,
                        GError **error)
{
    FLAC__Metadata_SimpleIterator *iter;
    const gchar *flac_error_msg;
    gchar *string = NULL;
    gchar *filename;
    gchar *filename_utf8;
    guint i;
    Picture *prev_pic = NULL;
    //gint j = 1;

    g_return_val_if_fail (file != NULL && FileTag != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    /* Initialize the iterator for the blocks. */
    filename = g_file_get_path (file);
    iter = FLAC__metadata_simple_iterator_new();

    if ( iter == NULL || !FLAC__metadata_simple_iterator_init(iter, filename, true, false) )
    {
        if ( iter == NULL )
        {
            // Error with "FLAC__metadata_simple_iterator_new"
            flac_error_msg = FLAC__Metadata_SimpleIteratorStatusString[FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR];
        }else
        {
            // Error with "FLAC__metadata_simple_iterator_init"
            FLAC__Metadata_SimpleIteratorStatus status = FLAC__metadata_simple_iterator_status(iter);
            flac_error_msg = FLAC__Metadata_SimpleIteratorStatusString[status];
            
            FLAC__metadata_simple_iterator_delete(iter);
        }

        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Error while opening file: %s"), flac_error_msg);
        g_free (filename);
        return FALSE;
    }
    
    filename_utf8 = filename_to_display (filename);
    g_free (filename);

    /* libFLAC is able to detect (and skip) ID3v2 tags by itself */

    while (FLAC__metadata_simple_iterator_next(iter))
    {
        // Get block data
        FLAC__StreamMetadata *block = FLAC__metadata_simple_iterator_get_block(iter);
        //g_print("Read: %d %s -> block type: %d\n",j++,g_path_get_basename(filename),FLAC__metadata_simple_iterator_get_block_type(iter));
        
        // Action to do according the type
        switch ( FLAC__metadata_simple_iterator_get_block_type(iter) )
        {
            //
            // Read the VORBIS_COMMENT block (only one should exist)
            //
            case FLAC__METADATA_TYPE_VORBIS_COMMENT:
            {
                FLAC__StreamMetadata_VorbisComment       *vc;
                FLAC__StreamMetadata_VorbisComment_Entry *field;
                gint   field_num;
                gint   field_len;
                gchar *field_value;
                gchar *field_value_tmp;
                
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
                        if ( field_value && g_utf8_strlen(field_value, -1) > 0 )
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value_tmp = g_strndup(field_value, field_len);
                            field_value = Try_To_Validate_Utf8_String(field_value_tmp);
                            g_free(field_value_tmp);
                            if (FileTag->title==NULL)
                                FileTag->title = g_strdup(field_value);
                            else
                                FileTag->title = g_strconcat(FileTag->title,MULTIFIELD_SEPARATOR,field_value,NULL);
                            g_free(field_value);
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
                        if ( field_value && g_utf8_strlen(field_value, -1) > 0 )
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value_tmp = g_strndup(field_value, field_len);
                            field_value = Try_To_Validate_Utf8_String(field_value_tmp);
                            g_free(field_value_tmp);
                            if (FileTag->artist==NULL)
                                FileTag->artist = g_strdup(field_value);
                            else
                                FileTag->artist = g_strconcat(FileTag->artist,MULTIFIELD_SEPARATOR,field_value,NULL);
                            g_free(field_value);
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
                        if ( field_value && g_utf8_strlen(field_value, -1) > 0 )
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value_tmp = g_strndup(field_value, field_len);
                            field_value = Try_To_Validate_Utf8_String(field_value_tmp);
                            g_free(field_value_tmp);
                            if (FileTag->album_artist==NULL)
                                FileTag->album_artist = g_strdup(field_value);
                            else
                                FileTag->album_artist = g_strconcat(FileTag->album_artist,MULTIFIELD_SEPARATOR,field_value,NULL);
                            g_free(field_value);
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
                        if ( field_value && g_utf8_strlen(field_value, -1) > 0 )
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value_tmp = g_strndup(field_value, field_len);
                            field_value = Try_To_Validate_Utf8_String(field_value_tmp);
                            g_free(field_value_tmp);
                            if (FileTag->album==NULL)
                                FileTag->album = g_strdup(field_value);
                            else
                                FileTag->album = g_strconcat(FileTag->album,MULTIFIELD_SEPARATOR,field_value,NULL);
                            g_free(field_value);
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
                        if (field_value && g_utf8_strlen (field_value, -1) > 0)
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value_tmp = g_strndup (field_value,
                                                         field_len);
                            field_value = Try_To_Validate_Utf8_String (field_value_tmp);
                            g_free (field_value_tmp);

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
                        if ( field_value && g_utf8_strlen(field_value, -1) > 0 )
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value_tmp = g_strndup(field_value, field_len);
                            field_value = Try_To_Validate_Utf8_String(field_value_tmp);
                            g_free(field_value_tmp);
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
                        if ( field_value && g_utf8_strlen(field_value, -1) > 0 )
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value_tmp = g_strndup(field_value, field_len);
                            field_value = Try_To_Validate_Utf8_String(field_value_tmp);
                            g_free(field_value_tmp);
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
                        if ( field_value && g_utf8_strlen(field_value, -1) > 0 )
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value_tmp = g_strndup(field_value, field_len);
                            field_value = Try_To_Validate_Utf8_String(field_value_tmp);
                            g_free(field_value_tmp);

                            FileTag->track_total = et_track_number_to_string (atoi (field_value));
                            g_free(field_value);
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
                        if ( field_value && g_utf8_strlen(field_value, -1) > 0 )
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value_tmp = g_strndup(field_value, field_len);
                            field_value = Try_To_Validate_Utf8_String(field_value_tmp);
                            g_free(field_value_tmp);
                            string = g_utf8_strchr(field_value, -1, '/');

                            if (string && !FileTag->track_total)
                            {
                                FileTag->track_total = et_track_number_to_string (atoi (string + 1));
                                *string = '\0';
                            }
                            FileTag->track = et_track_number_to_string (atoi (field_value));

                            g_free(field_value);
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
                        if ( field_value && g_utf8_strlen(field_value, -1) > 0 )
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value_tmp = g_strndup(field_value, field_len);
                            field_value = Try_To_Validate_Utf8_String(field_value_tmp);
                            g_free(field_value_tmp);
                            if (FileTag->genre==NULL)
                                FileTag->genre = g_strdup(field_value);
                            else
                                FileTag->genre = g_strconcat(FileTag->genre,MULTIFIELD_SEPARATOR,field_value,NULL);
                            g_free(field_value);
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
                        if ( field_value && g_utf8_strlen(field_value, -1) > 0 )
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value_tmp = g_strndup(field_value, field_len);
                            field_value = Try_To_Validate_Utf8_String(field_value_tmp);
                            g_free(field_value_tmp);
                            if (FileTag->comment==NULL)
                                FileTag->comment = g_strdup(field_value);
                            else
                                FileTag->comment = g_strconcat(FileTag->comment,MULTIFIELD_SEPARATOR,field_value,NULL);
                            g_free(field_value);
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
                        if ( field_value && g_utf8_strlen(field_value, -1) > 0 )
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value_tmp = g_strndup(field_value, field_len);
                            field_value = Try_To_Validate_Utf8_String(field_value_tmp);
                            g_free(field_value_tmp);
                            if (FileTag->composer==NULL)
                                FileTag->composer = g_strdup(field_value);
                            else
                                FileTag->composer = g_strconcat(FileTag->composer,MULTIFIELD_SEPARATOR,field_value,NULL);
                            g_free(field_value);
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
                        if ( field_value && g_utf8_strlen(field_value, -1) > 0 )
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value_tmp = g_strndup(field_value, field_len);
                            field_value = Try_To_Validate_Utf8_String(field_value_tmp);
                            g_free(field_value_tmp);
                            if (FileTag->orig_artist==NULL)
                                FileTag->orig_artist = g_strdup(field_value);
                            else
                                FileTag->orig_artist = g_strconcat(FileTag->orig_artist,MULTIFIELD_SEPARATOR,field_value,NULL);
                            g_free(field_value);
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
                        if ( field_value && g_utf8_strlen(field_value, -1) > 0 )
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value_tmp = g_strndup(field_value, field_len);
                            field_value = Try_To_Validate_Utf8_String(field_value_tmp);
                            g_free(field_value_tmp);
                            if (FileTag->copyright==NULL)
                                FileTag->copyright = g_strdup(field_value);
                            else
                                FileTag->copyright = g_strconcat(FileTag->copyright,MULTIFIELD_SEPARATOR,field_value,NULL);
                            g_free(field_value);
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
                        if ( field_value && g_utf8_strlen(field_value, -1) > 0 )
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value_tmp = g_strndup(field_value, field_len);
                            field_value = Try_To_Validate_Utf8_String(field_value_tmp);
                            g_free(field_value_tmp);
                            if (FileTag->url==NULL)
                                FileTag->url = g_strdup(field_value);
                            else
                                FileTag->url = g_strconcat(FileTag->url,MULTIFIELD_SEPARATOR,field_value,NULL);
                            g_free(field_value);
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
                        if ( field_value && g_utf8_strlen(field_value, -1) > 0 )
                        {
                            field_len = field->length - (field_value - (gchar*) field->entry);
                            field_value_tmp = g_strndup(field_value, field_len);
                            field_value = Try_To_Validate_Utf8_String(field_value_tmp);
                            g_free(field_value_tmp);
                            if (FileTag->encoded_by==NULL)
                                FileTag->encoded_by = g_strdup(field_value);
                            else
                                FileTag->encoded_by = g_strconcat(FileTag->encoded_by,MULTIFIELD_SEPARATOR,field_value,NULL);
                            g_free(field_value);
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
                
                break;
            }

            
            //
            // Read the PICTURE block (severals can exist)
            //
            case FLAC__METADATA_TYPE_PICTURE: 
            {
                
                /***********
                 * Picture *
                 ***********/
                FLAC__StreamMetadata_Picture *p;
                Picture *pic;
            
                // Get picture data from block
                p = &block->data.picture;
            
                pic = Picture_Allocate();
                if (!prev_pic)
                    FileTag->picture = pic;
                else
                    prev_pic->next = pic;
                prev_pic = pic;

                pic->size = p->data_length;
                pic->data = g_memdup(p->data,pic->size);
                pic->type = p->type;
                pic->description = g_strdup((gchar *)p->description);
                // Not necessary: will be calculated later
                //pic->height = p->height;
                //pic->width  = p->width;

                //g_print("Image type : %s\n",FLAC__StreamMetadata_Picture_TypeString[p->type]);
                //g_print("Mime    type : %s\n",p->mime_type);

                break;
            }
                
            default:
                break;
        }
        
        /* Free block data. */
        FLAC__metadata_object_delete (block);
    }
    
    // Free iter
    FLAC__metadata_simple_iterator_delete(iter);


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
        /* Ignore errors. */
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

    g_free(filename_utf8);
    return TRUE;
}



/*
 * Save field value in separated tags if it contains multifields
 */
static gboolean Flac_Write_Delimetered_Tag (FLAC__StreamMetadata *vc_block, const gchar *tag_name, gchar *values)
{
    gchar **strings = g_strsplit(values,MULTIFIELD_SEPARATOR,255);
    unsigned int i=0;
    
    for (i=0;i<g_strv_length(strings);i++)
    {
        if (strlen(strings[i])>0)
        {
            Flac_Write_Tag(vc_block, tag_name, strings[i]);
        }
    }
    g_strfreev(strings);
    return TRUE;
}

/*
 * Save field value in a single tag
 */
static gboolean Flac_Write_Tag (FLAC__StreamMetadata *vc_block, const gchar *tag_name, gchar *value)
{
    FLAC__StreamMetadata_VorbisComment_Entry field;
    char *string = g_strconcat(tag_name,value,NULL);

    field.entry = (FLAC__byte *)string;
    field.length = strlen(string); // Warning : g_utf8_strlen doesn't count the multibyte characters. Here we need the allocated size.
    FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
    g_free(string);
    return TRUE;
}

static gboolean Flac_Set_Tag (FLAC__StreamMetadata *vc_block, const gchar *tag_name, gchar *value, gboolean split)
{
    if ( value && split ) {
        return Flac_Write_Delimetered_Tag(vc_block,tag_name,value);
    } else if ( value ) {
        return Flac_Write_Tag(vc_block,tag_name,value);
    }

    return FALSE;
}

/*
 * Write Flac tag, using the level 2 flac interface
 */
gboolean
flac_tag_write_file_tag (const ET_File *ETFile,
                         GError **error)
{
    const File_Tag *FileTag;
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
    
    // Create a new chain instance to get all blocks in one time
    chain = FLAC__metadata_chain_new();
    if (chain == NULL || !FLAC__metadata_chain_read(chain,filename))
    {
        if (chain == NULL)
        {
            // Error with "FLAC__metadata_chain_new"
            flac_error_msg = FLAC__Metadata_ChainStatusString[FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR];
        }else
        {
            // Error with "FLAC__metadata_chain_read"
            FLAC__Metadata_ChainStatus status = FLAC__metadata_chain_status(chain);
            flac_error_msg = FLAC__Metadata_ChainStatusString[status];
            
            FLAC__metadata_chain_delete(chain);
        }
        
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Error while opening file ‘%s’ as FLAC: %s"),
                     filename_utf8, flac_error_msg);
        return FALSE;
    }
    
    // Create a new iterator instance for the chain
    iter = FLAC__metadata_iterator_new();
    if (iter == NULL)
    {
        flac_error_msg = FLAC__Metadata_ChainStatusString[FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR];

        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Error while opening file ‘%s’ as FLAC: %s"),
                     filename_utf8, flac_error_msg);
        return FALSE;
    }
    
    // Initialize the iterator to point to the first metadata block in the given chain.
    FLAC__metadata_iterator_init(iter,chain);
    
    while (FLAC__metadata_iterator_next(iter))
    {
        //g_print("Write: %d %s -> block type: %d\n",j++,g_path_get_basename(filename),FLAC__metadata_iterator_get_block_type(iter));
        
        // Action to do according the type
        switch ( FLAC__metadata_iterator_get_block_type(iter) )
        {
            //
            // Delete the VORBIS_COMMENT block and convert to padding. But before, save the original vendor string.
            //
            case FLAC__METADATA_TYPE_VORBIS_COMMENT:
            {
                // Get block data
                FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(iter);
                FLAC__StreamMetadata_VorbisComment *vc = &block->data.vorbis_comment;
                
                if (vc->vendor_string.entry != NULL)
                {
                    // Get initial vendor string, to don't alterate it by FLAC__VENDOR_STRING when saving file
                    vce_field_vendor_string.entry = (FLAC__byte *)g_strdup((gchar *)vc->vendor_string.entry);
                    vce_field_vendor_string.length = strlen((gchar *)vce_field_vendor_string.entry);
                    vce_field_vendor_string_found = TRUE;
                }
                
                // Free block data
                FLAC__metadata_iterator_delete_block(iter,true);
                break;
            }
            
            //
            // Delete all the PICTURE blocks, and convert to padding
            //
            case FLAC__METADATA_TYPE_PICTURE: 
            {
                FLAC__metadata_iterator_delete_block(iter,true);
                break;
            }
            
            default:
                break;
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
        Flac_Set_Tag (vc_block, "TITLE=", FileTag->title,
                      g_settings_get_boolean (MainSettings,
                                              "ogg-split-title"));

        /**********
         * Artist *
         **********/
        Flac_Set_Tag (vc_block, "ARTIST=", FileTag->artist,
                      g_settings_get_boolean (MainSettings,
                                              "ogg-split-artist"));

        /****************
         * Album Artist *
         ****************/
        Flac_Set_Tag (vc_block, "ALBUMARTIST=", FileTag->album_artist,
                      g_settings_get_boolean (MainSettings,
                                              "ogg-split-artist"));

        /*********
         * Album *
         *********/
        Flac_Set_Tag (vc_block, "ALBUM=", FileTag->album,
                      g_settings_get_boolean (MainSettings,
                                              "ogg-split-album"));

        /******************************
         * Disc Number and Disc Total *
         ******************************/
        Flac_Set_Tag (vc_block, "DISCNUMBER=", FileTag->disc_number, FALSE);
        Flac_Set_Tag (vc_block, "DISCTOTAL=", FileTag->disc_total, FALSE);

        /********
         * Year *
         ********/
        Flac_Set_Tag(vc_block,"DATE=",FileTag->year,FALSE);

        /*************************
         * Track and Total Track *
         *************************/
        Flac_Set_Tag(vc_block,"TRACKNUMBER=",FileTag->track,FALSE);
        Flac_Set_Tag(vc_block,"TRACKTOTAL=",FileTag->track_total,FALSE);

        /*********
         * Genre *
         *********/
        Flac_Set_Tag (vc_block, "GENRE=", FileTag->genre,
                      g_settings_get_boolean (MainSettings,
                                              "ogg-split-genre"));

        /***********
         * Comment *
         ***********/
        Flac_Set_Tag (vc_block, "DESCRIPTION=", FileTag->comment,
                      g_settings_get_boolean (MainSettings,
                                              "ogg-split-comment"));

        /************
         * Composer *
         ************/
        Flac_Set_Tag (vc_block, "COMPOSER=", FileTag->composer,
                      g_settings_get_boolean (MainSettings,
                                              "ogg-split-comment"));

        /*******************
         * Original artist *
         *******************/
        Flac_Set_Tag (vc_block, "PERFORMER=", FileTag->orig_artist,
                      g_settings_get_boolean (MainSettings,
                                              "ogg-split-original-artist"));

        /*************
         * Copyright *
         *************/
        Flac_Set_Tag(vc_block,"COPYRIGHT=",FileTag->copyright,FALSE);

        /*******
         * URL *
         *******/
        Flac_Set_Tag (vc_block, "CONTACT=", FileTag->url, FALSE);

        /**************
         * Encoded by *
         **************/
        Flac_Set_Tag(vc_block,"ENCODED-BY=",FileTag->encoded_by,FALSE);


        /**************************
         * Set unsupported fields *
         **************************/
        for (l = FileTag->other; l != NULL; l = g_list_next (l))
        {
            Flac_Set_Tag (vc_block, "", (gchar *)l->data, FALSE);
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
        Picture *pic = FileTag->picture;
        while (pic)
        {
            if (pic->data)
            {
                const gchar *violation;
                FLAC__StreamMetadata *picture_block; // For picture data
                Picture_Format format;
                
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
                FLAC__metadata_object_picture_set_data (picture_block,
                                                        (FLAC__byte *)pic->data, (FLAC__uint32) pic->size,
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
    
    // Free iter
    FLAC__metadata_iterator_delete(iter);
    
    
    //
    // Prepare for writing tag
    //
    
    // Move all PADDING blocks to the end on the metadata, and merge them into a single block.
    FLAC__metadata_chain_sort_padding(chain);
 
    /* Write tag. */
    if (!FLAC__metadata_chain_write (chain, /*padding*/TRUE,
                                     g_settings_get_boolean (MainSettings,
                                                             "file-preserve-modification-time")))
    {
        // Error with "FLAC__metadata_chain_write"
        FLAC__Metadata_ChainStatus status = FLAC__metadata_chain_status(chain);
        flac_error_msg = FLAC__Metadata_ChainStatusString[status];

        FLAC__metadata_chain_delete(chain);
        
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Failed to write comments to file ‘%s’: %s"),
                     filename_utf8, flac_error_msg);
        return FALSE;
    }
    
    FLAC__metadata_chain_delete(chain);

    
#ifdef ENABLE_MP3
    {
        // Delete the ID3 tags (create a dummy ETFile for the Id3tag_... function)
        ET_File   *ETFile_tmp    = ET_File_Item_New();
        File_Name *FileName_tmp  = ET_File_Name_Item_New();
        File_Tag  *FileTag_tmp   = ET_File_Tag_Item_New();
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
