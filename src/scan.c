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

#include "scan.h"

#include <string.h>

/*
 * Function to replace underscore '_' by a space. No need to reallocate.
 */
void
Scan_Convert_Underscore_Into_Space (gchar *string)
{
    gchar *tmp = string;

    while ((tmp = strchr (tmp, '_')) != NULL)
    {
        *tmp = ' ';
    }
}

/*
 * Function to replace %20 by a space. No need to reallocate.
 */
void
Scan_Convert_P20_Into_Space (gchar *string)
{
    gchar *tmp = string;

    while ((tmp = strstr (tmp, "%20")) != NULL)
    {
        *(tmp++) = ' ';
        strcpy (tmp, tmp + 2);
    }
}

/*
 * Function to replace space by '_'. No need to reallocate.
 */
void
Scan_Convert_Space_Into_Underscore (gchar *string)
{
    gchar *tmp = string;

    while ((tmp = strchr (tmp, ' ')) != NULL)
    {
        *tmp = '_';
    }
}

void
Scan_Process_Fields_Remove_Space (gchar *string)
{
    gchar *tmp, *tmp1;

    tmp = tmp1 = string;

    while (*tmp)
    {
        while (*tmp == ' ')
            tmp++;
        if (*tmp)
            *(tmp1++) = *(tmp++);
    }
    *tmp1 = '\0';
}

/*
 * Scan_Process_Fields_Insert_Space:
 * @string: Input string
 *
 * This function will insert space before every uppercase character.
 *
 * Returns: A newly allocated string.
 */
gchar *
Scan_Process_Fields_Insert_Space (const gchar *string)
{
    gchar *iter;
    gunichar c;
    GString *string1;

    string1 = g_string_new ("");
    g_string_append_c (string1, *string);

    for (iter = g_utf8_next_char (string); *iter; iter = g_utf8_next_char (iter))
    {
        c = g_utf8_get_char (iter);

        if (g_unichar_isupper (c))
        {
            g_string_append_c (string1, ' ');
        }

        g_string_append_unichar (string1, c);
    }

    return g_string_free (string1, FALSE);
}

/*
 * The function removes the duplicated spaces. No need to reallocate.
 */
void
Scan_Process_Fields_Keep_One_Space (gchar *string)
{
    gchar *tmp, *tmp1;

    tmp = tmp1 = string;

    // Remove multiple consecutive underscores and spaces.
    while (*tmp1)
    {
        while (*tmp1 && *tmp1 != ' ' && *tmp1 != '_')
            *(tmp++) = *(tmp1++);
        if (!*tmp1)
            break;
        *(tmp++) = *(tmp1++);
        while (*tmp1 == ' ' || *tmp1 == '_')
            tmp1++;
    }
    *tmp = '\0';
}

/*
 * Function to remove spaces
 * No need to reallocate
 */
void
Scan_Remove_Spaces (gchar *string)
{
  int nextnotspace = 0, pos = 0;

  while(string[pos] != '\0')
  {
    if(string[pos] == ' ')
    {
      nextnotspace = pos;
      while(string[++nextnotspace] == ' ');
      string[pos] = string[nextnotspace];
      string[nextnotspace] = ' ';
      continue;
    }
    pos++;
  }
}

/* Returns a newly-allocated string. */
gchar *
Scan_Process_Fields_All_Uppercase (const gchar *string)
{
    return g_utf8_strup (string, -1);
}

/* Returns a newly-allocated string. */
gchar *
Scan_Process_Fields_All_Downcase (const gchar *string)
{
    return g_utf8_strdown (string, -1);
}

/* Returns a newly-allocated string. */
gchar *
Scan_Process_Fields_Letter_Uppercase (const gchar *string)
{
    const gchar *temp;
    gchar temp2[6];
    gboolean set_to_upper_case = TRUE;
    gunichar c;
    GString *string1;

    string1 = g_string_new ("");

    for (temp = string; *temp; temp = g_utf8_next_char (temp))
    {
        gchar *temp3;
        int l;

        c = g_utf8_get_char (temp);
        l = g_unichar_to_utf8 (c, temp2);

        if (set_to_upper_case && g_unichar_islower(c))
        {
            temp3 = g_utf8_strup (temp2, l);
            g_string_append (string1, temp3);
            g_free (temp3);
        }
        else if (!set_to_upper_case && g_unichar_isupper(c))
        {
            temp3 = g_utf8_strdown (temp2, l);
            g_string_append (string1, temp3);
            g_free (temp3);
        }
        else
        {
            g_string_append_len (string1, temp2, l);
        }

        /* Uppercase the word 'I' in english */
        if (!set_to_upper_case &&
            (*(temp - 1) == ' ' || *(temp - 1) == '_') &&
            (*temp == 'i' || *temp == 'I') &&
            (*(temp + 1) == ' ' || *(temp + 1) == '_'))
        {
            string1->str [string1->len - 1] = 'I';
        }

        /* After the first time, all will be lower case. */
        set_to_upper_case = FALSE;
    }

    return g_string_free (string1, FALSE);
}
