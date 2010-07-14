/* flac_tag.c - 2003/12/27 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2001-2003  Jerome Couderc <easytag@gmail.com>
 *  Copyright (C) 2003       Pavel Minayev <thalion@front.ru>
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

#ifdef ENABLE_FLAC

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
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

/* Patch from Josh Coalson
 * FLAC 1.1.3 has FLAC_API_VERSION_CURRENT == 8 *
 * by LEGACY_FLAC we mean pre-FLAC 1.1.3; in FLAC 1.1.3 the FLAC__FileDecoder was merged into the FLAC__StreamDecoder */
#if !defined(FLAC_API_VERSION_CURRENT) || FLAC_API_VERSION_CURRENT < 8
#define LEGACY_FLAC // For FLAC version < 1.1.3
#else
#undef LEGACY_FLAC
#endif


/* FLAC uses Ogg Vorbis comments
 * Ogg Vorbis fields names :
 *  - TITLE        : Track name
 *  - VERSION      : The version field may be used to differentiate multiple version of the same track title in a single collection. (e.g. remix info)
 *  - ALBUM        : The collection name to which this track belongs
 *  - TRACKNUMBER  : The track number of this piece if part of a specific larger collection or album
 *  - TRACKTOTAL   :
 *  - ARTIST       : Track performer
 *  - ORGANIZATION : Name of the organization producing the track (i.e. the 'record label')
 *  - DESCRIPTION  : A short text description of the contents
 *  - COMME?T      : same than DESCRIPTION
 *  - GENRE        : A short text indication of music genre
 *  - DATE         : Date the track was recorded
 *  - LOCATION     : Location where track was recorded
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
gboolean Flac_Tag_Write_File (FILE *file_in, gchar *filename_in, vcedit_state *state);


/*************
 * Functions *
 *************/

/*
 * Read tag data from a FLAC file.
 * Note:
 *  - if field is found but contains no info (strlen(str)==0), we don't read it
 */
gboolean Flac_Tag_Read_File_Tag (gchar *filename, File_Tag *FileTag)
{
    FLAC__Metadata_SimpleIterator               *iter;
    FLAC__StreamMetadata                        *vc_block;
    FLAC__StreamMetadata_VorbisComment          *vc;
    FLAC__StreamMetadata_VorbisComment_Entry    *field;
    gchar                                       *field_value;
    gchar                                       *field_value_tmp;
    gchar                                       *string = NULL;
    gint                                        field_num;
    gint                                        field_len;
    guint                                       i;


    if (!filename || !FileTag)
        return FALSE;

    flac_error_msg = NULL;

    iter = FLAC__metadata_simple_iterator_new();
    if ( iter == NULL || !FLAC__metadata_simple_iterator_init(iter, filename, true, false) )
    {
        gchar *filename_utf8 = filename_to_display(filename);
        if ( iter == NULL )
        {
#ifdef WIN32
            const char ** const iter = FLAC__Metadata_SimpleIteratorStatusString; /* this is for win32 auto-import of this external symbol works */
            flac_error_msg = iter[FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR];
#else
            flac_error_msg = FLAC__Metadata_SimpleIteratorStatusString[FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR];
#endif
        }else
        {
            flac_error_msg = FLAC__Metadata_SimpleIteratorStatusString[FLAC__metadata_simple_iterator_status(iter)];
            FLAC__metadata_simple_iterator_delete(iter);
        }

        Log_Print(_("ERROR while opening file: '%s' as FLAC (%s)."),filename_utf8,flac_error_msg);
        g_free(filename_utf8);
        return FALSE;
    }

    /* libFLAC is able to detect (and skip) ID3v2 tags by itself */

    /* Find the VORBIS_COMMENT block */
    while ( FLAC__metadata_simple_iterator_get_block_type(iter) != FLAC__METADATA_TYPE_VORBIS_COMMENT )
    {
        if ( !FLAC__metadata_simple_iterator_next(iter) )
        {
            /* End of metadata: comment block not found, nothing to read */
            FLAC__metadata_simple_iterator_delete(iter);
            return TRUE;
        }
    }

    /* Get comments from block */
    vc_block = FLAC__metadata_simple_iterator_get_block(iter);
    vc = &vc_block->data.vorbis_comment;

    /* Get vendor string */
    /*{
        FLAC__StreamMetadata_VorbisComment_Entry vce;
        vce = vc->vendor_string;
        g_print("File %s : \n",filename);
        g_print(" - FLAC File vendor string : '%s'\n",g_strndup(vce.entry,vce.length));
        g_print(" - FLAC Lib vendor string  : '%s'\n",FLAC__VENDOR_STRING);
    }*/


    /*********
     * Title *
     *********/
    field_num = 0;
    while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(vc_block,field_num,"TITLE")) >= 0 )
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
    while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(vc_block,field_num,"ARTIST")) >= 0 )
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

    /*********
     * Album *
     *********/
    field_num = 0;
    while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(vc_block,field_num,"ALBUM")) >= 0 )
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

    /***************
     * Disc Number *
     ***************/
    if ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(vc_block,0,"DISCNUMBER")) >= 0 )
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
                FileTag->disc_number = field_value;
            }
        }
    }

    /********
     * Year *
     ********/
    if ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(vc_block,0,"DATE")) >= 0 )
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
    if ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(vc_block,0,"TRACKTOTAL")) >= 0 )
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
                if (NUMBER_TRACK_FORMATED)
                {
                    FileTag->track_total = g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,atoi(field_value));
                }else
                {
                    FileTag->track_total = g_strdup(field_value);
                }
                g_free(field_value);
            }
        }
        // Below is also filled track_total if not done here
    }

    if ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(vc_block,0,"TRACKNUMBER")) >= 0 )
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
                if (NUMBER_TRACK_FORMATED)
                {
                    // If track_total not filled before, try now...
                    if (string && !FileTag->track_total)
                    {
                        FileTag->track_total = g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,atoi(string+1));
                        *string = '\0';
                    }
                    FileTag->track = g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,atoi(field_value));
                }else
                {
                    if (string && !FileTag->track_total)
                    {
                        FileTag->track_total = g_strdup(string+1);
                        *string = '\0';
                    }
                    FileTag->track = g_strdup(field_value);
                }
                g_free(field_value);
            }
        }
    }

    /*********
     * Genre *
     *********/
    field_num = 0;
    while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(vc_block,field_num,"GENRE")) >= 0 )
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
        field_num1 = FLAC__metadata_object_vorbiscomment_find_entry_from(vc_block,field_num,"DESCRIPTION");
        field_num2 = FLAC__metadata_object_vorbiscomment_find_entry_from(vc_block,field_num,"COMMENT");

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
    while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(vc_block,field_num,"COMPOSER")) >= 0 )
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
    while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(vc_block,field_num,"PERFORMER")) >= 0 )
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
    while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(vc_block,field_num,"COPYRIGHT")) >= 0 )
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
    while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(vc_block,field_num,"LICENSE")) >= 0 )
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
    while ( (field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(vc_block,field_num,"ENCODED-BY")) >= 0 )
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


    /***********
     * Picture *
     ***********/
    // For FLAC > 1.1.3
    #ifndef LEGACY_FLAC
    
    #endif


    /***************************
     * Save unsupported fields *
     ***************************/
    for (i=0;i<(guint)vc->num_comments;i++)
    {
        field = &vc->comments[i];
        if ( strncasecmp((gchar *)field->entry,"TITLE=",       MIN(6,  field->length)) != 0
          && strncasecmp((gchar *)field->entry,"ARTIST=",      MIN(7,  field->length)) != 0
          && strncasecmp((gchar *)field->entry,"ALBUM=",       MIN(6,  field->length)) != 0
          && strncasecmp((gchar *)field->entry,"DISCNUMBER=",  MIN(11, field->length)) != 0
          && strncasecmp((gchar *)field->entry,"DATE=",        MIN(5,  field->length)) != 0
          && strncasecmp((gchar *)field->entry,"TRACKNUMBER=", MIN(12, field->length)) != 0
          && strncasecmp((gchar *)field->entry,"TRACKTOTAL=",  MIN(11, field->length)) != 0
          && strncasecmp((gchar *)field->entry,"GENRE=",       MIN(6,  field->length)) != 0
          && strncasecmp((gchar *)field->entry,"DESCRIPTION=", MIN(12, field->length)) != 0
          && strncasecmp((gchar *)field->entry,"COMMENT=",     MIN(8,  field->length)) != 0
          && strncasecmp((gchar *)field->entry,"COMPOSER=",    MIN(9,  field->length)) != 0
          && strncasecmp((gchar *)field->entry,"PERFORMER=",   MIN(10, field->length)) != 0
          && strncasecmp((gchar *)field->entry,"COPYRIGHT=",   MIN(10, field->length)) != 0
          && strncasecmp((gchar *)field->entry,"LICENSE=",     MIN(8,  field->length)) != 0
          && strncasecmp((gchar *)field->entry,"ENCODED-BY=",  MIN(11, field->length)) != 0 )
        {
            //g_print("custom %*s\n", field->length, field->entry);
            FileTag->other = g_list_append(FileTag->other,g_strndup((const gchar *)field->entry, field->length));
        }
    }

    FLAC__metadata_object_delete(vc_block);
    FLAC__metadata_simple_iterator_delete(iter);

#ifdef ENABLE_MP3
    /* If no FLAC vorbis tag found : we try to get the ID3 tag if it exists
     * (will be deleted when writing the tag */
    if ( FileTag->title       == NULL
      && FileTag->artist      == NULL
      && FileTag->album       == NULL
      && FileTag->disc_number == NULL
      && FileTag->year        == NULL
      && FileTag->track       == NULL
      && FileTag->track_total == NULL
      && FileTag->genre       == NULL
      && FileTag->comment     == NULL
      && FileTag->composer    == NULL
      && FileTag->orig_artist == NULL
      && FileTag->copyright   == NULL
      && FileTag->url         == NULL
      && FileTag->encoded_by  == NULL)
    {
        gint rc = Id3tag_Read_File_Tag(filename,FileTag);

        // If an ID3 tag has been found (and no FLAC tag), we mark the file as
        // unsaved to rewrite a flac tag
        if ( FileTag->title       != NULL
          || FileTag->artist      != NULL
          || FileTag->album       != NULL
          || FileTag->disc_number != NULL
          || FileTag->year        != NULL
          || FileTag->track       != NULL
          || FileTag->track_total != NULL
          || FileTag->genre       != NULL
          || FileTag->comment     != NULL
          || FileTag->composer    != NULL
          || FileTag->orig_artist != NULL
          || FileTag->copyright   != NULL
          || FileTag->url         != NULL
          || FileTag->encoded_by  != NULL)
        {
            FileTag->saved = FALSE;
        }

        return rc;
    }

    /* Part to get cover artist :
     * If we have read the ID3 tag previously we don't arrive here (and we have
     * the picture if it exists)
     * Else the ID3 tag wasn't read (as there was data in FLAC tag) so we try
     * to read it only to get the picture (not supported by the FLAC tag) */
    if (WRITE_ID3_TAGS_IN_FLAC_FILE && FileTag->picture == NULL)
    {
        File_Tag *FileTag_tmp = ET_File_Tag_Item_New();
        gint rc = Id3tag_Read_File_Tag(filename,FileTag_tmp);
        if (rc && FileTag_tmp->picture)
        {
            // Copy picture to FileTag
            FileTag->picture = Picture_Copy(FileTag_tmp->picture);
        }

        ET_Free_File_Tag_Item(FileTag_tmp);

        return rc;
    }
#endif

    return TRUE;
}


gboolean Flac_Tag_Write_File_Tag (ET_File *ETFile)
{
    File_Tag                                 *FileTag;
    gchar                                    *filename_utf8, *filename;
    gchar                                    *basename_utf8;
    FLAC__Metadata_SimpleIterator            *iter;
    FLAC__StreamMetadata                     *vc_block;
    FLAC__StreamMetadata_VorbisComment_Entry field;
    FLAC__bool                               write_ok;
    gchar                                    *string;
    GList                                    *list;
    // To get original vendor string
    FLAC__StreamMetadata                     *vc_block_svg = NULL;
    FLAC__StreamMetadata_VorbisComment       *vc_svg;
    FLAC__StreamMetadata_VorbisComment_Entry field_svg;
    gboolean                                 vc_found_svg = TRUE;


    if (!ETFile || !ETFile->FileTag)
        return FALSE;

    FileTag       = (File_Tag *)ETFile->FileTag->data;
    filename      = ((File_Name *)ETFile->FileNameCur->data)->value;
    filename_utf8 = ((File_Name *)ETFile->FileNameCur->data)->value_utf8;
    flac_error_msg = NULL;

    /* libFLAC is able to detect (and skip) ID3v2 tags by itself */

    iter = FLAC__metadata_simple_iterator_new();
    if ( iter == NULL || !FLAC__metadata_simple_iterator_init(iter,filename,false,false) )
    {
        if ( iter == NULL )
        {
#ifdef WIN32
            const char **iter = FLAC__Metadata_SimpleIteratorStatusString;  /* this is for win32 auto-import of this external symbol works */
            flac_error_msg = iter[FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR];
#else
            flac_error_msg = FLAC__Metadata_SimpleIteratorStatusString[FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR];
#endif
        }else
        {
            flac_error_msg = FLAC__Metadata_SimpleIteratorStatusString[FLAC__metadata_simple_iterator_status(iter)];
            FLAC__metadata_simple_iterator_delete(iter);
        }

        Log_Print(_("ERROR while opening file: '%s' as FLAC (%s)."),filename_utf8,flac_error_msg);
        return FALSE;
    }

    /* Find the VORBIS_COMMENT block to get original vendor string */
    while ( FLAC__metadata_simple_iterator_get_block_type(iter) != FLAC__METADATA_TYPE_VORBIS_COMMENT )
    {
        if ( !FLAC__metadata_simple_iterator_next(iter) )
        {
            /* End of metadata: comment block not found, nothing to read */
            vc_found_svg = FALSE;
            break;
        }
    }
    if (vc_found_svg)
    {
        /* Get comments from block */
        vc_block_svg = FLAC__metadata_simple_iterator_get_block(iter);
        vc_svg = &vc_block_svg->data.vorbis_comment;
        /* Get original vendor string */
        field_svg = vc_svg->vendor_string;
    }


    vc_block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);

    /* Set the original vendor string else it is version of library */
    if (vc_found_svg)
        FLAC__metadata_object_vorbiscomment_set_vendor_string(vc_block, field_svg, true);


    /*********
     * Title *
     *********/
    if ( FileTag->title )
    {
        string  = g_strconcat("TITLE=",FileTag->title,NULL);
        field.entry = (FLAC__byte *)string;
        field.length = strlen(string); // Warning : g_utf8_strlen doesn't count the multibyte characters. Here we need the allocated size.
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        g_free(string);
    }

    /**********
     * Artist *
     **********/
    if ( FileTag->artist )
    {
        string  = g_strconcat("ARTIST=",FileTag->artist,NULL);
        field.entry = (FLAC__byte *)string;
        field.length = strlen(string);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        g_free(string);
    }

    /*********
     * Album *
     *********/
    if ( FileTag->album )
    {
        string  = g_strconcat("ALBUM=",FileTag->album,NULL);
        field.entry = (FLAC__byte *)string;
        field.length = strlen(string);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        g_free(string);
    }

    /***************
     * Disc Number *
     ***************/
    if ( FileTag->disc_number )
    {
        string  = g_strconcat("DISCNUMBER=",FileTag->disc_number,NULL);
        field.entry = (FLAC__byte *)string;
        field.length = strlen(string);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        g_free(string);
    }

    /********
     * Year *
     ********/
    if ( FileTag->year )
    {
        string  = g_strconcat("DATE=",FileTag->year,NULL);
        field.entry = (FLAC__byte *)string;
        field.length = strlen(string);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        g_free(string);
    }

    /*************************
     * Track and Total Track *
     *************************/
    if ( FileTag->track )
    {
        string = g_strconcat("TRACKNUMBER=",FileTag->track,NULL);
        field.entry = (FLAC__byte *)string;
        field.length = strlen(string);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        g_free(string);
    }
    if ( FileTag->track_total /*&& strlen(FileTag->track_total)>0*/ )
    {
        string = g_strconcat("TRACKTOTAL=",FileTag->track_total,NULL);
        field.entry = (FLAC__byte *)string;
        field.length = strlen(string);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        g_free(string);
    }

    /*********
     * Genre *
     *********/
    if ( FileTag->genre )
    {
        string  = g_strconcat("GENRE=",FileTag->genre,NULL);
        field.entry = (FLAC__byte *)string;
        field.length = strlen(string);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        g_free(string);
    }

    /***********
     * Comment *
     ***********/
    // We write the comment using the "both" format
    if ( FileTag->comment )
    {
        string = g_strconcat("DESCRIPTION=",FileTag->comment,NULL);
        field.entry = (FLAC__byte *)string;
        field.length = strlen(string);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        g_free(string);

        string = g_strconcat("COMMENT=",FileTag->comment,NULL);
        field.entry = (FLAC__byte *)string;
        field.length = strlen(string);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        g_free(string);
    }

    /************
     * Composer *
     ************/
    if ( FileTag->composer )
    {
        string  = g_strconcat("COMPOSER=",FileTag->composer,NULL);
        field.entry = (FLAC__byte *)string;
        field.length = strlen(string);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        g_free(string);
    }

    /*******************
     * Original artist *
     *******************/
    if ( FileTag->orig_artist )
    {
        string  = g_strconcat("PERFORMER=",FileTag->orig_artist,NULL);
        field.entry = (FLAC__byte *)string;
        field.length = strlen(string);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        g_free(string);
    }

    /*************
     * Copyright *
     *************/
    if ( FileTag->copyright )
    {
        string  = g_strconcat("COPYRIGHT=",FileTag->copyright,NULL);
        field.entry = (FLAC__byte *)string;
        field.length = strlen(string);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        g_free(string);
    }

    /*******
     * URL *
     *******/
    if ( FileTag->url )
    {
        string  = g_strconcat("LICENSE=",FileTag->url,NULL);
        field.entry = (FLAC__byte *)string;
        field.length = strlen(string);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        g_free(string);
    }

    /**************
     * Encoded by *
     **************/
    if ( FileTag->encoded_by )
    {
        string  = g_strconcat("ENCODED-BY=",FileTag->encoded_by,NULL);
        field.entry = (FLAC__byte *)string;
        field.length = strlen(string);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        g_free(string);
    }


    /**************************
     * Set unsupported fields *
     **************************/
    list = FileTag->other;
    while (list)
    {
        if (list->data)
        {
            string = (gchar*)list->data;
            field.entry = (FLAC__byte *)string;
            field.length = strlen(string);
            FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        }
        list = list->next;
    }

    /* Find the VORBIS_COMMENT block */
    while ( FLAC__metadata_simple_iterator_get_block_type(iter) != FLAC__METADATA_TYPE_VORBIS_COMMENT )
    {
        if ( !FLAC__metadata_simple_iterator_next(iter) )
            break;
    }


    /*
     * Write FLAC tag (as Vorbis comment)
     */
    if ( FLAC__metadata_simple_iterator_get_block_type(iter) != FLAC__METADATA_TYPE_VORBIS_COMMENT )
    {
        /* End of metadata: no comment block, so insert one */
        write_ok = FLAC__metadata_simple_iterator_insert_block_after(iter,vc_block,true);
    }else
    {
        write_ok = FLAC__metadata_simple_iterator_set_block(iter,vc_block,true);
    }

    if ( !write_ok )
    {
        flac_error_msg = FLAC__Metadata_SimpleIteratorStatusString[FLAC__metadata_simple_iterator_status(iter)];
        Log_Print(_("ERROR: Failed to write comments to file '%s' (%s)."),filename_utf8,flac_error_msg);
        FLAC__metadata_simple_iterator_delete(iter);
        FLAC__metadata_object_delete(vc_block);
        return FALSE;
    }else
    {
        basename_utf8 = g_path_get_basename(filename_utf8);
        Log_Print(_("Written tag of '%s'"),basename_utf8);
        g_free(basename_utf8);
    }

    FLAC__metadata_simple_iterator_delete(iter);
    FLAC__metadata_object_delete(vc_block);
    if (vc_found_svg)
        FLAC__metadata_object_delete(vc_block_svg);

#ifdef ENABLE_MP3
    /*
     * Write also the ID3 tags (ID3v1 and/or ID3v2) if wanted (as needed by some players)
     */
    if (WRITE_ID3_TAGS_IN_FLAC_FILE)
    {
        Id3tag_Write_File_Tag(ETFile);
    }else
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
        Id3tag_Write_File_Tag(ETFile_tmp);
        ET_Free_File_List_Item(ETFile_tmp);
    }
#endif

    return TRUE;
}


#endif /* ENABLE_FLAC */
