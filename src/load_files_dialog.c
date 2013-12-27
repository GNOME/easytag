/* EasyTAG - tag editor for audio files
 * Copyright (C) 2013  David King <amigadave@amigadave.com>
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

#include "load_files_dialog.h"

#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>

#include "bar.h"
#include "browser.h"
#include "charset.h"
#include "easytag.h"
#include "gtk2_compat.h"
#include "log.h"
#include "misc.h"
#include "picture.h"
#include "scan_dialog.h"
#include "setting.h"

/* TODO: Use G_DEFINE_TYPE_WITH_PRIVATE. */
G_DEFINE_TYPE (EtLoadFilesDialog, et_load_files_dialog, GTK_TYPE_DIALOG)

#define et_load_files_dialog_get_instance_private(dialog) (dialog->priv)

static const guint BOX_SPACING = 6;

enum
{
    LOAD_FILE_CONTENT_TEXT,
    LOAD_FILE_CONTENT_COUNT
};

enum
{
    LOAD_FILE_NAME_TEXT,
    LOAD_FILE_NAME_POINTER,
    LOAD_FILE_NAME_COUNT
};

struct _EtLoadFilesDialogPrivate
{
    GtkWidget *file_to_load_combo;
    GtkListStore *file_to_load_model;
    GtkWidget *load_file_content_view;
    GtkListStore *load_file_content_model;
    GtkWidget *load_file_name_view;
    GtkListStore *load_file_name_model;
    GtkWidget *load_file_run_scanner;

    GtkWidget *selected_line_entry;
};


/*
 * Set the new filename of each file.
 * Associate lines from priv->load_file_content_view with priv->load_file_name_view
 */
static void
Load_Filename_Set_Filenames (EtLoadFilesDialog *self)
{
    EtLoadFilesDialogPrivate *priv;
    gint row;
    ET_File *ETFile = NULL;
    File_Name *FileName;
    gchar *list_text = NULL;
    gint rowcount;
    gboolean found;

    GtkTreePath *currentPath = NULL;
    GtkTreeIter iter_name;
    GtkTreeIter iter_content;

    priv = et_load_files_dialog_get_instance_private (self);

    if ( !ETCore->ETFileList || !priv->load_file_content_view || !priv->load_file_name_view)
        return;

    /* Save current file */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    rowcount = MIN(gtk_tree_model_iter_n_children(GTK_TREE_MODEL(priv->load_file_name_model), NULL),
                   gtk_tree_model_iter_n_children(GTK_TREE_MODEL(priv->load_file_content_model), NULL));

    for (row=0; row < rowcount; row++)
    {
        if (row == 0)
            currentPath = gtk_tree_path_new_first();
        else
            gtk_tree_path_next(currentPath);

        found = gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->load_file_name_model), &iter_name, currentPath);
        if (found)
            gtk_tree_model_get(GTK_TREE_MODEL(priv->load_file_name_model), &iter_name, 
                               LOAD_FILE_NAME_POINTER, &ETFile, -1);

        found = gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->load_file_content_model), &iter_content, currentPath);
        if (found)
            gtk_tree_model_get(GTK_TREE_MODEL(priv->load_file_content_model), &iter_content, 
                               LOAD_FILE_CONTENT_TEXT, &list_text, -1);

        if (ETFile && list_text && (g_utf8_strlen (list_text, -1) > 0))
        {
            gchar *list_text_tmp;
            gchar *filename_new_utf8;

            list_text_tmp = g_strdup(list_text);
            ET_File_Name_Convert_Character(list_text_tmp); // Replace invalid characters

            /* Build the filename with the path */
            filename_new_utf8 = ET_File_Name_Generate(ETFile,list_text_tmp);
            g_free(list_text_tmp);

            /* Set the new filename */
            // Create a new 'File_Name' item
            FileName = ET_File_Name_Item_New();
            // Save changes of the 'File_Name' item
            ET_Set_Filename_File_Name_Item(FileName,filename_new_utf8,NULL);
            ET_Manage_Changes_Of_File_Data(ETFile,FileName,NULL);

            g_free(filename_new_utf8);

            // Then run current scanner if asked...
            if (ScannerWindow && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->load_file_run_scanner)) )
                Scan_Select_Mode_And_Run_Scanner(ETFile);
        }
        g_free(list_text);
    }

    gtk_tree_path_free(currentPath);

    Browser_List_Refresh_Whole_List();
    ET_Display_File_Data_To_UI(ETCore->ETFileDisplayed);
}

/*
 * on_response:
 * @dialog: the dialog which emitted the response signal
 * @response_id: the response ID
 * @user_data: user data set when the signal was connected
 *
 * Signal handler for the load filenames from text file dialog.
 */
static void
on_response (GtkDialog *dialog, gint response_id, gpointer user_data)
{
    switch (response_id)
    {
        case GTK_RESPONSE_APPLY:
            Load_Filename_Set_Filenames (ET_LOAD_FILES_DIALOG (dialog));
            break;
        case GTK_RESPONSE_CANCEL:
            et_load_files_dialog_apply_changes (ET_LOAD_FILES_DIALOG (dialog));
            gtk_widget_hide (GTK_WIDGET (dialog));
            break;
        case GTK_RESPONSE_DELETE_EVENT:
            et_load_files_dialog_apply_changes (ET_LOAD_FILES_DIALOG (dialog));
            break;
        default:
            g_assert_not_reached ();
    }
}

/*
 * To enable/disable sensitivity of the button 'Load'
 */
static void
set_load_button_sensitivity (GtkWidget *button, GtkWidget *entry)
{
    gchar *path;
    GFile *file;
    GFileInfo *info;
    GError *error = NULL;

    if (!entry || !button)
        return;

    path = filename_from_display(gtk_entry_get_text(GTK_ENTRY(entry)));

    if (!path)
    {
        gtk_widget_set_sensitive(GTK_WIDGET(button),FALSE);
        return;
    }

    file = g_file_new_for_path (path);
    info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_TYPE,
                              G_FILE_QUERY_INFO_NONE, NULL, &error);

    if (info && G_FILE_TYPE_REGULAR == g_file_info_get_file_type (info))
        gtk_widget_set_sensitive(GTK_WIDGET(button),TRUE);
    else
    {
        gtk_widget_set_sensitive(GTK_WIDGET(button),FALSE);
        if (!info)
        {
            Log_Print (LOG_ERROR, _("Cannot retrieve file info (%s)"),
                       error->message);
            g_error_free (error);
            g_object_unref (file);
            g_free (path);
            return;
        }
    }

    g_object_unref (info);
    g_object_unref (file);
    g_free(path);
}

/*
 * Load content of the file into the priv->load_file_content_view list
 */
static void
Load_File_Content (G_GNUC_UNUSED GtkButton *button, gpointer user_data)
{
    EtLoadFilesDialogPrivate *priv;
    GtkWidget *entry;
    GFile *file;
    GFileInputStream *istream;
    GDataInputStream *data;
    GError *error = NULL;
    gsize size_read;
    gchar *filename;
    const gchar *filename_utf8;
    gchar *line;
    gchar *valid;

    priv = et_load_files_dialog_get_instance_private (ET_LOAD_FILES_DIALOG (user_data));
    entry = gtk_bin_get_child (GTK_BIN (priv->file_to_load_combo));

    // The file to read
    filename_utf8 = gtk_entry_get_text(GTK_ENTRY(entry)); // Don't free me!
    Add_String_To_Combo_List(priv->file_to_load_model, filename_utf8);
    filename = filename_from_display(filename_utf8);

    file = g_file_new_for_path (filename);
    istream = g_file_read (file, NULL, &error);
    g_object_unref (file);

    if (!istream)
    {
        Log_Print (LOG_ERROR, _("Can't open file '%s' (%s)"), filename_utf8,
                   error->message);
        g_error_free (error);
        g_object_unref (file);
        g_free(filename);
        return;
    }

    data = g_data_input_stream_new (G_INPUT_STREAM (istream));
    /* TODO: Find a safer alternative to _ANY. */
    g_data_input_stream_set_newline_type (data,
                                          G_DATA_STREAM_NEWLINE_TYPE_ANY);
    gtk_list_store_clear (priv->load_file_content_model);

    while ((line = g_data_input_stream_read_line (data, &size_read, NULL,
                                                  &error)))
    {
        /* FIXME: This should use the GLib filename encoding, not UTF-8. */
        valid = Try_To_Validate_Utf8_String (line);
        g_free (line);

        gtk_list_store_insert_with_values (priv->load_file_content_model, NULL,
                                           G_MAXINT, LOAD_FILE_CONTENT_TEXT,
                                           valid, -1);
        g_free (valid);
    }

    if (error)
    {
        Log_Print (LOG_ERROR, _("Error reading file (%s)"), error->message);
        g_error_free (error);
    }

    g_object_unref (data);
    g_object_unref (istream);
    g_free (filename);
}

/*
 * Delete the selected line in the treeview passed as parameter
 */
static void
Load_Filename_List_Delete_Line (GtkWidget *treeview)
{
    GtkTreeSelection *selection;
    GtkTreeIter selectedIter, itercopy;
    GtkTreeModel *model;
    gboolean rowafter;

    g_return_if_fail (treeview != NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    if (gtk_tree_selection_get_selected(selection, &model, &selectedIter) != TRUE)
    {
        return;
    }

    // If there is a line following the one about to be removed, select it for convenience
    itercopy = selectedIter;
    rowafter = gtk_tree_model_iter_next(model, &itercopy);

    // Remove the line to be deleted
    gtk_list_store_remove(GTK_LIST_STORE(model), &selectedIter);

    if (rowafter)
        gtk_tree_selection_select_iter(selection, &itercopy);
}

/*
 * Insert a blank line before the selected line in the treeview passed as parameter
 */
static void
Load_Filename_List_Insert_Blank_Line (GtkWidget *treeview)
{
    GtkTreeSelection *selection;
    GtkTreeIter selectedIter;
    GtkTreeIter *temp;
    GtkTreeModel *model;

    g_return_if_fail (treeview != NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    if (gtk_tree_selection_get_selected(selection, &model, &selectedIter) != TRUE)
        return;

    temp = &selectedIter; /* Not used here, but it must be non-NULL to keep GTK+ happy! */
    gtk_list_store_insert_before(GTK_LIST_STORE(model), temp, &selectedIter);
}

static gboolean
Load_Filename_List_Key_Press (GtkWidget *treeview, GdkEvent *event)
{
    if (event && event->type == GDK_KEY_PRESS)
    {
        GdkEventKey *kevent = (GdkEventKey *)event;

        switch(kevent->keyval)
        {
            case GDK_KEY_Delete:
                Load_Filename_List_Delete_Line(treeview);
                return GDK_EVENT_STOP;
                break;
            case GDK_KEY_I:
            case GDK_KEY_i:
                Load_Filename_List_Insert_Blank_Line(treeview);
                return GDK_EVENT_STOP;
                break;
        }
    }

    return GDK_EVENT_PROPAGATE;
}

/*
 * Delete all blank lines in the treeview passed as parameter
 */
static void
Load_Filename_List_Delete_All_Blank_Lines (GtkWidget *treeview)
{
    gchar *text = NULL;
    GtkTreeIter iter;
    GtkTreeModel *model;

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

    if (!gtk_tree_model_get_iter_first(model, &iter))
        return;

    while (TRUE)
    {
        gtk_tree_model_get(model, &iter, LOAD_FILE_NAME_TEXT, &text, -1);

        /* Check for blank entry */
        if (!text || g_utf8_strlen(text, -1) == 0)
        {
            g_free(text);

            if (!gtk_list_store_remove(GTK_LIST_STORE(model), &iter))
                break;
            else
                continue;
        }
        g_free(text);

        if (!gtk_tree_model_iter_next(model, &iter))
            break;
    }
}

/*
 * Move up the selected line in the treeview passed as parameter
 */
static void
Load_Filename_List_Move_Up (GtkWidget *treeview)
{
    GtkTreeSelection *selection;
    GList *selectedRows;
    GList *l;
    GtkTreeIter currentFile;
    GtkTreeIter nextFile;
    GtkTreePath *currentPath;
    GtkTreeModel *treemodel;

    g_return_if_fail (treeview != NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);

    if (!selectedRows)
    {
        return;
    }

    for (l = selectedRows; l != NULL; l = g_list_next (l))
    {
        currentPath = (GtkTreePath *)l->data;

        if (gtk_tree_model_get_iter (treemodel, &currentFile, currentPath))
        {
            // Find the entry above the node...
            if (gtk_tree_path_prev(currentPath))
            {
                // ...and if it exists, swap the two rows by iter
                gtk_tree_model_get_iter(treemodel, &nextFile, currentPath);
                gtk_list_store_swap(GTK_LIST_STORE(treemodel), &currentFile, &nextFile);
            }
        }
    }

    g_list_free_full (selectedRows, (GDestroyNotify)gtk_tree_path_free);
}

/*
 * Move down the selected line in the treeview passed as parameter
 */
static void
Load_Filename_List_Move_Down (GtkWidget *treeview)
{
    GtkTreeSelection *selection;
    GList *selectedRows;
    GList *l;
    GtkTreeIter currentFile;
    GtkTreeIter nextFile;
    GtkTreePath *currentPath;
    GtkTreeModel *treemodel;

    g_return_if_fail (treeview != NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);

    if (!selectedRows)
    {
        return;
    }

    for (l = selectedRows; l != NULL; l = g_list_next (l))
    {
        currentPath = (GtkTreePath *)l->data;

        if (gtk_tree_model_get_iter (treemodel, &currentFile, currentPath))
        {
            // Find the entry below the node and swap the two nodes by iter
            gtk_tree_path_next(currentPath);
            if (gtk_tree_model_get_iter(treemodel, &nextFile, currentPath))
                gtk_list_store_swap(GTK_LIST_STORE(treemodel), &currentFile, &nextFile);
        }
    }

    g_list_free_full (selectedRows, (GDestroyNotify)gtk_tree_path_free);
}

/*
 * Load the names of the current list of files
 */
static void
Load_File_List (EtLoadFilesDialog *self)
{
    EtLoadFilesDialogPrivate *priv;
    GList *l;
    ET_File *etfile;
    gchar *filename_utf8;
    gchar *pos;

    priv = et_load_files_dialog_get_instance_private (self);

    gtk_list_store_clear(priv->load_file_name_model);

    for (l = g_list_first (ETCore->ETFileList); l != NULL; l = g_list_next (l))
    {
        etfile = (ET_File *)l->data;
        filename_utf8 = g_path_get_basename(((File_Name *)etfile->FileNameNew->data)->value_utf8);
        // Remove the extension ('filename' must be allocated to don't affect the initial value)
        if ((pos=strrchr(filename_utf8,'.'))!=NULL)
            *pos = 0;
        gtk_list_store_insert_with_values (priv->load_file_name_model, NULL,
                                           G_MAXINT, LOAD_FILE_NAME_TEXT,
                                           filename_utf8,
                                           LOAD_FILE_NAME_POINTER, l->data,
                                           -1);
        g_free(filename_utf8);
    }
}

static void
on_load_file_name_view_reload_clicked (G_GNUC_UNUSED GtkButton *button,
                                       gpointer user_data)
{
    Load_File_List (ET_LOAD_FILES_DIALOG (user_data));
}

static void
on_load_file_content_view_reload_clicked (G_GNUC_UNUSED GtkButton *button,
                                          gpointer user_data)
{
    Load_File_Content (NULL, ET_LOAD_FILES_DIALOG (user_data));
}

/*
 * To select the corresponding row in the other list
 */
static void
Load_Filename_Select_Row_In_Other_List (GtkWidget* treeview_target, gpointer origselection)
{
    GtkAdjustment *ct_adj, *ce_adj;
    GtkTreeSelection *selection_orig;
    GtkTreeSelection *selection_target;
    GtkTreeView *treeview_orig;
    GtkTreeModel *treemodel_orig;
    GtkTreeModel *treemodel_target;
    GtkTreeIter iter_orig;
    GtkTreeIter iter_target;
    GtkTreePath *path_orig;
    gint *indices_orig;
    gchar *stringiter;

    if (!treeview_target || !origselection)
        return;

    selection_orig = GTK_TREE_SELECTION(origselection);
    selection_target = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview_target));
    treemodel_target = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview_target));

    if (!gtk_tree_selection_get_selected(selection_orig, &treemodel_orig, &iter_orig))
        return; /* Might be called with no selection */

    treeview_orig = gtk_tree_selection_get_tree_view(selection_orig);
    path_orig = gtk_tree_model_get_path(treemodel_orig, &iter_orig);
    gtk_tree_selection_unselect_all(selection_target);

    /* Synchronize the two lists. */
    ce_adj = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (treeview_orig));
    ct_adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(treeview_target));

    if (gtk_adjustment_get_upper(GTK_ADJUSTMENT(ct_adj)) >= gtk_adjustment_get_page_size(GTK_ADJUSTMENT(ct_adj))
    &&  gtk_adjustment_get_upper(GTK_ADJUSTMENT(ce_adj)) >= gtk_adjustment_get_page_size(GTK_ADJUSTMENT(ce_adj)))
    {
        // Rules are displayed in the both clist
        if (gtk_adjustment_get_value(GTK_ADJUSTMENT(ce_adj)) <= gtk_adjustment_get_upper(GTK_ADJUSTMENT(ct_adj)) - gtk_adjustment_get_page_size(GTK_ADJUSTMENT(ct_adj)))
        {
            gtk_adjustment_set_value(GTK_ADJUSTMENT(ct_adj),gtk_adjustment_get_value(GTK_ADJUSTMENT(ce_adj)));
        } else
        {
            gtk_adjustment_set_value(GTK_ADJUSTMENT(ct_adj),gtk_adjustment_get_upper(GTK_ADJUSTMENT(ct_adj)) - gtk_adjustment_get_page_size(GTK_ADJUSTMENT(ct_adj)));
            indices_orig = gtk_tree_path_get_indices(path_orig);

            if (indices_orig[0] <= (gtk_tree_model_iter_n_children(treemodel_target, NULL) - 1))
                gtk_adjustment_set_value(GTK_ADJUSTMENT(ce_adj),gtk_adjustment_get_value(GTK_ADJUSTMENT(ct_adj)));

        }
    }else if (gtk_adjustment_get_upper(GTK_ADJUSTMENT(ct_adj)) < gtk_adjustment_get_page_size(GTK_ADJUSTMENT(ct_adj))) // Target Clist rule not visible
    {
        indices_orig = gtk_tree_path_get_indices(path_orig);

        if (indices_orig[0] <= (gtk_tree_model_iter_n_children(treemodel_target, NULL) - 1))
            gtk_adjustment_set_value(GTK_ADJUSTMENT(ce_adj),gtk_adjustment_get_value(GTK_ADJUSTMENT(ct_adj)));
    }

    // Must block the select signal of the target to avoid looping
    g_signal_handlers_block_by_func(G_OBJECT(selection_target), G_CALLBACK(Load_Filename_Select_Row_In_Other_List), G_OBJECT(treeview_orig));

    stringiter = gtk_tree_model_get_string_from_iter(treemodel_orig, &iter_orig);
    if (gtk_tree_model_get_iter_from_string(treemodel_target, &iter_target, stringiter))
    {
        gtk_tree_selection_select_iter(selection_target, &iter_target);
    }

    g_free(stringiter);
    g_signal_handlers_unblock_by_func(G_OBJECT(selection_target), G_CALLBACK(Load_Filename_Select_Row_In_Other_List), G_OBJECT(treeview_orig));
}

/*
 * Create and attach a popup menu on the two clist of the LoadFileWindow
 */
static gboolean
Load_Filename_Popup_Menu_Handler (GtkWidget *treeview, GdkEventButton *event,
                                  GtkMenu *menu)
{
    if (event && (event->type==GDK_BUTTON_PRESS) && (event->button==3))
    {
        gtk_menu_popup(menu,NULL,NULL,NULL,NULL,event->button,event->time);
        return TRUE;
    }
    return FALSE;
}

static void
create_load_file_content_view_popup (EtLoadFilesDialog *self, GtkWidget *list)
{
    GtkWidget *BrowserPopupMenu;
    GtkWidget *Image;
    GtkWidget *MenuItem;

    BrowserPopupMenu = gtk_menu_new();
    gtk_menu_attach_to_widget (GTK_MENU (BrowserPopupMenu), list, NULL);
    g_signal_connect (G_OBJECT (list), "button-press-event",
                      G_CALLBACK (Load_Filename_Popup_Menu_Handler),
                      BrowserPopupMenu);
    
    MenuItem = gtk_image_menu_item_new_with_label(_("Insert a blank line"));
    Image = gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu), MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate", G_CALLBACK(Load_Filename_List_Insert_Blank_Line), G_OBJECT(list));

    MenuItem = gtk_image_menu_item_new_with_label(_("Delete this line"));
    Image = gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate", G_CALLBACK(Load_Filename_List_Delete_Line), G_OBJECT(list));

    MenuItem = gtk_image_menu_item_new_with_label(_("Delete all blank lines"));
    Image = gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate", G_CALLBACK(Load_Filename_List_Delete_All_Blank_Lines),G_OBJECT(list));

    MenuItem = gtk_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);

    MenuItem = gtk_image_menu_item_new_with_label(_("Move up this line"));
    Image = gtk_image_new_from_stock(GTK_STOCK_GO_UP,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate", G_CALLBACK(Load_Filename_List_Move_Up),G_OBJECT(list));

    MenuItem = gtk_image_menu_item_new_with_label(_("Move down this line"));
    Image = gtk_image_new_from_stock(GTK_STOCK_GO_DOWN,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate", G_CALLBACK(Load_Filename_List_Move_Down),G_OBJECT(list));

    MenuItem = gtk_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);

    MenuItem = gtk_image_menu_item_new_with_label(_("Reload"));
    Image = gtk_image_new_from_stock(GTK_STOCK_REFRESH,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);
    g_signal_connect (MenuItem, "activate",
                      G_CALLBACK (on_load_file_content_view_reload_clicked),
                      self);

    gtk_widget_show_all(BrowserPopupMenu);
}

static void
create_load_file_name_view_popup (EtLoadFilesDialog *self, GtkWidget *list)
{
    GtkWidget *BrowserPopupMenu;
    GtkWidget *Image;
    GtkWidget *MenuItem;

    BrowserPopupMenu = gtk_menu_new();
    gtk_menu_attach_to_widget (GTK_MENU (BrowserPopupMenu), list, NULL);
    g_signal_connect (G_OBJECT (list), "button-press-event",
                      G_CALLBACK (Load_Filename_Popup_Menu_Handler),
                      BrowserPopupMenu);
    
    MenuItem = gtk_image_menu_item_new_with_label(_("Insert a blank line"));
    Image = gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu), MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate", G_CALLBACK(Load_Filename_List_Insert_Blank_Line), G_OBJECT(list));

    MenuItem = gtk_image_menu_item_new_with_label(_("Delete this line"));
    Image = gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate", G_CALLBACK(Load_Filename_List_Delete_Line), G_OBJECT(list));

    MenuItem = gtk_image_menu_item_new_with_label(_("Delete all blank lines"));
    Image = gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate", G_CALLBACK(Load_Filename_List_Delete_All_Blank_Lines),G_OBJECT(list));

    MenuItem = gtk_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);

    MenuItem = gtk_image_menu_item_new_with_label(_("Move up this line"));
    Image = gtk_image_new_from_stock(GTK_STOCK_GO_UP,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate", G_CALLBACK(Load_Filename_List_Move_Up),G_OBJECT(list));

    MenuItem = gtk_image_menu_item_new_with_label(_("Move down this line"));
    Image = gtk_image_new_from_stock(GTK_STOCK_GO_DOWN,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);
    g_signal_connect_swapped(G_OBJECT(MenuItem),"activate", G_CALLBACK(Load_Filename_List_Move_Down),G_OBJECT(list));

    MenuItem = gtk_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);

    MenuItem = gtk_image_menu_item_new_with_label(_("Reload"));
    Image = gtk_image_new_from_stock(GTK_STOCK_REFRESH,GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(MenuItem),Image);
    gtk_menu_shell_append(GTK_MENU_SHELL(BrowserPopupMenu),MenuItem);
    g_signal_connect (MenuItem, "activate",
                      G_CALLBACK (on_load_file_name_view_reload_clicked),
                      self);

    gtk_widget_show_all(BrowserPopupMenu);
}

/*
 * Update the text of the selected line into the list, with the text entered into the entry
 */
static void
Load_Filename_Update_Text_Line(GtkWidget *entry, GtkWidget *list)
{
    GtkTreeIter SelectedRow;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    gboolean hasSelectedRows;

    g_return_if_fail (entry != NULL || list != NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
    hasSelectedRows = gtk_tree_selection_get_selected(selection, &model, &SelectedRow);
    if (hasSelectedRows)
    {
        const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
        gtk_list_store_set(GTK_LIST_STORE(model), &SelectedRow, LOAD_FILE_CONTENT_TEXT, text, -1);
    }
}

/*
 * Set the text of the selected line of the list into the entry
 */
static void
Load_Filename_Edit_Text_Line (GtkTreeSelection *selection, gpointer user_data)
{
    EtLoadFilesDialogPrivate *priv;
    gchar *text;
    GtkTreeIter selectedIter;
    GtkEntry *entry;
    gulong handler;

    priv = et_load_files_dialog_get_instance_private (ET_LOAD_FILES_DIALOG (user_data));

    entry = GTK_ENTRY (priv->selected_line_entry);

    if (gtk_tree_selection_get_selected(selection, NULL, &selectedIter) != TRUE)
        return;

    gtk_tree_model_get(GTK_TREE_MODEL(priv->load_file_content_model), &selectedIter, LOAD_FILE_NAME_TEXT, &text, -1);

    handler = g_signal_handler_find(entry, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, Load_Filename_Update_Text_Line, NULL);
    g_signal_handler_block(entry, handler);
    if (text)
    {
        gtk_entry_set_text(entry, text);
        g_free(text);
    } else
        gtk_entry_set_text(entry, "");

    g_signal_handler_unblock(entry, handler);
}

/*
 * The window to load the filenames from a txt.
 */
static void
create_load_files_dialog (EtLoadFilesDialog *self)
{
    EtLoadFilesDialogPrivate *priv;
    GtkWidget *content_area, *hbox;
    GtkWidget *Label;
    GtkWidget *Button;
    GtkWidget *Icon;
    GtkWidget *Entry;
    GtkWidget *ButtonLoad;
    GtkWidget *Separator;
    GtkWidget *ScrollWindow;
    GtkWidget *loadedvbox;
    GtkWidget *filelistvbox;
    GtkWidget *vboxpaned;
    gchar *path;
    GtkCellRenderer* renderer;
    GtkTreeViewColumn* column;

    priv = et_load_files_dialog_get_instance_private (self);

    gtk_window_set_title (GTK_WINDOW (self),
                          _("Load Filenames From a Text File"));
    gtk_window_set_transient_for (GTK_WINDOW (self), GTK_WINDOW (MainWindow));
    gtk_window_set_destroy_with_parent (GTK_WINDOW (self), TRUE);
    gtk_dialog_add_buttons (GTK_DIALOG (self), GTK_STOCK_CLOSE,
                            GTK_RESPONSE_CANCEL, GTK_STOCK_APPLY,
                            GTK_RESPONSE_APPLY, NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_APPLY);
    g_signal_connect (self, "response", G_CALLBACK (on_response), NULL);
    g_signal_connect (self, "delete-event",
                      G_CALLBACK (gtk_widget_hide_on_delete), NULL);

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (self));
    gtk_container_set_border_width (GTK_CONTAINER (self), BOX_SPACING);
    gtk_box_set_spacing (GTK_BOX (content_area), BOX_SPACING);

    // Hbox for file entry and browser/load buttons
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_SPACING);
    gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, TRUE, 0);

    // File to load
    priv->file_to_load_model = gtk_list_store_new (MISC_COMBO_COUNT,
                                                   G_TYPE_STRING);

    Label = gtk_label_new(_("File:"));
    gtk_misc_set_alignment(GTK_MISC(Label),1.0,0.5);
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);
    priv->file_to_load_combo = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(priv->file_to_load_model));
    g_object_unref (priv->file_to_load_model);
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(priv->file_to_load_combo),MISC_COMBO_TEXT);
    gtk_widget_set_size_request(GTK_WIDGET(priv->file_to_load_combo), 200, -1);
    gtk_box_pack_start(GTK_BOX(hbox),priv->file_to_load_combo,TRUE,TRUE,0);
    // History List
    Load_File_To_Load_List(priv->file_to_load_model, MISC_COMBO_TEXT);
    // Initial value
    if ((path=Browser_Get_Current_Path())!=NULL)
        gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(priv->file_to_load_combo))),path);
    // the 'changed' signal is attached below to enable/disable the button to load
    // Button 'browse'
    Button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    g_signal_connect_swapped(G_OBJECT(Button),"clicked", G_CALLBACK(File_Selection_Window_For_File), G_OBJECT(gtk_bin_get_child(GTK_BIN(priv->file_to_load_combo))));
    // Button 'load'
    // the signal attached to this button, to load the file, is placed after the priv->load_file_content_view definition
    ButtonLoad = Create_Button_With_Icon_And_Label(GTK_STOCK_REVERT_TO_SAVED,_(" Load "));
    //ButtonLoad = gtk_button_new_with_label(_(" Load "));
    gtk_box_pack_start(GTK_BOX(hbox),ButtonLoad,FALSE,FALSE,0);
    g_signal_connect_swapped(G_OBJECT(gtk_bin_get_child(GTK_BIN(priv->file_to_load_combo))),"changed", G_CALLBACK(set_load_button_sensitivity), G_OBJECT(ButtonLoad));

    // Vbox for loaded files
    loadedvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);

    // Content of the loaded file
    ScrollWindow = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ScrollWindow),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(GTK_WIDGET(ScrollWindow), 250, 200);
    gtk_box_pack_start(GTK_BOX(loadedvbox), ScrollWindow, TRUE, TRUE, 0);
    priv->load_file_content_model = gtk_list_store_new(LOAD_FILE_CONTENT_COUNT, G_TYPE_STRING);
    priv->load_file_content_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(priv->load_file_content_model));
    g_object_unref (priv->load_file_content_model);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes (_("Content of Text File"),
                                                      renderer, "text", LOAD_FILE_CONTENT_TEXT, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(priv->load_file_content_view), column);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(priv->load_file_content_view), TRUE);
    //gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->load_file_content_view)),GTK_SELECTION_MULTIPLE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(priv->load_file_content_view),TRUE);
    gtk_container_add(GTK_CONTAINER(ScrollWindow),priv->load_file_content_view);

    // Signal to automatically load the file
    g_signal_connect (ButtonLoad, "clicked", G_CALLBACK(Load_File_Content),
                      self);
    g_signal_connect(G_OBJECT(priv->load_file_content_view),"key-press-event", G_CALLBACK(Load_Filename_List_Key_Press),NULL);

    // Commands (like the popup menu)
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_SPACING);
    gtk_box_pack_start(GTK_BOX(loadedvbox),hbox,FALSE,FALSE,0);

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Insert a blank line before the selected line"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Insert_Blank_Line), G_OBJECT(priv->load_file_content_view));

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Delete the selected line"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Delete_Line), G_OBJECT(priv->load_file_content_view));
    
    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Delete all blank lines"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Delete_All_Blank_Lines), G_OBJECT(priv->load_file_content_view));
    
    Label = gtk_label_new("   ");
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Move up the selected line"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Move_Up), G_OBJECT(priv->load_file_content_view));

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Move down the selected line"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Move_Down), G_OBJECT(priv->load_file_content_view));

    Label = gtk_label_new("   ");
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_REFRESH,GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Reload"));
    g_signal_connect (Button, "clicked",
                      G_CALLBACK (on_load_file_content_view_reload_clicked),
                      self);
    
    gtk_widget_show_all(loadedvbox);

    
    //
    // Vbox for file list files
    //
    filelistvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BOX_SPACING);

    // List of current filenames
    ScrollWindow = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ScrollWindow),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(GTK_WIDGET(ScrollWindow), 250, 200);
    gtk_box_pack_start(GTK_BOX(filelistvbox), ScrollWindow, TRUE, TRUE, 0);
    priv->load_file_name_model = gtk_list_store_new(LOAD_FILE_NAME_COUNT, G_TYPE_STRING,G_TYPE_POINTER);
    priv->load_file_name_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(priv->load_file_name_model));
    g_object_unref (priv->load_file_name_model);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("List of Files"),
                                                      renderer, "text", LOAD_FILE_NAME_TEXT, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(priv->load_file_name_view), column);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(priv->load_file_name_view), TRUE);
    //gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->load_file_name_view)),GTK_SELECTION_MULTIPLE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(priv->load_file_name_view),TRUE);
    g_signal_connect(G_OBJECT(priv->load_file_name_view),"key-press-event", G_CALLBACK(Load_Filename_List_Key_Press),NULL);
    gtk_container_add(GTK_CONTAINER(ScrollWindow),priv->load_file_name_view);

    // Signals to 'select' the same row into the other list (to show the corresponding filenames)
    g_signal_connect_swapped(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->load_file_content_view))),"changed", G_CALLBACK(Load_Filename_Select_Row_In_Other_List), G_OBJECT(priv->load_file_name_view));
    g_signal_connect_swapped(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->load_file_name_view))),"changed", G_CALLBACK(Load_Filename_Select_Row_In_Other_List), G_OBJECT(priv->load_file_content_view));

    // Commands (like the popup menu)
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_SPACING);
    gtk_box_pack_start(GTK_BOX(filelistvbox),hbox,FALSE,FALSE,0);

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Insert a blank line before the selected line"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Insert_Blank_Line), G_OBJECT(priv->load_file_name_view));

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Delete the selected line"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Delete_Line), G_OBJECT(priv->load_file_name_view));

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Delete all blank lines"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Delete_All_Blank_Lines), G_OBJECT(priv->load_file_name_view));
    
    Label = gtk_label_new("   ");
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Move up the selected line"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Move_Up), G_OBJECT(priv->load_file_name_view));

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Move down the selected line"));
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(Load_Filename_List_Move_Down), G_OBJECT(priv->load_file_name_view));

    Label = gtk_label_new("   ");
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    Button = gtk_button_new();
    Icon = gtk_image_new_from_stock(GTK_STOCK_REFRESH,GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(GTK_CONTAINER(Button),Icon);
    gtk_box_pack_start(GTK_BOX(hbox),Button,FALSE,FALSE,0);
    //gtk_button_set_relief(GTK_BUTTON(Button),GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(Button,_("Reload"));
    g_signal_connect (Button, "clicked",
                      G_CALLBACK (on_load_file_name_view_reload_clicked),
                      self);
    
    gtk_widget_show_all(filelistvbox);

    
    // Load the list of files in the list widget
    Load_File_List (self);

    vboxpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_pack1(GTK_PANED(vboxpaned),loadedvbox,  TRUE,FALSE);
    gtk_paned_pack2(GTK_PANED(vboxpaned),filelistvbox,TRUE,FALSE);
    gtk_box_pack_start(GTK_BOX(content_area),vboxpaned,TRUE,TRUE,0);

    // Create popup menus
    create_load_file_content_view_popup (self, priv->load_file_content_view);
    create_load_file_name_view_popup (self, priv->load_file_name_view);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_SPACING);
    gtk_box_pack_start(GTK_BOX(content_area),hbox,FALSE,TRUE,0);

    Label = gtk_label_new(_("Selected line:"));
    gtk_box_pack_start(GTK_BOX(hbox),Label,FALSE,FALSE,0);

    // Entry to edit a line into the list
    priv->selected_line_entry = Entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox),Entry,TRUE,TRUE,0);
    g_signal_connect(G_OBJECT(Entry),"changed",G_CALLBACK(Load_Filename_Update_Text_Line),G_OBJECT(priv->load_file_content_view));

    // Signal to load the line text in the editing entry
    g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->load_file_content_view)),
                      "changed", G_CALLBACK(Load_Filename_Edit_Text_Line),
                      self);

    // Separator line
    Separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(content_area),Separator,FALSE,FALSE,0);

    priv->load_file_run_scanner = gtk_check_button_new_with_label(_("Run the current scanner for each file"));
    gtk_box_pack_start(GTK_BOX(content_area),priv->load_file_run_scanner,FALSE,TRUE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->load_file_run_scanner),LOAD_FILE_RUN_SCANNER);
    gtk_widget_set_tooltip_text(priv->load_file_run_scanner,_("When activating this option, after loading the "
        "filenames, the current selected scanner will be ran (the scanner window must be opened)."));

    // To initialize 'ButtonLoad' sensivity
    g_signal_emit_by_name(G_OBJECT(gtk_bin_get_child(GTK_BIN(priv->file_to_load_combo))),"changed");
}

/*
 * For the configuration file...
 */
void
et_load_files_dialog_apply_changes (EtLoadFilesDialog *self)
{
    EtLoadFilesDialogPrivate *priv;

    g_return_if_fail (ET_LOAD_FILES_DIALOG (self));

    priv = et_load_files_dialog_get_instance_private (self);

    LOAD_FILE_RUN_SCANNER = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->load_file_run_scanner));
}


static void
et_load_files_dialog_finalize (GObject *object)
{
    G_OBJECT_CLASS (et_load_files_dialog_parent_class)->finalize (object);
}

static void
et_load_files_dialog_init (EtLoadFilesDialog *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, ET_TYPE_LOAD_FILES_DIALOG,
                                              EtLoadFilesDialogPrivate);

    create_load_files_dialog (self);
}

static void
et_load_files_dialog_class_init (EtLoadFilesDialogClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = et_load_files_dialog_finalize;

    g_type_class_add_private (klass, sizeof (EtLoadFilesDialogPrivate));
}

/*
 * et_load_files_dialog_new:
 *
 * Create a new EtLoadFilesDialog instance.
 *
 * Returns: a new #EtLoadFilesDialog
 */
EtLoadFilesDialog *
et_load_files_dialog_new (void)
{
    return g_object_new (ET_TYPE_LOAD_FILES_DIALOG, NULL);
}
