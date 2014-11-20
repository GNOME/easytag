/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


/* Some parts of this browser are taken from:
 * XMMS - Cross-platform multimedia player
 * Copyright (C) 1998-1999  Peter Alm, Mikael Alm, Olle Hallnas,
 * Thomas Nilsson and 4Front Technologies
 */

#include "config.h"

#include "browser.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <glib/gi18n.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#include "application_window.h"
#include "easytag.h"
#include "et_core.h"
#include "scan_dialog.h"
#include "log.h"
#include "misc.h"
#include "setting.h"
#include "charset.h"
#include "dlm.h"

#include "win32/win32dep.h"

/* TODO: Use G_DEFINE_TYPE_WITH_PRIVATE. */
G_DEFINE_TYPE (EtBrowser, et_browser, GTK_TYPE_BIN)

#define et_browser_get_instance_private(browser) (browser->priv)

struct _EtBrowserPrivate
{
    GtkWidget *label;
    GtkWidget *button;

    GtkWidget *entry_combo;
    GtkListStore *entry_model;

    GtkWidget *notebook;

    GtkListStore *file_model;
    GtkWidget *file_view;
    GtkWidget *file_menu;
    guint file_selected_handler;
    EtSortMode file_sort_mode;

    GtkWidget *album_list;
    GtkWidget *album_menu;
    GtkListStore *album_model;
    guint album_selected_handler;

    GtkWidget *artist_list;
    GtkWidget *artist_menu;
    GtkListStore *artist_model;
    guint artist_selected_handler;

    GtkWidget *tree; /* Tree of directories. */
    GtkWidget *tree_menu;
    GtkTreeStore *directory_model;

    GtkListStore *run_program_model;

    GtkWidget *open_directory_with_dialog;
    GtkWidget *open_directory_with_combobox;

    GtkWidget *open_files_with_dialog;
    GtkWidget *open_files_with_combobox;

    /* The Rename Directory window. */
    GtkWidget *rename_directory_dialog;
    GtkWidget *rename_directory_entry;
    GtkWidget *rename_directory_mask_toggle;
    GtkWidget *rename_directory_mask_entry;
    GtkWidget *rename_directory_preview_label;

    gchar *current_path;
};

/****************
 * Declarations *
 ****************/


static const guint BOX_SPACING = 6;

/*
 * EtPathState:
 * @ET_PATH_STATE_OPEN: the path is open or has been read
 * @ET_PATH_STATE_CLOSED: the path is closed or could not be read
 *
 * Whether to generate an icon with an indicaton that the directory is open
 * (being viewed) or closed (not yet viewed or read).
 */
typedef enum
{
    ET_PATH_STATE_OPEN,
    ET_PATH_STATE_CLOSED
} EtPathState;

enum
{
    LIST_FILE_NAME,
    /* Tag fields. */
    LIST_FILE_TITLE,
    LIST_FILE_ARTIST,
    LIST_FILE_ALBUM_ARTIST,
    LIST_FILE_ALBUM,
    LIST_FILE_YEAR,
    LIST_FILE_DISCNO,
    LIST_FILE_TRACK,
    LIST_FILE_GENRE,
    LIST_FILE_COMMENT,
    LIST_FILE_COMPOSER,
    LIST_FILE_ORIG_ARTIST,
    LIST_FILE_COPYRIGHT,
    LIST_FILE_URL,
    LIST_FILE_ENCODED_BY,
    /* End of columns with associated UI columns. */
    LIST_FILE_POINTER,
    LIST_FILE_KEY,
    LIST_FILE_OTHERDIR, /* To change color for alternate directories. */
    LIST_FONT_WEIGHT,
    LIST_ROW_BACKGROUND,
    LIST_ROW_FOREGROUND,
    LIST_COLUMN_COUNT
};

enum
{
    ALBUM_GICON,
    ALBUM_NAME,
    ALBUM_NUM_FILES,
    ALBUM_ETFILE_LIST_POINTER,
    ALBUM_FONT_STYLE,
    ALBUM_FONT_WEIGHT,
    ALBUM_ROW_FOREGROUND,
    ALBUM_ALL_ALBUMS_ROW,
    ALBUM_COLUMN_COUNT
};

enum
{
    ARTIST_PIXBUF,
    ARTIST_NAME,
    ARTIST_NUM_ALBUMS,
    ARTIST_NUM_FILES,
    ARTIST_ALBUM_LIST_POINTER,
    ARTIST_FONT_STYLE,
    ARTIST_FONT_WEIGHT,
    ARTIST_ROW_FOREGROUND,
    ARTIST_COLUMN_COUNT
};

enum
{
    TREE_COLUMN_DIR_NAME,
    TREE_COLUMN_FULL_PATH,
    TREE_COLUMN_SCANNED,
    TREE_COLUMN_HAS_SUBDIR,
    TREE_COLUMN_ICON,
    TREE_COLUMN_COUNT
};

/**************
 * Prototypes *
 **************/

static void Browser_Tree_Handle_Rename (EtBrowser *self,
                                        GtkTreeIter *parentnode,
                                        const gchar *old_path,
                                        const gchar *new_path);

static void Browser_List_Set_Row_Appearance (EtBrowser *self, GtkTreeIter *iter);
static gint Browser_List_Sort_Func (GtkTreeModel *model, GtkTreeIter *a,
                                    GtkTreeIter *b, gpointer data);
static void Browser_List_Select_File_By_Iter (EtBrowser *self,
                                              GtkTreeIter *iter,
                                              gboolean select_it);

static void Browser_Artist_List_Row_Selected (EtBrowser *self,
                                              GtkTreeSelection *selection);
static void Browser_Artist_List_Set_Row_Appearance (EtBrowser *self, GtkTreeIter *row);

static void Browser_Album_List_Load_Files (EtBrowser *self, GList *albumlist,
                                           ET_File *etfile_to_select);
static void Browser_Album_List_Row_Selected (EtBrowser *self,
                                             GtkTreeSelection *selection);
static void Browser_Album_List_Set_Row_Appearance (EtBrowser *self, GtkTreeIter *row);

static gboolean check_for_subdir (const gchar *path);

static GtkTreePath *Find_Child_Node (EtBrowser *self, GtkTreeIter *parent, gchar *searchtext);

static GIcon *get_gicon_for_path (const gchar *path, EtPathState path_state);

/* For window to rename a directory */
static void Destroy_Rename_Directory_Window (EtBrowser *self);
static void Rename_Directory_With_Mask_Toggled (EtBrowser *self);

/* For window to run a program with the directory */
static void Destroy_Run_Program_Tree_Window (EtBrowser *self);
static void Run_Program_With_Directory (EtBrowser *self);

/* For window to run a program with the file */
static void Destroy_Run_Program_List_Window (EtBrowser *self);

static void empty_entry_disable_widget (GtkWidget *widget, GtkEntry *entry);

static void et_rename_directory_on_response (GtkDialog *dialog,
                                             gint response_id,
                                             gpointer user_data);
static void et_run_program_tree_on_response (GtkDialog *dialog,
                                             gint response_id,
                                             gpointer user_data);
static void et_run_program_list_on_response (GtkDialog *dialog,
                                             gint response_id,
                                             gpointer user_data);

static void et_browser_on_column_clicked (GtkTreeViewColumn *column,
                                          gpointer data);


/*************
 * Functions *
 *************/
/*
 * Load home directory
 */
void
et_browser_go_home (EtBrowser *self)
{
    et_browser_select_dir (self, g_get_home_dir ());
}

/*
 * Load desktop directory
 */
void
et_browser_go_desktop (EtBrowser *self)
{
    et_browser_select_dir (self,
                           g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP));
}

/*
 * Load documents directory
 */
void
et_browser_go_documents (EtBrowser *self)
{
    et_browser_select_dir (self,
                           g_get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS));
}

/*
 * Load downloads directory
 */
void
et_browser_go_downloads (EtBrowser *self)
{
    et_browser_select_dir (self,
                           g_get_user_special_dir (G_USER_DIRECTORY_DOWNLOAD));
}

/*
 * Load music directory
 */
void
et_browser_go_music (EtBrowser *self)
{
    et_browser_select_dir (self,
                           g_get_user_special_dir (G_USER_DIRECTORY_MUSIC));
}


/*
 * Load default directory
 */
void
et_browser_load_default_dir (EtBrowser *self)
{
    GFile **files;
    GVariant *default_path;
    const gchar *path;

    default_path = g_settings_get_value (MainSettings, "default-path");
    path = g_variant_get_bytestring (default_path);

    files = g_new (GFile *, 1);
    files[0] = g_file_new_for_path (path);
    g_application_open (g_application_get_default (), files, 1, "");

    g_object_unref (files[0]);
    g_variant_unref (default_path);
    g_free (files);
}

void
et_browser_run_player_for_album_list (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    GList *l;
    GList *file_list = NULL;
    GError *error = NULL;

    priv = et_browser_get_instance_private (self);

    g_return_if_fail (priv->album_list != NULL);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->album_list));

    if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
        return;

    gtk_tree_model_get (GTK_TREE_MODEL (priv->album_model), &iter,
                        ALBUM_ETFILE_LIST_POINTER, &l, -1);

    for (; l != NULL; l = g_list_next (l))
    {
        ET_File *etfile = (ET_File *)l->data;
        gchar *path = ((File_Name *)etfile->FileNameCur->data)->value;
        file_list = g_list_prepend (file_list, g_file_new_for_path (path));
    }

    file_list = g_list_reverse (file_list);

    if (!et_run_audio_player (file_list, &error))
    {
        Log_Print (LOG_ERROR, _("Failed to launch program ‘%s’"),
                   error->message);
        g_error_free (error);
    }

    g_list_free_full (file_list, g_object_unref);
}

void
et_browser_run_player_for_artist_list (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    GList *l, *m;
    GList *file_list = NULL;
    GError *error = NULL;

    priv = et_browser_get_instance_private (self);

    g_return_if_fail (priv->artist_list != NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->artist_list));
    if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
        return;

    gtk_tree_model_get (GTK_TREE_MODEL (priv->artist_model), &iter,
                        ARTIST_ALBUM_LIST_POINTER, &l, -1);

    for (; l != NULL; l = g_list_next (l))
    {
        for (m = l->data; m != NULL; m = g_list_next (m))
        {
            ET_File *etfile = (ET_File *)m->data;
            gchar *path = ((File_Name *)etfile->FileNameCur->data)->value;
            file_list = g_list_prepend (file_list, g_file_new_for_path (path));
        }
    }

    file_list = g_list_reverse (file_list);

    if (!et_run_audio_player (file_list, &error))
    {
        Log_Print (LOG_ERROR, _("Failed to launch program ‘%s’"),
                   error->message);
        g_error_free (error);
    }

    g_list_free_full (file_list, g_object_unref);
}

void
et_browser_run_player_for_selection (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    GList *selfilelist = NULL;
    GList *l;
    GList *file_list = NULL;
    GtkTreeSelection *selection;
    GError *error = NULL;

    priv = et_browser_get_instance_private (self);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->file_view));
    selfilelist = gtk_tree_selection_get_selected_rows(selection, NULL);

    for (l = selfilelist; l != NULL; l = g_list_next (l))
    {
        ET_File *etfile = et_browser_get_et_file_from_path (self, l->data);
        gchar *path = ((File_Name *)etfile->FileNameCur->data)->value;
        file_list = g_list_prepend (file_list, g_file_new_for_path (path));
    }

    file_list = g_list_reverse (file_list);

    if (!et_run_audio_player (file_list, &error))
    {
        Log_Print (LOG_ERROR, _("Failed to launch program ‘%s’"),
                   error->message);
        g_error_free (error);
    }

    g_list_free_full (file_list, g_object_unref);
    g_list_free_full (selfilelist, (GDestroyNotify)gtk_tree_path_free);
}

/*
 * Get the path from the selected node (row) in the browser
 * Warning: return NULL if no row selected int the tree.
 * Remember to free the value returned from this function!
 */
static gchar *
Browser_Tree_Get_Path_Of_Selected_Node (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    GtkTreeSelection *selection;
    GtkTreeIter selectedIter;
    gchar *path;

    priv = et_browser_get_instance_private (self);

    g_return_val_if_fail (priv->tree != NULL, NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->tree));
    if (selection
    && gtk_tree_selection_get_selected(selection, NULL, &selectedIter))
    {
        gtk_tree_model_get(GTK_TREE_MODEL(priv->directory_model), &selectedIter,
                           TREE_COLUMN_FULL_PATH, &path, -1);
        return path;
    }else
    {
        return NULL;
    }
}


/*
 * Set the 'path' within the variable priv->current_path.
 */
static void
et_browser_set_current_path (EtBrowser *self, const gchar *path)
{
    EtBrowserPrivate *priv;

    g_return_if_fail (path != NULL);

    priv = et_browser_get_instance_private (self);

    /* Be sure that we aren't passing 'priv->current_path' as parameter of the
     * function to avoid an invalid read. */
    if (path == priv->current_path) return;

    g_free (priv->current_path);
    priv->current_path = g_strdup (path);

#ifdef G_OS_WIN32
    /* On win32 : "c:\path\to\dir" succeed with stat() for example, while "c:\path\to\dir\" fails */
    ET_Win32_Path_Remove_Trailing_Backslash(priv->current_path);
#endif /* G_OS_WIN32 */

    if (strcmp(G_DIR_SEPARATOR_S,priv->current_path) == 0)
        gtk_widget_set_sensitive (priv->button, FALSE);
    else
        gtk_widget_set_sensitive (priv->button, TRUE);
}


/*
 * Return the current path
 */
const gchar *
et_browser_get_current_path (EtBrowser *self)
{
    EtBrowserPrivate *priv;

    g_return_val_if_fail (ET_BROWSER (self), NULL);

    priv = et_browser_get_instance_private (self);

    return priv->current_path;
}

GtkTreeSelection *
et_browser_get_selection (EtBrowser *self)
{
    EtBrowserPrivate *priv;

    g_return_val_if_fail (ET_BROWSER (self), NULL);

    priv = et_browser_get_instance_private (self);

    return gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->file_view));
}

/*
 * Reload the current directory.
 */
void
et_browser_reload_directory (EtBrowser *self)
{
    EtBrowserPrivate *priv;

    priv = et_browser_get_instance_private (self);

    if (priv->tree && priv->current_path != NULL)
    {
        // Unselect files, to automatically reload the file of the directory
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->tree));
        if (selection)
        {
            gtk_tree_selection_unselect_all(selection);
        }
        et_browser_select_dir (self, priv->current_path);
    }
}

/*
 * Set the current path (selected node) in browser as default path (within config variable).
 */
void
et_browser_set_current_path_default (EtBrowser *self)
{
    EtBrowserPrivate *priv;

    g_return_if_fail (ET_BROWSER (self));

    priv = et_browser_get_instance_private (self);

    g_settings_set_value (MainSettings, "default-path",
                          g_variant_new_bytestring (priv->current_path));

    et_application_window_status_bar_message (ET_APPLICATION_WINDOW (MainWindow),
                                              _("New default directory selected for browser"),
                                              TRUE);
}

/*
 * When you press the key 'enter' in the BrowserEntry to validate the text (browse the directory)
 */
static void
Browser_Entry_Activated (EtBrowser *self, GtkEntry *entry)
{
    EtBrowserPrivate *priv;
    const gchar *path_utf8;
    gchar *path;

    priv = et_browser_get_instance_private (self);

    path_utf8 = gtk_entry_get_text (entry);
    Add_String_To_Combo_List (GTK_LIST_STORE (priv->entry_model), path_utf8);

    path = filename_from_display(path_utf8);

    et_browser_select_dir (self, path);
    g_free(path);
}

/*
 * Set a text into the BrowserEntry (and don't activate it)
 */
void
et_browser_entry_set_text (EtBrowser *self, const gchar *text)
{
    EtBrowserPrivate *priv;

    priv = et_browser_get_instance_private (self);

    if (!text || !priv->entry_combo)
        return;

    gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->entry_combo))),
                        text);
}

/*
 * Button to go to parent directory
 */
void
et_browser_go_parent (EtBrowser *self)
{
    gchar *parent_dir, *path;

    /* TODO: Replace this with g_file_get_parent(). */
    parent_dir = g_strdup (et_browser_get_current_path (self));

    if (strlen(parent_dir)>1)
    {
        if ( parent_dir[strlen(parent_dir)-1]==G_DIR_SEPARATOR )
            parent_dir[strlen(parent_dir)-1] = '\0';
        path = g_path_get_dirname(parent_dir);

        et_browser_select_dir (self, path);
        g_free(path);
    }

    g_free (parent_dir);
}

/*
 * Set a text into the priv->label
 */
void
et_browser_label_set_text (EtBrowser *self, const gchar *text)
{
    EtBrowserPrivate *priv;

    g_return_if_fail (ET_BROWSER (self));
    g_return_if_fail (text != NULL);

    priv = et_browser_get_instance_private (self);

    gtk_label_set_text (GTK_LABEL (priv->label), text);
}

/*
 * Key Press events into browser tree
 */
static gboolean
Browser_Tree_Key_Press (GtkWidget *tree, GdkEvent *event, gpointer data)
{
    GdkEventKey *kevent;
    GtkTreeIter SelectedNode;
    GtkTreeModel *treeModel;
    GtkTreeSelection *treeSelection;
    GtkTreePath *treePath;

    g_return_val_if_fail (tree != NULL, FALSE);

    treeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));

    if (event && event->type==GDK_KEY_PRESS)
    {
        if (!gtk_tree_selection_get_selected(treeSelection, &treeModel, &SelectedNode))
            return FALSE;

        kevent = (GdkEventKey *)event;
        treePath = gtk_tree_model_get_path(GTK_TREE_MODEL(treeModel), &SelectedNode);

        switch(kevent->keyval)
        {
            case GDK_KEY_KP_Enter:    /* Enter key in Num Pad */
            case GDK_KEY_Return:      /* 'Normal' Enter key */
            case GDK_KEY_t:           /* Expand/Collapse node */
            case GDK_KEY_T:           /* Expand/Collapse node */
                if(gtk_tree_view_row_expanded(GTK_TREE_VIEW(tree), treePath))
                    gtk_tree_view_collapse_row(GTK_TREE_VIEW(tree), treePath);
                else
                    gtk_tree_view_expand_row(GTK_TREE_VIEW(tree), treePath, FALSE);

                gtk_tree_path_free(treePath);
                return TRUE;
                break;

            case GDK_KEY_e:           /* Expand node */
            case GDK_KEY_E:           /* Expand node */
                gtk_tree_view_expand_row(GTK_TREE_VIEW(tree), treePath, FALSE);
                gtk_tree_path_free(treePath);
                return TRUE;
                break;

            case GDK_KEY_c:           /* Collapse node */
            case GDK_KEY_C:           /* Collapse node */
                gtk_tree_view_collapse_row(GTK_TREE_VIEW(tree), treePath);
                gtk_tree_path_free(treePath);
                return TRUE;
                break;
        }
        gtk_tree_path_free(treePath);
    }
    return FALSE;
}

/*
 * Key press into browser list
 *   - Delete = delete file
 * Also tries to capture text input and relate it to files
 */
static gboolean
Browser_List_Key_Press (GtkWidget *list, GdkEvent *event, gpointer data)
{
    GdkEventKey *kevent;
    GtkTreeSelection *fileSelection;

    g_return_val_if_fail (list != NULL, FALSE);

    fileSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));

    kevent = (GdkEventKey *)event;
    if (event && event->type==GDK_KEY_PRESS)
    {
        if (gtk_tree_selection_count_selected_rows(fileSelection))
        {
            switch(kevent->keyval)
            {
                case GDK_KEY_Delete:
                    g_action_group_activate_action (G_ACTION_GROUP (MainWindow),
                                                    "delete", NULL);
                    return TRUE;
            }
        }
    }

    return FALSE;
}

/*
 * Collapse (close) tree recursively up to the root node.
 */
void
et_browser_collapse (EtBrowser *self)
{
    EtBrowserPrivate *priv;
#ifndef G_OS_WIN32
    GtkTreePath *rootPath;
#endif /* !G_OS_WIN32 */

    priv = et_browser_get_instance_private (self);

    g_return_if_fail (priv->tree != NULL);

    gtk_tree_view_collapse_all(GTK_TREE_VIEW(priv->tree));

#ifndef G_OS_WIN32
    /* But keep the main directory opened */
    rootPath = gtk_tree_path_new_first();
    gtk_tree_view_expand_to_path(GTK_TREE_VIEW(priv->tree), rootPath);
    gtk_tree_path_free(rootPath);
#endif /* !G_OS_WIN32 */
}


/*
 * Set a row (or node) visible in the TreeView (by scrolling the tree)
 */
static void
Browser_Tree_Set_Node_Visible (GtkWidget *directoryView, GtkTreePath *path)
{
    g_return_if_fail (directoryView != NULL || path != NULL);

    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(directoryView), path, NULL, TRUE, 0.5, 0.0);
}


/*
 * Set a row visible in the file list (by scrolling the list)
 */
static void
et_browser_set_row_visible (EtBrowser *self, GtkTreeIter *rowIter)
{
    EtBrowserPrivate *priv;

    /*
     * TODO: Make this only scroll to the row if it is not visible
     * (like in easytag GTK1)
     * See function gtk_tree_view_get_visible_rect() ??
     */
    GtkTreePath *rowPath;

    priv = et_browser_get_instance_private (self);

    g_return_if_fail (rowIter != NULL);

    rowPath = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->file_model),
                                       rowIter);
    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (priv->file_view), rowPath,
                                  NULL, FALSE, 0, 0);
    gtk_tree_path_free (rowPath);
}

/*
 * Triggers when a new node in the browser tree is selected
 * Do file-save confirmation, and then prompt the new dir to be loaded
 */
static gboolean
Browser_Tree_Node_Selected (EtBrowser *self, GtkTreeSelection *selection)
{
    EtBrowserPrivate *priv;
    gchar *pathName, *pathName_utf8;
    static int counter = 0;
    GtkTreeIter selectedIter;
    GtkTreePath *selectedPath;

    priv = et_browser_get_instance_private (self);

    if (!gtk_tree_selection_get_selected(selection, NULL, &selectedIter))
        return TRUE;
    selectedPath = gtk_tree_model_get_path(GTK_TREE_MODEL(priv->directory_model), &selectedIter);

    /* Open the node */
    if (g_settings_get_boolean (MainSettings, "browse-expand-children"))
    {
        gtk_tree_view_expand_row(GTK_TREE_VIEW(priv->tree), selectedPath, FALSE);
    }
    gtk_tree_path_free(selectedPath);

    /* Don't start a new reading, if another one is running... */
    if (ReadingDirectory == TRUE)
        return TRUE;

    //Browser_Tree_Set_Node_Visible(priv->tree, selectedPath);
    gtk_tree_model_get(GTK_TREE_MODEL(priv->directory_model), &selectedIter,
                       TREE_COLUMN_FULL_PATH, &pathName, -1);
    if (!pathName)
        return FALSE;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);
    /* FIXME: Not clean to put this here. */
    et_application_window_update_actions (ET_APPLICATION_WINDOW (MainWindow));

    /* Check if all files have been saved before changing the directory */
    if (g_settings_get_boolean (MainSettings, "confirm-when-unsaved-files")
        && ET_Check_If_All_Files_Are_Saved () != TRUE)
    {
        GtkWidget *msgdialog;
        gint response;

        msgdialog = gtk_message_dialog_new(GTK_WINDOW(MainWindow),
                                           GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_QUESTION,
                                           GTK_BUTTONS_NONE,
                                           "%s",
                                           _("Some files have been modified but not saved"));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (msgdialog),
                                                  "%s",
                                                  _("Do you want to save them before changing directory?"));
        gtk_dialog_add_buttons (GTK_DIALOG (msgdialog), _("_Discard"),
                                GTK_RESPONSE_NO, _("_Cancel"),
                                GTK_RESPONSE_CANCEL, _("_Save"),
                                GTK_RESPONSE_YES, NULL);
        gtk_dialog_set_default_response (GTK_DIALOG (msgdialog),
                                         GTK_RESPONSE_YES);
        gtk_window_set_title(GTK_WINDOW(msgdialog),_("Confirm Directory Change"));

        response = gtk_dialog_run(GTK_DIALOG(msgdialog));
        gtk_widget_destroy(msgdialog);
        switch (response)
        {
            case GTK_RESPONSE_YES:
                if (Save_All_Files_With_Answer(FALSE)==-1)
                {
                    g_free (pathName);
                    return TRUE;
                }
                break;
            case GTK_RESPONSE_NO:
                break;
            case GTK_RESPONSE_CANCEL:
            case GTK_RESPONSE_DELETE_EVENT:
                g_free (pathName);
                return TRUE;
                break;
            default:
                g_assert_not_reached ();
                break;
        }
    }

    /* Memorize the current path */
    et_browser_set_current_path (self, pathName);

    /* Display the selected path into the BrowserEntry */
    pathName_utf8 = filename_to_display(pathName);
    gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(priv->entry_combo))), pathName_utf8);

    /* Start to read the directory */
    /* The first time, 'counter' is equal to zero. And if we don't want to load
     * directory on startup, we skip the 'reading', but newt we must read it each time */
    if (g_settings_get_boolean (MainSettings, "load-on-startup") || counter)
    {
        gboolean dir_loaded;
        GtkTreeIter parentIter;

        dir_loaded = Read_Directory(pathName);

        // If the directory can't be loaded, the directory musn't exist.
        // So we load the parent node and refresh the children
        if (dir_loaded == FALSE)
        {
            if (gtk_tree_selection_get_selected(selection, NULL, &selectedIter))
            {
                GFile *file = g_file_new_for_path (pathName);

                /* If the path could not be read, then it is possible that it
                 * has a subdirectory with readable permissions. In that case
                 * do not refresh the children. */
                if (gtk_tree_model_iter_parent(GTK_TREE_MODEL(priv->directory_model),&parentIter,&selectedIter) )
                {
                    selectedPath = gtk_tree_model_get_path(GTK_TREE_MODEL(priv->directory_model), &parentIter);
                    gtk_tree_selection_select_iter (selection, &parentIter);
                    if (gtk_tree_model_iter_has_child (GTK_TREE_MODEL (priv->directory_model),
                                                       &selectedIter) == FALSE
                                                       && !g_file_query_exists (file, NULL))
                    {
                        gtk_tree_view_collapse_row (GTK_TREE_VIEW (priv->tree),
                                                    selectedPath);
                        if (g_settings_get_boolean (MainSettings,
                                                    "browse-expand-children"))
                        {
                            gtk_tree_view_expand_row (GTK_TREE_VIEW (priv->tree),
                                                      selectedPath, FALSE);
                        }
                        gtk_tree_path_free (selectedPath);
                    }
                }
                g_object_unref (file);
            }
        }

    }else
    {
        /* As we don't use the function 'Read_Directory' we must add this function here */
        et_application_window_update_actions (ET_APPLICATION_WINDOW (MainWindow));
    }
    counter++;

    g_free(pathName);
    g_free(pathName_utf8);
    return FALSE;
}


#ifdef G_OS_WIN32
static gboolean
et_browser_win32_get_drive_root (EtBrowser *self,
                                 gchar *drive,
                                 GtkTreeIter *rootNode,
                                 GtkTreePath **rootPath)
{
    EtBrowserPrivate *priv;
    gint root_index;
    gboolean found = FALSE;
    GtkTreeIter parentNode;
    gchar *nodeName;

    priv = et_browser_get_instance_private (self);

    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->directory_model), &parentNode);

    // Find root of path, ie: the drive letter
    root_index = 0;

    do
    {
        gtk_tree_model_get(GTK_TREE_MODEL(priv->directory_model), &parentNode,
                           TREE_COLUMN_FULL_PATH, &nodeName, -1);
        if (strncasecmp(drive,nodeName, strlen(drive)) == 0)
        {
            g_free(nodeName);
            found = TRUE;
            break;
        }
        root_index++;
    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->directory_model), &parentNode));

    if (!found) return FALSE;

    *rootNode = parentNode;
    *rootPath = gtk_tree_path_new_from_indices(root_index, -1);

    return TRUE;
}
#endif /* G_OS_WIN32 */


/*
 * et_browser_select_dir: Select the directory corresponding to the 'path' in
 * the tree browser, but it doesn't read it!
 * Check if path is correct before selecting it.
 *
 * - "current_path" is in file system encoding (not UTF-8)
 */
void
et_browser_select_dir (EtBrowser *self, const gchar *current_path)
{
    EtBrowserPrivate *priv;
    GtkTreePath *rootPath = NULL;
    GtkTreeIter parentNode, currentNode;
    gint index = 1; // Skip the first token as it is NULL due to leading /
    gchar **parts;
    gchar *nodeName;
    gchar *temp;

    priv = et_browser_get_instance_private (self);

    g_return_if_fail (priv->tree != NULL);

    /* Load current_path */
    if(!current_path || !*current_path)
    {
        return;
    }

    /* Don't check here if the path is valid. It will be done later when
     * selecting a node in the tree */

    et_browser_set_current_path (self, current_path);

    parts = g_strsplit(current_path, G_DIR_SEPARATOR_S, 0);

    // Expand root node (fill parentNode and rootPath)
#ifdef G_OS_WIN32
    if (!et_browser_win32_get_drive_root (self, parts[0], &parentNode,
                                          &rootPath))
    {
        return;
    }
#else /* !G_OS_WIN32 */
    if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->directory_model),
                                        &parentNode))
    {
        g_message ("%s", "priv->directory_model is empty");
        return;
    }

    rootPath = gtk_tree_path_new_first();
#endif /* !G_OS_WIN32 */
    if (rootPath)
    {
        gtk_tree_view_expand_to_path(GTK_TREE_VIEW(priv->tree), rootPath);
        gtk_tree_path_free(rootPath);
    }

    while (parts[index]) // it is NULL-terminated
    {
        if (strlen(parts[index]) == 0)
        {
            index++;
            continue;
        }

        if (!gtk_tree_model_iter_children(GTK_TREE_MODEL(priv->directory_model), &currentNode, &parentNode))
        {
            gchar *path, *parent_path;
            GFile *file;

            gtk_tree_model_get (GTK_TREE_MODEL (priv->directory_model),
                                &parentNode, TREE_COLUMN_FULL_PATH,
                                &parent_path, -1);
            path = g_build_path (G_DIR_SEPARATOR_S, parent_path, parts[index],
                                 NULL);
            g_free (parent_path);

            file = g_file_new_for_path (path);

            /* As dir name was not found in any node, check whether it exists
             * or not. */
            if (g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL)
                == G_FILE_TYPE_DIRECTORY)
            {
                /* It exists and is readable permission of parent directory is executable */
                GIcon *icon;
                GtkTreeIter iter;

                /* Create a new node for this directory name. */
                icon = get_gicon_for_path (path, ET_PATH_STATE_CLOSED);

                gtk_tree_store_insert_with_values (GTK_TREE_STORE (priv->directory_model),
                                                   &iter, &parentNode, 0,
                                                   TREE_COLUMN_DIR_NAME, parts[index],
                                                   TREE_COLUMN_FULL_PATH, path,
                                                   TREE_COLUMN_HAS_SUBDIR, check_for_subdir (current_path),
                                                   TREE_COLUMN_SCANNED, TRUE,
                                                   TREE_COLUMN_ICON, icon, -1);

                currentNode = iter;
                g_object_unref (icon);
            }
            else
            {
                g_object_unref (file);
                g_free (path);
                break;
            }

            g_object_unref (file);
            g_free (path);
        }

        do
        {
            gtk_tree_model_get(GTK_TREE_MODEL(priv->directory_model), &currentNode,
                               TREE_COLUMN_FULL_PATH, &temp, -1);
            nodeName = g_path_get_basename(temp);
            g_free(temp);
#ifdef G_OS_WIN32
            if (strcasecmp(parts[index],nodeName) == 0)
#else /* !G_OS_WIN32 */
            if (strcmp(parts[index],nodeName) == 0)
#endif /* !G_OS_WIN32 */
            {
                g_free(nodeName);
                break;
            }
            g_free(nodeName);

            if (!gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->directory_model),
                                           &currentNode))
            {
                /* Path was not found in tree, such as when a hidden path was
                 * passed in, but hidden paths are set to not be displayed. */
                g_strfreev (parts);
                return;
            }
        } while (1);

        parentNode = currentNode;
        rootPath = gtk_tree_model_get_path(GTK_TREE_MODEL(priv->directory_model), &parentNode);
        if (rootPath)
        {
            gtk_tree_view_expand_to_path(GTK_TREE_VIEW(priv->tree), rootPath);
            gtk_tree_path_free(rootPath);
        }
        index++;
    }

    rootPath = gtk_tree_model_get_path(GTK_TREE_MODEL(priv->directory_model), &parentNode);
    if (rootPath)
    {
        gtk_tree_view_expand_to_path(GTK_TREE_VIEW(priv->tree), rootPath);
        // Select the node to load the corresponding directory
        gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->tree)), rootPath);
        Browser_Tree_Set_Node_Visible(priv->tree, rootPath);
        gtk_tree_path_free(rootPath);
    }

    g_strfreev(parts);
    return;
}

/*
 * Callback to select-row event
 * Displays the file info of the lowest selected file in the right-hand pane
 */
static void
Browser_List_Row_Selected (EtBrowser *self, GtkTreeSelection *selection)
{
    EtBrowserPrivate *priv;
    gint n_selected;
    GtkTreePath *cursor_path;
    GtkTreeIter cursor_iter;
    ET_File *cursor_et_file;


    priv = et_browser_get_instance_private (self);

    n_selected = gtk_tree_selection_count_selected_rows (selection);

    /*
     * After a file is deleted, this function is called :
     * So we must handle the situation if no rows are selected
     */
    if (n_selected == 0)
    {
        return;
    }

    gtk_tree_view_get_cursor (GTK_TREE_VIEW (priv->file_view),
                              &cursor_path, NULL);

    if (!cursor_path)
    {
        return;
    }

    if (gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->file_model),
                                 &cursor_iter, cursor_path))
    {
        gtk_tree_model_get (GTK_TREE_MODEL (priv->file_model),
                            &cursor_iter, LIST_FILE_POINTER,
                            &cursor_et_file, -1);
        et_application_window_select_file_by_et_file (ET_APPLICATION_WINDOW (MainWindow),
                                                      cursor_et_file);
    }
    else
    {
        g_warning ("%s", "Error getting iter from cursor path");
    }

    gtk_tree_path_free (cursor_path);
}

/*
 * Empty model, disabling Browser_List_Row_Selected () during clear because it
 * is called and causes crashes otherwise.
 */
static void
et_browser_clear_file_model (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    GtkTreeSelection *selection;

    priv = et_browser_get_instance_private (self);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->file_view));

    g_signal_handler_block (selection, priv->file_selected_handler);

    gtk_list_store_clear (priv->file_model);
    gtk_tree_view_columns_autosize (GTK_TREE_VIEW (priv->file_view));

    g_signal_handler_unblock (selection, priv->file_selected_handler);
}

/*
 * Loads the specified etfilelist into the browser list
 * Also supports optionally selecting a specific etfile
 * but be careful, this does not call Browser_List_Row_Selected !
 */
void
et_browser_load_file_list (EtBrowser *self,
                           GList *etfilelist,
                           const ET_File *etfile_to_select)
{
    EtBrowserPrivate *priv;
    GList *l;
    gboolean activate_bg_color = 0;
    GtkTreeIter rowIter;

    g_return_if_fail (ET_BROWSER (self));

    priv = et_browser_get_instance_private (self);

    et_browser_clear_file_model (self);

    for (l = g_list_first (etfilelist); l != NULL; l = g_list_next (l))
    {
        guint fileKey = ((ET_File *)l->data)->ETFileKey;
        gchar *current_filename_utf8 = ((File_Name *)((ET_File *)l->data)->FileNameCur->data)->value_utf8;
        gchar *basename_utf8         = g_path_get_basename(current_filename_utf8);
        File_Tag *FileTag = ((File_Tag *)((ET_File *)l->data)->FileTag->data);
        gchar *track;
        gchar *disc;

        // Change background color when changing directory (the first row must not be changed)
        if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(priv->file_model), NULL) > 0)
        {
            gchar *dir1_utf8;
            gchar *dir2_utf8;
            gchar *previous_filename_utf8 = ((File_Name *)((ET_File *)l->prev->data)->FileNameCur->data)->value_utf8;

            dir1_utf8 = g_path_get_dirname(previous_filename_utf8);
            dir2_utf8 = g_path_get_dirname(current_filename_utf8);

            if (g_utf8_collate(dir1_utf8, dir2_utf8) != 0)
                activate_bg_color = !activate_bg_color;

            g_free(dir1_utf8);
            g_free(dir2_utf8);
        }

        /* File list displays the current filename (name on disc) and tag
         * fields. */
        track = g_strconcat(FileTag->track ? FileTag->track : "",FileTag->track_total ? "/" : NULL,FileTag->track_total,NULL);
        disc  = g_strconcat (FileTag->disc_number ? FileTag->disc_number : "",
                             FileTag->disc_total ? "/"
                                                 : NULL, FileTag->disc_total,
                             NULL);

        gtk_list_store_insert_with_values (priv->file_model, &rowIter, G_MAXINT,
                                           LIST_FILE_NAME, basename_utf8,
                                           LIST_FILE_POINTER, l->data,
                                           LIST_FILE_KEY, fileKey,
                                           LIST_FILE_OTHERDIR,
                                           activate_bg_color,
                                           LIST_FILE_TITLE, FileTag->title,
                                           LIST_FILE_ARTIST, FileTag->artist,
                                           LIST_FILE_ALBUM_ARTIST,
                                           FileTag->album_artist,
                                           LIST_FILE_ALBUM, FileTag->album,
                                           LIST_FILE_YEAR, FileTag->year,
                                           LIST_FILE_DISCNO, disc,
                                           LIST_FILE_TRACK, track,
                                           LIST_FILE_GENRE, FileTag->genre,
                                           LIST_FILE_COMMENT, FileTag->comment,
                                           LIST_FILE_COMPOSER,
                                           FileTag->composer,
                                           LIST_FILE_ORIG_ARTIST,
                                           FileTag->orig_artist,
                                           LIST_FILE_COPYRIGHT,
                                           FileTag->copyright,
                                           LIST_FILE_URL, FileTag->url,
                                           LIST_FILE_ENCODED_BY,
                                           FileTag->encoded_by, -1);
        g_free(basename_utf8);
        g_free(track);
        g_free (disc);

        if (etfile_to_select == l->data)
        {
            Browser_List_Select_File_By_Iter (self, &rowIter, TRUE);
            //ET_Display_File_Data_To_UI (l->data);
        }

        /* Set appearance of the row. */
        Browser_List_Set_Row_Appearance (self, &rowIter);
    }
}


/*
 * Update state of files in the list after changes (without clearing the list model!)
 *  - Refresh 'filename' is file saved,
 *  - Change color is something changed on the file
 */
void
et_browser_refresh_list (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    ET_File   *ETFile;
    File_Tag  *FileTag;
    File_Name *FileName;
    //GtkTreeIter iter;
    GtkTreePath *currentPath = NULL;
    GtkTreeIter iter;
    gint row;
    gchar *current_basename_utf8;
    gchar *track;
    gchar *disc;
    gboolean valid;
    GVariant *variant;

    g_return_if_fail (ET_BROWSER (self));

    priv = et_browser_get_instance_private (self);

    if (!ETCore->ETFileDisplayedList || !priv->file_view
    ||  gtk_tree_model_iter_n_children(GTK_TREE_MODEL(priv->file_model), NULL) == 0)
    {
        return;
    }

    // Browse the full list for changes
    //gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->file_model), &iter);
    //    g_print("above worked %d rows\n", gtk_tree_model_iter_n_children(GTK_TREE_MODEL(priv->file_model), NULL));

    currentPath = gtk_tree_path_new_first();

    valid = gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->file_model), &iter, currentPath);
    while (valid)
    {
        // Refresh filename and other fields
        gtk_tree_model_get(GTK_TREE_MODEL(priv->file_model), &iter,
                           LIST_FILE_POINTER, &ETFile, -1);

        FileName = (File_Name *)ETFile->FileNameCur->data;
        FileTag  = (File_Tag *)ETFile->FileTag->data;

        current_basename_utf8 = g_path_get_basename(FileName->value_utf8);
        track = g_strconcat(FileTag->track ? FileTag->track : "",FileTag->track_total ? "/" : NULL,FileTag->track_total,NULL);
        disc  = g_strconcat (FileTag->disc_number ? FileTag->disc_number : "",
                             FileTag->disc_total ? "/" : NULL,
                             FileTag->disc_total, NULL);

        gtk_list_store_set(priv->file_model, &iter,
                           LIST_FILE_NAME,          current_basename_utf8,
                           LIST_FILE_TITLE,         FileTag->title,
                           LIST_FILE_ARTIST,        FileTag->artist,
                           LIST_FILE_ALBUM_ARTIST,  FileTag->album_artist,
						   LIST_FILE_ALBUM,         FileTag->album,
                           LIST_FILE_YEAR,          FileTag->year,
                           LIST_FILE_DISCNO, disc,
                           LIST_FILE_TRACK,         track,
                           LIST_FILE_GENRE,         FileTag->genre,
                           LIST_FILE_COMMENT,       FileTag->comment,
                           LIST_FILE_COMPOSER,      FileTag->composer,
                           LIST_FILE_ORIG_ARTIST,   FileTag->orig_artist,
                           LIST_FILE_COPYRIGHT,     FileTag->copyright,
                           LIST_FILE_URL,           FileTag->url,
                           LIST_FILE_ENCODED_BY,    FileTag->encoded_by,
                           -1);
        g_free(current_basename_utf8);
        g_free(track);
        g_free (disc);

        Browser_List_Set_Row_Appearance (self, &iter);

        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->file_model), &iter);
    }
    gtk_tree_path_free(currentPath);

    variant = g_action_group_get_action_state (G_ACTION_GROUP (MainWindow),
                                               "file-artist-view");

    // When displaying Artist + Album lists => refresh also rows color
    if (strcmp (g_variant_get_string (variant, NULL), "artist") == 0)
    {

        for (row = 0; row < gtk_tree_model_iter_n_children (GTK_TREE_MODEL (priv->artist_model), NULL); row++)
        {
            if (row == 0)
                currentPath = gtk_tree_path_new_first();
            else
                gtk_tree_path_next(currentPath);

            gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->artist_model), &iter, currentPath);
            Browser_Artist_List_Set_Row_Appearance (self, &iter);
        }
        gtk_tree_path_free(currentPath);


        for (row=0; row < gtk_tree_model_iter_n_children(GTK_TREE_MODEL(priv->album_model), NULL); row++)
        {
            if (row == 0)
                currentPath = gtk_tree_path_new_first();
            else
                gtk_tree_path_next(currentPath);

            gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->album_model), &iter, currentPath);
            Browser_Album_List_Set_Row_Appearance (self, &iter);
        }
        gtk_tree_path_free(currentPath);
    }

    g_variant_unref (variant);
}


/*
 * Update state of one file in the list after changes (without clearing the clist!)
 *  - Refresh filename is file saved,
 *  - Change color is something change on the file
 */
void
et_browser_refresh_file_in_list (EtBrowser *self,
                                 const ET_File *ETFile)
{
    EtBrowserPrivate *priv;
    GList *selectedRow = NULL;
    GVariant *variant;
    GtkTreeSelection *selection;
    GtkTreeIter selectedIter;
    ET_File   *etfile;
    File_Tag  *FileTag;
    File_Name *FileName;
    gboolean row_found = FALSE;
    gchar *current_basename_utf8;
    gchar *track;
    gchar *disc;
    gboolean valid;
    gchar *artist, *album;

    g_return_if_fail (ET_BROWSER (self));

    priv = et_browser_get_instance_private (self);

    if (!ETCore->ETFileDisplayedList || !priv->file_view || !ETFile ||
        gtk_tree_model_iter_n_children(GTK_TREE_MODEL(priv->file_model), NULL) == 0)
    {
        return;
    }

    // Search the row of the modified file to update it (when found: row_found=TRUE)
    // 1/3. Get position of ETFile in ETFileList
    if (row_found == FALSE)
    {
        valid = gtk_tree_model_iter_nth_child (GTK_TREE_MODEL(priv->file_model), &selectedIter, NULL, ETFile->IndexKey-1);
        if (valid)
        {
            gtk_tree_model_get(GTK_TREE_MODEL(priv->file_model), &selectedIter,
                           LIST_FILE_POINTER, &etfile, -1);
            if (ETFile->ETFileKey == etfile->ETFileKey)
            {
                row_found = TRUE;
            }
        }
    }

    // 2/3. Try with the selected file in list (works only if we select the same file)
    if (row_found == FALSE)
    {
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->file_view));
        selectedRow = gtk_tree_selection_get_selected_rows(selection, NULL);
        if (selectedRow && selectedRow->data != NULL)
        {
            valid = gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->file_model), &selectedIter,
                                    (GtkTreePath*) selectedRow->data);
            if (valid)
            {
                gtk_tree_model_get(GTK_TREE_MODEL(priv->file_model), &selectedIter,
                                   LIST_FILE_POINTER, &etfile, -1);
                if (ETFile->ETFileKey == etfile->ETFileKey)
                {
                    row_found = TRUE;
                }
            }
        }

        g_list_free_full (selectedRow, (GDestroyNotify)gtk_tree_path_free);
    }

    // 3/3. Fails, now we browse the full list to find it
    if (row_found == FALSE)
    {
        valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->file_model), &selectedIter);
        while (valid)
        {
            gtk_tree_model_get(GTK_TREE_MODEL(priv->file_model), &selectedIter,
                               LIST_FILE_POINTER, &etfile, -1);
            if (ETFile->ETFileKey == etfile->ETFileKey)
            {
                row_found = TRUE;
                break;
            } else
            {
                valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->file_model), &selectedIter);
            }
        }
    }

    // Error somewhere...
    if (row_found == FALSE)
        return;

    // Displayed the filename and refresh other fields
    FileName = (File_Name *)etfile->FileNameCur->data;
    FileTag  = (File_Tag *)etfile->FileTag->data;

    current_basename_utf8 = g_path_get_basename(FileName->value_utf8);
    track = g_strconcat(FileTag->track ? FileTag->track : "",FileTag->track_total ? "/" : NULL,FileTag->track_total,NULL);
    disc  = g_strconcat (FileTag->disc_number ? FileTag->disc_number : "",
                         FileTag->disc_total ? "/" : NULL, FileTag->disc_total,
                         NULL);

    gtk_list_store_set(priv->file_model, &selectedIter,
                       LIST_FILE_NAME,          current_basename_utf8,
                       LIST_FILE_TITLE,         FileTag->title,
                       LIST_FILE_ARTIST,        FileTag->artist,
                       LIST_FILE_ALBUM_ARTIST,  FileTag->album_artist,
					   LIST_FILE_ALBUM,         FileTag->album,
                       LIST_FILE_YEAR,          FileTag->year,
                       LIST_FILE_DISCNO, disc,
                       LIST_FILE_TRACK,         track,
                       LIST_FILE_GENRE,         FileTag->genre,
                       LIST_FILE_COMMENT,       FileTag->comment,
                       LIST_FILE_COMPOSER,      FileTag->composer,
                       LIST_FILE_ORIG_ARTIST,   FileTag->orig_artist,
                       LIST_FILE_COPYRIGHT,     FileTag->copyright,
                       LIST_FILE_URL,           FileTag->url,
                       LIST_FILE_ENCODED_BY,    FileTag->encoded_by,
                       -1);
    g_free(current_basename_utf8);
    g_free(track);
    g_free (disc);

    /* Change appearance (line to red) if filename changed. */
    Browser_List_Set_Row_Appearance (self, &selectedIter);

    variant = g_action_group_get_action_state (G_ACTION_GROUP (MainWindow),
                                               "file-artist-view");

    /* When displaying Artist + Album lists => refresh also rows color. */
    if (strcmp (g_variant_get_string (variant, NULL), "artist") == 0)
    {
        gchar *current_artist = ((File_Tag *)ETFile->FileTag->data)->artist;
        gchar *current_album  = ((File_Tag *)ETFile->FileTag->data)->album;

        valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->artist_model),
                                               &selectedIter);

        while (valid)
        {
                gtk_tree_model_get (GTK_TREE_MODEL (priv->artist_model),
                                    &selectedIter, ARTIST_NAME, &artist, -1);

                if ((!current_artist && !artist)
                    || (current_artist && artist
                        && g_utf8_collate (current_artist, artist) == 0))
                {
                    /* Set color of the row. */
                    Browser_Artist_List_Set_Row_Appearance (self,
                                                            &selectedIter);
                    g_free (artist);
                    break;
                }

                g_free (artist);

                valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->artist_model),
                                                  &selectedIter);
        }

        //
        // FIX ME : see also if we must add a new line / or change list of the ETFile
        //
        valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->album_model),
                                               &selectedIter);

        while (valid)
        {
            gtk_tree_model_get (GTK_TREE_MODEL (priv->album_model),
                                &selectedIter, ALBUM_NAME, &album, -1);

            if ((!current_album && !album)
                || (current_album && album
                    && g_utf8_collate (current_album, album) == 0))
            {
                /* Set color of the row. */
                Browser_Album_List_Set_Row_Appearance (self, &selectedIter);
                g_free (album);
                break;
            }

            g_free (album);

            valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->album_model),
                                              &selectedIter);
        }

        //
        // FIX ME : see also if we must add a new line / or change list of the ETFile
        //
    }

    g_variant_unref (variant);
}


/*
 * Set the appearance of the row
 *  - change background according LIST_FILE_OTHERDIR
 *  - change foreground according file status (saved or not)
 */
static void
Browser_List_Set_Row_Appearance (EtBrowser *self, GtkTreeIter *iter)
{
    EtBrowserPrivate *priv;
    ET_File *rowETFile = NULL;
    gboolean otherdir = FALSE;
    const GdkRGBA LIGHT_BLUE = { 0.866, 0.933, 1.0, 1.0 };
    const GdkRGBA *background;
    //gchar *temp = NULL;

    priv = et_browser_get_instance_private (self);

    if (iter == NULL)
        return;

    // Get the ETFile reference
    gtk_tree_model_get(GTK_TREE_MODEL(priv->file_model), iter,
                       LIST_FILE_POINTER,   &rowETFile,
                       LIST_FILE_OTHERDIR,  &otherdir,
                       //LIST_FILE_NAME,      &temp,
                       -1);

    // Must change background color?
    if (otherdir)
        background = &LIGHT_BLUE;
    else
        background = NULL;

    // Set text to bold/red if 'filename' or 'tag' changed
    if ( ET_Check_If_File_Is_Saved(rowETFile) == FALSE )
    {
        if (g_settings_get_boolean (MainSettings, "file-changed-bold"))
        {
            gtk_list_store_set(priv->file_model, iter,
                               LIST_FONT_WEIGHT,    PANGO_WEIGHT_BOLD,
                               LIST_ROW_BACKGROUND, background,
                               LIST_ROW_FOREGROUND, NULL, -1);
        } else
        {
            gtk_list_store_set(priv->file_model, iter,
                               LIST_FONT_WEIGHT,    PANGO_WEIGHT_NORMAL,
                               LIST_ROW_BACKGROUND, background,
                               LIST_ROW_FOREGROUND, &RED, -1);
        }
    } else
    {
        gtk_list_store_set(priv->file_model, iter,
                           LIST_FONT_WEIGHT,    PANGO_WEIGHT_NORMAL,
                           LIST_ROW_BACKGROUND, background,
                           LIST_ROW_FOREGROUND, NULL ,-1);
    }

    // Update text fields
    // Don't do it here
    /*if (rowETFile)
    {
        File_Tag *FileTag = ((File_Tag *)((ET_File *)rowETFile)->FileTag->data);

        gtk_list_store_set(priv->file_model, iter,
                           LIST_FILE_TITLE,         FileTag->title,
                           LIST_FILE_ARTIST,        FileTag->artist,
                           LIST_FILE_ALBUM_ARTIST,  FileTag->album_artist,
						   LIST_FILE_ALBUM,         FileTag->album,
                           LIST_FILE_YEAR,          FileTag->year,
                           LIST_FILE_TRACK,         FileTag->track,
                           LIST_FILE_GENRE,         FileTag->genre,
                           LIST_FILE_COMMENT,       FileTag->comment,
                           LIST_FILE_COMPOSER,      FileTag->composer,
                           LIST_FILE_ORIG_ARTIST,   FileTag->orig_artist,
                           LIST_FILE_COPYRIGHT,     FileTag->copyright,
                           LIST_FILE_URL,           FileTag->url,
                           LIST_FILE_ENCODED_BY,    FileTag->encoded_by,
                           -1);
    }*/

    // Frees allocated item from gtk_tree_model_get...
    //g_free(temp);
}


/*
 * Remove a file from the list, by ETFile
 */
void
et_browser_remove_file (EtBrowser *self,
                        const ET_File *searchETFile)
{
    EtBrowserPrivate *priv;
    gint row;
    GtkTreePath *currentPath = NULL;
    GtkTreeIter currentIter;
    ET_File *currentETFile;
    gboolean valid;

    if (searchETFile == NULL)
        return;

    priv = et_browser_get_instance_private (self);

    // Go through the file list until it is found
    for (row=0; row < gtk_tree_model_iter_n_children(GTK_TREE_MODEL(priv->file_model), NULL); row++)
    {
        if (row == 0)
            currentPath = gtk_tree_path_new_first();
        else
            gtk_tree_path_next(currentPath);

        valid = gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->file_model), &currentIter, currentPath);
        if (valid)
        {
            gtk_tree_model_get(GTK_TREE_MODEL(priv->file_model), &currentIter,
                               LIST_FILE_POINTER, &currentETFile, -1);

            if (currentETFile == searchETFile)
            {
                gtk_list_store_remove(priv->file_model, &currentIter);
                break;
            }
        }
    }

    gtk_tree_path_free (currentPath);
}

/*
 * Get ETFile pointer of a file from a Tree Iter
 */
ET_File *
et_browser_get_et_file_from_path (EtBrowser *self, GtkTreePath *path)
{
    EtBrowserPrivate *priv;
    GtkTreeIter iter;

    g_return_val_if_fail (ET_BROWSER (self), NULL);

    priv = et_browser_get_instance_private (self);

    if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->file_model), &iter,
                                  path))
    {
        return NULL;
    }

    return et_browser_get_et_file_from_iter (self, &iter);
}

/*
 * Get ETFile pointer of a file from a Tree Iter
 */
ET_File *
et_browser_get_et_file_from_iter (EtBrowser *self, GtkTreeIter *iter)
{
    EtBrowserPrivate *priv;
    ET_File *etfile;

    g_return_val_if_fail (ET_BROWSER (self), NULL);

    priv = et_browser_get_instance_private (self);

    gtk_tree_model_get (GTK_TREE_MODEL (priv->file_model), iter,
                        LIST_FILE_POINTER, &etfile, -1);
    return etfile;
}


/*
 * Select the specified file in the list, by its ETFile
 */
void
et_browser_select_file_by_et_file (EtBrowser *self,
                                   const ET_File *file,
                                   gboolean select_it)
{
    GtkTreePath *currentPath = NULL;

    currentPath = et_browser_select_file_by_et_file2 (self, file, select_it,
                                                      NULL);

    if (currentPath)
    {
        gtk_tree_path_free (currentPath);
    }
}
/*
 * Select the specified file in the list, by its ETFile
 *  - startPath : if set : starting path to try increase speed
 *  - returns allocated "currentPath" to free
 */
GtkTreePath *
et_browser_select_file_by_et_file2 (EtBrowser *self,
                                    const ET_File *searchETFile,
                                    gboolean select_it,
                                    GtkTreePath *startPath)
{
    EtBrowserPrivate *priv;
    gint row;
    GtkTreePath *currentPath = NULL;
    GtkTreeIter currentIter;
    ET_File *currentETFile;
    gboolean valid;

    g_return_val_if_fail (searchETFile != NULL, NULL);

    priv = et_browser_get_instance_private (self);

    // If the path is used, we try the next item (to increase speed), as it is correct in many cases...
    if (startPath)
    {
        // Try the next path
        gtk_tree_path_next(startPath);
        valid = gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->file_model),
                                         &currentIter, startPath);
        if (valid)
        {
            gtk_tree_model_get(GTK_TREE_MODEL(priv->file_model), &currentIter,
                               LIST_FILE_POINTER, &currentETFile, -1);
            // It is the good file?
            if (currentETFile == searchETFile)
            {
                Browser_List_Select_File_By_Iter (self, &currentIter,
                                                  select_it);
                return startPath;
            }
        }
    }

    // Else, we try the whole list...
    // Go through the file list until it is found
    currentPath = gtk_tree_path_new_first();
    for (row=0; row < gtk_tree_model_iter_n_children(GTK_TREE_MODEL(priv->file_model), NULL); row++)
    {
        valid = gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->file_model), &currentIter, currentPath);
        if (valid)
        {
            gtk_tree_model_get(GTK_TREE_MODEL(priv->file_model), &currentIter,
                               LIST_FILE_POINTER, &currentETFile, -1);

            if (currentETFile == searchETFile)
            {
                Browser_List_Select_File_By_Iter (self, &currentIter,
                                                  select_it);
                return currentPath;
                //break;
            }
        }
        gtk_tree_path_next(currentPath);
    }
    gtk_tree_path_free(currentPath);

    return NULL;
}


/*
 * Select the specified file in the list, by an iter
 */
static void
Browser_List_Select_File_By_Iter (EtBrowser *self,
                                  GtkTreeIter *rowIter,
                                  gboolean select_it)
{
    EtBrowserPrivate *priv;

    priv = et_browser_get_instance_private (self);

    if (select_it)
    {
        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->file_view));

        if (selection)
        {
            g_signal_handler_block (selection, priv->file_selected_handler);
            gtk_tree_selection_select_iter(selection, rowIter);
            g_signal_handler_unblock (selection, priv->file_selected_handler);
        }
    }
    et_browser_set_row_visible (self, rowIter);
}

/*
 * Select the specified file in the list, by a string representation of an iter
 * e.g. output of gtk_tree_model_get_string_from_iter()
 */
void
et_browser_select_file_by_iter_string (EtBrowser *self,
                                       const gchar* stringIter,
                                       gboolean select_it)
{
    EtBrowserPrivate *priv;
    GtkTreeIter iter;

    priv = et_browser_get_instance_private (self);

    g_return_if_fail (priv->file_model != NULL || priv->file_view != NULL);

    if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(priv->file_model), &iter, stringIter))
    {
        if (select_it)
        {
            GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->file_view));

            // FIX ME : Why signal was blocked if selected? Don't remember...
            if (selection)
            {
                //g_signal_handlers_block_by_func(G_OBJECT(selection),G_CALLBACK(Browser_List_Row_Selected),NULL);
                gtk_tree_selection_select_iter(selection, &iter);
                //g_signal_handlers_unblock_by_func(G_OBJECT(selection),G_CALLBACK(Browser_List_Row_Selected),NULL);
            }
        }
        et_browser_set_row_visible (self, &iter);
    }
}

/*
 * Select the specified file in the list, by fuzzy string matching based on
 * the Damerau-Levenshtein Metric (patch from Santtu Lakkala - 23/08/2004)
 */
ET_File *
et_browser_select_file_by_dlm (EtBrowser *self,
                               const gchar* string,
                               gboolean select_it)
{
    EtBrowserPrivate *priv;
    GtkTreeIter iter;
    GtkTreeIter iter2;
    GtkTreeSelection *selection;
    ET_File *current_etfile = NULL, *retval = NULL;
    gchar *current_filename = NULL, *current_title = NULL;
    int max = 0, this;

    priv = et_browser_get_instance_private (self);

    g_return_val_if_fail (priv->file_model != NULL || priv->file_view != NULL, NULL);

    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->file_model),
                                       &iter))
    {
        do
        {
            gtk_tree_model_get(GTK_TREE_MODEL(priv->file_model), &iter,
                               LIST_FILE_NAME,    &current_filename,
                               LIST_FILE_POINTER, &current_etfile, -1);
            current_title = ((File_Tag *)current_etfile->FileTag->data)->title;

            if ((this = dlm((current_title ? current_title : current_filename), string)) > max) // See "dlm.c"
            {
                max = this;
                iter2 = iter;
                retval = current_etfile;
            }

            g_free (current_filename);
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->file_model), &iter));

        if (select_it)
        {
            selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->file_view));
            if (selection)
            {
                g_signal_handler_block (selection,
                                        priv->file_selected_handler);
                gtk_tree_selection_select_iter(selection, &iter2);
                g_signal_handler_unblock (selection,
                                          priv->file_selected_handler);
            }
        }
        et_browser_set_row_visible (self, &iter2);
    }
    return retval;
}

/*
 * Clear all entries on the file list
 */
void
et_browser_clear (EtBrowser *self)
{
    g_return_if_fail (ET_BROWSER (self));

    et_browser_clear_file_model (self);
    et_browser_clear_artist_model (self);
    et_browser_clear_album_model (self);
}

/*
 * Refresh the list sorting (call me after sort-mode has changed)
 */
void
et_browser_refresh_sort (EtBrowser *self)
{
    EtBrowserPrivate *priv;

    g_return_if_fail (ET_BROWSER (self));

    priv = et_browser_get_instance_private (self);

    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (priv->file_model), 0,
                                     Browser_List_Sort_Func, NULL, NULL);
}

/*
 * Intelligently sort the file list based on the current sorting method
 * see also 'ET_Sort_File_List'
 */
static gint
Browser_List_Sort_Func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,
                        gpointer data)
{
    ET_File *ETFile1;
    ET_File *ETFile2;
    gint result = 0;

    gtk_tree_model_get(model, a, LIST_FILE_POINTER, &ETFile1, -1);
    gtk_tree_model_get(model, b, LIST_FILE_POINTER, &ETFile2, -1);

    switch (g_settings_get_enum (MainSettings, "sort-mode"))
    {
        case ET_SORT_MODE_ASCENDING_FILENAME:
            result = ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_FILENAME:
            result = ET_Comp_Func_Sort_File_By_Descending_Filename(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_TITLE:
            result = ET_Comp_Func_Sort_File_By_Ascending_Title(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_TITLE:
            result = ET_Comp_Func_Sort_File_By_Descending_Title(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_ARTIST:
            result = ET_Comp_Func_Sort_File_By_Ascending_Artist(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_ARTIST:
            result = ET_Comp_Func_Sort_File_By_Descending_Artist(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_ALBUM_ARTIST:
            result = ET_Comp_Func_Sort_File_By_Ascending_Album_Artist(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_ALBUM_ARTIST:
            result = ET_Comp_Func_Sort_File_By_Descending_Album_Artist(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_ALBUM:
            result = ET_Comp_Func_Sort_File_By_Ascending_Album(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_ALBUM:
            result = ET_Comp_Func_Sort_File_By_Descending_Album(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_YEAR:
            result = ET_Comp_Func_Sort_File_By_Ascending_Year(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_YEAR:
            result = ET_Comp_Func_Sort_File_By_Descending_Year(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_DISC_NUMBER:
            result = et_comp_func_sort_file_by_ascending_disc_number (ETFile1,
                                                                      ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_DISC_NUMBER:
            result = et_comp_func_sort_file_by_descending_disc_number (ETFile1,
                                                                       ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_TRACK_NUMBER:
            result = ET_Comp_Func_Sort_File_By_Ascending_Track_Number (ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_TRACK_NUMBER:
            result = ET_Comp_Func_Sort_File_By_Descending_Track_Number (ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_GENRE:
            result = ET_Comp_Func_Sort_File_By_Ascending_Genre(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_GENRE:
            result = ET_Comp_Func_Sort_File_By_Descending_Genre(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_COMMENT:
            result = ET_Comp_Func_Sort_File_By_Ascending_Comment(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_COMMENT:
            result = ET_Comp_Func_Sort_File_By_Descending_Comment(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_COMPOSER:
            result = ET_Comp_Func_Sort_File_By_Ascending_Composer(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_COMPOSER:
            result = ET_Comp_Func_Sort_File_By_Descending_Composer(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_ORIG_ARTIST:
            result = ET_Comp_Func_Sort_File_By_Ascending_Orig_Artist(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_ORIG_ARTIST:
            result = ET_Comp_Func_Sort_File_By_Descending_Orig_Artist(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_COPYRIGHT:
            result = ET_Comp_Func_Sort_File_By_Ascending_Copyright(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_COPYRIGHT:
            result = ET_Comp_Func_Sort_File_By_Descending_Copyright(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_URL:
            result = ET_Comp_Func_Sort_File_By_Ascending_Url(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_URL:
            result = ET_Comp_Func_Sort_File_By_Descending_Url(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_ENCODED_BY:
            result = ET_Comp_Func_Sort_File_By_Ascending_Encoded_By(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_ENCODED_BY:
            result = ET_Comp_Func_Sort_File_By_Descending_Encoded_By(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_CREATION_DATE:
            result = ET_Comp_Func_Sort_File_By_Ascending_Creation_Date (ETFile1,
                                                                        ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_CREATION_DATE:
            result = ET_Comp_Func_Sort_File_By_Descending_Creation_Date (ETFile1,
                                                                         ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_FILE_TYPE:
            result = ET_Comp_Func_Sort_File_By_Ascending_File_Type(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_FILE_TYPE:
            result = ET_Comp_Func_Sort_File_By_Descending_File_Type(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_FILE_SIZE:
            result = ET_Comp_Func_Sort_File_By_Ascending_File_Size(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_FILE_SIZE:
            result = ET_Comp_Func_Sort_File_By_Descending_File_Size(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_FILE_DURATION:
            result = ET_Comp_Func_Sort_File_By_Ascending_File_Duration(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_FILE_DURATION:
            result = ET_Comp_Func_Sort_File_By_Descending_File_Duration(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_FILE_BITRATE:
            result = ET_Comp_Func_Sort_File_By_Ascending_File_Bitrate(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_FILE_BITRATE:
            result = ET_Comp_Func_Sort_File_By_Descending_File_Bitrate(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_ASCENDING_FILE_SAMPLERATE:
            result = ET_Comp_Func_Sort_File_By_Ascending_File_Samplerate(ETFile1, ETFile2);
            break;
        case ET_SORT_MODE_DESCENDING_FILE_SAMPLERATE:
            result = ET_Comp_Func_Sort_File_By_Descending_File_Samplerate(ETFile1, ETFile2);
            break;
    }

    return result;
}

/*
 * Select all files on the file list
 */
void
et_browser_select_all (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    GtkTreeSelection *selection;

    g_return_if_fail (ET_BROWSER (self));

    priv = et_browser_get_instance_private (self);
    selection = et_browser_get_selection (self);

    if (selection)
    {
        /* Must block the select signal to avoid the selecting, one by one, of
         * all files in the main files list. */
        g_signal_handler_block (selection, priv->file_selected_handler);
        gtk_tree_selection_select_all(selection);
        g_signal_handler_unblock (selection, priv->file_selected_handler);
    }
}

/*
 * Unselect all files on the file list
 */
void
et_browser_unselect_all (EtBrowser *self)
{
    GtkTreeSelection *selection;

    selection = et_browser_get_selection (self);

    if (selection)
    {
        gtk_tree_selection_unselect_all (selection);
    }
}

/*
 * Invert the selection of the file list
 */
void
et_browser_invert_selection (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    gboolean valid;

    priv = et_browser_get_instance_private (self);

    g_return_if_fail (priv->file_model != NULL || priv->file_view != NULL);

    selection = et_browser_get_selection (self);
    if (selection)
    {
        /* Must block the select signal to avoid selecting all files (one by
         * one) in the main files list. */
        g_signal_handler_block (selection, priv->file_selected_handler);
        valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->file_model), &iter);
        while (valid)
        {
            if (gtk_tree_selection_iter_is_selected(selection, &iter))
            {
                gtk_tree_selection_unselect_iter(selection, &iter);
            } else
            {
                gtk_tree_selection_select_iter(selection, &iter);
            }
            valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->file_model), &iter);
        }
        g_signal_handler_unblock (selection, priv->file_selected_handler);
    }
}

void
et_browser_clear_artist_model (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    GtkTreeSelection *selection;

    /* Empty Model, Disable Browser_Artist_List_Row_Selected() during clear
     * because it may be called and may crash.
    */

    priv = et_browser_get_instance_private (self);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->artist_list));

    g_signal_handler_block (selection, priv->artist_selected_handler);

    gtk_list_store_clear (priv->artist_model);

    g_signal_handler_unblock (selection, priv->artist_selected_handler);
}

static void
Browser_Artist_List_Load_Files (EtBrowser *self, ET_File *etfile_to_select)
{
    EtBrowserPrivate *priv;
    GList *AlbumList;
    GList *etfilelist;
    ET_File *etfile;
    GList *l;
    GList *m;
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    gchar *artistname, *artist_to_select = NULL;

    priv = et_browser_get_instance_private (self);

    g_return_if_fail (priv->artist_list != NULL);

    if (etfile_to_select)
        artist_to_select = ((File_Tag *)etfile_to_select->FileTag->data)->artist;

    et_browser_clear_artist_model (self);
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->artist_list));

    for (l = ETCore->ETArtistAlbumFileList; l != NULL; l = g_list_next (l))
    {
        gint   nbr_files = 0;
        GdkPixbuf* pixbuf;

        // Insert a line for each artist
        AlbumList = (GList *)l->data;
        etfilelist = (GList *)AlbumList->data;
        etfile     = (ET_File *)etfilelist->data;
        artistname = ((File_Tag *)etfile->FileTag->data)->artist;

        // Third column text : number of files
        for (m = g_list_first (AlbumList); m != NULL; m = g_list_next (m))
        {
            nbr_files += g_list_length (g_list_first ((GList *)m->data));
        }

        /* Add the new row. */
        pixbuf = gdk_pixbuf_new_from_resource ("/org/gnome/EasyTAG/images/artist.png",
                                               NULL);
        gtk_list_store_insert_with_values (priv->artist_model, &iter, G_MAXINT,
                                           ARTIST_PIXBUF, pixbuf,
                                           ARTIST_NAME, artistname,
                                           ARTIST_NUM_ALBUMS,
                                           g_list_length (g_list_first (AlbumList)),
                                           ARTIST_NUM_FILES, nbr_files,
                                           ARTIST_ALBUM_LIST_POINTER,
                                           AlbumList, -1);

        g_object_unref (pixbuf);

        // Todo: Use something better than string comparison
        if ( (!artistname && !artist_to_select)
        ||   (artistname  &&  artist_to_select && strcmp(artistname,artist_to_select) == 0) )
        {
            GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(priv->artist_model), &iter);

            g_signal_handler_block (selection, priv->artist_selected_handler);
            gtk_tree_selection_select_iter(selection, &iter);
            g_signal_handler_unblock (selection, priv->artist_selected_handler);

            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(priv->artist_list), path, NULL, FALSE, 0, 0);
            gtk_tree_path_free(path);

            Browser_Album_List_Load_Files (self, AlbumList, etfile_to_select);

            // Now that we've found the artist, no need to continue searching
            artist_to_select = NULL;
        }

        // Set color of the row
        Browser_Artist_List_Set_Row_Appearance (self, &iter);
    }

    // Select the first line if we weren't asked to select anything
    if (!etfile_to_select && gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->artist_model), &iter))
    {
        gtk_tree_model_get(GTK_TREE_MODEL(priv->artist_model), &iter,
                           ARTIST_ALBUM_LIST_POINTER, &AlbumList,
                           -1);
        ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);
        Browser_Album_List_Load_Files (self, AlbumList,NULL);
    }
}


/*
 * Callback to select-row event
 */
static void
Browser_Artist_List_Row_Selected (EtBrowser *self, GtkTreeSelection* selection)
{
    EtBrowserPrivate *priv;
    GList *AlbumList;
    GtkTreeIter iter;

    priv = et_browser_get_instance_private (self);

    // Display the relevant albums
    if(!gtk_tree_selection_get_selected(selection, NULL, &iter))
        return; // We might be called with no row selected

    // Save the current displayed data
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    gtk_tree_model_get (GTK_TREE_MODEL (priv->artist_model), &iter,
                        ARTIST_ALBUM_LIST_POINTER, &AlbumList, -1);
    Browser_Album_List_Load_Files (self, AlbumList, NULL);
}

/*
 * Set the color of the row of priv->artist_list
 */
static void
Browser_Artist_List_Set_Row_Appearance (EtBrowser *self, GtkTreeIter *iter)
{
    EtBrowserPrivate *priv;
    GList *l;
    GList *m;
    gboolean not_all_saved = FALSE;

    priv = et_browser_get_instance_private (self);

    // Change the style (red/bold) of the row if one of the files was changed
    for (gtk_tree_model_get (GTK_TREE_MODEL (priv->artist_model), iter,
                             ARTIST_ALBUM_LIST_POINTER, &l, -1);
         l != NULL; l = g_list_next (l))
    {
        for (m = (GList *)l->data; m != NULL; m = g_list_next (m))
        {
            if (ET_Check_If_File_Is_Saved ((ET_File *)m->data) == FALSE)
            {
                if (g_settings_get_boolean (MainSettings, "file-changed-bold"))
                {
                    // Set the font-style to "bold"
                    gtk_list_store_set(priv->artist_model, iter,
                                       ARTIST_FONT_WEIGHT, PANGO_WEIGHT_BOLD, -1);
                } else
                {
                    // Set the background-color to "red"
                    gtk_list_store_set(priv->artist_model, iter,
                                       ARTIST_FONT_WEIGHT,    PANGO_WEIGHT_NORMAL,
                                       ARTIST_ROW_FOREGROUND, &RED, -1);
                }
                not_all_saved = TRUE;
                break;
            }
        }
    }

    // Reset style if all files saved
    if (not_all_saved == FALSE)
    {
        gtk_list_store_set(priv->artist_model, iter,
                           ARTIST_FONT_WEIGHT,    PANGO_WEIGHT_NORMAL,
                           ARTIST_ROW_FOREGROUND, NULL, -1);
    }
}

void
et_browser_clear_album_model (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    GtkTreeSelection *selection;
    gboolean valid;
    GtkTreeIter iter;

    g_return_if_fail (ET_BROWSER (self));

    priv = et_browser_get_instance_private (self);

    /* Free the attached list in the "all albums" row. */
    valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->album_model),
                                           &iter);

    while (valid)
    {
        GList *l;
        gboolean all_albums_row = FALSE;

        gtk_tree_model_get (GTK_TREE_MODEL (priv->album_model), &iter,
                            ALBUM_ETFILE_LIST_POINTER, &l,
                            ALBUM_ALL_ALBUMS_ROW, &all_albums_row, -1);

        if (all_albums_row && l)
        {
            g_list_free (l);
            break;
        }

        valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->album_model),
                                          &iter);
    }

    /* Empty model, disable Browser_Album_List_Row_Selected () during clear
     * because it is called and crashed. */

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->album_list));

    g_signal_handler_block (selection, priv->album_selected_handler);

    gtk_list_store_clear (priv->album_model);

    g_signal_handler_unblock (selection, priv->album_selected_handler);
}

/*
 * Load the list of Albums for each Artist
 */
static void
Browser_Album_List_Load_Files (EtBrowser *self,
                               GList *albumlist,
                               ET_File *etfile_to_select)
{
    EtBrowserPrivate *priv;
    GList *l;
    GList *etfilelist = NULL;
    ET_File *etfile;
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    gchar *albumname, *album_to_select = NULL;

    priv = et_browser_get_instance_private (self);

    g_return_if_fail (priv->album_list != NULL);

    if (etfile_to_select)
        album_to_select = ((File_Tag *)etfile_to_select->FileTag->data)->album;

    et_browser_clear_album_model (self);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->album_list));

    // Create a first row to select all albums of the artist
    for (l = albumlist; l != NULL; l = g_list_next (l))
    {
        GList *etfilelist_tmp;

        etfilelist_tmp = (GList *)l->data;
        // We must make a copy to not "alter" the initial list by appending another list
        etfilelist_tmp = g_list_copy(etfilelist_tmp);
        etfilelist = g_list_concat(etfilelist, etfilelist_tmp);
    }

    gtk_list_store_insert_with_values (priv->album_model, &iter, G_MAXINT,
                                       ALBUM_NAME, _("<All albums>"),
                                       ALBUM_NUM_FILES,
                                       g_list_length (g_list_first (etfilelist)),
                                       ALBUM_ETFILE_LIST_POINTER, etfilelist,
                                       ALBUM_ALL_ALBUMS_ROW, TRUE,
                                       -1);

    // Create a line for each album of the artist
    for (l = albumlist; l != NULL; l = g_list_next (l))
    {
        GIcon *icon;

        // Insert a line for each album
        etfilelist = (GList *)l->data;
        etfile     = (ET_File *)etfilelist->data;
        albumname  = ((File_Tag *)etfile->FileTag->data)->album;

        /* TODO: Make the icon use the symbolic variant. */
        icon = g_themed_icon_new_with_default_fallbacks ("media-optical-cd-audio");

        /* Add the new row. */
        gtk_list_store_insert_with_values (priv->album_model, &iter, G_MAXINT,
                                           ALBUM_GICON, icon,
                                           ALBUM_NAME, albumname,
                                           ALBUM_NUM_FILES,
                                           g_list_length (g_list_first (etfilelist)),
                                           ALBUM_ETFILE_LIST_POINTER,
                                           etfilelist,
                                           ALBUM_ALL_ALBUMS_ROW, FALSE, -1);

        g_object_unref (icon);

        if ( (!albumname && !album_to_select)
        ||   (albumname &&  album_to_select && strcmp(albumname,album_to_select) == 0) )
        {
            GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(priv->album_model), &iter);

            g_signal_handler_block (selection, priv->album_selected_handler);
            gtk_tree_selection_select_iter(selection, &iter);
            g_signal_handler_unblock (selection, priv->album_selected_handler);

            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(priv->album_list), path, NULL, FALSE, 0, 0);
            gtk_tree_path_free(path);

            ET_Set_Displayed_File_List(etfilelist);
            et_browser_load_file_list (self, etfilelist, etfile_to_select);

            // Now that we've found the album, no need to continue searching
            album_to_select = NULL;
        }

        // Set color of the row
        Browser_Album_List_Set_Row_Appearance (self, &iter);
    }

    // Select the first line if we werent asked to select anything
    if (!etfile_to_select && gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->album_model), &iter))
    {
        gtk_tree_model_get(GTK_TREE_MODEL(priv->album_model), &iter,
                           ALBUM_ETFILE_LIST_POINTER, &etfilelist,
                           -1);
        ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

        // Set the attached list as "Displayed List"
        ET_Set_Displayed_File_List(etfilelist);
        et_browser_load_file_list (self, etfilelist, NULL);

        /* Displays the first item. */
        et_application_window_select_file_by_et_file (ET_APPLICATION_WINDOW (MainWindow),
                                                      (ET_File *)etfilelist->data);
    }
}

/*
 * Callback to select-row event
 */
static void
Browser_Album_List_Row_Selected (EtBrowser *self, GtkTreeSelection *selection)
{
    EtBrowserPrivate *priv;
    GList *etfilelist;
    GtkTreeIter iter;

    priv = et_browser_get_instance_private (self);

    // We might be called with no rows selected
    if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
        return;

    gtk_tree_model_get (GTK_TREE_MODEL (priv->album_model), &iter,
                       ALBUM_ETFILE_LIST_POINTER, &etfilelist, -1);

    // Save the current displayed data
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    // Set the attached list as "Displayed List"
    ET_Set_Displayed_File_List(etfilelist);

    et_browser_load_file_list (self, etfilelist, NULL);

    /* Displays the first item. */
    et_application_window_select_file_by_et_file (ET_APPLICATION_WINDOW (MainWindow),
                                                  (ET_File *)etfilelist->data);
}

/*
 * Set the color of the row of priv->album_list
 */
static void
Browser_Album_List_Set_Row_Appearance (EtBrowser *self, GtkTreeIter *iter)
{
    EtBrowserPrivate *priv;
    GList *l;
    gboolean not_all_saved = FALSE;

    priv = et_browser_get_instance_private (self);

    // Change the style (red/bold) of the row if one of the files was changed
    for (gtk_tree_model_get (GTK_TREE_MODEL (priv->album_model), iter,
                             ALBUM_ETFILE_LIST_POINTER, &l, -1);
         l != NULL; l = g_list_next (l))
    {
        if (ET_Check_If_File_Is_Saved ((ET_File *)l->data) == FALSE)
        {
            if (g_settings_get_boolean (MainSettings, "file-changed-bold"))
            {
                // Set the font-style to "bold"
                gtk_list_store_set(priv->album_model, iter,
                                   ALBUM_FONT_WEIGHT, PANGO_WEIGHT_BOLD, -1);
            } else
            {
                // Set the background-color to "red"
                gtk_list_store_set(priv->album_model, iter,
                                   ALBUM_FONT_WEIGHT,    PANGO_WEIGHT_NORMAL,
                                   ALBUM_ROW_FOREGROUND, &RED, -1);
            }
            not_all_saved = TRUE;
            break;
        }
    }

    // Reset style if all files saved
    if (not_all_saved == FALSE)
    {
        gtk_list_store_set(priv->album_model, iter,
                           ALBUM_FONT_WEIGHT,    PANGO_WEIGHT_NORMAL,
                           ALBUM_ROW_FOREGROUND, NULL, -1);
    }
}

void
et_browser_set_display_mode (EtBrowser *self,
                             EtBrowserMode mode)
{
    EtBrowserPrivate *priv;
    ET_File *etfile = ETCore->ETFileDisplayed; // ETFile to display again after changing browser view

    g_return_if_fail (ET_BROWSER (self));

    priv = et_browser_get_instance_private (self);

    /* Save the current displayed data. */
    ET_Save_File_Data_From_UI (ETCore->ETFileDisplayed);

    switch (mode)
    {
        case ET_BROWSER_MODE_FILE:
            /* Set the whole list as "Displayed list". */
            ET_Set_Displayed_File_List (ETCore->ETFileList);

            /* Display Tree Browser. */
            gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), 0);
            et_browser_load_file_list (self, ETCore->ETFileDisplayedList,
                                       etfile);

            /* Displays the first file if nothing specified. */
            if (!etfile)
            {
                GList *etfilelist = ET_Displayed_File_List_First ();
                if (etfilelist)
                {
                    etfile = (ET_File *)etfilelist->data;
                }

                et_application_window_select_file_by_et_file (ET_APPLICATION_WINDOW (MainWindow),
                                                              etfile);
            }
            break;
        case ET_BROWSER_MODE_ARTIST:
            /* Display Artist + Album lists. */
            gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), 1);
            ET_Create_Artist_Album_File_List ();
            Browser_Artist_List_Load_Files (self, etfile);
            break;
        default:
            g_assert_not_reached ();
    }
    //ET_Display_File_Data_To_UI(etfile); // Causes a crash
}

/*
 * Disable (FALSE) / Enable (TRUE) all user widgets in the browser area (Tree + List + Entry)
 */
void
et_browser_set_sensitive (EtBrowser *self, gboolean sensitive)
{
    EtBrowserPrivate *priv;

    priv = et_browser_get_instance_private (self);

    gtk_widget_set_sensitive (GTK_WIDGET (priv->entry_combo), sensitive);
    gtk_widget_set_sensitive (GTK_WIDGET (priv->tree), sensitive);
    gtk_widget_set_sensitive (GTK_WIDGET (priv->file_view), sensitive);
    gtk_widget_set_sensitive (GTK_WIDGET (priv->artist_list), sensitive);
    gtk_widget_set_sensitive (GTK_WIDGET (priv->album_list), sensitive);
    gtk_widget_set_sensitive (GTK_WIDGET (priv->button), sensitive);
    gtk_widget_set_sensitive (GTK_WIDGET (priv->label), sensitive);
}

static void
do_popup_menu (EtBrowser *self,
               GdkEventButton *event,
               GtkTreeView *view,
               GtkWidget *menu)
{
    gint button;
    gint event_time;

    if (event)
    {
        button = event->button;
        event_time = event->time;
    }
    else
    {
        button = 0;
        event_time = gtk_get_current_event_time ();
    }

    /* TODO: Add popup positioning function. */
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, button,
                    event_time);
}

static void
select_row_for_button_press_event (GtkTreeView *treeview,
                                   GdkEventButton *event)
{
    if (event->window == gtk_tree_view_get_bin_window (treeview))
    {
        GtkTreePath *tree_path;

        if (gtk_tree_view_get_path_at_pos (treeview, event->x,
                                           event->y, &tree_path, NULL,
                                           NULL,NULL))
        {
            gtk_tree_selection_select_path (gtk_tree_view_get_selection (treeview),
                                            tree_path);
            gtk_tree_path_free (tree_path);
        }
    }
}

static gboolean
on_album_tree_popup_menu (GtkWidget *treeview,
                          EtBrowser *self)
{
    EtBrowserPrivate *priv;

    priv = et_browser_get_instance_private (self);

    do_popup_menu (self, NULL, GTK_TREE_VIEW (treeview), priv->album_menu);

    return GDK_EVENT_STOP;
}

static gboolean
on_artist_tree_popup_menu (GtkWidget *treeview,
                           EtBrowser *self)
{
    EtBrowserPrivate *priv;

    priv = et_browser_get_instance_private (self);

    do_popup_menu (self, NULL, GTK_TREE_VIEW (treeview), priv->artist_menu);

    return GDK_EVENT_STOP;
}

static gboolean
on_directory_tree_popup_menu (GtkWidget *treeview,
                              EtBrowser *self)
{
    EtBrowserPrivate *priv;

    priv = et_browser_get_instance_private (self);

    do_popup_menu (self, NULL, GTK_TREE_VIEW (treeview), priv->tree_menu);

    return GDK_EVENT_STOP;
}

static gboolean
on_file_tree_popup_menu (GtkWidget *treeview,
                         EtBrowser *self)
{
    EtBrowserPrivate *priv;

    priv = et_browser_get_instance_private (self);

    do_popup_menu (self, NULL, GTK_TREE_VIEW (treeview), priv->file_menu);

    return GDK_EVENT_STOP;
}

static gboolean
on_album_tree_button_press_event (GtkWidget *widget,
                                  GdkEventButton *event,
                                  EtBrowser *self)
{
    if (gdk_event_triggers_context_menu ((GdkEvent *)event))
    {
        EtBrowserPrivate *priv;

        priv = et_browser_get_instance_private (self);

        if (GTK_IS_TREE_VIEW (widget))
        {
            select_row_for_button_press_event (GTK_TREE_VIEW (widget), event);
        }

        do_popup_menu (self, event, GTK_TREE_VIEW (priv->album_list),
                       priv->album_menu);

        return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
}

static gboolean
on_artist_tree_button_press_event (GtkWidget *widget,
                                   GdkEventButton *event,
                                   EtBrowser *self)
{
    if (gdk_event_triggers_context_menu ((GdkEvent *)event))
    {
        EtBrowserPrivate *priv;

        priv = et_browser_get_instance_private (self);

        if (GTK_IS_TREE_VIEW (widget))
        {
            select_row_for_button_press_event (GTK_TREE_VIEW (widget), event);
        }

        do_popup_menu (self, event, GTK_TREE_VIEW (priv->artist_list),
                       priv->artist_menu);

        return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
}

static gboolean
on_directory_tree_button_press_event (GtkWidget *widget,
                                      GdkEventButton *event,
                                      EtBrowser *self)
{
    if (gdk_event_triggers_context_menu ((GdkEvent *)event))
    {
        EtBrowserPrivate *priv;

        priv = et_browser_get_instance_private (self);

        if (GTK_IS_TREE_VIEW (widget))
        {
            select_row_for_button_press_event (GTK_TREE_VIEW (widget), event);
        }

        do_popup_menu (self, event, GTK_TREE_VIEW (priv->tree),
                       priv->tree_menu);

        return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
}

/*
 * Browser_Popup_Menu_Handler : displays the corresponding menu
 */
static gboolean
on_file_tree_button_press_event (GtkWidget *widget,
                                 GdkEventButton *event,
                                 EtBrowser *self)
{
    if (gdk_event_triggers_context_menu ((GdkEvent *)event))
    {
        EtBrowserPrivate *priv;

        priv = et_browser_get_instance_private (self);

        if (GTK_IS_TREE_VIEW (widget))
        {
            select_row_for_button_press_event (GTK_TREE_VIEW (widget), event);
        }

        do_popup_menu (self, event, GTK_TREE_VIEW (priv->file_view),
                       priv->file_menu);

        return GDK_EVENT_STOP;
    }
    else if (event->type == GDK_2BUTTON_PRESS
        && event->button == GDK_BUTTON_PRIMARY)
    {
        /* Double left mouse click. Select files of the same directory (useful
         * when browsing sub-directories). */
        GdkWindow *bin_window;
        GList *l;
        gchar *path_ref = NULL;
        gchar *patch_check = NULL;
        GtkTreePath *currentPath = NULL;

        if (!ETCore->ETFileDisplayed)
        {
            return GDK_EVENT_PROPAGATE;
        }

        bin_window = gtk_tree_view_get_bin_window (GTK_TREE_VIEW (widget));

        if (bin_window != event->window)
        {
            /* If the double-click is not on a tree view row, for example when
             * resizing a header column, ignore it. */
            return GDK_EVENT_PROPAGATE;
        }

        /* File taken as reference. */
        path_ref = g_path_get_dirname (((File_Name *)ETCore->ETFileDisplayed->FileNameCur->data)->value);

        /* Search and select files of the same directory. */
        for (l = g_list_first (ETCore->ETFileDisplayedList); l != NULL;
             l = g_list_next (l))
        {
            /* Path of the file to check if it is in the same directory. */
            patch_check = g_path_get_dirname (((File_Name *)((ET_File *)l->data)->FileNameCur->data)->value);

            if (path_ref && patch_check && strcmp (path_ref, patch_check) == 0)
            {
                /* Use of 'currentPath' to try to increase speed. Indeed, in
                 * many cases, the next file to select, is the next in the
                 * list. */
                currentPath = et_browser_select_file_by_et_file2 (self,
                                                                  (ET_File *)l->data,
                                                                  TRUE,
                                                                  currentPath);
            }

            g_free (patch_check);
        }

        g_free (path_ref);

        if (currentPath)
        {
            gtk_tree_path_free (currentPath);
        }

        return GDK_EVENT_STOP;
    }
    else if (event->type == GDK_3BUTTON_PRESS
             && event->button == GDK_BUTTON_PRIMARY)
    {
        /* Triple left mouse click, select all files of the list. */
        g_action_group_activate_action (G_ACTION_GROUP (MainWindow),
                                        "select-all", NULL);
        return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
}

/*
 * Destroy the whole tree up to the root node
 */
static void
Browser_Tree_Initialize (EtBrowser *self)
{
    EtBrowserPrivate *priv;
#ifdef G_OS_WIN32
    GVolumeMonitor *monitor;
    GList *mounts;
    GList *l;
#endif
    GtkTreeIter parent_iter;
    GtkTreeIter dummy_iter;
    GIcon *drive_icon;

    priv = et_browser_get_instance_private (self);

    g_return_if_fail (priv->directory_model != NULL);

    gtk_tree_store_clear (priv->directory_model);

#ifdef G_OS_WIN32
    /* TODO: Connect to the monitor changed signals. */
    monitor = g_volume_monitor_get ();
    mounts = g_volume_monitor_get_mounts (monitor);

    for (l = mounts; l != NULL; l = g_list_next (l))
    {
        GMount *mount;
        gchar *name;
        GFile *root;
        gchar *path;

        mount = l->data;
        drive_icon = g_mount_get_icon (mount);
        name = g_mount_get_name (mount);
        root = g_mount_get_root (mount);
        path = g_file_get_path (root);

        gtk_tree_store_insert_with_values (priv->directory_model,
                                           &parent_iter, NULL, G_MAXINT,
                                           TREE_COLUMN_DIR_NAME,
                                           name,
                                           TREE_COLUMN_FULL_PATH,
                                           path,
                                           TREE_COLUMN_HAS_SUBDIR, TRUE,
                                           TREE_COLUMN_SCANNED, FALSE,
                                           TREE_COLUMN_ICON, drive_icon,
                                           -1);
        /* Insert dummy node. */
        gtk_tree_store_append (priv->directory_model, &dummy_iter,
                               &parent_iter);

        g_free (path);
        g_free (name);
        g_object_unref (root);
        g_object_unref (drive_icon);
    }

    g_list_free_full (mounts, g_object_unref);
    g_object_unref (monitor);
#else /* !G_OS_WIN32 */
    drive_icon = get_gicon_for_path (G_DIR_SEPARATOR_S, ET_PATH_STATE_CLOSED);
    gtk_tree_store_insert_with_values (priv->directory_model, &parent_iter, NULL,
                                       G_MAXINT, TREE_COLUMN_DIR_NAME,
                                       G_DIR_SEPARATOR_S,
                                       TREE_COLUMN_FULL_PATH,
                                       G_DIR_SEPARATOR_S,
                                       TREE_COLUMN_HAS_SUBDIR, TRUE,
                                       TREE_COLUMN_SCANNED, FALSE,
                                       TREE_COLUMN_ICON, drive_icon, -1);
    /* Insert dummy node. */
    gtk_tree_store_append (priv->directory_model, &dummy_iter, &parent_iter);

    g_object_unref (drive_icon);
#endif /* !G_OS_WIN32 */
}

/*
 * et_browser_reload: Refresh the tree browser by destroying it and rebuilding it.
 * Opens tree nodes corresponding to the current path.
 */
void
et_browser_reload (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    gchar *current_path = NULL;
    GtkTreeSelection *selection;

    priv = et_browser_get_instance_private (self);

    /* Memorize the current path to load it again at the end */
    current_path = Browser_Tree_Get_Path_Of_Selected_Node (self);

    if (current_path==NULL && priv->entry_combo)
    {
        /* If no node selected, get path from BrowserEntry or default path */
        if (priv->current_path != NULL)
            current_path = g_strdup(priv->current_path);
        else if (g_utf8_strlen(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(priv->entry_combo)))), -1) > 0)
            current_path = filename_from_display(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(priv->entry_combo)))));
        else
        {
            GVariant *path = g_settings_get_value (MainSettings,
                                                   "default-path");
            current_path = g_variant_dup_bytestring (path, NULL);
            g_variant_unref (path);
        }
    }

    /* Select again the memorized path without loading files */
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree));

    if (selection)
    {
        g_signal_handlers_block_by_func (selection,
                                         G_CALLBACK (Browser_Tree_Node_Selected),
                                         self);
        Browser_Tree_Initialize (self);
        et_browser_select_dir (self, current_path);
        g_signal_handlers_unblock_by_func (selection,
                                           G_CALLBACK (Browser_Tree_Node_Selected),
                                           self);
    }
    g_free(current_path);

    et_application_window_update_actions (ET_APPLICATION_WINDOW (MainWindow));
}

/*
 * Renames a directory
 * last_path:
 * new_path:
 * Parameters are non-utf8!
 */
static void
Browser_Tree_Rename_Directory (EtBrowser *self,
                               const gchar *last_path,
                               const gchar *new_path)
{
    EtBrowserPrivate *priv;
    gchar **textsplit;
    gint i;
    GtkTreeIter  iter;
    GtkTreePath *childpath;
    GtkTreePath *parentpath;
    gchar *new_basename;
    gchar *new_basename_utf8;
    gchar *path;

    if (!last_path || !new_path)
        return;

    priv = et_browser_get_instance_private (self);

    /*
     * Find the existing tree entry
     */
    textsplit = g_strsplit(last_path, G_DIR_SEPARATOR_S, 0);

#ifdef G_OS_WIN32
    if (!et_browser_win32_get_drive_root (self, textsplit[0], &iter,
                                          &parentpath))
    {
        return;
    }
#else /* !G_OS_WIN32 */
    parentpath = gtk_tree_path_new_first();
#endif /* !G_OS_WIN32 */

    for (i = 1; textsplit[i] != NULL; i++)
    {
        gboolean valid = gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->directory_model),
                                                  &iter, parentpath);
        if (valid)
        {
            childpath = Find_Child_Node (self, &iter, textsplit[i]);
        }
        else
        {
            childpath = NULL;
        }

        if (childpath == NULL)
        {
            // ERROR! Could not find it!
            gchar *text_utf8 = filename_to_display(textsplit[i]);
            g_critical ("Error: Searching for %s, could not find node %s in tree.",
                        last_path, text_utf8);
            g_strfreev(textsplit);
            g_free(text_utf8);
            return;
        }
        gtk_tree_path_free(parentpath);
        parentpath = childpath;
    }

    gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->directory_model), &iter, parentpath);
    gtk_tree_path_free(parentpath);

    /* Rename the on-screen node */
    new_basename = g_path_get_basename(new_path);
    new_basename_utf8 = filename_to_display(new_basename);
    gtk_tree_store_set(priv->directory_model, &iter,
                       TREE_COLUMN_DIR_NAME,  new_basename_utf8,
                       TREE_COLUMN_FULL_PATH, new_path,
                       -1);

    /* Update fullpath of child nodes */
    Browser_Tree_Handle_Rename (self, &iter, last_path, new_path);

    /* Update the variable of the current path */
    path = Browser_Tree_Get_Path_Of_Selected_Node (self);
    et_browser_set_current_path (self, path);
    g_free(path);

    g_strfreev(textsplit);
    g_free(new_basename);
    g_free(new_basename_utf8);
}

/*
 * Recursive function to update paths of all child nodes
 */
static void
Browser_Tree_Handle_Rename (EtBrowser *self,
                            GtkTreeIter *parentnode,
                            const gchar *old_path,
                            const gchar *new_path)
{
    EtBrowserPrivate *priv;
    GtkTreeIter iter;
    gchar *path;
    gchar *path_shift;
    gchar *path_new;

    priv = et_browser_get_instance_private (self);

    /* If there are no children then nothing needs to be done! */
    if (!gtk_tree_model_iter_children (GTK_TREE_MODEL (priv->directory_model),
                                       &iter, parentnode))
    {
        return;
    }

    do
    {
        gtk_tree_model_get(GTK_TREE_MODEL(priv->directory_model), &iter,
                           TREE_COLUMN_FULL_PATH, &path, -1);
        if(path == NULL)
            continue;

        path_shift = g_utf8_offset_to_pointer(path, g_utf8_strlen(old_path, -1));
        path_new = g_strconcat(new_path, path_shift, NULL);

        gtk_tree_store_set(priv->directory_model, &iter,
                           TREE_COLUMN_FULL_PATH, path_new, -1);

        g_free(path_new);
        g_free(path);

        /* Recurse if necessary. */
        if (gtk_tree_model_iter_has_child (GTK_TREE_MODEL (priv->directory_model),
                                           &iter))
        {
            Browser_Tree_Handle_Rename (self, &iter, old_path, new_path);
        }

    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->directory_model), &iter));

}

/*
 * Find the child node of "parentnode" that has text of "childtext
 * Returns NULL on failure
 */
static GtkTreePath *
Find_Child_Node (EtBrowser *self, GtkTreeIter *parentnode, gchar *childtext)
{
    EtBrowserPrivate *priv;
    gint row;
    GtkTreeIter iter;
    gchar *text;
    gchar *temp;

    priv = et_browser_get_instance_private (self);

    for (row=0; row < gtk_tree_model_iter_n_children(GTK_TREE_MODEL(priv->directory_model), parentnode); row++)
    {
        if (row == 0)
        {
            if (gtk_tree_model_iter_children(GTK_TREE_MODEL(priv->directory_model), &iter, parentnode) == FALSE) return NULL;
        } else
        {
            if (gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->directory_model), &iter) == FALSE)
                return NULL;
        }
        gtk_tree_model_get(GTK_TREE_MODEL(priv->directory_model), &iter,
                           TREE_COLUMN_FULL_PATH, &temp, -1);
        text = g_path_get_basename(temp);
        g_free(temp);
        if(strcmp(childtext,text) == 0)
        {
            g_free(text);
            return gtk_tree_model_get_path(GTK_TREE_MODEL(priv->directory_model), &iter);
        }
        g_free(text);

    }

    return NULL;
}

/*
 * check_for_subdir:
 * @path: (type filename): the path to test
 *
 * Check if @path has any subdirectories.
 *
 * Returns: %TRUE if subdirectories exist, %FALSE otherwise
 */
static gboolean
check_for_subdir (const gchar *path)
{
    GFile *dir;
    GFileEnumerator *enumerator;

    dir = g_file_new_for_path (path);
    enumerator = g_file_enumerate_children (dir,
                                            G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                                            G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN,
                                            G_FILE_QUERY_INFO_NONE,
                                            NULL, NULL);
    g_object_unref (dir);

    if (enumerator)
    {
        GFileInfo *childinfo;

        while ((childinfo = g_file_enumerator_next_file (enumerator,
                                                         NULL, NULL)))
        {
            if ((g_file_info_get_file_type (childinfo) ==
                 G_FILE_TYPE_DIRECTORY) &&
                (g_settings_get_boolean (MainSettings, "browse-show-hidden")
                 || !g_file_info_get_is_hidden (childinfo)))
            {
                g_object_unref (childinfo);
                g_file_enumerator_close (enumerator, NULL, NULL);
                g_object_unref (enumerator);
                return TRUE;
            }

            g_object_unref (childinfo);
        }

        g_file_enumerator_close (enumerator, NULL, NULL);
        g_object_unref (enumerator);
    }

    return FALSE;
}

/*
 * get_gicon_for_path:
 * @path: (type filename): path to create icon for
 * @path_state: whether the icon should be shown open or closed
 *
 * Check the permissions for the supplied @path (authorized?, readonly?,
 * unreadable?) and return an appropriate icon.
 *
 * Returns: an icon corresponding to the @path
 */
static GIcon *
get_gicon_for_path (const gchar *path, EtPathState path_state)
{
    GIcon *folder_icon;
    GIcon *emblem_icon;
    GIcon *emblemed_icon;
    GEmblem *emblem;
    GFile *file;
    GFileInfo *info;
    GError *error = NULL;

    switch (path_state)
    {
        case ET_PATH_STATE_OPEN:
            folder_icon = g_themed_icon_new ("folder-open");
            break;
        case ET_PATH_STATE_CLOSED:
            folder_icon = g_themed_icon_new ("folder");
            break;
        default:
            g_assert_not_reached ();
    }

    file = g_file_new_for_path (path);
    info = g_file_query_info (file, G_FILE_ATTRIBUTE_ACCESS_CAN_READ ","
                              G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,
                              G_FILE_QUERY_INFO_NONE, NULL, &error);

    if (info == NULL)
    {
        g_warning ("Error while querying path information: %s",
                   error->message);
        g_clear_error (&error);
        info = g_file_info_new ();
        g_file_info_set_attribute_boolean (info,
                                           G_FILE_ATTRIBUTE_ACCESS_CAN_READ,
                                           FALSE);
    }

    if (!g_file_info_get_attribute_boolean (info,
                                            G_FILE_ATTRIBUTE_ACCESS_CAN_READ))
    {
        emblem_icon = g_themed_icon_new ("emblem-unreadable");
        emblem = g_emblem_new_with_origin (emblem_icon,
                                           G_EMBLEM_ORIGIN_LIVEMETADATA);
        emblemed_icon = g_emblemed_icon_new (folder_icon, emblem);
        g_object_unref (folder_icon);
        g_object_unref (emblem_icon);
        g_object_unref (emblem);

        folder_icon = emblemed_icon;
    }
    else if (!g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE))
    {
        emblem_icon = g_themed_icon_new ("emblem-readonly");
        emblem = g_emblem_new_with_origin (emblem_icon,
                                           G_EMBLEM_ORIGIN_LIVEMETADATA);
        emblemed_icon = g_emblemed_icon_new (folder_icon, emblem);
        g_object_unref (folder_icon);
        g_object_unref (emblem_icon);
        g_object_unref (emblem);

        folder_icon = emblemed_icon;
    }

    g_object_unref (file);
    g_object_unref (info);

    return folder_icon;
}

/*
 * Open up a node on the browser tree
 * Scanning and showing all subdirectories
 */
static void
expand_cb (EtBrowser *self, GtkTreeIter *iter, GtkTreePath *gtreePath, GtkTreeView *tree)
{
    EtBrowserPrivate *priv;
    GFile *dir;
    GFileEnumerator *enumerator;
    gchar *fullpath_file;
    gchar *parentPath;
    gboolean treeScanned;
    gboolean has_subdir = FALSE;
    GtkTreeIter currentIter;
    GtkTreeIter subNodeIter;
    GIcon *icon;

    priv = et_browser_get_instance_private (self);

    g_return_if_fail (priv->directory_model != NULL);

    gtk_tree_model_get(GTK_TREE_MODEL(priv->directory_model), iter,
                       TREE_COLUMN_FULL_PATH, &parentPath,
                       TREE_COLUMN_SCANNED,   &treeScanned, -1);

    if (treeScanned)
        return;

    dir = g_file_new_for_path (parentPath);
    enumerator = g_file_enumerate_children (dir,
                                            G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                                            G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME ","
                                            G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                            G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN,
                                            G_FILE_QUERY_INFO_NONE,
                                            NULL, NULL);

    if (enumerator)
    {
        GFileInfo *childinfo;

        while ((childinfo = g_file_enumerator_next_file (enumerator,
                                                         NULL, NULL))
               != NULL)
        {
            const gchar *name;
            GFile *child;
            gboolean isdir = FALSE;

            name = g_file_info_get_name (childinfo);
            child = g_file_get_child (dir, name);
            fullpath_file = g_file_get_path (child);
            isdir = g_file_info_get_file_type (childinfo) == G_FILE_TYPE_DIRECTORY;

            if (isdir &&
                (g_settings_get_boolean (MainSettings, "browse-show-hidden")
                 || !g_file_info_get_is_hidden (childinfo)))
            {
                const gchar *dirname_utf8;
                dirname_utf8 = g_file_info_get_display_name (childinfo);

                has_subdir = check_for_subdir (fullpath_file);

                /* Select pixmap according permissions for the directory. */
                icon = get_gicon_for_path (fullpath_file,
                                           ET_PATH_STATE_CLOSED);

                gtk_tree_store_insert_with_values (priv->directory_model,
                                                   &currentIter, iter,
                                                   G_MAXINT,
                                                   TREE_COLUMN_DIR_NAME,
                                                   dirname_utf8,
                                                   TREE_COLUMN_FULL_PATH,
                                                   fullpath_file,
                                                   TREE_COLUMN_HAS_SUBDIR,
                                                   !has_subdir,
                                                   TREE_COLUMN_SCANNED, FALSE,
                                                   TREE_COLUMN_ICON, icon, -1);

                if (has_subdir)
                {
                    /* Insert a dummy node. */
                    gtk_tree_store_append(priv->directory_model, &subNodeIter, &currentIter);
                }

                g_object_unref (icon);
            }

            g_free (fullpath_file);
            g_object_unref (childinfo);
            g_object_unref (child);
        }

        g_file_enumerator_close (enumerator, NULL, NULL);
        g_object_unref (enumerator);

        /* Remove dummy node. */
        gtk_tree_model_iter_children (GTK_TREE_MODEL (priv->directory_model),
                                      &subNodeIter, iter);
        gtk_tree_store_remove (priv->directory_model, &subNodeIter);
    }

    g_object_unref (dir);
    icon = get_gicon_for_path (parentPath, ET_PATH_STATE_OPEN);

#ifdef G_OS_WIN32
    // set open folder pixmap except on drive (depth == 0)
    if (gtk_tree_path_get_depth(gtreePath) > 1)
    {
        // update the icon of the node to opened folder :-)
        gtk_tree_store_set(priv->directory_model, iter,
                           TREE_COLUMN_SCANNED, TRUE,
                           TREE_COLUMN_ICON, icon,
                           -1);
    }
#else /* !G_OS_WIN32 */
    // update the icon of the node to opened folder :-)
    gtk_tree_store_set(priv->directory_model, iter,
                       TREE_COLUMN_SCANNED, TRUE,
                       TREE_COLUMN_ICON, icon,
                       -1);
#endif /* !G_OS_WIN32 */

    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(priv->directory_model),
                                         TREE_COLUMN_DIR_NAME, GTK_SORT_ASCENDING);

    g_object_unref (icon);
    g_free(parentPath);
}

static void
collapse_cb (EtBrowser *self, GtkTreeIter *iter, GtkTreePath *treePath, GtkTreeView *tree)
{
    EtBrowserPrivate *priv;
    GtkTreeIter subNodeIter;
    gchar *path;
    GIcon *icon;
    GFile *file;
    GFileInfo *fileinfo;
    GError *error = NULL;

    priv = et_browser_get_instance_private (self);

    g_return_if_fail (priv->directory_model != NULL);

    gtk_tree_model_get (GTK_TREE_MODEL (priv->directory_model), iter,
                        TREE_COLUMN_FULL_PATH, &path, -1);

    /* If the directory is not readable, do not delete its children. */
    file = g_file_new_for_path (path);
    g_free (path);
    fileinfo = g_file_query_info (file, G_FILE_ATTRIBUTE_ACCESS_CAN_READ,
                                  G_FILE_QUERY_INFO_NONE, NULL, &error);
    g_object_unref (file);

    if (fileinfo)
    {
        if (!g_file_info_get_attribute_boolean (fileinfo,
                                                G_FILE_ATTRIBUTE_ACCESS_CAN_READ))
        {
            g_object_unref (fileinfo);
            return;
        }

        g_object_unref (fileinfo);
    }

    gtk_tree_model_iter_children(GTK_TREE_MODEL(priv->directory_model),
                                 &subNodeIter, iter);
    while (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(priv->directory_model), iter))
    {
        gtk_tree_model_iter_children(GTK_TREE_MODEL(priv->directory_model), &subNodeIter, iter);
        gtk_tree_store_remove(priv->directory_model, &subNodeIter);
    }

    gtk_tree_model_get (GTK_TREE_MODEL (priv->directory_model), iter,
                        TREE_COLUMN_FULL_PATH, &path, -1);
    icon = get_gicon_for_path (path, ET_PATH_STATE_OPEN);
    g_free (path);
#ifdef G_OS_WIN32
    // set closed folder pixmap except on drive (depth == 0)
    if(gtk_tree_path_get_depth(treePath) > 1)
    {
        // update the icon of the node to closed folder :-)
        gtk_tree_store_set(priv->directory_model, iter,
                           TREE_COLUMN_SCANNED, FALSE,
                           TREE_COLUMN_ICON, icon, -1);
    }
#else /* !G_OS_WIN32 */
    // update the icon of the node to closed folder :-)
    gtk_tree_store_set(priv->directory_model, iter,
                       TREE_COLUMN_SCANNED, FALSE,
                       TREE_COLUMN_ICON, icon, -1);
#endif /* !G_OS_WIN32 */

    /* Insert dummy node only if directory exists. */
    if (error)
    {
        /* Remove the parent (missing) directory from the tree. */
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
        {
            gtk_tree_store_remove (priv->directory_model, iter);
        }

        g_error_free (error);
    }
    else
    {
        gtk_tree_store_append (priv->directory_model, &subNodeIter, iter);
    }

    g_object_unref (icon);
}

static void
on_sort_mode_changed (EtBrowser *self, gchar *key, GSettings *settings)
{
    EtBrowserPrivate *priv;
    EtSortMode sort_mode;
    GtkTreeViewColumn *column;

    priv = et_browser_get_instance_private (self);

    sort_mode = g_settings_get_enum (settings, key);
    column = et_browser_get_column_for_column_id (self, sort_mode / 2);

    /* If the column to sort is different than the old sorted column. */
    if (sort_mode / 2 != priv->file_sort_mode / 2)
    {
        GtkTreeViewColumn *old_column;

        old_column = et_browser_get_column_for_column_id (self,
                                                          priv->file_sort_mode / 2);

        /* Reset the sort order of the old sort column. */
        if (gtk_tree_view_column_get_sort_order (old_column)
            == GTK_SORT_DESCENDING)
        {
            gtk_tree_view_column_set_sort_order (old_column,
                                                 GTK_SORT_ASCENDING);
        }

        gtk_tree_view_column_set_sort_indicator (old_column, FALSE);
    }

    /* New sort mode is for a column with a visible counterpart. */
    if (sort_mode / 2 < ET_SORT_MODE_ASCENDING_CREATION_DATE)
    {
        gtk_tree_view_column_set_sort_indicator (column, TRUE);
    }

    /* Even is GTK_SORT_ASCENDING, odd is GTK_SORT_DESCENDING. */
    gtk_tree_view_column_set_sort_order (column, sort_mode % 2);

    /* Store the new sort mode. */
    priv->file_sort_mode = sort_mode;

    et_browser_refresh_sort (self);
}

/*
 * Create item of the browser (Entry + Tree + List).
 */
static void
create_browser (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    GtkWidget *grid;
    gsize i;
    GtkBuilder *builder;
    GError *error = NULL;
    GMenuModel *menu_model;
    const gchar * const ids[] = { "filename_column", "title_column",
                                  "artist_column", "album_artist_column",
                                  "album_column", "year_column", "disc_column",
                                  "track_column", "genre_column",
                                  "comment_column", "composer_column",
                                  "orig_artist_column", "copyright_column",
                                  "url_column", "encoded_by_column" };

    priv = et_browser_get_instance_private (self);

    gtk_container_set_border_width (GTK_CONTAINER (self), 2);

    /* The entry box for displaying path. */
    builder = gtk_builder_new ();
    gtk_builder_add_from_resource (builder, "/org/gnome/EasyTAG/browser.ui",
                                   &error);

    if (error != NULL)
    {
        g_error ("Unable to get browser from resource: %s",
                 error->message);
    }

    grid = GTK_WIDGET (gtk_builder_get_object (builder, "browser_grid"));
    gtk_container_add (GTK_CONTAINER (self), grid);

    priv->entry_model = GTK_LIST_STORE (gtk_builder_get_object (builder,
                                                                "directory_model"));

    priv->entry_combo = GTK_WIDGET (gtk_builder_get_object (builder,
                                                            "browser_combo"));
    /* History list */
    Load_Path_Entry_List (priv->entry_model, MISC_COMBO_TEXT);

    g_signal_connect_swapped (gtk_bin_get_child (GTK_BIN (priv->entry_combo)),
                              "activate", G_CALLBACK (Browser_Entry_Activated),
                              self);

    /* The button to select a directory to browse. */
    priv->button = GTK_WIDGET (gtk_builder_get_object (builder,
                                                       "open_button"));
    g_signal_connect_swapped (priv->button, "clicked",
                              G_CALLBACK (File_Selection_Window_For_Directory),
                              gtk_bin_get_child (GTK_BIN (priv->entry_combo)));

    /* The label for displaying number of files in path (without subdirs). */
    /* Translators: No files, as in "0 files". */
    priv->label = GTK_WIDGET (gtk_builder_get_object (builder, "files_label"));

    /* Browser notebook: one tab for the priv->tree, one tab for the
     * priv->artist_list and the priv->album_list. */
    priv->notebook = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "directory_album_artist_notebook"));

    /* The ScrollWindow and the Directory-Tree. */
    priv->directory_model = GTK_TREE_STORE (gtk_builder_get_object (builder,
                                                                    "tree_model"));

    /* The tree view */
    priv->tree = GTK_WIDGET (gtk_builder_get_object (builder,
                                                     "directory_view"));

    Browser_Tree_Initialize (self);

    /* Signals */
    g_signal_connect_swapped (priv->tree, "row-expanded",
                              G_CALLBACK (expand_cb), self);
    g_signal_connect_swapped (priv->tree, "row-collapsed",
                              G_CALLBACK (collapse_cb), self);
    g_signal_connect_swapped (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree)),
                              "changed",
                              G_CALLBACK (Browser_Tree_Node_Selected), self);

    g_signal_connect (priv->tree, "key-press-event",
                      G_CALLBACK (Browser_Tree_Key_Press), NULL);

    /* Create popup menu on browser tree view. */
    gtk_builder_add_from_resource (builder, "/org/gnome/EasyTAG/menus.ui",
                                   &error);

    if (error != NULL)
    {
        g_error ("Unable to get popup menu from resource: %s",
                 error->message);
    }

    menu_model = G_MENU_MODEL (gtk_builder_get_object (builder,
                                                       "directory-menu"));
    priv->tree_menu = gtk_menu_new_from_model (menu_model);
    gtk_menu_attach_to_widget (GTK_MENU (priv->tree_menu), priv->tree, NULL);
    g_signal_connect (priv->tree, "button-press-event",
                      G_CALLBACK (on_directory_tree_button_press_event), self);
    g_signal_connect (priv->tree, "popup-menu",
                      G_CALLBACK (on_directory_tree_popup_menu), self);

    /* The ScrollWindows with the Artist and Album Lists. */
    priv->artist_model = GTK_LIST_STORE (gtk_builder_get_object (builder,
                                                                 "artist_model"));

    priv->artist_list = GTK_WIDGET (gtk_builder_get_object (builder,
                                                            "artist_view"));
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->artist_list)),
                                                              GTK_SELECTION_SINGLE);
    priv->artist_selected_handler = g_signal_connect_swapped (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->artist_list)),
                                                              "changed",
                                                              G_CALLBACK (Browser_Artist_List_Row_Selected),
                                                              self);

    /* Create popup menu on browser artist list. */
    menu_model = G_MENU_MODEL (gtk_builder_get_object (builder,
                                                       "directory-artist-menu"));
    priv->artist_menu = gtk_menu_new_from_model (menu_model);
    gtk_menu_attach_to_widget (GTK_MENU (priv->artist_menu), priv->artist_list,
                               NULL);
    g_signal_connect (priv->artist_list, "button-press-event",
                      G_CALLBACK (on_artist_tree_button_press_event), self);
    g_signal_connect (priv->artist_list, "popup-menu",
                      G_CALLBACK (on_artist_tree_popup_menu), self);

    priv->album_model = GTK_LIST_STORE (gtk_builder_get_object (builder,
                                                                "album_model"));
    priv->album_list = GTK_WIDGET (gtk_builder_get_object (builder,
                                                           "album_view"));
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->album_list)),
                                 GTK_SELECTION_SINGLE);

    priv->album_selected_handler = g_signal_connect_swapped (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->album_list)),
                                                             "changed",
                                                             G_CALLBACK (Browser_Album_List_Row_Selected),
                                                             self);

    /* Create Popup Menu on browser album list. */
    menu_model = G_MENU_MODEL (gtk_builder_get_object (builder,
                                                       "directory-album-menu"));
    priv->album_menu = gtk_menu_new_from_model (menu_model);
    gtk_menu_attach_to_widget (GTK_MENU (priv->album_menu), priv->album_list,
                               NULL);
    g_signal_connect (priv->album_list, "button-press-event",
                      G_CALLBACK (on_album_tree_button_press_event), self);
    g_signal_connect (priv->album_list, "popup-menu",
                      G_CALLBACK (on_album_tree_popup_menu), self);

    /* The file list */
    priv->file_model = GTK_LIST_STORE (gtk_builder_get_object (builder,
                                                               "files_model"));
    priv->file_view = GTK_WIDGET (gtk_builder_get_object (builder,
                                                          "files_view"));

    /* Add columns to tree view. See ET_FILE_LIST_COLUMN. */
    for (i = 0; i <= LIST_FILE_ENCODED_BY; i++)
    {
        const guint ascending_sort = 2 * i;
        GtkTreeViewColumn *column = GTK_TREE_VIEW_COLUMN (gtk_builder_get_object (builder, ids[i]));

        g_object_set_data (G_OBJECT (column), "browser", self);
        g_signal_connect (column, "clicked",
                          G_CALLBACK (et_browser_on_column_clicked),
                          GINT_TO_POINTER (ascending_sort));
    }

    g_signal_connect_swapped (MainSettings, "changed::sort-mode",
                              G_CALLBACK (on_sort_mode_changed), self);
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->file_view)),
                                 GTK_SELECTION_MULTIPLE);
    // To sort list
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (priv->file_model), 0,
                                      Browser_List_Sort_Func, NULL, NULL);
    et_browser_refresh_sort (self);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (priv->file_model),
                                          0, GTK_SORT_ASCENDING);

    priv->file_selected_handler = g_signal_connect_swapped (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->file_view)),
                                                            "changed",
                                                            G_CALLBACK (Browser_List_Row_Selected),
                                                            self);
    g_signal_connect (priv->file_view, "key-press-event",
                      G_CALLBACK (Browser_List_Key_Press), NULL);

    /* Create popup menu on file list. */
    menu_model = G_MENU_MODEL (gtk_builder_get_object (builder, "file-menu"));
    priv->file_menu = gtk_menu_new_from_model (menu_model);
    gtk_menu_attach_to_widget (GTK_MENU (priv->file_menu), priv->file_view,
                               NULL);
    g_signal_connect (priv->file_view, "button-press-event",
                      G_CALLBACK (on_file_tree_button_press_event), self);
    g_signal_connect (priv->file_view, "popup-menu",
                      G_CALLBACK (on_file_tree_popup_menu), self);

    g_object_unref (builder);

    /* The list store for run program combos. */
    priv->run_program_model = gtk_list_store_new(MISC_COMBO_COUNT, G_TYPE_STRING);

    /* TODO: Give the browser area a sensible default size. */
    gtk_widget_show_all (GTK_WIDGET (self));

    /* Set home variable as current path */
    et_browser_set_current_path (self, g_get_home_dir ());
}

/*
 * et_browser_on_column_clicked:
 * @column: the tree view column to sort
 * @data: the (required) #ET_Sorting_Type, converted to a pointer with
 * #GINT_TO_POINTER
 *
 * Set the sort mode and display appropriate sort indicator when
 * column is clicked.
 */
static void
et_browser_on_column_clicked (GtkTreeViewColumn *column, gpointer data)
{
    /* Flip to a descing sort mode if the row is already sorted ascending. */
    if (gtk_tree_view_column_get_sort_order (column) == GTK_SORT_ASCENDING)
    {
        g_settings_set_enum (MainSettings, "sort-mode",
                             GPOINTER_TO_INT (data) + 1);
    }
    else
    {
        g_settings_set_enum (MainSettings, "sort-mode",
                             GPOINTER_TO_INT (data));
    }
}

/*******************************
 * Scanner To Rename Directory *
 *******************************/
static void
rename_directory_generate_preview (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    gchar *preview_text = NULL;
    gchar *mask = NULL;

    priv = et_browser_get_instance_private (self);

    if (!ETCore->ETFileDisplayed
    ||  !priv->rename_directory_dialog || !priv->rename_directory_mask_entry || !priv->rename_directory_preview_label)
        return;

    mask = g_settings_get_string (MainSettings,
                                  "rename-directory-default-mask");

    if (!mask)
        return;

    preview_text = et_scan_generate_new_filename_from_mask (ETCore->ETFileDisplayed,
                                                            mask, FALSE);

    if (GTK_IS_LABEL(priv->rename_directory_preview_label))
    {
        if (preview_text)
        {
            //gtk_label_set_text(GTK_LABEL(priv->rename_file_preview_label),preview_text);
            gchar *tmp_string = g_markup_printf_escaped("%s",preview_text); // To avoid problem with strings containing characters like '&'
            gchar *str = g_strdup_printf("<i>%s</i>",tmp_string);
            gtk_label_set_markup(GTK_LABEL(priv->rename_directory_preview_label),str);
            g_free(tmp_string);
            g_free(str);
        } else
        {
            gtk_label_set_text(GTK_LABEL(priv->rename_directory_preview_label),"");
        }

        // Force the window to be redrawed else the preview label may be not placed correctly
        gtk_widget_queue_resize(priv->rename_directory_dialog);
    }

    g_free(mask);
    g_free(preview_text);
}

/*
 * The window to Rename a directory into the browser.
 */
void
et_browser_show_rename_directory_dialog (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    GtkBuilder *builder;
    GError *error = NULL;
    GtkWidget *label;
    GtkWidget *button;
    gchar *directory_parent = NULL;
    gchar *directory_name = NULL;
    gchar *directory_name_utf8 = NULL;
    gchar *address = NULL;
    gchar *string;

    priv = et_browser_get_instance_private (self);

    if (priv->rename_directory_dialog != NULL)
    {
        gtk_window_present(GTK_WINDOW(priv->rename_directory_dialog));
        return;
    }

    /* We get the full path but we musn't display the parent directories */
    directory_parent = g_strdup(priv->current_path);
    if (!directory_parent || strlen(directory_parent) == 0)
    {
        g_free(directory_parent);
        return;
    }

    // Remove the last '/' in the path if it exists
    if (strlen(directory_parent)>1 && directory_parent[strlen(directory_parent)-1]==G_DIR_SEPARATOR)
        directory_parent[strlen(directory_parent)-1]=0;
    // Get name of the directory to rename (without path)
    address = strrchr(directory_parent,G_DIR_SEPARATOR);
    if (!address) return;
    directory_name = g_strdup(address+1);
    *(address+1) = 0;

    if (!directory_name || strlen(directory_name)==0)
    {
        g_free(directory_name);
        g_free(directory_parent);
        return;
    }

    directory_name_utf8 = filename_to_display(directory_name);

    builder = gtk_builder_new ();
    gtk_builder_add_from_resource (builder,
                                   "/org/gnome/EasyTAG/browser.ui",
                                   &error);

    if (error != NULL)
    {
        g_error ("Unable to get rename directory dialog from resource: %s",
                 error->message);
    }

    priv->rename_directory_dialog = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                        "rename_directory_dialog"));

    gtk_window_set_transient_for (GTK_WINDOW (priv->rename_directory_dialog),
                                  GTK_WINDOW (MainWindow));
    gtk_dialog_set_default_response (GTK_DIALOG (priv->rename_directory_dialog),
                                     GTK_RESPONSE_APPLY);

    /* We attach useful data to the combobox */
    g_object_set_data (G_OBJECT (priv->rename_directory_dialog),
                       "Parent_Directory", directory_parent);
    g_object_set_data (G_OBJECT (priv->rename_directory_dialog),
                       "Current_Directory", directory_name);
    g_signal_connect (priv->rename_directory_dialog, "response",
                      G_CALLBACK (et_rename_directory_on_response), self);

    string = g_strdup_printf (_("Rename the directory ‘%s’ to:"),
                              directory_name_utf8);
    label = GTK_WIDGET (gtk_builder_get_object (builder, "rename_label"));
    gtk_label_set_label (GTK_LABEL (label), string);
    g_free (string);
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

    /* The entry to rename the directory. */
    priv->rename_directory_entry = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                       "rename_entry"));
    /* Set the directory into the combobox */
    gtk_entry_set_text (GTK_ENTRY (priv->rename_directory_entry),
                        directory_name_utf8);

    /* Rename directory : check box + entry + Status icon */
    priv->rename_directory_mask_toggle = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                             "rename_mask_check"));
    g_settings_bind (MainSettings, "rename-directory-with-mask",
                     priv->rename_directory_mask_toggle, "active",
                     G_SETTINGS_BIND_DEFAULT);
    g_signal_connect_swapped (priv->rename_directory_mask_toggle, "toggled",
                              G_CALLBACK (Rename_Directory_With_Mask_Toggled),
                              self);

    /* The entry to enter the mask to apply. */
    priv->rename_directory_mask_entry = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                            "rename_mask_entry"));
    gtk_widget_set_size_request(priv->rename_directory_mask_entry, 80, -1);

    /* Signal to generate preview (preview of the new directory). */
    g_signal_connect_swapped (priv->rename_directory_mask_entry,
                              "changed",
                              G_CALLBACK (rename_directory_generate_preview),
                              self);

    g_settings_bind (MainSettings, "rename-directory-default-mask",
                     priv->rename_directory_mask_entry, "text",
                     G_SETTINGS_BIND_DEFAULT);

    /* Mask status icon. Signal connection to check if mask is correct to the
     * mask entry. */
    g_signal_connect (priv->rename_directory_mask_entry, "changed",
                      G_CALLBACK (entry_check_rename_file_mask), NULL);

    /* Preview label. */
    priv->rename_directory_preview_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                               "rename_preview_label"));
    /* Button to save: to rename directory */
    button = gtk_dialog_get_widget_for_response (GTK_DIALOG (priv->rename_directory_dialog),
                                                 GTK_RESPONSE_APPLY);
    g_signal_connect_swapped (priv->rename_directory_entry,
                              "changed",
                              G_CALLBACK (empty_entry_disable_widget),
                              G_OBJECT (button));

    g_object_unref (builder);

    gtk_widget_show_all (priv->rename_directory_dialog);

    /* To initialize the 'Use mask' check button state. */
    g_signal_emit_by_name (G_OBJECT (priv->rename_directory_mask_toggle),
                           "toggled");

    /* To initialize PreviewLabel + MaskStatusIconBox. */
    g_signal_emit_by_name (priv->rename_directory_mask_entry, "changed");

    g_free (directory_name_utf8);
}

static void
Destroy_Rename_Directory_Window (EtBrowser *self)
{
    EtBrowserPrivate *priv;

    priv = et_browser_get_instance_private (self);

    if (priv->rename_directory_dialog)
    {
        g_free(g_object_get_data(G_OBJECT(priv->rename_directory_dialog),"Parent_Directory"));
        g_free(g_object_get_data(G_OBJECT(priv->rename_directory_dialog),"Current_Directory"));

        gtk_widget_destroy(priv->rename_directory_dialog);
        priv->rename_directory_preview_label = NULL;
        priv->rename_directory_dialog = NULL;
    }
}

static void
Rename_Directory (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    DIR   *dir;
    gchar *directory_parent;
    gchar *directory_last_name;
    gchar *directory_new_name;
    gchar *directory_new_name_file;
    gchar *last_path;
    gchar *last_path_utf8;
    gchar *new_path;
    gchar *new_path_utf8;
    gchar *tmp_path;
    gchar *tmp_path_utf8;
    gint   fd_tmp;

    priv = et_browser_get_instance_private (self);

    g_return_if_fail (priv->rename_directory_dialog != NULL);

    directory_parent    = g_object_get_data(G_OBJECT(priv->rename_directory_dialog),"Parent_Directory");
    directory_last_name = g_object_get_data(G_OBJECT(priv->rename_directory_dialog),"Current_Directory");

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->rename_directory_mask_toggle)))
    {
        /* Renamed from mask. */
        gchar *mask;

        mask = g_settings_get_string (MainSettings,
                                      "rename-directory-default-mask");
        directory_new_name = et_scan_generate_new_directory_name_from_mask (ETCore->ETFileDisplayed,
                                                                            mask,
                                                                            FALSE);
        g_free (mask);

    }
    else
    {
        /* Renamed 'manually'. */
        directory_new_name  = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->rename_directory_entry)));
    }

    /* Check if a name for the directory have been supplied */
    if (!directory_new_name || g_utf8_strlen(directory_new_name, -1) < 1)
    {
        GtkWidget *msgdialog;

        msgdialog = gtk_message_dialog_new(GTK_WINDOW(MainWindow),
                                           GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_CLOSE,
                                           "%s",
                                           _("You must type a directory name"));
        gtk_window_set_title(GTK_WINDOW(msgdialog),_("Directory Name Error"));

        gtk_dialog_run(GTK_DIALOG(msgdialog));
        gtk_widget_destroy(msgdialog);
        g_free(directory_new_name);
        return;
    }

    /* Check that we can write the new directory name */
    directory_new_name_file = filename_from_display(directory_new_name);
    if (!directory_new_name_file)
    {
        GtkWidget *msgdialog;

        msgdialog = gtk_message_dialog_new(GTK_WINDOW(MainWindow),
                                           GTK_DIALOG_MODAL  | GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_CLOSE,
                                           _("Could not convert ‘%s’ into filename encoding"),
                                           directory_new_name);
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (msgdialog),
                                                  _("Please use another name."));
        gtk_window_set_title(GTK_WINDOW(msgdialog),_("Directory Name Error"));

        gtk_dialog_run(GTK_DIALOG(msgdialog));
        gtk_widget_destroy(msgdialog);
        g_free(directory_new_name_file);
    }

    g_free (directory_new_name);

    /* If the directory name haven't been changed, we do nothing! */
    if (directory_last_name && directory_new_name_file
    && strcmp(directory_last_name,directory_new_name_file)==0)
    {
        Destroy_Rename_Directory_Window (self);
        g_free(directory_new_name_file);
        return;
    }

    /* Build the current and new absolute paths */
    last_path = g_strconcat(directory_parent, directory_last_name, NULL);
    last_path_utf8 = filename_to_display(last_path);
    new_path = g_strconcat(directory_parent, directory_new_name_file, NULL);
    new_path_utf8 = filename_to_display(new_path);

    /* Check if the new directory name doesn't already exists, and detect if
     * it's only a case change (needed for vfat) */
    if ( (dir=opendir(new_path))!=NULL )
    {
        GtkWidget *msgdialog;
        //gint response;

        closedir(dir);
        if (strcasecmp(last_path,new_path) != 0)
        {
    // TODO
    //        // The same directory already exists. So we ask if we want to move the files
    //        msg = g_strdup_printf(_("The directory already exists!\n(%s)\nDo you want "
    //            "to move the files?"),new_path_utf8);
    //        msgbox = msg_box_new(_("Confirm"),
    //                             GTK_WINDOW(MainWindow),
    //                             NULL,
    //                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
    //                             msg,
    //                             GTK_STOCK_DIALOG_QUESTION,
    //                             GTK_STOCK_NO,  GTK_RESPONSE_NO,
	//                             GTK_STOCK_YES, GTK_RESPONSE_YES,
    //                             NULL);
    //        g_free(msg);
    //        response = gtk_dialog_run(GTK_DIALOG(msgbox));
    //        gtk_widget_destroy(msgbox);
    //
    //        switch (response)
    //        {
    //            case GTK_STOCK_YES:
    //                // Here we must rename all files with the new location, and remove the directory
    //
    //                Rename_File ()
    //
    //                break;
    //            case BUTTON_NO:
    //                break;
    //        }

            msgdialog = gtk_message_dialog_new(GTK_WINDOW(MainWindow),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR,
                                               GTK_BUTTONS_CLOSE,
                                               "%s",
                                               "Cannot rename file");
            gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (msgdialog),
                                                      _("The directory name ‘%s’ already exists."),new_path_utf8);
            gtk_window_set_title(GTK_WINDOW(msgdialog),_("Rename File Error"));

            gtk_dialog_run(GTK_DIALOG(msgdialog));
            gtk_widget_destroy(msgdialog);

            g_free(directory_new_name_file);
            g_free(last_path);
            g_free(last_path_utf8);
            g_free(new_path);
            g_free(new_path_utf8);

            return;
        }
    }

    /* Temporary path (useful when changing only string case) */
    tmp_path = g_strdup_printf("%s.XXXXXX",last_path);
    tmp_path_utf8 = filename_to_display(tmp_path);

    if ( (fd_tmp = mkstemp(tmp_path)) >= 0 )
    {
        close(fd_tmp);
        unlink(tmp_path);
    }

    /* Rename the directory from 'last name' to 'tmp name' */
    if ( rename(last_path,tmp_path)!=0 )
    {
        GtkWidget *msgdialog;

        msgdialog = gtk_message_dialog_new(GTK_WINDOW(MainWindow),
                                           GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_CLOSE,
                                           "Cannot rename directory '%s' to '%s'",
                                           last_path_utf8,
                                           tmp_path_utf8);
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msgdialog),"%s",g_strerror(errno));
        gtk_window_set_title(GTK_WINDOW(msgdialog),_("Rename Directory Error"));

        gtk_dialog_run(GTK_DIALOG(msgdialog));
        gtk_widget_destroy(msgdialog);

        g_free(directory_new_name_file);
        g_free(last_path);
        g_free(last_path_utf8);
        g_free(new_path);
        g_free(new_path_utf8);
        g_free(tmp_path);
        g_free(tmp_path_utf8);

        return;
    }

    /* Rename the directory from 'tmp name' to 'new name' (final name) */
    if ( rename(tmp_path,new_path)!=0 )
    {
        GtkWidget *msgdialog;

        msgdialog = gtk_message_dialog_new(GTK_WINDOW(MainWindow),
                                           GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_CLOSE,
                                           "Cannot rename directory '%s' to '%s",
                                           tmp_path_utf8,
                                           new_path_utf8);
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msgdialog),"%s",g_strerror(errno));
        gtk_window_set_title(GTK_WINDOW(msgdialog),_("Rename Directory Error"));

        gtk_dialog_run(GTK_DIALOG(msgdialog));
        gtk_widget_destroy(msgdialog);

        g_free(directory_new_name_file);
        g_free(last_path);
        g_free(last_path_utf8);
        g_free(new_path);
        g_free(new_path_utf8);
        g_free(tmp_path);
        g_free(tmp_path_utf8);

        return;
    }

    ET_Update_Directory_Name_Into_File_List(last_path,new_path);
    Browser_Tree_Rename_Directory (self, last_path, new_path);

    // To update file path in the browser entry
    if (ETCore->ETFileDisplayedList)
    {
        ET_Display_File_Data_To_UI(ETCore->ETFileDisplayed);
    }else
    {
        gchar *tmp = filename_to_display (et_browser_get_current_path (self));
        et_browser_entry_set_text (self, tmp);
        g_free(tmp);
    }

    Destroy_Rename_Directory_Window (self);
    g_free(last_path);
    g_free(last_path_utf8);
    g_free(new_path);
    g_free(new_path_utf8);
    g_free(tmp_path);
    g_free(tmp_path_utf8);
    g_free(directory_new_name_file);
    et_application_window_status_bar_message (ET_APPLICATION_WINDOW (MainWindow),
                                              _("Directory renamed"), TRUE);
}

static void
Rename_Directory_With_Mask_Toggled (EtBrowser *self)
{
    EtBrowserPrivate *priv;

    priv = et_browser_get_instance_private (self);

    gtk_widget_set_sensitive (priv->rename_directory_entry,
                              !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->rename_directory_mask_toggle)));
    gtk_widget_set_sensitive (priv->rename_directory_mask_entry,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->rename_directory_mask_toggle)));
    gtk_widget_set_sensitive (priv->rename_directory_preview_label,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->rename_directory_mask_toggle)));
}


/*
 * Window where is typed the name of the program to run, which
 * receives the current directory as parameter.
 */
void
et_browser_show_open_directory_with_dialog (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    GtkBuilder *builder;
    GError *error = NULL;
    GtkWidget *button;
    gchar *current_directory = NULL;

    g_return_if_fail (ET_BROWSER (self));

    priv = et_browser_get_instance_private (self);

    if (priv->open_directory_with_dialog != NULL)
    {
        gtk_window_present(GTK_WINDOW(priv->open_directory_with_dialog));
        return;
    }

    /* Current directory. */
    current_directory = g_strdup (priv->current_path);

    if (!current_directory || strlen(current_directory)==0)
        return;

    builder = gtk_builder_new ();
    gtk_builder_add_from_resource (builder,
                                   "/org/gnome/EasyTAG/browser.ui",
                                   &error);

    if (error != NULL)
    {
        g_error ("Unable to get open directory with dialog from resource: %s",
                 error->message);
    }

    priv->open_directory_with_dialog = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                           "open_directory_dialog"));

    gtk_window_set_transient_for (GTK_WINDOW (priv->open_directory_with_dialog),
                                  GTK_WINDOW (MainWindow));
    gtk_dialog_set_default_response (GTK_DIALOG (priv->open_directory_with_dialog),
                                     GTK_RESPONSE_OK);
    g_signal_connect (priv->open_directory_with_dialog, "response",
                      G_CALLBACK (et_run_program_tree_on_response), self);

    /* The combobox to enter the program to run */
    priv->open_directory_with_combobox = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                             "open_directory_combo"));
    gtk_combo_box_set_model (GTK_COMBO_BOX (priv->open_directory_with_combobox),
                             GTK_TREE_MODEL (priv->run_program_model));

    /* History list */
    gtk_list_store_clear (priv->run_program_model);
    Load_Run_Program_With_Directory_List (priv->run_program_model,
                                          MISC_COMBO_TEXT);
    g_signal_connect_swapped (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->open_directory_with_combobox))),
                              "activate",
                              G_CALLBACK (Run_Program_With_Directory),
                              self);

    /* The button to Browse */
    button = GTK_WIDGET (gtk_builder_get_object (builder,
                                                 "open_directory_button"));
    g_signal_connect_swapped (button, "clicked",
                              G_CALLBACK (File_Selection_Window_For_File),
                              G_OBJECT (gtk_bin_get_child (GTK_BIN (priv->open_directory_with_combobox))));

    /* We attach useful data to the combobox (into Run_Program_With_Directory) */
    g_object_set_data (G_OBJECT (priv->open_directory_with_combobox),
                       "Current_Directory", current_directory);

    /* Button to execute */
    button = gtk_dialog_get_widget_for_response (GTK_DIALOG (priv->open_directory_with_dialog),
                                                 GTK_RESPONSE_OK);
    g_signal_connect_swapped (button, "clicked",
                              G_CALLBACK (Run_Program_With_Directory),
                              self);
    g_signal_connect_swapped (gtk_bin_get_child (GTK_BIN (priv->open_directory_with_combobox)),
                              "changed",
                              G_CALLBACK (empty_entry_disable_widget),
                              G_OBJECT (button));
    g_signal_emit_by_name (G_OBJECT (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->open_directory_with_combobox)))),
                           "changed", NULL);

    gtk_widget_show_all (priv->open_directory_with_dialog);
}

static void
Destroy_Run_Program_Tree_Window (EtBrowser *self)
{
    EtBrowserPrivate *priv;

    priv = et_browser_get_instance_private (self);

    if (priv->open_directory_with_dialog)
    {
        gtk_widget_hide (priv->open_directory_with_dialog);
    }
}

void
Run_Program_With_Directory (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    gchar *program_name;
    gchar *current_directory;
    GList *args_list = NULL;
    gboolean program_ran;
    GError *error = NULL;

    priv = et_browser_get_instance_private (self);

    program_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->open_directory_with_combobox)))));
    current_directory = g_object_get_data (G_OBJECT (priv->open_directory_with_combobox),
                                           "Current_Directory");
#ifdef G_OS_WIN32
    /* On win32 : 'winamp.exe "c:\path\to\dir"' succeed, while 'winamp.exe "c:\path\to\dir\"' fails */
    ET_Win32_Path_Remove_Trailing_Backslash(current_directory);
#endif /* G_OS_WIN32 */

    // List of parameters (here only one! : the current directory)
    args_list = g_list_append(args_list,current_directory);

    program_ran = et_run_program (program_name, args_list, &error);
    g_list_free(args_list);

    if (program_ran)
    {
        gchar *msg;

        // Append newest choice to the drop down list
        Add_String_To_Combo_List(priv->run_program_model, program_name);

        // Save list attached to the combobox
        Save_Run_Program_With_Directory_List(priv->run_program_model, MISC_COMBO_TEXT);

        Destroy_Run_Program_Tree_Window (self);

        msg = g_strdup_printf (_("Executed command ‘%s’"), program_name);
        et_application_window_status_bar_message (ET_APPLICATION_WINDOW (MainWindow),
                                                  msg, TRUE);

        g_free (msg);
    }
    else
    {
        Log_Print (LOG_ERROR, _("Failed to launch program ‘%s’"),
                   error->message);
        g_clear_error (&error);
    }

    g_free(program_name);
}

static void
Run_Program_With_Selected_Files (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    gchar   *program_name;
    ET_File *ETFile;
    GList   *selected_paths;
    GList *l;
    GList   *args_list = NULL;
    GtkTreeIter iter;
    gboolean program_ran;
    GError *error = NULL;

    priv = et_browser_get_instance_private (self);

    if (!GTK_IS_COMBO_BOX (priv->open_files_with_combobox) || !ETCore->ETFileDisplayedList)
        return;

    // Programe name to run
    program_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->open_files_with_combobox)))));

    // List of files to pass as parameters
    selected_paths = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->file_view)), NULL);

    for (l = selected_paths; l != NULL; l = g_list_next (l))
    {
        if (gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->file_model), &iter,
                                     (GtkTreePath *)l->data))
        {
            gtk_tree_model_get (GTK_TREE_MODEL (priv->file_model), &iter,
                                LIST_FILE_POINTER, &ETFile, -1);

            args_list = g_list_prepend (args_list,
                                        ((File_Name *)ETFile->FileNameCur->data)->value);
            //args_list = g_list_append(args_list,((File_Name *)ETFile->FileNameCur->data)->value_utf8);
        }
    }

    args_list = g_list_reverse (args_list);
    program_ran = et_run_program (program_name, args_list, &error);

    g_list_free_full (selected_paths, (GDestroyNotify)gtk_tree_path_free);
    g_list_free(args_list);

    if (program_ran)
    {
        gchar *msg;

        // Append newest choice to the drop down list
        //gtk_list_store_prepend(GTK_LIST_STORE(priv->run_program_model), &iter);
        //gtk_list_store_set(priv->run_program_model, &iter, MISC_COMBO_TEXT, program_name, -1);
        Add_String_To_Combo_List(GTK_LIST_STORE(priv->run_program_model), program_name);

        // Save list attached to the combobox
        Save_Run_Program_With_File_List(priv->run_program_model, MISC_COMBO_TEXT);

        Destroy_Run_Program_List_Window (self);

        msg = g_strdup_printf (_("Executed command ‘%s’"), program_name);
        et_application_window_status_bar_message (ET_APPLICATION_WINDOW (MainWindow),
                                                  msg, TRUE);

        g_free (msg);
    }
    else
    {
        Log_Print (LOG_ERROR, _("Failed to launch program ‘%s’"),
                   error->message);
        g_clear_error (&error);
    }

    g_free(program_name);
}

/*
 * Window where is typed the name of the program to run, which
 * receives the current file as parameter.
 */
void
et_browser_show_open_files_with_dialog (EtBrowser *self)
{
    EtBrowserPrivate *priv;
    GtkBuilder *builder;
    GError *error = NULL;
    GtkWidget *button;

    g_return_if_fail (ET_BROWSER (self));

    priv = et_browser_get_instance_private (self);

    if (priv->open_files_with_dialog != NULL)
    {
        gtk_window_present(GTK_WINDOW(priv->open_files_with_dialog));
        return;
    }

    builder = gtk_builder_new ();
    gtk_builder_add_from_resource (builder,
                                   "/org/gnome/EasyTAG/browser.ui",
                                   &error);

    if (error != NULL)
    {
        g_error ("Unable to get open with files dialog from resource: %s",
                 error->message);
    }

    priv->open_files_with_dialog = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                       "open_files_dialog"));
    gtk_dialog_set_default_response (GTK_DIALOG (priv->open_files_with_dialog),
                                     GTK_RESPONSE_OK);
    gtk_window_set_transient_for (GTK_WINDOW (priv->open_files_with_dialog),
                                  GTK_WINDOW (MainWindow));
    g_signal_connect ((priv->open_files_with_dialog), "response",
                      G_CALLBACK (et_run_program_list_on_response), self);

    /* The combobox to enter the program to run */
    priv->open_files_with_combobox = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                         "open_files_combo"));
    gtk_combo_box_set_model (GTK_COMBO_BOX (priv->open_files_with_combobox),
                             GTK_TREE_MODEL (priv->run_program_model));
    gtk_widget_set_size_request (GTK_WIDGET (priv->open_files_with_combobox),
                                 250, -1);

    /* History list */
    gtk_list_store_clear (priv->run_program_model);
    Load_Run_Program_With_File_List (priv->run_program_model, MISC_COMBO_TEXT);
    g_signal_connect_swapped (gtk_bin_get_child (GTK_BIN (priv->open_files_with_combobox)),
                              "activate",
                              G_CALLBACK (Run_Program_With_Selected_Files),
			                  self);

    /* The button to Browse */
    button = GTK_WIDGET (gtk_builder_get_object (builder,
                                                 "open_files_button"));
    g_signal_connect_swapped (button, "clicked",
                              G_CALLBACK (File_Selection_Window_For_File),
                              G_OBJECT (gtk_bin_get_child (GTK_BIN (priv->open_files_with_combobox))));

    g_object_unref (builder);

    /* Button to execute */
    button = gtk_dialog_get_widget_for_response (GTK_DIALOG (priv->open_files_with_dialog),
                                                 GTK_RESPONSE_OK);
    g_signal_connect_swapped (button, "clicked",
                              G_CALLBACK (Run_Program_With_Selected_Files),
			                  self);
    g_signal_connect_swapped (gtk_bin_get_child (GTK_BIN (priv->open_files_with_combobox)),
                              "changed",
                              G_CALLBACK (empty_entry_disable_widget),
                              G_OBJECT (button));
    g_signal_emit_by_name (gtk_bin_get_child (GTK_BIN (priv->open_files_with_combobox)),
                           "changed", NULL);

    gtk_widget_show_all (priv->open_files_with_dialog);
}

static void
Destroy_Run_Program_List_Window (EtBrowser *self)
{
    EtBrowserPrivate *priv;

    priv = et_browser_get_instance_private (self);

    if (priv->open_files_with_dialog)
    {
        gtk_widget_hide (priv->open_files_with_dialog);
    }
}

/*
 * empty_entry_disable_widget:
 * @widget: a widget to set sensitive if @entry contains text
 * @entry: the entry for which to test the text
 *
 * Make @widget insensitive if @entry contains no text, or sensitive otherwise.
 */
static void
empty_entry_disable_widget (GtkWidget *widget, GtkEntry *entry)
{
    const gchar *text;

    g_return_if_fail (widget != NULL && entry != NULL);

    text = gtk_entry_get_text (GTK_ENTRY (entry));

    gtk_widget_set_sensitive (widget, text && *text);
}

/*
 * et_rename_directory_on_response:
 * @dialog: the dialog which emitted the response signal
 * @response_id: the response ID
 * @user_data: user data set when the signal was connected
 *
 * Signal handler for the rename directory dialog.
 */
static void
et_rename_directory_on_response (GtkDialog *dialog, gint response_id,
                                 gpointer user_data)
{
    EtBrowser *self;

    self = ET_BROWSER (user_data);

    switch (response_id)
    {
        case GTK_RESPONSE_APPLY:
            Rename_Directory (self);
            break;
        case GTK_RESPONSE_CANCEL:
        case GTK_RESPONSE_DELETE_EVENT:
            Destroy_Rename_Directory_Window (self);
            break;
        default:
            g_assert_not_reached ();
    }
}

/*
 * et_run_program_tree_on_response:
 * @dialog: the dialog which emitted the response signal
 * @response_id: the response ID
 * @user_data: user data set when the signal was connected
 *
 * Signal handler for the run program on directory tree dialog.
 */
static void
et_run_program_tree_on_response (GtkDialog *dialog, gint response_id,
                                 gpointer user_data)
{
    EtBrowser *self;

    self = ET_BROWSER (user_data);

    switch (response_id)
    {
        case GTK_RESPONSE_OK:
            /* FIXME: Ignored for now. */
            break;
        case GTK_RESPONSE_CANCEL:
        case GTK_RESPONSE_DELETE_EVENT:
            Destroy_Run_Program_Tree_Window (self);
            break;
        default:
            g_assert_not_reached ();
    }
}
/*
 * et_run_program_list_on_response:
 * @dialog: the dialog which emitted the response signal
 * @response_id: the response ID
 * @user_data: user data set when the signal was connected
 *
 * Signal handler for the run program on selected file dialog.
 */
static void
et_run_program_list_on_response (GtkDialog *dialog, gint response_id,
                                 gpointer user_data)
{
    EtBrowser *self;

    self = ET_BROWSER (user_data);

    switch (response_id)
    {
        case GTK_RESPONSE_OK:
            /* FIXME: Ignored for now. */
            break;
        case GTK_RESPONSE_CANCEL:
        case GTK_RESPONSE_DELETE_EVENT:
            Destroy_Run_Program_List_Window (self);
            break;
        default:
            g_assert_not_reached ();
    }
}

/*
 * get_sort_order_for_column_id:
 * @column_id: the column ID for which to set the sort order
 *
 * Gets the sort order for the given column ID from the browser list treeview
 * column.
 *
 * Returns: the sort order for @column_id
 */
GtkSortType
et_browser_get_sort_order_for_column_id (EtBrowser *self, gint column_id)
{
    GtkTreeViewColumn *column;

    column = et_browser_get_column_for_column_id (self, column_id);
    return gtk_tree_view_column_get_sort_order (column);
}

/*
 * get_column_for_column_id:
 * @column_id: the column ID of the #GtkTreeViewColumn to fetch
 *
 * Gets the browser list treeview column for the given column ID.
 *
 * Returns: (transfer none): the tree view column corresponding to @column_id
 */
GtkTreeViewColumn *
et_browser_get_column_for_column_id (EtBrowser *self, gint column_id)
{
    EtBrowserPrivate *priv;

    priv = et_browser_get_instance_private (self);

    return gtk_tree_view_get_column (GTK_TREE_VIEW (priv->file_view),
                                     column_id);
}

static void
et_browser_destroy (GtkWidget *widget)
{
    EtBrowserPrivate *priv;

    priv = et_browser_get_instance_private (ET_BROWSER (widget));

    /* Save combobox history list before exit. */
    if (priv->entry_model)
    {
        Save_Path_Entry_List (priv->entry_model, MISC_COMBO_TEXT);
        priv->entry_model = NULL;
        /* The model is disposed when the combo box is disposed. */
    }

    GTK_WIDGET_CLASS (et_browser_parent_class)->destroy (widget);
}

static void
et_browser_finalize (GObject *object)
{
    EtBrowserPrivate *priv;

    priv = et_browser_get_instance_private (ET_BROWSER (object));

    g_free (priv->current_path);
    priv->current_path = NULL;
    g_clear_object (&priv->run_program_model);

    G_OBJECT_CLASS (et_browser_parent_class)->finalize (object);
}

static void
et_browser_init (EtBrowser *self)
{
    EtBrowserPrivate *priv;

    priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, ET_TYPE_BROWSER,
                                                     EtBrowserPrivate);

    priv->open_directory_with_dialog = NULL;
    priv->open_directory_with_combobox = NULL;
    priv->open_files_with_dialog = NULL;
    priv->open_files_with_combobox = NULL;
    priv->current_path = NULL;

    create_browser (self);
}

static void
et_browser_class_init (EtBrowserClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = et_browser_finalize;
    GTK_WIDGET_CLASS (klass)->destroy = et_browser_destroy;

    g_type_class_add_private (klass, sizeof (EtBrowserPrivate));
}

/*
 * et_browser_new:
 *
 * Create a new EtBrowser instance.
 *
 * Returns: a new #EtBrowser
 */
EtBrowser *
et_browser_new (void)
{
    return g_object_new (ET_TYPE_BROWSER, NULL);
}
