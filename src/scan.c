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

static gint
Scan_Word_Is_Roman_Numeral (const gchar *text)
{
    /* No need for caseless strchr. */
    static const gchar romans[] = "MmDdCcLlXxVvIi";

    gsize next_allowed = 0;
    gsize prev = 0;
    gsize count = 0;
    const gchar *i;

    for (i = text; *i; i++)
    {
        const char *s = strchr (romans, *i);

        if (s)
        {
            gsize c = (s - romans) / 2;

            if (c < next_allowed)
            {
                return 0;
            }

            if (c < prev)
            {
                /* After subtraction, no more subtracted chars allowed. */
                next_allowed = prev + 1;
            }
            else if (c == prev)
            {
                /* Allow indefinite repetition for m; three for c, x and i; and
                 * none for d, l and v. */
                if ((c && ++count > 3) || (c & 1))
                {
                    return 0;
                }

                /* No more subtraction. */
                next_allowed = c;
            }
            else if (c && !(c & 1))
            {
                /* For first occurrence of c, x and i, allow "subtraction" from
                 * 10 and 5 times self, reset counting. */
                next_allowed = c - 2;
                count = 1;
            }

            prev = c;
        }
        else
        {
            if (g_unichar_isalnum (g_utf8_get_char (i)))
            {
                return 0;
            }

            break;
        }
    }

    /* Return length of found Roman numeral. */
    return i - text;
}

/*
 * Function to set the first letter of each word to uppercase, according the "Chicago Manual of Style" (http://www.docstyles.com/cmscrib.htm#Note2)
 * No needed to reallocate
 */
void
Scan_Process_Fields_First_Letters_Uppercase (gchar **str,
                                             gboolean uppercase_preps,
                                             gboolean handle_roman)
{
/**** DANIEL TEST *****
    gchar *iter;
    gchar utf8_character[6];
    gboolean set_to_upper_case = TRUE;
    gunichar c;

    for (iter = text; *iter; iter = g_utf8_next_char(iter))
    {
        c = g_utf8_get_char(iter);
        if (set_to_upper_case && g_unichar_islower(c))
            strncpy(iter, utf8_character, g_unichar_to_utf8(g_unichar_toupper(c), utf8_character));
        else if (!set_to_upper_case && g_unichar_isupper(c))
            strncpy(iter, utf8_character, g_unichar_to_utf8(g_unichar_tolower(c), utf8_character));

        set_to_upper_case = (g_unichar_isalpha(c)
                            || c == (gunichar)'.'
                            || c == (gunichar)'\''
                            || c == (gunichar)'`') ? FALSE : TRUE;
    }
****/
/**** Barış Çiçek version ****/
    gchar *string = *str;
    gchar *word, *word1, *word2, *temp;
    gint i, len;
    gchar utf8_character[6];
    gunichar c;
    gboolean set_to_upper_case, set_to_upper_case_tmp;
    // There have to be space at the end of words to seperate them from prefix
    // Chicago Manual of Style "Heading caps" Capitalization Rules (CMS 1993, 282) (http://www.docstyles.com/cmscrib.htm#Note2)
    const gchar * exempt[] =
    {
        "a ",       "a_",
        "against ", "against_",
        "an ",      "an_",
        "and ",     "and_",
        "at ",      "at_",
        "between ", "between_",
        "but ",     "but_",
        "feat. ",   "feat._",
        "for ",     "for_",
        "in ",      "in_",
        "nor ",     "nor_",
        "of ",      "of_",
        //"off ",     "off_",   // Removed by Slash Bunny
        "on ",      "on_",
        "or ",      "or_",
        //"over ",    "over_",  // Removed by Slash Bunny
        "so ",      "so_",
        "the ",     "the_",
        "to ",      "to_",
        "with ",    "with_",
        "yet ",     "yet_",
        NULL
    };

    temp = Scan_Process_Fields_All_Downcase (string);
    g_free (*str);
    *str = string = temp;

    if (!g_utf8_validate(string,-1,NULL))
    {
        /* FIXME: Translatable string. */
        g_warning ("%s",
                   "Scan_Process_Fields_First_Letters_Uppercase: Not valid UTF-8!");
        return;
    }
    /* Removes trailing whitespace. */
    string = g_strchomp(string);

    temp = string;

    /* If the word is a roman numeral, capitalize all of it. */
    if (handle_roman && (len = Scan_Word_Is_Roman_Numeral (temp)))
    {
        gchar *tmp = g_utf8_strup (temp, len);
        strncpy (string, tmp, len);
        g_free (tmp);
    }
    else
    {
        // Set first character to uppercase
        c = g_utf8_get_char(temp);
        strncpy(string, utf8_character, g_unichar_to_utf8(g_unichar_toupper(c), utf8_character));
    }

    // Uppercase first character of each word, except for 'exempt[]' words lists
    while ( temp )
    {
        word = temp; // Needed if there is only one word
        word1 = strchr (temp, ' ');
        word2 = strchr (temp, '_');

        // Take the first string found (near beginning of string)
        if (word1 && word2)
            word = MIN(word1,word2);
        else if (word1)
            word = word1;
        else if (word2)
            word = word2;
        else
        {
            // Last word of the string: the first letter is always uppercase,
            // even if it's in the exempt list. This is a Chicago Manual of Style rule.
            // Last Word In String - Should Capitalize Regardless of Word (Chicago Manual of Style)
            c = g_utf8_get_char(word);
            strncpy(word, utf8_character, g_unichar_to_utf8(g_unichar_toupper(c), utf8_character));
            break;
        }

        // Go to first character of the word (char. after ' ' or '_')
        word = word+1;

        // If the word is a roman numeral, capitalize all of it
        if (handle_roman && (len = Scan_Word_Is_Roman_Numeral (word)))
        {
            gchar *tmp = g_utf8_strup (word, len);
            strncpy (word, tmp, len);
            g_free (tmp);
        }
        else
        {
            // Set uppercase the first character of this word
            c = g_utf8_get_char(word);
            strncpy(word, utf8_character, g_unichar_to_utf8(g_unichar_toupper(c), utf8_character));

            if (uppercase_preps)
            {
                goto increment;
            }

            /* Lowercase the first character of this word if found in the
             * exempt words list. */
            for (i=0; exempt[i]!=NULL; i++)
            {
                if (g_ascii_strncasecmp(exempt[i], word, strlen(exempt[i])) == 0)
                {
                    c = g_utf8_get_char(word);
                    strncpy(word, utf8_character, g_unichar_to_utf8(g_unichar_tolower(c), utf8_character));
                    break;
                }
            }
        }

increment:
        temp = word;
    }

    // Uppercase letter placed after some characters like '(', '[', '{'
    set_to_upper_case = FALSE;
    for (temp = string; *temp; temp = g_utf8_next_char(temp))
    {
        c = g_utf8_get_char(temp);
        set_to_upper_case_tmp = (  c == (gunichar)'('
                                || c == (gunichar)'['
                                || c == (gunichar)'{'
                                || c == (gunichar)'"'
                                || c == (gunichar)':'
                                || c == (gunichar)'.'
                                || c == (gunichar)'`'
                                || c == (gunichar)'-'
                                ) ? TRUE : FALSE;

        if (set_to_upper_case && g_unichar_islower(c))
            strncpy(temp, utf8_character, g_unichar_to_utf8(g_unichar_toupper(c), utf8_character));

        set_to_upper_case = set_to_upper_case_tmp;
    }
}
