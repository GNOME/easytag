/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014,2015  David King <amigadave@amigadave.com>
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

#ifndef ET_FILE_TAG_H_
#define ET_FILE_TAG_H_

#include <glib.h>

G_BEGIN_DECLS

#include "picture.h"

/*
 * File_Tag:
 * @key: incremented value
 * @saved: %TRUE if the tag has been saved, %FALSE otherwise
 * @title: track name
 * @artist: track artist
 * @album_artist: artist for the album of which this track is part
 * @album: album name
 * @disc_number: disc number within a set (as a string)
 * @disc_total: total number of discs in the set (as a string)
 * @year: year (as a string)
 * @track: track number (as a string)
 * @track_total: total number of tracks (as a string)
 * @genre: text genre
 * @comment: comment
 * @composer: composer
 * @orig_artist: original artist
 * @copyright: copyright
 * @url: URL
 * @encoded_by: encoded by (strictly, a person, but often the encoding
 *              application)
 * @picture: #EtPicture, which may have several other linked instances
 * @other: a list of other tags, used for Vorbis comments
 * Description of each item of the TagList list
 */
typedef struct
{
    guint key;
    gboolean saved;

    gchar *title;
    gchar *artist;
    gchar *album_artist;
    gchar *album;
    gchar *disc_number;
    gchar *disc_total;
    gchar *year;
    gchar *track;
    gchar *track_total;
    gchar *genre;
    gchar *comment;
    gchar *composer;
    gchar *orig_artist;
    gchar *copyright;
    gchar *url;
    gchar *encoded_by;
    EtPicture *picture;
    GList *other;
} File_Tag;

File_Tag * et_file_tag_new (void);
void et_file_tag_free (File_Tag *file_tag);

void et_file_tag_set_title (File_Tag *file_tag, const gchar *title);
void et_file_tag_set_artist (File_Tag *file_tag, const gchar *artist);
void et_file_tag_set_album_artist (File_Tag *file_tag, const gchar *album_artist);
void et_file_tag_set_album (File_Tag *file_tag, const gchar *album);
void et_file_tag_set_disc_number (File_Tag *file_tag, const gchar *disc_number);
void et_file_tag_set_disc_total (File_Tag *file_tag, const gchar *disc_total);
void et_file_tag_set_year (File_Tag *file_tag, const gchar *year);
void et_file_tag_set_track_number (File_Tag *file_tag, const gchar *track_number);
void et_file_tag_set_track_total (File_Tag *file_tag, const gchar *track_total);
void et_file_tag_set_genre (File_Tag *file_tag, const gchar *genre);
void et_file_tag_set_comment (File_Tag *file_tag, const gchar *comment);
void et_file_tag_set_composer (File_Tag *file_tag, const gchar *composer);
void et_file_tag_set_orig_artist (File_Tag *file_tag, const gchar *orig_artist);
void et_file_tag_set_copyright (File_Tag *file_tag, const gchar *copyright);
void et_file_tag_set_url (File_Tag *file_tag, const gchar *url);
void et_file_tag_set_encoded_by (File_Tag *file_tag, const gchar *encoded_by);
void et_file_tag_set_picture (File_Tag *file_tag, const EtPicture *pic);

void et_file_tag_copy_into (File_Tag *destination, const File_Tag *source);
void et_file_tag_copy_other_into (File_Tag *destination, const File_Tag *source);

gboolean et_file_tag_detect_difference (const File_Tag *FileTag1, const File_Tag  *FileTag2);

G_END_DECLS

#endif /* !ET_FILE_TAG_H_ */
