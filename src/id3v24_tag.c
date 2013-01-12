/* id3v24_tag.c - 2007/05/25 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2001-2003  Jerome Couderc <easytag@gmail.com>
 *  Copyright (C) 2006-2007  Alexey Illarionov <littlesavage@rambler.ru>
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
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#include "id3_tag.h"
#include "picture.h"
#include "easytag.h"
#include "browser.h"
#include "genres.h"
#include "setting.h"
#include "misc.h"
#include "log.h"
#include "et_core.h"
#include "charset.h"

#ifdef G_OS_WIN32
#include "win32/win32dep.h"
#endif /* G_OS_WIN32 */


#ifdef ENABLE_MP3

#include <id3tag.h>


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
static int etag_write_tags (const gchar *filename, struct id3_tag const *v1tag,
                            struct id3_tag const *v2tag, gboolean strip_tags);

/*************
 * Functions *
 *************/

/*
 * Read id3v1.x / id3v2 tag and load data into the File_Tag structure.
 * Returns TRUE on success, else FALSE.
 * If a tag entry exists (ex: title), we allocate memory, else value stays to NULL
 */
gboolean Id3tag_Read_File_Tag (gchar *filename, File_Tag *FileTag)
{
    int tmpfile;
    struct id3_file *file;
    struct id3_tag *tag;
    struct id3_frame *frame;
    union id3_field *field;
    gchar *string1, *string2;
    Picture *prev_pic = NULL;
    int i, j;
    unsigned tmpupdate, update = 0;
    long tagsize;


    if (!filename || !FileTag)
        return FALSE;

    if ( (tmpfile=open(filename,O_RDONLY)) < 0 )
    {
        gchar *filename_utf8 = filename_to_display(filename);
        g_print(_("Error while opening file: '%s' (%s).\n\a"),filename_utf8,g_strerror(errno));
        g_free(filename_utf8);
        return FALSE;
    }

    string1 = g_try_malloc(ID3_TAG_QUERYSIZE);
    if (string1==NULL)
    {
        close(tmpfile);
        return FALSE;
    }

    // Check if the file has an ID3v2 tag or/and an ID3v1 tags
    // 1) ID3v2 tag
    if (read(tmpfile, string1, ID3_TAG_QUERYSIZE) != ID3_TAG_QUERYSIZE)
    {
        close(tmpfile);
        return FALSE;
    }

    if ((tagsize = id3_tag_query((id3_byte_t const *)string1, ID3_TAG_QUERYSIZE)) <= ID3_TAG_QUERYSIZE)
    {
        // ID3v2 tag not found!
        update = FILE_WRITING_ID3V2_WRITE_TAG;
    }else
    {
        /* ID3v2 tag found */
        if (FILE_WRITING_ID3V2_WRITE_TAG == 0)
        {
            // To delete the tag
            update = 1;
        }else
        {
            /* Determine version if user want to upgrade old tags */
            if (CONVERT_OLD_ID3V2_TAG_VERSION
            && (string1 = realloc(string1, tagsize))
            && (read(tmpfile, &string1[ID3_TAG_QUERYSIZE], tagsize - ID3_TAG_QUERYSIZE) == tagsize - ID3_TAG_QUERYSIZE)
            && (tag = id3_tag_parse((id3_byte_t const *)string1, tagsize))
               )
            {
                unsigned version = id3_tag_version(tag);
#ifdef ENABLE_ID3LIB
                /* Besides upgrade old tags we will downgrade id3v2.4 to id3v2.3 */
                if ( FILE_WRITING_ID3V2_VERSION_4 )
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

    // 2) ID3v1 tag
    if ( (lseek(tmpfile,-128, SEEK_END) >= 0) // Go to the beginning of ID3v1 tag
    && (string1)
    && (read(tmpfile, string1, 3) == 3)
    && (string1[0] == 'T')
    && (string1[1] == 'A')
    && (string1[2] == 'G')
       )
    {
        // ID3v1 tag found!
        if (!FILE_WRITING_ID3V1_WRITE_TAG)
            update = 1;
    }else
    {
        // ID3v1 tag not found!
        if (FILE_WRITING_ID3V1_WRITE_TAG)
            update = 1;
    }

    g_free(string1);

    if ((file = id3_file_fdopen(tmpfile, ID3_FILE_MODE_READONLY)) == NULL)
    {
        close(tmpfile);
        return FALSE;
    }

    if ( ((tag = id3_file_tag(file)) == NULL)
    ||   (tag->nframes == 0))
    {
        id3_file_close(file);
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
    if ( (frame = id3_tag_findframe(tag,"TPOS", 0)) )
        update |= libid3tag_Get_Frame_Str(frame, ~0, &FileTag->disc_number);

    /********************
     * Year (TYER/TDRC) *
     ********************/
    if ( (frame = id3_tag_findframe(tag, ID3_FRAME_YEAR, 0)) )
    {
        update |= libid3tag_Get_Frame_Str(frame, ~0, &string1);
        if ( string1 )
        {
            Strip_String(string1);
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
            string2 = g_utf8_strchr(string1,-1,'/');

            if (NUMBER_TRACK_FORMATED)
            {
                if (string2)
                {
                    FileTag->track_total = g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,atoi(string2+1)); // Just to have numbers like this : '01', '05', '12', ...
                    *string2 = '\0'; // To cut string1
                }
                FileTag->track = g_strdup_printf("%.*d",NUMBER_TRACK_FORMATED_SPIN_BUTTON,atoi(string1)); // Just to have numbers like this : '01', '05', '12', ...
            }else
            {
                if (string2)
                {
                    FileTag->track_total = g_strdup(string2+1);
                    *string2 = '\0'; // To cut string1
                }
                FileTag->track = g_strdup(string1);
            }
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

            if ( (string1[0]=='(') && (tmp=strchr(string1,')')) && (tmp+1) && (strlen((tmp+1))>0) )
                /* Convert a genre written as '(3)EuroDance' into 'EuroDance' */
            {
                FileTag->genre = g_strdup(tmp+1);
            } else if ( (string1[0]=='(') && strchr(string1,')') )
            {
                /* Convert a genre written as '(3)' into 'Dance' */
                genre = strtol(string1+1, &tmp, 10);
                if (*tmp != ')')
                {
                    FileTag->genre = g_strdup(string1);
                }
            } else
            {
                genre = strtol(string1, &tmp, 10);
                if (tmp == string1)
                    FileTag->genre = g_strdup(string1);
            }

            if (!FileTag->genre
            && id3_genre_index(genre))
                FileTag->genre = (gchar *)id3_ucs4_utf8duplicate(id3_genre_index(genre));

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
            if (strncasecmp(string1, EASYTAG_STRING_ENCODEDBY MULTIFIELD_SEPARATOR, strlen(EASYTAG_STRING_ENCODEDBY MULTIFIELD_SEPARATOR)) == 0)
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
        Picture *pic;

        pic = Picture_Allocate();
        if (!prev_pic)
            FileTag->picture = pic;
        else
            prev_pic->next = pic;
        prev_pic = pic;

        pic->data = NULL;

        // Picture file data
        for (j = 0; (field = id3_frame_field(frame, j)); j++)
        {
            switch (id3_field_type(field))
            {
                case ID3_FIELD_TYPE_BINARYDATA:
                    {
                        id3_length_t size;
                        id3_byte_t const *data;
                        
                        data = id3_field_getbinarydata(field, &size);
                        if (pic->data)
                            g_free(pic->data);
                        if ( data && (pic->data = g_memdup(data, size)) )
                            pic->size = size;
                    }
                    break;
                case ID3_FIELD_TYPE_INT8:
                    pic->type = id3_field_getint(field);
                    break;
                default:
                    break;
            }
        }

        // Picture description
        update |= libid3tag_Get_Frame_Str(frame, EASYTAG_ID3_FIELD_STRING, &pic->description);
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
    const gchar *charset;

    if (!ustr || !*ustr)
    {
        if (ret)
            *ret = NULL;
        return 0;
    }

    if (USE_NON_STANDARD_ID3_READING_CHARACTER_SET)
        charset = FILE_READING_ID3V1V2_CHARACTER_SET;
    else if (!FILE_WRITING_ID3V2_USE_UNICODE_CHARACTER_SET) /* XXX */
        charset = FILE_WRITING_ID3V2_NO_UNICODE_CHARACTER_SET;
    else g_get_charset(&charset);

    if (!charset)
        charset = "ISO-8859-1";

    tmp = (gchar *)id3_ucs4_utf8duplicate(ustr);
    str = g_convert(tmp, -1, charset, "UTF-8", NULL, NULL, NULL);
    if (str)
    {
        g_free(str);
        if (ret)
            *ret = tmp;
        else
            free (tmp);
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
        return 1;
    }

    g_free(str);

    if (ret)
        *ret = tmp;
    else
        free(tmp);

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

    if (is_latin && USE_NON_STANDARD_ID3_READING_CHARACTER_SET)
    {
        if ((latinstr = (gchar *)id3_ucs4_latin1duplicate(usrc)))
        {
            retstr = convert_string(latinstr, FILE_READING_ID3V1V2_CHARACTER_SET, "UTF-8", FALSE);
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
libid3tag_Get_Frame_Str(const struct id3_frame *frame, unsigned etag_field_type, gchar **retstr)
{
    const union id3_field  *field;
    unsigned    i, j, strcnt;
    gchar   *ret, *tmpstr, *tmpstr2, *latinstr;
    unsigned field_type;
    const id3_ucs4_t *usrc;
    unsigned is_latin, is_utf16;
    unsigned retval;

    ret = NULL;
    retval = 0;
    is_latin = 1, is_utf16 = 0;

    // Find the encoding used for the field
    for (i = 0; (field = id3_frame_field(frame, i)); i++)
    {
        if (id3_field_type(field) == ID3_FIELD_TYPE_TEXTENCODING)
        {
            is_latin = (id3_field_gettextencoding(field) == ID3_FIELD_TEXTENCODING_ISO_8859_1);
            is_utf16 = (id3_field_gettextencoding(field) == ID3_FIELD_TEXTENCODING_UTF_16);
            break;
        }
    }

    for (i = 0; (field = id3_frame_field(frame, i)); i++)
    {
        tmpstr = tmpstr2 = NULL;
        switch (field_type = id3_field_type(field))
        {
            case ID3_FIELD_TYPE_LATIN1:
            case ID3_FIELD_TYPE_LATIN1FULL:
                if (field_type == ID3_FIELD_TYPE_LATIN1)
                {
                    if (!(etag_field_type & EASYTAG_ID3_FIELD_LATIN1))
                        continue;
                }else
                    if (!(etag_field_type & EASYTAG_ID3_FIELD_LATIN1FULL))
                        continue;
                latinstr = g_strdup(field_type == ID3_FIELD_TYPE_LATIN1 ? (gchar *)id3_field_getlatin1(field) : (gchar *)id3_field_getfulllatin1(field));
                if (USE_NON_STANDARD_ID3_READING_CHARACTER_SET)
                {
                    tmpstr = convert_string(latinstr, FILE_READING_ID3V1V2_CHARACTER_SET, "UTF-8", FALSE);
                    free(latinstr);
                }
                else
                    tmpstr = latinstr;
                break;

            case ID3_FIELD_TYPE_STRING:
            case ID3_FIELD_TYPE_STRINGFULL:
                if (field_type == ID3_FIELD_TYPE_STRING)
                {
                    if (!(etag_field_type & EASYTAG_ID3_FIELD_STRING))
                        continue;
                }else
                    if (!(etag_field_type & EASYTAG_ID3_FIELD_STRINGFULL))
                        continue;
                usrc = (field_type == ID3_FIELD_TYPE_STRING) ? id3_field_getstring(field) : id3_field_getfullstring(field);
                retval |= etag_ucs42gchar(usrc, is_latin, is_utf16, &tmpstr);
                break;

            case ID3_FIELD_TYPE_STRINGLIST:
                if (!(etag_field_type & EASYTAG_ID3_FIELD_STRINGLIST))
                    continue;
                strcnt = id3_field_getnstrings(field);
                for (j = 0; j < strcnt; j++)
                {
                    retval |= etag_ucs42gchar(
                        id3_field_getstrings(field, j),
                        is_latin, is_utf16, &tmpstr );

                    if (tmpstr2 && *tmpstr2 && g_utf8_validate(tmpstr2, -1, NULL))
                    {
                        if (tmpstr)
                            tmpstr = g_strconcat(tmpstr, " ", tmpstr2, NULL);
                        else
                            tmpstr = g_strdup(tmpstr2);
                    }

                    free(tmpstr2);
                }

            default:
                break;
        }
        if (tmpstr && *tmpstr && g_utf8_validate(tmpstr, -1, NULL))
        {
            if (ret)
                ret = g_strconcat(ret, MULTIFIELD_SEPARATOR, tmpstr, NULL);
            else
                ret = g_strdup(tmpstr);
        }
        g_free(tmpstr);
    }

    if (retstr)
        *retstr = ret;
    else
        free(ret);

    return retval;
}


/*
 * Write the ID3 tags to the file. Returns TRUE on success, else 0.
 */
gboolean Id3tag_Write_File_v24Tag (ET_File *ETFile)
{
    File_Tag         *FileTag;
    gchar            *filename, *filename_utf8;
    gchar            *basename_utf8;
    struct id3_tag   *v1tag, *v2tag;
    struct id3_frame *frame;
    union id3_field  *field;
    gchar            *string1;
    Picture          *pic;
    gint error = 0;
    gboolean strip_tags = TRUE;
    guchar genre_value = ID3_INVALID_GENRE;


    if (!ETFile && !ETFile->FileTag)
        return FALSE;

    FileTag       = (File_Tag *)ETFile->FileTag->data;
    filename      = ((File_Name *)ETFile->FileNameCur->data)->value;
    filename_utf8 = ((File_Name *)ETFile->FileNameCur->data)->value_utf8;

    v1tag = v2tag = NULL;

    // Write ID3v2 tag
    if (FILE_WRITING_ID3V2_WRITE_TAG)
    {
        struct id3_file *file;
        struct id3_tag *tmptag;
        unsigned tagsize;
        id3_byte_t *tmpbuf = NULL;

        /* Read old v2 tag */
        if ((file = id3_file_open(filename, ID3_FILE_MODE_READWRITE)) == NULL)
            return FALSE;

        if ((tmptag = id3_file_tag(file)) == NULL)
        {
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
        
        if (FILE_WRITING_ID3V2_USE_CRC32)
            id3_tag_options(v2tag, ID3_TAG_OPTION_CRC, ~0);
        if (FILE_WRITING_ID3V2_USE_COMPRESSION)
            id3_tag_options(v2tag, ID3_TAG_OPTION_COMPRESSION, ~0);
    }

    // Write ID3v1 tag
    if (FILE_WRITING_ID3V1_WRITE_TAG)
    {
        v1tag = id3_tag_new();
        if (!v1tag)
            return FALSE;
        
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
    etag_set_tags(FileTag->disc_number, "TPOS", ID3_FIELD_TYPE_STRINGLIST, NULL, v2tag, &strip_tags);

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

    if ((genre_value == ID3_INVALID_GENRE)||(FILE_WRITING_ID3V2_TEXT_ONLY_GENRE))
        string1 = g_strdup(FileTag->genre);
    else
        string1 = g_strdup_printf("(%d)",genre_value);

    etag_set_tags(string1, ID3_FRAME_GENRE, ID3_FIELD_TYPE_STRINGLIST, v1tag, v2tag, &strip_tags);
    g_free(string1);

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
        pic = FileTag->picture;
        while (pic)
        {
            gint i;

            if ((frame = id3_frame_new("APIC")) == NULL)
                continue;

            id3_tag_attachframe(v2tag, frame);
            for (i = 0; (field = id3_frame_field(frame, i)); i++)
            {
                Picture_Format format;
                
                switch (id3_field_type(field))
                {
                    case ID3_FIELD_TYPE_LATIN1:
                        format = Picture_Format_From_Data(pic);
                        id3_field_setlatin1(field, (id3_latin1_t const *)Picture_Mime_Type_String(format));
                        break;
                    case ID3_FIELD_TYPE_INT8:
                        id3_field_setint(field, pic->type);
                        break;
                    case ID3_FIELD_TYPE_BINARYDATA:
                        id3_field_setbinarydata(field, pic->data, pic->size);
                        break;
                    default:
                        break;
                }
            }

            if (pic->description)
                id3taglib_set_field(frame, pic->description, ID3_FIELD_TYPE_STRING, 0, 0, 0);

            strip_tags = FALSE;
            pic = pic->next;
        }
    }

    /****************************************
     * File length (in milliseconds) DISCARD*
     ****************************************/

    /*********************************
     * Update id3v1.x and id3v2 tags *
     *********************************/
    error |= etag_write_tags(filename, v1tag, v2tag, strip_tags);

    // Free data
    if (v1tag)
        id3_tag_delete(v1tag);
    if (v2tag)
        id3_tag_delete(v2tag);

    if (error == 0)
    {
        basename_utf8 = g_path_get_basename(filename_utf8);
        Log_Print(LOG_OK,_("Updated tag of '%s'"),basename_utf8);
        g_free(basename_utf8);
    }

    if (error) return FALSE;
    else       return TRUE;

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
            && (strncasecmp(str, param1, strlen(param1)) == 0) )
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

    if (!tag || !name || !*name)
        return NULL;

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
        /* Prepare str for writing according to easytag charset coversion settings */
        if ((FILE_WRITING_ID3V2_USE_UNICODE_CHARACTER_SET == 0)
        || (type == ID3_FIELD_TYPE_LATIN1)
        || (type == ID3_FIELD_TYPE_LATIN1FULL)
        || id3v1)
        {
            encname = NULL;
            /* id3v1 fields converted using its own character set and iconv options */
            if ( id3v1 )
            {
                if ( !FILE_WRITING_ID3V1_ICONV_OPTIONS_NO )
                    encname = g_strconcat(
                        FILE_WRITING_ID3V1_CHARACTER_SET,
                        FILE_WRITING_ID3V1_ICONV_OPTIONS_TRANSLIT ? "//TRANSLIT" : "//IGNORE",
                        NULL);
                else
                    encname = g_strdup(FILE_WRITING_ID3V1_CHARACTER_SET);
            } else
            {
                /* latin1 fields (such as URL) always converted with ISO-8859-1*/
                if ((type != ID3_FIELD_TYPE_LATIN1) && (type != ID3_FIELD_TYPE_LATIN1FULL))
                {
                    if ( FILE_WRITING_ID3V2_ICONV_OPTIONS_NO == 0)
                        encname = g_strconcat(
                            FILE_WRITING_ID3V2_NO_UNICODE_CHARACTER_SET,
                            FILE_WRITING_ID3V2_ICONV_OPTIONS_TRANSLIT ? "//TRANSLIT" : "//IGNORE",
                            NULL);
                    else
                        encname = g_strdup(FILE_WRITING_ID3V2_NO_UNICODE_CHARACTER_SET);
                }
            }

            latinstr = convert_string(str, "UTF-8", encname ? encname : "ISO-8859-1//IGNORE", TRUE);
            free(encname);
            buf = id3_latin1_ucs4duplicate((id3_latin1_t const *)latinstr);
        } else
        {
            if (!strcmp(FILE_WRITING_ID3V2_UNICODE_CHARACTER_SET, "UTF-16"))
            {
                enc_field = ID3_FIELD_TEXTENCODING_UTF_16;
                buf = id3_utf8_ucs4duplicate((id3_utf8_t const *)str);
            }else
            {
                enc_field = ID3_FIELD_TEXTENCODING_UTF_8;
                buf = id3_utf8_ucs4duplicate((id3_utf8_t const *)str);
            }
        }
    }

    if (frame)
        frame->flags &= ~ID3_FRAME_FLAG_FORMATFLAGS;

    for (i = 0; (field = id3_frame_field(frame, i)); i++)
    {
        if (is_set && !clear)
            break;

        switch (curtype = id3_field_type(field))
        {
            case ID3_FIELD_TYPE_TEXTENCODING:
                id3_field_settextencoding(field, enc_field);
                break;
            case ID3_FIELD_TYPE_LATIN1:
                if (clear)
                    id3_field_setlatin1(field, NULL);
                if ((type == curtype) && !is_set)
                {
                    if (num == 0)
                    {
                        id3_field_setlatin1(field, (id3_latin1_t const *)latinstr);
                        is_set = 1;
                    }else
                        num--;
                }
                break;
            case ID3_FIELD_TYPE_LATIN1FULL:
                if (clear)
                    id3_field_setfulllatin1(field, NULL);
                if ((type == curtype) && !is_set)
                {
                    if (num == 0)
                    {
                        id3_field_setfulllatin1(field, (id3_latin1_t const *)latinstr);
                        is_set = 1;
                    }else
                        num--;
                }
                break;
            case ID3_FIELD_TYPE_STRING:
                if (clear)
                    id3_field_setstring(field, NULL);
                if ((type == curtype) && !is_set)
                {
                    if (num == 0)
                    {
                        id3_field_setstring(field, buf);
                        is_set = 1;
                    }else
                        num--;
                }
                break;
            case ID3_FIELD_TYPE_STRINGFULL:
                if (clear)
                    id3_field_setfullstring(field, NULL);
                if ((type == curtype) && !is_set)
                {
                    if (num == 0)
                    {
                        id3_field_setfullstring(field, buf);
                        is_set = 1;
                    }else
                        num--;
                }
                break;
            case ID3_FIELD_TYPE_STRINGLIST:
                if (clear)
                    id3_field_setstrings(field, 0, NULL);
                if ((type == curtype) && !is_set)
                {
                    if ((num == 0) && (buf))
                    {
                        id3_field_addstring(field, buf);
                        is_set = 1;
                    }else
                        num--;
                }
                break;
            default:
                break;
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

static int
etag_write_tags (const gchar *filename, 
                 struct id3_tag const *v1tag,
                 struct id3_tag const *v2tag,
                 gboolean strip_tags)
{
    id3_byte_t *v1buf, *v2buf;
    id3_length_t v1size = 0, v2size = 0;
    char tmp[ID3_TAG_QUERYSIZE];
    int fd;
    int curpos;
    long filev2size, ctxsize;
    char *ctx = NULL;
    int err = 0;
    long size_read = 0;

    v1buf = v2buf = NULL;
    if ( !strip_tags )
    {
        /* Render v1 tag */
        if (v1tag)
        {
            v1size = id3_tag_render(v1tag, NULL);
            if (v1size == 128)
            {
                v1buf = g_try_malloc(v1size);
                if (id3_tag_render(v1tag, v1buf) != v1size)
                {
                    /* NOTREACHED */
                    g_free(v1buf);
                    v1buf = NULL;
                }
            }
        }

        /* Render v2 tag */
        if (v2tag)
        {
            v2size = id3_tag_render(v2tag, NULL);
            if (v2size > 10)
            {
                v2buf = g_try_malloc0(v2size);
                if ((v2size = id3_tag_render(v2tag, v2buf)) == 0)
                {
                    /* NOTREACHED */
                    g_free(v2buf);
                    v2buf = NULL;
                }
            }
        }
    }
    
    if (v1buf == NULL)
        v1size = 0;
    if (v2buf == NULL)
        v2size = 0;

    if ((fd = open(filename, O_RDWR)) < 0)
    {
        err = errno;
        g_free(v1buf);
        g_free(v2buf);
        return (err);
    }

    err = 1;

    /* Handle Id3v1 tag */
    if ((curpos = lseek(fd, -128, SEEK_END)) < 0)
        goto out;
    if ( (size_read = read(fd, tmp, ID3_TAG_QUERYSIZE)) != ID3_TAG_QUERYSIZE)
    {
        goto out;
    }

    if ( (tmp[0] == 'T')
    &&   (tmp[1] == 'A')
    &&   (tmp[2] == 'G')
       )
    {
        if ((curpos = lseek(fd, -128, SEEK_END)) < 0)
        {
            goto out;
        }
    }else
    {
        if ((curpos = lseek(fd, 0, SEEK_END)) < 0)
        {
            goto out;
        }
    }

    /* Search id3v2 tags at the end of the file (before any ID3v1 tag) */
    /* XXX: Unsafe */
    if ((curpos = lseek(fd, -ID3_TAG_QUERYSIZE, SEEK_CUR)) >= 0)
    {
        if (read(fd, tmp, ID3_TAG_QUERYSIZE) != ID3_TAG_QUERYSIZE)
            goto out;
        filev2size = id3_tag_query((id3_byte_t const *)tmp, ID3_TAG_QUERYSIZE);
        if ( (filev2size > 10)
        &&   (curpos = lseek(fd, -filev2size, SEEK_CUR)) )
        {
            if ( (size_read = read(fd, tmp, ID3_TAG_QUERYSIZE)) != ID3_TAG_QUERYSIZE)
            {
                goto out;
            }
            if (id3_tag_query((id3_byte_t const *)tmp, ID3_TAG_QUERYSIZE) != filev2size)
                curpos = lseek(fd, -ID3_TAG_QUERYSIZE - filev2size, SEEK_CUR);
            else
                curpos = lseek(fd, -ID3_TAG_QUERYSIZE, SEEK_CUR);
        }
    }

    /* Write id3v1 tag */
    if (v1buf)
    {
        if ( write(fd, v1buf, v1size) != v1size)
            goto out;
    }

    /* Truncate file (strip tags at the end of file) */
    if ((curpos = lseek(fd, 0, SEEK_CUR)) <= 0 )
        goto out;
    if ((err = ftruncate(fd, curpos)))
        goto out;

    /* Handle Id3v2 tag */
    if ((curpos = lseek(fd, 0, SEEK_SET)) < 0)
        goto out;

    if ( (size_read = read(fd, tmp, ID3_TAG_QUERYSIZE)) != ID3_TAG_QUERYSIZE)
    {
        goto out;
    }

    filev2size = id3_tag_query((id3_byte_t const *)tmp, ID3_TAG_QUERYSIZE);

    // No ID3v2 tag in the file, and no new tag
    if ( (filev2size == 0)
    &&   (v2size == 0) )
        goto out;

    if (filev2size == v2size)
    {
        if ((curpos = lseek(fd, 0, SEEK_SET)) < 0)
            goto out;
        if (write(fd, v2buf, v2size) != v2size)
            goto out;
    }else
    {
        /* XXX */
        // Size of audio data (tags at the end were already removed)
        ctxsize = lseek(fd, 0, SEEK_END) - filev2size;

        if ((ctx = g_try_malloc(ctxsize)) == NULL)
            goto out;
        if ((curpos = lseek(fd, filev2size, SEEK_SET)) < 0)
            goto out;
        if ((size_read = /*err = */read(fd, ctx, ctxsize)) != ctxsize)
        {
            gchar *filename_utf8 = filename_to_display(filename);
            gchar *basename_utf8 = g_path_get_basename(filename_utf8);

            Log_Print(LOG_ERROR,_("Cannot write tag of file '%s' (%d bytes were read but %d bytes were expected)"),basename_utf8,size_read,ctxsize);
            g_free(filename_utf8);
            g_free(basename_utf8);
            goto out;
        }
        
        // Return to the beginning of the file
        if (lseek(fd, 0, SEEK_SET) < 0)
            goto out;
            
        // Write the ID3v2 tag
        if (v2buf)
            write(fd, v2buf, v2size);
        
        // Write audio data
        if (write(fd, ctx, ctxsize) != ctxsize)
        {
            gchar *filename_utf8 = filename_to_display(filename);
            gchar *basename_utf8 = g_path_get_basename(filename_utf8);
            Log_Print(LOG_ERROR,_("Size error while saving tag of '%s'"),basename_utf8);
            g_free(filename_utf8);
            g_free(basename_utf8);
            goto out;
        }

        if ((curpos = lseek(fd, 0, SEEK_CUR)) <= 0 )
            goto out;

        if ((err = ftruncate(fd, curpos)))
            goto out;
    }

    err = 0;
out:
    g_free(ctx);
    lseek(fd, 0, SEEK_SET);
    close(fd);
    g_free(v1buf);
    g_free(v2buf);
    return err;
}

#endif /* ENABLE_MP3 */
