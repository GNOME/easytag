/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2013-2015  David King <amigadave@amigadave.com>
 * Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
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

#include "config.h"

#include "preferences_dialog.h"

#include <errno.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <string.h>

#include "application_window.h"
#include "setting.h"
#include "misc.h"
#include "scan_dialog.h"
#include "easytag.h"
#include "enums.h"
#include "browser.h"
#include "cddb_dialog.h"
#include "charset.h"
#include "win32/win32dep.h"

typedef struct
{
    GtkWidget *default_path_button;
    GtkWidget *browser_startup_check;
    GtkWidget *browser_subdirs_check;
    GtkWidget *browser_expand_subdirs_check;
    GtkWidget *browser_hidden_check;
    GtkWidget *browser_case_check;
    GtkWidget *log_show_check;
    GtkWidget *header_show_check;
    GtkWidget *list_bold_radio;
    GtkWidget *file_name_replace_check;
    GtkWidget *name_lower_radio;
    GtkWidget *name_upper_radio;
    GtkWidget *name_no_change_radio;
    GtkWidget *file_preserve_check;
    GtkWidget *file_parent_check;
    GtkWidget *file_encoding_try_alternative_radio;
    GtkWidget *file_encoding_transliterate_radio;
    GtkWidget *file_encoding_ignore_radio;
    GtkWidget *tags_auto_date_check;
    GtkWidget *tags_auto_image_type_check;
    GtkWidget *tags_track_check;
    GtkWidget *tags_track_button;
    GtkWidget *tags_disc_check;
    GtkWidget *tags_disc_button;
    GtkWidget *tags_preserve_focus_check;
    GtkWidget *split_title_check;
    GtkWidget *split_artist_check;
    GtkWidget *split_album_check;
    GtkWidget *split_genre_check;
    GtkWidget *split_comment_check;
    GtkWidget *split_composer_check;
    GtkWidget *split_orig_artist_check;
    GtkWidget *id3_strip_check;
    GtkWidget *id3_v2_convert_check;
    GtkWidget *id3_v2_crc32_check;
    GtkWidget *id3_v2_compression_check;
    GtkWidget *id3_v2_genre_check;
    GtkWidget *id3_v2_check;
    GtkWidget *id3_v2_version_label;
    GtkWidget *id3_v2_version_combo;
    GtkWidget *id3_v2_encoding_label;
    GtkWidget *id3_v2_unicode_radio;
    GtkWidget *id3_v2_unicode_encoding_combo;
    GtkWidget *id3_v2_other_radio;
    GtkWidget *id3_v2_override_encoding_combo;
    GtkWidget *id3_v2_iconv_label;
    GtkWidget *id3_v2_none_radio;
    GtkWidget *id3_v2_transliterate_radio;
    GtkWidget *id3_v2_ignore_radio;
    GtkWidget *id3_v1_check;
    GtkWidget *id3_v1_encoding_grid;
    GtkWidget *id3_v1_encoding_combo;
    GtkWidget *id3_v1_none_radio;
    GtkWidget *id3_v1_transliterate_radio;
    GtkWidget *id3_v1_ignore_radio;
    GtkWidget *id3_read_encoding_check;
    GtkWidget *id3_read_encoding_combo;
    GtkWidget *preferences_notebook;
    GtkWidget *scanner_grid;
    GtkWidget *fts_underscore_p20_radio;
    GtkWidget *fts_spaces_radio;
    GtkWidget *fts_none_radio;
    GtkWidget *rfs_underscore_p20_radio;
    GtkWidget *rfs_spaces_radio;
    GtkWidget *rfs_remove_radio;
    GtkWidget *pfs_uppercase_prep_check;
    GtkWidget *overwrite_fields_check;
    GtkWidget *default_comment_check;
    GtkWidget *default_comment_entry;
    GtkWidget *crc32_default_check;
    GtkWidget *cddb_automatic_host1_combo;
    GtkWidget *cddb_automatic_port1_button;
    GtkWidget *cddb_automatic_path1_entry;
    GtkWidget *cddb_automatic_host2_combo;
    GtkWidget *cddb_automatic_port2_button;
    GtkWidget *cddb_automatic_path2_entry;
    GtkWidget *cddb_manual_host_combo;
    GtkWidget *cddb_manual_port_button;
    GtkWidget *cddb_manual_path_entry;
    GtkWidget *cddb_follow_check;
    GtkWidget *cddb_dlm_check;
    GtkWidget *confirm_write_check;
    GtkWidget *confirm_rename_check;
    GtkWidget *confirm_delete_check;
    GtkWidget *confirm_write_playlist_check;
    GtkWidget *confirm_unsaved_files_check;
    GtkWidget *scanner_dialog_startup_check;

    GtkListStore *default_path_model;
    GtkListStore *file_player_model;

    gint options_notebook_scanner;
} EtPreferencesDialogPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EtPreferencesDialog, et_preferences_dialog, GTK_TYPE_DIALOG)

/**************
 * Prototypes *
 **************/
/* Options window */
static void notify_id3_settings_active (GObject *object, GParamSpec *pspec, EtPreferencesDialog *self);

static void et_preferences_on_response (GtkDialog *dialog, gint response_id,
                                        gpointer user_data);


/*************
 * Functions *
 *************/
static void
et_prefs_current_folder_changed (EtPreferencesDialog *self,
                                 GtkFileChooser *default_path_button)
{
    gchar *path;

    /* The path that is currently selected, not that which is currently being
     * displayed. */
    path = gtk_file_chooser_get_filename (default_path_button);

    if (path)
    {
        g_settings_set_value (MainSettings, "default-path",
                              g_variant_new_bytestring (path));
        g_free (path);
    }
}

static void
on_default_path_changed (GSettings *settings,
                         const gchar *key,
                         GtkFileChooserButton *default_path_button)
{
    GVariant *default_path;
    const gchar *path;

    default_path = g_settings_get_value (settings, key);
    path = g_variant_get_bytestring (default_path);

    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (default_path_button),
                                         path);
    g_variant_unref (default_path);
}

#ifdef ENABLE_ID3LIB
static gboolean
et_preferences_id3v2_version_get (GValue *value,
                                  GVariant *variant,
                                  gpointer user_data)
{
    gboolean id3v24;

    id3v24 = g_variant_get_boolean (variant);

    g_value_set_int (value, id3v24 ? 1 : 0);

    return TRUE;
}

static GVariant *
et_preferences_id3v2_version_set (const GValue *value,
                                  const GVariantType *variant_type,
                                  gpointer user_data)
{
    GVariant *id3v24;
    gint active_row;

    active_row = g_value_get_int (value);

    id3v24 = g_variant_new_boolean (active_row == 1);

    return id3v24;
}
#endif /* ENABLE_ID3LIB */

static gboolean
et_preferences_id3v2_unicode_charset_get (GValue *value,
                                          GVariant *variant,
                                          gpointer user_data)
{
    const gchar *charset;

    charset = g_variant_get_string (variant, NULL);

    if (strcmp (charset, "UTF-8") == 0)
    {
        g_value_set_int (value, 0);
    }
    else if (strcmp (charset, "UTF-16") == 0)
    {
        g_value_set_int (value, 1);
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

static GVariant *
et_preferences_id3v2_unicode_charset_set (const GValue *value,
                                          const GVariantType *variant_type,
                                          gpointer user_data)
{
    gint active_row;

    active_row = g_value_get_int (value);

    switch (active_row)
    {
        case 0:
            return g_variant_new_string ("UTF-8");
            break;
        case 1:
            return g_variant_new_string ("UTF-16");
            break;
        default:
            g_assert_not_reached ();
    }
}

/*
 * The window for options
 */
static void
create_preferences_dialog (EtPreferencesDialog *self)
{
    EtPreferencesDialogPrivate *priv;

    priv = et_preferences_dialog_get_instance_private (self);

    /* Browser. */
    on_default_path_changed (MainSettings, "default-path",
                             GTK_FILE_CHOOSER_BUTTON (priv->default_path_button));
    g_signal_connect (MainSettings, "changed::default-path",
                      G_CALLBACK (on_default_path_changed),
                      priv->default_path_button);

    /* Load directory on startup */
    g_settings_bind (MainSettings, "load-on-startup",
                     priv->browser_startup_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Browse subdirectories */
    g_settings_bind (MainSettings, "browse-subdir",
                     priv->browser_subdirs_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Open the node to show subdirectories */
    g_settings_bind (MainSettings, "browse-expand-children",
                     priv->browser_expand_subdirs_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Browse hidden directories */
    g_settings_bind (MainSettings, "browse-show-hidden",
                     priv->browser_hidden_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (MainSettings, "sort-case-sensitive",
                     priv->browser_case_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Show / hide log view. */
    g_settings_bind (MainSettings, "log-show", priv->log_show_check, "active",
                     G_SETTINGS_BIND_DEFAULT);
   
    /* Show header information. */
    g_settings_bind (MainSettings, "file-show-header", priv->header_show_check,
                     "active", G_SETTINGS_BIND_DEFAULT);

    /* Display color mode for changed files in list. */
    /* Set "new" Gtk+-2.0ish black/bold style for changed items. */
    g_settings_bind (MainSettings, "file-changed-bold", priv->list_bold_radio,
                     "active", G_SETTINGS_BIND_DEFAULT);
    g_signal_connect_swapped (priv->list_bold_radio, "notify::active",
                              G_CALLBACK (et_application_window_browser_refresh_list),
                              MainWindow);

    /*
     * File Settings
     */
    /* File (name) Options */
    g_settings_bind (MainSettings, "rename-replace-illegal-chars",
                     priv->file_name_replace_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Extension case (lower/upper?) */
    g_settings_bind_with_mapping (MainSettings, "rename-extension-mode",
                                  priv->name_lower_radio, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->name_lower_radio, NULL);
    g_settings_bind_with_mapping (MainSettings, "rename-extension-mode",
                                  priv->name_upper_radio, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->name_upper_radio, NULL);
    g_settings_bind_with_mapping (MainSettings, "rename-extension-mode",
                                  priv->name_no_change_radio, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->name_no_change_radio, NULL);

    /* Preserve modification time */
    g_settings_bind (MainSettings, "file-preserve-modification-time",
                     priv->file_preserve_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Change directory modification time */
    g_settings_bind (MainSettings, "file-update-parent-modification-time",
                     priv->file_parent_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Character Set for Filename */
    g_settings_bind_with_mapping (MainSettings, "rename-encoding",
                                  priv->file_encoding_try_alternative_radio,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->file_encoding_try_alternative_radio,
                                  NULL);
    g_settings_bind_with_mapping (MainSettings, "rename-encoding",
                                  priv->file_encoding_transliterate_radio,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->file_encoding_transliterate_radio,
                                  NULL);
    g_settings_bind_with_mapping (MainSettings, "rename-encoding",
                                  priv->file_encoding_ignore_radio, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->file_encoding_ignore_radio, NULL);

    /*
     * Tag Settings
     */
    /* Tag Options */
    g_settings_bind (MainSettings, "tag-date-autocomplete",
                     priv->tags_auto_date_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (MainSettings, "tag-image-type-automatic",
                     priv->tags_auto_image_type_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Track formatting. */
    g_settings_bind (MainSettings, "tag-number-padded", priv->tags_track_check,
                     "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "tag-number-length",
                     priv->tags_track_button, "value",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "tag-number-padded",
                     priv->tags_track_button, "sensitive",
                     G_SETTINGS_BIND_GET);

    /* Disc formatting. */
    g_settings_bind (MainSettings, "tag-disc-padded", priv->tags_disc_check,
                     "active", G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (MainSettings, "tag-disc-length",
                     priv->tags_disc_button, "value",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "tag-disc-padded",
                     priv->tags_disc_button, "sensitive",
                     G_SETTINGS_BIND_GET);
    g_signal_emit_by_name (G_OBJECT (priv->tags_disc_check), "toggled");

    /* Tag field focus */
    g_settings_bind (MainSettings, "tag-preserve-focus",
                     priv->tags_preserve_focus_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Tag Splitting */
    g_settings_bind (MainSettings, "ogg-split-title", priv->split_title_check,
                     "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "ogg-split-artist", priv->split_artist_check,
                     "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "ogg-split-album", priv->split_album_check,
                     "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "ogg-split-genre", priv->split_genre_check,
                     "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "ogg-split-comment",
                     priv->split_comment_check, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "ogg-split-composer",
                     priv->split_composer_check, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "ogg-split-original-artist",
                     priv->split_orig_artist_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /*
     * ID3 Tag Settings
     */
    /* Strip tag when fields (managed by EasyTAG) are empty */
    g_settings_bind (MainSettings, "id3-strip-empty", priv->id3_strip_check,
                     "active", G_SETTINGS_BIND_DEFAULT);

    /* Convert old ID3v2 tag version */
    g_settings_bind (MainSettings, "id3v2-convert-old",
                     priv->id3_v2_convert_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Use CRC32 */
    g_settings_bind (MainSettings, "id3v2-crc32", priv->id3_v2_crc32_check,
                     "active", G_SETTINGS_BIND_DEFAULT);

    /* Use Compression */
    g_settings_bind (MainSettings, "id3v2-compression",
                     priv->id3_v2_compression_check, "active",
                     G_SETTINGS_BIND_DEFAULT);
	
    /* Write Genre in text */
    g_settings_bind (MainSettings, "id3v2-text-only-genre",
                     priv->id3_v2_genre_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Write ID3v2 tag */
    g_settings_bind (MainSettings, "id3v2-enabled", priv->id3_v2_check,
                     "active", G_SETTINGS_BIND_DEFAULT);
    g_signal_connect (priv->id3_v2_check, "notify::active",
                      G_CALLBACK (notify_id3_settings_active), self);

#ifdef ENABLE_ID3LIB
    /* ID3v2 tag version */
    g_settings_bind_with_mapping (MainSettings, "id3v2-version-4",
                                  priv->id3_v2_version_combo, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_preferences_id3v2_version_get,
                                  et_preferences_id3v2_version_set, self,
                                  NULL);
    g_signal_connect (MainSettings, "changed::id3v2-version-4",
                      G_CALLBACK (notify_id3_settings_active), self);
#endif

    /* Charset */
    /* Unicode. */
    g_settings_bind_with_mapping (MainSettings, "id3v2-unicode-charset",
                                  priv->id3_v2_unicode_encoding_combo,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_preferences_id3v2_unicode_charset_get,
                                  et_preferences_id3v2_unicode_charset_set,
                                  NULL, NULL);

    g_settings_bind (MainSettings, "id3v2-enable-unicode",
                     priv->id3_v2_other_radio, "active",
                     G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_INVERT_BOOLEAN);
    g_signal_connect (priv->id3_v2_unicode_radio,
                      "notify::active",
                      G_CALLBACK (notify_id3_settings_active), self);

    /* Non-Unicode. */
    Charset_Populate_Combobox (GTK_COMBO_BOX (priv->id3_v2_override_encoding_combo),
                               g_settings_get_enum (MainSettings,
                                                    "id3v2-no-unicode-charset"));
    g_settings_bind_with_mapping (MainSettings, "id3v2-no-unicode-charset",
                                  priv->id3_v2_override_encoding_combo,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_get, et_settings_enum_set,
                                  GSIZE_TO_POINTER (ET_TYPE_CHARSET), NULL);
    g_signal_connect (priv->id3_v2_other_radio,
                      "notify::active",
                      G_CALLBACK (notify_id3_settings_active), self);

    /* ID3v2 Additional iconv() options. */
    g_settings_bind_with_mapping (MainSettings, "id3v2-encoding-option",
                                  priv->id3_v2_none_radio, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->id3_v2_none_radio, NULL);
    g_settings_bind_with_mapping (MainSettings, "id3v2-encoding-option",
                                  priv->id3_v2_transliterate_radio,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->id3_v2_transliterate_radio, NULL);
    g_settings_bind_with_mapping (MainSettings, "id3v2-encoding-option",
                                  priv->id3_v2_ignore_radio, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->id3_v2_ignore_radio, NULL);

    /* Write ID3v1 tag */
    g_settings_bind (MainSettings, "id3v1-enabled", priv->id3_v1_check,
                     "active", G_SETTINGS_BIND_DEFAULT);
    g_signal_connect (priv->id3_v1_check, "notify::active",
                      G_CALLBACK (notify_id3_settings_active), self);

    /* Id3V1 writing character set */
    Charset_Populate_Combobox (GTK_COMBO_BOX (priv->id3_v1_encoding_combo),
                               g_settings_get_enum (MainSettings,
                                                    "id3v1-charset"));
    g_settings_bind_with_mapping (MainSettings, "id3v1-charset",
                                  priv->id3_v1_encoding_combo,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_get, et_settings_enum_set,
                                  GSIZE_TO_POINTER (ET_TYPE_CHARSET), NULL);

    /* ID3V1 Additional iconv() options*/
    g_settings_bind_with_mapping (MainSettings, "id3v1-encoding-option",
                                  priv->id3_v1_none_radio, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->id3_v1_none_radio, NULL);
    g_settings_bind_with_mapping (MainSettings, "id3v1-encoding-option",
                                  priv->id3_v1_transliterate_radio,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->id3_v1_transliterate_radio, NULL);
    g_settings_bind_with_mapping (MainSettings, "id3v1-encoding-option",
                                  priv->id3_v1_ignore_radio, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->id3_v1_ignore_radio, NULL);

    /* Character Set for reading tag */
    /* "File Reading Charset" Check Button + Combo. */
    g_settings_bind (MainSettings, "id3-override-read-encoding",
                     priv->id3_read_encoding_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    Charset_Populate_Combobox (GTK_COMBO_BOX (priv->id3_read_encoding_combo),
                               g_settings_get_enum (MainSettings,
                                                    "id3v1v2-charset"));
    g_settings_bind_with_mapping (MainSettings, "id3v1v2-charset",
                                  priv->id3_read_encoding_combo,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_get, et_settings_enum_set,
                                  GSIZE_TO_POINTER (ET_TYPE_CHARSET), NULL);
    g_settings_bind (MainSettings, "id3-override-read-encoding",
                     priv->id3_read_encoding_combo, "sensitive",
                     G_SETTINGS_BIND_GET);
    notify_id3_settings_active (NULL, NULL, self);

    /*
     * Scanner
     */
    /* Save the number of the page. Asked in Scanner window */
    priv->options_notebook_scanner = gtk_notebook_page_num (GTK_NOTEBOOK (priv->preferences_notebook),
                                                            priv->scanner_grid);

    /* Character conversion for the 'Fill Tag' scanner (=> FTS...) */
    g_settings_bind_with_mapping (MainSettings, "fill-convert-spaces",
                                  priv->fts_underscore_p20_radio,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->fts_underscore_p20_radio, NULL);
    g_settings_bind_with_mapping (MainSettings, "fill-convert-spaces",
                                  priv->fts_spaces_radio, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->fts_spaces_radio, NULL);
    g_settings_bind_with_mapping (MainSettings, "fill-convert-spaces",
                                  priv->fts_none_radio, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->fts_none_radio, NULL);
    /* TODO: No change tooltip. */

    /* Character conversion for the 'Rename File' scanner (=> RFS...) */
    g_settings_bind_with_mapping (MainSettings, "rename-convert-spaces",
                                  priv->rfs_underscore_p20_radio,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->rfs_underscore_p20_radio, NULL);
    g_settings_bind_with_mapping (MainSettings, "rename-convert-spaces",
                                  priv->rfs_spaces_radio, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->rfs_spaces_radio, NULL);
    g_settings_bind_with_mapping (MainSettings, "rename-convert-spaces",
                                  priv->rfs_remove_radio, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->rfs_remove_radio,
                                  NULL);

    /* Character conversion for the 'Process Fields' scanner (=> PFS...) */
    g_settings_bind (MainSettings, "process-uppercase-prepositions",
                     priv->pfs_uppercase_prep_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Other options */
    g_settings_bind (MainSettings, "fill-overwrite-tag-fields",
                     priv->overwrite_fields_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Set a default comment text or CRC-32 checksum. */
    g_settings_bind (MainSettings, "fill-set-default-comment",
                     priv->default_comment_check, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "fill-set-default-comment",
                     priv->default_comment_entry, "sensitive",
                     G_SETTINGS_BIND_GET);
    g_settings_bind (MainSettings, "fill-default-comment",
                     priv->default_comment_entry, "text",
                     G_SETTINGS_BIND_DEFAULT);

    /* CRC32 comment. */
    g_settings_bind (MainSettings, "fill-crc32-comment",
                     priv->crc32_default_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* CDDB */
    /* 1st automatic search server. */
    g_settings_bind (MainSettings, "cddb-automatic-search-hostname",
                     gtk_bin_get_child (GTK_BIN (priv->cddb_automatic_host1_combo)),
                     "text", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "cddb-automatic-search-port",
                     priv->cddb_automatic_port1_button, "value",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "cddb-automatic-search-path",
                     priv->cddb_automatic_path1_entry, "text",
                     G_SETTINGS_BIND_DEFAULT);

    /* 2nd automatic search server. */
    g_settings_bind (MainSettings, "cddb-automatic-search-hostname2",
                     gtk_bin_get_child (GTK_BIN (priv->cddb_automatic_host2_combo)),
                     "text", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "cddb-automatic-search-port2",
                     priv->cddb_automatic_port2_button, "value",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "cddb-automatic-search-path2",
                     priv->cddb_automatic_path2_entry, "text",
                     G_SETTINGS_BIND_DEFAULT);

    /* CDDB Server Settings (Manual Search). */
    g_settings_bind (MainSettings, "cddb-manual-search-hostname",
                     gtk_bin_get_child (GTK_BIN (priv->cddb_manual_host_combo)),
                     "text", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "cddb-manual-search-port",
                     priv->cddb_manual_port_button, "value",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "cddb-manual-search-path",
                     priv->cddb_manual_path_entry, "text",
                     G_SETTINGS_BIND_DEFAULT);

    /* Track Name list (CDDB results). */
    g_settings_bind (MainSettings, "cddb-follow-file", priv->cddb_follow_check,
                     "active", G_SETTINGS_BIND_DEFAULT);

    /* Check box to use DLM. */
    g_settings_bind (MainSettings, "cddb-dlm-enabled", priv->cddb_dlm_check,
                     "active", G_SETTINGS_BIND_DEFAULT);

    /* Confirmation */
    g_settings_bind (MainSettings, "confirm-write-tags",
                     priv->confirm_write_check, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "confirm-rename-file",
                     priv->confirm_rename_check, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "confirm-delete-file",
                     priv->confirm_delete_check, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "confirm-write-playlist",
                     priv->confirm_write_playlist_check, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "confirm-when-unsaved-files",
                     priv->confirm_unsaved_files_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Properties of the scanner window */
    g_settings_bind (MainSettings, "scan-startup",
                     priv->scanner_dialog_startup_check, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Load the default page */
    g_settings_bind (MainSettings, "preferences-page",
                     priv->preferences_notebook, "page",
                     G_SETTINGS_BIND_DEFAULT);
}

static void
notify_id3_settings_active (GObject *object,
                            GParamSpec *pspec,
                            EtPreferencesDialog *self)
{
    EtPreferencesDialogPrivate *priv;
    gboolean active;

    priv = et_preferences_dialog_get_instance_private (self);

    active = g_settings_get_boolean (MainSettings, "id3v2-enable-unicode");

    if (g_settings_get_boolean (MainSettings, "id3v2-enabled"))
    {
        gtk_widget_set_sensitive (priv->id3_v2_encoding_label, TRUE);

#ifdef ENABLE_ID3LIB
        gtk_widget_set_sensitive (priv->id3_v2_version_label, TRUE);
        gtk_widget_set_sensitive (priv->id3_v2_version_combo, TRUE);

        if (!g_settings_get_boolean (MainSettings, "id3v2-version-4"))
        {
            /* When "ID3v2.3" is selected. */
            gtk_combo_box_set_active (GTK_COMBO_BOX (priv->id3_v2_unicode_encoding_combo), 1);
            gtk_widget_set_sensitive (priv->id3_v2_unicode_encoding_combo,
                                      FALSE);
        }
        else
        {
            /* When "ID3v2.4" is selected, set "UTF-8" as default value. */
            gtk_combo_box_set_active (GTK_COMBO_BOX (priv->id3_v2_unicode_encoding_combo),
                                      0);
            gtk_widget_set_sensitive (priv->id3_v2_unicode_encoding_combo,
                                      active);
        }
#else 
        gtk_widget_set_sensitive (priv->id3_v2_unicode_encoding_combo,
                                  active);
#endif
        gtk_widget_set_sensitive(priv->id3_v2_unicode_radio, TRUE);
        gtk_widget_set_sensitive(priv->id3_v2_other_radio, TRUE);
        gtk_widget_set_sensitive (priv->id3_v2_override_encoding_combo,
                                  !active);
        gtk_widget_set_sensitive (priv->id3_v2_iconv_label, !active);
        gtk_widget_set_sensitive (priv->id3_v2_none_radio, !active);
        gtk_widget_set_sensitive (priv->id3_v2_transliterate_radio, !active);
        gtk_widget_set_sensitive (priv->id3_v2_ignore_radio, !active);
        gtk_widget_set_sensitive (priv->id3_v2_crc32_check, TRUE);
        gtk_widget_set_sensitive (priv->id3_v2_compression_check, TRUE);
        gtk_widget_set_sensitive (priv->id3_v2_genre_check, TRUE);
        gtk_widget_set_sensitive (priv->id3_v2_convert_check, TRUE);

    }else
    {
        gtk_widget_set_sensitive (priv->id3_v2_encoding_label, FALSE);
#ifdef ENABLE_ID3LIB
        gtk_widget_set_sensitive (priv->id3_v2_version_label, FALSE);
        gtk_widget_set_sensitive (priv->id3_v2_version_combo, FALSE);
#endif
        gtk_widget_set_sensitive (priv->id3_v2_unicode_radio, FALSE);
        gtk_widget_set_sensitive (priv->id3_v2_other_radio, FALSE);
        gtk_widget_set_sensitive (priv->id3_v2_unicode_encoding_combo, FALSE);
        gtk_widget_set_sensitive (priv->id3_v2_override_encoding_combo, FALSE);
        gtk_widget_set_sensitive (priv->id3_v2_iconv_label, FALSE);
        gtk_widget_set_sensitive (priv->id3_v2_none_radio, FALSE);
        gtk_widget_set_sensitive (priv->id3_v2_transliterate_radio, FALSE);
        gtk_widget_set_sensitive (priv->id3_v2_ignore_radio, FALSE);
        gtk_widget_set_sensitive (priv->id3_v2_crc32_check, FALSE);
        gtk_widget_set_sensitive (priv->id3_v2_compression_check, FALSE);
        gtk_widget_set_sensitive (priv->id3_v2_genre_check, FALSE);
        gtk_widget_set_sensitive (priv->id3_v2_convert_check, FALSE);
    }

    active = g_settings_get_boolean (MainSettings, "id3v1-enabled");

    gtk_widget_set_sensitive (priv->id3_v1_encoding_grid, active);
}

/*
 * Check_Config: Check if config information are correct
 *
 * Problem noted : if a character is escaped (like : 'C\351line DION') in
 *                 gtk_file_chooser it will converted to UTF-8. So after, there
 *                 is a problem to convert it in the right system encoding to be
 *                 passed to stat(), and it can find the directory.
 * exemple :
 *  - initial file on system                        : C\351line DION - D'eux (1995)
 *  - converted to UTF-8 (path_utf8)                : Céline DION - D'eux (1995)
 *  - try to convert to system encoding (path_real) : ?????
 */
static gboolean
Check_DefaultPathToMp3 (EtPreferencesDialog *self)
{
    GVariant *default_path;
    const gchar *path_real;
    GFile *file;
    GFileInfo *fileinfo;

    default_path = g_settings_get_value (MainSettings, "default-path");
    path_real = g_variant_get_bytestring (default_path);

    if (!*path_real)
    {
        g_variant_unref (default_path);
        return TRUE;
    }

    file = g_file_new_for_path (path_real);
    fileinfo = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                                  G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                                  G_FILE_QUERY_INFO_NONE, NULL, NULL);
    g_variant_unref (default_path);
    g_object_unref (file);

    if (fileinfo)
    {
        if (g_file_info_get_file_type (fileinfo) == G_FILE_TYPE_DIRECTORY)
        {
            g_object_unref (fileinfo);
            return TRUE; /* Path is good */
        }
        else
        {
            GtkWidget *msgdialog;
            const gchar *path_utf8;

            path_utf8 = g_file_info_get_display_name (fileinfo);
            msgdialog = gtk_message_dialog_new (GTK_WINDOW (self),
                                                GTK_DIALOG_MODAL
                                                | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_CLOSE,
                                                "%s",
                                                _("The selected default path is invalid"));
            gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (msgdialog),
                                                      _("Path: ‘%s’\nError: %s"),
                                                      path_utf8,
                                                      g_strerror (errno));
            gtk_window_set_title (GTK_WINDOW (msgdialog),
                                  _("Invalid Path Error"));

            gtk_dialog_run (GTK_DIALOG (msgdialog));
            gtk_widget_destroy (msgdialog);
        }

        g_object_unref (fileinfo);
    }

    return FALSE;
}

static gboolean
Check_Config (EtPreferencesDialog *self)
{
    if (Check_DefaultPathToMp3 (self))
        return TRUE; /* No problem detected */
    else
        return FALSE; /* Oops! */
}

/* Callback from et_preferences_dialog_on_response. */
static void
OptionsWindow_Save_Button (EtPreferencesDialog *self)
{
    if (!Check_Config (self)) return;

    gtk_widget_hide (GTK_WIDGET (self));
}

void
et_preferences_dialog_show_scanner (EtPreferencesDialog *self)
{
    EtPreferencesDialogPrivate *priv;

    g_return_if_fail (ET_PREFERENCES_DIALOG (self));

    priv = et_preferences_dialog_get_instance_private (self);

    gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->preferences_notebook),
                                   priv->options_notebook_scanner);
    gtk_window_present (GTK_WINDOW (self));
}

/*
 * et_preferences_on_response:
 * @dialog: the dialog which trigerred the response signal
 * @response_id: the response which was triggered
 * @user_data: user data set when the signal was connected
 *
 * Signal handler for the response signal, to check whether the OK or cancel
 * button was clicked, or if a delete event was received.
 */
static void
et_preferences_on_response (GtkDialog *dialog, gint response_id,
                            gpointer user_data)
{
    switch (response_id)
    {
        case GTK_RESPONSE_CLOSE:
            OptionsWindow_Save_Button (ET_PREFERENCES_DIALOG (dialog));
            break;
        case GTK_RESPONSE_DELETE_EVENT:
            break;
        default:
            g_assert_not_reached ();
    }
}

static void
et_preferences_dialog_init (EtPreferencesDialog *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
    create_preferences_dialog (self);
}

static void
et_preferences_dialog_class_init (EtPreferencesDialogClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/EasyTAG/preferences_dialog.ui");
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  default_path_button);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  browser_startup_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  browser_subdirs_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  browser_expand_subdirs_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  browser_hidden_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  browser_case_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  log_show_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  header_show_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  list_bold_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  file_name_replace_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  name_lower_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  name_upper_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  name_no_change_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  file_preserve_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  file_parent_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  file_encoding_try_alternative_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  file_encoding_transliterate_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  file_encoding_ignore_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  tags_auto_date_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  tags_auto_image_type_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  tags_track_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  tags_track_button);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  tags_disc_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  tags_disc_button);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  tags_preserve_focus_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  split_title_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  split_artist_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  split_album_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  split_genre_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  split_comment_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  split_composer_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  split_orig_artist_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_strip_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v2_convert_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v2_crc32_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v2_compression_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v2_genre_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v2_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v2_version_label);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v2_version_combo);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v2_encoding_label);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v2_unicode_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v2_unicode_encoding_combo);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v2_other_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v2_override_encoding_combo);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v2_iconv_label);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v2_none_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v2_transliterate_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v2_ignore_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v1_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v1_encoding_grid);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v1_encoding_combo);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v1_none_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v1_transliterate_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_v1_ignore_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_read_encoding_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  id3_read_encoding_combo);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  preferences_notebook);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  scanner_grid);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  fts_underscore_p20_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  fts_spaces_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  fts_none_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  rfs_underscore_p20_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  rfs_spaces_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  rfs_remove_radio);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  pfs_uppercase_prep_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  overwrite_fields_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  default_comment_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  default_comment_entry);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  crc32_default_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  cddb_automatic_host1_combo);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  cddb_automatic_port1_button);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  cddb_automatic_path1_entry);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  cddb_automatic_host2_combo);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  cddb_automatic_port2_button);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  cddb_automatic_path2_entry);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  cddb_manual_host_combo);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  cddb_manual_port_button);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  cddb_manual_path_entry);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  cddb_follow_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  cddb_dlm_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  confirm_write_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  confirm_rename_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  confirm_delete_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  confirm_write_playlist_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  confirm_unsaved_files_check);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtPreferencesDialog,
                                                  scanner_dialog_startup_check);
    gtk_widget_class_bind_template_callback (widget_class,
                                             et_preferences_on_response);
    gtk_widget_class_bind_template_callback (widget_class,
                                             et_prefs_current_folder_changed);
}

/*
 * et_preferences_dialog_new:
 *
 * Create a new EtPreferencesDialog instance.
 *
 * Returns: a new #EtPreferencesDialog
 */
EtPreferencesDialog *
et_preferences_dialog_new (GtkWindow *parent)
{
    GtkSettings *settings;
    gboolean use_header_bar = FALSE;

    g_return_val_if_fail (GTK_WINDOW (parent), NULL);

    settings = gtk_settings_get_default ();

    if (settings)
    {
        g_object_get (settings, "gtk-dialogs-use-header", &use_header_bar,
                      NULL);
    }

    return g_object_new (ET_TYPE_PREFERENCES_DIALOG, "transient-for", parent,
                         "use-header-bar", use_header_bar, NULL);
}
