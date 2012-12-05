/* dlm.c - 2004/07/04 - Santtu Lakkala */

#include <glib.h>
#include <string.h>

#include "dlm.h"

static int dlm_cost    (const gchar, const gchar);
static int dlm_minimum (int a, int b, int c, int d);

/*
 * Compute the Damerau-Levenshtein Distance between utf-8 strings ds and dt.
 */
int dlm (const gchar *ds, const gchar *dt)
{
    int i, j, n, m, metric;
    int *d;

    /* Casefold for better matching of the strings. */
    gchar *s = g_utf8_casefold(ds, -1);
    gchar *t = g_utf8_casefold(dt, -1);

    n = strlen(s);
    m = strlen(t);

    if (n && m)
    {
        n++;
        m++;

        d = (int *)g_malloc0(sizeof(int) * m * n);

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
        metric = 1000 - (1000 * (metric * 2)) / (m + n);

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
