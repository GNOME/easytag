/* EasyTAG - tag editor for audio files
 * Copyright (C) 2004  Santtu Lakkala <inz@inz.fi>
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

#include <string.h>

#include "dlm.h"

static int dlm_cost    (const gchar, const gchar);
static int dlm_minimum (int a, int b, int c, int d);

/*
 * Compute the Damerau-Levenshtein Distance between utf-8 strings ds and dt.
 */
gint
dlm (const gchar *ds, const gchar *dt)
{
    gsize n;
    gsize m;

    /* Casefold for better matching of the strings. */
    gchar *s = g_utf8_casefold(ds, -1);
    gchar *t = g_utf8_casefold(dt, -1);

    n = strlen(s);
    m = strlen(t);

    if (n && m)
    {
        gint *d;
        gsize i;
        gsize j;
        gint metric;

        n++;
        m++;

        d = (gint *)g_malloc0 (sizeof (gint) * m * n);

        for (i = 0; i < m; i++)
            d[i * n] = i;

        for (i = 1; i < n; i++)
        {
            d[i] = i;
            for (j = 1; j < m; j++)
            {
                d[j * n + i] = dlm_minimum(
                    d[(j - 1) * n + i] + 1,
                    d[j * n + i - 1] + 1,
                    d[(j - 1) * n + i - 1] + dlm_cost(s[i - 1], t[j - 1]),
                    (j>1 && i>1 ?
                         d[(j - 2) * n + i - 2] + dlm_cost(s[i - 1], t[j])
                        + dlm_cost(s[i], t[j - 1])
                    : 0x7fff)
                );
            }
        }
        metric = d[n * m - 1];
        g_free(d);

        /* Count a "similarity value" */
        /* Subtract the extra byte added to each string length earlier. */
        metric = 1000 - (1000 * (metric * 2)) / (m + n - 2);

        g_free(t);
        g_free(s);
        return metric;
    }
    g_free(t);
    g_free(s);

    /* Return value of -1 indicates an error */
    return -1;
}

/* "Cost" of changing from a to b. */
static int
dlm_cost (const char a, const char b)
{
    return a == b ? 0 : 1;
}

/* Return the smallest of four integers. */
static int
dlm_minimum (int a, int b, int c, int d)
{
    int min = a;
    if (b < min)
    {
        min = b;
    }
    if (c < min)
    {
        min = c;
    }
    if (d < min)
    {
        min = d;
    }
    return min;
}
