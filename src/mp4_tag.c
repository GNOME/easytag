/* mp4_tag.c - 2005/08/06 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Portions of this code was borrowed from the MPEG4IP tools project */
#include <config.h> // For definition of ENABLE_MP4

#ifdef ENABLE_MP4

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "mp4_tag.h"
#include "picture.h"
#include "easytag.h"
#include "setting.h"
#include "log.h"
#include "misc.h"
#include "et_core.h"
#include "charset.h"

/* These undefs are because the mpeg4ip library contains a gnu config file in it's .h file */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#include <mp4v2/mp4v2.h>


/****************
 * Declarations *
 ****************/


/**************
 * Prototypes *
 **************/


/*************
 * Functions *
 *************/

/*
 * Mp4_Tag_Read_File_Tag:
 *
 * Read tag data into an Mp4 file.
 *
 * cf. http://mp4v2.googlecode.com/svn/doc/1.9.0/api/example_2itmf_2tags_8c-example.html
 *
 * Note:
 *  - for string fields, //if field is found but contains no info (strlen(str)==0), we don't read it
 *  - for track numbers, if they are zero, then we don't read it
 */
gboolean Mp4tag_Read_File_Tag (gchar *filename, File_Tag *FileTag)
{
    FILE   *file;
    MP4FileHandle mp4file = NULL;
    const MP4Tags *mp4tags = NULL;
    uint16_t track, track_total;
    uint16_t disk, disktotal;
    Picture *prev_pic = NULL;
    gint pic_num;
    const MP4TagArtwork *mp4artwork = NULL;
    
    if (!filename || !FileTag)
        return FALSE;

    if ( (file=fopen(filename,"r"))==NULL )
    {
        gchar *filename_utf8 = filename_to_display(filename);
        Log_Print(LOG_ERROR,_("ERROR while opening file: '%s' (%s)."),filename_utf8,g_strerror(errno));
        g_free(filename_utf8);
        return FALSE;
    }
    fclose(file); // We close it cause mp4 opens/closes file itself

    /* Get data from tag */
    mp4file = MP4Read(filename);
    if (mp4file == MP4_INVALID_FILE_HANDLE)
    {
        gchar *filename_utf8 = filename_to_display(filename);
        Log_Print(LOG_ERROR,_("ERROR while opening file: '%s' (%s)."),filename_utf8,_("MP4 format invalid"));
        g_free(filename_utf8);
        return FALSE;
    }

    mp4tags = MP4TagsAlloc();
    if (!MP4TagsFetch(mp4tags, mp4file))
    {
        gchar *filename_utf8 = filename_to_display(filename);
        Log_Print(LOG_ERROR,_("ERROR reading tags from file: '%s' (%s)."),filename_utf8,_("MP4 format invalid"));
        g_free(filename_utf8);
        return FALSE;
    }

    /* TODO Add error detection */

    /*********
     * Title *
     *********/
    if (mp4tags->name)
        FileTag->title = g_strdup(mp4tags->name);

    /**********
     * Artist *
     **********/
    if (mp4tags->artist)
        FileTag->artist = g_strdup(mp4tags->artist);

    /*********
     * Album *
     *********/
    if (mp4tags->album)
        FileTag->album = g_strdup(mp4tags->album);

    /****************
     * Album Artist *
     ****************/
    if (mp4tags->albumArtist)
        FileTag->album_artist = g_strdup(mp4tags->albumArtist);

    /**********************
     * Disk / Total Disks *
     **********************/
    if (mp4tags->disk)
    {
	disk = mp4tags->disk->index, disktotal = mp4tags->disk->total;
        if (disk != 0 && disktotal != 0)
            FileTag->disc_number = g_strdup_printf("%d/%d",(gint)disk,(gint)disktotal);
        else if (disk != 0)
            FileTag->disc_number = g_strdup_printf("%d",(gint)disk);
        else if (disktotal != 0)
            FileTag->disc_number = g_strdup_printf("/%d",(gint)disktotal);
        //if (disktotal != 0)
        //    FileTag->disk_number_total = g_strdup_printf("%d",(gint)disktotal);
    }

    /********
     * Year *
     ********/
    if (mp4tags->releaseDate)
        FileTag->year = g_strdup(mp4tags->releaseDate);

    /*************************
     * Track and Total Track *
     *************************/
    if (mp4tags->track)
    {

	track = mp4tags->track->index, track_total = mp4tags->track->total;
        if (track != 0)
            FileTag->track = NUMBER_TRACK_FORMATED ? g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,(gint)track) : g_strdup_printf("%d",(gint)track);
        if (track_total != 0)
            FileTag->track_total = NUMBER_TRACK_FORMATED ? g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,(gint)track_total) : g_strdup_printf("%d",(gint)track_total);
    }

    /*********
     * Genre *
     *********/
    if (mp4tags->genre)
        FileTag->genre = g_strdup(mp4tags->genre);

    /***********
     * Comment *
     ***********/
    if (mp4tags->comments)
        FileTag->comment = g_strdup(mp4tags->comments);

    /**********************
     * Composer or Writer *
     **********************/
    if (mp4tags->composer)
        FileTag->composer = g_strdup(mp4tags->composer);

    /*****************
     * Encoding Tool *
     *****************/
    if (mp4tags->encodedBy)
        FileTag->encoded_by = g_strdup(mp4tags->encodedBy);

    /* Unimplemented
    Tempo / BPM
    MP4GetMetadataTempo(file, &string)
    */

    /***********
     * Picture *
     ***********/
    // Version 1.9.1 of mp4v2 and up handle multiple cover art
    mp4artwork = mp4tags->artwork;
    for (pic_num = 0; pic_num < mp4tags->artworkCount; ++pic_num, ++mp4artwork)
    {
        Picture *pic;
        
        pic = Picture_Allocate();
        if (!prev_pic)
            FileTag->picture = pic;
        else
            prev_pic->next = pic;
        prev_pic = pic;

        pic->size = mp4artwork->size;
        pic->data = g_memdup(mp4artwork->data, pic->size);
        pic->type = PICTURE_TYPE_FRONT_COVER;/* mp4artwork->type gives image type */
        pic->description = NULL;
    }


    /* Free allocated data */
    MP4TagsFree(mp4tags);
    MP4Close(mp4file, 0);

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
    FILE     *file;
    MP4FileHandle mp4file = NULL;
    const MP4Tags *mp4tags = NULL;
    MP4TagDisk mp4disk;
    MP4TagTrack mp4track;
    MP4TagArtwork mp4artwork; 
    gint error = 0;

    /* extra initializers */
    mp4disk.index  = 0;
    mp4disk.total  = 0;
    mp4track.index = 0;
    mp4track.total = 0;

    if (!ETFile || !ETFile->FileTag)
        return FALSE;

    FileTag = (File_Tag *)ETFile->FileTag->data;
    filename      = ((File_Name *)ETFile->FileNameCur->data)->value;
    filename_utf8 = ((File_Name *)ETFile->FileNameCur->data)->value_utf8;

    /* Test to know if we can write into the file */
    if ( (file=fopen(filename,"r+"))==NULL )
    {
        Log_Print(LOG_ERROR,_("ERROR while opening file: '%s' (%s)."),filename_utf8,g_strerror(errno));
        return FALSE;
    }
    fclose(file);

    /* Open file for writing */
    mp4file = MP4Modify(filename, 0);
    if (mp4file == MP4_INVALID_FILE_HANDLE)
    {
        Log_Print(LOG_ERROR,_("ERROR while opening file: '%s' (%s)."),filename_utf8,_("MP4 format invalid"));
        return FALSE;
    }

    mp4tags = MP4TagsAlloc();
    if (!MP4TagsFetch(mp4tags, mp4file))
    {
        Log_Print(LOG_ERROR,_("ERROR reading tags from file: '%s' (%s)."),filename_utf8,_("MP4 format invalid"));
        return FALSE;
    }

    /*********
     * Title *
     *********/
    if (FileTag->title && g_utf8_strlen(FileTag->title, -1) > 0)
    {
        MP4TagsSetName(mp4tags, FileTag->title);
    }else
    {
        MP4TagsSetName(mp4tags, "");
    }

    /**********
     * Artist *
     **********/
    if (FileTag->artist && g_utf8_strlen(FileTag->artist, -1) > 0)
    {
        MP4TagsSetArtist(mp4tags, FileTag->artist);
    }else
    {
        MP4TagsSetArtist(mp4tags, "");
    }

    /*********
     * Album *
     *********/
    if (FileTag->album && g_utf8_strlen(FileTag->album, -1) > 0)
    {
        MP4TagsSetAlbum(mp4tags, FileTag->album);
    }else
    {
        MP4TagsSetAlbum(mp4tags, "");
    }

    /****************
     * Album Artist *
     ****************/
    if (FileTag->album_artist && g_utf8_strlen(FileTag->album_artist, -1) > 0)
    {
        MP4TagsSetAlbumArtist(mp4tags, FileTag->album_artist);
    }else
    {
        MP4TagsSetAlbumArtist(mp4tags, "");
    }

    /**********************
     * Disk / Total Disks *
     **********************/
    if (FileTag->disc_number && g_utf8_strlen(FileTag->disc_number, -1) > 0)
    //|| FileTag->disc_number_total && g_utf8_strlen(FileTag->disc_number_total, -1) > 0)
    {
        /* At the present time, we manage only disk number like '1' or '1/2', we
         * don't use disk number total... so here we try to decompose */
        if (FileTag->disc_number)
        {
            gchar *dn_tmp = g_strdup(FileTag->disc_number);
            gchar *tmp    = strchr(dn_tmp,'/');
            if (tmp)
            {
                // A disc_number_total was entered
                if ( (tmp+1) && atoi(tmp+1) )
                    mp4disk.total = atoi(tmp+1);

                // Fill disc_number
                *tmp = '\0';
                mp4disk.index = atoi(dn_tmp);
            }else
            {
                mp4disk.index = atoi(FileTag->disc_number);
            }
            g_free(dn_tmp);
        }
        /*if (FileTag->disc_number)
            mp4disk.index = atoi(FileTag->disc_number);
        if (FileTag->disc_number_total)
            mp4disk.total = atoi(FileTag->disc_number_total);
        */
    }
    MP4TagsSetDisk(mp4tags, &mp4disk);

    /********
     * Year *
     ********/
    if (FileTag->year && g_utf8_strlen(FileTag->year, -1) > 0)
    {
        MP4TagsSetReleaseDate(mp4tags, FileTag->year);
    }else
    {
        MP4TagsSetReleaseDate(mp4tags, "");
    }

    /*************************
     * Track and Total Track *
     *************************/
    if ( (FileTag->track       && g_utf8_strlen(FileTag->track, -1) > 0)
    ||   (FileTag->track_total && g_utf8_strlen(FileTag->track_total, -1) > 0) )
    {
        if (FileTag->track)
            mp4track.index = atoi(FileTag->track);
        if (FileTag->track_total)
            mp4track.total = atoi(FileTag->track_total);
    }
    MP4TagsSetTrack(mp4tags, &mp4track);

    /*********
     * Genre *
     *********/
    if (FileTag->genre && g_utf8_strlen(FileTag->genre, -1) > 0 )
    {
        MP4TagsSetGenre(mp4tags, FileTag->genre);
    }else
    {
        //MP4DeleteMetadataGenre(mp4tags);
        MP4TagsSetGenre(mp4tags, "");
    }

    /***********
     * Comment *
     ***********/
    if (FileTag->comment && g_utf8_strlen(FileTag->comment, -1) > 0)
    {
        MP4TagsSetComments(mp4tags, FileTag->comment);
    }else
    {
        MP4TagsSetComments(mp4tags, "");
    }

    /**********************
     * Composer or Writer *
     **********************/
    if (FileTag->composer && g_utf8_strlen(FileTag->composer, -1) > 0)
    {
        MP4TagsSetComposer(mp4tags, FileTag->composer);
    }else
    {
        MP4TagsSetComposer(mp4tags, "");
    }

    /*****************
     * Encoding Tool *
     *****************/
    if (FileTag->encoded_by && g_utf8_strlen(FileTag->encoded_by, -1) > 0)
    {
        MP4TagsSetEncodedBy(mp4tags, FileTag->encoded_by);
    }else
    {
        MP4TagsSetEncodedBy(mp4tags, "");
    }

    /***********
     * Picture *
     ***********/
    {
        // Can handle only one picture...
        Picture *pic;
        if (mp4tags->artworkCount && mp4tags->artwork)
            MP4TagsRemoveArtwork(mp4tags, 0);
        MP4TagsSetArtwork(mp4tags, 0, NULL);
        for (pic = FileTag->picture; pic; pic = pic->next)
        {
            if (pic->type == PICTURE_TYPE_FRONT_COVER)
            {
                 mp4artwork.data = pic->data;
                 mp4artwork.size = pic->size;
                 switch (pic->type) {
                  case PICTURE_FORMAT_JPEG:
                     mp4artwork.type = MP4_ART_JPEG;
                     break;
                  case PICTURE_FORMAT_PNG:
                     mp4artwork.type = MP4_ART_PNG;
                     break;
                  default:
                     mp4artwork.type = MP4_ART_UNDEFINED;
                 }
                 if (mp4tags->artworkCount)
                     MP4TagsSetArtwork(mp4tags, 0, &mp4artwork);
                 else
                     MP4TagsAddArtwork(mp4tags, &mp4artwork);
            }
        }
    }

    MP4TagsStore(mp4tags, mp4file);
    MP4TagsFree(mp4tags);
    MP4Close(mp4file, 0);

    if (error) return FALSE;
    else       return TRUE;
}


#endif /* ENABLE_MP4 */
