/* EasyTAG - tag editor for audio files
 * Copyright (C) 2016 David King <amigadave@amigadave.com>
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

#include <glib.h>

#include "genres.h"

static void
genres_genre_no (void)
{
    gsize i;

    for (i = 0; i < G_N_ELEMENTS (id3_genres); i++)
    {
        g_assert_cmpstr (genre_no (i), !=, "Unknown");
    }

    g_assert_cmpint (i, ==, GENRE_MAX + 1);

    for (; i < 255; i++)
    {
        g_assert_cmpstr (genre_no (i), ==, "Unknown");
    }
}

int
main (int argc, char** argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/genres/genre_no", genres_genre_no);

    return g_test_run ();
}
