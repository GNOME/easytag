/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2013-2014  David King <amigadave@amigadave.com>
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

/* TODO: Use G_DEFINE_TYPE_WITH_PRIVATE. */
G_DEFINE_TYPE (EtPreferencesDialog, et_preferences_dialog, GTK_TYPE_DIALOG)

#define et_preferences_dialog_get_instance_private(dialog) (dialog->priv)

static const guint BOX_SPACING = 6;

struct _EtPreferencesDialogPrivate
{
    GtkListStore *default_path_model;
    GtkListStore *file_player_model;

    GtkWidget *id3_v1_encoding_grid;

    GtkWidget *LabelAdditionalId3v2IconvOptions;
    GtkWidget *LabelId3v2Charset;
    GtkWidget *LabelId3v2Version;
    GtkWidget *FileWritingId3v2VersionCombo;
    GtkWidget *FileWritingId3v2UseUnicodeCharacterSet;
    GtkWidget *FileWritingId3v2UseNoUnicodeCharacterSet;

    GtkWidget *FileWritingId3v2UnicodeCharacterSetCombo;
    GtkWidget *FileWritingId3v2NoUnicodeCharacterSetCombo;
    GtkWidget *FileWritingId3v1CharacterSetCombo;
    GtkWidget *FileReadingId3v1v2CharacterSetCombo;

    GtkWidget *ConvertOldId3v2TagVersion;
    GtkWidget *FileWritingId3v2UseCrc32;
    GtkWidget *FileWritingId3v2UseCompression;
    GtkWidget *FileWritingId3v2TextOnlyGenre;
    GtkWidget *FileWritingId3v2IconvOptionsNo;
    GtkWidget *FileWritingId3v2IconvOptionsTranslit;
    GtkWidget *FileWritingId3v2IconvOptionsIgnore;
    GtkWidget *FileWritingId3v1IconvOptionsNo;
    GtkWidget *FileWritingId3v1IconvOptionsTranslit;
    GtkWidget *FileWritingId3v1IconvOptionsIgnore;

    GtkWidget *options_notebook;
    gint options_notebook_scanner;
};

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
    GtkWidget *vbox;
    GtkWidget *LoadOnStartup;
    GtkWidget *BrowseSubdir;
    GtkWidget *OpenSelectedBrowserNode;
    GtkWidget *BrowseHiddendir;
    GtkWidget *ShowHeaderInfos;
    GtkWidget *ChangedFilesDisplayedToBold;
    GtkWidget *SortingFileCaseSensitive;
    GtkWidget *ShowLogView;
    GtkWidget *LogMaxLinesSpinButton;
    GtkWidget *ReplaceIllegalCharactersInFilename;
    GtkWidget *PreserveModificationTime;
    GtkWidget *UpdateParentDirectoryModificationTime;
    GtkWidget *FilenameCharacterSetOther;
    GtkWidget *FilenameCharacterSetApproximate;
    GtkWidget *FilenameCharacterSetDiscard;
    GtkWidget *DateAutoCompletion;
    GtkWidget *widget;
    GtkWidget *NumberTrackFormated;
    GtkWidget *NumberTrackFormatedSpinButton;
    GtkWidget *pad_disc_number;
    GtkWidget *pad_disc_number_spinbutton;
    GtkWidget *SetFocusToSameTagField;
    GtkWidget *VorbisSplitFieldTitle;
    GtkWidget *VorbisSplitFieldArtist;
    GtkWidget *VorbisSplitFieldAlbum;
    GtkWidget *VorbisSplitFieldGenre;
    GtkWidget *VorbisSplitFieldComment;
    GtkWidget *VorbisSplitFieldComposer;
    GtkWidget *VorbisSplitFieldOrigArtist;
    GtkWidget *StripTagWhenEmptyFields;
    GtkWidget *FileWritingId3v2WriteTag;
    GtkWidget *FileWritingId3v1WriteTag;
    GtkWidget *UseNonStandardId3ReadingCharacterSet;
    GtkWidget *FTSConvertUnderscoreAndP20IntoSpace;
    GtkWidget *FTSConvertSpaceIntoUnderscore;
    GtkWidget *FTSConvertSpaceNoChange;
    GtkWidget *RFSConvertUnderscoreAndP20IntoSpace;
    GtkWidget *RFSConvertSpaceIntoUnderscore;
    GtkWidget *RFSRemoveSpaces;
    GtkWidget *PFSDontUpperSomeWords;
    GtkWidget *OpenScannerWindowOnStartup;
    GtkWidget *OverwriteTagField;
    GtkWidget *SetDefaultComment;
    GtkWidget *DefaultComment;
    GtkWidget *Crc32Comment;
    GtkWidget *CddbServerNameAutomaticSearch;
    GtkWidget *CddbServerPortAutomaticSearch;
    GtkWidget *CddbServerCgiPathAutomaticSearch;
    GtkWidget *CddbServerNameAutomaticSearch2;
    GtkWidget *CddbServerPortAutomaticSearch2;
    GtkWidget *CddbServerCgiPathAutomaticSearch2;
    GtkWidget *CddbServerNameManualSearch;
    GtkWidget *CddbServerPortManualSearch;
    GtkWidget *CddbServerCgiPathManualSearch;
    GtkWidget *CddbUseProxy;
    GtkWidget *CddbProxyName;
    GtkWidget *CddbProxyPort;
    GtkWidget *CddbProxyUserName;
    GtkWidget *CddbProxyUserPassword;
    GtkWidget *CddbFollowFile;
    GtkWidget *CddbUseDLM;
    GtkWidget *ConfirmBeforeExit;
    GtkWidget *ConfirmWriteTag;
    GtkWidget *ConfirmRenameFile;
    GtkWidget *ConfirmDeleteFile;
    GtkWidget *ConfirmWritePlayList;
    GtkWidget *ConfirmWhenUnsavedFiles;
    GtkWidget *FilenameExtensionNoChange;
    GtkWidget *FilenameExtensionLowerCase;
    GtkWidget *FilenameExtensionUpperCase;
    GtkWidget *default_path_button;
    GtkBuilder *builder;
    GError *error = NULL;

    priv = et_preferences_dialog_get_instance_private (self);

    /* The window */
    gtk_window_set_title (GTK_WINDOW (self), _("Preferences"));
    gtk_window_set_destroy_with_parent (GTK_WINDOW (self), TRUE);
    gtk_dialog_add_buttons (GTK_DIALOG (self), _("_Close"), GTK_RESPONSE_CLOSE,
                            NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_CLOSE);
    g_signal_connect (self, "response",
                      G_CALLBACK (et_preferences_on_response), NULL);
    g_signal_connect (self, "delete-event",
                      G_CALLBACK (gtk_widget_hide_on_delete), NULL);

    gtk_container_set_border_width (GTK_CONTAINER (self), BOX_SPACING);

     /* Options */
     /* The vbox */
    vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));
    gtk_box_set_spacing (GTK_BOX (vbox), BOX_SPACING);

     /* Options NoteBook */
    builder = gtk_builder_new ();
    gtk_builder_add_from_resource (builder,
                                   "/org/gnome/EasyTAG/preferences_dialog.ui",
                                   &error);

    if (error != NULL)
    {
        g_error ("Unable to get scanner page from resource: %s",
                 error->message);
    }

    priv->options_notebook = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                 "preferences_notebook"));
    gtk_box_pack_start (GTK_BOX (vbox), priv->options_notebook, TRUE, TRUE, 0);

    /*
     * Browser
     */
    default_path_button = GTK_WIDGET (gtk_builder_get_object (builder,
                                                              "default_path_button"));
    on_default_path_changed (MainSettings, "default-path",
                             GTK_FILE_CHOOSER_BUTTON (default_path_button));
    /* Connecting to current-folder-changed does not work if the user selects
     * a directory from the combo box list provided by the file chooser button.
     */
    g_signal_connect_swapped (default_path_button, "file-set",
                              G_CALLBACK (et_prefs_current_folder_changed),
                              self);
    g_signal_connect (MainSettings, "changed::default-path",
                      G_CALLBACK (on_default_path_changed),
                      default_path_button);

    /* Load directory on startup */
    LoadOnStartup = GTK_WIDGET (gtk_builder_get_object (builder,
                                                        "browser_startup_check"));
    g_settings_bind (MainSettings, "load-on-startup", LoadOnStartup, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Browse subdirectories */
    BrowseSubdir = GTK_WIDGET (gtk_builder_get_object (builder,
                                                       "browser_subdirs_check"));
    g_settings_bind (MainSettings, "browse-subdir", BrowseSubdir, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Open the node to show subdirectories */
    OpenSelectedBrowserNode = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                  "browser_expand_subdirs_check"));
    g_settings_bind (MainSettings, "browse-expand-children",
                     OpenSelectedBrowserNode, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Browse hidden directories */
    BrowseHiddendir = GTK_WIDGET (gtk_builder_get_object (builder,
                                                          "browser_hidden_check"));
    g_settings_bind (MainSettings, "browse-show-hidden", BrowseHiddendir,
                     "active", G_SETTINGS_BIND_DEFAULT);

    SortingFileCaseSensitive = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                   "browser_case_check"));
    g_settings_bind (MainSettings, "sort-case-sensitive",
                     SortingFileCaseSensitive, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Show / hide log view. */
    ShowLogView = GTK_WIDGET (gtk_builder_get_object (builder,
                                                      "log_show_check"));
    g_settings_bind (MainSettings, "log-show", ShowLogView, "active",
                     G_SETTINGS_BIND_DEFAULT);
   
    /* Max number of lines. */
    LogMaxLinesSpinButton = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                "log_lines_button"));
    g_settings_bind (MainSettings, "log-lines", LogMaxLinesSpinButton,
                     "value", G_SETTINGS_BIND_DEFAULT);

    /* Show header informantion. */
    ShowHeaderInfos = GTK_WIDGET (gtk_builder_get_object (builder,
                                                          "header_show_check"));
    g_settings_bind (MainSettings, "file-show-header", ShowHeaderInfos,
                     "active", G_SETTINGS_BIND_DEFAULT);

    /* Display color mode for changed files in list. */
    /* Set "new" Gtk+-2.0ish black/bold style for changed items. */
    ChangedFilesDisplayedToBold = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                      "list_bold_radio"));
    g_settings_bind (MainSettings, "file-changed-bold",
                     ChangedFilesDisplayedToBold, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_signal_connect_swapped (ChangedFilesDisplayedToBold, "notify::active",
                              G_CALLBACK (et_application_window_browser_refresh_list),
                              MainWindow);

    /*
     * File Settings
     */
    /* File (name) Options */
    ReplaceIllegalCharactersInFilename = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                             "file_name_replace_check"));
    g_settings_bind (MainSettings, "rename-replace-illegal-chars",
                     ReplaceIllegalCharactersInFilename, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Extension case (lower/upper?) */
    FilenameExtensionLowerCase = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                     "name_lower_radio"));
    FilenameExtensionUpperCase = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                     "name_upper_radio"));
    FilenameExtensionNoChange = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                    "name_no_change_radio"));

    g_settings_bind_with_mapping (MainSettings, "rename-extension-mode",
                                  FilenameExtensionLowerCase, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  FilenameExtensionLowerCase, NULL);
    g_settings_bind_with_mapping (MainSettings, "rename-extension-mode",
                                  FilenameExtensionUpperCase, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  FilenameExtensionUpperCase, NULL);
    g_settings_bind_with_mapping (MainSettings, "rename-extension-mode",
                                  FilenameExtensionNoChange, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  FilenameExtensionNoChange, NULL);

    /* Preserve modification time */
    PreserveModificationTime = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                   "file_preserve_check"));
    g_settings_bind (MainSettings, "file-preserve-modification-time",
                     PreserveModificationTime, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Change directory modification time */
    UpdateParentDirectoryModificationTime = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                                "file_parent_check"));
    g_settings_bind (MainSettings, "file-update-parent-modification-time",
                     UpdateParentDirectoryModificationTime, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Character Set for Filename */
    FilenameCharacterSetOther = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                    "file_encoding_try_alternative_radio"));
    FilenameCharacterSetApproximate = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                          "file_encoding_transliterate_radio"));
    FilenameCharacterSetDiscard = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                      "file_encoding_ignore_radio"));

    g_settings_bind_with_mapping (MainSettings, "rename-encoding",
                                  FilenameCharacterSetOther, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  FilenameCharacterSetOther, NULL);
    g_settings_bind_with_mapping (MainSettings, "rename-encoding",
                                  FilenameCharacterSetApproximate,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  FilenameCharacterSetApproximate, NULL);
    g_settings_bind_with_mapping (MainSettings, "rename-encoding",
                                  FilenameCharacterSetDiscard, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  FilenameCharacterSetDiscard, NULL);

    /*
     * Tag Settings
     */
    /* Tag Options */
    DateAutoCompletion = GTK_WIDGET (gtk_builder_get_object (builder,
                                                             "tags_auto_date_check"));
    g_settings_bind (MainSettings, "tag-date-autocomplete", DateAutoCompletion,
                     "active", G_SETTINGS_BIND_DEFAULT);

    widget = GTK_WIDGET (gtk_builder_get_object (builder,
                                                 "tags_auto_image_type_check"));
    g_settings_bind (MainSettings, "tag-image-type-automatic", widget,
                     "active", G_SETTINGS_BIND_DEFAULT);

    /* Track formatting. */
    NumberTrackFormated = GTK_WIDGET (gtk_builder_get_object (builder,
                                                              "tags_track_check"));
    g_settings_bind (MainSettings, "tag-number-padded", NumberTrackFormated,
                     "active", G_SETTINGS_BIND_DEFAULT);

    NumberTrackFormatedSpinButton = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                        "tags_track_button"));
    g_settings_bind (MainSettings, "tag-number-length",
                     NumberTrackFormatedSpinButton, "value",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "tag-number-padded",
                     NumberTrackFormatedSpinButton, "sensitive",
                     G_SETTINGS_BIND_GET);

    /* Disc formatting. */
    pad_disc_number = GTK_WIDGET (gtk_builder_get_object (builder,
                                                          "tags_disc_check"));
    g_settings_bind (MainSettings, "tag-disc-padded", pad_disc_number,
                     "active", G_SETTINGS_BIND_DEFAULT);

    pad_disc_number_spinbutton = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                     "tags_disc_button"));
    g_settings_bind (MainSettings, "tag-disc-length",
                     pad_disc_number_spinbutton, "value",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "tag-disc-padded",
                     pad_disc_number_spinbutton, "sensitive",
                     G_SETTINGS_BIND_GET);
    g_signal_emit_by_name (G_OBJECT (pad_disc_number), "toggled");

    /* Tag field focus */
    SetFocusToSameTagField = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                 "tags_preserve_focus_check"));
    g_settings_bind (MainSettings, "tag-preserve-focus", SetFocusToSameTagField,
                     "active", G_SETTINGS_BIND_DEFAULT);

    /* Tag Splitting */
    VorbisSplitFieldTitle = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                "split_title_check"));
    VorbisSplitFieldArtist = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                 "split_artist_check"));
    VorbisSplitFieldAlbum = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                "split_album_check"));
    VorbisSplitFieldGenre = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                "split_genre_check"));
    VorbisSplitFieldComment = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                  "split_comment_check"));
    VorbisSplitFieldComposer = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                   "split_composer_check"));
    VorbisSplitFieldOrigArtist = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                     "split_orig_artist_check"));

    g_settings_bind (MainSettings, "ogg-split-title", VorbisSplitFieldTitle,
                     "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "ogg-split-artist", VorbisSplitFieldArtist,
                     "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "ogg-split-album", VorbisSplitFieldAlbum,
                     "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "ogg-split-genre", VorbisSplitFieldGenre,
                     "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "ogg-split-comment", VorbisSplitFieldComment,
                     "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "ogg-split-composer",
                     VorbisSplitFieldComposer, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "ogg-split-original-artist",
                     VorbisSplitFieldOrigArtist, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /*
     * ID3 Tag Settings
     */
    /* Strip tag when fields (managed by EasyTAG) are empty */
    StripTagWhenEmptyFields = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                  "id3_strip_check"));
    g_settings_bind (MainSettings, "id3-strip-empty", StripTagWhenEmptyFields,
                     "active", G_SETTINGS_BIND_DEFAULT);

    /* Convert old ID3v2 tag version */
    priv->ConvertOldId3v2TagVersion = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                          "id3_v2_convert_check"));
    g_settings_bind (MainSettings, "id3v2-convert-old",
                     priv->ConvertOldId3v2TagVersion, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Use CRC32 */
    priv->FileWritingId3v2UseCrc32 = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                         "id3_v2_crc32_check"));
    g_settings_bind (MainSettings, "id3v2-crc32", priv->FileWritingId3v2UseCrc32,
                     "active", G_SETTINGS_BIND_DEFAULT);

    /* Use Compression */
    priv->FileWritingId3v2UseCompression = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                               "id3_v2_compression_check"));
    g_settings_bind (MainSettings, "id3v2-compression",
                     priv->FileWritingId3v2UseCompression, "active",
                     G_SETTINGS_BIND_DEFAULT);
	
    /* Write Genre in text */
    priv->FileWritingId3v2TextOnlyGenre = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                              "id3_v2_genre_check"));
    g_settings_bind (MainSettings, "id3v2-text-only-genre",
                     priv->FileWritingId3v2TextOnlyGenre, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Write ID3v2 tag */
    FileWritingId3v2WriteTag = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                   "id3_v2_check"));
    g_settings_bind (MainSettings, "id3v2-enabled", FileWritingId3v2WriteTag,
                     "active", G_SETTINGS_BIND_DEFAULT);
    g_signal_connect (FileWritingId3v2WriteTag, "notify::active",
                      G_CALLBACK (notify_id3_settings_active), self);

#ifdef ENABLE_ID3LIB
    /* ID3v2 tag version */
    priv->LabelId3v2Version = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                  "id3_v2_version_label"));
    priv->FileWritingId3v2VersionCombo = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                             "id3_v2_version_combo"));
    g_settings_bind_with_mapping (MainSettings, "id3v2-version-4",
                                  priv->FileWritingId3v2VersionCombo, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_preferences_id3v2_version_get,
                                  et_preferences_id3v2_version_set, self,
                                  NULL);
    g_signal_connect (MainSettings, "changed::id3v2-version-4",
                      G_CALLBACK (notify_id3_settings_active), self);
#endif

    /* Charset */
    priv->LabelId3v2Charset = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                  "id3_v2_encoding_label"));
    /* Unicode. */
    priv->FileWritingId3v2UseUnicodeCharacterSet = GTK_WIDGET (gtk_builder_get_object (builder, "id3_v2_unicode_radio"));
    g_settings_bind (MainSettings, "id3v2-enable-unicode",
                     priv->FileWritingId3v2UseUnicodeCharacterSet, "active",
                     G_SETTINGS_BIND_DEFAULT);

    priv->FileWritingId3v2UnicodeCharacterSetCombo = GTK_WIDGET (gtk_builder_get_object (builder, "id3_v2_unicode_encoding_combo"));
    g_settings_bind_with_mapping (MainSettings, "id3v2-unicode-charset",
                                  priv->FileWritingId3v2UnicodeCharacterSetCombo,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_preferences_id3v2_unicode_charset_get,
                                  et_preferences_id3v2_unicode_charset_set,
                                  NULL, NULL);
    g_signal_connect (priv->FileWritingId3v2UseUnicodeCharacterSet,
                      "notify::active",
                      G_CALLBACK (notify_id3_settings_active), self);

    /* Non-Unicode. */
    priv->FileWritingId3v2UseNoUnicodeCharacterSet = GTK_WIDGET (gtk_builder_get_object (builder, "id3_v2_other_radio"));
    priv->FileWritingId3v2NoUnicodeCharacterSetCombo = GTK_WIDGET (gtk_builder_get_object (builder, "id3_v2_override_encoding_combo"));
    Charset_Populate_Combobox (GTK_COMBO_BOX (priv->FileWritingId3v2NoUnicodeCharacterSetCombo), 
                               g_settings_get_enum (MainSettings,
                                                    "id3v2-no-unicode-charset"));
    g_settings_bind_with_mapping (MainSettings, "id3v2-no-unicode-charset",
                                  priv->FileWritingId3v2NoUnicodeCharacterSetCombo,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_get, et_settings_enum_set,
                                  GSIZE_TO_POINTER (ET_TYPE_CHARSET), NULL);
    g_signal_connect (priv->FileWritingId3v2UseNoUnicodeCharacterSet,
                      "notify::active",
                      G_CALLBACK (notify_id3_settings_active), self);

    /* ID3v2 Additional iconv() options. */
    priv->LabelAdditionalId3v2IconvOptions = GTK_WIDGET (gtk_builder_get_object (builder, "id3_v2_iconv_label"));
    priv->FileWritingId3v2IconvOptionsNo = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                               "id3_v2_none_radio"));
    priv->FileWritingId3v2IconvOptionsTranslit = GTK_WIDGET (gtk_builder_get_object (builder, "id3_v2_transliterate_radio")),
    priv->FileWritingId3v2IconvOptionsIgnore = GTK_WIDGET (gtk_builder_get_object (builder, "id3_v2_ignore_radio"));

    g_settings_bind_with_mapping (MainSettings, "id3v2-encoding-option",
                                  priv->FileWritingId3v2IconvOptionsNo, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->FileWritingId3v2IconvOptionsNo, NULL);
    g_settings_bind_with_mapping (MainSettings, "id3v2-encoding-option",
                                  priv->FileWritingId3v2IconvOptionsTranslit,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->FileWritingId3v2IconvOptionsTranslit, NULL);
    g_settings_bind_with_mapping (MainSettings, "id3v2-encoding-option",
                                  priv->FileWritingId3v2IconvOptionsIgnore, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->FileWritingId3v2IconvOptionsIgnore, NULL);

    /* Write ID3v1 tag */
    FileWritingId3v1WriteTag = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                   "id3_v1_check"));
    g_settings_bind (MainSettings, "id3v1-enabled", FileWritingId3v1WriteTag,
                     "active", G_SETTINGS_BIND_DEFAULT);
    g_signal_connect (FileWritingId3v1WriteTag, "notify::active",
                      G_CALLBACK (notify_id3_settings_active), self);

    priv->id3_v1_encoding_grid = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                     "id3_v1_encoding_grid"));
    /* Id3V1 writing character set */
    priv->FileWritingId3v1CharacterSetCombo = GTK_WIDGET (gtk_builder_get_object (builder, "id3_v1_encoding_combo"));
    Charset_Populate_Combobox (GTK_COMBO_BOX (priv->FileWritingId3v1CharacterSetCombo),
                               g_settings_get_enum (MainSettings,
                                                    "id3v1-charset"));
    g_settings_bind_with_mapping (MainSettings, "id3v1-charset",
                                  priv->FileWritingId3v1CharacterSetCombo,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_get, et_settings_enum_set,
                                  GSIZE_TO_POINTER (ET_TYPE_CHARSET), NULL);

    /* ID3V1 Additional iconv() options*/
    priv->FileWritingId3v1IconvOptionsNo = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                               "id3_v1_none_radio"));
    priv->FileWritingId3v1IconvOptionsTranslit = GTK_WIDGET (gtk_builder_get_object (builder, "id3_v1_transliterate_radio"));
    priv->FileWritingId3v1IconvOptionsIgnore = GTK_WIDGET (gtk_builder_get_object (builder, "id3_v1_ignore_radio"));

    g_settings_bind_with_mapping (MainSettings, "id3v1-encoding-option",
                                  priv->FileWritingId3v1IconvOptionsNo, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->FileWritingId3v1IconvOptionsNo, NULL);
    g_settings_bind_with_mapping (MainSettings, "id3v1-encoding-option",
                                  priv->FileWritingId3v1IconvOptionsTranslit,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->FileWritingId3v1IconvOptionsTranslit, NULL);
    g_settings_bind_with_mapping (MainSettings, "id3v1-encoding-option",
                                  priv->FileWritingId3v1IconvOptionsIgnore, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  priv->FileWritingId3v1IconvOptionsIgnore, NULL);

    /* Character Set for reading tag */
    /* "File Reading Charset" Check Button + Combo. */
    UseNonStandardId3ReadingCharacterSet = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                               "id3_read_encoding_check"));
    g_settings_bind (MainSettings, "id3-override-read-encoding",
                     UseNonStandardId3ReadingCharacterSet, "active",
                     G_SETTINGS_BIND_DEFAULT);

    priv->FileReadingId3v1v2CharacterSetCombo = GTK_WIDGET (gtk_builder_get_object (builder, "id3_read_encoding_combo"));
    Charset_Populate_Combobox (GTK_COMBO_BOX (priv->FileReadingId3v1v2CharacterSetCombo),
                               g_settings_get_enum (MainSettings,
                                                    "id3v1v2-charset"));
    g_settings_bind_with_mapping (MainSettings, "id3v1v2-charset",
                                  priv->FileReadingId3v1v2CharacterSetCombo,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_get, et_settings_enum_set,
                                  GSIZE_TO_POINTER (ET_TYPE_CHARSET), NULL);
    g_settings_bind (MainSettings, "id3-override-read-encoding",
                     priv->FileReadingId3v1v2CharacterSetCombo, "sensitive",
                     G_SETTINGS_BIND_GET);
    notify_id3_settings_active (NULL, NULL, self);

    /*
     * Scanner
     */
    /* Save the number of the page. Asked in Scanner window */
    vbox = GTK_WIDGET (gtk_builder_get_object (builder, "scanner_grid"));
    priv->options_notebook_scanner = gtk_notebook_page_num (GTK_NOTEBOOK (priv->options_notebook),
                                                            vbox);

    /* Character conversion for the 'Fill Tag' scanner (=> FTS...) */
    FTSConvertUnderscoreAndP20IntoSpace = GTK_WIDGET (gtk_builder_get_object (builder, "fts_underscore_p20_radio"));
    FTSConvertSpaceIntoUnderscore = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                        "fts_spaces_radio"));
    FTSConvertSpaceNoChange = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                  "fts_none_radio"));
    g_settings_bind_with_mapping (MainSettings, "fill-convert-spaces",
                                  FTSConvertUnderscoreAndP20IntoSpace,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  FTSConvertUnderscoreAndP20IntoSpace, NULL);
    g_settings_bind_with_mapping (MainSettings, "fill-convert-spaces",
                                  FTSConvertSpaceIntoUnderscore, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  FTSConvertSpaceIntoUnderscore, NULL);
    g_settings_bind_with_mapping (MainSettings, "fill-convert-spaces",
                                  FTSConvertSpaceNoChange, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  FTSConvertSpaceNoChange, NULL);
    /* TODO: No change tooltip. */

    /* Character conversion for the 'Rename File' scanner (=> RFS...) */
    RFSConvertUnderscoreAndP20IntoSpace = GTK_WIDGET (gtk_builder_get_object (builder, "rfs_underscore_p20_radio"));
    RFSConvertSpaceIntoUnderscore = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                        "rfs_spaces_radio"));
    RFSRemoveSpaces = GTK_WIDGET (gtk_builder_get_object (builder,
                                                          "rfs_remove_radio"));
    g_settings_bind_with_mapping (MainSettings, "rename-convert-spaces",
                                  RFSConvertUnderscoreAndP20IntoSpace,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  RFSConvertUnderscoreAndP20IntoSpace, NULL);
    g_settings_bind_with_mapping (MainSettings, "rename-convert-spaces",
                                  RFSConvertSpaceIntoUnderscore, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set,
                                  RFSConvertSpaceIntoUnderscore, NULL);
    g_settings_bind_with_mapping (MainSettings, "rename-convert-spaces",
                                  RFSRemoveSpaces, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_radio_get,
                                  et_settings_enum_radio_set, RFSRemoveSpaces,
                                  NULL);

    /* Character conversion for the 'Process Fields' scanner (=> PFS...) */
    PFSDontUpperSomeWords = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                "pfs_uppercase_prep_check"));
    g_settings_bind (MainSettings, "process-uppercase-prepositions",
                     PFSDontUpperSomeWords, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Other options */
    OverwriteTagField = GTK_WIDGET (gtk_builder_get_object (builder,
                                                            "overwrite_fields_check"));
    g_settings_bind (MainSettings, "fill-overwrite-tag-fields",
                     OverwriteTagField, "active", G_SETTINGS_BIND_DEFAULT);

    /* Set a default comment text or CRC-32 checksum. */
    SetDefaultComment = GTK_WIDGET (gtk_builder_get_object (builder,
                                                            "default_comment_check"));
    DefaultComment = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "default_comment_entry"));
    g_settings_bind (MainSettings, "fill-set-default-comment",
                     SetDefaultComment, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "fill-set-default-comment", DefaultComment,
                     "sensitive", G_SETTINGS_BIND_GET);
    g_settings_bind (MainSettings, "fill-default-comment", DefaultComment,
                     "text", G_SETTINGS_BIND_DEFAULT);

    /* CRC32 comment. */
    Crc32Comment = GTK_WIDGET (gtk_builder_get_object (builder,
                                                       "crc32_default_check"));
    g_settings_bind (MainSettings, "fill-crc32-comment", Crc32Comment,
                     "active", G_SETTINGS_BIND_DEFAULT);

    /*
     * CDDB
     */
    /* 1st automatic search server. */
    CddbServerNameAutomaticSearch = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                        "cddb_automatic_host1_combo"));
    g_settings_bind (MainSettings, "cddb-automatic-search-hostname",
                     gtk_bin_get_child (GTK_BIN (CddbServerNameAutomaticSearch)),
                     "text", G_SETTINGS_BIND_DEFAULT);

    CddbServerPortAutomaticSearch = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                        "cddb_automatic_port1_button"));
    g_settings_bind (MainSettings, "cddb-automatic-search-port",
                     CddbServerPortAutomaticSearch, "value",
                     G_SETTINGS_BIND_DEFAULT);

    CddbServerCgiPathAutomaticSearch = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                           "cddb_automatic_path1_entry"));
    g_settings_bind (MainSettings, "cddb-automatic-search-path",
                     CddbServerCgiPathAutomaticSearch, "text",
                     G_SETTINGS_BIND_DEFAULT);

    /* 2nd automatic search server. */
    CddbServerNameAutomaticSearch2 = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                         "cddb_automatic_host2_combo"));
    g_settings_bind (MainSettings, "cddb-automatic-search-hostname2",
                     gtk_bin_get_child (GTK_BIN (CddbServerNameAutomaticSearch2)),
                     "text", G_SETTINGS_BIND_DEFAULT);

    CddbServerPortAutomaticSearch2 = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                         "cddb_automatic_port2_button"));
    g_settings_bind (MainSettings, "cddb-automatic-search-port2",
                     CddbServerPortAutomaticSearch2, "value",
                     G_SETTINGS_BIND_DEFAULT);

    CddbServerCgiPathAutomaticSearch2 = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                            "cddb_automatic_path2_entry"));
    g_settings_bind (MainSettings, "cddb-automatic-search-path2",
                     CddbServerCgiPathAutomaticSearch2, "text",
                     G_SETTINGS_BIND_DEFAULT);

    /* CDDB Server Settings (Manual Search). */
    CddbServerNameManualSearch = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                     "cddb_manual_host_combo"));
    g_settings_bind (MainSettings, "cddb-manual-search-hostname",
                     gtk_bin_get_child (GTK_BIN (CddbServerNameManualSearch)),
                     "text", G_SETTINGS_BIND_DEFAULT);

    CddbServerPortManualSearch = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                     "cddb_manual_port_button"));
    g_settings_bind (MainSettings, "cddb-manual-search-port",
                     CddbServerPortManualSearch, "value",
                     G_SETTINGS_BIND_DEFAULT);

    CddbServerCgiPathManualSearch = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                        "cddb_manual_path_entry"));
    g_settings_bind (MainSettings, "cddb-manual-search-path",
                     CddbServerCgiPathManualSearch, "text",
                     G_SETTINGS_BIND_DEFAULT);

    /* CDDB Proxy Settings. */
    CddbUseProxy = GTK_WIDGET (gtk_builder_get_object (builder,
                                                       "cddb_proxy_check"));
    g_settings_bind (MainSettings, "cddb-proxy-enabled", CddbUseProxy, "active",
                     G_SETTINGS_BIND_DEFAULT);

    CddbProxyName = GTK_WIDGET (gtk_builder_get_object (builder,
                                                        "cddb_host_entry"));
    g_settings_bind (MainSettings, "cddb-proxy-hostname",
                     CddbProxyName, "text", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "cddb-proxy-enabled", CddbProxyName,
                     "sensitive", G_SETTINGS_BIND_GET);
    CddbProxyPort = GTK_WIDGET (gtk_builder_get_object (builder,
                                                        "cddb_port_button"));
    g_settings_bind (MainSettings, "cddb-proxy-port", CddbProxyPort, "value",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "cddb-proxy-enabled", CddbProxyPort,
                     "sensitive", G_SETTINGS_BIND_GET);
    CddbProxyUserName = GTK_WIDGET (gtk_builder_get_object (builder,
                                                            "cddb_user_entry"));
    g_settings_bind (MainSettings, "cddb-proxy-username", CddbProxyUserName,
                     "text", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "cddb-proxy-enabled", CddbProxyUserName,
                     "sensitive", G_SETTINGS_BIND_GET);
    CddbProxyUserPassword = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                "cddb_password_entry"));
    g_settings_bind (MainSettings, "cddb-proxy-password", CddbProxyUserPassword,
                     "text", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "cddb-proxy-enabled", CddbProxyUserPassword,
                     "sensitive", G_SETTINGS_BIND_GET);

    /* Track Name list (CDDB results). */
    CddbFollowFile = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "cddb_follow_check"));
    g_settings_bind (MainSettings, "cddb-follow-file", CddbFollowFile,
                     "active", G_SETTINGS_BIND_DEFAULT);

    /* Check box to use DLM. */
    CddbUseDLM = GTK_WIDGET (gtk_builder_get_object (builder,
                                                     "cddb_dlm_check"));
    g_settings_bind (MainSettings, "cddb-dlm-enabled", CddbUseDLM, "active",
                     G_SETTINGS_BIND_DEFAULT);


    /*
     * Confirmation
     */
    ConfirmBeforeExit = GTK_WIDGET (gtk_builder_get_object (builder,
                                                            "confirm_quit_check"));
    g_settings_bind (MainSettings, "confirm-quit", ConfirmBeforeExit, "active",
                     G_SETTINGS_BIND_DEFAULT);

    ConfirmWriteTag = GTK_WIDGET (gtk_builder_get_object (builder,
                                                          "confirm_write_check"));
    g_settings_bind (MainSettings, "confirm-write-tags", ConfirmWriteTag,
                     "active", G_SETTINGS_BIND_DEFAULT);

    ConfirmRenameFile = GTK_WIDGET (gtk_builder_get_object (builder,
                                                            "confirm_rename_check"));
    g_settings_bind (MainSettings, "confirm-rename-file", ConfirmRenameFile,
                     "active", G_SETTINGS_BIND_DEFAULT);

    ConfirmDeleteFile = GTK_WIDGET (gtk_builder_get_object (builder,
                                                            "confirm_delete_check"));
    g_settings_bind (MainSettings, "confirm-delete-file", ConfirmDeleteFile,
                     "active", G_SETTINGS_BIND_DEFAULT);

    ConfirmWritePlayList = GTK_WIDGET (gtk_builder_get_object (builder,
                                                               "confirm_write_playlist_check"));
    g_settings_bind (MainSettings, "confirm-write-playlist",
                     ConfirmWritePlayList, "active", G_SETTINGS_BIND_DEFAULT);

    ConfirmWhenUnsavedFiles = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                  "confirm_unsaved_files_check"));
    g_settings_bind (MainSettings, "confirm-when-unsaved-files",
                     ConfirmWhenUnsavedFiles, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Properties of the scanner window */
    OpenScannerWindowOnStartup = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                     "scanner_dialog_startup_check"));
    g_settings_bind (MainSettings, "scan-startup", OpenScannerWindowOnStartup,
                     "active", G_SETTINGS_BIND_DEFAULT);

    g_object_unref (builder);

    /* Load the default page */
    g_settings_bind (MainSettings, "preferences-page", priv->options_notebook,
                     "page", G_SETTINGS_BIND_DEFAULT);
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
        gtk_widget_set_sensitive (priv->LabelId3v2Charset, TRUE);

#ifdef ENABLE_ID3LIB
        gtk_widget_set_sensitive (priv->LabelId3v2Version, TRUE);
        gtk_widget_set_sensitive (priv->FileWritingId3v2VersionCombo, TRUE);

        if (!g_settings_get_boolean (MainSettings, "id3v2-version-4"))
        {
            /* When "ID3v2.3" is selected. */
            gtk_combo_box_set_active (GTK_COMBO_BOX (priv->FileWritingId3v2UnicodeCharacterSetCombo), 1);
            gtk_widget_set_sensitive (priv->FileWritingId3v2UnicodeCharacterSetCombo,
                                      FALSE);
        }
        else
        {
            /* When "ID3v2.4" is selected, set "UTF-8" as default value. */
            gtk_combo_box_set_active (GTK_COMBO_BOX (priv->FileWritingId3v2UnicodeCharacterSetCombo),
                                      0);
            gtk_widget_set_sensitive (priv->FileWritingId3v2UnicodeCharacterSetCombo,
                                      active);
        }
#else 
        gtk_widget_set_sensitive (priv->FileWritingId3v2UnicodeCharacterSetCombo,
                                  active);
#endif
        gtk_widget_set_sensitive(priv->FileWritingId3v2UseUnicodeCharacterSet, TRUE);
        gtk_widget_set_sensitive(priv->FileWritingId3v2UseNoUnicodeCharacterSet, TRUE);
        gtk_widget_set_sensitive (priv->FileWritingId3v2NoUnicodeCharacterSetCombo,
                                  !active);
        gtk_widget_set_sensitive (priv->LabelAdditionalId3v2IconvOptions, !active);
        gtk_widget_set_sensitive (priv->FileWritingId3v2IconvOptionsNo, !active);
        gtk_widget_set_sensitive (priv->FileWritingId3v2IconvOptionsTranslit, !active);
        gtk_widget_set_sensitive (priv->FileWritingId3v2IconvOptionsIgnore, !active);
        gtk_widget_set_sensitive (priv->FileWritingId3v2UseCrc32, TRUE);
        gtk_widget_set_sensitive (priv->FileWritingId3v2UseCompression, TRUE);
        gtk_widget_set_sensitive (priv->FileWritingId3v2TextOnlyGenre, TRUE);
        gtk_widget_set_sensitive (priv->ConvertOldId3v2TagVersion, TRUE);

    }else
    {
        gtk_widget_set_sensitive (priv->LabelId3v2Charset, FALSE);
#ifdef ENABLE_ID3LIB
        gtk_widget_set_sensitive (priv->LabelId3v2Version, FALSE);
        gtk_widget_set_sensitive (priv->FileWritingId3v2VersionCombo, FALSE);
#endif
        gtk_widget_set_sensitive (priv->FileWritingId3v2UseUnicodeCharacterSet, FALSE);
        gtk_widget_set_sensitive (priv->FileWritingId3v2UseNoUnicodeCharacterSet, FALSE);
        gtk_widget_set_sensitive (priv->FileWritingId3v2UnicodeCharacterSetCombo,
                                  FALSE);
        gtk_widget_set_sensitive (priv->FileWritingId3v2NoUnicodeCharacterSetCombo,
                                  FALSE);
        gtk_widget_set_sensitive (priv->LabelAdditionalId3v2IconvOptions, FALSE);
        gtk_widget_set_sensitive (priv->FileWritingId3v2IconvOptionsNo, FALSE);
        gtk_widget_set_sensitive (priv->FileWritingId3v2IconvOptionsTranslit, FALSE);
        gtk_widget_set_sensitive (priv->FileWritingId3v2IconvOptionsIgnore, FALSE);
        gtk_widget_set_sensitive (priv->FileWritingId3v2UseCrc32, FALSE);
        gtk_widget_set_sensitive (priv->FileWritingId3v2UseCompression, FALSE);
        gtk_widget_set_sensitive (priv->FileWritingId3v2TextOnlyGenre, FALSE);
        gtk_widget_set_sensitive (priv->ConvertOldId3v2TagVersion, 0);
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

    gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->options_notebook),
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
et_preferences_dialog_finalize (GObject *object)
{
    G_OBJECT_CLASS (et_preferences_dialog_parent_class)->finalize (object);
}

static void
et_preferences_dialog_init (EtPreferencesDialog *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, ET_TYPE_PREFERENCES_DIALOG,
                                              EtPreferencesDialogPrivate);

    create_preferences_dialog (self);
}

static void
et_preferences_dialog_class_init (EtPreferencesDialogClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = et_preferences_dialog_finalize;

    g_type_class_add_private (klass, sizeof (EtPreferencesDialogPrivate));
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
    g_return_val_if_fail (GTK_WINDOW (parent), NULL);

    return g_object_new (ET_TYPE_PREFERENCES_DIALOG, "transient-for", parent,
                         NULL);
}
