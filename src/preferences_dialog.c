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
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "application_window.h"
#include "gtk2_compat.h"
#include "setting.h"
#include "bar.h"
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

    GtkWidget *LabelAdditionalId3v1IconvOptions;
    GtkWidget *LabelAdditionalId3v2IconvOptions;
    GtkWidget *LabelId3v2Charset;
    GtkWidget *LabelId3v1Charset;
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
static void Number_Track_Formatted_Spin_Button_Changed (GtkWidget *Label,
                                                        GtkWidget *SpinButton);
static void et_prefs_on_pad_disc_number_spinbutton_changed (GtkWidget *label,
                                                            GtkWidget *spinbutton);

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

    default_path = g_settings_get_value (settings, "default-path");
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

static void
id3_settings_changed (GtkComboBox *combo,
                      EtPreferencesDialog *self)
{
    notify_id3_settings_active (NULL, NULL, self);
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
    GtkWidget *OptionsVBox;
    GtkWidget *Label;
    GtkWidget *Frame;
    GtkWidget *Table;
    GtkWidget *VBox, *vbox;
    GtkWidget *HBox, *hbox, *id3v1v2hbox;
    GtkWidget *Separator;
    GtkWidget *LoadOnStartup;
    GtkWidget *BrowseSubdir;
    GtkWidget *OpenSelectedBrowserNode;
    GtkWidget *BrowseHiddendir;
    GtkWidget *ShowHeaderInfos;
    GtkWidget *ChangedFilesDisplayedToRed;
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

    priv = et_preferences_dialog_get_instance_private (self);

    /* The window */
    gtk_window_set_title (GTK_WINDOW (self), _("Preferences"));
    gtk_window_set_transient_for (GTK_WINDOW (self), GTK_WINDOW (MainWindow));
    gtk_window_set_destroy_with_parent (GTK_WINDOW (self), TRUE);
    gtk_dialog_add_buttons (GTK_DIALOG (self), GTK_STOCK_CLOSE,
                            GTK_RESPONSE_CLOSE, NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_CLOSE);
    g_signal_connect (self, "response",
                      G_CALLBACK (et_preferences_on_response), NULL);
    g_signal_connect (self, "delete-event",
                      G_CALLBACK (gtk_widget_hide_on_delete), NULL);

    gtk_container_set_border_width (GTK_CONTAINER (self), BOX_SPACING);

     /* Options */
     /* The vbox */
    OptionsVBox = gtk_dialog_get_content_area (GTK_DIALOG (self));
    gtk_box_set_spacing (GTK_BOX (OptionsVBox), BOX_SPACING);

     /* Options NoteBook */
    priv->options_notebook = gtk_notebook_new();
    gtk_notebook_popup_enable(GTK_NOTEBOOK(priv->options_notebook));
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(priv->options_notebook),TRUE);
    gtk_box_pack_start(GTK_BOX(OptionsVBox),priv->options_notebook,TRUE,TRUE,0);



    /*
     * Browser
     */
    Label = gtk_label_new(_("Browser"));
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_notebook_append_page (GTK_NOTEBOOK (priv->options_notebook), vbox, Label);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BOX_SPACING);

    /* Default directory */
    HBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_SPACING);
    gtk_box_pack_start(GTK_BOX(vbox),HBox,FALSE,FALSE,0);

    /* Label. */
    Label = gtk_label_new(_("Default directory:"));
    gtk_box_pack_start(GTK_BOX(HBox),Label,FALSE,FALSE,0);

    default_path_button = gtk_file_chooser_button_new (_("Default Browser Path"),
                                                       GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    on_default_path_changed (MainSettings, "changed::default-path",
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
    gtk_box_pack_start (GTK_BOX (HBox), default_path_button, TRUE, TRUE, 0);
    gtk_file_chooser_button_set_width_chars (GTK_FILE_CHOOSER_BUTTON (default_path_button),
                                             30);
    gtk_widget_set_tooltip_text (default_path_button,
                                 _("Specify the directory where your files are"
                                 " located. This path will be loaded when"
                                 " EasyTAG starts without parameter."));

    /* Load directory on startup */
    LoadOnStartup = gtk_check_button_new_with_label(_("Load on startup the default directory or the directory passed as argument"));
    gtk_box_pack_start(GTK_BOX(vbox),LoadOnStartup,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "load-on-startup", LoadOnStartup, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(LoadOnStartup,_("Automatically search files, when EasyTAG starts, "
        "into the default directory. Note that this path may be overridden by the parameter "
        "passed to easytag (easytag /path_to/mp3_files)."));

    /* Browse subdirectories */
    BrowseSubdir = gtk_check_button_new_with_label(_("Search subdirectories"));
    gtk_box_pack_start(GTK_BOX(vbox),BrowseSubdir,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "browse-subdir", BrowseSubdir, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(BrowseSubdir,_("Search subdirectories for files when reading "
        "a directory into the tree."));

    /* Open the node to show subdirectories */
    OpenSelectedBrowserNode = gtk_check_button_new_with_label(_("Show subdirectories when selecting "
        "a directory"));
    gtk_box_pack_start(GTK_BOX(vbox),OpenSelectedBrowserNode,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "browse-expand-children",
                     OpenSelectedBrowserNode, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(OpenSelectedBrowserNode,_("This expands the selected node into the file "
        "browser to display the sub-directories."));

    /* Browse hidden directories */
    BrowseHiddendir = gtk_check_button_new_with_label(_("Search hidden directories"));
    g_settings_bind (MainSettings, "browse-show-hidden", BrowseHiddendir,
                     "active", G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(BrowseHiddendir,_("Search hidden directories for files "
        "(directories starting by a '.')."));



    /*
     * Misc
     */
    Label = gtk_label_new (_("Misc"));
    VBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_notebook_append_page (GTK_NOTEBOOK (priv->options_notebook), VBox, Label);
    gtk_container_set_border_width (GTK_CONTAINER (VBox), BOX_SPACING);

    /* User interface */
    Frame = gtk_frame_new (_("User Interface"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BOX_SPACING);

    // Show header infos
    ShowHeaderInfos = gtk_check_button_new_with_label(_("Show header information of file"));
    gtk_box_pack_start(GTK_BOX(vbox),ShowHeaderInfos,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "file-show-header", ShowHeaderInfos, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(ShowHeaderInfos,_("If activated, information about the file as "
        "the bitrate, the time, the size, will be displayed under the filename entry."));

    // Display color mode for changed files in list
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_SPACING);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    Label = gtk_label_new(_("Display changed files in list using:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    ChangedFilesDisplayedToRed = gtk_radio_button_new_with_label(NULL,_("Red color"));
    gtk_box_pack_start(GTK_BOX(hbox),ChangedFilesDisplayedToRed,FALSE,FALSE,4);

    // Set "new" Gtk+-2.0ish black/bold style for changed items
    ChangedFilesDisplayedToBold = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(ChangedFilesDisplayedToRed)),_("Bold style"));
    gtk_box_pack_start(GTK_BOX(hbox),ChangedFilesDisplayedToBold,FALSE,FALSE,2);
    g_settings_bind (MainSettings, "file-changed-bold",
                     ChangedFilesDisplayedToBold, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_signal_connect_swapped (ChangedFilesDisplayedToBold, "notify::active",
                              G_CALLBACK (et_application_window_browser_refresh_list),
                              MainWindow);

    /* Sorting List Options */
    Frame = gtk_frame_new (_("Sorting List Options"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);

    SortingFileCaseSensitive = gtk_check_button_new_with_label(_("Case sensitive"));
    gtk_container_add (GTK_CONTAINER (Frame), SortingFileCaseSensitive);
    g_settings_bind (MainSettings, "sort-case-sensitive",
                     SortingFileCaseSensitive, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(SortingFileCaseSensitive,_("If activated, the "
        "sorting of the list will be dependent on the case."));

    /* Log options */
    Frame = gtk_frame_new (_("Log Options"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BOX_SPACING);

    // Show / hide log view
    ShowLogView = gtk_check_button_new_with_label(_("Show log view in main window"));
    gtk_box_pack_start(GTK_BOX(vbox),ShowLogView,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "log-show", ShowLogView, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(ShowLogView,_("If activated, the log view would be "
                                            "visible in the main window."));
   
    // Max number of lines
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_SPACING);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    Label = gtk_label_new (_("Max number of lines:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);
    
    LogMaxLinesSpinButton = gtk_spin_button_new_with_range(10.0,1500.0,10.0);
    gtk_box_pack_start(GTK_BOX(hbox),LogMaxLinesSpinButton,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "log-lines", LogMaxLinesSpinButton,
                     "value", G_SETTINGS_BIND_DEFAULT);


    /*
     * File Settings
     */
    Label = gtk_label_new (_("File Settings"));
    VBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_notebook_append_page (GTK_NOTEBOOK (priv->options_notebook), VBox, Label);
    gtk_container_set_border_width (GTK_CONTAINER (VBox), BOX_SPACING);

    /* File (name) Options */
    Frame = gtk_frame_new (_("File Options"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width (GTK_CONTAINER(vbox), BOX_SPACING);

    ReplaceIllegalCharactersInFilename = gtk_check_button_new_with_label(_("Replace illegal characters in filename (for Windows and CD-Rom)"));
    gtk_box_pack_start(GTK_BOX(vbox),ReplaceIllegalCharactersInFilename,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "rename-replace-illegal-chars",
                     ReplaceIllegalCharactersInFilename, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(ReplaceIllegalCharactersInFilename,_("Convert illegal characters for "
        "FAT32/16 and ISO9660 + Joliet filesystems ('\\', ':', ';', '*', '?', '\"', '<', '>', '|') "
        "of the filename to avoid problem when renaming the file. This is useful when renaming the "
        "file from the tag with the scanner."));

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_SPACING);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    /* Extension case (lower/upper?) */
    Label = gtk_label_new(_("Convert filename extension to:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    FilenameExtensionLowerCase = gtk_radio_button_new_with_label(NULL,_("Lower Case"));
    gtk_widget_set_name (FilenameExtensionLowerCase, "lower-case");
    gtk_box_pack_start(GTK_BOX(hbox),FilenameExtensionLowerCase,FALSE,FALSE,2);
    gtk_widget_set_tooltip_text(FilenameExtensionLowerCase,_("For example, the extension will be converted to '.mp3'"));

    FilenameExtensionUpperCase = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(FilenameExtensionLowerCase)),_("Upper Case"));
    gtk_widget_set_name (FilenameExtensionUpperCase, "upper-case");
    gtk_box_pack_start(GTK_BOX(hbox),FilenameExtensionUpperCase,FALSE,FALSE,2);
    gtk_widget_set_tooltip_text(FilenameExtensionUpperCase,_("For example, the extension will be converted to '.MP3'"));

    FilenameExtensionNoChange = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(FilenameExtensionLowerCase)),_("No Change"));
    gtk_widget_set_name (FilenameExtensionNoChange, "no-change");
    gtk_box_pack_start(GTK_BOX(hbox),FilenameExtensionNoChange,FALSE,FALSE,2);
    gtk_widget_set_tooltip_text(FilenameExtensionNoChange,_("The extension will not be converted"));

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
    PreserveModificationTime = gtk_check_button_new_with_label(_("Preserve modification time of the file"));
    gtk_box_pack_start(GTK_BOX(vbox),PreserveModificationTime,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "file-preserve-modification-time",
                     PreserveModificationTime, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(PreserveModificationTime,_("Preserve the modification time "
        "(in file properties) when saving the file."));

    /* Change directory modification time */
    UpdateParentDirectoryModificationTime = gtk_check_button_new_with_label(_("Update modification time "
        "of the parent directory of the file (recommended when using Amarok)"));
    gtk_box_pack_start(GTK_BOX(vbox),UpdateParentDirectoryModificationTime,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "file-update-parent-modification-time",
                     UpdateParentDirectoryModificationTime, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(UpdateParentDirectoryModificationTime,_("The modification time "
        "of the parent directory of the file will be updated when saving tag the file. At the "
        "present time it is automatically done only when renaming a file.\nThis feature is "
        "interesting when using applications like Amarok. For performance reasons, they refresh "
        "file information by detecting changes of the parent directory."));


    /* Character Set for Filename */
    Frame = gtk_frame_new (_("Character Set for Filename"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BOX_SPACING);

    Table = et_grid_new (4, 2);
    gtk_box_pack_start(GTK_BOX(vbox),Table,FALSE,FALSE,0);
    /*gtk_grid_set_row_spacing (GTK_GRID (Table), 2);*/
    gtk_grid_set_column_spacing (GTK_GRID (Table), 2 * BOX_SPACING);

    /* Rules for character set */
    Label = gtk_label_new(_("Rules to apply if some characters can't be converted to "
        "the system character encoding when writing filename:"));
    gtk_grid_attach (GTK_GRID (Table), Label, 0, 0, 2, 1);
    gtk_widget_set_halign (Label, GTK_ALIGN_START);

    FilenameCharacterSetOther = gtk_radio_button_new_with_label(NULL,_("Try another "
        "character encoding"));
    gtk_grid_attach (GTK_GRID (Table), FilenameCharacterSetOther, 1, 1, 1, 1);
    gtk_widget_set_name (FilenameCharacterSetOther, "try-alternative");
    gtk_widget_set_tooltip_text(FilenameCharacterSetOther,_("With this option, it will "
        "try the conversion to the encoding associated to your locale (for example: "
        "ISO-8859-1 for 'fr', KOI8-R for 'ru', ISO-8859-2 for 'ro'). If it fails, it "
        "will try the character encoding ISO-8859-1."));

    FilenameCharacterSetApproximate = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(FilenameCharacterSetOther)),
        _("Force using the system character encoding and activate the transliteration"));
    gtk_grid_attach (GTK_GRID (Table), FilenameCharacterSetApproximate, 1, 2,
                     1, 1);
    gtk_widget_set_name (FilenameCharacterSetApproximate, "transliterate");
    gtk_widget_set_tooltip_text(FilenameCharacterSetApproximate,_("With this option, when "
        "a character cannot be represented in the target character set, it can be "
        "approximated through one or several similarly looking characters."));

    FilenameCharacterSetDiscard = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(FilenameCharacterSetOther)),
        _("Force using the system character encoding and silently discard some characters"));
    gtk_grid_attach (GTK_GRID (Table), FilenameCharacterSetDiscard, 1, 3, 1,
                     1);
    gtk_widget_set_name (FilenameCharacterSetDiscard, "ignore");
    gtk_widget_set_tooltip_text(FilenameCharacterSetDiscard,_("With this option, when "
        "a character cannot be represented in the target character set, it will "
        "be silently discarded."));

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
    Label = gtk_label_new (_("Tag Settings"));
    VBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_notebook_append_page (GTK_NOTEBOOK (priv->options_notebook), VBox, Label);
    gtk_container_set_border_width (GTK_CONTAINER (VBox), BOX_SPACING);

    /* Tag Options */
    Frame = gtk_frame_new (_("Tag Options"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BOX_SPACING);

    DateAutoCompletion = gtk_check_button_new_with_label(_("Auto completion of date if not complete"));
    gtk_box_pack_start(GTK_BOX(vbox),DateAutoCompletion,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "tag-date-autocomplete", DateAutoCompletion,
                     "active", G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(DateAutoCompletion,_("Try to complete the year field if you enter "
        "only the last numerals of the date (for instance, if the current year is 2005: "
        "5 => 2005, 4 => 2004, 6 => 1996, 95 => 1995…)."));

    /* Track formatting. */
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_SPACING);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);

    NumberTrackFormated = gtk_check_button_new_with_label(_("Write the track field with the following number of digits:"));
    gtk_box_pack_start(GTK_BOX(hbox),NumberTrackFormated,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "tag-number-padded", NumberTrackFormated,
                     "active", G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(NumberTrackFormated,_("If activated, the track field is written using "
        "the number '0' as padding to obtain a number with 'n' digits (for example, with two digits: '05', "
        "'09', '10'…). Else it keeps the 'raw' track value."));

    NumberTrackFormatedSpinButton = gtk_spin_button_new_with_range(2.0,6.0,1.0);
    gtk_box_pack_start(GTK_BOX(hbox),NumberTrackFormatedSpinButton,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "tag-number-length",
                     NumberTrackFormatedSpinButton, "value",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "tag-number-padded",
                     NumberTrackFormatedSpinButton, "sensitive",
                     G_SETTINGS_BIND_GET);

    Label = gtk_label_new(""); // Label to show the example
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,4);
    g_signal_connect_swapped (NumberTrackFormatedSpinButton, "changed",
                              G_CALLBACK (Number_Track_Formatted_Spin_Button_Changed),
                              Label);
    g_signal_emit_by_name(G_OBJECT(NumberTrackFormatedSpinButton),"changed",NULL);

    /* Disc formatting. */
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_SPACING);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    pad_disc_number = gtk_check_button_new_with_label (_("Write the disc field with the following number of digits:"));
    gtk_box_pack_start (GTK_BOX (hbox), pad_disc_number, FALSE, FALSE, 0);
    g_settings_bind (MainSettings, "tag-number-padded", pad_disc_number,
                     "active", G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text (pad_disc_number,
                                 _("Whether to pad the disc field with leading zeroes"));

    pad_disc_number_spinbutton = gtk_spin_button_new_with_range (1.0, 6.0,
                                                                 1.0);
    gtk_box_pack_start (GTK_BOX (hbox), pad_disc_number_spinbutton, FALSE,
                        FALSE, 0);
    g_settings_bind (MainSettings, "tag-number-length",
                     pad_disc_number_spinbutton, "value",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "tag-number-padded",
                     pad_disc_number_spinbutton, "sensitive",
                     G_SETTINGS_BIND_GET);
    g_signal_emit_by_name (G_OBJECT (pad_disc_number), "toggled");

    /* Label to show the example. */
    Label = gtk_label_new ("");
    gtk_box_pack_start (GTK_BOX (hbox), Label, FALSE, FALSE, BOX_SPACING);
    g_signal_connect_swapped ((pad_disc_number_spinbutton), "changed",
                              G_CALLBACK (et_prefs_on_pad_disc_number_spinbutton_changed),
                              Label);
    g_signal_emit_by_name (G_OBJECT (pad_disc_number_spinbutton), "changed");

    // Separator line
    Separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox),Separator,FALSE,FALSE,0);

    /* Tag field focus */
    SetFocusToSameTagField = gtk_check_button_new_with_label (_("Keep focus on the same tag field when switching files"));
    gtk_box_pack_start (GTK_BOX (vbox), SetFocusToSameTagField, FALSE, FALSE,
                        0);
    g_settings_bind (MainSettings, "tag-preserve-focus", SetFocusToSameTagField,
                     "active", G_SETTINGS_BIND_DEFAULT);

    /* Tag Splitting */
    Frame = gtk_frame_new (_("Tag Splitting"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);

    Table = et_grid_new (5, 2);
    gtk_container_add(GTK_CONTAINER(Frame),Table);
    gtk_container_set_border_width (GTK_CONTAINER (Table), BOX_SPACING);
    gtk_grid_set_column_spacing (GTK_GRID (Table), BOX_SPACING);
    gtk_grid_set_row_spacing (GTK_GRID (Table), BOX_SPACING);
    
    Label = gtk_label_new(_("For Vorbis tags, selected fields will be split at dashes and saved as separate tags"));
    gtk_grid_attach (GTK_GRID (Table), Label, 0, 0, 2, 1);
    gtk_widget_set_halign (Label, GTK_ALIGN_START);

    VorbisSplitFieldTitle = gtk_check_button_new_with_label(_("Title"));
    VorbisSplitFieldArtist = gtk_check_button_new_with_label(_("Artist"));
    VorbisSplitFieldAlbum = gtk_check_button_new_with_label(_("Album"));
    VorbisSplitFieldGenre = gtk_check_button_new_with_label(_("Genre"));
    VorbisSplitFieldComment = gtk_check_button_new_with_label(_("Comment"));
    VorbisSplitFieldComposer = gtk_check_button_new_with_label(_("Composer"));
    VorbisSplitFieldOrigArtist = gtk_check_button_new_with_label(_("Original artist"));

    gtk_grid_attach (GTK_GRID (Table), VorbisSplitFieldTitle, 0, 1, 1, 1);
    gtk_grid_attach (GTK_GRID (Table), VorbisSplitFieldArtist, 0, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (Table), VorbisSplitFieldAlbum, 0, 3, 1, 1);
    gtk_grid_attach (GTK_GRID (Table), VorbisSplitFieldGenre, 0, 4, 1, 1);
    gtk_grid_attach (GTK_GRID (Table), VorbisSplitFieldComment, 1, 1, 1, 1);
    gtk_grid_attach (GTK_GRID (Table), VorbisSplitFieldComposer, 1, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (Table), VorbisSplitFieldOrigArtist, 1, 3, 1, 1);

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
    Label = gtk_label_new (_("ID3 Tag Settings"));
    VBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
#ifdef ENABLE_MP3
    gtk_notebook_append_page (GTK_NOTEBOOK (priv->options_notebook), VBox, Label);
#endif
    gtk_container_set_border_width (GTK_CONTAINER (VBox), BOX_SPACING);


    /* Tag Rules frame */
    Frame = gtk_frame_new (_("ID3 Tag Rules"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BOX_SPACING);

    Table = et_grid_new (3, 2);
    gtk_box_pack_start(GTK_BOX(vbox),Table,FALSE,FALSE,0);
    gtk_grid_set_row_spacing (GTK_GRID (Table), BOX_SPACING);
    gtk_grid_set_column_spacing (GTK_GRID (Table), BOX_SPACING);

    /* Strip tag when fields (managed by EasyTAG) are empty */
    StripTagWhenEmptyFields = gtk_check_button_new_with_label(_("Strip tags if all fields are set to blank"));
    gtk_grid_attach (GTK_GRID (Table), StripTagWhenEmptyFields, 0, 0, 1, 1);
    g_settings_bind (MainSettings, "id3-strip-empty", StripTagWhenEmptyFields,
                     "active", G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(StripTagWhenEmptyFields,_("As ID3v2 tags may contain other data than "
        "Title, Artist, Album, Year, Track, Genre or Comment (as an attached image, lyrics…), "
        "this option allows you to strip the whole tag when these seven standard data fields have "
        "been set to blank."));

    /* Convert old ID3v2 tag version */
    priv->ConvertOldId3v2TagVersion = gtk_check_button_new_with_label (_("Automatically convert old ID3v2 tag versions"));
    gtk_grid_attach (GTK_GRID (Table), priv->ConvertOldId3v2TagVersion, 0, 1, 1, 1);
    g_settings_bind (MainSettings, "id3v2-convert-old",
                     priv->ConvertOldId3v2TagVersion, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text (priv->ConvertOldId3v2TagVersion,_("If activated, an old ID3v2 tag version (as "
        "ID3v2.2) will be updated to the ID3v2.3 version."));

    /* Use CRC32 */
    priv->FileWritingId3v2UseCrc32 = gtk_check_button_new_with_label (_("Use CRC32"));
    gtk_grid_attach (GTK_GRID (Table), priv->FileWritingId3v2UseCrc32, 1, 0, 1, 1);
    g_settings_bind (MainSettings, "id3v2-crc32", priv->FileWritingId3v2UseCrc32,
                     "active", G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text (priv->FileWritingId3v2UseCrc32,_("Set CRC32 in the ID3v2 tags"));

    /* Use Compression */
    priv->FileWritingId3v2UseCompression = gtk_check_button_new_with_label(_("Use Compression"));
    gtk_grid_attach (GTK_GRID (Table), priv->FileWritingId3v2UseCompression, 1, 1, 1,
                     1);
    g_settings_bind (MainSettings, "id3v2-compression",
                     priv->FileWritingId3v2UseCompression, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text (priv->FileWritingId3v2UseCompression,_("Set Compression in the ID3v2 tags"));
	
    /* Write Genre in text */
    priv->FileWritingId3v2TextOnlyGenre = gtk_check_button_new_with_label(_("Write Genre in text only"));
    gtk_grid_attach (GTK_GRID (Table), priv->FileWritingId3v2TextOnlyGenre, 0, 2, 1,
                     1);
    g_settings_bind (MainSettings, "id3v2-text-only-genre",
                     priv->FileWritingId3v2TextOnlyGenre, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text (priv->FileWritingId3v2TextOnlyGenre,
                                 _("Don't use ID3v1 number references in genre tag. Enable this if you see numbers as genre in your music player."));	

    /* Character Set for writing ID3 tag */
    Frame = gtk_frame_new (_("Character Set for writing ID3 tags"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    id3v1v2hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),id3v1v2hbox);
    gtk_container_set_border_width (GTK_CONTAINER (id3v1v2hbox), BOX_SPACING);

    // ID3v2 tags
    Frame = gtk_frame_new (_("ID3v2 tags"));
    gtk_box_pack_start(GTK_BOX(id3v1v2hbox),Frame,FALSE,FALSE,2);

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BOX_SPACING);

    Table = et_grid_new (8, 6);
    gtk_box_pack_start(GTK_BOX(vbox),Table,FALSE,FALSE,0);
    gtk_grid_set_row_spacing (GTK_GRID (Table), BOX_SPACING);
    gtk_grid_set_column_spacing (GTK_GRID (Table), BOX_SPACING);

    /* Write ID3v2 tag */
    FileWritingId3v2WriteTag = gtk_check_button_new_with_label(_("Write ID3v2 tag"));
    gtk_grid_attach (GTK_GRID (Table), FileWritingId3v2WriteTag, 0, 0, 5, 1);
    g_settings_bind (MainSettings, "id3v2-enabled", FileWritingId3v2WriteTag,
                     "active", G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(FileWritingId3v2WriteTag,_("If activated, an ID3v2.4 tag will be added or "
        "updated at the beginning of the MP3 files. Else it will be stripped."));
    g_signal_connect (FileWritingId3v2WriteTag, "notify::active",
                      G_CALLBACK (notify_id3_settings_active), self);

#ifdef ENABLE_ID3LIB
    /* ID3v2 tag version */
    priv->LabelId3v2Version = gtk_label_new (_("Version:"));
    gtk_grid_attach (GTK_GRID (Table), priv->LabelId3v2Version, 0, 1, 2, 1);
    gtk_widget_set_halign (priv->LabelId3v2Version, GTK_ALIGN_START);

    priv->FileWritingId3v2VersionCombo = gtk_combo_box_text_new ();
    gtk_widget_set_tooltip_text (priv->FileWritingId3v2VersionCombo,
                                 _("Select the ID3v2 tag version to write:\n"
                                   " - ID3v2.3 is written using id3lib,\n"
                                   " - ID3v2.4 is written using libid3tag (recommended)."));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (priv->FileWritingId3v2VersionCombo), "ID3v2.4");
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (priv->FileWritingId3v2VersionCombo), "ID3v2.3");
    g_settings_bind_with_mapping (MainSettings, "id3v2-version-4",
                                  priv->FileWritingId3v2VersionCombo, "active",
                                  G_SETTINGS_BIND_DEFAULT,
                                  et_preferences_id3v2_version_get,
                                  et_preferences_id3v2_version_set, self,
                                  NULL);
    gtk_grid_attach (GTK_GRID (Table), priv->FileWritingId3v2VersionCombo, 2, 1, 2,
                     1);
    g_signal_connect (MainSettings, "changed::id3v2-version-4",
                      G_CALLBACK (notify_id3_settings_active), self);
#endif


    /* Charset */
    priv->LabelId3v2Charset = gtk_label_new (_("Charset:"));
    gtk_grid_attach (GTK_GRID (Table), priv->LabelId3v2Charset, 0, 2, 5, 1);
    gtk_widget_set_halign (priv->LabelId3v2Charset, GTK_ALIGN_START);

    /* Unicode. */
    priv->FileWritingId3v2UseUnicodeCharacterSet = gtk_radio_button_new_with_label (NULL, _("Unicode "));
    g_settings_bind (MainSettings, "id3v2-enable-unicode",
                     priv->FileWritingId3v2UseUnicodeCharacterSet, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_grid_attach (GTK_GRID (Table), priv->FileWritingId3v2UseUnicodeCharacterSet,
                     1, 3, 1, 1);

    priv->FileWritingId3v2UnicodeCharacterSetCombo = gtk_combo_box_text_new ();
    gtk_widget_set_tooltip_text (priv->FileWritingId3v2UnicodeCharacterSetCombo,
                                 _("Unicode type to use"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (priv->FileWritingId3v2UnicodeCharacterSetCombo),
                                    "UTF-8");
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (priv->FileWritingId3v2UnicodeCharacterSetCombo),
                                    "UTF-16");
    g_settings_bind_with_mapping (MainSettings, "id3v2-unicode-charset",
                                  priv->FileWritingId3v2UnicodeCharacterSetCombo,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_preferences_id3v2_unicode_charset_get,
                                  et_preferences_id3v2_unicode_charset_set,
                                  NULL, NULL);
    gtk_grid_attach (GTK_GRID (Table),
                     priv->FileWritingId3v2UnicodeCharacterSetCombo, 2, 3, 2,
                     1);
    g_signal_connect (priv->FileWritingId3v2UseUnicodeCharacterSet,
                      "notify::active",
                      G_CALLBACK (notify_id3_settings_active), self);

    /* Non-Unicode. */
    priv->FileWritingId3v2UseNoUnicodeCharacterSet = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(priv->FileWritingId3v2UseUnicodeCharacterSet)),
        _("Other"));
    gtk_grid_attach (GTK_GRID (Table),
                     priv->FileWritingId3v2UseNoUnicodeCharacterSet, 1, 4, 1, 1);

    priv->FileWritingId3v2NoUnicodeCharacterSetCombo = gtk_combo_box_text_new();
    gtk_widget_set_tooltip_text (priv->FileWritingId3v2NoUnicodeCharacterSetCombo,
                                 _("Character set used to write the tag data in the file."));

    Charset_Populate_Combobox (GTK_COMBO_BOX (priv->FileWritingId3v2NoUnicodeCharacterSetCombo), 
                               g_settings_get_enum (MainSettings,
                                                    "id3v2-no-unicode-charset"));
    gtk_grid_attach (GTK_GRID (Table),
                     priv->FileWritingId3v2NoUnicodeCharacterSetCombo, 2, 4, 3,
                     1);
    g_settings_bind_with_mapping (MainSettings, "id3v2-no-unicode-charset",
                                  priv->FileWritingId3v2NoUnicodeCharacterSetCombo,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_get, et_settings_enum_set,
                                  GSIZE_TO_POINTER (ET_TYPE_CHARSET), NULL);
    g_signal_connect (priv->FileWritingId3v2UseNoUnicodeCharacterSet,
                      "notify::active",
                      G_CALLBACK (notify_id3_settings_active), self);

    /* ID3v2 Additional iconv() options. */
    priv->LabelAdditionalId3v2IconvOptions = gtk_label_new (_("Additional settings for iconv():"));
    gtk_grid_attach (GTK_GRID (Table), priv->LabelAdditionalId3v2IconvOptions,
                     2, 5, 3, 1);
    gtk_widget_set_halign (priv->LabelAdditionalId3v2IconvOptions,
                           GTK_ALIGN_START);

    priv->FileWritingId3v2IconvOptionsNo = gtk_radio_button_new_with_label (NULL,
                                                                            _("No"));
    gtk_grid_attach (GTK_GRID (Table), priv->FileWritingId3v2IconvOptionsNo, 2, 6, 1,
                     1);
    gtk_widget_set_name (priv->FileWritingId3v2IconvOptionsNo, "none");
    gtk_widget_set_tooltip_text (priv->FileWritingId3v2IconvOptionsNo, _("With this option, when "
        "a character cannot be represented in the target character set, it isn't changed. "
        "But note that an error message will be displayed for information."));
    priv->FileWritingId3v2IconvOptionsTranslit = gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (priv->FileWritingId3v2IconvOptionsNo)),
        _("//TRANSLIT"));
    gtk_grid_attach (GTK_GRID (Table), priv->FileWritingId3v2IconvOptionsTranslit, 3,
                     6, 1, 1);
    gtk_widget_set_name (priv->FileWritingId3v2IconvOptionsTranslit,
                         "transliterate");
    gtk_widget_set_tooltip_text (priv->FileWritingId3v2IconvOptionsTranslit, _("With this option, when "
        "a character cannot be represented in the target character set, it can be "
        "approximated through one or several similarly looking characters."));

    priv->FileWritingId3v2IconvOptionsIgnore = gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (priv->FileWritingId3v2IconvOptionsNo)),
        _("//IGNORE"));
    gtk_grid_attach (GTK_GRID (Table), priv->FileWritingId3v2IconvOptionsIgnore, 4,
                     6, 1, 1);
    gtk_widget_set_name (priv->FileWritingId3v2IconvOptionsIgnore, "ignore");
    gtk_widget_set_tooltip_text (priv->FileWritingId3v2IconvOptionsIgnore, _("With this option, when "
        "a character cannot be represented in the target character set, it will "
        "be silently discarded."));

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

    // ID3v1 tags
    Frame = gtk_frame_new (_("ID3v1 tags"));
    gtk_box_pack_start(GTK_BOX(id3v1v2hbox),Frame,FALSE,FALSE,2);

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BOX_SPACING);

    Table = et_grid_new (6, 5);
    gtk_box_pack_start(GTK_BOX(vbox),Table,FALSE,FALSE,0);
    gtk_grid_set_row_spacing (GTK_GRID (Table), BOX_SPACING);
    gtk_grid_set_column_spacing (GTK_GRID (Table), BOX_SPACING);


    /* Write ID3v1 tag */
    FileWritingId3v1WriteTag = gtk_check_button_new_with_label(_("Write ID3v1.x tag"));
    gtk_grid_attach (GTK_GRID (Table), FileWritingId3v1WriteTag, 0, 0, 4, 1);
    g_settings_bind (MainSettings, "id3v1-enabled", FileWritingId3v1WriteTag,
                     "active", G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(FileWritingId3v1WriteTag,_("If activated, an ID3v1 tag will be added or "
        "updated at the end of the MP3 files. Else it will be stripped."));
    g_signal_connect (FileWritingId3v1WriteTag, "notify::active",
                      G_CALLBACK (notify_id3_settings_active), self);

    /* Id3V1 writing character set */
    priv->LabelId3v1Charset = gtk_label_new (_("Charset:"));
    gtk_grid_attach (GTK_GRID (Table), priv->LabelId3v1Charset, 0, 1, 4, 1);
    gtk_widget_set_halign (priv->LabelId3v1Charset, GTK_ALIGN_START);

    priv->FileWritingId3v1CharacterSetCombo = gtk_combo_box_text_new();
    gtk_grid_attach (GTK_GRID (Table), priv->FileWritingId3v1CharacterSetCombo,
                     1, 2, 3, 1);
    gtk_widget_set_tooltip_text (priv->FileWritingId3v1CharacterSetCombo,
                                 _("Character set used to write ID3v1 tag data in the file."));
    Charset_Populate_Combobox (GTK_COMBO_BOX (priv->FileWritingId3v1CharacterSetCombo),
                               g_settings_get_enum (MainSettings,
                                                    "id3v1-charset"));
    g_settings_bind_with_mapping (MainSettings, "id3v1-charset",
                                  priv->FileWritingId3v1CharacterSetCombo,
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  et_settings_enum_get, et_settings_enum_set,
                                  GSIZE_TO_POINTER (ET_TYPE_CHARSET), NULL);

    /* ID3V1 Additional iconv() options*/
    priv->LabelAdditionalId3v1IconvOptions = gtk_label_new (_("Additional settings for iconv():"));
    gtk_grid_attach (GTK_GRID (Table), priv->LabelAdditionalId3v1IconvOptions,
                     1, 3, 3, 1);
    gtk_widget_set_halign (priv->LabelAdditionalId3v1IconvOptions,
                           GTK_ALIGN_START);

    priv->FileWritingId3v1IconvOptionsNo = gtk_radio_button_new_with_label (NULL,
                                                                            _("No"));
    gtk_grid_attach (GTK_GRID (Table), priv->FileWritingId3v1IconvOptionsNo, 1, 4, 1,
                     1);
    gtk_widget_set_name (priv->FileWritingId3v1IconvOptionsNo, "none");
    gtk_widget_set_tooltip_text (priv->FileWritingId3v1IconvOptionsNo, _("With this option, when "
        "a character cannot be represented in the target character set, it isn't changed. "
        "But note that an error message will be displayed for information."));
    priv->FileWritingId3v1IconvOptionsTranslit = gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (priv->FileWritingId3v1IconvOptionsNo)),
        _("//TRANSLIT"));
    gtk_grid_attach (GTK_GRID (Table), priv->FileWritingId3v1IconvOptionsTranslit, 2,
                     4, 1, 1);
    gtk_widget_set_name (priv->FileWritingId3v1IconvOptionsTranslit,
                         "transliterate");
    gtk_widget_set_tooltip_text (priv->FileWritingId3v1IconvOptionsTranslit,_("With this option, when "
        "a character cannot be represented in the target character set, it can be "
        "approximated through one or several similarly looking characters."));

    priv->FileWritingId3v1IconvOptionsIgnore = gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (priv->FileWritingId3v1IconvOptionsNo)),
        _("//IGNORE"));
    gtk_grid_attach (GTK_GRID (Table), priv->FileWritingId3v1IconvOptionsIgnore, 3,
                     4, 1, 1);
    gtk_widget_set_name (priv->FileWritingId3v1IconvOptionsIgnore, "ignore");
    gtk_widget_set_tooltip_text (priv->FileWritingId3v1IconvOptionsIgnore, _("With this option, when "
        "a character cannot be represented in the target character set, it will "
        "be silently discarded."));

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
    Frame = gtk_frame_new (_("Character Set for reading ID3 tags"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BOX_SPACING);

    Table = et_grid_new(4,2);
    gtk_box_pack_start(GTK_BOX(vbox),Table,FALSE,FALSE,0);
    //gtk_container_set_border_width(GTK_CONTAINER(Table), 2);
    /*gtk_grid_set_row_spacing (GTK_GRID (Table), 2);*/
    gtk_grid_set_column_spacing (GTK_GRID (Table), BOX_SPACING);

    // "File Reading Charset" Check Button + Combo
    UseNonStandardId3ReadingCharacterSet = gtk_check_button_new_with_label(_(
        "Non-standard:"));
    g_settings_bind (MainSettings, "id3-override-read-encoding",
                     UseNonStandardId3ReadingCharacterSet, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_grid_attach (GTK_GRID (Table), UseNonStandardId3ReadingCharacterSet, 0,
                     0, 1, 1);
    gtk_widget_set_tooltip_text(UseNonStandardId3ReadingCharacterSet,
        _("This character set will be used when reading the tag data, to convert "
        "each string found in an ISO-8859-1 field in the tag (for ID3v2 or/and ID3v1 tag).\n"
        "\n"
        "For example:\n"
        "  - In previous versions of EasyTAG, you could save UTF-8 strings in an ISO-8859-1 "
        "field. This is not correct. To convert these tags to Unicode: activate this option "
        "and select UTF-8. You must also activate above the option 'Try to save tags to "
        "ISO-8859-1. If it isn't possible then use UNICODE (recommended)' or 'Always save "
        "tags to UNICODE character set'.\n"
        "  - If Unicode was not used, Russian people can select the character set "
        "'Windows-1251' to load tags written under Windows. And 'KOI8-R' to load tags "
        "written under Unix systems."));

    priv->FileReadingId3v1v2CharacterSetCombo = gtk_combo_box_text_new();
    gtk_grid_attach (GTK_GRID (Table),
                     priv->FileReadingId3v1v2CharacterSetCombo, 2, 0, 1, 1);

    gtk_widget_set_tooltip_text (priv->FileReadingId3v1v2CharacterSetCombo,
                                 _("Character set used to read tag data in the file."));

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
    Label = gtk_label_new (_("Scanner"));
    VBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_notebook_append_page (GTK_NOTEBOOK(priv->options_notebook), VBox, Label);
    gtk_container_set_border_width (GTK_CONTAINER (VBox), BOX_SPACING);

    /* Save the number of the page. Asked in Scanner window */
    priv->options_notebook_scanner = gtk_notebook_page_num (GTK_NOTEBOOK (priv->options_notebook),
                                                            VBox);

    /* Character conversion for the 'Fill Tag' scanner (=> FTS...) */
    Frame = gtk_frame_new (_("Fill Tag Scanner - Character Conversion"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BOX_SPACING);

    FTSConvertUnderscoreAndP20IntoSpace = gtk_radio_button_new_with_label_from_widget (NULL,
       _("Convert underscore character '_' and string '%20' to space ' '"));
    gtk_widget_set_name (FTSConvertUnderscoreAndP20IntoSpace, "spaces");
    FTSConvertSpaceIntoUnderscore = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (FTSConvertUnderscoreAndP20IntoSpace),
                                                                                 _("Convert space ' ' to underscore '_'"));
    gtk_widget_set_name (FTSConvertSpaceIntoUnderscore, "underscores");
    FTSConvertSpaceNoChange = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (FTSConvertUnderscoreAndP20IntoSpace),
                                                                           _("No conversion"));
    gtk_widget_set_name (FTSConvertSpaceNoChange, "no-change");
    gtk_box_pack_start(GTK_BOX(vbox),FTSConvertUnderscoreAndP20IntoSpace,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(vbox),FTSConvertSpaceIntoUnderscore,      FALSE,FALSE,0);
    gtk_box_pack_start (GTK_BOX (vbox), FTSConvertSpaceNoChange, FALSE, FALSE,
                        0);
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
    gtk_widget_set_tooltip_text(FTSConvertUnderscoreAndP20IntoSpace,_("If activated, this conversion "
        "will be used when applying a mask from the scanner for tags."));
    gtk_widget_set_tooltip_text(FTSConvertSpaceIntoUnderscore,_("If activated, this conversion "
        "will be used when applying a mask from the scanner for tags."));
    /* TODO: No change tooltip. */

    /* Character conversion for the 'Rename File' scanner (=> RFS...) */
    Frame = gtk_frame_new (_("Rename File Scanner - Character Conversion"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BOX_SPACING);
    RFSConvertUnderscoreAndP20IntoSpace = gtk_radio_button_new_with_label(NULL, _("Convert underscore " "character '_' and string '%20' to space ' '"));
    RFSConvertSpaceIntoUnderscore = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(RFSConvertUnderscoreAndP20IntoSpace), _("Convert space ' ' to underscore '_'"));
    RFSRemoveSpaces = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (RFSConvertUnderscoreAndP20IntoSpace),
                                                                   _("Remove spaces"));
    gtk_widget_set_name (RFSConvertUnderscoreAndP20IntoSpace, "spaces");
    gtk_widget_set_name (RFSConvertSpaceIntoUnderscore, "underscores");
    gtk_widget_set_name (RFSRemoveSpaces, "remove");
    gtk_box_pack_start(GTK_BOX(vbox),RFSConvertUnderscoreAndP20IntoSpace,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(vbox),RFSConvertSpaceIntoUnderscore,      FALSE,FALSE,0);
    gtk_box_pack_start (GTK_BOX (vbox), RFSRemoveSpaces, FALSE, FALSE, 0);
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
    gtk_widget_set_tooltip_text(RFSConvertUnderscoreAndP20IntoSpace,_("If activated, this conversion "
        "will be used when applying a mask from the scanner for filenames."));
    gtk_widget_set_tooltip_text(RFSConvertSpaceIntoUnderscore,_("If activated, this conversion "
        "will be used when applying a mask from the scanner for filenames."));
    gtk_widget_set_tooltip_text(RFSRemoveSpaces,_("If activated, this conversion "        "will be used when applying a mask from the scanner for filenames."));

    /* Character conversion for the 'Process Fields' scanner (=> PFS...) */
    Frame = gtk_frame_new (_("Process Fields Scanner - Character Conversion"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BOX_SPACING);

    /* Don't convert some words like to, feat. first letter uppercase. */
    PFSDontUpperSomeWords = gtk_check_button_new_with_label(_("Don't uppercase "
        "first letter of words for some prepositions and articles."));
    gtk_box_pack_start(GTK_BOX(vbox),PFSDontUpperSomeWords, FALSE, FALSE, 0);
    g_settings_bind (MainSettings, "process-uppercase-prepositions",
                     PFSDontUpperSomeWords, "active",
                     G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_INVERT_BOOLEAN);
    gtk_widget_set_tooltip_text(PFSDontUpperSomeWords, _("Don't convert first "
        "letter of words like prepositions, articles and words like feat., "
        "when using the scanner 'First letter uppercase of each word' (for "
        "example, you will obtain 'Text in an Entry' instead of 'Text In An Entry')."));

    /* Properties of the scanner window */
    Frame = gtk_frame_new (_("Scanner Window"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BOX_SPACING);

    OpenScannerWindowOnStartup = gtk_check_button_new_with_label(_("Open the Scanner Window on startup"));
    gtk_box_pack_start(GTK_BOX(vbox),OpenScannerWindowOnStartup,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "scan-startup", OpenScannerWindowOnStartup,
                     "active", G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(OpenScannerWindowOnStartup,_("Activate this option to open automatically "
        "the scanner window when EasyTAG starts."));


    /* Other options */
    Frame = gtk_frame_new (_("Fields"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BOX_SPACING);

    // Overwrite text into tag fields
    OverwriteTagField = gtk_check_button_new_with_label(_("Overwrite fields when scanning tags"));
    gtk_box_pack_start(GTK_BOX(vbox),OverwriteTagField,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "fill-overwrite-tag-fields",
                     OverwriteTagField, "active", G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(OverwriteTagField,_("If activated, the scanner will replace existing text "
        "in fields by the new one. If deactivated, only blank fields of the tag will be filled."));

    /* Set a default comment text or CRC-32 checksum. */
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_SPACING);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    SetDefaultComment = gtk_check_button_new_with_label(_("Set this text as default comment:"));
    gtk_box_pack_start(GTK_BOX(hbox),SetDefaultComment,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "fill-set-default-comment",
                     SetDefaultComment, "active", G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(SetDefaultComment,_("Activate this option if you want to put the "
        "following string into the comment field when using the 'Fill Tag' scanner."));
    DefaultComment = gtk_entry_new ();
    gtk_box_pack_start(GTK_BOX(hbox),DefaultComment,FALSE,FALSE,0);
    gtk_widget_set_size_request(GTK_WIDGET(DefaultComment), 250, -1);
    g_settings_bind (MainSettings, "fill-set-default-comment", DefaultComment,
                     "sensitive", G_SETTINGS_BIND_GET);

    g_settings_bind (MainSettings, "fill-default-comment", DefaultComment,
                     "text", G_SETTINGS_BIND_DEFAULT);

    /* CRC32 comment. */
    Crc32Comment = gtk_check_button_new_with_label(_("Use CRC32 as the default "
        "comment (for files with ID3 tags only)."));
    gtk_box_pack_start(GTK_BOX(vbox),Crc32Comment,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "fill-crc32-comment", Crc32Comment, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(Crc32Comment,_("Calculates the CRC-32 value of the file "
        "and writes it into the comment field when using the 'Fill Tag' scanner."));

    /*
     * CDDB
     */
    Label = gtk_label_new (_("CDDB"));
    VBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_notebook_append_page (GTK_NOTEBOOK (priv->options_notebook), VBox, Label);
    gtk_container_set_border_width (GTK_CONTAINER (VBox), BOX_SPACING);

    // CDDB Server Settings (Automatic Search)
    Frame = gtk_frame_new (_("Server Settings for Automatic Search"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE, 0);
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BOX_SPACING);

    // 1rst automatic search server
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(vbox),hbox);
    Label = gtk_label_new(_("Name:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerNameAutomaticSearch = gtk_combo_box_text_new_with_entry();
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerNameAutomaticSearch,FALSE,FALSE,0);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "freedb.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "www.gnudb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "at.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "au.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "ca.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "es.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "fi.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "ru.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "uk.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch), "us.freedb.org");
    g_settings_bind (MainSettings, "cddb-automatic-search-hostname",
                     gtk_bin_get_child (GTK_BIN (CddbServerNameAutomaticSearch)),
                     "text", G_SETTINGS_BIND_DEFAULT);

    Label = gtk_label_new (_("Port:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerPortAutomaticSearch = gtk_spin_button_new_with_range (0.0,
                                                                    65535.0,
                                                                    1.0);
    g_settings_bind (MainSettings, "cddb-automatic-search-port",
                     CddbServerPortAutomaticSearch, "value",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_size_request(GTK_WIDGET(CddbServerPortAutomaticSearch), 45, -1);
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerPortAutomaticSearch,FALSE,FALSE,0);

    Label = gtk_label_new (_("CGI Path:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerCgiPathAutomaticSearch = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerCgiPathAutomaticSearch,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "cddb-automatic-search-path",
                     CddbServerCgiPathAutomaticSearch, "text",
                     G_SETTINGS_BIND_DEFAULT);

    // 2sd automatic search server
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(vbox),hbox);
    Label = gtk_label_new(_("Name:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerNameAutomaticSearch2 = gtk_combo_box_text_new_with_entry();
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerNameAutomaticSearch2,FALSE,FALSE,0);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameAutomaticSearch2), "freedb.musicbrainz.org");
    g_settings_bind (MainSettings, "cddb-automatic-search-hostname2",
                     gtk_bin_get_child (GTK_BIN (CddbServerNameAutomaticSearch2)),
                     "text", G_SETTINGS_BIND_DEFAULT);

    Label = gtk_label_new (_("Port:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerPortAutomaticSearch2 = gtk_spin_button_new_with_range (0.0,
                                                                     65535.0,
                                                                     1.0);
    g_settings_bind (MainSettings, "cddb-automatic-search-port2",
                     CddbServerPortAutomaticSearch2, "value",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_size_request(GTK_WIDGET(CddbServerPortAutomaticSearch2), 45, -1);
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerPortAutomaticSearch2,FALSE,FALSE,0);

    Label = gtk_label_new (_("CGI Path:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerCgiPathAutomaticSearch2 = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerCgiPathAutomaticSearch2,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "cddb-automatic-search-path2",
                     CddbServerCgiPathAutomaticSearch2, "text",
                     G_SETTINGS_BIND_DEFAULT);

    // CDDB Server Settings (Manual Search)
    Frame = gtk_frame_new (_("Server Settings for Manual Search"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(vbox),hbox);
    Label = gtk_label_new(_("Name:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerNameManualSearch = gtk_combo_box_text_new_with_entry();
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerNameManualSearch,FALSE,FALSE,0);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameManualSearch), "www.freedb.org");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(CddbServerNameManualSearch), "www.gnudb.org");
    g_settings_bind (MainSettings, "cddb-manual-search-hostname",
                     gtk_bin_get_child (GTK_BIN (CddbServerNameManualSearch)),
                     "text", G_SETTINGS_BIND_DEFAULT);

    Label = gtk_label_new (_("Port:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerPortManualSearch = gtk_spin_button_new_with_range (0.0, 65535.0,
                                                                 1.0);
    gtk_widget_set_size_request(GTK_WIDGET(CddbServerPortManualSearch), 45, -1);
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerPortManualSearch,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "cddb-manual-search-port",
                     CddbServerPortManualSearch, "value",
                     G_SETTINGS_BIND_DEFAULT);

    Label = gtk_label_new (_("CGI Path:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,2);
    CddbServerCgiPathManualSearch = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox),CddbServerCgiPathManualSearch,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "cddb-manual-search-path",
                     CddbServerCgiPathManualSearch, "text",
                     G_SETTINGS_BIND_DEFAULT);

    // CDDB Proxy Settings
    Frame = gtk_frame_new (_("Proxy Settings"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);

    Table = et_grid_new (3, 5);
    gtk_container_add(GTK_CONTAINER(Frame),Table);
    gtk_grid_set_row_spacing (GTK_GRID (Table), BOX_SPACING);
    gtk_grid_set_column_spacing (GTK_GRID (Table), BOX_SPACING);
    gtk_container_set_border_width(GTK_CONTAINER(Table), BOX_SPACING);

    CddbUseProxy = gtk_check_button_new_with_label(_("Use a proxy"));
    gtk_grid_attach (GTK_GRID (Table), CddbUseProxy, 0, 0, 5, 1);
    g_settings_bind (MainSettings, "cddb-proxy-enabled", CddbUseProxy, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(CddbUseProxy,_("Set active the settings of the proxy server."));

    Label = gtk_label_new(_("Host Name:"));
    gtk_grid_attach (GTK_GRID (Table), Label, 1, 1, 1, 1);
    gtk_widget_set_halign (Label, GTK_ALIGN_END);
    CddbProxyName = gtk_entry_new();
    gtk_grid_attach (GTK_GRID (Table), CddbProxyName, 2, 1, 1, 1);
    g_settings_bind (MainSettings, "cddb-proxy-hostname",
                     CddbProxyName, "text", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "cddb-proxy-enabled", CddbProxyName,
                     "sensitive", G_SETTINGS_BIND_GET);
    gtk_widget_set_tooltip_text(CddbProxyName,_("Name of the proxy server."));
    Label = gtk_label_new (_("Port:"));
    gtk_grid_attach (GTK_GRID (Table), Label, 3, 1, 1, 1);
    gtk_widget_set_halign (Label, GTK_ALIGN_END);
    CddbProxyPort = gtk_spin_button_new_with_range (0.0, 65535.0, 1.0);
    g_settings_bind (MainSettings, "cddb-proxy-port", CddbProxyPort, "value",
                     G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "cddb-proxy-enabled", CddbProxyPort,
                     "sensitive", G_SETTINGS_BIND_GET);
    gtk_widget_set_size_request(GTK_WIDGET(CddbProxyPort), 45, -1);
    gtk_grid_attach (GTK_GRID (Table), CddbProxyPort, 4, 1, 1, 1);
    gtk_widget_set_tooltip_text(CddbProxyPort,_("Port of the proxy server."));
    Label = gtk_label_new(_("User Name:"));
    gtk_grid_attach (GTK_GRID (Table), Label, 1, 2, 1, 1);
    gtk_widget_set_halign (Label, GTK_ALIGN_END);
    CddbProxyUserName = gtk_entry_new();
    g_settings_bind (MainSettings, "cddb-proxy-username", CddbProxyUserName,
                     "text", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "cddb-proxy-enabled", CddbProxyUserName,
                     "sensitive", G_SETTINGS_BIND_GET);
    gtk_grid_attach (GTK_GRID (Table), CddbProxyUserName, 2, 2, 1, 1);
    gtk_widget_set_tooltip_text(CddbProxyUserName,_("Name of user for the the proxy server."));
    Label = gtk_label_new(_("User Password:"));
    gtk_grid_attach (GTK_GRID (Table), Label, 3, 2, 1, 1);
    gtk_widget_set_halign (Label, GTK_ALIGN_END);
    CddbProxyUserPassword = gtk_entry_new();
    gtk_entry_set_visibility (GTK_ENTRY (CddbProxyUserPassword), FALSE);
    g_settings_bind (MainSettings, "cddb-proxy-password", CddbProxyUserPassword,
                     "text", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (MainSettings, "cddb-proxy-enabled", CddbProxyUserPassword,
                     "sensitive", G_SETTINGS_BIND_GET);
    gtk_grid_attach (GTK_GRID (Table), CddbProxyUserPassword, 4, 2, 1, 1);
    gtk_widget_set_tooltip_text (CddbProxyUserPassword,
                                 _("Password of user for the proxy server."));

    // Track Name list (CDDB results)
    Frame = gtk_frame_new (_("Track Name List"));
    gtk_box_pack_start(GTK_BOX(VBox),Frame,FALSE,FALSE,0);

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_container_add(GTK_CONTAINER(Frame),vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BOX_SPACING);

    CddbFollowFile = gtk_check_button_new_with_label(_("Select corresponding audio "
        "file (according position or DLM if activated below)"));
    gtk_box_pack_start(GTK_BOX(vbox),CddbFollowFile,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "cddb-follow-file", CddbFollowFile,
                     "active", G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(CddbFollowFile,_("If activated, when selecting a "
        "line in the list of tracks name, the corresponding audio file in the "
        "main list will be also selected."));

    // Check box to use DLM (also used in the cddb window)
    CddbUseDLM = gtk_check_button_new_with_label(_("Use the Levenshtein algorithm "
        "(DLM) to match lines (using title) with audio files (using filename)"));
    gtk_box_pack_start(GTK_BOX(vbox),CddbUseDLM,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "cddb-dlm-enabled", CddbUseDLM, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(CddbUseDLM,_("When activating this option, the "
        "Levenshtein algorithm (DLM: Damerau-Levenshtein Metric) will be used "
        "to match the CDDB title against every filename in the current folder, "
        "and to select the best match. This will be used when selecting the "
        "corresponding audio file, or applying CDDB results, instead of using "
        "directly the position order."));


    /*
     * Confirmation
     */
    Label = gtk_label_new (_("Confirmation"));
    VBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);
    gtk_notebook_append_page (GTK_NOTEBOOK(priv->options_notebook), VBox, Label);
    gtk_container_set_border_width (GTK_CONTAINER (VBox), BOX_SPACING);

    ConfirmBeforeExit = gtk_check_button_new_with_label(_("Confirm exit from program"));
    gtk_box_pack_start(GTK_BOX(VBox),ConfirmBeforeExit,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "confirm-quit", ConfirmBeforeExit, "active",
                     G_SETTINGS_BIND_DEFAULT);
    gtk_widget_set_tooltip_text(ConfirmBeforeExit,_("If activated, opens a dialog box to ask "
        "confirmation before exiting the program."));

    ConfirmWriteTag = gtk_check_button_new_with_label(_("Confirm writing of file tag"));
    gtk_box_pack_start(GTK_BOX(VBox),ConfirmWriteTag,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "confirm-write-tags", ConfirmWriteTag,
                     "active", G_SETTINGS_BIND_DEFAULT);

    ConfirmRenameFile = gtk_check_button_new_with_label(_("Confirm renaming of file"));
    gtk_box_pack_start(GTK_BOX(VBox),ConfirmRenameFile,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "confirm-rename-file", ConfirmRenameFile,
                     "active", G_SETTINGS_BIND_DEFAULT);

    ConfirmDeleteFile = gtk_check_button_new_with_label(_("Confirm deleting of file"));
    g_settings_bind (MainSettings, "confirm-delete-file", ConfirmDeleteFile,
                     "active", G_SETTINGS_BIND_DEFAULT);

    ConfirmWritePlayList = gtk_check_button_new_with_label(_("Confirm writing of playlist"));
    gtk_box_pack_start(GTK_BOX(VBox),ConfirmWritePlayList,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "confirm-write-playlist",
                     ConfirmWritePlayList, "active", G_SETTINGS_BIND_DEFAULT);

    ConfirmWhenUnsavedFiles = gtk_check_button_new_with_label(_("Confirm changing directory when there are unsaved changes"));
    gtk_box_pack_start(GTK_BOX(VBox),ConfirmWhenUnsavedFiles,FALSE,FALSE,0);
    g_settings_bind (MainSettings, "confirm-when-unsaved-files",
                     ConfirmWhenUnsavedFiles, "active",
                     G_SETTINGS_BIND_DEFAULT);

    /* Load the default page */
    g_settings_bind (MainSettings, "preferences-page", priv->options_notebook,
                     "page", G_SETTINGS_BIND_DEFAULT);
}

static void
Number_Track_Formatted_Spin_Button_Changed (GtkWidget *Label,
                                            GtkWidget *SpinButton)
{
    gchar *tmp;
    gint val;

    if (g_settings_get_boolean (MainSettings, "tag-number-padded"))
    {
        val = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (SpinButton));
    }
    else
        val = 1;

    // For translators : be aware to NOT translate '%.*d' in this string
    tmp = g_strdup_printf(_("(Example: %.*d_-_Track_name_1.mp3)"),val,1);

    gtk_label_set_text(GTK_LABEL(Label),tmp);
    g_free(tmp);
}

static void
et_prefs_on_pad_disc_number_spinbutton_changed (GtkWidget *label,
                                                GtkWidget *spinbutton)
{
    gchar *tmp;
    guint val;

    if (g_settings_get_boolean (MainSettings, "tag-disc-padded"))
    {
        val = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinbutton));
    }
    else
    {
        val = 1;
    }

    /* Translators: please do NOT translate '%.*d' in this string. */
    tmp = g_strdup_printf (_("(Example: disc_%.*d_of_10/Track_name_1.mp3)"),
                           val, 1);

    gtk_label_set_text (GTK_LABEL (label), tmp);
    g_free (tmp);
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

    gtk_widget_set_sensitive (priv->LabelId3v1Charset, active);
    gtk_widget_set_sensitive (priv->FileWritingId3v1CharacterSetCombo, active);
    gtk_widget_set_sensitive (priv->LabelAdditionalId3v1IconvOptions, active);
    gtk_widget_set_sensitive (priv->FileWritingId3v1IconvOptionsNo, active);
    gtk_widget_set_sensitive (priv->FileWritingId3v1IconvOptionsTranslit, active);
    gtk_widget_set_sensitive (priv->FileWritingId3v1IconvOptionsIgnore, active);
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
                                                _("The selected path for 'Default path to files' is invalid"));
            gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (msgdialog),
                                                      _("Path: '%s'\nError: %s"),
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

/*
 * The character set conversion is used for ID3 tag. UTF-8 is used to display.
 *  - reading_character is converted to UTF-8
 *  - writing_character is converted from UTF-8
 */
/*****************
gint Check_CharacterSetTranslation (void)
{
    gchar *temp;
    gchar *reading_character;
    gchar *writing_character;

    temp = Get_Active_Combo_Box_Item(GTK_COMBO_BOX(FileReadingCharacterSetCombo));
    reading_character = Charset_Get_Name_From_Title(temp);
    g_free(temp);

    temp = Get_Active_Combo_Box_Item(GTK_COMBO_BOX(FileWritingCharacterSetCombo));
    writing_character = Charset_Get_Name_From_Title(temp);
    g_free(temp);

    // Check conversion when reading file
    if ( GTK_TOGGLE_BUTTON(UseNonStandardId3ReadingCharacterSet)->active
    && (test_conversion_charset(reading_character,"UTF-8")!=TRUE) )
    {
        gchar *msg = g_strdup_printf(_("The character set translation from '%s'\n"
                                       "to '%s' is not supported"),reading_character,"UTF-8");
        GtkWidget *msgbox = msg_box_new(_("Error"),
                                        GTK_WINDOW(OptionsWindow),
                                        NULL,
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        msg,
                                        GTK_STOCK_DIALOG_ERROR,
                                        GTK_STOCK_OK, GTK_RESPONSE_OK,
                                        NULL);
        gtk_dialog_run(GTK_DIALOG(msgbox));
        gtk_widget_destroy(msgbox);
        g_free(msg);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(UseNonStandardId3ReadingCharacterSet),FALSE);
        return 0;
    }
    // Check conversion when writing file
    if ( GTK_TOGGLE_BUTTON(UseNonStandardId3WritingCharacterSet)->active
    && (test_conversion_charset("UTF-8",writing_character)!=TRUE) )
    {
        gchar *msg = g_strdup_printf(_("The character set translation from '%s'\n"
                                       "to '%s' is not supported"),"UTF-8",writing_character);
        GtkWidget *msgbox = msg_box_new(_("Error"),
                                        GTK_WINDOW(OptionsWindow),
                                        NULL,
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        msg,
                                        GTK_STOCK_DIALOG_ERROR,
                                        GTK_STOCK_OK, GTK_RESPONSE_OK,
                                        NULL);
        gtk_dialog_run(GTK_DIALOG(msgbox));
        gtk_widget_destroy(msgbox);
        g_free(msg);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(UseNonStandardId3WritingCharacterSet),FALSE);
        return 0;
    }

    return 1;
}
*************/

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
et_preferences_dialog_new (void)
{
    return g_object_new (ET_TYPE_PREFERENCES_DIALOG, NULL);
}
