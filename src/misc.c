/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014-2015  David King <amigadave@amigadave.com>
 * Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include "misc.h"

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "easytag.h"
#include "id3_tag.h"
#include "browser.h"
#include "setting.h"
#include "preferences_dialog.h"

#ifdef G_OS_WIN32
#include <windows.h>
#endif /* G_OS_WIN32 */


/*
 * Add the 'string' passed in parameter to the list store
 * If this string already exists in the list store, it doesn't add it.
 * Returns TRUE if string was added.
 */
gboolean Add_String_To_Combo_List (GtkListStore *liststore, const gchar *str)
{
    GtkTreeIter iter;
    gchar *text;
    const gint HISTORY_MAX_LENGTH = 15;
    //gboolean found = FALSE;
    gchar *string = g_strdup(str);

    if (et_str_empty (string))
    {
        g_free (string);
        return FALSE;
    }

#if 0
    // We add the string to the beginning of the list store
    // So we will start to parse from the second line below
    gtk_list_store_prepend(liststore, &iter);
    gtk_list_store_set(liststore, &iter, MISC_COMBO_TEXT, string, -1);

    // Search in the list store if string already exists and remove other same strings in the list
    found = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(liststore), &iter);
    //gtk_tree_model_get(GTK_TREE_MODEL(liststore), &iter, MISC_COMBO_TEXT, &text, -1);
    while (found && gtk_tree_model_iter_next(GTK_TREE_MODEL(liststore), &iter))
    {
        gtk_tree_model_get(GTK_TREE_MODEL(liststore), &iter, MISC_COMBO_TEXT, &text, -1);
        //g_print(">0>%s\n>1>%s\n",string,text);
        if (g_utf8_collate(text, string) == 0)
        {
            g_free(text);
            // FIX ME : it seems that after it selects the next item for the
            // combo (changes 'string')????
            // So should select the first item?
            gtk_list_store_remove(liststore, &iter);
            // Must be rewinded?
            found = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(liststore), &iter);
            //gtk_tree_model_get(GTK_TREE_MODEL(liststore), &iter, MISC_COMBO_TEXT, &text, -1);
            continue;
        }
        g_free(text);
    }

    // Limit list size to HISTORY_MAX_LENGTH
    while (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(liststore),NULL) > HISTORY_MAX_LENGTH)
    {
        if ( gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(liststore),
                                           &iter,NULL,HISTORY_MAX_LENGTH) )
        {
            gtk_list_store_remove(liststore, &iter);
        }
    }

    g_free(string);
    // Place again to the beginning of the list, to select the right value?
    //gtk_tree_model_get_iter_first(GTK_TREE_MODEL(liststore), &iter);

    return TRUE;

#else

    // Search in the list store if string already exists.
    // FIXME : insert string at the beginning of the list (if already exists),
    //         and remove other same strings in the list
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(liststore), &iter))
    {
        do
        {
            gtk_tree_model_get(GTK_TREE_MODEL(liststore), &iter, MISC_COMBO_TEXT, &text, -1);
            if (g_utf8_collate(text, string) == 0)
            {
                g_free (string);
                g_free(text);
                return FALSE;
            }

            g_free(text);
        } while(gtk_tree_model_iter_next(GTK_TREE_MODEL(liststore), &iter));
    }

    /* We add the string to the beginning of the list store. */
    gtk_list_store_insert_with_values (liststore, &iter, 0, MISC_COMBO_TEXT,
                                       string, -1);

    // Limit list size to HISTORY_MAX_LENGTH
    while (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(liststore),NULL) > HISTORY_MAX_LENGTH)
    {
        if ( gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(liststore),
                                           &iter,NULL,HISTORY_MAX_LENGTH) )
        {
            gtk_list_store_remove(liststore, &iter);
        }
    }

    g_free(string);
    return TRUE;
#endif
}

/*
 * Run a program with a list of parameters
 *  - args_list : list of filename (with path)
 */
gboolean
et_run_program (const gchar *program_name,
                GList *args_list,
                GError **error)
{
    gchar *program_tmp;
    const gchar *program_args;
    gchar **program_args_argv = NULL;
    guint n_program_args = 0;
    gsize i;
    gchar **argv;
    GSubprocess *subprocess;
    GList *l;
    gchar *program_path;
    gboolean res = FALSE;

    g_return_val_if_fail (program_name != NULL && args_list != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    /* Check if a name for the program has been supplied */
    if (!*program_name)
    {
        GtkWidget *msgdialog;

        msgdialog = gtk_message_dialog_new(GTK_WINDOW(MainWindow),
                                           GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_OK,
                                           "%s",
                                           _("You must type a program name"));
        gtk_window_set_title(GTK_WINDOW(msgdialog),_("Program Name Error"));

        gtk_dialog_run(GTK_DIALOG(msgdialog));
        gtk_widget_destroy(msgdialog);
        return res;
    }

    /* If user arguments are included, try to skip them. FIXME: This works
     * poorly when there are spaces in the absolute path to the binary. */
    program_tmp = g_strdup (program_name);

    /* Skip the binary name and a delimiter. */
#ifdef G_OS_WIN32
    /* FIXME: Should also consider .com, .bat, .sys. See
     * g_find_program_in_path(). */
    if ((program_args = strstr (program_tmp, ".exe")))
    {
        /* Skip ".exe". */
        program_args += 4;
    }
#else /* !G_OS_WIN32 */
    /* Remove arguments if found. */
    program_args = strchr (program_tmp, ' ');
#endif /* !G_OS_WIN32 */

    if (program_args && *program_args)
    {
        size_t len;

        len = program_args - program_tmp;
        program_path = g_strndup (program_name, len);

        /* FIXME: Splitting arguments based on a delimiting space is bogus
         * if the arguments have been quoted. */
        program_args_argv = g_strsplit (program_args, " ", 0);
        n_program_args = g_strv_length (program_args_argv);
    }
    else
    {
        n_program_args = 1;
        program_path = g_strdup (program_name);
    }

    g_free (program_tmp);

    /* +1 for NULL, program_name is already included in n_program_args. */
    argv = g_new0 (gchar *, n_program_args + g_list_length (args_list) + 1);

    argv[0] = program_path;

    if (program_args_argv)
    {
        /* Skip program_args_argv[0], which is " ". */
        for (i = 1; program_args_argv[i] != NULL; i++)
        {
            argv[i] = program_args_argv[i];
        }
    }
    else
    {
        i = 1;
    }

    /* Load arguments from 'args_list'. */
    for (l = args_list; l != NULL; l = g_list_next (l), i++)
    {
        argv[i] = (gchar *)l->data;
    }

    argv[i] = NULL;

    /* Execution ... */
    if ((subprocess = g_subprocess_newv ((const gchar * const *)argv,
                                         G_SUBPROCESS_FLAGS_NONE, error)))
    {
        res = TRUE;
        /* There is no need to watch to see if the child process exited. */
        g_object_unref (subprocess);
    }

    g_strfreev (program_args_argv);
    g_free (program_path);
    g_free (argv);

    return res;
}

gboolean
et_run_audio_player (GList *files,
                     GError **error)
{
    GFileInfo *info;
    const gchar *content_type;
    GAppInfo *app_info;
    GdkAppLaunchContext *context;

    g_return_val_if_fail (files != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    info = g_file_query_info (files->data,
                              G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                              G_FILE_QUERY_INFO_NONE, NULL, error);

    if (info == NULL)
    {
        return FALSE;
    }

    content_type = g_file_info_get_content_type (info);
    app_info = g_app_info_get_default_for_type (content_type, FALSE);
    g_object_unref (info);

    context = gdk_display_get_app_launch_context (gdk_display_get_default ());

    if (!g_app_info_launch (app_info, files, G_APP_LAUNCH_CONTEXT (context),
                            error))
    {
        g_object_unref (context);
        g_object_unref (app_info);

        return FALSE;
    }

    g_object_unref (context);
    g_object_unref (app_info);

    return TRUE;
}

/*
 * Convert a series of seconds into a readable duration
 * Remember to free the string that is returned
 */
gchar *Convert_Duration (gulong duration)
{
    guint hour=0;
    guint minute=0;
    guint second=0;
    gchar *data = NULL;

    if (duration == 0)
    {
        return g_strdup_printf ("%u:%.2u", minute, second);
    }

    hour   = duration/3600;
    minute = (duration%3600)/60;
    second = (duration%3600)%60;

    if (hour)
    {
        data = g_strdup_printf ("%u:%.2u:%.2u", hour, minute, second);
    }
    else
    {
        data = g_strdup_printf ("%u:%.2u", minute, second);
    }

    return data;
}

gchar *
et_disc_number_to_string (const guint disc_number)
{
    if (g_settings_get_boolean (MainSettings, "tag-disc-padded"))
    {
        return g_strdup_printf ("%.*u",
                                (gint)g_settings_get_uint (MainSettings,
                                                           "tag-disc-length"),
                                disc_number);
    }

    return g_strdup_printf ("%u", disc_number);
}

gchar *
et_track_number_to_string (const guint track_number)
{
    if (g_settings_get_boolean (MainSettings, "tag-number-padded"))
    {
        return g_strdup_printf ("%.*u",
                                (gint)g_settings_get_uint (MainSettings,
                                                           "tag-number-length"),
                                track_number);
    }
    else
    {
        return g_strdup_printf ("%u", track_number);
    }
}

/*
 * et_rename_file:
 * @old_filepath: path of file to be renamed
 * @new_filepath: path of renamed file
 * @error: a #GError to provide information on errors, or %NULL to ignore
 *
 * Rename @old_filepath to @new_filepath.
 *
 * Returns: %TRUE if the rename was successful, %FALSE otherwise
 */
gboolean
et_rename_file (const char *old_filepath,
                const char *new_filepath,
                GError **error)
{
    GFile *file_old;
    GFile *file_new;
    GFile *file_new_parent;

    g_return_val_if_fail (old_filepath != NULL && new_filepath != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    file_old = g_file_new_for_path (old_filepath);
    file_new = g_file_new_for_path (new_filepath);
    file_new_parent = g_file_get_parent (file_new);

    if (!g_file_make_directory_with_parents (file_new_parent, NULL, error))
    {
        /* Ignore an error if the directory already exists. */
        if (!g_error_matches (*error, G_IO_ERROR, G_IO_ERROR_EXISTS))
        {
            g_object_unref (file_new_parent);
            goto err;
        }

        g_clear_error (error);
    }

    g_assert (error == NULL || *error == NULL);
    g_object_unref (file_new_parent);

    /* Move the file. */
    if (!g_file_move (file_old, file_new, G_FILE_COPY_NONE, NULL, NULL, NULL,
                      error))
    {
        if (g_error_matches (*error, G_IO_ERROR, G_IO_ERROR_EXISTS))
        {
            /* Possibly a case change on a case-insensitive filesystem. */
            /* TODO: casefold the paths of both files, and check to see whether
             * they only differ by case? */
            gchar *tmp_filename;
            mode_t old_mode;
            gint fd;
            GFile *tmp_file;
            GError *tmp_error = NULL;

            tmp_filename = g_strconcat (old_filepath, ".XXXXXX", NULL);

            old_mode = umask (077);
            fd = g_mkstemp (tmp_filename);
            umask (old_mode);

            if (fd >= 0)
            {
                /* TODO: Handle error. */
                g_close (fd, NULL);
            }

            tmp_file = g_file_new_for_path (tmp_filename);
            g_free (tmp_filename);

            if (!g_file_move (file_old, tmp_file, G_FILE_COPY_OVERWRITE, NULL,
                              NULL, NULL, &tmp_error))
            {
                g_file_delete (tmp_file, NULL, NULL);

                g_object_unref (tmp_file);
                g_clear_error (error);
                g_propagate_error (error, tmp_error);
                goto err;
            }
            else
            {
                /* Move to temporary file succeeded, now move to the real new
                 * location. */
                if (!g_file_move (tmp_file, file_new, G_FILE_COPY_NONE, NULL,
                                  NULL, NULL, &tmp_error))
                {
                    g_file_move (tmp_file, file_old, G_FILE_COPY_NONE, NULL,
                                 NULL, NULL, NULL);
                    g_object_unref (tmp_file);
                    g_clear_error (error);
                    g_propagate_error (error, tmp_error);
                    goto err;
                }
                else
                {
                    /* Move succeeded, so clear the original error about the
                     * new file already existing. */
                    g_object_unref (tmp_file);
                    g_clear_error (error);
                    goto out;
                }
            }
        }
        else
        {
            /* Error moving file. */
            goto err;
        }
    }

out:
    g_object_unref (file_old);
    g_object_unref (file_new);
    g_assert (error == NULL || *error == NULL);
    return TRUE;

err:
    g_object_unref (file_old);
    g_object_unref (file_new);
    g_assert (error == NULL || *error != NULL);
    return FALSE;
}

/*
 * et_filename_prepare:
 * @filename_utf8: UTF8-encoded basename
 * @replace_illegal: whether to replace illegal characters in the file name
 *
 * Used to replace (in place) the illegal characters in the filename.
 */
void
et_filename_prepare (gchar *filename_utf8,
                     gboolean replace_illegal)
{
    gchar *character;

    g_return_if_fail (filename_utf8 != NULL);

    // Convert automatically the directory separator ('/' on LINUX and '\' on WIN32) to '-'.
    while ((character = strchr (filename_utf8, G_DIR_SEPARATOR)) != NULL)
    {
        *character = '-';
    }

#ifdef G_OS_WIN32
    /* Convert character '/' on WIN32 to '-'. May be converted to '\' after. */
    while ((character = strchr (filename_utf8, '/')) != NULL)
    {
        *character = '-';
    }
#endif /* G_OS_WIN32 */

    /* Convert other illegal characters on FAT32/16 filesystems and ISO9660 and
     * Joliet (CD-ROM filesystems). */
    if (replace_illegal)
    {
        size_t last;

        while ((character = strchr (filename_utf8, ':')) != NULL)
        {
            *character = '-';
        }
        while ((character = strchr (filename_utf8, '*')) != NULL)
        {
            *character = '+';
        }
        while ((character = strchr (filename_utf8, '?')) != NULL)
        {
            *character = '_';
        }
        while ((character = strchr (filename_utf8, '\"')) != NULL)
        {
            *character = '\'';
        }
        while ((character = strchr (filename_utf8, '<')) != NULL)
        {
            *character = '(';
        }
        while ((character = strchr (filename_utf8, '>')) != NULL)
        {
            *character = ')';
        }
        while ((character = strchr (filename_utf8, '|')) != NULL)
        {
            *character = '-';
        }

        /* FAT has additional restrictions on the last character of a filename.
         * https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247%28v=vs.85%29.aspx#naming_conventions */
        last = strlen (filename_utf8) - 1;

        if (filename_utf8[last] == ' ' || filename_utf8[last] == '.')
        {
            filename_utf8[last] = '_';
        }
    }
}

/* Key for Undo */
guint
et_undo_key_new (void)
{
    static guint ETUndoKey = 0;
    return ++ETUndoKey;
}

/*
 * et_normalized_strcmp0:
 * @str1: UTF-8 string, or %NULL
 * @str2: UTF-8 string to compare against, or %NULL
 *
 * Compare two UTF-8 strings, normalizing them before doing so, and return the
 * difference.
 *
 * Returns: an integer less than, equal to, or greater than zero, if str1 is <,
 * == or > than str2
 */
gint
et_normalized_strcmp0 (const gchar *str1,
                       const gchar *str2)
{
    gint result;
    gchar *normalized1;
    gchar *normalized2;

    /* Check for NULL, as it cannot be passed to g_utf8_normalize(). */
    if (!str1)
    {
        return -(str1 != str2);
    }

    if (!str2)
    {
        return str1 != str2;
    }

    normalized1 = g_utf8_normalize (str1, -1, G_NORMALIZE_DEFAULT);
    normalized2 = g_utf8_normalize (str2, -1, G_NORMALIZE_DEFAULT);

    result = g_strcmp0 (normalized1, normalized2);

    g_free (normalized1);
    g_free (normalized2);

    return result;
}

/*
 * et_normalized_strcasecmp0:
 * @str1: UTF-8 string, or %NULL
 * @str2: UTF-8 string to compare against, or %NULL
 *
 * Compare two UTF-8 strings, normalizing them before doing so, in a
 * case-insensitive manner.
 *
 * Returns: an integer less than, equal to, or greater than zero, if str1 is
 * less than, equal to or greater than str2
 */
gint
et_normalized_strcasecmp0 (const gchar *str1,
                           const gchar *str2)
{
    gint result;
    gchar *casefolded1;
    gchar *casefolded2;

    /* Check for NULL, as it cannot be passed to g_utf8_casefold(). */
    if (!str1)
    {
        return -(str1 != str2);
    }

    if (!str2)
    {
        return str1 != str2;
    }

    /* The strings are automatically normalized during casefolding. */
    casefolded1 = g_utf8_casefold (str1, -1);
    casefolded2 = g_utf8_casefold (str2, -1);

    result = g_utf8_collate (casefolded1, casefolded2);

    g_free (casefolded1);
    g_free (casefolded2);

    return result;
}

/*
 * et_str_empty:
 * @str: string to test for emptiness
 *
 * Test if @str is empty, in other words either %NULL or the empty string.
 *
 * Returns: %TRUE is @str is either %NULL or "", %FALSE otherwise
 */
gboolean
et_str_empty (const gchar *str)
{
    return !str || !str[0];
}
