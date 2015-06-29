/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014-2015 David King <amigadave@amigadave.com>
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
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "picture.h"

#include <gtk/gtk.h>
#include <string.h>

GtkWidget *MainWindow;
GSettings *MainSettings;

static void
picture_copy (void)
{
    GBytes *bytes;
    EtPicture *pic1;
    EtPicture *pic2;
    EtPicture *pic3;
    EtPicture *pic4;
    EtPicture *pic1_copy;
    EtPicture *pic2_copy;
    const EtPicture *pic3_copy;
    EtPicture *pic4_copy;

    bytes = g_bytes_new_static ("foobar", 6);
    pic1 = et_picture_new (ET_PICTURE_TYPE_LEAFLET_PAGE, "foobar.png", 640,
                           480, bytes);
    g_bytes_unref (bytes);

    pic2 = et_picture_copy_all (pic1);

    g_assert (!et_picture_detect_difference (pic1, pic2));
    g_assert (pic2->next == NULL);

    bytes = g_bytes_new_static ("foo", 3);
    pic3 = et_picture_new (ET_PICTURE_TYPE_ILLUSTRATION, "bash.jpg", 320, 240,
                           bytes);
    g_bytes_unref (bytes);

    pic1->next = pic2;
    pic2->next = pic3;

    pic2_copy = et_picture_copy_single (pic2);

    g_assert (!et_picture_detect_difference (pic2, pic2_copy));
    g_assert (pic2_copy->next == NULL);

    pic1_copy = et_picture_copy_all (pic1);

    g_assert (pic1_copy->next != NULL);
    g_assert (pic1_copy->next->next != NULL);
    g_assert (pic1_copy->next->next->next == NULL);

    pic3_copy = pic1_copy->next->next;

    g_assert (!et_picture_detect_difference (pic3, pic3_copy));

    bytes = g_bytes_new_static ("foobarbaz", 9);
    pic4 = et_picture_new (ET_PICTURE_TYPE_MEDIA, "baz.gif", 800, 600, bytes);
    g_bytes_unref (bytes);

    pic4_copy = g_boxed_copy (ET_TYPE_PICTURE, pic4);

    g_assert (!et_picture_detect_difference (pic4, pic4_copy));

    et_picture_free (pic1_copy);
    et_picture_free (pic2_copy);
    g_boxed_free (ET_TYPE_PICTURE, pic4_copy);
    et_picture_free (pic1);
    et_picture_free (pic4);
}

static void
picture_difference (void)
{
    GBytes *bytes;
    EtPicture *pic1;
    EtPicture *pic2;

    pic1 = NULL;
    pic2 = NULL;

    g_assert (!et_picture_detect_difference (pic1, pic2));

    bytes = g_bytes_new_static ("foobar", 6);
    pic1 = et_picture_new (ET_PICTURE_TYPE_LEAFLET_PAGE, "foobar.png", 640,
                           480, bytes);
    g_bytes_unref (bytes);

    g_assert (et_picture_detect_difference (pic1, pic2));
    g_assert (et_picture_detect_difference (pic2, pic1));

    pic2 = et_picture_new (ET_PICTURE_TYPE_ILLUSTRATION, "foobar.png", 640,
                           480, pic1->bytes);

    g_assert (et_picture_detect_difference (pic1, pic2));

    et_picture_free (pic2);
    pic2 = et_picture_new (ET_PICTURE_TYPE_LEAFLET_PAGE, "foobar.png", 480,
                           640, pic1->bytes);

    g_assert (et_picture_detect_difference (pic1, pic2));

    et_picture_free (pic2);
    pic2 = et_picture_new (ET_PICTURE_TYPE_LEAFLET_PAGE, "foobar.png", 640,
                           640, pic1->bytes);

    g_assert (et_picture_detect_difference (pic1, pic2));

    et_picture_free (pic2);
    pic2 = et_picture_new (ET_PICTURE_TYPE_LEAFLET_PAGE, "foobar.png", 480,
                           480, pic1->bytes);

    g_assert (et_picture_detect_difference (pic1, pic2));

    et_picture_free (pic2);
    pic2 = et_picture_new (ET_PICTURE_TYPE_LEAFLET_PAGE, "baz.gif", 640,
                           480, pic1->bytes);

    g_assert (et_picture_detect_difference (pic1, pic2));

    et_picture_free (pic2);
    bytes = g_bytes_new_static ("baz", 3);
    pic2 = et_picture_new (ET_PICTURE_TYPE_LEAFLET_PAGE, "foobar.png", 640,
                           480, bytes);
    g_bytes_unref (bytes);

    g_assert (et_picture_detect_difference (pic1, pic2));

    et_picture_free (pic2);
    pic2 = et_picture_copy_single (pic1);
    pic1->next = pic2;

    /* et_picture_detect_difference() does not examine the next pointer. */
    g_assert (!et_picture_detect_difference (pic1, pic2));

    et_picture_free (pic1);
}

static void
picture_type_from_filename (void)
{
    gsize i;

    static const struct
    {
        const gchar *filename;
        EtPictureType type;
    } pictures[] = 
    {
        { "no clues here", ET_PICTURE_TYPE_FRONT_COVER },
        { "cover.jpg", ET_PICTURE_TYPE_FRONT_COVER },
        { "inside cover.png", ET_PICTURE_TYPE_LEAFLET_PAGE },
        { "acdc", ET_PICTURE_TYPE_MEDIA },
        { "ACDC", ET_PICTURE_TYPE_MEDIA },
        { "aCdC", ET_PICTURE_TYPE_MEDIA },
        { "aC dC", ET_PICTURE_TYPE_FRONT_COVER },
        { "back in black", ET_PICTURE_TYPE_BACK_COVER },
        { "illustrations of grandeur", ET_PICTURE_TYPE_ILLUSTRATION },
        { "inside outside", ET_PICTURE_TYPE_LEAFLET_PAGE },
        { "front to back", ET_PICTURE_TYPE_FRONT_COVER },
        { "back to front", ET_PICTURE_TYPE_FRONT_COVER },
        { "inlay", ET_PICTURE_TYPE_LEAFLET_PAGE },
        { "leaflet", ET_PICTURE_TYPE_LEAFLET_PAGE },
        { "page", ET_PICTURE_TYPE_LEAFLET_PAGE },
        { "multimedia", ET_PICTURE_TYPE_MEDIA },
        { "artist band", ET_PICTURE_TYPE_ARTIST_PERFORMER },
        { "band", ET_PICTURE_TYPE_BAND_ORCHESTRA },
        { "orchestra", ET_PICTURE_TYPE_BAND_ORCHESTRA },
        { "performer", ET_PICTURE_TYPE_ARTIST_PERFORMER },
        { "composer", ET_PICTURE_TYPE_COMPOSER },
        { "lyricist", ET_PICTURE_TYPE_LYRICIST_TEXT_WRITER },
        { "writer", ET_PICTURE_TYPE_FRONT_COVER },
        { "publisher", ET_PICTURE_TYPE_PUBLISHER_STUDIO_LOGOTYPE },
        { "studio", ET_PICTURE_TYPE_FRONT_COVER }
    };

    for (i = 0; i < G_N_ELEMENTS (pictures); i++)
    {
        g_assert_cmpint (pictures[i].type, ==,
                         et_picture_type_from_filename (pictures[i].filename));
    }
}

static void
picture_format_from_data (void)
{
    gsize i;

    static const struct
    {
        const gchar *data;
        Picture_Format format;
    } pictures[] =
    {
        { "\xff\xd8", PICTURE_FORMAT_UNKNOWN },
        { "\xff\xd8\xff", PICTURE_FORMAT_JPEG },
        { "\x89PNG\x0d\x0a\x1a\x0a", PICTURE_FORMAT_PNG },
        { "GIF87a", PICTURE_FORMAT_GIF },
        { "GIF89a", PICTURE_FORMAT_GIF },
        { "GIF900", PICTURE_FORMAT_UNKNOWN }
    };

    for (i = 0; i < G_N_ELEMENTS (pictures); i++)
    {
        GBytes *bytes;
        EtPicture *pic;

        bytes = g_bytes_new_static (pictures[i].data,
                                    strlen (pictures[i].data) + 1);
        pic = et_picture_new (ET_PICTURE_TYPE_FRONT_COVER, "", 0, 0, bytes);
        g_bytes_unref (bytes);

        g_assert_cmpint (pictures[i].format, ==,
                         Picture_Format_From_Data (pic));

        et_picture_free (pic);
    }
}

int
main (int argc, char** argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/picture/copy", picture_copy);
    g_test_add_func ("/picture/difference", picture_difference);
    g_test_add_func ("/picture/format-from-data", picture_format_from_data);
    g_test_add_func ("/picture/type-from-filename",
                     picture_type_from_filename);

    return g_test_run ();
}
