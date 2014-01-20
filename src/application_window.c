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

    g_return_if_fail (ETCore->ETFileDisplayedList != NULL ||
                      BrowserList != NULL);

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

    // Warning : 'selection_filelist' is not a list of 'ETFile' items!
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
    selection_filelist = gtk_tree_selection_get_selected_rows(selection, NULL);

    // Create an 'ETFile' list from 'selection_filelist'
    for (l = selection_filelist; l != NULL; l = g_list_next (l))
    {
        etfile = Browser_List_Get_ETFile_From_Path (l->data);
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
        gchar *path = NULL;
        gchar *path1 = NULL;
        gint i = 0;

        /* FIX ME!: see to fill also the Total Track (it's a good idea?) */
        etfilelistfull = g_list_first(ETCore->ETFileList);

        // Sort 'etfilelistfull' and 'etfilelist' in the same order
        etfilelist     = ET_Sort_File_List(etfilelist,SORTING_FILE_MODE);
        etfilelistfull = ET_Sort_File_List(etfilelistfull,SORTING_FILE_MODE);

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

    // Refresh the whole list (faster than file by file) to show changes
    Browser_List_Refresh_Whole_List();

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
    Update_Command_Buttons_Sensivity();
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
create_browser_area (void)
{
    GtkWidget *frame;
    GtkWidget *tree;

    frame = gtk_frame_new (_("Browser"));
    gtk_container_set_border_width (GTK_CONTAINER (frame), 2);

    tree = Create_Browser_Items (MainWindow);
    gtk_container_add (GTK_CONTAINER (frame), tree);

    /* Don't load init dir here because Tag area hasn't been yet created!.
     * It will be load at the end of the main function */
    //Browser_Tree_Select_Dir(DEFAULT_PATH_TO_MP3);

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
        GtkWidget *menu_area;
        GtkWidget *tool_area;

        Create_UI (window, &menu_area, &tool_area);
        gtk_box_pack_start (GTK_BOX (main_vbox), menu_area, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (main_vbox), tool_area, FALSE, FALSE, 0);
    }

    /* The two panes: BrowserArea on the left, FileArea+TagArea on the right */
    priv->hpaned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);

    /* Browser (Tree + File list + Entry) */
    widget = create_browser_area ();
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

    gtk_widget_show_all (GTK_WIDGET (self));
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
et_application_window_get_playlist_dialog (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (self != NULL, NULL);

    priv = et_application_window_get_instance_private (self);

    return priv->playlist_dialog;
}

void
et_application_window_show_playlist_dialog (G_GNUC_UNUSED GtkAction *action,
                                            gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

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

GtkWidget *
et_application_window_get_load_files_dialog (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (self != NULL, NULL);

    priv = et_application_window_get_instance_private (self);

    return priv->load_files_dialog;
}

void
et_application_window_show_load_files_dialog (G_GNUC_UNUSED GtkAction *action,
                                              gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

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

GtkWidget *
et_application_window_get_search_dialog (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (self != NULL, NULL);

    priv = et_application_window_get_instance_private (self);

    return priv->search_dialog;
}

void
et_application_window_show_search_dialog (G_GNUC_UNUSED GtkAction *action,
                                          gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

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

GtkWidget *
et_application_window_get_preferences_dialog (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (self != NULL, NULL);

    priv = et_application_window_get_instance_private (self);

    return priv->preferences_dialog;
}

void
et_application_window_show_preferences_dialog (G_GNUC_UNUSED GtkAction *action,
                                               gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

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
et_application_window_show_cddb_dialog (G_GNUC_UNUSED GtkAction *action,
                                        gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

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

void
et_application_window_search_cddb_for_selection (G_GNUC_UNUSED GtkAction *action,
                                                 gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_application_window_show_cddb_dialog (action, user_data);
    et_cddb_dialog_search_from_selection (ET_CDDB_DIALOG (priv->cddb_dialog));
}

GtkWidget *
et_application_window_get_scan_dialog (EtApplicationWindow *self)
{
    EtApplicationWindowPrivate *priv;

    g_return_val_if_fail (self != NULL, NULL);

    priv = et_application_window_get_instance_private (self);

    return priv->scan_dialog;
}

void
et_application_window_show_scan_dialog (GtkAction *action, gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    gboolean active;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);
    active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

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
}

/*
 * Action when Scan button is pressed
 */
void
et_application_window_scan_selected_files (G_GNUC_UNUSED GtkAction *action,
                                           gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);

    priv = et_application_window_get_instance_private (self);

    et_scan_dialog_scan_selected_files (ET_SCAN_DIALOG (priv->scan_dialog));
}

/*
 * et_on_action_select_scan_mode:
 * @action: the action on which the signal was emitted
 * @current: the member of the action group which has just been activated
 * @user_data: user data set when the signal handler was connected
 *
 * Select the current scanner mode and open the scanner window.
 */
void
et_on_action_select_scan_mode (GtkRadioAction *action, GtkRadioAction *current,
                               gpointer user_data)
{
    EtApplicationWindowPrivate *priv;
    EtApplicationWindow *self = ET_APPLICATION_WINDOW (user_data);
    gint active_value;

    priv = et_application_window_get_instance_private (self);

    active_value = gtk_radio_action_get_current_value (action);

    if (SCANNER_TYPE != active_value)
    {
        SCANNER_TYPE = active_value;
    }

    et_scan_dialog_open (ET_SCAN_DIALOG (priv->scan_dialog), SCANNER_TYPE);
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

    // Special controls to display or not!
    switch (ETFile->ETFileDescription->TagType)
    {
        case ID3_TAG:
            if (!FILE_WRITING_ID3V2_WRITE_TAG)
            {
                // ID3v1 : Hide specifics ID3v2 fields if not activated!
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

/*
 * Action when selecting all files
 */
void
et_application_window_select_all (GtkAction *action, gpointer user_data)
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
        /* Save the current displayed data */
        ET_Save_File_Data_From_UI (ETCore->ETFileDisplayed);

        Browser_List_Select_All_Files ();
        Update_Command_Buttons_Sensivity ();
    }
}

/*
 * Action when unselecting all
 */
void
et_application_window_unselect_all (GtkAction *action, gpointer user_data)
{
    GtkWidget *focused;

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
    else /* Assume that other widgets should select all in the file view. */
    {
        /* Save the current displayed data */
        ET_Save_File_Data_From_UI (ETCore->ETFileDisplayed);

        Browser_List_Unselect_All_Files ();
        ETCore->ETFileDisplayed = NULL;
    }
}

/*
 * Action when First button is selected
 */
void
et_application_window_select_first_file (GtkAction *action, gpointer user_data)
{
    GList *etfilelist;

    if (!ETCore->ETFileDisplayedList)
        return;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    /* Go to the first item of the list */
    etfilelist = ET_Displayed_File_List_First();
    if (etfilelist)
    {
        Browser_List_Unselect_All_Files(); // To avoid the last line still selected
        Browser_List_Select_File_By_Etfile((ET_File *)etfilelist->data,TRUE);
        ET_Display_File_Data_To_UI((ET_File *)etfilelist->data);
    }

    Update_Command_Buttons_Sensivity();
    et_scan_dialog_update_previews (ET_SCAN_DIALOG (et_application_window_get_scan_dialog (ET_APPLICATION_WINDOW (user_data))));

    if (SET_FOCUS_TO_FIRST_TAG_FIELD)
        gtk_widget_grab_focus(GTK_WIDGET(TitleEntry));
}


/*
 * Action when Prev button is selected
 */
void
et_application_window_select_prev_file (GtkAction *action, gpointer user_data)
{
    GList *etfilelist;

    if (!ETCore->ETFileDisplayedList || !ETCore->ETFileDisplayedList->prev)
        return;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    /* Go to the prev item of the list */
    etfilelist = ET_Displayed_File_List_Previous();
    if (etfilelist)
    {
        Browser_List_Unselect_All_Files();
        Browser_List_Select_File_By_Etfile((ET_File *)etfilelist->data,TRUE);
        ET_Display_File_Data_To_UI((ET_File *)etfilelist->data);
    }

//    if (!ETFileList->prev)
//        gdk_beep(); // Warm the user

    Update_Command_Buttons_Sensivity();
    et_scan_dialog_update_previews (ET_SCAN_DIALOG (et_application_window_get_scan_dialog (ET_APPLICATION_WINDOW (user_data))));

    if (SET_FOCUS_TO_FIRST_TAG_FIELD)
        gtk_widget_grab_focus(GTK_WIDGET(TitleEntry));
}


/*
 * Action when Next button is selected
 */
void
et_application_window_select_next_file (GtkAction *acton, gpointer user_data)
{
    GList *etfilelist;

    if (!ETCore->ETFileDisplayedList || !ETCore->ETFileDisplayedList->next)
        return;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    /* Go to the next item of the list */
    etfilelist = ET_Displayed_File_List_Next();
    if (etfilelist)
    {
        Browser_List_Unselect_All_Files();
        Browser_List_Select_File_By_Etfile((ET_File *)etfilelist->data,TRUE);
        ET_Display_File_Data_To_UI((ET_File *)etfilelist->data);
    }

//    if (!ETFileList->next)
//        gdk_beep(); // Warm the user

    Update_Command_Buttons_Sensivity();
    et_scan_dialog_update_previews (ET_SCAN_DIALOG (et_application_window_get_scan_dialog (ET_APPLICATION_WINDOW (user_data))));

    if (SET_FOCUS_TO_FIRST_TAG_FIELD)
        gtk_widget_grab_focus(GTK_WIDGET(TitleEntry));
}


/*
 * Action when Last button is selected
 */
void
et_application_window_select_last_file (GtkAction *action, gpointer user_data)
{
    GList *etfilelist;

    if (!ETCore->ETFileDisplayedList || !ETCore->ETFileDisplayedList->next)
        return;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    /* Go to the last item of the list */
    etfilelist = ET_Displayed_File_List_Last();
    if (etfilelist)
    {
        Browser_List_Unselect_All_Files();
        Browser_List_Select_File_By_Etfile((ET_File *)etfilelist->data,TRUE);
        ET_Display_File_Data_To_UI((ET_File *)etfilelist->data);
    }

    Update_Command_Buttons_Sensivity();
    et_scan_dialog_update_previews (ET_SCAN_DIALOG (et_application_window_get_scan_dialog (ET_APPLICATION_WINDOW (user_data))));

    if (SET_FOCUS_TO_FIRST_TAG_FIELD)
        gtk_widget_grab_focus(GTK_WIDGET(TitleEntry));
}
