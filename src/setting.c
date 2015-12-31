/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
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

#include <glib/gi18n.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include "setting.h"
#include "application_window.h"
#include "cddb_dialog.h"
#include "load_files_dialog.h"
#include "playlist_dialog.h"
#include "preferences_dialog.h"
#include "search_dialog.h"
#include "easytag.h"
#include "charset.h"
#include "scan_dialog.h"
#include "log.h"
#include "misc.h"
#include "browser.h"
#include "et_core.h"

#include "win32/win32dep.h"

/* Referenced in the header. */
GSettings *MainSettings;

/***************
 * Declaration *
 ***************/

/*
 * Nota :
 *  - no trailing slashes on directory name to avoid problem with
 *    NetBSD's mkdir(2).
 */

// File of masks for tag scanner
static const gchar SCAN_TAG_MASKS_FILE[] = "scan_tag.mask";
// File of masks for rename file scanner
static const gchar RENAME_FILE_MASKS_FILE[] = "rename_file.mask";
// File for history of BrowserEntry combobox
static const gchar PATH_ENTRY_HISTORY_FILE[] = "browser_path.history";
// File for history of run program combobox for directories
static const gchar RUN_PROGRAM_WITH_DIRECTORY_HISTORY_FILE[] = "run_program_with_directory.history";
// File for history of run program combobox for files
static const gchar RUN_PROGRAM_WITH_FILE_HISTORY_FILE[] = "run_program_with_file.history";
// File for history of search string combobox
static const gchar SEARCH_FILE_HISTORY_FILE[] = "search_file.history";



/**************
 * Prototypes *
 **************/

static gboolean Create_Easytag_Directory (void);

/*************
 * Functions *
 *************/

static void
check_default_path (void)
{
    GVariant *default_path;
    const gchar *path;

    default_path = g_settings_get_value (MainSettings, "default-path");
    path = g_variant_get_bytestring (default_path);

    if (!*path)
    {
        path = g_get_user_special_dir (G_USER_DIRECTORY_MUSIC);
        g_settings_set_value (MainSettings, "default-path",
                              g_variant_new_bytestring (path ? path
                                                             : g_get_home_dir ()));
    }

    g_variant_unref (default_path);
}

/*
 * Define and Load default values into config variables
 */
void Init_Config_Variables (void)
{
    MainSettings = g_settings_new ("org.gnome.EasyTAG");

    /*
     * Common
     */
    check_default_path ();
}

/*
 * check_or_create_file:
 * @filename: (type filename): the filename to create
 *
 * Check that the provided @filename exists, and if not, create it.
 */
static void
check_or_create_file (const gchar *filename)
{
    gchar *file_path;
    GFile *file;
    GFileOutputStream *ostream;
    GError *error = NULL;

    g_return_if_fail (filename != NULL);

    file_path = g_build_filename (g_get_user_config_dir (), PACKAGE_TARNAME,
                                  filename, NULL);

    file = g_file_new_for_path (file_path);

    if (!(ostream = g_file_append_to (file, G_FILE_CREATE_NONE, NULL, &error)))
    {
        g_debug ("Cannot create or open file ‘%s’: %s", file_path,
                 error->message);
        g_error_free (error);
    }
    else
    {
        g_object_unref (ostream);
    }

    g_free (file_path);
    g_object_unref (file);
}

/*
 * Create the main directory with empty history files
 */
gboolean Setting_Create_Files (void)
{
    /* The file to write */
    if (!Create_Easytag_Directory ())
    {
        return FALSE;
    }

    check_or_create_file (SCAN_TAG_MASKS_FILE);
    check_or_create_file (RENAME_FILE_MASKS_FILE);
    check_or_create_file (PATH_ENTRY_HISTORY_FILE);
    check_or_create_file (RUN_PROGRAM_WITH_DIRECTORY_HISTORY_FILE);
    check_or_create_file (RUN_PROGRAM_WITH_FILE_HISTORY_FILE);
    check_or_create_file (SEARCH_FILE_HISTORY_FILE);

    return TRUE;
}



/*
 * Save the contents of a list store to a file
 */
static void
Save_List_Store_To_File (const gchar *filename,
                         GtkListStore *liststore,
                         gint colnum)
{
    gchar *file_path;
    gchar *display_path;
    GFile *file;
    GFileOutputStream *ostream;
    GtkTreeIter iter;
    GError *error = NULL;

    if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (liststore), &iter)
        || !Create_Easytag_Directory ())
    {
        return;
    }

    /* The file to write */
    file_path = g_build_filename (g_get_user_config_dir (), PACKAGE_TARNAME,
                                  filename, NULL);
    file = g_file_new_for_path (file_path);
    g_free (file_path);

    if (!(ostream = g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE, NULL,
                                    &error)))
    {
        goto err;
    }
    else
    {
        GString *data;
        gsize bytes_written;

        data = g_string_new ("");

        do
        {
            gchar *text;

            gtk_tree_model_get (GTK_TREE_MODEL (liststore), &iter, colnum,
                                &text, -1);
            g_string_append (data, text);
            g_free (text);
            g_string_append_c (data, '\n');
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (liststore), &iter));

        if (!g_output_stream_write_all (G_OUTPUT_STREAM (ostream), data->str,
                                        data->len, &bytes_written, NULL,
                                        &error))
        {
            g_string_free (data, TRUE);
            g_object_unref (ostream);
            goto err;
        }

        g_string_free (data, TRUE);
        g_object_unref (ostream);
    }

    g_object_unref (file);
    return;

err:
    file_path = g_file_get_path (file);
    display_path = g_filename_display_name (file_path);
    Log_Print (LOG_ERROR, _("Cannot write list to file ‘%s’: %s"),
               display_path, error->message);

    g_error_free (error);
    g_free (display_path);
    g_free (file_path);
    g_object_unref (file);
    return;
}

/*
 * Populate a list store with data from a file passed in as first parameter
 */
static gboolean
Populate_List_Store_From_File (const gchar *filename,
                               GtkListStore *liststore,
                               gint text_column)
{
    gchar *file_path;
    GFile *file;
    GFileInputStream *istream;
    gboolean entries_set = FALSE;
    GError *error = NULL;

    /* The file to write */
    g_return_val_if_fail (filename != NULL, FALSE);

    file_path = g_build_filename (g_get_user_config_dir (), PACKAGE_TARNAME,
                                  filename, NULL);
    file = g_file_new_for_path (file_path);
    g_free (file_path);

    istream = g_file_read (file, NULL, &error);

    if (!istream)
    {
        goto err;
    }
    else
    {
        GDataInputStream *data;
        gsize bytes_read;
        gchar *line;

        data = g_data_input_stream_new (G_INPUT_STREAM (istream));
        /* TODO: Find a safer alternative to _ANY. */
        g_data_input_stream_set_newline_type (data,
                                              G_DATA_STREAM_NEWLINE_TYPE_ANY);

        while ((line = g_data_input_stream_read_line (data, &bytes_read, NULL,
                                                      &error)))
        {
            gchar *utf8_line;

            utf8_line = Try_To_Validate_Utf8_String (line);
            g_free (line);

            if (!et_str_empty (utf8_line))
            {
                gtk_list_store_insert_with_values (liststore, NULL, G_MAXINT,
                                                   text_column, utf8_line, -1);
                entries_set = TRUE;
            }

            g_free (utf8_line);
        }

        g_object_unref (data);

        if (error)
        {
            g_object_unref (istream);
            goto err;
        }
    }

    g_object_unref (istream);
    g_object_unref (file);

    return entries_set;

err:
    file_path = g_file_get_path (file);
    Log_Print (LOG_ERROR, _("Cannot open file ‘%s’: %s"), file_path,
               error->message);

    g_free (file_path);
    g_error_free (error);
    g_object_unref (file);
    return entries_set;
}


/*
 * Functions for writing and reading list of 'Fill Tag' masks
 */
void
Load_Scan_Tag_Masks_List (GtkListStore *liststore, gint colnum,
                          const gchar * const *fallback)
{
    gsize i = 0;
    GtkTreeIter iter;

    if (!Populate_List_Store_From_File(SCAN_TAG_MASKS_FILE, liststore, colnum))
    {
        /* Fall back to defaults. */
        Log_Print (LOG_OK, _("Loading default ‘Fill Tag’ masks…"));

        while(fallback[i])
        {
            gtk_list_store_insert_with_values (liststore, &iter, G_MAXINT,
                                               colnum, fallback[i], -1);
            i++;
        }
    }
}

void Save_Scan_Tag_Masks_List (GtkListStore *liststore, gint colnum)
{
    Save_List_Store_To_File(SCAN_TAG_MASKS_FILE, liststore, colnum);
}


/*
 * Functions for writing and reading list of 'Rename File' masks
 */
void
Load_Rename_File_Masks_List (GtkListStore *liststore, gint colnum,
                             const gchar * const *fallback)
{
    gsize i = 0;
    GtkTreeIter iter;

    if (!Populate_List_Store_From_File(RENAME_FILE_MASKS_FILE, liststore, colnum))
    {
        /* Fall back to defaults. */
        Log_Print (LOG_OK, _("Loading default ‘Rename File’ masks…"));

        while(fallback[i])
        {
            gtk_list_store_insert_with_values (liststore, &iter, G_MAXINT,
                                               colnum, fallback[i], -1);
            i++;
        }
    }
}

void Save_Rename_File_Masks_List (GtkListStore *liststore, gint colnum)
{
    Save_List_Store_To_File(RENAME_FILE_MASKS_FILE, liststore, colnum);
}

/*
 * Functions for writing and reading list of 'BrowserEntry' combobox
 */
void Load_Path_Entry_List (GtkListStore *liststore, gint colnum)
{
    Populate_List_Store_From_File(PATH_ENTRY_HISTORY_FILE, liststore, colnum);
}
void Save_Path_Entry_List (GtkListStore *liststore, gint colnum)
{
    Save_List_Store_To_File(PATH_ENTRY_HISTORY_FILE, liststore, colnum);
}

/*
 * Functions for writing and reading list of combobox to run program (tree browser)
 */
void Load_Run_Program_With_Directory_List (GtkListStore *liststore, gint colnum)
{
    Populate_List_Store_From_File(RUN_PROGRAM_WITH_DIRECTORY_HISTORY_FILE, liststore, colnum);
}
void Save_Run_Program_With_Directory_List (GtkListStore *liststore, gint colnum)
{
    Save_List_Store_To_File(RUN_PROGRAM_WITH_DIRECTORY_HISTORY_FILE, liststore, colnum);
}

/*
 * Functions for writing and reading list of combobox to run program (file browser)
 */
void Load_Run_Program_With_File_List (GtkListStore *liststore, gint colnum)
{
    Populate_List_Store_From_File(RUN_PROGRAM_WITH_FILE_HISTORY_FILE, liststore, colnum);
}
void Save_Run_Program_With_File_List (GtkListStore *liststore, gint colnum)
{
    Save_List_Store_To_File(RUN_PROGRAM_WITH_FILE_HISTORY_FILE, liststore, colnum);
}

/*
 * Functions for writing and reading list of combobox to search a string into file (tag or filename)
 */
void Load_Search_File_List (GtkListStore *liststore, gint colnum)
{
    Populate_List_Store_From_File(SEARCH_FILE_HISTORY_FILE, liststore, colnum);
}
void Save_Search_File_List (GtkListStore *liststore, gint colnum)
{
    Save_List_Store_To_File(SEARCH_FILE_HISTORY_FILE, liststore, colnum);
}

/*
 * migrate_config_to_xdg_dir:
 * @old_path: (type filename): the path to migrate from
 * @new_path: (type filename): the path to migrate to
 *
 * Migrate the EasyTAG configuration files contained in the old path to the new
 * one.
 */
static void
migrate_config_file_dir (const gchar *old_path, const gchar *new_path)
{
    gsize i;
    static const gchar *filenames[] = { SCAN_TAG_MASKS_FILE,
                                        RENAME_FILE_MASKS_FILE,
                                        PATH_ENTRY_HISTORY_FILE,
                                        RUN_PROGRAM_WITH_DIRECTORY_HISTORY_FILE,
                                        RUN_PROGRAM_WITH_FILE_HISTORY_FILE,
                                        SEARCH_FILE_HISTORY_FILE,
                                        NULL
    };

    g_debug ("Migrating configuration from directory ‘%s’ to ‘%s’", old_path,
             new_path);

    for (i = 0; filenames[i]; i++)
    {
        gchar *old_filename, *new_filename;
        GFile *old_file, *new_file;

        old_filename = g_build_filename (old_path, filenames[i], NULL);

        if (!g_file_test (old_filename, G_FILE_TEST_EXISTS))
        {
            g_free (old_filename);
            continue;
        }

        new_filename = g_build_filename (new_path, filenames[i], NULL);
        old_file = g_file_new_for_path (old_filename);
        new_file = g_file_new_for_path (new_filename);

        if (!g_file_move (old_file, new_file, G_FILE_COPY_NONE, NULL, NULL,
                          NULL, NULL))
        {
            g_debug ("Failed to migrate configuration file ‘%s’",
                     filenames[i]);
        }

        g_free (old_filename);
        g_free (new_filename);
        g_object_unref (old_file);
        g_object_unref (new_file);
    }
}

/**
 * Create the directory used by EasyTAG to store user configuration files.
 *
 * Returns: %TRUE if the directory was created, or already exists. %FALSE if
 * the directory could not be created.
 */
static gboolean
Create_Easytag_Directory (void)
{
    gchar *easytag_path = NULL;
    gint result;

    /* Directory to create (if it does not exist) with absolute path. */
    easytag_path = g_build_filename (g_get_user_config_dir (), PACKAGE_TARNAME,
                                     NULL);

    if (g_file_test (easytag_path, G_FILE_TEST_IS_DIR))
    {
        g_free (easytag_path);
        return TRUE;
    }

    result = g_mkdir_with_parents (easytag_path, S_IRWXU);

    if (result == -1)
    {
        g_debug ("Cannot create directory ‘%s’: %s", easytag_path,
                 g_strerror (errno));
        g_free (easytag_path);
        return FALSE;
    }
    else
    {
        gchar *old_path = g_build_filename (g_get_home_dir (),
                                            "." PACKAGE_TARNAME, NULL);

        if (g_file_test (old_path, G_FILE_TEST_IS_DIR))
        {
            migrate_config_file_dir (old_path, easytag_path);
        }

        g_free (old_path);
        g_free (easytag_path);

        return TRUE;
    }
}

/*
 * et_settings_enum_get:
 * @value: the property value to be set (active item on combo box)
 * @variant: the variant to set the @value from
 * @user_data: the #GType of the #GSettings enum
 *
 * Wrapper function to convert an enum-type GSettings key into an integer
 * value.
 *
 * Returns: %TRUE if the mapping was successful, %FALSE otherwise
 */
gboolean
et_settings_enum_get (GValue *value, GVariant *variant, gpointer user_data)
{
    GType enum_type;
    GEnumClass *enum_class;
    GEnumValue *enum_value;
    const gchar *nick;

    g_return_val_if_fail (user_data != NULL, FALSE);

    enum_type = (GType)GPOINTER_TO_SIZE (user_data);
    enum_class = g_type_class_ref (enum_type);
    nick = g_variant_get_string (variant, NULL);
    enum_value = g_enum_get_value_by_nick (enum_class, nick);
    g_type_class_unref (enum_class);

    if (!enum_value)
    {
        g_warning ("Unable to lookup %s enum nick '%s' from GType",
                   g_type_name (enum_type),
                   nick);
        return FALSE;
    }

    g_value_set_int (value, enum_value->value);
    return TRUE;
}

/*
 * et_settings_enum_set:
 * @value: the property value to set the @variant from
 * @expected_type: the expected type of the returned variant
 * @user_data: the #GType of the #GSettings enum
 *
 * Wrapper function to convert an integer value into a string suitable for
 * storing into an enum-type GSettings key.
 *
 * Returns: a new GVariant containing the mapped value, or %NULL upon failure
 */
GVariant *
et_settings_enum_set (const GValue *value, const GVariantType *expected_type,
                      gpointer user_data)
{
    GType enum_type;
    GEnumClass *enum_class;
    GEnumValue *enum_value;

    g_return_val_if_fail (user_data != NULL, NULL);

    enum_type = (GType)GPOINTER_TO_SIZE (user_data);
    enum_class = g_type_class_ref (enum_type);
    enum_value = g_enum_get_value (enum_class, g_value_get_int (value));
    g_type_class_unref (enum_class);

    if (!enum_value)
    {
        g_warning ("Unable to lookup %s enum value '%d' from GType",
                   g_type_name (enum_type), g_value_get_int (value));
        return NULL;
    }

    return g_variant_new (g_variant_type_peek_string (expected_type),
                          enum_value->value_nick);
}

/*
 * et_settings_enum_radio_get:
 * @value: the property value to be set
 * @variant: the variant to set the @value from
 * @user_data: the widget on which the setting should be applied
 *
 * Wrapper function to convert an enum-type GSettings key state to the active
 * radio button.
 *
 * Returns: %TRUE
 */
gboolean
et_settings_enum_radio_get (GValue *value, GVariant *variant,
                            gpointer user_data)
{
    const gchar *name;
    const gchar *setting;

    name = gtk_widget_get_name (GTK_WIDGET (user_data));
    setting = g_variant_get_string (variant, NULL);

    /* Only set the radio button which matches the setting to active. */
    if (g_strcmp0 (name, setting) == 0)
    {
        g_value_set_boolean (value, TRUE);
    }

    return TRUE;
}

/*
 * et_settings_enum_radio_set:
 * @value: the property value to set the @variant from
 * @expected_type: the expected type of the returned variant
 * @user_data: the widget which the setting should be taken from
 *
 * Wrapper function to convert the active radiobutton to the value of an
 * enum-type GSettings key.
 *
 * Returns: a new GVariant containing the mapped value, or %NULL upon failure
 */
GVariant *
et_settings_enum_radio_set (const GValue *value,
                            const GVariantType *expected_type,
                            gpointer user_data)
{
    GVariant *variant = NULL;
    const gchar *name;

    /* Ignore buttons that are not active. */
    if (!g_value_get_boolean (value))
    {
        return variant;
    }

    name = gtk_widget_get_name (GTK_WIDGET (user_data));
    variant = g_variant_new_string (name);

    return variant;
}

/*
 * et_settings_flags_toggle_get:
 * @value: the property value to be set (active item on combo box)
 * @variant: the variant to set the @value from
 * @user_data: the #GType of the #GSettings flags
 *
 * Wrapper function to convert a flags-type GSettings key state into the active
 * toggle button.
 *
 * Returns: %TRUE if the mapping was successful, %FALSE otherwise
 */
gboolean
et_settings_flags_toggle_get (GValue *value, GVariant *variant, gpointer user_data)
{
    const gchar *name;
    GType flags_type;
    GFlagsClass *flags_class;
    GVariantIter iter;
    GFlagsValue *flags_value;
    const gchar *nick;
    guint flags = 0;

    g_return_val_if_fail (user_data != NULL, FALSE);

    name = gtk_widget_get_name (GTK_WIDGET (user_data));
    flags_type = (GType)GPOINTER_TO_SIZE (g_object_get_data (G_OBJECT (user_data),
                                                                       "flags-type"));
    flags_class = g_type_class_ref (flags_type);

    g_variant_iter_init (&iter, variant);

    while (g_variant_iter_next (&iter, "&s", &nick))
    {
        flags_value = g_flags_get_value_by_nick (flags_class, nick);

        if (flags_value)
        {
            flags |= flags_value->value;
        }
        else
        {
            g_warning ("Unable to lookup %s flags nick '%s' from GType",
                       g_type_name (flags_type), nick);
            g_type_class_unref (flags_class);
            return FALSE;
        }
    }

    flags_value = g_flags_get_value_by_nick (flags_class, name);
    g_type_class_unref (flags_class);

    /* TRUE if settings flag is set for this widget, which will make the widget
     * active. */
    g_value_set_boolean (value, flags & flags_value->value);
    return TRUE;
}

/*
 * et_settings_flags_toggle_set:
 * @value: the property value to set the @variant from
 * @expected_type: the expected type of the returned variant
 * @user_data: the widget associated with the changed setting
 *
 * Wrapper function to convert a boolean value into a string suitable for
 * storing into a flags-type GSettings key.
 *
 * Returns: a new GVariant containing the mapped value, or %NULL upon failure
 */
GVariant *
et_settings_flags_toggle_set (const GValue *value,
                              const GVariantType *expected_type,
                              gpointer user_data)
{
    const gchar *name;
    GType flags_type;
    GFlagsClass *flags_class;
    GFlagsValue *flags_value;
    guint mask;
    GVariantBuilder builder;
    const gchar *flags_key;
    guint flags;

    g_return_val_if_fail (user_data != NULL, NULL);

    name = gtk_widget_get_name (GTK_WIDGET (user_data));
    flags_type = (GType)GPOINTER_TO_SIZE (g_object_get_data (G_OBJECT (user_data),
                                                                       "flags-type"));
    flags_class = g_type_class_ref (flags_type);
    flags_value = g_flags_get_value_by_nick (flags_class, name);
    mask = flags_class->mask;

    if (!flags_value)
    {
        g_warning ("Unable to lookup %s flags value '%d' from GType",
                   g_type_name (flags_type), g_value_get_boolean (value));
        g_type_class_unref (flags_class);
        return NULL;
    }

    flags_key = g_object_get_data (G_OBJECT (user_data), "flags-key");
    flags = g_settings_get_flags (MainSettings, flags_key);

    if (g_value_get_boolean (value))
    {
        flags |= flags_value->value;
    }
    else
    {
        flags &= (flags_value->value ^ mask);
    }

    g_variant_builder_init (&builder, expected_type);

    while (flags)
    {
        flags_value = g_flags_get_first_value (flags_class, flags);

        if (flags_value == NULL)
        {
            g_variant_builder_clear (&builder);
            g_type_class_unref (flags_class);
            return NULL;
        }

        g_variant_builder_add (&builder, "s", flags_value->value_nick);
        flags &= ~flags_value->value;
    }

    g_type_class_unref (flags_class);

    return g_variant_builder_end (&builder);
}
