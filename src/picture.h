/* picture.h - 2004/11/21 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef ET_PICTURE_H_
#define ET_PICTURE_H_

#include "et_core.h"

G_BEGIN_DECLS

/***************
 * Declaration *
 ***************/

/* Defined in et_core.h
typedef struct _Picture Picture;
struct _Picture
{
    gint     type;
    gchar   *description;
    gint     width;         // Original width of the picture
    gint     height;        // Original height of the picture
    gulong   size;          // Picture size in bits
    guchar  *data;
    Picture *next;
};*/

typedef enum // Picture types
{
    // Same values for FLAC, see: http://flac.sourceforge.net/api/group__flac__format.html#ga113
    ET_PICTURE_TYPE_OTHER = 0,
    ET_PICTURE_TYPE_FILE_ICON,
    ET_PICTURE_TYPE_OTHER_FILE_ICON,
    ET_PICTURE_TYPE_FRONT_COVER,
    ET_PICTURE_TYPE_BACK_COVER,
    ET_PICTURE_TYPE_LEAFLET_PAGE,
    ET_PICTURE_TYPE_MEDIA,
    ET_PICTURE_TYPE_LEAD_ARTIST_LEAD_PERFORMER_SOLOIST,
    ET_PICTURE_TYPE_ARTIST_PERFORMER,
    ET_PICTURE_TYPE_CONDUCTOR,
    ET_PICTURE_TYPE_BAND_ORCHESTRA,
    ET_PICTURE_TYPE_COMPOSER,
    ET_PICTURE_TYPE_LYRICIST_TEXT_WRITER,
    ET_PICTURE_TYPE_RECORDING_LOCATION,
    ET_PICTURE_TYPE_DURING_RECORDING,
    ET_PICTURE_TYPE_DURING_PERFORMANCE,
    ET_PICTURE_TYPE_MOVIE_VIDEO_SCREEN_CAPTURE,
    ET_PICTURE_TYPE_A_BRIGHT_COLOURED_FISH,
    ET_PICTURE_TYPE_ILLUSTRATION,
    ET_PICTURE_TYPE_BAND_ARTIST_LOGOTYPE,
    ET_PICTURE_TYPE_PUBLISHER_STUDIO_LOGOTYPE,
    
    ET_PICTURE_TYPE_UNDEFINED
} EtPictureType;

typedef enum
{
    PICTURE_FORMAT_JPEG,
    PICTURE_FORMAT_PNG,
    PICTURE_FORMAT_GIF,
    PICTURE_FORMAT_UNKNOWN
} Picture_Format;

/**************
 * Prototypes *
 **************/

Picture       *Picture_Allocate         (void);
Picture       *Picture_Copy_One         (const Picture *pic);
Picture       *Picture_Copy             (const Picture *pic);
void           Picture_Free             (Picture *pic);
Picture_Format Picture_Format_From_Data (const Picture *pic);
const gchar   *Picture_Mime_Type_String (Picture_Format format);
const gchar * Picture_Type_String (EtPictureType type);
gchar * Picture_Info (const Picture *pic, ET_Tag_Type tag_type);

Picture *et_picture_load_file_data (GFile *file, GError **error);
gboolean et_picture_save_file_data (const Picture *pic, GFile *file, GError **error);

EtPictureType et_picture_type_from_filename (const gchar *filename_utf8);

G_END_DECLS

#endif /* ET_PICTURE_H_ */
