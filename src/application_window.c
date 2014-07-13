/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
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
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "application_window.h"

#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>

#include "bar.h"
#include "browser.h"
#include "cddb_dialog.h"
#include "easytag.h"
#include "gtk2_compat.h"
#include "load_files_dialog.h"
#include "log.h"
#include "misc.h"
#include "picture.h"
#include "playlist_dialog.h"
#include "preferences_dialog.h"
#include "search_dialog.h"
#include "scan.h"
#include "scan_dialog.h"
#include "setting.h"

/* TODO: Use G_DEFINE_TYPE_WITH_PRIVATE. */
G_DEFINE_TYPE (EtApplicationWindow, et_application_window, GTK_TYPE_APPLICATION_WINDOW)

#define et_application_window_get_instance_private(window) (window->priv)

struct _EtApplicationWindowPrivate
{
    GtkWidget *browser;

    GtkWidget *file_area;
    GtkWidget *log_area;
    GtkWidget *tag_area;
    GtkWidget *cddb_dialog;
    GtkWidget *load_files_dialog;
    GtkWidget *playlist_dialog;
    GtkWidget *preferences_dialog;
    GtkWidget *scan_dialog;
    GtkWidget *search_dialog;

    /* Tag area labels. */
    GtkWidget *title_label;
    GtkWidget *artist_label;
    GtkWidget *album_artist_label;
    GtkWidget *album_label;
    GtkWidget *disc_number_label;
    GtkWidget *year_label;
    GtkWidget *track_label;
    GtkWidget *genre_label;
    GtkWidget *comment_label;
    GtkWidget *composer_label;
    GtkWidget *orig_artist_label;
    GtkWidget *copyright_label;
    GtkWidget *url_label;
    GtkWidget *encoded_by_label;

    /* FIXME: Tag area entries. */

    /* Notebook tabs. */
    GtkWidget *images_tab;

    /* Image treeview model. */
    GtkListStore *images_model;

    GtkWidget *hpaned;
    GtkWidget *vpaned;

    /* Mini buttons. */
    GtkWidget *track_sequence_button;
    GtkWidget *track_number_button;
    GtkToolItem *apply_image_toolitem;
};

/* Used to force to hide the msgbox when deleting file */
static gboolean SF_HideMsgbox_Delete_File;
/* To remember which button was pressed when deleting file */
static gint SF_ButtonPressed_Delete_File;

static gboolean et_tag_field_on_key_press_event (GtkEntry *entry,
                                                 GdkEventKey *event,
                                                 gpointer user_data);

static void Mini_Button_Clicked (GObject *object);

static gboolean
on_main_window_delete_event (GtkWidget *window,
                             GdkEvent *event,
                             gpointer user_data)
{
    Quit_MainWindow ();

    /* Handled the event, so stop propagation. */
    return TRUE;
}

static void
Convert_P20_And_Underscore_Into_Spaces (GtkWidget *entry)
{
    gchar *string = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    Scan_Convert_Underscore_Into_Space (string);
    Scan_Convert_P20_Into_Space (string);
    gtk_entry_set_text (GTK_ENTRY (entry), string);
    g_free (string);
}

static void
Convert_Space_Into_Underscore (GtkWidget *entry)
{
    gchar *string = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    Scan_Convert_Space_Into_Underscore (string);
    gtk_entry_set_text (GTK_ENTRY (entry), string);
    g_free (string);
}

static void
Convert_All_Uppercase (GtkWidget *entry)
{
    gchar *res;
    const gchar *string = gtk_entry_get_text (GTK_ENTRY (entry));

    res = Scan_Process_Fields_All_Uppercase (string);
    gtk_entry_set_text (GTK_ENTRY (entry), res);
    g_free (res);
}

static void
Convert_All_Lowercase (GtkWidget *entry)
{
    gchar *res;
    const gchar *string = gtk_entry_get_text (GTK_ENTRY (entry));

    res = Scan_Process_Fields_All_Downcase (string);
    gtk_entry_set_text (GTK_ENTRY (entry), res);
    g_free (res);
}

static void
Convert_Letter_Uppercase (GtkWidget *entry)
{
    gchar *res;
    const gchar *string = gtk_entry_get_text (GTK_ENTRY (entry));

    res = Scan_Process_Fields_Letter_Uppercase (string);
    gtk_entry_set_text (GTK_ENTRY (entry), res);
    g_free (res);
}

static void
Convert_First_Letters_Uppercase (GtkWidget *entry)
{
    EtScanDialog *dialog;
    gchar *string;

    string = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
    dialog = ET_SCAN_DIALOG (et_application_window_get_scan_dialog (ET_APPLICATION_WINDOW (MainWindow)));

    Scan_Process_Fields_First_Letters_Uppercase (dialog, &string);
    gtk_entry_set_text (GTK_ENTRY (entry), string);
    g_free (string);
}

static void
Convert_Remove_Space (GtkWidget *entry)
{
    gchar *string = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    Scan_Process_Fields_Remove_Space (string);
    gtk_entry_set_text (GTK_ENTRY (entry), string);
    g_free (string);
}

static void
Convert_Insert_Space (GtkWidget *entry)
{
    gchar *res;
    const gchar *string = (gtk_entry_get_text (GTK_ENTRY (entry)));

    res = Scan_Process_Fields_Insert_Space (string);
    gtk_entry_set_text (GTK_ENTRY (entry), res);
    g_free (res);
}

static void
Convert_Only_One_Space (GtkWidget *entry)
{
    gchar *string = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    Scan_Process_Fields_Keep_One_Space (string);
    gtk_entry_set_text (GTK_ENTRY (entry), string);
    g_free (string);
}

static void
Convert_Remove_All_Text (GtkWidget *entry)
{
    gtk_entry_set_text (GTK_ENTRY (entry), "");
}

/* Show the popup menu when the third mouse button is pressed. */
static gboolean
Entry_Popup_Menu_Handler (GtkMenu *menu,
                          GdkEventButton *event,
                          gpointer user_data)
{
    if (event && (event->type == GDK_BUTTON_PRESS) && (event->button == 3))
    {
        gtk_menu_popup (menu, NULL, NULL, NULL, NULL, event->button,
                        event->time);
        return TRUE;
    }

    return FALSE;
}

/*
 * Popup menu attached to all entries of tag + filename + rename combobox.
 * Displayed when pressing the right mouse button and contains functions to process ths strings.
 */
static void
Attach_Popup_Menu_To_Tag_Entries (GtkEntry *entry)
{
    GtkWidget *menu;
    GtkWidget *image;
    GtkWidget *menu_item;

    menu = gtk_menu_new ();
    g_signal_connect_swapped (entry, "button-press-event",
                              G_CALLBACK (Entry_Popup_Menu_Handler),
                              G_OBJECT (menu));

    /* Menu items */
    menu_item = gtk_image_menu_item_new_with_label (_("Tag selected files with this field"));
    image = gtk_image_new_from_stock (GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Mini_Button_Clicked),
                              G_OBJECT (entry));

    /* Separator */
    menu_item = gtk_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    menu_item = gtk_image_menu_item_new_with_label (_("Convert '_' and '%20' to spaces"));
    image = gtk_image_new_from_stock (GTK_STOCK_CONVERT, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_P20_And_Underscore_Into_Spaces),
                              G_OBJECT (entry));

    menu_item = gtk_image_menu_item_new_with_label (_("Convert ' ' to '_'"));
    image = gtk_image_new_from_stock (GTK_STOCK_CONVERT, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_Space_Into_Underscore),
                              G_OBJECT (entry));

    /* Separator */
    menu_item = gtk_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    menu_item = gtk_image_menu_item_new_with_label (_("All uppercase"));
    image = gtk_image_new_from_stock ("easytag-all-uppercase",
                                      GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_All_Uppercase),
                              G_OBJECT (entry));

    menu_item = gtk_image_menu_item_new_with_label (_("All lowercase"));
    image = gtk_image_new_from_stock ("easytag-all-downcase",
                                      GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_All_Lowercase),
                              G_OBJECT (entry));

    menu_item = gtk_image_menu_item_new_with_label (_("First letter uppercase"));
    image = gtk_image_new_from_stock ("easytag-first-letter-uppercase",
                                      GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_Letter_Uppercase),
                              G_OBJECT (entry));

    menu_item = gtk_image_menu_item_new_with_label (_("First letter uppercase of each word"));
    image = gtk_image_new_from_stock ("easytag-first-letter-uppercase-word",
                                      GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_First_Letters_Uppercase),
                              G_OBJECT (entry));

    /* Separator */
    menu_item = gtk_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    menu_item = gtk_image_menu_item_new_with_label (_("Remove spaces"));
    image = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_Remove_Space),
                              G_OBJECT (entry));

    menu_item = gtk_image_menu_item_new_with_label (_("Insert space before uppercase letter"));
    image = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_Insert_Space),
                              G_OBJECT (entry));

    menu_item = gtk_image_menu_item_new_with_label (_("Remove duplicate spaces or underscores"));
    image = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_Only_One_Space),
                              G_OBJECT (entry));

    menu_item = gtk_image_menu_item_new_with_label (_("Remove all text"));
    image = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (Convert_Remove_All_Text),
                              G_OBJECT (entry));

    gtk_widget_show_all (menu);
}


/*
 * Clear the entries of tag area
 */
void
Clear_Tag_Entry_Fields (void)
{
    g_return_if_fail (TitleEntry != NULL);

    gtk_entry_set_text (GTK_ENTRY (TitleEntry), "");
    gtk_entry_set_text (GTK_ENTRY (ArtistEntry), "");
    gtk_entry_set_text (GTK_ENTRY (AlbumArtistEntry), "");
    gtk_entry_set_text (GTK_ENTRY (AlbumEntry), "");
    gtk_entry_set_text (GTK_ENTRY (DiscNumberEntry), "");
    gtk_entry_set_text (GTK_ENTRY (YearEntry), "");
    gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (TrackEntryCombo))),
                        "");
    gtk_entry_set_text (GTK_ENTRY (TrackTotalEntry), "");
    gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GenreCombo))),
                        "");
    gtk_entry_set_text (GTK_ENTRY (CommentEntry), "");
    gtk_entry_set_text (GTK_ENTRY (ComposerEntry), "");
    gtk_entry_set_text (GTK_ENTRY (OrigArtistEntry), "");
    gtk_entry_set_text (GTK_ENTRY (CopyrightEntry), "");
    gtk_entry_set_text (GTK_ENTRY (URLEntry), "");
    gtk_entry_set_text (GTK_ENTRY (EncodedByEntry), "");
    PictureEntry_Clear ();
}


/*
 * Clear the entry of file area
 */
void
Clear_File_Entry_Field (void)
{
    g_return_if_fail (FileEntry != NULL);

    gtk_entry_set_text (GTK_ENTRY (FileEntry),"");
}


/*
 * Clear the header information
 */
void
Clear_Header_Fields (void)
{
    g_return_if_fail (VersionValueLabel != NULL);

    /* Default values are MPs data */
    gtk_label_set_text (GTK_LABEL (VersionLabel), _("Encoder:"));
    gtk_label_set_text (GTK_LABEL (VersionValueLabel), "");
    gtk_label_set_text (GTK_LABEL (BitrateValueLabel), "");
    gtk_label_set_text (GTK_LABEL (SampleRateValueLabel), "");
    gtk_label_set_text (GTK_LABEL (ModeLabel), _("Mode:"));
    gtk_label_set_text (GTK_LABEL (ModeValueLabel), "");
    gtk_label_set_text (GTK_LABEL (SizeValueLabel), "");
    gtk_label_set_text (GTK_LABEL (DurationValueLabel), "");
}


static void
Mini_Button_Clicked (GObject *object)
{
    GtkWidget *toplevel;
    EtApplicationWindowPrivate *priv;
    GList *etfilelist = NULL;
    GList *selection_filelist = NULL;
    GList *l;
    gchar *string_to_set = NULL;
    gchar *string_to_set1 = NULL;
    gchar *msg = NULL;
    ET_File *etfile;
    File_Tag *FileTag;
    GtkTreeSelection *selection;

    g_return_if_fail (ETCore->ETFileDisplayedList != NULL);

    /* FIXME: hack! */
    toplevel = gtk_widget_get_toplevel (GTK_WIDGET (object));

    if (gtk_widget_is_toplevel (toplevel))
    {
        priv = et_application_window_get_instance_private (ET_APPLICATION_WINDOW (toplevel));
    }
    else
    {
        g_error ("Main window is not a toplevel!");
    }

    /* Save the current displayed data. */
    ET_Save_File_Data_From_UI (ETCore->ETFileDisplayed);

    /* Warning : 'selection_filelist' is not a list of 'ETFile' items! */
    selection = et_application_window_browser_get_selection (ET_APPLICATION_WINDOW (toplevel));
    selection_filelist = gtk_tree_selection_get_selected_rows (selection, NULL);

    // Create an 'ETFile' list from 'selection_filelist'
    for (l = selection_filelist; l != NULL; l = g_list_next (l))
    {
        etfile = et_browser_get_et_file_from_path (ET_BROWSER (priv->browser),
                                                   l->data);
        etfilelist = g_list_prepend (etfilelist, etfile);
    }

    etfilelist = g_list_reverse (etfilelist);
    g_list_free_full (selection_filelist, (GDestroyNotify)gtk_tree_path_free);


    if (object == G_OBJECT (TitleEntry))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(TitleEntry),0,-1); // The string to apply to all other files

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->title,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with title '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed title from selected files."));
    }
    else if (object == G_OBJECT (ArtistEntry))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(ArtistEntry),0,-1);

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->artist,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with artist '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed artist from selected files."));
    }
    else if (object == G_OBJECT (AlbumArtistEntry))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(AlbumArtistEntry),0,-1);
        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->album_artist,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with album artist '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed album artist from selected files."));
    }
    else if (object == G_OBJECT (AlbumEntry))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(AlbumEntry),0,-1);

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->album,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with album '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed album name from selected files."));
    }
    else if (object == G_OBJECT (DiscNumberEntry))
    {
        const gchar *entry_text;
        gchar *separator;

        entry_text = gtk_entry_get_text (GTK_ENTRY (DiscNumberEntry));
        separator = g_utf8_strchr (entry_text, -1, '/');

        if (separator)
        {
            string_to_set1 = g_strdup (separator + 1);
            string_to_set = g_strndup (entry_text,
                                       separator - entry_text);
        }
        else
        {
            string_to_set = g_strdup (entry_text);
            string_to_set1 = NULL;
        }

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item (&FileTag->disc_number, string_to_set);
            ET_Set_Field_File_Tag_Item (&FileTag->disc_total, string_to_set1);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }

        if (string_to_set != NULL && g_utf8_strlen (string_to_set, -1) > 0)
        {
            if (string_to_set1 != NULL
                && g_utf8_strlen (string_to_set1, -1) > 0)
            {
                msg = g_strdup_printf (_("Selected files tagged with disc number '%s/%s'."),
                                       string_to_set, string_to_set1);
            }
            else
            {
                msg = g_strdup_printf (_("Selected files tagged with disc number like 'xx'."));
            }
        }
        else
        {
            msg = g_strdup (_("Removed disc number from selected files."));
        }
    }
    else if (object == G_OBJECT (YearEntry))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(YearEntry),0,-1);

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->year,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with year '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed year from selected files."));
    }
    else if (object == G_OBJECT (TrackTotalEntry))
    {
        /* Used of Track and Total Track values */
        string_to_set = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(TrackEntryCombo)))));
        string_to_set1 = gtk_editable_get_chars(GTK_EDITABLE(TrackTotalEntry),0,-1);

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);

            // We apply the TrackEntry field to all others files only if it is to delete
            // the field (string=""). Else we don't overwrite the track number
            if (!string_to_set || g_utf8_strlen(string_to_set, -1) == 0)
                ET_Set_Field_File_Tag_Item(&FileTag->track,string_to_set);
            ET_Set_Field_File_Tag_Item(&FileTag->track_total,string_to_set1);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }

        if ( string_to_set && g_utf8_strlen(string_to_set, -1) > 0 ) //&& atoi(string_to_set)>0 )
        {
            if ( string_to_set1 != NULL && g_utf8_strlen(string_to_set1, -1)>0 ) //&& atoi(string_to_set1)>0 )
            {
                msg = g_strdup_printf(_("Selected files tagged with track like 'xx/%s'."),string_to_set1);
            }else
            {
                msg = g_strdup_printf(_("Selected files tagged with track like 'xx'."));
            }
        }else
        {
            msg = g_strdup(_("Removed track number from selected files."));
        }
    }
    else if (object == G_OBJECT (priv->track_sequence_button))
    {
        /* This part doesn't set the same track number to all files, but sequence the tracks.
         * So we must browse the whole 'etfilelistfull' to get position of each selected file.
         * Note : 'etfilelistfull' and 'etfilelist' must be sorted in the same order */
        GList *etfilelistfull = NULL;
        gint sort_mode;
        gchar *path = NULL;
        gchar *path1 = NULL;
        gint i = 0;

        /* FIX ME!: see to fill also the Total Track (it's a good idea?) */
        etfilelistfull = g_list_first(ETCore->ETFileList);

        /* Sort 'etfilelistfull' and 'etfilelist' in the same order. */
        sort_mode = g_settings_get_enum (MainSettings, "sort-mode");
        etfilelist = ET_Sort_File_List (etfilelist, sort_mode);
        etfilelistfull = ET_Sort_File_List (etfilelistfull, sort_mode);

        while (etfilelist && etfilelistfull)
        {
            // To get the path of the file
            File_Name *FileNameCur = (File_Name *)((ET_File *)etfilelistfull->data)->FileNameCur->data;
            // The ETFile in the selected file list
            etfile = etfilelist->data;

            // Restart counter when entering a new directory
            g_free(path1);
            path1 = g_path_get_dirname(FileNameCur->value);
            if ( path && path1 && strcmp(path,path1)!=0 )
                i = 0;

            string_to_set = et_track_number_to_string (++i);

            // The file is in the selection?
            if ( (ET_File *)etfilelistfull->data == etfile )
            {
                FileTag = ET_File_Tag_Item_New();
                ET_Copy_File_Tag_Item(etfile,FileTag);
                ET_Set_Field_File_Tag_Item(&FileTag->track,string_to_set);
                ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);

                if (!etfilelist->next) break;
                etfilelist = g_list_next(etfilelist);
            }

            g_free(string_to_set);
            g_free(path);
            path = g_strdup(path1);

            etfilelistfull = g_list_next(etfilelistfull);
        }
        g_free(path);
        g_free(path1);
        //msg = g_strdup_printf(_("All %d tracks numbered sequentially."), ETCore->ETFileSelectionList_Length);
        msg = g_strdup_printf(_("Selected tracks numbered sequentially."));
    }
    else if (object==G_OBJECT(priv->track_number_button))
    {
        /* Used of Track and Total Track values */
        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            gchar *path_utf8, *filename_utf8;

            etfile        = (ET_File *)l->data;
            filename_utf8 = ((File_Name *)etfile->FileNameNew->data)->value_utf8;
            path_utf8     = g_path_get_dirname(filename_utf8);

            string_to_set = et_track_number_to_string (ET_Get_Number_Of_Files_In_Directory (path_utf8));

            g_free(path_utf8);
            if (!string_to_set1)
                string_to_set1 = g_strdup(string_to_set); // Just for the message below...

            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->track_total,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }

        if ( string_to_set1 != NULL && g_utf8_strlen(string_to_set1, -1)>0 ) //&& atoi(string_to_set1)>0 )
        {
            msg = g_strdup_printf(_("Selected files tagged with track like 'xx/%s'."),string_to_set1);
        }else
        {
            msg = g_strdup(_("Removed track number from selected files."));
        }
    }
    else if (object == G_OBJECT (gtk_bin_get_child (GTK_BIN (GenreCombo))))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(GenreCombo))),0,-1);

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->genre,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with genre '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed genre from selected files."));
    }
    else if (object == G_OBJECT (CommentEntry))
    {
        //GtkTextBuffer *textbuffer;
        //GtkTextIter    start_iter;
        //GtkTextIter    end_iter;
        //textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(CommentView));
        //gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(textbuffer),&start_iter,&end_iter);
        //string_to_set = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(textbuffer),&start_iter,&end_iter,TRUE);

        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(CommentEntry),0,-1);

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->comment,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with comment '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed comment from selected files."));
    }
    else if (object == G_OBJECT (ComposerEntry))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(ComposerEntry),0,-1);
        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->composer,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with composer '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed composer from selected files."));
    }
    else if (object == G_OBJECT (OrigArtistEntry))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(OrigArtistEntry),0,-1);

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->orig_artist,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with original artist '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed original artist from selected files."));
    }
    else if (object == G_OBJECT (CopyrightEntry))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(CopyrightEntry),0,-1);

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->copyright,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with copyright '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed copyright from selected files."));
    }
    else if (object == G_OBJECT (URLEntry))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(URLEntry),0,-1);

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->url,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with URL '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed URL from selected files."));
    }
    else if (object == G_OBJECT (EncodedByEntry))
    {
        string_to_set = gtk_editable_get_chars(GTK_EDITABLE(EncodedByEntry),0,-1);

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Item(&FileTag->encoded_by,string_to_set);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }
        if (string_to_set != NULL && g_utf8_strlen(string_to_set, -1)>0)
            msg = g_strdup_printf(_("Selected files tagged with encoder name '%s'."),string_to_set);
        else
            msg = g_strdup(_("Removed encoder name from selected files."));
    }
    else if (object == G_OBJECT (priv->apply_image_toolitem))
    {
        Picture *res = NULL, *pic, *prev_pic = NULL;
        GtkTreeModel *model;
        GtkTreeIter iter;

        model = gtk_tree_view_get_model(GTK_TREE_VIEW(PictureEntryView));
        if (gtk_tree_model_get_iter_first(model, &iter))
        {
            do
            {
                gtk_tree_model_get(model, &iter, PICTURE_COLUMN_DATA, &pic, -1);
                pic = Picture_Copy_One(pic);
                if (!res)
                    res = pic;
                else
                    prev_pic->next = pic;
                prev_pic = pic;
            } while (gtk_tree_model_iter_next(model, &iter));
        }

        for (l = etfilelist; l != NULL; l = g_list_next (l))
        {
            etfile = (ET_File *)l->data;
            FileTag = ET_File_Tag_Item_New();
            ET_Copy_File_Tag_Item(etfile,FileTag);
            ET_Set_Field_File_Tag_Picture((Picture **)&FileTag->picture, res);
            ET_Manage_Changes_Of_File_Data(etfile,NULL,FileTag);
        }
        if (res)
            msg = g_strdup (_("Selected files tagged with images."));
        else
            msg = g_strdup (_("Removed images from selected files."));
        Picture_Free(res);
    }

    g_list_free(etfilelist);

    /* Refresh the whole list (faster than file by file) to show changes. */
    et_application_window_browser_refresh_list (ET_APPLICATION_WINDOW (toplevel));

    /* Display the current file (Needed when sequencing tracks) */
    ET_Display_File_Data_To_UI(ETCore->ETFileDisplayed);

    if (msg)
    {
        Log_Print(LOG_OK,"%s",msg);
        Statusbar_Message(msg,TRUE);
        g_free(msg);
    }
    g_free(string_to_set);
    g_free(string_to_set1);

    /* To update state of Undo button */
    et_application_window_update_actions (ET_APPLICATION_WINDOW (toplevel));
}

/*
 * et_tag_field_connect_signals:
 * @entry: the entry for which to connect signals
 *
 * Connect the GtkWidget::key-press-event and GtkEntry::icon-release signals
 * of @entry to appropriate handlers for tag entry fields.
 */
static void
et_tag_field_connect_signals (GtkEntry *entry)
{
    g_signal_connect_after (entry, "key-press-event",
                            G_CALLBACK (et_tag_field_on_key_press_event),
                            NULL);
    g_signal_connect (entry, "icon-release", G_CALLBACK (Mini_Button_Clicked),
                      NULL);
}

/*
 * et_tag_field_on_key_press_event:
 * @entry: the tag entry field on which the event was generated
 * @event: the generated event
 * @user_data: user data set when the signal was connected
 *
 * Handle the Ctrl+Return combination being pressed in the tag field GtkEntrys
 * and apply the tag to selected files.
 *
 * Returns: %TRUE if the event was handled, %FALSE if the event should
 * propagate further
 */
static gboolean
et_tag_field_on_key_press_event (GtkEntry *entry, GdkEventKey *event,
                                 gpointer user_data)
{
    GdkModifierType modifiers = gtk_accelerator_get_default_mod_mask ();

    switch (event->keyval)
    {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_ISO_Enter:
            if ((event->state & modifiers) == GDK_CONTROL_MASK)
            {
                Mini_Button_Clicked (G_OBJECT (entry));
            }
            return TRUE;
        default:
            return FALSE;
    }
}

static GtkWidget *
create_browser_area (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;
    GtkWidget *frame;

    priv = et_application_window_get_instance_private (self);

    frame = gtk_frame_new (_("Browser"));
    gtk_container_set_border_width (GTK_CONTAINER (frame), 2);

    priv->browser = GTK_WIDGET (et_browser_new ());
    gtk_container_add (GTK_CONTAINER (frame), priv->browser);

    /* Don't load init dir here because Tag area hasn't been yet created!.
     * It will be load at the end of the main function */

    return frame;
}


static GtkWidget *
create_file_area (void)
{
    GtkWidget *vbox, *hbox;
    GtkWidget *separator;

    FileFrame = gtk_frame_new (_("File"));
    gtk_container_set_border_width (GTK_CONTAINER (FileFrame), 2);

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (FileFrame), vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);

    /* HBox for FileEntry and IconBox */
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    /* File index (position in list + list length) */
    FileIndex = gtk_label_new ("0/0:");
    gtk_box_pack_start (GTK_BOX (hbox), FileIndex, FALSE, FALSE, 0);

    /* Filename. */
    FileEntry = gtk_entry_new ();
    gtk_editable_set_editable (GTK_EDITABLE (FileEntry), TRUE);
    gtk_box_pack_start (GTK_BOX (hbox), FileEntry, TRUE, TRUE, 2);

    Attach_Popup_Menu_To_Tag_Entries (GTK_ENTRY (FileEntry));

    /*
     *  File Infos
     */
    HeaderInfosTable = et_grid_new (3, 5);
    gtk_container_add (GTK_CONTAINER (vbox), HeaderInfosTable);
    gtk_container_set_border_width (GTK_CONTAINER (HeaderInfosTable), 2);
    gtk_grid_set_row_spacing (GTK_GRID (HeaderInfosTable), 1);
    gtk_grid_set_column_spacing (GTK_GRID (HeaderInfosTable), 2);

    VersionLabel = gtk_label_new (_("Encoder:"));
    gtk_grid_attach (GTK_GRID (HeaderInfosTable), VersionLabel, 0, 0, 1, 1);
    VersionValueLabel = gtk_label_new ("");
    gtk_grid_attach (GTK_GRID (HeaderInfosTable), VersionValueLabel, 1, 0, 1,
                     1);
    gtk_misc_set_alignment (GTK_MISC (VersionLabel), 1.0, 0.5);
    gtk_misc_set_alignment (GTK_MISC (VersionValueLabel), 0.0, 0.5);

    BitrateLabel = gtk_label_new (_("Bitrate:"));
    gtk_grid_attach (GTK_GRID (HeaderInfosTable), BitrateLabel, 0, 1, 1, 1);
    BitrateValueLabel = gtk_label_new ("");
    gtk_grid_attach (GTK_GRID (HeaderInfosTable), BitrateValueLabel, 1, 1, 1,
                     1);
    gtk_misc_set_alignment (GTK_MISC (BitrateLabel), 1.0, 0.5);
    gtk_misc_set_alignment (GTK_MISC (BitrateValueLabel), 0.0, 0.5);

    /* Translators: Please try to keep this string as short as possible as it
     * is shown in a narrow column. */
    SampleRateLabel = gtk_label_new (_("Frequency:"));
    gtk_grid_attach (GTK_GRID (HeaderInfosTable), SampleRateLabel, 0, 2, 1, 1);
    SampleRateValueLabel = gtk_label_new("");
    gtk_grid_attach (GTK_GRID (HeaderInfosTable), SampleRateValueLabel, 1, 2,
                     1, 1);
    gtk_misc_set_alignment (GTK_MISC (SampleRateLabel), 1.0, 0.5);
    gtk_misc_set_alignment (GTK_MISC (SampleRateValueLabel), 0.0, 0.5);

    separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
    gtk_grid_attach (GTK_GRID (HeaderInfosTable), separator, 2, 0, 1, 4);

    ModeLabel = gtk_label_new(_("Mode:"));
    gtk_grid_attach (GTK_GRID (HeaderInfosTable), ModeLabel, 3, 0, 1, 1);
    ModeValueLabel = gtk_label_new ("");
    gtk_grid_attach (GTK_GRID (HeaderInfosTable), ModeValueLabel, 4, 0, 1, 1);
    gtk_misc_set_alignment (GTK_MISC (ModeLabel), 1.0, 0.5);
    gtk_misc_set_alignment (GTK_MISC (ModeValueLabel), 0.0, 0.5);

    SizeLabel = gtk_label_new (_("Size:"));
    gtk_grid_attach (GTK_GRID (HeaderInfosTable), SizeLabel, 3, 1, 1, 1);
    SizeValueLabel = gtk_label_new ("");
    gtk_grid_attach (GTK_GRID (HeaderInfosTable), SizeValueLabel, 4, 1, 1, 1);
    gtk_misc_set_alignment (GTK_MISC (SizeLabel), 1.0, 0.5);
    gtk_misc_set_alignment (GTK_MISC (SizeValueLabel), 0.0, 0.5);

    DurationLabel = gtk_label_new (_("Duration:"));
    gtk_grid_attach (GTK_GRID (HeaderInfosTable), DurationLabel, 3, 2, 1, 1);
    DurationValueLabel = gtk_label_new ("");
    gtk_grid_attach (GTK_GRID (HeaderInfosTable), DurationValueLabel, 4, 2, 1,
                     1);
    gtk_misc_set_alignment (GTK_MISC (DurationLabel), 1.0, 0.5);
    gtk_misc_set_alignment (GTK_MISC (DurationValueLabel), 0.0, 0.5);

    /* FIXME */
    #if 0
    if (SHOW_HEADER_INFO)
        gtk_widget_show_all(HeaderInfosTable);
    #endif
    return FileFrame;
}

#include "data/pixmaps/sequence_track.xpm"
static GtkWidget *
create_tag_area (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;
    GtkWidget *separator;
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *scrolled_window;
    GtkWidget *toolbar;
    GIcon *icon;
    GtkWidget *image;
    GtkWidget *vbox;
    GList *focus_chain = NULL;
    GtkEntryCompletion *completion;
    gint MButtonSize = 13;
    gint TablePadding = 2;

    /* For Picture. */
    static const GtkTargetEntry drops[] = { { "text/uri-list", 0,
                                              TARGET_URI_LIST } };
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;

    priv = et_application_window_get_instance_private (self);

    /* Main Frame */
    TagFrame = gtk_frame_new (_("Tag"));
    gtk_container_set_border_width (GTK_CONTAINER (TagFrame), 2);

    /* Box for the notebook (only for setting a border) */
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL ,0);
    gtk_container_add (GTK_CONTAINER (TagFrame), vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);

    /*
     * Note book
     */
    TagNoteBook = gtk_notebook_new ();
    gtk_notebook_popup_enable (GTK_NOTEBOOK (TagNoteBook));
    gtk_box_pack_start (GTK_BOX (vbox), TagNoteBook, TRUE, TRUE, 0);
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (TagNoteBook), GTK_POS_TOP);
    gtk_notebook_set_show_border (GTK_NOTEBOOK (TagNoteBook), FALSE);
    gtk_notebook_popup_enable (GTK_NOTEBOOK (TagNoteBook));

    /*
     * 1 - Page for common tag fields
     */
    label = gtk_label_new (_("Common"));

    table = et_grid_new (11, 11);
    gtk_notebook_append_page (GTK_NOTEBOOK (TagNoteBook), table, label);
    gtk_container_set_border_width (GTK_CONTAINER (table), 2);

    /* Title */
    priv->title_label = gtk_label_new (_("Title:"));
    et_grid_attach_full (GTK_GRID (table), priv->title_label, 0, 0, 1, 1,
                         FALSE, FALSE, TablePadding, TablePadding);
    gtk_misc_set_alignment (GTK_MISC (priv->title_label), 1.0, 0.5);

    TitleEntry = gtk_entry_new ();
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (TitleEntry),
                                       GTK_ENTRY_ICON_SECONDARY, "insert-text");
    et_grid_attach_full (GTK_GRID (table), TitleEntry, 1, 0, 9, 1, TRUE, TRUE,
                         TablePadding, TablePadding);

    et_tag_field_connect_signals (GTK_ENTRY (TitleEntry));
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (TitleEntry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _("Tag selected files with this title"));

    Attach_Popup_Menu_To_Tag_Entries (GTK_ENTRY (TitleEntry));

    /* Artist */
    priv->artist_label = gtk_label_new (_("Artist:"));
    et_grid_attach_full (GTK_GRID (table), priv->artist_label, 0, 1, 1, 1,
                         FALSE, FALSE, TablePadding, TablePadding);
    gtk_misc_set_alignment (GTK_MISC (priv->artist_label), 1.0, 0.5);

    ArtistEntry = gtk_entry_new ();
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (ArtistEntry),
                                       GTK_ENTRY_ICON_SECONDARY, "insert-text");
    et_grid_attach_full (GTK_GRID (table), ArtistEntry, 1, 1, 9, 1, TRUE, TRUE,
                         TablePadding,TablePadding);

    et_tag_field_connect_signals (GTK_ENTRY (ArtistEntry));
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (ArtistEntry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _("Tag selected files with this artist"));

    Attach_Popup_Menu_To_Tag_Entries (GTK_ENTRY (ArtistEntry));

    /* Album Artist */
    priv->album_artist_label = gtk_label_new (_("Album artist:"));
    et_grid_attach_full (GTK_GRID (table), priv->album_artist_label, 0, 2, 1,
                         1, FALSE, FALSE, TablePadding, TablePadding);
    gtk_misc_set_alignment (GTK_MISC (priv->album_artist_label), 1.0, 0.5);

    AlbumArtistEntry = gtk_entry_new ();
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (AlbumArtistEntry),
                                       GTK_ENTRY_ICON_SECONDARY, "insert-text");
    et_grid_attach_full (GTK_GRID (table), AlbumArtistEntry, 1, 2, 9, 1, TRUE,
                         TRUE, TablePadding, TablePadding);

    et_tag_field_connect_signals (GTK_ENTRY (AlbumArtistEntry));
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (AlbumArtistEntry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _("Tag selected files with this album artist"));

    Attach_Popup_Menu_To_Tag_Entries (GTK_ENTRY (AlbumArtistEntry));

    /* Album */
    priv->album_label = gtk_label_new (_("Album:"));
    et_grid_attach_full (GTK_GRID (table), priv->album_label, 0, 3, 1, 1,
                         FALSE, FALSE, TablePadding, TablePadding);
    gtk_misc_set_alignment (GTK_MISC (priv->album_label), 1.0, 0.5);

    AlbumEntry = gtk_entry_new ();
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (AlbumEntry),
                                       GTK_ENTRY_ICON_SECONDARY, "insert-text");
    et_grid_attach_full (GTK_GRID (table), AlbumEntry, 1, 3, 6, 1, TRUE, TRUE,
                         TablePadding, TablePadding);

    et_tag_field_connect_signals (GTK_ENTRY (AlbumEntry));
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (AlbumEntry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _("Tag selected files with this album name"));

    Attach_Popup_Menu_To_Tag_Entries (GTK_ENTRY (AlbumEntry));

    /* Disc Number */
    priv->disc_number_label = gtk_label_new (_("CD:"));
    et_grid_attach_full (GTK_GRID (table), priv->disc_number_label, 8, 3, 1, 1,
                         FALSE, FALSE, TablePadding, TablePadding);
    gtk_misc_set_alignment (GTK_MISC (priv->disc_number_label), 1.0, 0.5);

    DiscNumberEntry = gtk_entry_new ();
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (DiscNumberEntry),
                                       GTK_ENTRY_ICON_SECONDARY, "insert-text");
    et_grid_attach_full (GTK_GRID (table), DiscNumberEntry, 9, 3, 1, 1, TRUE,
                         TRUE, TablePadding, TablePadding);
    gtk_entry_set_width_chars (GTK_ENTRY (DiscNumberEntry), 3);
    /* FIXME should allow to type only something like : 1/3. */
    /*g_signal_connect(G_OBJECT(GTK_ENTRY(DiscNumberEntry)),"insert_text",G_CALLBACK(Insert_Only_Digit),NULL); */

    et_tag_field_connect_signals (GTK_ENTRY (DiscNumberEntry));
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (DiscNumberEntry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _("Tag selected files with this disc number"));

    Attach_Popup_Menu_To_Tag_Entries (GTK_ENTRY (DiscNumberEntry));

    /* Year */
    priv->year_label = gtk_label_new (_("Year:"));
    et_grid_attach_full (GTK_GRID (table), priv->year_label, 0, 4, 1, 1, FALSE,
                         FALSE, TablePadding, TablePadding);
    gtk_misc_set_alignment (GTK_MISC (priv->year_label), 1.0, 0.5);

    YearEntry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (YearEntry), 4);
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (YearEntry),
                                       GTK_ENTRY_ICON_SECONDARY, "insert-text");
    et_grid_attach_full (GTK_GRID (table), YearEntry, 1, 4, 1, 1, TRUE, TRUE,
                         TablePadding, TablePadding);
    gtk_entry_set_width_chars (GTK_ENTRY (YearEntry), 5);
    g_signal_connect (YearEntry, "insert-text", G_CALLBACK (Insert_Only_Digit),
                      NULL);
    g_signal_connect (YearEntry, "activate", G_CALLBACK (Parse_Date), NULL);
    g_signal_connect (YearEntry, "focus-out-event", G_CALLBACK (Parse_Date),
                      NULL);

    et_tag_field_connect_signals (GTK_ENTRY (YearEntry));
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (YearEntry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _("Tag selected files with this year"));

    /* Small vertical separator */
    separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
    et_grid_attach_full (GTK_GRID (table), separator, 3, 4, 1, 1, FALSE, FALSE,
                         TablePadding,TablePadding);

    /* Track and Track total */
    priv->track_sequence_button = gtk_button_new ();
    gtk_widget_set_size_request (priv->track_sequence_button, MButtonSize,
                                 MButtonSize);
    et_grid_attach_full (GTK_GRID (table), priv->track_sequence_button, 4, 4,
                         1, 1, FALSE, FALSE, TablePadding, TablePadding);
    g_signal_connect (priv->track_sequence_button, "clicked",
                      G_CALLBACK (Mini_Button_Clicked), NULL);
    gtk_widget_set_tooltip_text (priv->track_sequence_button,
                                 _("Number selected tracks sequentially. "
                                   "Starts at 01 in each subdirectory."));
    /* Pixmap into priv->track_sequence_button button. */
    image = Create_Xpm_Image ((const char **)sequence_track_xpm);
    gtk_container_add (GTK_CONTAINER (priv->track_sequence_button), image);
    gtk_widget_set_can_default (priv->track_sequence_button, TRUE);
    gtk_widget_set_can_focus (priv->track_sequence_button, FALSE);

    priv->track_label = gtk_label_new (_("Track #:"));
    et_grid_attach_full (GTK_GRID (table), priv->track_label, 5, 4, 1, 1,
                         FALSE, FALSE, TablePadding, TablePadding);
    gtk_misc_set_alignment (GTK_MISC (priv->track_label), 1.0, 0.5);

    if (TrackEntryComboModel != NULL)
    {
        gtk_list_store_clear (TrackEntryComboModel);
    }
    else
    {
        TrackEntryComboModel = gtk_list_store_new (MISC_COMBO_COUNT,
                                                   G_TYPE_STRING);
    }

    TrackEntryCombo = gtk_combo_box_new_with_model_and_entry (GTK_TREE_MODEL (TrackEntryComboModel));
    gtk_combo_box_set_entry_text_column (GTK_COMBO_BOX (TrackEntryCombo),
                                         MISC_COMBO_TEXT);
    et_grid_attach_full (GTK_GRID (table), TrackEntryCombo, 6, 4, 1, 1, TRUE,
                         TRUE, TablePadding, TablePadding);
    gtk_combo_box_set_wrap_width (GTK_COMBO_BOX (TrackEntryCombo), 3); // Three columns to display track numbers list

    gtk_entry_set_width_chars (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (TrackEntryCombo))),
                               2);
    g_signal_connect (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (TrackEntryCombo))),
                      "insert-text", G_CALLBACK (Insert_Only_Digit), NULL);

    label = gtk_label_new ("/");
    et_grid_attach_full (GTK_GRID (table), label, 7, 4, 1, 1, FALSE, FALSE,
                         TablePadding, TablePadding);
    gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

    priv->track_number_button = gtk_button_new ();
    gtk_widget_set_size_request (priv->track_number_button, MButtonSize,
                                 MButtonSize);
    et_grid_attach_full (GTK_GRID (table), priv->track_number_button, 8, 4, 1,
                         1, FALSE, FALSE, TablePadding, TablePadding);
    g_signal_connect (priv->track_number_button, "clicked",
                      G_CALLBACK (Mini_Button_Clicked), NULL);
    gtk_widget_set_tooltip_text (priv->track_number_button,
                                 _("Set the number of files, in the same directory of the displayed file, to the selected tracks."));
    /* Pixmap into priv->track_number_button button. */
    image = Create_Xpm_Image ((const char **)sequence_track_xpm);
    gtk_container_add (GTK_CONTAINER (priv->track_number_button), image);
    gtk_widget_set_can_default (priv->track_number_button, TRUE);
    gtk_widget_set_can_focus (priv->track_number_button, FALSE);

    TrackTotalEntry = gtk_entry_new();
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (TrackTotalEntry),
                                       GTK_ENTRY_ICON_SECONDARY, "insert-text");
    et_grid_attach_full (GTK_GRID (table), TrackTotalEntry, 9, 4, 1, 1, TRUE,
                         TRUE, TablePadding, TablePadding);
    gtk_entry_set_width_chars (GTK_ENTRY (TrackTotalEntry), 3);
    g_signal_connect (GTK_ENTRY (TrackTotalEntry), "insert-text",
                      G_CALLBACK (Insert_Only_Digit), NULL);

    et_tag_field_connect_signals (GTK_ENTRY (TrackTotalEntry));
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (TrackTotalEntry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _("Tag selected files with this number of tracks"));

    /* Genre */
    priv->genre_label = gtk_label_new (_("Genre:"));
    et_grid_attach_full (GTK_GRID (table), priv->genre_label, 0, 5, 1, 1,
                         FALSE, FALSE, TablePadding, TablePadding);
    gtk_misc_set_alignment (GTK_MISC (priv->genre_label), 1.0, 0.5);

    if (GenreComboModel != NULL)
    {
        gtk_list_store_clear (GenreComboModel);
    }
    else
    {
        GenreComboModel = gtk_list_store_new (MISC_COMBO_COUNT, G_TYPE_STRING);
    }
    GenreCombo = gtk_combo_box_new_with_model_and_entry (GTK_TREE_MODEL (GenreComboModel));
    gtk_combo_box_set_entry_text_column (GTK_COMBO_BOX (GenreCombo),
                                         MISC_COMBO_TEXT);
    completion = gtk_entry_completion_new ();
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GenreCombo))),
                                       GTK_ENTRY_ICON_SECONDARY, "insert-text");
    gtk_entry_set_completion (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GenreCombo))),
                              completion);
    g_object_unref (completion);
    gtk_entry_completion_set_model (completion,
                                    GTK_TREE_MODEL (GenreComboModel));
    gtk_entry_completion_set_text_column (completion, 0);
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (GenreComboModel),
                                     MISC_COMBO_TEXT, Combo_Alphabetic_Sort,
                                     NULL, NULL);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (GenreComboModel),
                                          MISC_COMBO_TEXT, GTK_SORT_ASCENDING);
    et_grid_attach_full (GTK_GRID (table), GenreCombo, 1, 5, 9, 1, TRUE, TRUE,
                         TablePadding, TablePadding);
    Load_Genres_List_To_UI ();
    gtk_combo_box_set_wrap_width (GTK_COMBO_BOX (GenreCombo), 2); // Two columns to display genres list

    et_tag_field_connect_signals (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GenreCombo))));
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GenreCombo))),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _("Tag selected files with this genre"));

    Attach_Popup_Menu_To_Tag_Entries (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GenreCombo))));

    /* Comment */
    priv->comment_label = gtk_label_new (_("Comment:"));
    et_grid_attach_full (GTK_GRID (table), priv->comment_label, 0, 6, 1, 1,
                         FALSE, FALSE, TablePadding, TablePadding);
    gtk_misc_set_alignment (GTK_MISC (priv->comment_label), 1.0, 0.5);

    CommentEntry = gtk_entry_new ();
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (CommentEntry),
                                       GTK_ENTRY_ICON_SECONDARY, "insert-text");
    et_grid_attach_full (GTK_GRID (table), CommentEntry, 1, 6, 9, 1, TRUE,
                         TRUE, TablePadding, TablePadding);

    et_tag_field_connect_signals (GTK_ENTRY (CommentEntry));
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (CommentEntry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _("Tag selected files with this comment"));

    Attach_Popup_Menu_To_Tag_Entries (GTK_ENTRY (CommentEntry));

    /* Composer (name of the composers) */
    priv->composer_label = gtk_label_new (_("Composer:"));
    et_grid_attach_full (GTK_GRID (table), priv->composer_label, 0, 7, 1, 1,
                         FALSE, FALSE, TablePadding, TablePadding);
    gtk_misc_set_alignment (GTK_MISC (priv->composer_label), 1.0, 0.5);

    ComposerEntry = gtk_entry_new ();
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (ComposerEntry),
                                       GTK_ENTRY_ICON_SECONDARY, "insert-text");
    et_grid_attach_full (GTK_GRID (table), ComposerEntry, 1, 7, 9, 1, TRUE,
                         TRUE, TablePadding, TablePadding);

    et_tag_field_connect_signals (GTK_ENTRY (ComposerEntry));
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (ComposerEntry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _("Tag selected files with this composer"));

    Attach_Popup_Menu_To_Tag_Entries (GTK_ENTRY (ComposerEntry));

    /* Translators: Original Artist / Performer. Please try to keep this string
     * as short as possible, as it must fit into a narrow column. */
    priv->orig_artist_label = gtk_label_new (_("Orig. artist:"));
    et_grid_attach_full (GTK_GRID (table), priv->orig_artist_label, 0, 8, 1, 1,
                         FALSE, FALSE, TablePadding, TablePadding);
    gtk_misc_set_alignment (GTK_MISC (priv->orig_artist_label), 1.0, 0.5);

    OrigArtistEntry = gtk_entry_new ();
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (OrigArtistEntry),
                                       GTK_ENTRY_ICON_SECONDARY, "insert-text");
    et_grid_attach_full (GTK_GRID (table), OrigArtistEntry, 1, 8, 9, 1, TRUE,
                         TRUE, TablePadding, TablePadding);

    et_tag_field_connect_signals (GTK_ENTRY (OrigArtistEntry));
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (OrigArtistEntry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _("Tag selected files with this original artist"));

    Attach_Popup_Menu_To_Tag_Entries (GTK_ENTRY (OrigArtistEntry));


    /* Copyright */
    priv->copyright_label = gtk_label_new (_("Copyright:"));
    et_grid_attach_full (GTK_GRID (table), priv->copyright_label, 0, 9, 1, 1,
                         FALSE, FALSE, TablePadding, TablePadding);
    gtk_misc_set_alignment (GTK_MISC (priv->copyright_label), 1.0, 0.5);

    CopyrightEntry = gtk_entry_new ();
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (CopyrightEntry),
                                       GTK_ENTRY_ICON_SECONDARY, "insert-text");
    et_grid_attach_full (GTK_GRID (table), CopyrightEntry, 1, 9, 9, 1, TRUE,
                         TRUE, TablePadding, TablePadding);

    et_tag_field_connect_signals (GTK_ENTRY (CopyrightEntry));
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (CopyrightEntry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _("Tag selected files with this copyright"));

    Attach_Popup_Menu_To_Tag_Entries (GTK_ENTRY (CopyrightEntry));


    /* URL */
    priv->url_label = gtk_label_new (_("URL:"));
    et_grid_attach_full (GTK_GRID (table), priv->url_label, 0, 10, 1, 1, FALSE,
                         FALSE, TablePadding, TablePadding);
    gtk_misc_set_alignment (GTK_MISC (priv->url_label), 1.0, 0.5);

    URLEntry = gtk_entry_new ();
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (URLEntry),
                                       GTK_ENTRY_ICON_SECONDARY, "insert-text");
    et_grid_attach_full (GTK_GRID (table), URLEntry, 1, 10, 9, 1, TRUE, TRUE,
                         TablePadding, TablePadding);

    et_tag_field_connect_signals (GTK_ENTRY (URLEntry));
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (URLEntry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _("Tag selected files with this URL"));

    Attach_Popup_Menu_To_Tag_Entries(GTK_ENTRY(URLEntry));


    /* Encoded by */
    priv->encoded_by_label = gtk_label_new (_("Encoded by:"));
    et_grid_attach_full (GTK_GRID (table), priv->encoded_by_label, 0, 11, 1, 1,
                         FALSE, FALSE, TablePadding, TablePadding);
    gtk_misc_set_alignment (GTK_MISC (priv->encoded_by_label), 1.0, 0.5);

    EncodedByEntry = gtk_entry_new ();
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (EncodedByEntry),
                                       GTK_ENTRY_ICON_SECONDARY, "insert-text");
    et_grid_attach_full (GTK_GRID (table), EncodedByEntry, 1, 11, 9, 1, TRUE,
                         TRUE, TablePadding, TablePadding);

    et_tag_field_connect_signals (GTK_ENTRY (EncodedByEntry));
    gtk_entry_set_icon_tooltip_text (GTK_ENTRY (EncodedByEntry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     _("Tag selected files with this encoder name"));

    Attach_Popup_Menu_To_Tag_Entries (GTK_ENTRY (EncodedByEntry));

    /* Set focus chain. */
    focus_chain = g_list_prepend (focus_chain, TitleEntry);
    focus_chain = g_list_prepend (focus_chain, ArtistEntry);
    focus_chain = g_list_prepend (focus_chain, AlbumArtistEntry);
    focus_chain = g_list_prepend (focus_chain, AlbumEntry);
    focus_chain = g_list_prepend (focus_chain, DiscNumberEntry);
    focus_chain = g_list_prepend (focus_chain, YearEntry);
    focus_chain = g_list_prepend (focus_chain, TrackEntryCombo);
    focus_chain = g_list_prepend (focus_chain, TrackTotalEntry);
    focus_chain = g_list_prepend (focus_chain, GenreCombo);
    focus_chain = g_list_prepend (focus_chain, CommentEntry);
    focus_chain = g_list_prepend (focus_chain, ComposerEntry);
    focus_chain = g_list_prepend (focus_chain, OrigArtistEntry);
    focus_chain = g_list_prepend (focus_chain, CopyrightEntry);
    focus_chain = g_list_prepend (focus_chain, URLEntry);
    focus_chain = g_list_prepend (focus_chain, EncodedByEntry);
    /* More efficient than using g_list_append(), which must traverse the
     * whole list. */
    focus_chain = g_list_reverse (focus_chain);
    gtk_container_set_focus_chain (GTK_CONTAINER (table), focus_chain);
    g_list_free (focus_chain);

    /*
     * 2 - Page for extra tag fields
     */
    /* Also used in ET_Display_File_Tag_To_UI. */
    label = gtk_label_new (_("Images"));

    priv->images_tab = table = et_grid_new (1, 2);
    gtk_notebook_append_page (GTK_NOTEBOOK (TagNoteBook), table, label);
    gtk_container_set_border_width (GTK_CONTAINER (table), 2);

    /* Scroll window for PictureEntryView. */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
    et_grid_attach_full (GTK_GRID (table), scrolled_window, 0, 0, 1, 1,
                         TRUE, TRUE, TablePadding, TablePadding);

    priv->images_model = gtk_list_store_new (PICTURE_COLUMN_COUNT,
                                             GDK_TYPE_PIXBUF, G_TYPE_STRING,
                                             G_TYPE_POINTER);
    PictureEntryView = gtk_tree_view_new_with_model (GTK_TREE_MODEL (priv->images_model));
    g_object_unref (priv->images_model);
    gtk_container_add (GTK_CONTAINER (scrolled_window), PictureEntryView);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (PictureEntryView), FALSE);
    gtk_widget_set_size_request (PictureEntryView, -1, 200);
    gtk_widget_set_tooltip_text (PictureEntryView,
                                 _("You can use drag and drop to add an image"));

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (PictureEntryView));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

    renderer = gtk_cell_renderer_pixbuf_new ();
    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "pixbuf",
                                         PICTURE_COLUMN_PIC, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (PictureEntryView), column);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "text",
                                         PICTURE_COLUMN_TEXT, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (PictureEntryView), column);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    /* Activate Drag'n'Drop for the PictureEntryView. */
    gtk_drag_dest_set(GTK_WIDGET(PictureEntryView),
                      GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                      drops, sizeof(drops) / sizeof(GtkTargetEntry),
                      GDK_ACTION_COPY);
    g_signal_connect (PictureEntryView, "drag-data-received",
                      G_CALLBACK (Tag_Area_Picture_Drag_Data), 0);
    g_signal_connect (selection, "changed",
                      G_CALLBACK (Picture_Selection_Changed_cb), NULL);
    g_signal_connect (PictureEntryView, "button-press-event",
                      G_CALLBACK (Picture_Entry_View_Button_Pressed), NULL);
    g_signal_connect (PictureEntryView, "key-press-event",
                      G_CALLBACK (Picture_Entry_View_Key_Pressed), NULL);

    /* Picture action toolbar. */
    toolbar = gtk_toolbar_new ();
    gtk_style_context_add_class (gtk_widget_get_style_context (toolbar),
                                 GTK_STYLE_CLASS_INLINE_TOOLBAR);
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
    gtk_toolbar_set_icon_size (GTK_TOOLBAR (toolbar), GTK_ICON_SIZE_MENU);
    et_grid_attach_full (GTK_GRID (table), toolbar, 0, 1, 1, 1, FALSE, FALSE,
                        TablePadding, TablePadding);

    /* TODO: Make the icons use the symbolic variants. */
    icon = g_themed_icon_new_with_default_fallbacks ("list-add");
    image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_MENU);
    add_image_toolitem = gtk_tool_button_new (image, NULL);
    g_object_unref (icon);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), add_image_toolitem, -1);
    gtk_widget_set_tooltip_text (GTK_WIDGET (add_image_toolitem),
                                 _("Add images to the tag"));
    g_signal_connect (add_image_toolitem, "clicked",
                      G_CALLBACK (Picture_Add_Button_Clicked), NULL);

    /* Activate Drag'n'Drop for the add_image_toolitem. */
    gtk_drag_dest_set (GTK_WIDGET (add_image_toolitem),
                       GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                       drops, sizeof(drops) / sizeof(GtkTargetEntry),
                       GDK_ACTION_COPY);
    g_signal_connect (add_image_toolitem, "drag-data-received",
                      G_CALLBACK (Tag_Area_Picture_Drag_Data), 0);

    icon = g_themed_icon_new_with_default_fallbacks ("list-remove");
    image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_MENU);
    remove_image_toolitem = gtk_tool_button_new (image, NULL);
    g_object_unref (icon);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), remove_image_toolitem, -1);
    gtk_widget_set_tooltip_text (GTK_WIDGET (remove_image_toolitem),
                                 _("Remove selected images from the tag"));
    gtk_widget_set_sensitive (GTK_WIDGET (remove_image_toolitem), FALSE);
    g_signal_connect (remove_image_toolitem, "clicked",
                      G_CALLBACK (Picture_Clear_Button_Clicked), NULL);

    icon = g_themed_icon_new_with_default_fallbacks ("document-save");
    image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_MENU);
    save_image_toolitem = gtk_tool_button_new (image, NULL);
    g_object_unref (icon);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), save_image_toolitem, -1);
    gtk_widget_set_tooltip_text (GTK_WIDGET (save_image_toolitem),
                                 _("Save the selected images to files"));
    gtk_widget_set_sensitive (GTK_WIDGET (save_image_toolitem), FALSE);
    g_signal_connect (save_image_toolitem, "clicked",
                      G_CALLBACK (Picture_Save_Button_Clicked), NULL);

    icon = g_themed_icon_new_with_default_fallbacks ("document-properties");
    image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_MENU);
    image_properties_toolitem = gtk_tool_button_new (image, NULL);
    g_object_unref (icon);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), image_properties_toolitem, -1);
    gtk_widget_set_tooltip_text (GTK_WIDGET (image_properties_toolitem),
                                 _("Edit image properties"));
    gtk_widget_set_sensitive (GTK_WIDGET (image_properties_toolitem), FALSE);
    g_signal_connect (image_properties_toolitem, "clicked",
                      G_CALLBACK (Picture_Properties_Button_Clicked), NULL);

    icon = g_themed_icon_new_with_default_fallbacks ("insert-image");
    image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_MENU);
    priv->apply_image_toolitem = gtk_tool_button_new (image, NULL);
    g_object_unref (icon);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), priv->apply_image_toolitem, -1);
    gtk_widget_set_tooltip_text (GTK_WIDGET (priv->apply_image_toolitem),
                                 _("Tag selected files with these images"));
    g_signal_connect (priv->apply_image_toolitem, "clicked",
                      G_CALLBACK (Mini_Button_Clicked), NULL);

    /*Attach_Popup_Menu_To_Tag_Entries (GTK_ENTRY (PictureEntryView));*/

    gtk_widget_show_all (TagFrame);
    return TagFrame;
}

static void
et_application_window_show_cddb_dialog (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    if (priv->cddb_dialog)
    {
        gtk_widget_show (priv->cddb_dialog);
    }
    else
    {
        priv->cddb_dialog = GTK_WIDGET (et_cddb_dialog_new ());
        gtk_widget_show_all (priv->cddb_dialog);
    }
}

/*
 * Delete the file ETFile
 */
static gint
delete_file (ET_File *ETFile, gboolean multiple_files, GError **error)
{
    GtkWidget *msgdialog;
    GtkWidget *msgdialog_check_button = NULL;
    gchar *cur_filename;
    gchar *cur_filename_utf8;
    gchar *basename_utf8;
    gint response;
    gint stop_loop;

    g_return_val_if_fail (ETFile != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    /* Filename of the file to delete. */
    cur_filename      = ((File_Name *)(ETFile->FileNameCur)->data)->value;
    cur_filename_utf8 = ((File_Name *)(ETFile->FileNameCur)->data)->value_utf8;
    basename_utf8 = g_path_get_basename (cur_filename_utf8);

    /*
     * Remove the file
     */
    if (g_settings_get_boolean (MainSettings, "confirm-delete-file")
        && !SF_HideMsgbox_Delete_File)
    {
        if (multiple_files)
        {
            GtkWidget *message_area;
            msgdialog = gtk_message_dialog_new(GTK_WINDOW(MainWindow),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_QUESTION,
                                               GTK_BUTTONS_NONE,
                                               _("Do you really want to delete the file '%s'?"),
                                               basename_utf8);
            message_area = gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(msgdialog));
            msgdialog_check_button = gtk_check_button_new_with_label(_("Repeat action for the remaining files"));
            gtk_container_add(GTK_CONTAINER(message_area),msgdialog_check_button);
            gtk_dialog_add_buttons(GTK_DIALOG(msgdialog),GTK_STOCK_NO,GTK_RESPONSE_NO,GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_DELETE,GTK_RESPONSE_YES,NULL);
            gtk_window_set_title(GTK_WINDOW(msgdialog),_("Delete File"));
            //GTK_TOGGLE_BUTTON(msgbox_check_button)->active = TRUE; // Checked by default
        }else
        {
            msgdialog = gtk_message_dialog_new(GTK_WINDOW(MainWindow),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_QUESTION,
                                               GTK_BUTTONS_NONE,
                                               _("Do you really want to delete the file '%s'?"),
                                               basename_utf8);
            gtk_window_set_title(GTK_WINDOW(msgdialog),_("Delete File"));
            gtk_dialog_add_buttons(GTK_DIALOG(msgdialog),GTK_STOCK_CANCEL,GTK_RESPONSE_NO,GTK_STOCK_DELETE,GTK_RESPONSE_YES,NULL);
        }
        gtk_dialog_set_default_response (GTK_DIALOG (msgdialog),
                                         GTK_RESPONSE_YES);
        SF_ButtonPressed_Delete_File = response = gtk_dialog_run(GTK_DIALOG(msgdialog));
        if (msgdialog_check_button && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(msgdialog_check_button)))
            SF_HideMsgbox_Delete_File = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(msgdialog_check_button));
        gtk_widget_destroy(msgdialog);
    }else
    {
        if (SF_HideMsgbox_Delete_File)
            response = SF_ButtonPressed_Delete_File;
        else
            response = GTK_RESPONSE_YES;
    }

    switch (response)
    {
        case GTK_RESPONSE_YES:
        {
            GFile *cur_file = g_file_new_for_path (cur_filename);

            if (g_file_delete (cur_file, NULL, error))
            {
                gchar *msg = g_strdup_printf(_("File '%s' deleted"), basename_utf8);
                Statusbar_Message(msg,FALSE);
                g_free(msg);
                g_free(basename_utf8);
                g_object_unref (cur_file);
                g_assert (error == NULL || *error == NULL);
                return 1;
            }

            /* Error in deleting file. */
            g_assert (error == NULL || *error != NULL);
            break;
        }
        case GTK_RESPONSE_NO:
            break;
        case GTK_RESPONSE_CANCEL:
        case GTK_RESPONSE_DELETE_EVENT:
            stop_loop = -1;
            g_free(basename_utf8);
            return stop_loop;
            break;
        default:
            g_assert_not_reached ();
            break;
    }

    g_free(basename_utf8);
    return 0;
}

static void
on_open_with (GSimpleAction *action,
              GVariant *variant,
              gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_browser_show_open_files_with_dialog (ET_BROWSER (priv->browser));
}

static void
on_run_player (GSimpleAction *action,
               GVariant *variant,
               gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_browser_run_player_for_selection (ET_BROWSER (priv->browser));
}

static void
on_invert_selection (GSimpleAction *action,
                     GVariant *variant,
                     gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI (ETCore->ETFileDisplayed);

    et_browser_invert_selection (ET_BROWSER (priv->browser));
    et_application_window_update_actions (self);
}

static void
on_delete (GSimpleAction *action,
           GVariant *variant,
           gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;
    GList *selfilelist;
    GList *rowreflist = NULL;
    GList *l;
    gint   progress_bar_index;
    gint   saving_answer;
    gint   nb_files_to_delete;
    gint   nb_files_deleted = 0;
    gchar *msg;
    gchar progress_bar_text[30];
    double fraction;
    GtkTreeModel *treemodel;
    GtkTreeRowReference *rowref;
    GtkTreeSelection *selection;
    GError *error = NULL;

    g_return_if_fail (ETCore->ETFileDisplayedList != NULL);

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    /* Number of files to save */
    selection = et_application_window_browser_get_selection (self);
    nb_files_to_delete = gtk_tree_selection_count_selected_rows (selection);

    /* Initialize status bar */
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar),0);
    progress_bar_index = 0;
    g_snprintf(progress_bar_text, 30, "%d/%d", progress_bar_index, nb_files_to_delete);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ProgressBar), progress_bar_text);

    /* Set to unsensitive all command buttons (except Quit button) */
    et_application_window_disable_command_actions (self);
    et_application_window_browser_set_sensitive (self, FALSE);
    et_application_window_tag_area_set_sensitive (self, FALSE);
    et_application_window_file_area_set_sensitive (self, FALSE);

    /* Show msgbox (if needed) to ask confirmation */
    SF_HideMsgbox_Delete_File = 0;

    selfilelist = gtk_tree_selection_get_selected_rows (selection, &treemodel);

    for (l = selfilelist; l != NULL; l = g_list_next (l))
    {
        rowref = gtk_tree_row_reference_new (treemodel, l->data);
        rowreflist = g_list_prepend (rowreflist, rowref);
    }

    g_list_free_full (selfilelist, (GDestroyNotify)gtk_tree_path_free);
    rowreflist = g_list_reverse (rowreflist);

    for (l = rowreflist; l != NULL; l = g_list_next (l))
    {
        GtkTreePath *path;
        ET_File *ETFile;

        path = gtk_tree_row_reference_get_path (l->data);
        ETFile = et_browser_get_et_file_from_path (ET_BROWSER (priv->browser),
                                                   path);
        gtk_tree_path_free (path);

        ET_Display_File_Data_To_UI(ETFile);
        et_application_window_browser_select_file_by_et_file (self, ETFile,
                                                              FALSE);
        fraction = (++progress_bar_index) / (double) nb_files_to_delete;
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (ProgressBar),
                                       fraction);
        g_snprintf (progress_bar_text, 30, "%d/%d", progress_bar_index,
                    nb_files_to_delete);
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (ProgressBar),
                                   progress_bar_text);
        /* FIXME: Needed to refresh status bar */
        while (gtk_events_pending ())
        {
            gtk_main_iteration ();
        }

        saving_answer = delete_file (ETFile,
                                     nb_files_to_delete > 1 ? TRUE : FALSE,
                                     &error);

        switch (saving_answer)
        {
            case 1:
                nb_files_deleted += saving_answer;
                /* Remove file in the browser (corresponding line in the
                 * clist). */
                et_browser_remove_file (ET_BROWSER (priv->browser), ETFile);
                /* Remove file from file list. */
                ET_Remove_File_From_File_List (ETFile);
                break;
            case 0:
                /* Distinguish between the file being skipped, and there being
                 * an error during deletion. */
                if (error)
                {
                    Log_Print (LOG_ERROR, _("Cannot delete file (%s)"),
                               error->message);
                    g_clear_error (&error);
                }
                break;
            case -1:
                // Stop deleting files + reinit progress bar
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar),0.0);
                /* To update state of command buttons. */
                et_application_window_update_actions (self);
                et_application_window_browser_set_sensitive (self, TRUE);
                et_application_window_tag_area_set_sensitive (self, TRUE);
                et_application_window_file_area_set_sensitive (self, TRUE);

                return; /*We stop all actions. */
        }
    }

    g_list_free_full (rowreflist, (GDestroyNotify)gtk_tree_row_reference_free);

    if (nb_files_deleted < nb_files_to_delete)
        msg = g_strdup (_("Files have been partially deleted"));
    else
        msg = g_strdup (_("All files have been deleted"));

    /* It's important to displayed the new item, as it'll check the changes in et_browser_toggle_display_mode. */
    if (ETCore->ETFileDisplayed)
        ET_Display_File_Data_To_UI(ETCore->ETFileDisplayed);
    /*else if (ET_Displayed_File_List_Current())
        ET_Display_File_Data_To_UI((ET_File *)ET_Displayed_File_List_Current()->data);*/

    /* Load list... */
    et_browser_load_file_list (ET_BROWSER (priv->browser),
                               ETCore->ETFileDisplayedList, NULL);
    /* Rebuild the list... */
    /*et_browser_toggle_display_mode (ET_BROWSER (priv->browser));*/

    /* To update state of command buttons */
    et_application_window_update_actions (self);
    et_application_window_browser_set_sensitive (self, TRUE);
    et_application_window_tag_area_set_sensitive (self, TRUE);
    et_application_window_file_area_set_sensitive (self, TRUE);

    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ProgressBar), "");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ProgressBar), 0);
    Statusbar_Message(msg,TRUE);
    g_free(msg);

    return;
}

static void
on_undo_file_changes (GSimpleAction *action,
                      GVariant *variant,
                      gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;
    GList *selfilelist = NULL;
    GList *l;
    gboolean state = FALSE;
    ET_File *etfile;
    GtkTreeSelection *selection;

    g_return_if_fail (ETCore->ETFileDisplayedList != NULL);

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    selection = et_application_window_browser_get_selection (self);
    selfilelist = gtk_tree_selection_get_selected_rows(selection, NULL);

    for (l = selfilelist; l != NULL; l = g_list_next (l))
    {
        etfile = et_browser_get_et_file_from_path (ET_BROWSER (priv->browser),
                                                   l->data);
        state |= ET_Undo_File_Data(etfile);
    }

    g_list_free_full (selfilelist, (GDestroyNotify)gtk_tree_path_free);

    /* Refresh the whole list (faster than file by file) to show changes. */
    et_application_window_browser_refresh_list (self);

    /* Display the current file */
    ET_Display_File_Data_To_UI(ETCore->ETFileDisplayed);
    et_application_window_update_actions (self);

    //ET_Debug_Print_File_List(ETCore->ETFileList,__FILE__,__LINE__,__FUNCTION__);
}

static void
on_redo_file_changes (GSimpleAction *action,
                      GVariant *variant,
                      gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;
    GList *selfilelist = NULL;
    GList *l;
    gboolean state = FALSE;
    ET_File *etfile;
    GtkTreeSelection *selection;

    g_return_if_fail (ETCore->ETFileDisplayedList != NULL);

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    selection = et_application_window_browser_get_selection (ET_APPLICATION_WINDOW (user_data));
    selfilelist = gtk_tree_selection_get_selected_rows(selection, NULL);

    for (l = selfilelist; l != NULL; l = g_list_next (l))
    {
        etfile = et_browser_get_et_file_from_path (ET_BROWSER (priv->browser),
                                                   l->data);
        state |= ET_Redo_File_Data(etfile);
    }

    g_list_free_full (selfilelist, (GDestroyNotify)gtk_tree_path_free);

    /* Refresh the whole list (faster than file by file) to show changes. */
    et_application_window_browser_refresh_list (ET_APPLICATION_WINDOW (user_data));

    /* Display the current file */
    ET_Display_File_Data_To_UI(ETCore->ETFileDisplayed);
    et_application_window_update_actions (self);
}

static void
on_save (GSimpleAction *action,
         GVariant *variant,
         gpointer user_data)
{
    Action_Save_Selected_Files ();
}

static void
on_save_force (GSimpleAction *action,
               GVariant *variant,
               gpointer user_data)
{
    Action_Force_Saving_Selected_Files ();
}

static void
on_find (GSimpleAction *action,
         GVariant *variant,
         gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    if (priv->search_dialog)
    {
        gtk_widget_show (priv->search_dialog);
    }
    else
    {
        priv->search_dialog = GTK_WIDGET (et_search_dialog_new ());
        gtk_widget_show_all (priv->search_dialog);
    }
}

static void
on_select_all (GSimpleAction *action,
               GVariant *variant,
               gpointer user_data)
{
    GtkWidget *focused;

    /* Use the currently-focused widget and "select all" as appropriate.
     * https://bugzilla.gnome.org/show_bug.cgi?id=697515 */
    focused = gtk_window_get_focus (GTK_WINDOW (user_data));

    if (GTK_IS_EDITABLE (focused))
    {
        gtk_editable_select_region (GTK_EDITABLE (focused), 0, -1);
    }
    else if (focused == PictureEntryView)
    {
        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (focused));
        gtk_tree_selection_select_all (selection);
    }
    else /* Assume that other widgets should select all in the file view. */
    {
        EtApplicationWindowPrivate *priv;
        EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

        priv = et_application_window_get_instance_private (self);

        /* Save the current displayed data */
        ET_Save_File_Data_From_UI (ETCore->ETFileDisplayed);

        et_browser_select_all (ET_BROWSER (priv->browser));
        et_application_window_update_actions (self);
    }
}

static void
on_unselect_all (GSimpleAction *action,
                 GVariant *variant,
                 gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;
    GtkWidget *focused;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    focused = gtk_window_get_focus (GTK_WINDOW (user_data));

    if (GTK_IS_EDITABLE (focused))
    {
        GtkEditable *editable;
        gint pos;

        editable = GTK_EDITABLE (focused);
        pos = gtk_editable_get_position (editable);
        gtk_editable_select_region (editable, 0, 0);
        gtk_editable_set_position (editable, pos);
    }
    else if (focused == PictureEntryView)
    {
        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (focused));
        gtk_tree_selection_unselect_all (selection);
    }
    else /* Assume that other widgets should unselect all in the file view. */
    {
        /* Save the current displayed data */
        ET_Save_File_Data_From_UI (ETCore->ETFileDisplayed);

        et_browser_unselect_all (ET_BROWSER (priv->browser));

        ETCore->ETFileDisplayed = NULL;
    }
}

static void
on_undo_last_changes (GSimpleAction *action,
                      GVariant *variant,
                      gpointer user_data)
{
    EtApplicationWindow *self;
    ET_File *ETFile;

    self = ET_APPLICATION_WINDOW (user_data);

    g_return_if_fail (ETCore->ETFileList != NULL);

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI (ETCore->ETFileDisplayed);

    ETFile = ET_Undo_History_File_Data ();

    if (ETFile)
    {
        ET_Display_File_Data_To_UI (ETFile);
        et_application_window_browser_select_file_by_et_file (self, ETFile,
                                                              TRUE);
        et_application_window_browser_refresh_file_in_list (self, ETFile);
    }

    et_application_window_update_actions (self);
}

static void
on_redo_last_changes (GSimpleAction *action,
                      GVariant *variant,
                      gpointer user_data)
{
    EtApplicationWindow *self;
    ET_File *ETFile;

    self = ET_APPLICATION_WINDOW (user_data);

    g_return_if_fail (ETCore->ETFileDisplayedList != NULL);

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI (ETCore->ETFileDisplayed);

    ETFile = ET_Redo_History_File_Data ();

    if (ETFile)
    {
        ET_Display_File_Data_To_UI (ETFile);
        et_application_window_browser_select_file_by_et_file (self, ETFile,
                                                              TRUE);
        et_application_window_browser_refresh_file_in_list (self, ETFile);
    }

    et_application_window_update_actions (self);
}

static void
on_remove_tags (GSimpleAction *action,
                GVariant *variant,
                gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;
    GList *selfilelist = NULL;
    GList *l;
    ET_File *etfile;
    File_Tag *FileTag;
    gint progress_bar_index;
    gint selectcount;
    double fraction;
    GtkTreeSelection *selection;

    g_return_if_fail (ETCore->ETFileDisplayedList != NULL);

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI (ETCore->ETFileDisplayed);

    /* Initialize status bar */
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (ProgressBar), 0.0);
    selection = et_application_window_browser_get_selection (self);
    selectcount = gtk_tree_selection_count_selected_rows (selection);
    progress_bar_index = 0;

    selfilelist = gtk_tree_selection_get_selected_rows (selection, NULL);

    for (l = selfilelist; l != NULL; l = g_list_next (l))
    {
        etfile = et_browser_get_et_file_from_path (ET_BROWSER (priv->browser),
                                                   l->data);
        FileTag = ET_File_Tag_Item_New ();
        ET_Manage_Changes_Of_File_Data (etfile, NULL, FileTag);

        fraction = (++progress_bar_index) / (double) selectcount;
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (ProgressBar),
                                       fraction);
        /* Needed to refresh status bar */
        while (gtk_events_pending ())
        {
            gtk_main_iteration ();
        }
    }

    g_list_free_full (selfilelist, (GDestroyNotify)gtk_tree_path_free);

    /* Refresh the whole list (faster than file by file) to show changes. */
    et_application_window_browser_refresh_list (self);

    /* Display the current file */
    ET_Display_File_Data_To_UI (ETCore->ETFileDisplayed);
    et_application_window_update_actions (self);

    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (ProgressBar), 0.0);
    Statusbar_Message (_("All tags have been removed"),TRUE);
}

static void
on_preferences (GSimpleAction *action,
                GVariant *variant,
                gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    if (priv->preferences_dialog)
    {
        gtk_widget_show (priv->preferences_dialog);
    }
    else
    {
        priv->preferences_dialog = GTK_WIDGET (et_preferences_dialog_new ());
        gtk_widget_show_all (priv->preferences_dialog);
    }
}

static void
on_action_toggle (GSimpleAction *action,
                  GVariant *variant,
                  gpointer user_data)
{
    GVariant *state;

    /* Toggle the current state. */
    state = g_action_get_state (G_ACTION (action));
    g_action_change_state (G_ACTION (action),
                           g_variant_new_boolean (!g_variant_get_boolean (state)));
    g_variant_unref (state);
}

static void
on_action_radio (GSimpleAction *action,
                 GVariant *variant,
                 gpointer user_data)
{
    /* Set the action state to the just-activated state. */
    g_action_change_state (G_ACTION (action), variant);
}

static void
on_scanner_change (GSimpleAction *action,
                   GVariant *variant,
                   gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;
    gboolean active;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);
    active = g_variant_get_boolean (variant);

    if (!active)
    {
        if (priv->scan_dialog)
        {
            gtk_widget_hide (priv->scan_dialog);
        }
        else
        {
            return;
        }
    }
    else
    {
        if (priv->scan_dialog)
        {
            gtk_widget_show (priv->scan_dialog);
        }
        else
        {
            priv->scan_dialog = GTK_WIDGET (et_scan_dialog_new ());
            gtk_widget_show (priv->scan_dialog);
        }
    }

    g_simple_action_set_state (action, variant);
}

static void
on_file_artist_view_change (GSimpleAction *action,
                            GVariant *variant,
                            gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;
    const gchar *state;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);
    state = g_variant_get_string (variant, NULL);

    g_return_if_fail (ETCore->ETFileDisplayedList != NULL);

    /* Save the current displayed data. */
    ET_Save_File_Data_From_UI (ETCore->ETFileDisplayed);

    if (strcmp (state, "file") == 0)
    {
        et_browser_set_display_mode (ET_BROWSER (priv->browser),
                                     ET_BROWSER_MODE_FILE);
    }
    else if (strcmp (state, "artist") == 0)
    {
        et_browser_set_display_mode (ET_BROWSER (priv->browser),
                                     ET_BROWSER_MODE_ARTIST);
    }
    else
    {
        g_assert_not_reached ();
    }

    g_simple_action_set_state (action, variant);

    et_application_window_update_actions (ET_APPLICATION_WINDOW (user_data));
}

static void
on_collapse_tree (GSimpleAction *action,
                  GVariant *variant,
                  gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_browser_collapse (ET_BROWSER (priv->browser));
}

static void
on_reload_tree (GSimpleAction *action,
                GVariant *variant,
                gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_browser_reload (ET_BROWSER (priv->browser));
}

static void
on_reload_directory (GSimpleAction *action,
                     GVariant *variant,
                     gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_browser_reload_directory (ET_BROWSER (priv->browser));
}

static void
on_set_default_path (GSimpleAction *action,
                     GVariant *variant,
                     gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_set_current_path_default (ET_BROWSER (priv->browser));
}

static void
on_rename_directory (GSimpleAction *action,
                     GVariant *variant,
                     gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_show_rename_directory_dialog (ET_BROWSER (priv->browser));
}

static void
on_browse_directory (GSimpleAction *action,
                     GVariant *variant,
                     gpointer user_data)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_show_open_directory_with_dialog (ET_BROWSER (priv->browser));
}

static void
on_show_cddb (GSimpleAction *action,
              GVariant *variant,
              gpointer user_data)
{
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);

    et_application_window_show_cddb_dialog (self);
}

static void
on_show_load_filenames (GSimpleAction *action,
                        GVariant *variant,
                        gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    if (priv->load_files_dialog)
    {
        gtk_widget_show (priv->load_files_dialog);
    }
    else
    {
        priv->load_files_dialog = GTK_WIDGET (et_load_files_dialog_new ());
        gtk_widget_show_all (priv->load_files_dialog);
    }
}

static void
on_show_playlist (GSimpleAction *action,
                  GVariant *variant,
                  gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    if (priv->playlist_dialog)
    {
        gtk_widget_show (priv->playlist_dialog);
    }
    else
    {
        priv->playlist_dialog = GTK_WIDGET (et_playlist_dialog_new ());
        gtk_widget_show_all (priv->playlist_dialog);
    }
}

static void
on_go_home (GSimpleAction *action,
            GVariant *variant,
            gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_go_home (ET_BROWSER (priv->browser));
}

static void
on_go_desktop (GSimpleAction *action,
               GVariant *variant,
               gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_go_desktop (ET_BROWSER (priv->browser));
}

static void
on_go_documents (GSimpleAction *action,
                 GVariant *variant,
                 gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_go_documents (ET_BROWSER (priv->browser));
}

static void
on_go_downloads (GSimpleAction *action,
                 GVariant *variant,
                 gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_go_downloads (ET_BROWSER (priv->browser));
}

static void
on_go_music (GSimpleAction *action,
             GVariant *variant,
             gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_go_music (ET_BROWSER (priv->browser));
}

static void
on_go_parent (GSimpleAction *action,
              GVariant *variant,
              gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_go_parent (ET_BROWSER (priv->browser));
}

static void
on_go_default (GSimpleAction *action,
               GVariant *variant,
               gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    et_browser_load_default_dir (ET_BROWSER (priv->browser));
}

static void
on_go_first (GSimpleAction *action,
             GVariant *variant,
             gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;
    GList *etfilelist;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    if (!ETCore->ETFileDisplayedList)
        return;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI (ETCore->ETFileDisplayed);

    /* Go to the first item of the list */
    etfilelist = ET_Displayed_File_List_First ();

    if (etfilelist)
    {
        /* To avoid the last line still selected. */
        et_browser_unselect_all (ET_BROWSER (priv->browser));
        et_application_window_browser_select_file_by_et_file (self,
                                                              (ET_File *)etfilelist->data,
                                                              TRUE);
        ET_Display_File_Data_To_UI ((ET_File *)etfilelist->data);
    }

    et_application_window_update_actions (self);
    et_scan_dialog_update_previews (ET_SCAN_DIALOG (et_application_window_get_scan_dialog (self)));

    if (!g_settings_get_boolean (MainSettings, "tag-preserve-focus"))
    {
        gtk_widget_grab_focus (GTK_WIDGET (TitleEntry));
    }
}

static void
on_go_previous (GSimpleAction *action,
                GVariant *variant,
                gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;
    GList *etfilelist;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    if (!ETCore->ETFileDisplayedList || !ETCore->ETFileDisplayedList->prev)
        return;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI (ETCore->ETFileDisplayed);

    /* Go to the prev item of the list */
    etfilelist = ET_Displayed_File_List_Previous ();

    if (etfilelist)
    {
        et_browser_unselect_all (ET_BROWSER (priv->browser));
        et_application_window_browser_select_file_by_et_file (self,
                                                              (ET_File *)etfilelist->data,
                                                              TRUE);
        ET_Display_File_Data_To_UI((ET_File *)etfilelist->data);
    }

    et_application_window_update_actions (self);
    et_scan_dialog_update_previews (ET_SCAN_DIALOG (et_application_window_get_scan_dialog (self)));

    if (!g_settings_get_boolean (MainSettings, "tag-preserve-focus"))
    {
        gtk_widget_grab_focus (GTK_WIDGET (TitleEntry));
    }
}

static void
on_go_next (GSimpleAction *action,
            GVariant *variant,
            gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;
    GList *etfilelist;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    if (!ETCore->ETFileDisplayedList || !ETCore->ETFileDisplayedList->next)
        return;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI (ETCore->ETFileDisplayed);

    /* Go to the next item of the list */
    etfilelist = ET_Displayed_File_List_Next ();

    if (etfilelist)
    {
        et_browser_unselect_all (ET_BROWSER (priv->browser));
        et_application_window_browser_select_file_by_et_file (self,
                                                              (ET_File *)etfilelist->data,
                                                              TRUE);
        ET_Display_File_Data_To_UI((ET_File *)etfilelist->data);
    }

    et_application_window_update_actions (self);
    et_scan_dialog_update_previews (ET_SCAN_DIALOG (et_application_window_get_scan_dialog (self)));

    if (!g_settings_get_boolean (MainSettings, "tag-preserve-focus"))
    {
        gtk_widget_grab_focus (GTK_WIDGET (TitleEntry));
    }
}

static void
on_go_last (GSimpleAction *action,
            GVariant *variant,
            gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self;
    GList *etfilelist;

    self = ET_APPLICATION_WINDOW (user_data);
    priv = et_application_window_get_instance_private (self);

    if (!ETCore->ETFileDisplayedList || !ETCore->ETFileDisplayedList->next)
        return;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI (ETCore->ETFileDisplayed);

    /* Go to the last item of the list */
    etfilelist = ET_Displayed_File_List_Last ();

    if (etfilelist)
    {
        et_browser_unselect_all (ET_BROWSER (priv->browser));
        et_application_window_browser_select_file_by_et_file (self,
                                                              (ET_File *)etfilelist->data,
                                                              TRUE);
        ET_Display_File_Data_To_UI ((ET_File *)etfilelist->data);
    }

    et_application_window_update_actions (self);
    et_scan_dialog_update_previews (ET_SCAN_DIALOG (et_application_window_get_scan_dialog (self)));

    if (!g_settings_get_boolean (MainSettings, "tag-preserve-focus"))
    {
        gtk_widget_grab_focus (GTK_WIDGET (TitleEntry));
    }
}

static void
on_show_cddb_selection (GSimpleAction *action,
                        GVariant *variant,
                        gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_application_window_show_cddb_dialog (self);
    et_cddb_dialog_search_from_selection (ET_CDDB_DIALOG (priv->cddb_dialog));
}

static void
on_clear_log (GSimpleAction *action,
              GVariant *variant,
              gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_log_area_clear (ET_LOG_AREA (priv->log_area));
}

static void
on_run_player_album (GSimpleAction *action,
                     GVariant *variant,
                     gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_browser_run_player_for_album_list (ET_BROWSER (priv->browser));
}

static void
on_run_player_artist (GSimpleAction *action,
                      GVariant *variant,
                      gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_browser_run_player_for_artist_list (ET_BROWSER (priv->browser));
}

static void
on_run_player_directory (GSimpleAction *action,
                         GVariant *variant,
                         gpointer user_data)
{
    Run_Audio_Player_Using_Directory ();
}

static void
on_stop (GSimpleAction *action,
         GVariant *variant,
         gpointer user_data)
{
    Action_Main_Stop_Button_Pressed ();
}

static const GActionEntry actions[] =
{
    /* File menu. */
    { "open-with", on_open_with },
    { "run-player", on_run_player },
    { "invert-selection", on_invert_selection },
    { "delete", on_delete },
    { "undo-file-changes", on_undo_file_changes },
    { "redo-file-changes", on_redo_file_changes },
    { "save", on_save },
    { "save-force", on_save_force },
    /* Edit menu. */
    { "find", on_find },
    { "select-all", on_select_all },
    { "unselect-all", on_unselect_all },
    { "undo-last-changes", on_undo_last_changes },
    { "redo-last-changes", on_redo_last_changes },
    { "remove-tags", on_remove_tags },
    { "preferences", on_preferences },
    /* View menu. */
    { "scanner", on_action_toggle, NULL, "false", on_scanner_change },
    /* { "scan-mode", on_action_radio, NULL, "false", on_scan_mode_change },
     * Created from GSetting. */
    /* { "sort-mode", on_action_radio, "s", "'ascending-filename'",
     * on_sort_mode_change }, Created from GSetting */
    { "file-artist-view", on_action_radio, "s", "'file'",
      on_file_artist_view_change },
    { "collapse-tree", on_collapse_tree },
    { "reload-tree", on_reload_tree },
    { "reload-directory", on_reload_directory },
    /* { "browse-show-hidden", on_action_toggle, NULL, "true",
      on_browse_show_hidden_change }, Created from GSetting. */
    /* Browser menu. */
    { "set-default-path", on_set_default_path },
    { "rename-directory", on_rename_directory },
    { "browse-directory", on_browse_directory },
    /* { "browse-subdir", on_browse_subdir }, Created from GSetting. */
    /* Miscellaneous menu. */
    { "show-cddb", on_show_cddb },
    { "show-load-filenames", on_show_load_filenames },
    { "show-playlist", on_show_playlist },
    /* Go menu. */
    { "go-home", on_go_home },
    { "go-desktop", on_go_desktop },
    { "go-documents", on_go_documents },
    { "go-downloads", on_go_downloads },
    { "go-music", on_go_music },
    { "go-parent", on_go_parent },
    { "go-default", on_go_default },
    { "go-first", on_go_first },
    { "go-previous", on_go_previous },
    { "go-next", on_go_next },
    { "go-last", on_go_last },
    /* Popup menus. */
    { "show-cddb-selection", on_show_cddb_selection },
    { "clear-log", on_clear_log },
    { "run-player-album", on_run_player_album },
    { "run-player-artist", on_run_player_artist },
    { "run-player-directory", on_run_player_directory },
    /* Toolbar. */
    { "stop", on_stop }
};

static void
et_application_window_dispose (GObject *object)
{
    EtApplicationWindow *self;
    EtApplicationWindowPrivate *priv;

    self = ET_APPLICATION_WINDOW (object);
    priv = et_application_window_get_instance_private (self);

    if (priv->cddb_dialog)
    {
        gtk_widget_destroy (priv->cddb_dialog);
        priv->cddb_dialog = NULL;
    }

    if (priv->load_files_dialog)
    {
        gtk_widget_destroy (priv->load_files_dialog);
        priv->load_files_dialog = NULL;
    }

    if (priv->playlist_dialog)
    {
        gtk_widget_destroy (priv->playlist_dialog);
        priv->playlist_dialog = NULL;
    }

    if (priv->preferences_dialog)
    {
        gtk_widget_destroy (priv->preferences_dialog);
        priv->preferences_dialog = NULL;
    }

    if (priv->scan_dialog)
    {
        gtk_widget_destroy (priv->scan_dialog);
        priv->scan_dialog = NULL;
    }

    if (priv->search_dialog)
    {
        gtk_widget_destroy (priv->search_dialog);
        priv->search_dialog = NULL;
    }

    G_OBJECT_CLASS (et_application_window_parent_class)->dispose (object);
}

static void
et_application_window_init (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;
    GAction *action;
    GtkWindow *window;
    GtkWidget *main_vbox;
    GtkWidget *hbox, *vbox;
    GtkWidget *widget;

    priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                                     ET_TYPE_APPLICATION_WINDOW,
                                                     EtApplicationWindowPrivate);

    priv->cddb_dialog = NULL;
    priv->load_files_dialog = NULL;
    priv->playlist_dialog = NULL;
    priv->preferences_dialog = NULL;
    priv->scan_dialog = NULL;
    priv->search_dialog = NULL;

    g_action_map_add_action_entries (G_ACTION_MAP (self), actions,
                                     G_N_ELEMENTS (actions), self);

    action = g_settings_create_action (MainSettings, "browse-show-hidden");
    g_action_map_add_action (G_ACTION_MAP (self), action);
    g_object_unref (action);
    action = g_settings_create_action (MainSettings, "browse-subdir");
    g_action_map_add_action (G_ACTION_MAP (self), action);
    g_object_unref (action);
    action = g_settings_create_action (MainSettings, "scan-mode");
    g_action_map_add_action (G_ACTION_MAP (self), action);
    g_object_unref (action);
    action = g_settings_create_action (MainSettings, "sort-mode");
    g_action_map_add_action (G_ACTION_MAP (self), action);
    g_object_unref (action);

    window = GTK_WINDOW (self);

    gtk_window_set_default_size (window, 1024, 768);
    gtk_window_set_icon_name (window, PACKAGE_TARNAME);
    gtk_window_set_title (window, PACKAGE_NAME);

    g_signal_connect (self, "delete-event",
                      G_CALLBACK (on_main_window_delete_event), NULL);

    /* Mainvbox for Menu bar + Tool bar + "Browser Area & FileArea & TagArea" + Log Area + "Status bar & Progress bar" */
    main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (self), main_vbox);

    /* Menu bar and tool bar. */
    {
        GtkBuilder *builder;
        GError *error = NULL;
        GtkWidget *toolbar;

        builder = gtk_builder_new ();
        gtk_builder_add_from_resource (builder,
                                       "/org/gnome/EasyTAG/toolbar.ui",
                                       &error);

        if (error != NULL)
        {
            g_error ("Unable to get toolbar from resource: %s",
                     error->message);
        }

        toolbar = GTK_WIDGET (gtk_builder_get_object (builder, "main_toolbar"));
        gtk_box_pack_start (GTK_BOX (main_vbox), toolbar, FALSE, FALSE, 0);

        g_object_unref (builder);
    }

    /* The two panes: BrowserArea on the left, FileArea+TagArea on the right */
    priv->hpaned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);

    /* Browser (Tree + File list + Entry) */
    widget = create_browser_area (self);
    gtk_paned_pack1 (GTK_PANED (priv->hpaned), widget, TRUE, TRUE);

    /* Vertical box for FileArea + TagArea */
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_paned_pack2 (GTK_PANED (priv->hpaned), vbox, FALSE, FALSE);

    /* TODO: Set a sensible default size for both widgets in the paned. */
    gtk_paned_set_position (GTK_PANED (priv->hpaned), 600);

    /* File */
    priv->file_area = create_file_area ();
    gtk_box_pack_start (GTK_BOX (vbox), priv->file_area, FALSE, FALSE, 0);

    /* Tag */
    priv->tag_area = create_tag_area (self);
    gtk_box_pack_start (GTK_BOX (vbox), priv->tag_area, FALSE, FALSE, 0);

    /* Vertical pane for Browser Area + FileArea + TagArea */
    priv->vpaned = gtk_paned_new (GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start (GTK_BOX (main_vbox), priv->vpaned, TRUE, TRUE, 0);
    gtk_paned_pack1 (GTK_PANED (priv->vpaned), priv->hpaned, TRUE,
                     FALSE);


    /* Log */
    priv->log_area = et_log_area_new ();
    gtk_paned_pack2 (GTK_PANED (priv->vpaned), priv->log_area, FALSE, TRUE);

    /* Horizontal box for Status bar + Progress bar */
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    /* Status bar */
    widget = Create_Status_Bar ();
    gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

    /* Progress bar */
    widget = Create_Progress_Bar ();
    gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

    gtk_widget_show_all (GTK_WIDGET (main_vbox));
}

static void
et_application_window_class_init (EtApplicationWindowClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = et_application_window_dispose;

    g_type_class_add_private (klass, sizeof (EtApplicationWindowPrivate));
}

/*
 * et_application_window_new:
 *
 * Create a new EtApplicationWindow instance.
 *
 * Returns: a new #EtApplicationWindow
 */
EtApplicationWindow *
et_application_window_new (GtkApplication *application)
{
    g_return_val_if_fail (GTK_IS_APPLICATION (application), NULL);

    return g_object_new (ET_TYPE_APPLICATION_WINDOW, "application",
                         application, NULL);
}

void
et_application_window_hide_log_area (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (self != NULL);

    priv = et_application_window_get_instance_private (self);

    gtk_widget_show_all (priv->log_area);
}

void
et_application_window_show_log_area (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (self != NULL);

    priv = et_application_window_get_instance_private (self);

    gtk_widget_hide (priv->log_area);
}

GtkWidget *
et_application_window_get_log_area (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (self != NULL, NULL);

    priv = et_application_window_get_instance_private (self);

    return priv->log_area;
}

GtkWidget *
et_application_window_get_load_files_dialog (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (self != NULL, NULL);

    priv = et_application_window_get_instance_private (self);

    return priv->load_files_dialog;
}

GtkWidget *
et_application_window_get_search_dialog (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (self != NULL, NULL);

    priv = et_application_window_get_instance_private (self);

    return priv->search_dialog;
}

GtkWidget *
et_application_window_get_preferences_dialog (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (self != NULL, NULL);

    priv = et_application_window_get_instance_private (self);

    return priv->preferences_dialog;
}

void
et_application_window_show_preferences_dialog_scanner (G_GNUC_UNUSED GtkAction *action,
                                                       gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    if (!priv->preferences_dialog)
    {
        priv->preferences_dialog = GTK_WIDGET (et_preferences_dialog_new ());
    }

    et_preferences_dialog_show_scanner (ET_PREFERENCES_DIALOG (priv->preferences_dialog));
}

GtkWidget *
et_application_window_get_cddb_dialog (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (self != NULL, NULL);

    priv = et_application_window_get_instance_private (self);

    return priv->cddb_dialog;
}

void
et_application_window_browser_toggle_display_mode (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;
    GVariant *variant;

    priv = et_application_window_get_instance_private (self);

    variant = g_action_group_get_action_state (G_ACTION_GROUP (self),
                                               "file-artist-view");

    if (strcmp (g_variant_get_string (variant, NULL), "file") == 0)
    {
        et_browser_set_display_mode (ET_BROWSER (priv->browser),
                                     ET_BROWSER_MODE_FILE);
    }
    else if (strcmp (g_variant_get_string (variant, NULL), "artist") == 0)
    {
        et_browser_set_display_mode (ET_BROWSER (priv->browser),
                                     ET_BROWSER_MODE_ARTIST);
    }
    else
    {
        g_assert_not_reached ();
    }
}

void
et_application_window_browser_set_sensitive (EtApplicationWindow *self,
                                             gboolean sensitive)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    g_return_if_fail (priv->browser != NULL);

    et_browser_set_sensitive (ET_BROWSER (priv->browser), sensitive);
}

void
et_application_window_browser_clear (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    g_return_if_fail (priv->browser != NULL);

    et_browser_clear (ET_BROWSER (priv->browser));
}

void
et_application_window_browser_clear_album_model (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    g_return_if_fail (priv->browser != NULL);

    et_browser_clear_album_model (ET_BROWSER (priv->browser));
}

void
et_application_window_browser_clear_artist_model (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    g_return_if_fail (priv->browser != NULL);

    et_browser_clear_artist_model (ET_BROWSER (priv->browser));
}

void
et_application_window_select_dir (EtApplicationWindow *self, const gchar *path)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    et_browser_select_dir (ET_BROWSER (priv->browser), path);
}

const gchar *
et_application_window_get_current_path (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (ET_APPLICATION_WINDOW (self), NULL);

    priv = et_application_window_get_instance_private (self);

    return et_browser_get_current_path (ET_BROWSER (priv->browser));
}

GtkWidget *
et_application_window_get_scan_dialog (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (self != NULL, NULL);

    priv = et_application_window_get_instance_private (self);

    return priv->scan_dialog;
}

/*
 * et_on_action_select_scan_mode:
 * @action: the action on which the signal was emitted
 * @current: the member of the action group which has just been activated
 * @user_data: user data set when the signal handler was connected
 *
 * Select the current scanner mode.
 */
void
et_on_action_select_scan_mode (GtkRadioAction *action, GtkRadioAction *current,
                               gpointer user_data)
{
    gint active_value;

    active_value = gtk_radio_action_get_current_value (action);

    g_settings_set_enum (MainSettings, "scan-mode", active_value);
}

void
et_on_action_select_browser_mode (G_GNUC_UNUSED GtkRadioAction *action,
                                  G_GNUC_UNUSED GtkRadioAction *current,
                                  gpointer user_data)
{
}

/*
 * Disable (FALSE) / Enable (TRUE) all user widgets in the tag area
 */
void
et_application_window_tag_area_set_sensitive (EtApplicationWindow *self,
                                              gboolean sensitive)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    g_return_if_fail (priv->tag_area != NULL);

    /* TAG Area (entries + buttons). */
    gtk_widget_set_sensitive (gtk_bin_get_child (GTK_BIN (priv->tag_area)),
                              sensitive);
}

/*
 * Disable (FALSE) / Enable (TRUE) all user widgets in the file area
 */
void
et_application_window_file_area_set_sensitive (EtApplicationWindow *self,
                                               gboolean sensitive)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    g_return_if_fail (priv->file_area != NULL);

    /* File Area. */
    gtk_widget_set_sensitive (gtk_bin_get_child (GTK_BIN (priv->file_area)),
                              sensitive);
}

static void
set_action_state (EtApplicationWindow *self,
                  const gchar *action_name,
                  gboolean enabled)
{
    GSimpleAction *action;

    action = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (self),
                                                          action_name));

    if (action == NULL)
    {
        g_error ("Unable to find action '%s' in application window",
                 action_name);
    }

    g_simple_action_set_enabled (action, enabled);
}

/* et_application_window_disable_command_actions:
 * Disable buttons when saving files (do not disable Quit button).
 */
void
et_application_window_disable_command_actions (EtApplicationWindow *self)
{
    GtkDialog *dialog;

    dialog = GTK_DIALOG (et_application_window_get_scan_dialog (self));

    /* Scanner Window */
    if (dialog)
    {
        gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_APPLY, FALSE);
    }

    /* "File" menu commands */
    set_action_state (self, "open-with", FALSE);
    set_action_state (self, "invert-selection", FALSE);
    set_action_state (self, "delete", FALSE);
    set_action_state (self, "go-first", FALSE);
    set_action_state (self, "go-previous", FALSE);
    set_action_state (self, "go-next", FALSE);
    set_action_state (self, "go-last", FALSE);
    set_action_state (self, "remove-tags", FALSE);
    set_action_state (self, "undo-file-changes", FALSE);
    set_action_state (self, "redo-file-changes", FALSE);
    set_action_state (self, "save", FALSE);
    set_action_state (self, "save-force", FALSE);
    set_action_state (self, "undo-last-changes", FALSE);
    set_action_state (self, "redo-last-changes", FALSE);

    /* FIXME: "Scanner" menu commands */
    /*set_action_state (self, "scan-mode", FALSE);*/
}


/* et_application_window_update_actions:
 * Set to sensitive/unsensitive the state of each button into
 * the commands area and menu items in function of state of the "main list".
 */
void
et_application_window_update_actions (EtApplicationWindow *self)
{
    GtkDialog *dialog;

    dialog = GTK_DIALOG (et_application_window_get_scan_dialog (self));

    if (!ETCore->ETFileDisplayedList)
    {
        /* No file found */

        /* File and Tag frames */
        et_application_window_file_area_set_sensitive (self, FALSE);
        et_application_window_tag_area_set_sensitive (self, FALSE);

        /* Tool bar buttons (the others are covered by the menu) */
        set_action_state (self, "stop", FALSE);

        /* Scanner Window */
        if (dialog)
        {
            gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_APPLY,
                                               FALSE);
        }

        /* Menu commands */
        set_action_state (self, "open-with", FALSE);
        set_action_state (self, "invert-selection", FALSE);
        set_action_state (self, "delete", FALSE);
        /* FIXME: set_action_state (self, "sort-mode", FALSE); */
        set_action_state (self, "go-previous", FALSE);
        set_action_state (self, "go-next", FALSE);
        set_action_state (self, "go-first", FALSE);
        set_action_state (self, "go-last", FALSE);
        set_action_state (self, "remove-tags", FALSE);
        set_action_state (self, "undo-file-changes", FALSE);
        set_action_state (self, "redo-file-changes", FALSE);
        set_action_state (self, "save", FALSE);
        set_action_state (self, "save-force", FALSE);
        set_action_state (self, "undo-last-changes", FALSE);
        set_action_state (self, "redo-last-changes", FALSE);
        set_action_state (self, "find", FALSE);
        set_action_state (self, "show-load-filenames", FALSE);
        set_action_state (self, "show-playlist", FALSE);
        set_action_state (self, "run-player", FALSE);
        /* FIXME set_action_state (self, "scan-mode", FALSE);*/
        set_action_state (self, "file-artist-view", FALSE);

        return;
    }else
    {
        GList *selfilelist = NULL;
        ET_File *etfile;
        gboolean has_undo = FALSE;
        gboolean has_redo = FALSE;
        //gboolean has_to_save = FALSE;
        GtkTreeSelection *selection;

        /* File and Tag frames */
        et_application_window_file_area_set_sensitive (self, TRUE);
        et_application_window_tag_area_set_sensitive (self, TRUE);

        /* Tool bar buttons */
        set_action_state (self, "stop", FALSE);

        /* Scanner Window */
        if (dialog)
        {
            gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_APPLY,
                                               TRUE);
        }

        /* Commands into menu */
        set_action_state (self, "open-with", TRUE);
        set_action_state (self, "invert-selection", TRUE);
        set_action_state (self, "delete", TRUE);
        /* FIXME set_action_state (self, "sort-mode", TRUE); */
        set_action_state (self, "remove-tags", TRUE);
        set_action_state (self, "find", TRUE);
        set_action_state (self, "show-load-filenames", TRUE);
        set_action_state (self, "show-playlist", TRUE);
        set_action_state (self, "run-player", TRUE);
        /* FIXME set_action_state (self, "scan-mode", TRUE); */
        set_action_state (self, "file-artist-view", TRUE);

        /* Check if one of the selected files has undo or redo data */
        {
            GList *l;

            selection = et_application_window_browser_get_selection (self);
            selfilelist = gtk_tree_selection_get_selected_rows(selection, NULL);

            for (l = selfilelist; l != NULL; l = g_list_next (l))
            {
                etfile = et_application_window_browser_get_et_file_from_path (self,
                                                                              l->data);
                has_undo    |= ET_File_Data_Has_Undo_Data(etfile);
                has_redo    |= ET_File_Data_Has_Redo_Data(etfile);
                //has_to_save |= ET_Check_If_File_Is_Saved(etfile);
                if ((has_undo && has_redo /*&& has_to_save*/) || !l->next) // Useless to check the other files
                    break;
            }

            g_list_free_full (selfilelist, (GDestroyNotify)gtk_tree_path_free);
        }

        /* Enable undo commands if there are undo data */
        if (has_undo)
        {
            set_action_state (self, "undo-file-changes", TRUE);
        }
        else
        {
            set_action_state (self, "undo-file-changes", FALSE);
        }

        /* Enable redo commands if there are redo data */
        if (has_redo)
        {
            set_action_state (self, "redo-file-changes", TRUE);
        }
        else
        {
            set_action_state (self, "redo-file-changes", FALSE);
        }

        /* Enable save file command if file has been changed */
        // Desactivated because problem with only one file in the list, as we can't change the selected file => can't mark file as changed
        /*if (has_to_save)
            ui_widget_set_sensitive(MENU_FILE, AM_SAVE, FALSE);
        else*/
            set_action_state (self, "save", TRUE);
        
        set_action_state (self, "save-force", TRUE);

        /* Enable undo command if there are data into main undo list (history list) */
        if (ET_History_File_List_Has_Undo_Data ())
        {
            set_action_state (self, "undo-last-changes", TRUE);
        }
        else
        {
            set_action_state (self, "undo-last-changes", FALSE);
        }

        /* Enable redo commands if there are data into main redo list (history list) */
        if (ET_History_File_List_Has_Redo_Data ())
        {
            set_action_state (self, "redo-last-changes", TRUE);
        }
        else
        {
            set_action_state (self, "redo-last-changes", FALSE);
        }

        {
            GVariant *variant;
            const gchar *state;

            variant = g_action_group_get_action_state (G_ACTION_GROUP (self),
                                                     "file-artist-view");

            state = g_variant_get_string (variant, NULL);

            if (strcmp (state, "artist") == 0)
            {
                set_action_state (self, "collapse-tree", FALSE);
                set_action_state (self, "reload-tree", FALSE);
            }
            else if (strcmp (state, "file") == 0)
            {
                set_action_state (self, "collapse-tree", TRUE);
                set_action_state (self, "reload-tree", TRUE);
            }
            else
            {
                g_assert_not_reached ();
            }

            g_variant_unref (variant);
        }
    }

    if (!ETCore->ETFileDisplayedList->prev)    /* Is it the 1st item ? */
    {
        set_action_state (self, "go-previous", FALSE);
        set_action_state (self, "go-first", FALSE);
    }
    else
    {
        set_action_state (self, "go-previous", TRUE);
        set_action_state (self, "go-first", TRUE);
    }

    if (!ETCore->ETFileDisplayedList->next)    /* Is it the last item ? */
    {
        set_action_state (self, "go-next", FALSE);
        set_action_state (self, "go-last", FALSE);
    }
    else
    {
        set_action_state (self, "go-next", TRUE);
        set_action_state (self, "go-last", TRUE);
    }
}

static void
et_application_window_hide_images_tab (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    gtk_widget_hide (priv->images_tab);
}

static void
et_application_window_show_images_tab (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    gtk_widget_show (priv->images_tab);
}

/*
 * Display controls according the kind of tag... (Hide some controls if not available for a tag type)
 */
void
et_application_window_tag_area_display_controls (EtApplicationWindow *self,
                                                 ET_File *ETFile)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));
    g_return_if_fail (ETFile != NULL && ETFile->ETFileDescription != NULL);

    priv = et_application_window_get_instance_private (self);

    g_return_if_fail (priv->title_label != NULL);

    /* Common controls for all tags. */
    gtk_widget_show (GTK_WIDGET (priv->title_label));
    gtk_widget_show(GTK_WIDGET(TitleEntry));
    gtk_widget_show (GTK_WIDGET (priv->artist_label));
    gtk_widget_show(GTK_WIDGET(ArtistEntry));
    gtk_widget_show (GTK_WIDGET (priv->album_artist_label));
    gtk_widget_show(GTK_WIDGET(AlbumArtistEntry));
    gtk_widget_show (GTK_WIDGET (priv->album_label));
    gtk_widget_show(GTK_WIDGET(AlbumEntry));
    gtk_widget_show (GTK_WIDGET (priv->year_label));
    gtk_widget_show(GTK_WIDGET(YearEntry));
    gtk_widget_show (GTK_WIDGET (priv->track_label));
    gtk_widget_show(GTK_WIDGET(TrackEntryCombo));
    gtk_widget_show(GTK_WIDGET(TrackTotalEntry));
    gtk_widget_show(GTK_WIDGET(priv->track_sequence_button));
    gtk_widget_show(GTK_WIDGET(priv->track_number_button));
    gtk_widget_show (GTK_WIDGET (priv->genre_label));
    gtk_widget_show(GTK_WIDGET(GenreCombo));
    gtk_widget_show (GTK_WIDGET (priv->comment_label));
    gtk_widget_show(GTK_WIDGET(CommentEntry));

    /* Special controls to display or not! */
    switch (ETFile->ETFileDescription->TagType)
    {
        case ID3_TAG:
            if (!g_settings_get_boolean (MainSettings, "id3v2-enabled"))
            {
                /* ID3v1 : Hide specifics ID3v2 fields if not activated! */
                gtk_widget_hide (GTK_WIDGET (priv->disc_number_label));
                gtk_widget_hide(GTK_WIDGET(DiscNumberEntry));
                gtk_widget_hide (GTK_WIDGET (priv->composer_label));
                gtk_widget_hide(GTK_WIDGET(ComposerEntry));
                gtk_widget_hide (GTK_WIDGET (priv->orig_artist_label));
                gtk_widget_hide(GTK_WIDGET(OrigArtistEntry));
                gtk_widget_hide (GTK_WIDGET (priv->copyright_label));
                gtk_widget_hide(GTK_WIDGET(CopyrightEntry));
                gtk_widget_hide (GTK_WIDGET (priv->url_label));
                gtk_widget_hide(GTK_WIDGET(URLEntry));
                gtk_widget_hide (GTK_WIDGET (priv->encoded_by_label));
                gtk_widget_hide(GTK_WIDGET(EncodedByEntry));
                et_application_window_hide_images_tab (self);
            }else
            {
                gtk_widget_show (GTK_WIDGET (priv->disc_number_label));
                gtk_widget_show(GTK_WIDGET(DiscNumberEntry));
                gtk_widget_show (GTK_WIDGET (priv->composer_label));
                gtk_widget_show(GTK_WIDGET(ComposerEntry));
                gtk_widget_show (GTK_WIDGET (priv->orig_artist_label));
                gtk_widget_show(GTK_WIDGET(OrigArtistEntry));
                gtk_widget_show (GTK_WIDGET (priv->copyright_label));
                gtk_widget_show(GTK_WIDGET(CopyrightEntry));
                gtk_widget_show (GTK_WIDGET (priv->url_label));
                gtk_widget_show(GTK_WIDGET(URLEntry));
                gtk_widget_show (GTK_WIDGET (priv->encoded_by_label));
                gtk_widget_show(GTK_WIDGET(EncodedByEntry));
                et_application_window_show_images_tab (self);
            }
            break;

#ifdef ENABLE_OGG
        case OGG_TAG:
            gtk_widget_show (GTK_WIDGET (priv->disc_number_label));
            gtk_widget_show(GTK_WIDGET(DiscNumberEntry));
            gtk_widget_show (GTK_WIDGET (priv->composer_label));
            gtk_widget_show(GTK_WIDGET(ComposerEntry));
            gtk_widget_show (GTK_WIDGET (priv->orig_artist_label));
            gtk_widget_show(GTK_WIDGET(OrigArtistEntry));
            gtk_widget_show (GTK_WIDGET (priv->copyright_label));
            gtk_widget_show(GTK_WIDGET(CopyrightEntry));
            gtk_widget_show (GTK_WIDGET (priv->url_label));
            gtk_widget_show(GTK_WIDGET(URLEntry));
            gtk_widget_show (GTK_WIDGET (priv->encoded_by_label));
            gtk_widget_show(GTK_WIDGET(EncodedByEntry));
            et_application_window_show_images_tab (self);
            break;
#endif

#ifdef ENABLE_OPUS
        case OPUS_TAG:
            gtk_widget_show (GTK_WIDGET (priv->disc_number_label));
            gtk_widget_show (GTK_WIDGET (DiscNumberEntry));
            gtk_widget_show (GTK_WIDGET (priv->composer_label));
            gtk_widget_show (GTK_WIDGET (ComposerEntry));
            gtk_widget_show (GTK_WIDGET (priv->orig_artist_label));
            gtk_widget_show (GTK_WIDGET (OrigArtistEntry));
            gtk_widget_show (GTK_WIDGET (priv->copyright_label));
            gtk_widget_show (GTK_WIDGET (CopyrightEntry));
            gtk_widget_show (GTK_WIDGET (priv->url_label));
            gtk_widget_show (GTK_WIDGET (URLEntry));
            gtk_widget_show (GTK_WIDGET (priv->encoded_by_label));
            gtk_widget_show (GTK_WIDGET (EncodedByEntry));
            et_application_window_show_images_tab (self);
            break;
#endif

#ifdef ENABLE_FLAC
        case FLAC_TAG:
            gtk_widget_show (GTK_WIDGET (priv->disc_number_label));
            gtk_widget_show(GTK_WIDGET(DiscNumberEntry));
            gtk_widget_show (GTK_WIDGET (priv->composer_label));
            gtk_widget_show(GTK_WIDGET(ComposerEntry));
            gtk_widget_show (GTK_WIDGET (priv->orig_artist_label));
            gtk_widget_show(GTK_WIDGET(OrigArtistEntry));
            gtk_widget_show (GTK_WIDGET (priv->copyright_label));
            gtk_widget_show(GTK_WIDGET(CopyrightEntry));
            gtk_widget_show (GTK_WIDGET (priv->url_label));
            gtk_widget_show(GTK_WIDGET(URLEntry));
            gtk_widget_show (GTK_WIDGET (priv->encoded_by_label));
            gtk_widget_show(GTK_WIDGET(EncodedByEntry));
            et_application_window_show_images_tab (self);
            break;
#endif

        case APE_TAG:
            gtk_widget_show (GTK_WIDGET (priv->disc_number_label));
            gtk_widget_show(GTK_WIDGET(DiscNumberEntry));
            gtk_widget_show (GTK_WIDGET (priv->composer_label));
            gtk_widget_show(GTK_WIDGET(ComposerEntry));
            gtk_widget_show (GTK_WIDGET (priv->orig_artist_label));
            gtk_widget_show(GTK_WIDGET(OrigArtistEntry));
            gtk_widget_show (GTK_WIDGET (priv->copyright_label));
            gtk_widget_show(GTK_WIDGET(CopyrightEntry));
            gtk_widget_show (GTK_WIDGET (priv->url_label));
            gtk_widget_show(GTK_WIDGET(URLEntry));
            gtk_widget_show (GTK_WIDGET (priv->encoded_by_label));
            gtk_widget_show(GTK_WIDGET(EncodedByEntry));
            et_application_window_show_images_tab (self);
            break;

#ifdef ENABLE_MP4
        case MP4_TAG:
            gtk_widget_show (GTK_WIDGET (priv->disc_number_label));
            gtk_widget_show (GTK_WIDGET (DiscNumberEntry));
            gtk_widget_show (GTK_WIDGET (priv->composer_label));
            gtk_widget_show (GTK_WIDGET (ComposerEntry));
            gtk_widget_hide (GTK_WIDGET (priv->orig_artist_label));
            gtk_widget_hide(GTK_WIDGET(OrigArtistEntry));
            gtk_widget_show (GTK_WIDGET (priv->copyright_label));
            gtk_widget_show (GTK_WIDGET (CopyrightEntry));
            gtk_widget_hide (GTK_WIDGET (priv->url_label));
            gtk_widget_hide(GTK_WIDGET(URLEntry));
            gtk_widget_show (GTK_WIDGET (priv->encoded_by_label));
            gtk_widget_show (GTK_WIDGET(EncodedByEntry));
            et_application_window_show_images_tab (self);
            break;
#endif

#ifdef ENABLE_WAVPACK
        case WAVPACK_TAG:
            gtk_widget_show (GTK_WIDGET (priv->disc_number_label));
            gtk_widget_show(GTK_WIDGET(DiscNumberEntry));
            gtk_widget_show (GTK_WIDGET (priv->composer_label));
            gtk_widget_show(GTK_WIDGET(ComposerEntry));
            gtk_widget_show (GTK_WIDGET (priv->orig_artist_label));
            gtk_widget_show(GTK_WIDGET(OrigArtistEntry));
            gtk_widget_show (GTK_WIDGET (priv->copyright_label));
            gtk_widget_show(GTK_WIDGET(CopyrightEntry));
            gtk_widget_show (GTK_WIDGET (priv->url_label));
            gtk_widget_show(GTK_WIDGET(URLEntry));
            gtk_widget_show (GTK_WIDGET (priv->encoded_by_label));
            gtk_widget_show(GTK_WIDGET(EncodedByEntry));
            et_application_window_hide_images_tab (self);
            break;
#endif /* ENABLE_WAVPACK */

        case UNKNOWN_TAG:
        default:
            gtk_widget_hide (GTK_WIDGET (priv->disc_number_label));
            gtk_widget_hide(GTK_WIDGET(DiscNumberEntry));
            gtk_widget_hide (GTK_WIDGET (priv->composer_label));
            gtk_widget_hide(GTK_WIDGET(ComposerEntry));
            gtk_widget_hide (GTK_WIDGET (priv->orig_artist_label));
            gtk_widget_hide(GTK_WIDGET(OrigArtistEntry));
            gtk_widget_hide (GTK_WIDGET (priv->copyright_label));
            gtk_widget_hide(GTK_WIDGET(CopyrightEntry));
            gtk_widget_hide (GTK_WIDGET (priv->url_label));
            gtk_widget_hide(GTK_WIDGET(URLEntry));
            gtk_widget_hide (GTK_WIDGET (priv->encoded_by_label));
            gtk_widget_hide(GTK_WIDGET(EncodedByEntry));
            et_application_window_hide_images_tab (self);
            break;
    }
}

void
et_application_window_browser_entry_set_text (EtApplicationWindow *self,
                                              const gchar *text)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    et_browser_entry_set_text (ET_BROWSER (priv->browser), text);
}

void
et_application_window_browser_label_set_text (EtApplicationWindow *self,
                                              const gchar *text)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    et_browser_label_set_text (ET_BROWSER (priv->browser), text);
}

ET_File *
et_application_window_browser_get_et_file_from_path (EtApplicationWindow *self,
                                                     GtkTreePath *path)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    return et_browser_get_et_file_from_path (ET_BROWSER (priv->browser), path);
}

ET_File *
et_application_window_browser_get_et_file_from_iter (EtApplicationWindow *self,
                                                     GtkTreeIter *iter)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    return et_browser_get_et_file_from_iter (ET_BROWSER (priv->browser), iter);
}

GtkTreeSelection *
et_application_window_browser_get_selection (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    return et_browser_get_selection (ET_BROWSER (priv->browser));
}

GtkTreeViewColumn *
et_application_window_browser_get_column_for_column_id (EtApplicationWindow *self,
                                                        gint column_id)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    return et_browser_get_column_for_column_id (ET_BROWSER (priv->browser),
                                                column_id);
}

GtkSortType
et_application_window_browser_get_sort_order_for_column_id (EtApplicationWindow *self,
                                                            gint column_id)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    return et_browser_get_sort_order_for_column_id (ET_BROWSER (priv->browser),
                                                    column_id);
}

void
et_application_window_browser_select_file_by_iter_string (EtApplicationWindow *self,
                                                          const gchar *iter_string,
                                                          gboolean select)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    et_browser_select_file_by_iter_string (ET_BROWSER (priv->browser),
                                           iter_string, select);
}

void
et_application_window_browser_select_file_by_et_file (EtApplicationWindow *self,
                                                      ET_File *file,
                                                      gboolean select)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    return et_browser_select_file_by_et_file (ET_BROWSER (priv->browser), file,
                                              select);
}

GtkTreePath *
et_application_window_browser_select_file_by_et_file2 (EtApplicationWindow *self,
                                                       ET_File *file,
                                                       gboolean select,
                                                       GtkTreePath *start_path)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    return et_browser_select_file_by_et_file2 (ET_BROWSER (priv->browser),
                                               file, select, start_path);
}

ET_File *
et_application_window_browser_select_file_by_dlm (EtApplicationWindow *self,
                                                  const gchar *string,
                                                  gboolean select)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    return et_browser_select_file_by_dlm (ET_BROWSER (priv->browser), string,
                                          select);
}

void
et_application_window_browser_unselect_all (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    priv = et_application_window_get_instance_private (self);

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI (ETCore->ETFileDisplayed);

    et_browser_unselect_all (ET_BROWSER (priv->browser));
    ETCore->ETFileDisplayed = NULL;
}

void
et_application_window_browser_refresh_list (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    et_browser_refresh_list (ET_BROWSER (priv->browser));
}

void
et_application_window_browser_refresh_file_in_list (EtApplicationWindow *self,
                                                    ET_File *file)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    et_browser_refresh_file_in_list (ET_BROWSER (priv->browser), file);
}

void
et_application_window_browser_refresh_sort (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_if_fail (ET_APPLICATION_WINDOW (self));

    priv = et_application_window_get_instance_private (self);

    et_browser_refresh_sort (ET_BROWSER (priv->browser));
}
