/* ape_tag.c */
/*
 *  EasyTAG - Tag editor for MP3, Ogg Vorbis and MPC files
 *  Copyright (C) 2001-2003  Jerome Couderc <easytag@gmail.com>
 *  Copyright (C) 2002-2003  Artur Polaczyñski <artii@o2.pl>
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

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "easytag.h"
#include "ape_tag.h"
#include "et_core.h"
#include "log.h"
#include "misc.h"
#include "setting.h"
#include "charset.h"
#include "libapetag/apetaglib.h"

/*************
 * Functions *
 *************/

/*
 * Note:
 *  - if field is found but contains no info (strlen(str)==0), we don't read it
 */
gboolean Ape_Tag_Read_File_Tag (gchar *filename, File_Tag *FileTag)
{
    FILE *file;
    gchar *string = NULL;
    gchar *string1 = NULL;
    apetag *ape_cnt;

    if (!filename || !FileTag)
        return FALSE;

    if ((file = fopen(filename, "rb")) == NULL)
    {
        gchar *filename_utf8 = filename_to_display(filename);
        Log_Print(LOG_ERROR,_("Error while opening file: '%s' (%s)."),filename_utf8, g_strerror(errno));
        g_free(filename_utf8);
        return FALSE;
    }

    ape_cnt = apetag_init();
    apetag_read_fp(ape_cnt, file, filename, 0); /* read all tags ape,id3v[12]*/

    /*********
     * Title *
     *********/
    string = apefrm_getstr(ape_cnt, APE_TAG_FIELD_TITLE);
    if (FileTag->title == NULL)
        FileTag->title = Try_To_Validate_Utf8_String(string);


    /**********
     * Artist *
     **********/
    string = apefrm_getstr(ape_cnt, APE_TAG_FIELD_ARTIST);
    if (FileTag->artist == NULL)
        FileTag->artist = Try_To_Validate_Utf8_String(string);

    /*********
     * Album *
     *********/
    string = apefrm_getstr(ape_cnt, APE_TAG_FIELD_ALBUM);
    if (FileTag->album == NULL)
        FileTag->album = Try_To_Validate_Utf8_String(string);

    /***************
     * Disc Number *
     ***************/
    string = apefrm_getstr(ape_cnt, "Part");
    if (FileTag->disc_number == NULL)
        FileTag->disc_number = Try_To_Validate_Utf8_String(string);

    /********
     * Year *
     ********/
    string = apefrm_getstr(ape_cnt, APE_TAG_FIELD_YEAR);
    FileTag->year = Try_To_Validate_Utf8_String(string);

    /*************************
     * Track and Total Track *
     *************************/
    string = apefrm_getstr(ape_cnt, APE_TAG_FIELD_TRACK);
    if (string)
    {
        string = Try_To_Validate_Utf8_String(string);

        string1 = g_utf8_strchr(string, -1, '/');    // strchr don't like NULL string
        if (NUMBER_TRACK_FORMATED)
        {
            if (string1)
            {
                FileTag->track_total = g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON, atoi(string1 + 1));
                *string1 = '\0';
            }
            FileTag->track = g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON, atoi(string));
        } else
        {
            if (string1)
            {
                FileTag->track_total = g_strdup(string1 + 1);
                *string1 = '\0';
            }
            FileTag->track = g_strdup(string);
        }
        g_free(string);
    } else
    {
        FileTag->track = FileTag->track_total = NULL;
    }

    /*********
     * Genre *
     *********/
    string = apefrm_getstr(ape_cnt, APE_TAG_FIELD_GENRE);
    if (FileTag->genre == NULL)
        FileTag->genre = Try_To_Validate_Utf8_String(string);

    /***********
     * Comment *
     ***********/
    string = apefrm_getstr(ape_cnt, APE_TAG_FIELD_COMMENT);
    if (FileTag->comment == NULL)
        FileTag->comment = Try_To_Validate_Utf8_String(string);

    /************
     * Composer *
     ************/
    string = apefrm_getstr(ape_cnt, APE_TAG_FIELD_COMPOSER);
    if (FileTag->composer == NULL)
        FileTag->composer = Try_To_Validate_Utf8_String(string);

    /*******************
     * Original artist *
     *******************/
    string = apefrm_getstr(ape_cnt, "Original Artist");
    if (FileTag->orig_artist == NULL)
        FileTag->orig_artist = Try_To_Validate_Utf8_String(string);

    /*************
     * Copyright *
     *************/
    string = apefrm_getstr(ape_cnt, APE_TAG_FIELD_COPYRIGHT);
    if (FileTag->copyright == NULL)
        FileTag->copyright = Try_To_Validate_Utf8_String(string);

    /*******
     * URL *
     *******/
    string = apefrm_getstr(ape_cnt, APE_TAG_FIELD_RELATED_URL);
    if (FileTag->url == NULL)
        FileTag->url = Try_To_Validate_Utf8_String(string);

    /**************
     * Encoded by *
     **************/
    string = apefrm_getstr(ape_cnt, "Encoded By");
    if (FileTag->encoded_by == NULL)
        FileTag->encoded_by = Try_To_Validate_Utf8_String(string);

    apetag_free(ape_cnt);
    fclose(file);

    return TRUE;
}



gboolean Ape_Tag_Write_File_Tag (ET_File *ETFile)
{

    File_Tag *FileTag;
    gchar    *filename_in;
    //FILE     *file_in;
    gchar    *string;
    //GList    *list;
    apetag   *ape_mem;

    if (!ETFile || !ETFile->FileTag)
        return FALSE;

    FileTag     = (File_Tag *)ETFile->FileTag->data;
    filename_in = ((File_Name *)ETFile->FileNameCur->data)->value;

    ape_mem = apetag_init();

    /*********
     * Title *
     *********/
    if ( FileTag->title && g_utf8_strlen(FileTag->title, -1) > 0)
        apefrm_add(ape_mem, 0, APE_TAG_FIELD_TITLE, FileTag->title);
    else
        apefrm_remove(ape_mem,APE_TAG_FIELD_TITLE);


    /**********
     * Artist *
     **********/
    if ( FileTag->artist && g_utf8_strlen(FileTag->artist, -1) > 0)
        apefrm_add(ape_mem, 0, APE_TAG_FIELD_ARTIST, FileTag->artist);
    else
        apefrm_remove(ape_mem,APE_TAG_FIELD_ARTIST);

    /*********
     * Album *
     *********/
    if ( FileTag->album && g_utf8_strlen(FileTag->album, -1) > 0)
        apefrm_add(ape_mem, 0, APE_TAG_FIELD_ALBUM, FileTag->album);
    else
        apefrm_remove(ape_mem,APE_TAG_FIELD_ALBUM);

    /***************
     * Disc Number *
     ***************/
    if ( FileTag->disc_number && g_utf8_strlen(FileTag->disc_number, -1) > 0)
        apefrm_add(ape_mem, 0, "Part", FileTag->disc_number);
    else
        apefrm_remove(ape_mem,"Part");

    /********
     * Year *
     ********/
    if ( FileTag->year && g_utf8_strlen(FileTag->year, -1) > 0)
        apefrm_add(ape_mem, 0, APE_TAG_FIELD_YEAR, FileTag->year);
    else
        apefrm_remove(ape_mem,APE_TAG_FIELD_YEAR);

    /*************************
     * Track and Total Track *
     *************************/
    if ( FileTag->track && g_utf8_strlen(FileTag->track, -1) > 0)
    {
        if ( FileTag->track_total && g_utf8_strlen(FileTag->track_total, -1) > 0 )
            string = g_strconcat(FileTag->track,"/",FileTag->track_total,NULL);
        else
            string = g_strconcat(FileTag->track,NULL);
        apefrm_add(ape_mem, 0, APE_TAG_FIELD_TRACK, string);
        g_free(string);
    }else
        apefrm_remove(ape_mem,APE_TAG_FIELD_TRACK);

    /*********
     * Genre *
     *********/
    if ( FileTag->genre && g_utf8_strlen(FileTag->genre, -1) > 0)
        apefrm_add(ape_mem, 0, APE_TAG_FIELD_GENRE, FileTag->genre);
    else
        apefrm_remove(ape_mem,APE_TAG_FIELD_GENRE);

    /***********
     * Comment *
     ***********/
    if ( FileTag->comment && g_utf8_strlen(FileTag->comment, -1) > 0)
        apefrm_add (ape_mem, 0, APE_TAG_FIELD_COMMENT, FileTag->comment);
    else
        apefrm_remove(ape_mem,APE_TAG_FIELD_COMMENT);

    /************
     * Composer *
     ************/
    if ( FileTag->composer && g_utf8_strlen(FileTag->composer, -1) > 0)
        apefrm_add(ape_mem, 0, APE_TAG_FIELD_COMPOSER, FileTag->composer);
    else
        apefrm_remove(ape_mem,APE_TAG_FIELD_COMPOSER);

    /*******************
     * Original artist *
     *******************/
    if ( FileTag->orig_artist && g_utf8_strlen(FileTag->orig_artist, -1) > 0)
        apefrm_add(ape_mem, 0, "Original Artist", FileTag->orig_artist);
    else
        apefrm_remove(ape_mem,"Original Artist");

    /*************
     * Copyright *
     *************/
    if ( FileTag->copyright && g_utf8_strlen(FileTag->copyright, -1) > 0)
        apefrm_add(ape_mem, 0, APE_TAG_FIELD_COPYRIGHT, FileTag->copyright);
    else
        apefrm_remove(ape_mem,APE_TAG_FIELD_COPYRIGHT);

    /*******
     * URL *
     *******/
    if ( FileTag->url && g_utf8_strlen(FileTag->url, -1) > 0)
        apefrm_add(ape_mem, 0, APE_TAG_FIELD_RELATED_URL, FileTag->url);
    else
        apefrm_remove(ape_mem,APE_TAG_FIELD_RELATED_URL);

    /**************
     * Encoded by *
     **************/
    if ( FileTag->encoded_by && g_utf8_strlen(FileTag->encoded_by, -1) > 0)
        apefrm_add(ape_mem, 0, "Encoded By", FileTag->encoded_by);
    else
        apefrm_remove(ape_mem,"Encoded By");


    /* reread all tag-type again  excl. changed frames by apefrm_remove() */
    apetag_save(filename_in,ape_mem,APE_TAG_V2+SAVE_NEW_OLD_APE_TAG);
    apetag_free(ape_mem);

    return TRUE;
}
