/* ogg_tag.c - 2001/11/08 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2001-2003  Jerome Couderc <easytag@gmail.com>
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

#include <config.h> // For definition of ENABLE_OGG

#ifdef ENABLE_OGG

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
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
#include "base64.h"
#include "picture.h"
#include "setting.h"
#include "charset.h"

#ifdef WIN32
// for mkstemp
#   include "win32/win32dep.h"
#endif


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
gboolean Ogg_Tag_Write_File (FILE *file_in, gchar *filename_in, vcedit_state *state);

static gboolean Ogg_Write_Delimetered_Tag (vorbis_comment *vc, const gchar *tag_name, gchar *values);
static gboolean Ogg_Write_Tag (vorbis_comment *vc, const gchar *tag_name, gchar *value);
static void Ogg_Set_Tag (vorbis_comment *vc, const gchar *tag_name, gchar *value, gboolean split);

/*************
 * Functions *
 *************/

/*
 * Read tag data into an Ogg Vorbis file.
 * Note:
 *  - if field is found but contains no info (strlen(str)==0), we don't read it
 */
gboolean Ogg_Tag_Read_File_Tag (gchar *filename, File_Tag *FileTag)
{
    FILE           *file;
    vcedit_state   *state;
    vorbis_comment *vc;
    gchar          *string = NULL;
    gchar          *string1 = NULL;
    gchar          *string2 = NULL;
    gchar          *filename_utf8 = filename_to_display(filename);
    guint           field_num, i;
    Picture        *prev_pic = NULL;


    if (!filename || !FileTag)
        return FALSE;

    ogg_error_msg = NULL;

    if ( (file=fopen(filename,"rb")) == NULL )
    {
        Log_Print(LOG_ERROR,_("Error while opening file: '%s' (%s)."),filename_utf8,g_strerror(errno));
        g_free(filename_utf8);
        return FALSE;
    }

    {
    // Skip the id3v2 tag
    guchar tmp_id3[4];
    gulong id3v2size;

    // Check if there is an ID3v2 tag...
    fseek(file, 0L, SEEK_SET);
    if (fread(tmp_id3, 1, 4, file) == 4)
    {
        // Calculate ID3v2 length
        if (tmp_id3[0] == 'I' && tmp_id3[1] == 'D' && tmp_id3[2] == '3' && tmp_id3[3] < 0xFF)
        {
            // id3v2 tag skipeer $49 44 33 yy yy xx zz zz zz zz [zz size]
            fseek(file, 2, SEEK_CUR); // Size is 6-9 position
            if (fread(tmp_id3, 1, 4, file) == 4)
            {
                id3v2size = 10 + ( (long)(tmp_id3[3])        | ((long)(tmp_id3[2]) << 7)
                                | ((long)(tmp_id3[1]) << 14) | ((long)(tmp_id3[0]) << 21) );
                fseek(file, id3v2size, SEEK_SET);
                Log_Print(LOG_ERROR,_("Warning: The Ogg Vorbis file '%s' contains an ID3v2 tag."),filename_utf8);
            }else
            {
                fseek(file, 0L, SEEK_SET);
            }
        }else
        {
            fseek(file, 0L, SEEK_SET);
        }
    }else
    {
        fseek(file, 0L, SEEK_SET);
    }
    }

    state = vcedit_new_state();    // Allocate memory for 'state'
    if ( vcedit_open(state,file) < 0 )
    {
        Log_Print(LOG_ERROR,_("Error: Failed to open file: '%s' as Vorbis (%s)."),filename_utf8,vcedit_error(state));
        ogg_error_msg = vcedit_error(state);
        fclose(file);
        g_free(filename_utf8);
        vcedit_clear(state);
        return FALSE;
    }


    /* Get data from tag */
    vc = vcedit_comments(state);
    /*{
        gint i; 
        for (i=0;i<vc->comments;i++) 
            g_print("%s -> Ogg vc:'%s'\n",g_path_get_basename(filename),vc->user_comments[i]);
    }*/

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

    /*******************************
     * Disc Number (Part of a Set) *
     *******************************/
    if ( (string = vorbis_comment_query(vc,"DISCNUMBER",0)) != NULL && g_utf8_strlen(string, -1) > 0 )
    {
        FileTag->disc_number = g_strdup(string);
    }

    /********
     * Year *
     ********/
    if ( (string = vorbis_comment_query(vc,"DATE",0)) != NULL && g_utf8_strlen(string, -1) > 0 )
    {
        FileTag->year = g_strdup(string);
        if (g_utf8_strlen(FileTag->year, -1) > 4)
            Log_Print(LOG_WARNING,_("The year value '%s' seems to be invalid in file '%s'. The information will be lost while saving tag."),FileTag->year,filename_utf8);
    }

    /*************************
     * Track and Total Track *
     *************************/
    if ( (string = vorbis_comment_query(vc,"TRACKNUMBER",0)) != NULL && g_utf8_strlen(string, -1) > 0 )
    {
        if (NUMBER_TRACK_FORMATED)
        {
            // Ckeck if TRACKTOTAL used, else takes it in TRACKNUMBER
            if ( (string1 = vorbis_comment_query(vc,"TRACKTOTAL",0)) != NULL && g_utf8_strlen(string1, -1) > 0 )
            {
                FileTag->track_total = g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,atoi(string1));
            }else
            if ( (string1 = g_utf8_strchr(string, -1, '/')) )
            {
                FileTag->track_total = g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,atoi(string1+1));
                *string1 = '\0';
            }
            FileTag->track = g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,atoi(string));
        }else
        {
            // Ckeck if TRACKTOTAL used, else takes it in TRACKNUMBER
            if ( (string1 = vorbis_comment_query(vc,"TRACKTOTAL",0)) != NULL && g_utf8_strlen(string1, -1) > 0 )
            {
                FileTag->track_total = g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,atoi(string1));
            }else
            if ( (string1 = g_utf8_strchr(string, -1, '/')) )
            {
                FileTag->track_total = g_strdup(string1+1);
                *string1 = '\0';
            }
            FileTag->track = g_strdup(string);
        }
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
    while ( (string = vorbis_comment_query(vc,"LICENSE",field_num++)) != NULL )
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
        gchar *data;
        gint size;
        Picture *pic;
            
        pic = Picture_Allocate();
        if (!prev_pic)
            FileTag->picture = pic;
        else
            prev_pic->next = pic;
        prev_pic = pic;

        pic->data = NULL;
        
        // Decode picture data
        data = g_strdup(string);
        size = base64_decode(string, data);
        if ( data && (pic->data = g_memdup(data, size)) )
            pic->size = size;
        g_free(data);

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
          && strncasecmp(vc->user_comments[i],"LICENSE=",          8) != 0
          && strncasecmp(vc->user_comments[i],"ENCODED-BY=",      11) != 0
          && strncasecmp(vc->user_comments[i],"COVERART=",         9) != 0
          && strncasecmp(vc->user_comments[i],"COVERARTTYPE=",       13) != 0
          && strncasecmp(vc->user_comments[i],"COVERARTMIME=",       13) != 0
          && strncasecmp(vc->user_comments[i],"COVERARTDESCRIPTION=",20) != 0
           )
        {
            FileTag->other = g_list_append(FileTag->other,
                                           Try_To_Validate_Utf8_String(vc->user_comments[i]));
        }
    }

    vcedit_clear(state);
    fclose(file);
    g_free(filename_utf8);

    return TRUE;
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

gboolean Ogg_Tag_Write_File_Tag (ET_File *ETFile)
{
    File_Tag       *FileTag;
    gchar          *filename;
    gchar          *filename_utf8;
    gchar          *basename_utf8;
    FILE           *file_in;
    vcedit_state   *state;
    vorbis_comment *vc;
    gchar          *string;
    GList          *list;
    Picture        *pic;

    if (!ETFile || !ETFile->FileTag)
        return FALSE;

    FileTag       = (File_Tag *)ETFile->FileTag->data;
    filename      = ((File_Name *)ETFile->FileNameCur->data)->value;
    filename_utf8 = ((File_Name *)ETFile->FileNameCur->data)->value_utf8;
    ogg_error_msg = NULL;

    /* Test to know if we can write into the file */
    if ( (file_in=fopen(filename,"rb"))==NULL )
    {
        Log_Print(LOG_ERROR,_("Error while opening file: '%s' (%s)."),filename_utf8,g_strerror(errno));
        return FALSE;
    }

    {
    // Skip the id3v2 tag
    guchar tmp_id3[4];
    gulong id3v2size;

    // Check if there is an ID3v2 tag...
    fseek(file_in, 0L, SEEK_SET);
    if (fread(tmp_id3, 1, 4, file_in) == 4)
    {
        // Calculate ID3v2 length
        if (tmp_id3[0] == 'I' && tmp_id3[1] == 'D' && tmp_id3[2] == '3' && tmp_id3[3] < 0xFF)
        {
            // id3v2 tag skipeer $49 44 33 yy yy xx zz zz zz zz [zz size]
            fseek(file_in, 2, SEEK_CUR); // Size is 6-9 position
            if (fread(tmp_id3, 1, 4, file_in) == 4)
            {
                id3v2size = 10 + ( (long)(tmp_id3[3])        | ((long)(tmp_id3[2]) << 7)
                                | ((long)(tmp_id3[1]) << 14) | ((long)(tmp_id3[0]) << 21) );
                fseek(file_in, id3v2size, SEEK_SET);
            }else
            {
                fseek(file_in, 0L, SEEK_SET);
            }
        }else
        {
            fseek(file_in, 0L, SEEK_SET);
        }
    }else
    {
        fseek(file_in, 0L, SEEK_SET);
    }
    }

    state = vcedit_new_state();    // Allocate memory for 'state'
    if ( vcedit_open(state,file_in) < 0 )
    {
        Log_Print(LOG_ERROR,_("Error: Failed to open file: '%s' as Vorbis (%s)."),filename_utf8,vcedit_error(state));
        ogg_error_msg = vcedit_error(state);
        fclose(file_in);
        vcedit_clear(state);
        return FALSE;
    }

    /* Get data from tag */
    vc = vcedit_comments(state);
    vorbis_comment_clear(vc);
    vorbis_comment_init(vc);

    /*********
     * Title *
     *********/
    Ogg_Set_Tag(vc,"TITLE=",FileTag->title,VORBIS_SPLIT_FIELD_TITLE);

    /**********
     * Artist *
     **********/
    Ogg_Set_Tag(vc,"ARTIST=",FileTag->artist, VORBIS_SPLIT_FIELD_ARTIST);

    /****************
     * Album Artist *
     ****************/
    Ogg_Set_Tag(vc,"ALBUMARTIST=",FileTag->album_artist, VORBIS_SPLIT_FIELD_ARTIST);

    /*********
     * Album *
     *********/
    Ogg_Set_Tag(vc,"ALBUM=",FileTag->album, VORBIS_SPLIT_FIELD_ALBUM);

    /***************
     * Disc Number *
     ***************/
    Ogg_Set_Tag(vc,"DISCNUMBER=",FileTag->disc_number,FALSE);

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
    Ogg_Set_Tag(vc,"GENRE=",FileTag->genre,VORBIS_SPLIT_FIELD_GENRE);

    /***********
     * Comment *
     ***********/
    // We write the comment using the two formats "DESCRIPTION" and "COMMENT" to be compatible with old versions
    // Format of new specification
    Ogg_Set_Tag(vc,"DESCRIPTION=",FileTag->comment,VORBIS_SPLIT_FIELD_COMMENT);

    // Format used in winamp plugin
    Ogg_Set_Tag(vc,"COMMENT=",FileTag->comment,VORBIS_SPLIT_FIELD_COMMENT);

    if (OGG_TAG_WRITE_XMMS_COMMENT)
    {
        // Format used into xmms-1.2.5
        Ogg_Set_Tag(vc,"=",FileTag->comment,VORBIS_SPLIT_FIELD_COMMENT);
    }


    /************
     * Composer *
     ************/
    Ogg_Set_Tag(vc,"COMPOSER=",FileTag->composer,VORBIS_SPLIT_FIELD_COMPOSER);

    /*******************
     * Original artist *
     *******************/
    Ogg_Set_Tag(vc,"PERFORMER=",FileTag->orig_artist,VORBIS_SPLIT_FIELD_ORIG_ARTIST);

    /*************
     * Copyright *
     *************/
    Ogg_Set_Tag(vc,"COPYRIGHT=",FileTag->copyright,FALSE);

    /*******
     * URL *
     *******/
    Ogg_Set_Tag(vc,"LICENSE=",FileTag->url,FALSE);

    /**************
     * Encoded by *
     **************/
    Ogg_Set_Tag(vc,"ENCODED-BY=",FileTag->encoded_by,FALSE);
    
    
    /***********
     * Picture *
     ***********/
    pic = FileTag->picture;
    while (pic)
    {
        if (pic->data)
        {
            gchar *data_encoded = NULL;
            gint size;
            Picture_Format format = Picture_Format_From_Data(pic);

            string = g_strdup_printf("COVERARTMIME=%s",Picture_Mime_Type_String(format));
            vorbis_comment_add(vc,string);
            g_free(string);
        
            if (pic->type)
            {
                string = g_strdup_printf("COVERARTTYPE=%d",pic->type);
                vorbis_comment_add(vc,string);
                g_free(string);
            }

            if (pic->description)
            {
                string = g_strdup_printf("COVERARTDESCRIPTION=%s",pic->description);
                vorbis_comment_add(vc,string);
                g_free(string);
            }

            size = base64_encode(pic->data, pic->size, &data_encoded);
            string = g_strdup_printf("COVERART=%s",data_encoded);
            vorbis_comment_add(vc,string);
            g_free(data_encoded);
            g_free(string);
        }

        pic = pic->next;
    }

    /**************************
     * Set unsupported fields *
     **************************/
    list = FileTag->other;
    while (list)
    {
        if (list->data)
            vorbis_comment_add(vc,(gchar *)list->data);
        list = list->next;
    }

    /* Write tag, and close also 'file_in' in all cases */
    if ( Ogg_Tag_Write_File(file_in,filename,state) == FALSE )
    {
        ogg_error_msg = vcedit_error(state);
        Log_Print(LOG_ERROR,_("Error: Failed to write comments to file '%s' (%s)."),filename_utf8,ogg_error_msg == NULL ? "" : ogg_error_msg);

        vcedit_clear(state);
        return FALSE;
    }else
    {
        basename_utf8 = g_path_get_basename(filename_utf8);
        Log_Print(LOG_OK,_("Written tag of '%s'"),basename_utf8);

        vcedit_clear(state);
    }

    return TRUE;
}



/* 
 * Write tag information to a new temporary file, and rename it the the initial name.
 */
gboolean Ogg_Tag_Write_File (FILE *file_in, gchar *filename_in, vcedit_state *state)
{
    gchar *filename_out;
    gint   file_mkstemp;
    FILE  *file_out;
    gboolean return_code = TRUE;


    filename_out = g_strdup_printf("%s.XXXXXX",filename_in);
    
    // Function mkstemp() opens also the file!
    if ((file_mkstemp = mkstemp(filename_out)) < 0)
    {
        fclose(file_in);
        close(file_mkstemp);
        g_free(filename_out);
        return FALSE;
    }
    close(file_mkstemp);
    
    if ( (file_out=fopen(filename_out,"wb")) == NULL )
    {
        fclose(file_in);
        remove(filename_out);
        g_free(filename_out);
        return FALSE;
    }
    
    if (vcedit_write(state,file_out) < 0)
    {
        fclose(file_in);
        fclose(file_out);
        remove(filename_out);
        g_free(filename_out);
        return FALSE;
    }
    fclose(file_in);
    fclose(file_out);

    // Some platforms fail to rename a file if the new name already
    // exists, so we need to remove, then rename...
    if (rename(filename_out,filename_in))
    {
        // Can't rename directly
        if (remove(filename_in))
        {
            // Can't remove file
            remove(filename_out);
            return_code = FALSE;

        }else if (rename(filename_out,filename_in)) 
        {
            // Can't rename after removing file
            remove(filename_out);
            return_code = FALSE;
        }
    }

    g_free(filename_out);

    return return_code;
}


#endif /* ENABLE_OGG */
