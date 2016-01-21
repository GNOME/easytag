/* EasyTAG - tag editor for audio files
 * Copyright (C) 2015-2016 David King <amigadave@amigadave.com>
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

#include "file_tag.h"

#include "misc.h"
#include "picture.h"

GtkWidget *MainWindow;
GSettings *MainSettings;

static void
file_tag_new (void)
{
    File_Tag *file_tag;

    file_tag = et_file_tag_new ();

    g_assert (file_tag);

    et_file_tag_free (file_tag);
}

static void
file_tag_copy (void)
{
    File_Tag *tag1;
    File_Tag *tag2;

    tag1 = et_file_tag_new ();

    g_assert (tag1);

    et_file_tag_set_title (tag1, "foo");
    et_file_tag_set_artist (tag1, "bar");
    et_file_tag_set_album_artist (tag1, "baz");

    g_assert_cmpstr (tag1->title, ==, "foo");
    g_assert_cmpstr (tag1->artist, ==, "bar");
    g_assert_cmpstr (tag1->album_artist, ==, "baz");

    tag2 = et_file_tag_new ();

    g_assert (tag2);

    et_file_tag_copy_into (tag2, tag1);

    g_assert_cmpstr (tag2->title, ==, "foo");
    g_assert_cmpstr (tag2->artist, ==, "bar");
    g_assert_cmpstr (tag2->album_artist, ==, "baz");

    et_file_tag_free (tag2);
    et_file_tag_free (tag1);
}

static void
file_tag_copy_other (void)
{
    File_Tag *tag1;
    File_Tag *tag2;
    GList *l;

    tag1 = et_file_tag_new ();

    g_assert (tag1);

    tag1->other = g_list_prepend (tag1->other, g_strdup ("foo"));

    tag2 = et_file_tag_new ();

    g_assert (tag2);

    tag2->other = g_list_prepend (tag2->other, g_strdup ("bar"));

    et_file_tag_copy_other_into (tag1, tag2);

    l = tag1->other;
    g_assert_cmpstr (l->data, ==, "foo");

    l = g_list_next (l);
    g_assert_cmpstr (l->data, ==, "bar");

    et_file_tag_free (tag2);
    et_file_tag_free (tag1);
}

static void
file_tag_difference (void)
{
    File_Tag *tag1;
    File_Tag *tag2;
    GBytes *bytes;

    tag1 = et_file_tag_new ();

    g_assert (tag1);

    et_file_tag_set_title (tag1, "foo：");

    /* Contains a full-width colon, which should compare differently to a
     * colon. */
    g_assert_cmpstr (tag1->title, ==, "foo：");

    tag2 = et_file_tag_new ();

    g_assert (tag2);

    et_file_tag_set_title (tag2, "foo:");

    g_test_bug ("744897");
    g_assert (et_file_tag_detect_difference (tag1, tag2));

    et_file_tag_free (tag2);
    et_file_tag_free (tag1);

    tag1 = et_file_tag_new ();

    et_file_tag_set_artist (tag1, "bar");

    tag2 = et_file_tag_new ();

    et_file_tag_set_artist (tag2, "baz");

    g_assert (et_file_tag_detect_difference (tag1, tag2));

    et_file_tag_free (tag2);
    et_file_tag_free (tag1);

    tag1 = et_file_tag_new ();
    bytes = g_bytes_new_static ("foo", 3);

    et_file_tag_set_picture (tag1,
                             et_picture_new (ET_PICTURE_TYPE_FRONT_COVER, "", 0, 0,
                                             bytes));

    g_bytes_unref (bytes);

    tag2 = et_file_tag_new ();

    g_assert (et_file_tag_detect_difference (tag1, tag2));

    et_file_tag_free (tag2);
    et_file_tag_free (tag1);
}

int
main (int argc, char** argv)
{
    g_test_init (&argc, &argv, NULL);
    g_test_bug_base ("https://bugzilla.gnome.org/show_bug.cgi?id=");

    g_test_add_func ("/file_tag/new", file_tag_new);
    g_test_add_func ("/file_tag/copy", file_tag_copy);
    g_test_add_func ("/file_tag/copy-other", file_tag_copy_other);
    g_test_add_func ("/file_tag/difference", file_tag_difference);

    return g_test_run ();
}
