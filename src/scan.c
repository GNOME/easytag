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
    gchar *tmp, *tmp1;

    while ((tmp = strstr (string, "%20")) != NULL)
    {
        tmp1 = tmp + 3;
        *(tmp++) = ' ';
        while (*tmp1)
            *(tmp++) = *(tmp1++);
        *tmp = '\0';
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
 * The function inserts a space before an uppercase letter
 * It is needed to realloc the memory!
 */
void
Scan_Process_Fields_Insert_Space (gchar **string)
{
    gchar *iter;
    gunichar c;
    gint j;
    guint string_length;
    gchar *string1;

    // FIX ME : we suppose that it will not grow more than 2 times its size...
    string_length = 2 * strlen(*string);
    //string1 = g_realloc(*string, string_length+1);
    string1       = g_malloc(string_length+1);
    strncpy(string1,*string,string_length);
    string1[string_length]='\0';
    g_free(*string);
    *string = string1;

    for (iter = g_utf8_next_char(*string); *iter; iter = g_utf8_next_char(iter)) // At start : g_utf8_next_char to not consider first "uppercase" letter
    {
        c = g_utf8_get_char(iter);

        if (g_unichar_isupper(c))
        {
            for (j = strlen(iter); j > 0; j--)
                *(iter + j) = *(iter + j - 1);
            *iter = ' ';
            iter++;
        }
    }
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

void
Scan_Process_Fields_All_Uppercase (gchar *string)
{
    gchar *temp;
    gchar temp2[6]; // Must have at least 6 bytes of space
    gunichar c;

    for (temp = string; *temp; temp = g_utf8_next_char(temp))
    {
        c = g_utf8_get_char(temp);
        if (g_unichar_islower(c))
            strncpy(temp, temp2, g_unichar_to_utf8(g_unichar_toupper(c), temp2));
    }
}

void
Scan_Process_Fields_All_Downcase (gchar *string)
{
    gchar *temp;
    gchar temp2[6];
    gunichar c;

    for (temp = string; *temp; temp = g_utf8_next_char(temp))
    {
        c = g_utf8_get_char(temp);
        if (g_unichar_isupper(c))
            strncpy(temp, temp2, g_unichar_to_utf8(g_unichar_tolower(c), temp2));
    }
}

void
Scan_Process_Fields_Letter_Uppercase (gchar *string)
{
    gchar *temp;
    gchar temp2[6];
    gboolean set_to_upper_case = TRUE;
    gunichar c;
    gchar utf8_character[6];
    gchar *word, *word1, *word2;

    for (temp = string; *temp; temp = g_utf8_next_char(temp))
    {
        c = g_utf8_get_char(temp);
        if (set_to_upper_case && g_unichar_islower(c))
            strncpy(temp, temp2, g_unichar_to_utf8(g_unichar_toupper(c), temp2));
        else if (!set_to_upper_case && g_unichar_isupper(c))
            strncpy(temp, temp2, g_unichar_to_utf8(g_unichar_tolower(c), temp2));
        set_to_upper_case = FALSE; // After the first time, all will be down case
    }

    temp = string;

    // Uppercase again the word 'I' in english
    while ( temp )
    {
        word = temp; // Needed if there is only one word
        word1 = g_utf8_strchr(temp,-1,' ');
        word2 = g_utf8_strchr(temp,-1,'_');

        // Take the first string found (near beginning of string)
        if (word1 && word2)
            word = MIN(word1,word2);
        else if (word1)
            word = word1;
        else if (word2)
            word = word2;
        else
            // Last word of the string
            break;

        // Go to first character of the word (char. after ' ' or '_')
        word = word+1;

        // Set uppercase word 'I'
        if (g_ascii_strncasecmp("I ", word, strlen("I ")) == 0)
        {
            c = g_utf8_get_char(word);
            strncpy(word, utf8_character, g_unichar_to_utf8(g_unichar_toupper(c), utf8_character));
        }

        temp = word;
    }
}
