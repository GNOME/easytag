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

#include "dlm.h"

static void
dlm_dlm (void)
{
    gsize i;

    static const struct
    {
        const gchar *str1;
        const gchar *str2;
        const gint metric;
    } strings[] = 
    {
        { "foo", "foo", 1000 },
        { "foo", "bar", 0 },
        { "bar", "baz", 667 },
        { "bar", "bra", 667 },
        { "bar", "ba", 600 },
        { "ba", "bar", 600 },
        { "foobarbaz", "foobarbaz", 1000 },
        { "foobarbaz", "foobazbar", 778 },
        { "foobarbaz", "zabraboof", 223 },
        { "foobarbaz", "iiiiiiiii", 0 },
        { "1234567890", "abcdefghij", 0 },
        { "", "", -1 },
        { "foo", "", -1 },
        { "", "foo", -1 },
    };

    for (i = 0; i < G_N_ELEMENTS (strings); i++)
    {
        gint metric;

        metric = dlm (strings[i].str1, strings[i].str2);
        g_assert_cmpint (strings[i].metric, ==, metric);
    }
}

static void
dlm_perf_dlm (void)
{
    gsize i;
    const gsize PERF_ITERATIONS = 500000;
    gdouble time;

    g_test_timer_start ();

    for (i = 0; i < PERF_ITERATIONS; i++)
    {
        g_assert_cmpint (dlm ("foobarbaz", "zabraboof"), ==, 223);
    }

    time = g_test_timer_elapsed ();

    g_test_minimized_result (time, "%6.1f seconds", time);
}

int
main (int argc, char** argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/dlm/dlm", dlm_dlm);

    if (g_test_perf ())
    {
        g_test_add_func ("/dlm/perf/dlm", dlm_perf_dlm);
    }

    return g_test_run ();
}
