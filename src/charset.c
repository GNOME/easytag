/*
 * Main part of code, written by:
 *
 * Copyright (C) 1999-2001  Håvard Kvålen <havardk@xmms.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include "config.h"

#include "charset.h"

#include <stdlib.h>
#include <glib/gi18n.h>

#ifdef HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

#include "setting.h"
#include "log.h"
#include "win32/win32dep.h"

typedef struct
{
    const gchar *charset_title;
    const gchar *charset_name;
} CharsetInfo;

#define CHARSET_TRANS_ARRAY_LEN ( sizeof(charset_trans_array) / sizeof((charset_trans_array)[0]) )
static const CharsetInfo charset_trans_array[] = {
    {N_("Arabic (IBM-864)"),                  "IBM864"        },
    {N_("Arabic (ISO-8859-6)"),               "ISO-8859-6"    },
    {N_("Arabic (Windows-1256)"),             "windows-1256"  },
    {N_("Baltic (ISO-8859-13)"),              "ISO-8859-13"   },
    {N_("Baltic (ISO-8859-4)"),               "ISO-8859-4"    },
    {N_("Baltic (Windows-1257)"),             "windows-1257"  },
    {N_("Celtic (ISO-8859-14)"),              "ISO-8859-14"   },
    {N_("Central European (IBM-852)"),        "IBM852"        },
    {N_("Central European (ISO-8859-2)"),     "ISO-8859-2"    },
    {N_("Central European (Windows-1250)"),   "windows-1250"  },
    {N_("Chinese Simplified (GB18030)"),      "gb18030"       },
    {N_("Chinese Simplified (GB2312)"),       "GB2312"        },
    {N_("Chinese Traditional (Big5)"),        "Big5"          },
    {N_("Chinese Traditional (Big5-HKSCS)"),  "Big5-HKSCS"    },
    {N_("Cyrillic (IBM-855)"),                "IBM855"        },
    {N_("Cyrillic (ISO-8859-5)"),             "ISO-8859-5"    },
    {N_("Cyrillic (ISO-IR-111)"),             "ISO-IR-111"    },
    {N_("Cyrillic (KOI8-R)"),                 "KOI8-R"        },
    {N_("Cyrillic (Windows-1251)"),           "windows-1251"  },
    {N_("Cyrillic/Russian (CP-866)"),         "IBM866"        },
    {N_("Cyrillic/Ukrainian (KOI8-U)"),       "KOI8-U"        },
    {N_("English (US-ASCII)"),                "us-ascii"      },
    {N_("Greek (ISO-8859-7)"),                "ISO-8859-7"    },
    {N_("Greek (Windows-1253)"),              "windows-1253"  },
    {N_("Hebrew (IBM-862)"),                  "IBM862"        },
    {N_("Hebrew (Windows-1255)"),             "windows-1255"  },
    {N_("Japanese (EUC-JP)"),                 "EUC-JP"        },
    {N_("Japanese (ISO-2022-JP)"),            "ISO-2022-JP"   },
    {N_("Japanese (Shift_JIS)"),              "Shift_JIS"     },
    {N_("Korean (EUC-KR)"),                   "EUC-KR"        },
    {N_("Nordic (ISO-8859-10)"),              "ISO-8859-10"   },
    {N_("South European (ISO-8859-3)"),       "ISO-8859-3"    },
    {N_("Thai (TIS-620)"),                    "TIS-620"       },
    {N_("Turkish (IBM-857)"),                 "IBM857"        },
    {N_("Turkish (ISO-8859-9)"),              "ISO-8859-9"    },
    {N_("Turkish (Windows-1254)"),            "windows-1254"  },
    //{N_("Unicode (UTF-7)"),                   "UTF-7"         },
    {N_("Unicode (UTF-8)"),                   "UTF-8"         },

    //{N_("Unicode (UTF-16BE)"),                "UTF-16BE"      },
    //{N_("Unicode (UTF-16LE)"),                "UTF-16LE"      },
    //{N_("Unicode (UTF-32BE)"),                "UTF-32BE"      },
    //{N_("Unicode (UTF-32LE)"),                "UTF-32LE"      },

    {N_("Vietnamese (VISCII)"),               "VISCII"        },
    {N_("Vietnamese (Windows-1258)"),         "windows-1258"  },
    {N_("Visual Hebrew (ISO-8859-8)"),        "ISO-8859-8"    },
    {N_("Western (IBM-850)"),                 "IBM850"        },
    {N_("Western (ISO-8859-1)"),              "ISO-8859-1"    },
    {N_("Western (ISO-8859-15)"),             "ISO-8859-15"   },
    {N_("Western (Windows-1252)"),            "windows-1252"  }

    /*
     * From this point, character sets aren't supported by iconv
     */
/*    {N_("Arabic (IBM-864-I)"),                "IBM864i"              },
    {N_("Arabic (ISO-8859-6-E)"),             "ISO-8859-6-E"         },
    {N_("Arabic (ISO-8859-6-I)"),             "ISO-8859-6-I"         },
    {N_("Arabic (MacArabic)"),                "x-mac-arabic"         },
    {N_("Armenian (ARMSCII-8)"),              "armscii-8"            },
    {N_("Central European (MacCE)"),          "x-mac-ce"             },
    {N_("Chinese Simplified (GBK)"),          "x-gbk"                },
    {N_("Chinese Simplified (HZ)"),           "HZ-GB-2312"           },
    {N_("Chinese Traditional (EUC-TW)"),      "x-euc-tw"             },
    {N_("Croatian (MacCroatian)"),            "x-mac-croatian"       },
    {N_("Cyrillic (MacCyrillic)"),            "x-mac-cyrillic"       },
    {N_("Cyrillic/Ukrainian (MacUkrainian)"), "x-mac-ukrainian"      },
    {N_("Farsi (MacFarsi)"),                  "x-mac-farsi"},
    {N_("Greek (MacGreek)"),                  "x-mac-greek"          },
    {N_("Gujarati (MacGujarati)"),            "x-mac-gujarati"       },
    {N_("Gurmukhi (MacGurmukhi)"),            "x-mac-gurmukhi"       },
    {N_("Hebrew (ISO-8859-8-E)"),             "ISO-8859-8-E"         },
    {N_("Hebrew (ISO-8859-8-I)"),             "ISO-8859-8-I"         },
    {N_("Hebrew (MacHebrew)"),                "x-mac-hebrew"         },
    {N_("Hindi (MacDevanagari)"),             "x-mac-devanagari"     },
    {N_("Icelandic (MacIcelandic)"),          "x-mac-icelandic"      },
    {N_("Korean (JOHAB)"),                    "x-johab"              },
    {N_("Korean (UHC)"),                      "x-windows-949"        },
    {N_("Romanian (MacRomanian)"),            "x-mac-romanian"       },
    {N_("Turkish (MacTurkish)"),              "x-mac-turkish"        },
    {N_("User Defined"),                      "x-user-defined"       },
    {N_("Vietnamese (TCVN)"),                 "x-viet-tcvn5712"      },
    {N_("Vietnamese (VPS)"),                  "x-viet-vps"           },
    {N_("Western (MacRoman)"),                "x-mac-roman"          },
    // charsets whithout possibly translatable names
    {"T61.8bit",                              "T61.8bit"             },
    {"x-imap4-modified-utf7",                 "x-imap4-modified-utf7"},
    {"x-u-escaped",                           "x-u-escaped"          },
    {"windows-936",                           "windows-936"          }
*/
};

static GHashTable *encodings;


/* stolen from gnome-desktop-item.c */
static gboolean
check_locale (const char *locale)
{
    GIConv cd = g_iconv_open ("UTF-8", locale);
    if ((GIConv)-1 == cd)
        return FALSE;
    g_iconv_close (cd);
    return TRUE;
}

/* stolen from gnome-desktop-item.c */
G_GNUC_NULL_TERMINATED static void
insert_locales (GHashTable *encs, const gchar *enc, ...)
{
    va_list args;
    char *s;

    va_start (args, enc);
    for (;;)
    {
        s = va_arg (args, char *);
        if (s == NULL)
            break;
        /* A GDestroyNotify is not passed, so casting away the const is
         * safe, as the key is never freed. */
        g_hash_table_insert (encs, s, (gpointer)enc);
    }
    va_end (args);
}

/* stolen from gnome-desktop-item.c */
/* make a standard conversion table from the desktop standard spec */
void
Charset_Insert_Locales_Init (void)
{
    /* FIXME: Use g_hash_table_new_full. */
    encodings = g_hash_table_new (g_str_hash, g_str_equal);

    /* "C" is plain ascii */
    insert_locales (encodings, "ASCII", "C", NULL);
#ifdef G_OS_WIN32
    insert_locales (encodings, "windows-1256", "ar", NULL); // 2006.12.31 - For testing with Arabic
#else /* !G_OS_WIN32 */
    insert_locales (encodings, "ISO-8859-6", "ar", NULL);
#endif /* !G_OS_WIN32 */
    insert_locales (encodings, "ARMSCII-8", "by", NULL);
    insert_locales (encodings, "BIG5", "zh_TW", NULL);
    insert_locales (encodings, "CP1251", "be", "bg", NULL);
    if (check_locale ("EUC-CN")) {
        insert_locales (encodings, "EUC-CN", "zh_CN", NULL);
    } else {
        insert_locales (encodings, "GB2312", "zh_CN", NULL);
    }
    insert_locales (encodings, "EUC-JP", "ja", NULL);
    insert_locales (encodings, "EUC-KR", "ko", NULL);
    /*insert_locales (encodings, "GEORGIAN-ACADEMY", NULL);*/
    insert_locales (encodings, "GEORGIAN-PS", "ka", NULL);
    insert_locales (encodings, "ISO-8859-1", "br", "ca", "da", "de", "en", "es", "eu", "fi", "fr", "gl", "it", "nl", "wa", "nb", "nn", "pt", "pt", "sv", NULL);
#ifdef G_OS_WIN32
    insert_locales (encodings, "windows-1250", "cs", "hr", "hu", "pl", "ro", "sk", "sl", "sq", "sr", NULL);
#else /* !G_OS_WIN32 */
    insert_locales (encodings, "ISO-8859-2", "cs", "hr", "hu", "pl", "ro", "sk", "sl", "sq", "sr", NULL);
#endif /* !G_OS_WIN32 */
    insert_locales (encodings, "ISO-8859-3", "eo", NULL);
    insert_locales (encodings, "ISO-8859-5", "mk", "sp", NULL);
#ifdef G_OS_WIN32
    insert_locales (encodings, "windows-1253", "el", NULL);
#else /* !G_OS_WIN32 */
    insert_locales (encodings, "ISO-8859-7", "el", NULL);
#endif /* !G_OS_WIN32 */
#ifdef G_OS_WIN32
    insert_locales (encodings, "windows-1254", "tr", NULL);
#else /* !G_OS_WIN32 */
    insert_locales (encodings, "ISO-8859-9", "tr", NULL);
#endif /* !G_OS_WIN32 */
    insert_locales (encodings, "ISO-8859-13", "lt", "lv", "mi", NULL);
    insert_locales (encodings, "ISO-8859-14", "ga", "cy", NULL);
    insert_locales (encodings, "ISO-8859-15", "et", NULL);
#ifdef G_OS_WIN32
    insert_locales (encodings, "windows-1251", "ru", NULL);
#else /* !G_OS_WIN32 */
    insert_locales (encodings, "KOI8-R", "ru", NULL);
#endif /* !G_OS_WIN32 */
    insert_locales (encodings, "KOI8-U", "uk", NULL);
    if (check_locale ("TCVN-5712")) {
        insert_locales (encodings, "TCVN-5712", "vi", NULL);
    } else {
        insert_locales (encodings, "TCVN", "vi", NULL);
    }
    insert_locales (encodings, "TIS-620", "th", NULL);
#ifdef G_OS_WIN32
    insert_locales (encodings, "windows-1255", "he", NULL);
#endif /* G_OS_WIN32 */
    /*insert_locales (encodings, "VISCII", NULL);*/
}

void
Charset_Insert_Locales_Destroy (void)
{
    g_hash_table_destroy (encodings);
}

/*
 * get_encoding_from_locale:
 * @locale: a locale string, of the form
 *          language[_territory][.codeset][@modifer]
 *
 * Get the legacy (pre-Unicode) character encoding for the @locale, falling
 * back to a hardcoded table if is not part of @locale, and as a last resort
 * falling back to UTF-8.
 *
 * Returns: the legacy character encoding of @locale, or "UTF-8" on failure
 */
const char *
get_encoding_from_locale (const char *locale)
{
    const char *encoding;
    GStrv variants;
    gsize i;

    g_return_val_if_fail (locale != NULL, NULL);

    /* Return early if the encoding is part of the locale. */
    encoding = strchr (locale, '.');

    if (encoding != NULL)
    {
        /* Ignore UTF-8 (and utf8). */
        if ((strncmp (encoding, ".UTF-8", 6) != 0)
            && (strncmp (encoding, ".utf8", 5) != 0))
        {
            const gchar *modifier;

            modifier = strchr (encoding, '@');

            if (modifier != NULL)
            {
                g_warning ("%s",
                           "Returning modifier in addition to character set");
            }

            return encoding;
        }
    }

    /* Loop over variants of the locale, returning the first match. */
    variants = g_get_locale_variants (locale);

    for (i = 0; variants[i]; i++)
    {
        encoding = g_hash_table_lookup (encodings, variants[i]);

        if (encoding != NULL)
        {
            g_strfreev (variants);
            return encoding;
        }
    }

    g_strfreev (variants);

    return "UTF-8";
}

/*
 * Return the locale from LANG if exists, else from LC_ALL
 *
 * http://www.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap08.html#tag_08_02
 *
 * LANG
 *     This variable shall determine the locale category for native language,
 *     local customs, and coded character set in the absence of the LC_ALL and
 *     other LC_* ( LC_COLLATE , LC_CTYPE , LC_MESSAGES , LC_MONETARY , LC_NUMERIC ,
 *     LC_TIME ) environment variables. This can be used by applications to
 *     determine the language to use for error messages and instructions, collating
 *     sequences, date formats, and so on.
 * LC_ALL
 *     This variable shall determine the values for all locale categories. The
 *     value of the LC_ALL environment variable has precedence over any of the
 *     other environment variables starting with LC_ ( LC_COLLATE , LC_CTYPE ,
 *     LC_MESSAGES , LC_MONETARY , LC_NUMERIC , LC_TIME ) and the LANG environment
 *     variable.
 * LC_COLLATE
 *     This variable shall determine the locale category for character collation.
 *     It determines collation information for regular expressions and sorting,
 *     including equivalence classes and multi-character collating elements, in
 *     various utilities and the strcoll() and strxfrm() functions. Additional
 *     semantics of this variable, if any, are implementation-defined.
 * LC_CTYPE
 *     This variable shall determine the locale category for character handling
 *     functions, such as tolower(), toupper(), and isalpha(). This environment
 *     variable determines the interpretation of sequences of bytes of text data
 *     as characters (for example, single as opposed to multi-byte characters),
 *     the classification of characters (for example, alpha, digit, graph), and
 *     the behavior of character classes. Additional semantics of this variable,
 *    if any, are implementation-defined.
 * LC_MESSAGES
 *     This variable shall determine the locale category for processing affirmative
 *     and negative responses and the language and cultural conventions in which
 *     messages should be written. [XSI] [Option Start]  It also affects the behavior
 *     of the catopen() function in determining the message catalog. [Option End]
 *     Additional semantics of this variable, if any, are implementation-defined.
 *     The language and cultural conventions of diagnostic and informative messages
 *     whose format is unspecified by IEEE Std 1003.1-2001 should be affected by
 *     the setting of LC_MESSAGES .
 * LC_MONETARY
 *     This variable shall determine the locale category for monetary-related
 *     numeric formatting information. Additional semantics of this variable, if
 *     any, are implementation-defined.
 * LC_NUMERIC
 *     This variable shall determine the locale category for numeric formatting
 *     (for example, thousands separator and radix character) information in
 *     various utilities as well as the formatted I/O operations in printf() and
 *     scanf() and the string conversion functions in strtod(). Additional semantics
 *     of this variable, if any, are implementation-defined.
 * LC_TIME
 *     This variable shall determine the locale category for date and time formatting
 *     information. It affects the behavior of the time functions in strftime().
 *     Additional semantics of this variable, if any, are implementation-defined.
 *
 *
 * The values of locale categories shall be determined by a precedence order; the
 * first condition met below determines the value:
 *
 *    1. If the LC_ALL environment variable is defined and is not null, the value
 *       of LC_ALL shall be used.
 *    2. If the LC_* environment variable ( LC_COLLATE , LC_CTYPE , LC_MESSAGES ,
 *       LC_MONETARY , LC_NUMERIC , LC_TIME ) is defined and is not null, the value
 *       of the environment variable shall be used to initialize the category that
 *       corresponds to the environment variable.
 *    3. If the LANG environment variable is defined and is not null, the value of
 *       the LANG environment variable shall be used.
 *    4. If the LANG environment variable is not set or is set to the empty string,
 *       the implementation-defined default locale shall be used.
 *
 */
const gchar *get_locale (void)
{
    const gchar *loc;
    
    if ((loc = g_getenv("LC_ALL")) && *loc)
        return loc;

    else if ((loc = g_getenv("LC_CTYPE")) && *loc)
        return loc;

    else if ((loc = g_getenv("LANG")) && *loc)
        return loc;

     else
         return NULL;
}



/*
 * convert_string : (don't use with UTF-16 strings)
 *  - display_error : if TRUE, may return an escaped string and display an error
 *                    message (if conversion fails).
 */
gchar *convert_string (const gchar *string, const gchar *from_codeset,
                       const gchar *to_codeset, const gboolean display_error)
{
    return convert_string_1(string, -1, from_codeset, to_codeset, display_error);
}

/* Length must be passed, as the string might be Unicode, in which case we can't
 * count zeroes (see strlen call below). */
gchar *
convert_string_1 (const gchar *string, gssize length, const gchar *from_codeset,
                         const gchar *to_codeset, const gboolean display_error)
{
    gchar *output;
    GError *error = NULL;
    gsize bytes_written;

    g_return_val_if_fail (string != NULL, NULL);

    output = g_convert(string, length, to_codeset, from_codeset, NULL, &bytes_written, &error);
    //output = g_convert_with_fallback(string, length, to_codeset, from_codeset, "?", NULL, &bytes_written, &error);

    if (output == NULL)
    {
        gchar *escaped_str = g_strescape(string, NULL);
        if (display_error)
        {
            Log_Print(LOG_ERROR,"convert_string(): Failed conversion from charset '%s' to '%s'. "
                      "String '%s'. Errcode %d (%s).",
                      from_codeset, to_codeset, escaped_str, error->code, error->message);
        }
        g_free(escaped_str);
        g_error_free(error);
        // Return the input string without converting it. If the string is
        // displayed in the UI, it must be in UTF-8!
        if ( (g_ascii_strcasecmp(to_codeset, "UTF-8"))
        ||   (g_utf8_validate(string, -1, NULL)) )
        {
            return g_strdup(string);
        }
    }else
    {
        // Patch from Alexey Illarionov:
        //    g_convert returns null-terminated string only with one \0 at the
        // end. It can cause some garbage at the end of a string for UTF-16.
        // The second \0 should be set manually.
        gchar *new_output;
        new_output = g_realloc (output, bytes_written + 2);
        if (new_output != NULL)
        {
            output = new_output;
            output[bytes_written] = output[bytes_written + 1] = 0;
        }
    }

    //g_print("from %s => len: %d, string: '%s'\n     (%x %x %x %x %x %x %x %x)\n",from_codeset,length,string,string[0],string[1],string[2],string[3],string[4],string[5],string[6],string[7]);
    //g_print("to   %s => len: %d, output: '%s'\n     (%x %x %x %x %x %x %x %x)\n\n",to_codeset,bytes_written+2,output,output[0],output[1],output[2],output[3],output[4],output[5],output[6],output[7]);

    return output;
}

/*
 * filename_from_display:
 * @string: a UTF-8 string
 *
 * Convert a string from UTF-8 to the filesystem encoding.
 *
 * Returns: a newly-allocated filename in the GLib filename encoding on
 *          success, or an escaped ASCII string on error
 */
gchar *
filename_from_display (const gchar *string)
{
    GError *error = NULL;
    gchar *ret = NULL;
    const gchar **filename_encodings;

    g_return_val_if_fail (string != NULL, NULL);
    g_return_val_if_fail (g_utf8_validate (string, -1, NULL), NULL);

    ret = g_filename_from_utf8 (string, -1, NULL, NULL, &error);

    if (!ret)
    {
        g_debug ("Error while converting filename from display to GLib encoding: %s",
                 error->message);
        g_clear_error (&error);
    }
    else
    {
        return ret;
    }

    /* If the target encoding is not UTF-8, try the user-chosen alternative. */
    if (!g_get_filename_charsets (&filename_encodings))
    {
        EtRenameEncoding enc_option = g_settings_get_enum (MainSettings,
                                                           "rename-encoding");

        switch (enc_option)
        {
            case ET_RENAME_ENCODING_TRY_ALTERNATIVE:
                /* Already called g_filename_from_utf8(). */
                break;
            case ET_RENAME_ENCODING_TRANSLITERATE:
            {
                ret = g_str_to_ascii (string, NULL);
                break;
            }
            case ET_RENAME_ENCODING_IGNORE:
            {
                /* iconv_open (3):
                 * When the string "//IGNORE" is appended to tocode, characters
                 * that cannot be represented in the target character set will
                 * be silently discarded.
                 */
                gchar *enc = g_strconcat (*filename_encodings, "//IGNORE", NULL);
                ret = g_convert (string, -1, enc, "UTF-8", NULL, NULL, &error);

                if (!ret)
                {
                    g_debug ("Error while converting filename from display to encoding with ignored failures '%s': %s",
                             enc, error->message);
                    g_clear_error (&error);
                }

                g_free (enc);
                break;
            }
            default:
                g_assert_not_reached ();
        }
    }

    /* Try alternative encodings. */
    if (!ret)
    {
        const gchar *legacy_encoding;

        /* Guess the legacy (pre-Unicode) filesystem encoding from the locale.
         * For example, fr_FR.UTF-8 => fr_FR => ISO-8859-1. */
        legacy_encoding = get_encoding_from_locale (get_locale ());
        ret = g_convert (string, -1, legacy_encoding, "UTF-8", NULL, NULL,
                         &error);

        if (!ret)
        {
            g_debug ("Error while converting filename from display to legacy encoding '%s': %s",
                     legacy_encoding, error->message);
            g_clear_error (&error);
        }
    }

    if (!ret)
    {
        /* Failing that, try ISO-8859-1. */
        ret = g_convert (string, -1, "ISO-8859-1", "UTF-8", NULL, NULL,
                         &error);

        if (!ret)
        {
            g_debug ("Error while converting filename from display to ISO-8859-1: %s",
                     error->message);
            g_clear_error (&error);
        }
    }

    /* If all conversions fail, return an escaped version of the supplied UTF-8
     * string. */
    if (!ret)
    {
        gchar *escaped_str = g_strescape (string, NULL);

        /* TODO: Improve error string. */
        Log_Print (LOG_ERROR,
                   _("The UTF-8 string ‘%s’ could not be converted into filename encoding: %s"),
                   string, _("Invalid UTF-8"));

        ret = escaped_str;
    }

    return ret;
}

/*
 * Try_To_Validate_Utf8_String:
 * @string: a string in unknown encoding
 *
 * Validate that @string is in UTF-8 encoding, or try to convert it to be so.
 * Several alternative encodings are attempted, based on the current locale and
 * some hardcoded fallbacks, before falling back to escaping the string.
 *
 * Returns: a newly-allocated UTF-8 encoded string
 */
gchar *
Try_To_Validate_Utf8_String (const gchar *string)
{
    gchar *ret = NULL;
    GError *error = NULL;

    g_return_val_if_fail (string != NULL, NULL);

    if (g_utf8_validate (string, -1, NULL))
    {
        /* String already in UTF-8. */
        ret = g_strdup (string);
    }
    else
    {
        const gchar *legacy_encoding;

        /* Guess the legacy (pre-Unicode) encoding associated with the locale.
         * For example, fr_FR.UTF-8 => fr_FR => ISO-8859-1. */
        legacy_encoding = get_encoding_from_locale (get_locale ());
        ret = g_convert (string, -1, "UTF-8", legacy_encoding, NULL, NULL,
                         &error);

        if (!ret)
        {
            /* Failing that, try ISO-8859-1. */
            g_debug ("Error converting string to legacy encoding '%s': %s",
                     legacy_encoding, error->message);
            g_clear_error (&error);
            ret = g_convert (string, -1, "UTF-8", "ISO-8859-1", NULL, NULL,
                             &error);
        }

        if (!ret)
        {
            gchar *escaped_str = g_strescape (string, NULL);

            /* TODO: Improve error string. */
            Log_Print (LOG_ERROR,
                       _("The string ‘%s’ could not be converted into UTF-8: %s"),
                       escaped_str, error->message);
            g_clear_error (&error);

            ret = escaped_str;
        }
    }

    return ret;
}

void
Charset_Populate_Combobox (GtkComboBox *combo, gint select_charset)
{
    gsize i;

    for (i = 0; i < CHARSET_TRANS_ARRAY_LEN; i++)
    {
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo),
                                        _(charset_trans_array[i].charset_title));

    }

    gtk_combo_box_set_active (combo, select_charset);
}

const gchar *
et_charset_get_name_from_index (guint index)
{
    g_return_val_if_fail (index <= CHARSET_TRANS_ARRAY_LEN, NULL);

    return charset_trans_array[index].charset_name;
}
