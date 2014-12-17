/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014 David King <amigadave@amigadave.com>
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

static void
picture_copy (void)
{
    EtPicture *pic1;
    EtPicture *pic2;
    EtPicture *pic3;
    EtPicture *pic1_copy;
    EtPicture *pic2_copy;
    const EtPicture *pic3_copy;

    pic1 = et_picture_new ();

    pic1->type = ET_PICTURE_TYPE_LEAFLET_PAGE;
    pic1->width = 640;
    pic1->height = 480;
    pic1->description = g_strdup ("foobar.png");
    pic1->bytes = g_bytes_new_static ("foobar", 6);

    pic2 = et_picture_copy_all (pic1);

    g_assert_cmpint (pic2->type, ==, ET_PICTURE_TYPE_LEAFLET_PAGE);
    g_assert_cmpint (pic2->width, ==, 640);
    g_assert_cmpint (pic2->height, ==, 480);
    g_assert_cmpstr (pic2->description, ==, "foobar.png");
    g_assert_cmpint (g_bytes_hash (pic2->bytes), ==,
                     g_bytes_hash (pic1->bytes));
    g_assert (pic2->bytes == pic1->bytes);
    g_assert (pic2->next == NULL);

    pic3 = et_picture_new ();

    pic3->type = ET_PICTURE_TYPE_ILLUSTRATION;
    pic3->width = 320;
    pic3->height = 240;
    pic3->description = g_strdup ("bash.jpg");
    pic3->bytes = g_bytes_new_static ("foo", 3);

    pic1->next = pic2;
    pic2->next = pic3;

    pic2_copy = et_picture_copy_single (pic2);

    g_assert_cmpint (pic2_copy->type, ==, ET_PICTURE_TYPE_LEAFLET_PAGE);
    g_assert_cmpint (pic2_copy->width, ==, 640);
    g_assert_cmpint (pic2_copy->height, ==, 480);
    g_assert_cmpstr (pic2_copy->description, ==, "foobar.png");
    g_assert_cmpint (g_bytes_get_size (pic2_copy->bytes), ==, 6);
    g_assert (pic2_copy->next == NULL);

    pic1_copy = et_picture_copy_all (pic1);

    g_assert (pic1_copy->next != NULL);
    g_assert (pic1_copy->next->next != NULL);
    g_assert (pic1_copy->next->next->next == NULL);

    pic3_copy = pic1_copy->next->next;

    g_assert_cmpint (pic3_copy->type, ==, ET_PICTURE_TYPE_ILLUSTRATION);
    g_assert_cmpint (pic3_copy->width, ==, 320);
    g_assert_cmpint (pic3_copy->height, ==, 240);
    g_assert_cmpstr (pic3_copy->description, ==, "bash.jpg");
    g_assert_cmpint (g_bytes_get_size (pic3_copy->bytes), ==, 3);

    et_picture_free (pic1_copy);
    et_picture_free (pic2_copy);
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

int
main (int argc, char** argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/picture/copy", picture_copy);
    g_test_add_func ("/picture/type-from-filename",
                     picture_type_from_filename);

    return g_test_run ();
}
