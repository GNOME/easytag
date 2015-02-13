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

#include "misc.h"

GtkWidget *MainWindow;
GSettings *MainSettings;

static void
misc_convert_duration (void)
{
    gsize i;
    gchar *time;

    static const struct
    {
        const gulong seconds;
        const gchar *result;
    } times[] = 
    {
        { 0, "0:00" },
        { 10, "0:10" },
        { 100, "1:40" },
        { 1000, "16:40" },
        { 10000, "2:46:40" },
        { 100000, "27:46:40" },
        { 1000000, "277:46:40" }
        /* TODO: Add more tests. */
    };

    for (i = 0; i < G_N_ELEMENTS (times); i++)
    {
        time = Convert_Duration (times[i].seconds);
        g_assert_cmpstr (time, ==, times[i].result);
        g_free (time);
    }
}

static void
misc_filename_prepare (void)
{
    gsize i;

    static const struct
    {
        const gchar *filename;
        const gchar *result_replace;
        const gchar *result;
    } filenames[] =
    {
        { "foo:bar", "foo-bar", "foo:bar" },
        { "foo" G_DIR_SEPARATOR_S "bar", "foo-bar", "foo-bar" },
        { "foo*bar", "foo+bar", "foo*bar" },
        { "foo?bar", "foo_bar", "foo?bar" },
        { "foo\"bar", "foo\'bar", "foo\"bar" },
        { "foo<bar", "foo(bar", "foo<bar" },
        { "foo>bar", "foo)bar", "foo>bar" },
        { "foo|bar", "foo-bar", "foo|bar" },
        { "foo|bar*baz", "foo-bar+baz", "foo|bar*baz" }
        /* TODO: Add more tests. */
    };

    for (i = 0; i < G_N_ELEMENTS (filenames); i++)
    {
        gchar *filename;

        filename = g_strdup (filenames[i].filename);

        /* Replace illegal characters. */
        et_filename_prepare (filename, TRUE);
        g_assert_cmpstr (filename, ==, filenames[i].result_replace);

        g_free (filename);

        filename = g_strdup (filenames[i].filename);

        /* Leave illegal characters unchanged. */
        et_filename_prepare (filename, FALSE);
        g_assert_cmpstr (filename, ==, filenames[i].result);

        g_free (filename);
    }
}

int
main (int argc, char** argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/misc/convert-duration", misc_convert_duration);
    g_test_add_func ("/misc/filename-prepare", misc_filename_prepare);

    return g_test_run ();
}
