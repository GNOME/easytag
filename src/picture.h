/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014-2015  David King <amigadave@amigadave.com>
 * Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef ET_PICTURE_H_
#define ET_PICTURE_H_

#include <gio/gio.h>

G_BEGIN_DECLS

#include "core_types.h"

#define ET_TYPE_PICTURE (et_picture_get_type ())

typedef enum // Picture types
{
    // Same values for FLAC, see: https://xiph.org/flac/api/group__flac__format.html#gaf6d3e836cee023e0b8d897f1fdc9825d
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

/*
 * EtPicture:
 * @type: type of cover art
 * @description: string to describe the image, often a suitable filename
 * @width: original width, or 0 if unknown
 * @height: original height, or 0 if unknown
 * @bytes: image data
 * @next: next image data in the list, or %NULL
 */
typedef struct _EtPicture EtPicture;
struct _EtPicture
{
    EtPictureType type;
    gchar *description;
    gint width;
    gint height;
    GBytes *bytes;
    EtPicture *next;
};

typedef enum
{
    PICTURE_FORMAT_JPEG,
    PICTURE_FORMAT_PNG,
    PICTURE_FORMAT_GIF,
    PICTURE_FORMAT_UNKNOWN
} Picture_Format;

GType et_picture_get_type (void);
EtPicture * et_picture_new (EtPictureType type, const gchar *description, guint width, guint height, GBytes *bytes);
EtPicture * et_picture_copy_single (const EtPicture *pic);
EtPicture * et_picture_copy_all (const EtPicture *pic);
void et_picture_free (EtPicture *pic);
Picture_Format Picture_Format_From_Data (const EtPicture *pic);
const gchar   *Picture_Mime_Type_String (Picture_Format format);
const gchar * Picture_Type_String (EtPictureType type);
gboolean et_picture_detect_difference (const EtPicture *a, const EtPicture *b);
gchar * et_picture_format_info (const EtPicture *pic, ET_Tag_Type tag_type);

GBytes * et_picture_load_file_data (GFile *file, GError **error);
gboolean et_picture_save_file_data (const EtPicture *pic, GFile *file, GError **error);

EtPictureType et_picture_type_from_filename (const gchar *filename_utf8);

G_END_DECLS

#endif /* ET_PICTURE_H_ */
