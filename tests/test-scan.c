/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014 David King <amigadave@amigadave.com>
 * Copyright (C) 2014 Abhinav Jangda <abhijangda@hotmail.com>
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

#include "scan.h"

/* TODO: Add more test strings. */

static const gsize PERF_ITERATIONS = 500000;

static void
check_string (gchar *cases, gchar *result)
{
    gchar *string1, *string2;

    string1 = g_utf8_normalize (cases, -1, G_NORMALIZE_ALL);
    string2 = g_utf8_normalize (result, -1, G_NORMALIZE_ALL);

    g_assert_cmpstr (string1, ==, string2);

    g_free (string1);
    g_free (string2);
}

static void
scan_underscore_to_space (void)
{
    gsize i;
    gchar *cases[] = {" ်0STRING ်0_A_B"};
    gchar *results[] = {" ်0STRING ်0 A B"};

    for (i = 0; i < G_N_ELEMENTS (cases); i++)
    {
        gchar *string;

        string = g_strdup (cases[i]);
        Scan_Convert_Underscore_Into_Space (string);
        check_string (string, results[i]);

        g_free (string);
    }
}

static void
scan_remove_space (void)
{
    gsize i;
    gchar *cases[] = { " STR ING A   B " };
    gchar *results[] = { "STRINGAB" };

    for (i = 0; i < G_N_ELEMENTS (cases); i++)
    {
        gchar *string;

        string = g_strdup (cases[i]);
        Scan_Process_Fields_Remove_Space (string);
        check_string (string, results[i]);

        g_free (string);
    }
}

static void
scan_p20_to_space (void)
{
    gsize i;
    gchar *cases[] = { "S%20T%20R%20", "%20ă b  %20c", "STЂR%20ING%20A%20B" };
    gchar *results[] = { "S T R ", " ă b   c", "STЂR ING A B" };

    for (i = 0; i < G_N_ELEMENTS (cases); i++)
    {
        gchar *string;

        string = g_strdup (cases[i]);
        Scan_Convert_P20_Into_Space (string);
        check_string (string, results[i]);

        g_free (string);
    }
}

static void
scan_insert_space (void)
{
    gsize i;
    gchar *cases[] = { "STRINGAB", "StRiNgAb", "tRßiNgAb", "AՄՆ", "bՄԵ", "cՄԻ",
                       "dՎՆ", "eՄԽ", "fꜲ"};
    gchar *results[] = { "S T R I N G A B", "St Ri Ng Ab", "t Rßi Ng Ab",
                         "A Մ Ն", "b Մ Ե", "c Մ Ի", "d Վ Ն", "e Մ Խ", "f Ꜳ" };

    for (i = 0; i < G_N_ELEMENTS (cases); i++)
    {
        gchar *string, *res;

        string = g_strdup (cases[i]);
        res = Scan_Process_Fields_Insert_Space (string);
        check_string (res, results[i]);

        g_free (string);
        g_free (res);
    }
}

static void
scan_all_uppercase (void)
{
    gsize i;
    gchar *cases[] = { "stringab", "tRßiNgAb", "aŉbcd", "lowΐer", "uppΰer",
                       "sTRINGև", "ᾖᾀ", "pᾖp", "sAﬄAs" };
    gchar *results[] = { "STRINGAB", "TRSSINGAB", "AʼNBCD", "LOWΪ́ER", "UPPΫ́ER",
                         "STRINGԵՒ", "ἮΙἈΙ", "PἮΙP", "SAFFLAS" };

    for (i = 0; i < G_N_ELEMENTS (cases); i++)
    {
        gchar *string, *res;

        string = g_strdup (cases[i]);
        res = Scan_Process_Fields_All_Uppercase (string);
        check_string (res, results[i]);

        g_free (string);
        g_free (res);
    }
}

static void
scan_all_lowercase (void)
{
    gsize i;
    gchar *cases[] = { "STRINGAB", "tRßiNgAb", "SMALLß", "AAAԵՒBB", "ʼN",
                       "PΪ́E", "ἮΙ", "Ϋ́E" };
    gchar *results[] = { "stringab", "trßingab", "smallß", "aaaեւbb", "ʼn",
                         "pΐe", "ἦι", "ΰe" };

    for (i = 0; i < G_N_ELEMENTS (cases); i++)
    {
        gchar *string, *res;

        string = g_strdup (cases[i]);
        res = Scan_Process_Fields_All_Downcase (string);
        check_string (res, results[i]);

        g_free (string);
        g_free (res);
    }
}

static void
scan_letter_uppercase (void)
{
    gsize i;
    gchar *cases[] = { "st ri ng in ab", "tr ßi ng ab", "ßr ßi ng ab",
                       "ßr i ng ab", "ßr mi ng ab", "I I ng ab", "ß I ng ab",
                       "ßi ng ab" };
    gchar *results[] = { "St ri ng in ab", "Tr ßi ng ab", "SSr ßi ng ab",
                         "SSr I ng ab", "SSr mi ng ab", "I I ng ab",
                         "SS I ng ab", "SSi ng ab" };

    for (i = 0; i < G_N_ELEMENTS (cases); i++)
    {
        gchar *string, *res;

        string = g_strdup (cases[i]);
        res = Scan_Process_Fields_Letter_Uppercase (string);
        check_string (res, results[i]);

        g_free (string);
        g_free (res);
    }
}

static void
scan_letters_uppercase (void)
{
    gsize i;
    const gchar *cases[] = { "Foo Bar The Baz", "The", "The The",
                             "The The The", "Vibrate (single version)",
                             "MCMXC", "Foo Bar The III (single version)" };
    gchar *results[] = { "Foo Bar the Baz", "The", "The The", "The the The",
                         "Vibrate (Single Version)", "Mcmxc",
                         "Foo Bar the Iii (Single Version)" };
    gchar *results_roman[] = { "Foo Bar the Baz", "The", "The The",
                               "The the The", "Vibrate (Single Version)",
                               "MCMXC", "Foo Bar the III (Single Version)" };
    gchar *results_preps[] = { "Foo Bar The Baz", "The", "The The",
                               "The The The", "Vibrate (Single Version)",
                               "Mcmxc", "Foo Bar The Iii (Single Version)" };
    gchar *results_preps_roman[] = { "Foo Bar The Baz", "The", "The The",
                                     "The The The", "Vibrate (Single Version)",
                                     "MCMXC",
                                     "Foo Bar The III (Single Version)" };

    for (i = 0; i < G_N_ELEMENTS (cases); i++)
    {
        gchar *string;

        /* Lower-case exempted words, do not handle Roman numerals. */
        string = g_strdup (cases[i]);
        Scan_Process_Fields_First_Letters_Uppercase (&string, FALSE, FALSE);
        check_string (string, results[i]);
        g_free (string);

        /* Lower-case exempted words, handle Roman numerals. */
        string = g_strdup (cases[i]);
        Scan_Process_Fields_First_Letters_Uppercase (&string, FALSE, TRUE);
        check_string (string, results_roman[i]);
        g_free (string);

        /* Upper-case all words, do not handle Roman numerals. */
        string = g_strdup (cases[i]);
        Scan_Process_Fields_First_Letters_Uppercase (&string, TRUE, FALSE);
        check_string (string, results_preps[i]);
        g_free (string);

        /* Upper-case all words, handle Roman numerals. */
        string = g_strdup (cases[i]);
        Scan_Process_Fields_First_Letters_Uppercase (&string, TRUE, TRUE);
        check_string (string, results_preps_roman[i]);
        g_free (string);
    }
}

static void
scan_perf (gconstpointer user_data)
{
    gsize i;
    gdouble time;

    g_test_timer_start ();

    for (i = 0; i < PERF_ITERATIONS; i++)
    {
        ((GTestFunc)user_data) ();
    }

    time = g_test_timer_elapsed ();

    g_test_minimized_result (time, "%6.1f seconds", time);
}

int
main (int argc, char** argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/scan/underscore-to-space", scan_underscore_to_space);
    g_test_add_func ("/scan/remove-space", scan_remove_space);
    g_test_add_func ("/scan/P20-to-space", scan_p20_to_space);
    g_test_add_func ("/scan/insert-space", scan_insert_space);
    g_test_add_func ("/scan/all-uppercase", scan_all_uppercase);
    g_test_add_func ("/scan/all-lowercase", scan_all_lowercase);
    g_test_add_func ("/scan/letter-uppercase", scan_letter_uppercase);
    g_test_add_func ("/scan/letters-uppercase", scan_letters_uppercase);

    if (g_test_perf ())
    {
        g_test_add_data_func ("/scan/perf/underscore-to-space",
                              scan_underscore_to_space, scan_perf);
        g_test_add_data_func ("/scan/perf/remove-space", scan_remove_space,
                              scan_perf);
        g_test_add_data_func ("/scan/perf/P20-to-space", scan_p20_to_space,
                              scan_perf);
        g_test_add_data_func ("/scan/perf/insert-space", scan_insert_space,
                              scan_perf);
        g_test_add_data_func ("/scan/perf/all-uppercase", scan_all_uppercase,
                              scan_perf);
        g_test_add_data_func ("/scan/perf/all-lowercase", scan_all_lowercase,
                              scan_perf);
        g_test_add_data_func ("/scan/perf/letter-uppercase",
                              scan_letter_uppercase, scan_perf);
        g_test_add_data_func ("/scan/perf/letters-uppercase",
                              scan_letters_uppercase, scan_perf);
    }

    return g_test_run ();
}
