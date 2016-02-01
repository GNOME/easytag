/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014-2016 David King <amigadave@amigadave.com>
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

#include <glib/gstdio.h>

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
        { "foo|bar*baz", "foo-bar+baz", "foo|bar*baz" },
        { "foo.", "foo_", "foo." },
        { "foo ", "foo_", "foo " }
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

static void
misc_normalized_strcmp0 (void)
{
    static const gchar str1[] = "foo";
    static const gchar str2[] = "bar";
    static const gchar str3[] = "BAR";

    g_assert_cmpint (et_normalized_strcmp0 (NULL, NULL), ==, 0);
    g_assert_cmpint (et_normalized_strcmp0 (str1, NULL), >, 0);
    g_assert_cmpint (et_normalized_strcmp0 (NULL, str2), <, 0);
    g_assert_cmpint (et_normalized_strcmp0 (str1, str2), >, 0);
    g_assert_cmpint (et_normalized_strcmp0 (str2, str3), >, 0);
}

static void
misc_normalized_strcasecmp0 (void)
{
    gsize i;

    static const struct
    {
        const gchar *str1;
        const gchar *str2;
        const gint result;
    } strings[] =
    {
        { NULL, NULL, 0 },
        { "foo", NULL, 1 },
        { NULL, "bar", -1 },
        { "foo", "bar", -1 },
        { "FOO", "foo", 0 },
        { "foo", "FOO", 0 }
        /* TODO: Add more tests. */
    };

    for (i = 0; i < G_N_ELEMENTS (strings); i++)
    {
        g_assert_cmpint (et_normalized_strcasecmp0 (strings->str1,
                                                    strings->str2), ==,
                         strings->result);
    }
}

static void
misc_rename_file (void)
{
    gchar *filename1;
    gchar *filename2;
    gchar *filename3;
    gchar *basename;
    gchar *basename_upper;
    gchar *dirname;
    gint fd1;
    gint fd2;
    gint fd3;
    GError *error1 = NULL;
    GError *error2 = NULL;
    GError *error3 = NULL;

    fd1 = g_file_open_tmp ("EasyTAG-test1.XXXXXX", &filename1, &error1);
    fd2 = g_file_open_tmp ("EasyTAG-test2.XXXXXX", &filename2, &error2);
    g_assert_no_error (error1);
    g_assert_no_error (error2);

    g_close (fd1, &error1);
    g_close (fd2, &error2);
    g_assert_no_error (error1);
    g_assert_no_error (error2);

    /* Renaming to an existing filename should fail. */
    et_rename_file (filename1, filename2, &error1);
    et_rename_file (filename2, filename1, &error2);
    g_assert_error (error1, G_IO_ERROR, G_IO_ERROR_EXISTS);
    g_assert_error (error2, G_IO_ERROR, G_IO_ERROR_EXISTS);

    g_clear_error (&error1);
    g_clear_error (&error2);

    fd3 = g_file_open_tmp ("EasyTAG-test3.XXXXXX", &filename3, &error3);
    g_assert_no_error (error3);

    g_close (fd3, &error3);
    g_assert_no_error (error3);

    g_assert_cmpint (g_unlink (filename3), ==, 0);

    g_assert_cmpint (g_rename (filename2, filename3), ==, 0);

    /* Renaming to a new filename should succeed. */
    et_rename_file (filename1, filename2, &error1);
    et_rename_file (filename2, filename1, &error2);
    g_assert_no_error (error1);
    g_assert_no_error (error2);

    g_assert_cmpint (g_unlink (filename1), ==, 0);

    g_free (filename1);
    g_free (filename2);

    basename = g_path_get_basename (filename3);
    dirname = g_path_get_dirname (filename3);

    basename_upper = g_ascii_strup (basename, -1);
    g_free (basename);

    filename2 = g_build_filename (dirname, basename_upper, NULL);
    g_free (basename_upper);
    g_free (dirname);

    /* Renaming to a new filename, differing only by case, should succeed, even
     * in the case of a case-insensitive filesystem. */
    et_rename_file (filename3, filename2, &error3);
    g_assert_no_error (error3);

    g_assert_cmpint (g_unlink (filename2), ==, 0);

    g_free (filename2);
    g_free (filename3);
}

static void
misc_str_empty (void)
{
    gsize i;
    static const struct
    {
        const gchar *string;
        const gboolean empty;
    } strings[] =
    {
        { NULL, TRUE },
        { "", TRUE },
        { "\0a", TRUE },
        { "a", FALSE }
    };

    for (i = 0; i < G_N_ELEMENTS (strings); i++)
    {
        gint result;

        result = et_str_empty (strings[i].string);
        g_assert (strings[i].empty == result);
    }
}

static void
misc_undo_key (void)
{
    guint undo_key;

    undo_key = et_undo_key_new ();
    g_assert_cmpint (undo_key, >, 0U);
    g_assert_cmpint (undo_key, <, et_undo_key_new ());
}
int
main (int argc, char** argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/misc/convert-duration", misc_convert_duration);
    g_test_add_func ("/misc/filename-prepare", misc_filename_prepare);
    g_test_add_func ("/misc/normalized-strcmp0", misc_normalized_strcmp0);
    g_test_add_func ("/misc/normalized-strcasecmp0",
                     misc_normalized_strcasecmp0);
    g_test_add_func ("/misc/rename-file", misc_rename_file);
    g_test_add_func ("/misc/str-empty", misc_str_empty);
    g_test_add_func ("/misc/undo-key", misc_undo_key);

    return g_test_run ();
}
