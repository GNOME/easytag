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
#include <mp4.h>


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
 * Note:
 *  - for string fields, //if field is found but contains no info (strlen(str)==0), we don't read it
 *  - for track numbers, if they are zero, then we don't read it
 */
gboolean Mp4tag_Read_File_Tag (gchar *filename, File_Tag *FileTag)
{
    FILE   *file;
    MP4FileHandle mp4file = NULL;
    uint16_t track, track_total;
    uint16_t disk, disktotal;
    u_int8_t *coverArt;
    u_int32_t coverSize;
    Picture *prev_pic = NULL;
#ifdef NEWMP4
    gint pic_num;
#endif
    
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
    mp4file = MP4Read(filename, 0);
    if (mp4file == MP4_INVALID_FILE_HANDLE)
    {
        gchar *filename_utf8 = filename_to_display(filename);
        Log_Print(LOG_ERROR,_("ERROR while opening file: '%s' (%s)."),filename_utf8,_("MP4 format invalid"));
        g_free(filename_utf8);
        return FALSE;
    }

    /* TODO Add error detection */

    /*********
     * Title *
     *********/
    MP4GetMetadataName(mp4file, &FileTag->title);

    /**********
     * Artist *
     **********/
    MP4GetMetadataArtist(mp4file, &FileTag->artist);

    /*********
     * Album *
     *********/
    MP4GetMetadataAlbum(mp4file, &FileTag->album);

    /**********************
     * Disk / Total Disks *
     **********************/
    if (MP4GetMetadataDisk(mp4file, &disk, &disktotal))
    {
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
    MP4GetMetadataYear(mp4file, &FileTag->year);

    /*************************
     * Track and Total Track *
     *************************/
    if (MP4GetMetadataTrack(mp4file, &track, &track_total))
    {
        if (track != 0)
            FileTag->track = g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,(gint)track); // Just to have numbers like this : '01', '05', '12', ...
        if (track_total != 0)
            FileTag->track_total = g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,(gint)track_total); // Just to have numbers like this : '01', '05', '12', ...
    }

    /*********
     * Genre *
     *********/
    MP4GetMetadataGenre(mp4file, &FileTag->genre);

    /***********
     * Comment *
     ***********/
    MP4GetMetadataComment(mp4file, &FileTag->comment);

    /**********************
     * Composer or Writer *
     **********************/
    MP4GetMetadataWriter(mp4file, &FileTag->composer);

    /*****************
     * Encoding Tool *
     *****************/
    MP4GetMetadataTool(mp4file, &FileTag->encoded_by);

    /* Unimplemented
    Tempo / BPM
    MP4GetMetadataTempo(file, &string)
    */

    /***********
     * Picture *
     ***********/
#ifdef NEWMP4
    // There version can handle multiple pictures!
    // Version 1.6 of libmp4v2 introduces an index argument for MP4GetMetadataCoverart
    for (pic_num = 0; (MP4GetMetadataCoverArt( mp4file, &coverArt, &coverSize,pic_num )); pic_num++)
#else
    // There version handle only one picture!
    if ( MP4GetMetadataCoverArt( mp4file, &coverArt, &coverSize ) )
#endif
    {
        Picture *pic;
        
        pic = Picture_Allocate();
        if (!prev_pic)
            FileTag->picture = pic;
        else
            prev_pic->next = pic;
        prev_pic = pic;

        pic->size = coverSize;
        pic->data = coverArt;
        pic->type = PICTURE_TYPE_FRONT_COVER;
        pic->description = NULL;
    }


    /* Free allocated data */
    MP4Close(mp4file);

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
    gint error = 0;

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
    mp4file = MP4Modify(filename,0,0);
    if (mp4file == MP4_INVALID_FILE_HANDLE)
    {
        Log_Print(LOG_ERROR,_("ERROR while opening file: '%s' (%s)."),filename_utf8,_("MP4 format invalid"));
        return FALSE;
    }

    /*********
     * Title *
     *********/
    if (FileTag->title && g_utf8_strlen(FileTag->title, -1) > 0)
    {
        MP4SetMetadataName(mp4file, FileTag->title);
    }else
    {
        //MP4DeleteMetadataName(mp4file); // Not available on mpeg4ip-1.2 (only in 1.3)
        MP4SetMetadataName(mp4file, "");
    }

    /**********
     * Artist *
     **********/
    if (FileTag->artist && g_utf8_strlen(FileTag->artist, -1) > 0)
    {
        MP4SetMetadataArtist(mp4file, FileTag->artist);
    }else
    {
        //MP4DeleteMetadataArtist(mp4file);
        MP4SetMetadataArtist(mp4file, "");
    }

    /*********
     * Album *
     *********/
    if (FileTag->album && g_utf8_strlen(FileTag->album, -1) > 0)
    {
        MP4SetMetadataAlbum(mp4file, FileTag->album);
    }else
    {
        //MP4DeleteMetadataAlbum(mp4file);
        MP4SetMetadataAlbum(mp4file, "");
    }

    /**********************
     * Disk / Total Disks *
     **********************/
    if (FileTag->disc_number && g_utf8_strlen(FileTag->disc_number, -1) > 0)
    //|| FileTag->disc_number_total && g_utf8_strlen(FileTag->disc_number_total, -1) > 0)
    {
        uint16_t disk      = 0;
        uint16_t disktotal = 0;

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
                    disktotal = atoi(tmp+1);

                // Fill disc_number
                *tmp = '\0';
                disk = atoi(dn_tmp);
            }else
            {
                disk = atoi(FileTag->disc_number);
            }
            g_free(dn_tmp);
        }
        /*if (FileTag->disc_number)
            disk = atoi(FileTag->disc_number);
        if (FileTag->disc_number_total)
            disktotal = atoi(FileTag->disc_number_total);
        */
        MP4SetMetadataDisk(mp4file, disk, disktotal);
    }else
    {
        //MP4DeleteMetadataDisk(mp4file);
        MP4SetMetadataDisk(mp4file, 0, 0);
    }

    /********
     * Year *
     ********/
    if (FileTag->year && g_utf8_strlen(FileTag->year, -1) > 0)
    {
        MP4SetMetadataYear(mp4file, FileTag->year);
    }else
    {
        //MP4DeleteMetadataYear(mp4file);
        MP4SetMetadataYear(mp4file, "");
    }

    /*************************
     * Track and Total Track *
     *************************/
    if ( (FileTag->track       && g_utf8_strlen(FileTag->track, -1) > 0)
    ||   (FileTag->track_total && g_utf8_strlen(FileTag->track_total, -1) > 0) )
    {
        uint16_t track       = 0;
        uint16_t track_total = 0;
        if (FileTag->track)
            track = atoi(FileTag->track);
        if (FileTag->track_total)
            track_total = atoi(FileTag->track_total);
        MP4SetMetadataTrack(mp4file, track, track_total);
    }else
    {
        //MP4DeleteMetadataTrack(mp4file);
        MP4SetMetadataTrack(mp4file, 0, 0);
    }

    /*********
     * Genre *
     *********/
    if (FileTag->genre && g_utf8_strlen(FileTag->genre, -1) > 0 )
    {
        MP4SetMetadataGenre(mp4file, FileTag->genre);
    }else
    {
        //MP4DeleteMetadataGenre(mp4file);
        MP4SetMetadataGenre(mp4file, "");
    }

    /***********
     * Comment *
     ***********/
    if (FileTag->comment && g_utf8_strlen(FileTag->comment, -1) > 0)
    {
        MP4SetMetadataComment(mp4file, FileTag->comment);
    }else
    {
        //MP4DeleteMetadataComment(mp4file);
        MP4SetMetadataComment(mp4file, "");
    }

    /**********************
     * Composer or Writer *
     **********************/
    if (FileTag->composer && g_utf8_strlen(FileTag->composer, -1) > 0)
    {
        MP4SetMetadataWriter(mp4file, FileTag->composer);
    }else
    {
        //MP4DeleteMetadataWriter(mp4file);
        MP4SetMetadataWriter(mp4file, "");
    }

    /*****************
     * Encoding Tool *
     *****************/
    if (FileTag->encoded_by && g_utf8_strlen(FileTag->encoded_by, -1) > 0)
    {
        MP4SetMetadataTool(mp4file, FileTag->encoded_by);
    }else
    {
        //MP4DeleteMetadataTool(mp4file);
        MP4SetMetadataTool(mp4file, "");
    }

    /***********
     * Picture *
     ***********/
    {
        // Can handle only one picture...
        Picture *pic;

        //MP4DeleteMetadataCoverArt(mp4file);
        MP4SetMetadataCoverArt(mp4file, NULL, 0);
        for( pic = FileTag->picture; pic; pic = pic->next )
        {
            if( pic->type == PICTURE_TYPE_FRONT_COVER )
            {
                MP4SetMetadataCoverArt(mp4file, pic->data, pic->size);
            }
        }
    }


    MP4Close(mp4file);

    if (error) return FALSE;
    else       return TRUE;
}


#endif /* ENABLE_MP4 */
