/* browser.c - 2000/04/28 */
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


/* Some parts of this browser are taken from:
 * XMMS - Cross-platform multimedia player
 * Copyright (C) 1998-1999  Peter Alm, Mikael Alm, Olle Hallnas,
 * Thomas Nilsson and 4Front Technologies
 */

#include <config.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <glib/gi18n-lib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "gtk2_compat.h"
#include "easytag.h"
#include "browser.h"
#include "et_core.h"
#include "scan.h"
#include "bar.h"
#include "log.h"
#include "misc.h"
#include "setting.h"
#include "charset.h"
#include "dlm.h"

#include <assert.h>

#ifdef G_OS_WIN32
#include <windows.h>
#include "win32/win32dep.h"
#endif /* G_OS_WIN32 */

/* Pixmaps */
#include "pixmaps/opened_folder.xpm"
#include "pixmaps/closed_folder.xpm"
#include "pixmaps/closed_folder_readonly.xpm"
#include "pixmaps/closed_folder_unreadable.xpm"
#ifdef G_OS_WIN32
#include "pixmaps/ram_disk.xpm"
#endif /* G_OS_WIN32 */



/****************
 * Declarations *
 ****************/

// Pixmaps
static GdkPixbuf *opened_folder_pixmap = NULL, *closed_folder_pixmap, *closed_folder_readonly_pixmap, *closed_folder_unreadable_pixmap;
#ifdef G_OS_WIN32
// Pixmap used for Win32 only
static GdkPixbuf *harddrive_pixmap, *removable_pixmap, *cdrom_pixmap, *network_pixmap, *ramdisk_pixmap;
#endif /* G_OS_WIN32 */

GtkWidget    *BrowserTree; // Tree of directories
GtkTreeStore *directoryTreeModel;
GtkWidget    *BrowserList;     // List of files
GtkListStore *fileListModel;
GtkWidget    *BrowserLabel;
GtkWidget    *BrowserButton;
GtkWidget    *BrowserParentButton;
GtkWidget    *BrowserNoteBook;
GtkWidget    *BrowserArtistList;
GtkListStore *artistListModel;
GtkWidget    *BrowserAlbumList;
GtkListStore *albumListModel;
gchar        *BrowserCurrentPath = NULL; // Path selected in the browser area (BrowserEntry or BrowserTree)

GtkListStore *RunProgramModel;

GtkWidget *RunProgramTreeWindow  = NULL;
GtkWidget *RunProgramListWindow  = NULL;

// The Rename Directory window
GtkWidget    *RenameDirectoryWindow = NULL;
GtkWidget    *RenameDirectoryCombo;
GtkWidget    *RenameDirectoryWithMask;
GtkWidget    *RenameDirectoryMaskCombo;
GtkListStore *RenameDirectoryMaskModel = NULL;
GtkWidget    *RenameDirectoryMaskStatusIconBox;
GtkWidget    *RenameDirectoryPreviewLabel = NULL;

guint blrs_idle_handler_id = 0;
guint blru_idle_handler_id = 0;
guint bl_row_selected;

ET_File *LastBrowserListETFileSelected; // The last ETFile selected in the BrowserList


gchar *Rename_Directory_Masks [] =
{
    "%a - %b",
    "%a_-_%b",
    "%a - %b (%y) - %g",
    "%a_-_%b_(%y)_-_%g",
    "VA - %b (%y)",
    "VA_-_%b_(%y)",
    NULL
};


/**************
 * Prototypes *
 **************/

static gboolean Browser_Tree_Key_Press (GtkWidget *tree, GdkEvent *event,
                                        gpointer data);
static void Browser_Tree_Set_Node_Visible (GtkWidget *directoryView,
                                           GtkTreePath *path);
static void Browser_List_Set_Row_Visible (GtkTreeModel *treeModel,
                                          GtkTreeIter *rowIter);
static void Browser_Tree_Initialize (void);
static gboolean Browser_Tree_Node_Selected (GtkTreeSelection *selection,
                                            gpointer user_data);
static void Browser_Tree_Rename_Directory (const gchar *last_path,
                                           const gchar *new_path);
static void Browser_Tree_Handle_Rename (GtkTreeIter *parentnode,
                                        const gchar *old_path,
                                        const gchar *new_path);

static gint Browser_List_Key_Press        (GtkWidget *list, GdkEvent *event, gpointer data);
static gboolean Browser_List_Button_Press (GtkTreeView *treeView,
                                           GdkEventButton *event);
static void Browser_List_Row_Selected (GtkTreeSelection * selection,
                                       gpointer data);
static void Browser_List_Set_Row_Appearance (GtkTreeIter *iter);
static gint Browser_List_Sort_Func (GtkTreeModel *model, GtkTreeIter *a,
                                    GtkTreeIter *b, gpointer data);
static void Browser_List_Select_File_By_Iter (GtkTreeIter *iter,
                                              gboolean select_it);
void        Browser_List_Select_All_Files       (void);
void        Browser_List_Unselect_All_Files     (void);
void        Browser_List_Invert_File_Selection  (void);

static void Browser_Entry_Activated (void);

static void Browser_Parent_Button_Clicked (void);

static void Browser_Artist_List_Load_Files (ET_File *etfile_to_select);
static void Browser_Artist_List_Row_Selected (GtkTreeSelection *selection,
                                              gpointer data);
static void Browser_Artist_List_Set_Row_Appearance (GtkTreeIter *row);

static void Browser_Album_List_Load_Files (GList *albumlist,
                                           ET_File *etfile_to_select);
static void Browser_Album_List_Row_Selected (GtkTreeSelection *selection,
                                             gpointer data);
static void Browser_Album_List_Set_Row_Appearance (GtkTreeIter *row);

gchar      *Browser_Get_Current_Path       (void);
static void Browser_Update_Current_Path (const gchar *path);
void        Browser_Load_Home_Directory    (void);
void        Browser_Load_Default_Directory (void);
void        Browser_Reload_Directory       (void);

#ifdef G_OS_WIN32
static gboolean Browser_Win32_Get_Drive_Root (gchar *drive,
                                              GtkTreeIter *rootNode,
                                              GtkTreePath **rootPath);
#endif /* G_OS_WIN32 */

static gboolean check_for_subdir   (gchar *path);

static GtkTreePath *Find_Child_Node(GtkTreeIter *parent, gchar *searchtext);

static GdkPixbuf *Pixmap_From_Directory_Permission (const gchar *path);

static void expand_cb   (GtkWidget *tree, GtkTreeIter *iter, GtkTreePath *path, gpointer data);
static void collapse_cb (GtkWidget *tree, GtkTreeIter *iter, GtkTreePath *treePath, gpointer data);

/* Pop up menus */
static gboolean Browser_Popup_Menu_Handler (GtkMenu *menu,
                                            GdkEventButton *event);

/* For window to rename a directory */
void        Browser_Open_Rename_Directory_Window (void);
static void Destroy_Rename_Directory_Window (void);
static void Rename_Directory (void);
static gboolean Rename_Directory_Window_Key_Press (GtkWidget *window,
                                                   GdkEvent *event);
static void Rename_Directory_With_Mask_Toggled (void);

/* For window to run a program with the directory */
void        Browser_Open_Run_Program_Tree_Window (void);
static void Destroy_Run_Program_Tree_Window (void);
static gboolean Run_Program_Tree_Window_Key_Press (GtkWidget *window,
                                                   GdkEvent *event);
static void Run_Program_With_Directory (GtkWidget *combobox);

/* For window to run a program with the file */
void        Browser_Open_Run_Program_List_Window (void);
static void Destroy_Run_Program_List_Window (void);
static gboolean Run_Program_List_Window_Key_Press (GtkWidget *window,
                                                   GdkEvent *event);
static void Run_Program_With_Selected_Files (GtkWidget *combobox);

static gboolean Run_Program (const gchar *program_name, GList *args_list);



/*************
 * Functions *
 *************/
/*
 * Load home directory
 */
void Browser_Load_Home_Directory (void)
{
    Browser_Tree_Select_Dir (g_get_home_dir ());
}

/*
 * Load desktop directory
 */
void Browser_Load_Desktop_Directory (void)
{
    Browser_Tree_Select_Dir(g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP));
}

/*
 * Load documents directory
 */
void Browser_Load_Documents_Directory (void)
{
    Browser_Tree_Select_Dir(g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS));
}

/*
 * Load downloads directory
 */
void Browser_Load_Downloads_Directory (void)
{
    Browser_Tree_Select_Dir(g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD));
}

/*
 * Load music directory
 */
void Browser_Load_Music_Directory (void)
{
    Browser_Tree_Select_Dir(g_get_user_special_dir(G_USER_DIRECTORY_MUSIC));
}


/*
 * Load default directory
 */
void Browser_Load_Default_Directory (void)
{
    gchar *path_utf8;
    gchar *path;

    path_utf8 = g_strdup(DEFAULT_PATH_TO_MP3);
    if (!path_utf8 || strlen(path_utf8)<=0)
    {
        g_free(path_utf8);
        path_utf8 = g_strdup (g_get_home_dir ());
    }

    // 'DEFAULT_PATH_TO_MP3' is stored in UTF-8, we must convert it to the file system encoding before...
    path = filename_from_display(path_utf8);
    Browser_Tree_Select_Dir(path);

    g_free(path);
    g_free(path_utf8);
}


/*
 * Get the path from the selected node (row) in the browser
 * Warning: return NULL if no row selected int the tree.
 * Remember to free the value returned from this function!
 */
static gchar *
Browser_Tree_Get_Path_Of_Selected_Node (void)
{
    GtkTreeSelection *selection;
    GtkTreeIter selectedIter;
    gchar *path;

    g_return_val_if_fail (BrowserTree != NULL, NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserTree));
    if (selection
    && gtk_tree_selection_get_selected(selection, NULL, &selectedIter))
    {
        gtk_tree_model_get(GTK_TREE_MODEL(directoryTreeModel), &selectedIter,
                           TREE_COLUMN_FULL_PATH, &path, -1);
        return path;
    }else
    {
        return NULL;
    }
}


/*
 * Set the 'path' within the variable BrowserCurrentPath.
 */
static void
Browser_Update_Current_Path (const gchar *path)
{
    /* Be sure that we aren't passing 'BrowserCurrentPath' as parameter of the function :
     * to avoid some memory problems */
    g_return_if_fail (path != NULL || path != BrowserCurrentPath);

    if (BrowserCurrentPath != NULL)
        g_free(BrowserCurrentPath);
    BrowserCurrentPath = g_strdup(path);

#ifdef G_OS_WIN32
    /* On win32 : "c:\path\to\dir" succeed with stat() for example, while "c:\path\to\dir\" fails */
    ET_Win32_Path_Remove_Trailing_Backslash(BrowserCurrentPath);
#endif /* G_OS_WIN32 */

    if (strcmp(G_DIR_SEPARATOR_S,BrowserCurrentPath) == 0)
        gtk_widget_set_sensitive(BrowserButton,FALSE);
    else
        gtk_widget_set_sensitive(BrowserButton,TRUE);
}


/*
 * Return the current path
 */
gchar *Browser_Get_Current_Path (void)
{
    return BrowserCurrentPath;
}

/*
 * Reload the current directory.
 */
void Browser_Reload_Directory (void)
{
    if (BrowserTree && BrowserCurrentPath != NULL)
    {
        // Unselect files, to automatically reload the file of the directory
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserTree));
        if (selection)
        {
            gtk_tree_selection_unselect_all(selection);
        }
        Browser_Tree_Select_Dir(BrowserCurrentPath);
    }
}

/*
 * Set the current path (selected node) in browser as default path (within config variable).
 */
void Set_Current_Path_As_Default (void)
{
    if (DEFAULT_PATH_TO_MP3 != NULL)
        g_free(DEFAULT_PATH_TO_MP3);
    DEFAULT_PATH_TO_MP3 = g_strdup(BrowserCurrentPath);
    Statusbar_Message(_("New default path for files selected"),TRUE);
}

/*
 * When you press the key 'enter' in the BrowserEntry to validate the text (browse the directory)
 */
static void
Browser_Entry_Activated (void)
{
    const gchar *path_utf8;
    gchar *path;

    path_utf8 = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(BrowserEntryCombo))));
    Add_String_To_Combo_List(GTK_LIST_STORE(BrowserEntryModel), (gchar *)path_utf8);

    path = filename_from_display(path_utf8);

    Browser_Tree_Select_Dir(path);
    g_free(path);
}

/*
 * Set a text into the BrowserEntry (and don't activate it)
 */
void Browser_Entry_Set_Text (gchar *text)
{
    if (!text || !BrowserEntryCombo)
        return;

    gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(BrowserEntryCombo))),text);
}

/*
 * Button to go to parent directory
 */
static void
Browser_Parent_Button_Clicked (void)
{
    gchar *parent_dir, *path;

    parent_dir = Browser_Get_Current_Path();
    if (strlen(parent_dir)>1)
    {
        if ( parent_dir[strlen(parent_dir)-1]==G_DIR_SEPARATOR )
            parent_dir[strlen(parent_dir)-1] = '\0';
        path = g_path_get_dirname(parent_dir);

        Browser_Tree_Select_Dir(path);
        g_free(path);
    }
}

/*
 * Set a text into the BrowserLabel
 */
void Browser_Label_Set_Text (gchar *text)
{
    if (BrowserLabel && text)
        gtk_label_set_text(GTK_LABEL(BrowserLabel),text ? text : "");
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
 * Key Press events into browser list
 */
static gboolean
Browser_List_Stop_Timer (guint *flag)
{
    *flag = FALSE;
    return FALSE;
}

/*
 * Key press into browser list
 *   - Delete = delete file
 * Also tries to capture text input and relate it to files
 */
gboolean Browser_List_Key_Press (GtkWidget *list, GdkEvent *event, gpointer data)
{
    gchar *string, *current_filename = NULL, *current_filename_copy = NULL, *temp;
    static gchar *key_string = NULL;
    gint key_string_length;
    static guint BrowserListTimerId = 0;
    static gboolean timer_is_running = FALSE;
    gint row;
    gboolean valid;
    GdkEventKey *kevent;

    GtkTreePath *currentPath = NULL;
    GtkTreeIter  currentIter;
    ET_File     *currentETFile;

    GtkTreeModel     *fileListModel;
    GtkTreeSelection *fileSelection;

    g_return_val_if_fail (list != NULL, FALSE);

    fileListModel = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
    fileSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));

    kevent = (GdkEventKey *)event;
    if (event && event->type==GDK_KEY_PRESS)
    {
        if (gtk_tree_selection_count_selected_rows(fileSelection))
        {
            switch(kevent->keyval)
            {
                case GDK_KEY_Delete:
                    Action_Delete_Selected_Files();
                    return TRUE;
            }
        }
    }

    /*
     * Tries to select file corresponding to the character sequence entered
     */
    if ( strlen(kevent->string)>0 ) // Alphanumeric key?
    {
        // If the timer is running: concatenate the character of the pressed key, else starts a new string
        string = key_string;
        if (timer_is_running)
            key_string = g_strconcat(key_string,kevent->string,NULL);
        else
            key_string = g_strdup(kevent->string);
        g_free(string);

        // Remove the current timer
        if (BrowserListTimerId)
        {
            g_source_remove(BrowserListTimerId);
            BrowserListTimerId = 0;
        }
        // Start a new timer of 500ms
        BrowserListTimerId = g_timeout_add(500,(GSourceFunc)Browser_List_Stop_Timer,&timer_is_running);
        timer_is_running = TRUE;

        // Browse the list keeping the current classification
        for (row=0; row < gtk_tree_model_iter_n_children(fileListModel, NULL); row++)
        {
            if (row == 0)
                currentPath = gtk_tree_path_new_first();
            else
                gtk_tree_path_next(currentPath);

            valid = gtk_tree_model_get_iter(GTK_TREE_MODEL(fileListModel), &currentIter, currentPath);
            if (valid)
            {
                gtk_tree_model_get(GTK_TREE_MODEL(fileListModel), &currentIter,
                                   LIST_FILE_POINTER, &currentETFile,
                                   LIST_FILE_NAME,    &current_filename,
                                   -1);

                /* UTF-8 comparison */
                //key_string = Try_To_Validate_Utf8_String(key_string);
                if (g_utf8_validate(key_string, -1, NULL) == FALSE)
                {
                    temp = convert_to_utf8(key_string);
                    g_free(key_string);
                    key_string = temp;
                }

                key_string_length = g_utf8_strlen(key_string, -1);
                current_filename_copy = g_malloc(strlen(current_filename) + 1);
                g_utf8_strncpy(current_filename_copy, current_filename, key_string_length);

                temp = g_utf8_casefold(current_filename_copy, -1);
                g_free(current_filename_copy);
                current_filename_copy = temp;

                temp = g_utf8_casefold(key_string, -1);
                g_free(key_string);
                key_string = temp;

                if (g_utf8_collate(current_filename_copy,key_string)==0 )
                {
                    if (!gtk_tree_selection_iter_is_selected(fileSelection,&currentIter))
                        gtk_tree_selection_select_iter(fileSelection,&currentIter);

                    g_free(current_filename);
                    break;
                }
                g_free(current_filename);
            }
        }
        g_free(current_filename_copy);
        gtk_tree_path_free(currentPath);
    }
    return FALSE;
}

/*
 * Action for double/triple click
 */
static gboolean
Browser_List_Button_Press (GtkTreeView *treeView, GdkEventButton *event)
{
    if (!event)
        return FALSE;

    if (event->type==GDK_2BUTTON_PRESS && event->button==1)
    {
        /* Double left mouse click */
        // Select files of the same directory (useful when browsing sub-directories)
        GList *etfilelist = NULL;
        gchar *path_ref = NULL;
        gchar *patch_check = NULL;
        GtkTreePath *currentPath = NULL;

        if (!ETCore->ETFileDisplayed)
            return FALSE;

        // File taken as reference...
        path_ref = g_path_get_dirname( ((File_Name *)ETCore->ETFileDisplayed->FileNameCur->data)->value );

        // Search and select files of the same directory
        etfilelist = g_list_first(ETCore->ETFileDisplayedList);
        while (etfilelist)
        {
            // Path of the file to check if it is in the same directory
            patch_check = g_path_get_dirname( ((File_Name *)((ET_File *)etfilelist->data)->FileNameCur->data)->value );

            if ( path_ref && patch_check && strcmp(path_ref,patch_check)==0 )
            {
                // Use of 'currentPath' to try to increase speed. Indeed, in many
                // cases, the next file to select, is the next in the list
                currentPath = Browser_List_Select_File_By_Etfile2((ET_File *)etfilelist->data,TRUE,currentPath);
            }
            etfilelist = g_list_next(etfilelist);
            g_free(patch_check);
        }
        g_free(path_ref);
        if (currentPath)
            gtk_tree_path_free(currentPath);
    }else if (event->type==GDK_3BUTTON_PRESS && event->button==1)
    {
        /* Triple left mouse click */
        // Select all files of the list
        Action_Select_All_Files();
    }
    return FALSE;
}

/*
 * Collapse (close) tree recursively up to the root node.
 */
void Browser_Tree_Collapse (void)
{
    GtkTreePath *rootPath;

    g_return_if_fail (BrowserTree != NULL);

    gtk_tree_view_collapse_all(GTK_TREE_VIEW(BrowserTree));

#ifndef G_OS_WIN32
    /* But keep the main directory opened */
    rootPath = gtk_tree_path_new_first();
    gtk_tree_view_expand_to_path(GTK_TREE_VIEW(BrowserTree), rootPath);
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
void Browser_List_Set_Row_Visible (GtkTreeModel *treeModel, GtkTreeIter *rowIter)
{
    /*
     * TODO: Make this only scroll to the row if it is not visible
     * (like in easytag GTK1)
     * See function gtk_tree_view_get_visible_rect() ??
     */
    GtkTreePath *rowPath;

    g_return_if_fail (treeModel != NULL || rowIter != NULL);

    rowPath = gtk_tree_model_get_path(treeModel, rowIter);
    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(BrowserList), rowPath, NULL, FALSE, 0, 0);
    gtk_tree_path_free(rowPath);
}

/*
 * Triggers when a new node in the browser tree is selected
 * Do file-save confirmation, and then prompt the new dir to be loaded
 */
static gboolean
Browser_Tree_Node_Selected (GtkTreeSelection *selection, gpointer user_data)
{
    gchar *pathName, *pathName_utf8;
    static int counter = 0;
    GtkTreeIter selectedIter;
    GtkTreePath *selectedPath;

    if (!gtk_tree_selection_get_selected(selection, NULL, &selectedIter))
        return TRUE;
    selectedPath = gtk_tree_model_get_path(GTK_TREE_MODEL(directoryTreeModel), &selectedIter);

    /* Open the node */
    if (OPEN_SELECTED_BROWSER_NODE)
    {
        gtk_tree_view_expand_row(GTK_TREE_VIEW(BrowserTree), selectedPath, FALSE);
    }
    gtk_tree_path_free(selectedPath);

    /* Don't start a new reading, if another one is running... */
    if (ReadingDirectory == TRUE)
        return TRUE;

    //Browser_Tree_Set_Node_Visible(BrowserTree, selectedPath);
    gtk_tree_model_get(GTK_TREE_MODEL(directoryTreeModel), &selectedIter,
                       TREE_COLUMN_FULL_PATH, &pathName, -1);
    if (!pathName)
        return FALSE;

    /* Save the current displayed data */
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);
    Update_Command_Buttons_Sensivity(); // Not clean to put this here...

    /* Check if all files have been saved before changing the directory */
    if (CONFIRM_WHEN_UNSAVED_FILES && ET_Check_If_All_Files_Are_Saved() != TRUE)
    {
        GtkWidget *msgdialog;
        gint response;

        msgdialog = gtk_message_dialog_new(GTK_WINDOW(MainWindow),
                                           GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_QUESTION,
                                           GTK_BUTTONS_NONE,
                                           "%s",
                                           _("Some files have been modified but not saved"));
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msgdialog),"%s",_("Do you want to save them before changing the directory?"));
        gtk_dialog_add_buttons(GTK_DIALOG(msgdialog),GTK_STOCK_DISCARD,GTK_RESPONSE_NO,GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_SAVE,GTK_RESPONSE_YES,NULL);
        gtk_window_set_title(GTK_WINDOW(msgdialog),_("Confirm Directory Change"));

        response = gtk_dialog_run(GTK_DIALOG(msgdialog));
        gtk_widget_destroy(msgdialog);
        switch (response)
        {
            case GTK_RESPONSE_YES:
                if (Save_All_Files_With_Answer(FALSE)==-1)
                    return TRUE;
                break;
            case GTK_RESPONSE_NO:
                break;
            case GTK_RESPONSE_CANCEL:
            case GTK_RESPONSE_NONE:
                return TRUE;
        }
    }

    /* Memorize the current path */
    Browser_Update_Current_Path(pathName);

    /* Display the selected path into the BrowserEntry */
    pathName_utf8 = filename_to_display(pathName);
    gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(BrowserEntryCombo))), pathName_utf8);

    /* Start to read the directory */
    /* The first time, 'counter' is equal to zero. And if we don't want to load
     * directory on startup, we skip the 'reading', but newt we must read it each time */
    if (LOAD_ON_STARTUP || counter)
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
                if (gtk_tree_model_iter_parent(GTK_TREE_MODEL(directoryTreeModel),&parentIter,&selectedIter) )
                {
                    gtk_tree_selection_select_iter(selection,&parentIter);
                    selectedPath = gtk_tree_model_get_path(GTK_TREE_MODEL(directoryTreeModel), &parentIter);
                    gtk_tree_view_collapse_row(GTK_TREE_VIEW(BrowserTree),selectedPath);
                    if (OPEN_SELECTED_BROWSER_NODE)
                    {
                        gtk_tree_view_expand_row(GTK_TREE_VIEW(BrowserTree),selectedPath,FALSE);
                    }
                    gtk_tree_path_free(selectedPath);
                }
            }
        }

    }else
    {
        /* As we don't use the function 'Read_Directory' we must add this function here */
        Update_Command_Buttons_Sensivity();
    }
    counter++;

    g_free(pathName);
    g_free(pathName_utf8);
    return FALSE;
}


#ifdef G_OS_WIN32
static gboolean
Browser_Win32_Get_Drive_Root (gchar *drive, GtkTreeIter *rootNode, GtkTreePath **rootPath)
{
    gint root_index;
    gboolean found = FALSE;
    GtkTreeIter parentNode;
    gchar *nodeName;

    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(directoryTreeModel), &parentNode);

    // Find root of path, ie: the drive letter
    root_index = 0;

    do
    {
        gtk_tree_model_get(GTK_TREE_MODEL(directoryTreeModel), &parentNode,
                           TREE_COLUMN_FULL_PATH, &nodeName, -1);
        if (strncasecmp(drive,nodeName, strlen(drive)) == 0)
        {
            g_free(nodeName);
            found = TRUE;
            break;
        }
        root_index++;
    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(directoryTreeModel), &parentNode));

    if (!found) return FALSE;

    *rootNode = parentNode;
    *rootPath = gtk_tree_path_new_from_indices(root_index, -1);

    return TRUE;
}
#endif /* G_OS_WIN32 */


/*
 * Browser_Tree_Select_Dir: Select the directory corresponding to the 'path' in
 * the tree browser, but it doesn't read it!
 * Check if path is correct before selecting it. And returns 1 on success, else 0.
 *
 * - "current_path" is in file system encoding (not UTF-8)
 */
gboolean Browser_Tree_Select_Dir (const gchar *current_path)
{
    GtkTreePath *rootPath = NULL;
    GtkTreeIter parentNode, currentNode;
    gint index = 1; // Skip the first token as it is NULL due to leading /
    gchar **parts;
    gchar *nodeName;
    gchar *temp;

    g_return_val_if_fail (BrowserTree != NULL, FALSE);

    /* Load current_path */
    if(!current_path || !*current_path)
    {
        return TRUE;
    }

#ifdef G_OS_WIN32
    /* On win32 : stat("c:\path\to\dir") succeed, while stat("c:\path\to\dir\") fails */
    ET_Win32_Path_Remove_Trailing_Backslash(current_path);
#endif /* G_OS_WIN32 */


    /* Don't check here if the path is valid. It will be done later when
     * selecting a node in the tree */

    Browser_Update_Current_Path(current_path);

    parts = g_strsplit(current_path, G_DIR_SEPARATOR_S, 0);

    // Expand root node (fill parentNode and rootPath)
#ifdef G_OS_WIN32
    if (!Browser_Win32_Get_Drive_Root(parts[0], &parentNode, &rootPath))
        return FALSE;
#else /* !G_OS_WIN32 */
    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(directoryTreeModel), &parentNode);
    rootPath = gtk_tree_path_new_first();
#endif /* !G_OS_WIN32 */
    if (rootPath)
    {
        gtk_tree_view_expand_to_path(GTK_TREE_VIEW(BrowserTree), rootPath);
        gtk_tree_path_free(rootPath);
    }

    while (parts[index]) // it is NULL-terminated
    {
        if (strlen(parts[index]) == 0)
        {
            index++;
            continue;
        }

        if (!gtk_tree_model_iter_children(GTK_TREE_MODEL(directoryTreeModel), &currentNode, &parentNode))
        {
            break;
        }
        do
        {
            gtk_tree_model_get(GTK_TREE_MODEL(directoryTreeModel), &currentNode,
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
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(directoryTreeModel), &currentNode));

        parentNode = currentNode;
        rootPath = gtk_tree_model_get_path(GTK_TREE_MODEL(directoryTreeModel), &parentNode);
        if (rootPath)
        {
            gtk_tree_view_expand_to_path(GTK_TREE_VIEW(BrowserTree), rootPath);
            gtk_tree_path_free(rootPath);
        }
        index++;
    }

    rootPath = gtk_tree_model_get_path(GTK_TREE_MODEL(directoryTreeModel), &parentNode);
    if (rootPath)
    {
        gtk_tree_view_expand_to_path(GTK_TREE_VIEW(BrowserTree), rootPath);
        // Select the node to load the corresponding directory
        gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserTree)), rootPath);
        Browser_Tree_Set_Node_Visible(BrowserTree, rootPath);
        gtk_tree_path_free(rootPath);
    }

    g_strfreev(parts);
    return TRUE;
}

/*
 * Callback to select-row event
 * Displays the file info of the lowest selected file in the right-hand pane
 */
static void
Browser_List_Row_Selected (GtkTreeSelection *selection, gpointer data)
{
    GList *selectedRows;
    GtkTreePath *lastSelected;
    GtkTreeIter lastFile;
    ET_File *fileETFile;

    selectedRows = gtk_tree_selection_get_selected_rows(selection, NULL);

    /*
     * After a file is deleted, this function is called :
     * So we must handle the situation if no rows are selected
     */
    if (g_list_length(selectedRows) == 0)
    {
        g_list_foreach(selectedRows, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(selectedRows);
        return;
    }

    if (!LastBrowserListETFileSelected)
    {
        // Returns the last line selected (in ascending line order) to display the item
        lastSelected = (GtkTreePath *)g_list_last(selectedRows)->data;
        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(fileListModel), &lastFile, lastSelected))
            gtk_tree_model_get(GTK_TREE_MODEL(fileListModel), &lastFile,
                               LIST_FILE_POINTER, &fileETFile, -1);

        Action_Select_Nth_File_By_Etfile(fileETFile);
    }else
    {
        // The real last selected line
        Action_Select_Nth_File_By_Etfile(LastBrowserListETFileSelected);
    }

    g_list_foreach(selectedRows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(selectedRows);
}

/*
 * Loads the specified etfilelist into the browser list
 * Also supports optionally selecting a specific etfile
 * but be careful, this does not call Browser_List_Row_Selected !
 */
void Browser_List_Load_File_List (GList *etfilelist, ET_File *etfile_to_select)
{
    gboolean activate_bg_color = 0;
    GtkTreeIter rowIter;

    g_return_if_fail (BrowserList != NULL);

    gtk_list_store_clear(fileListModel);
    etfilelist = g_list_first(etfilelist);
    while (etfilelist)
    {
        guint fileKey                = ((ET_File *)etfilelist->data)->ETFileKey;
        gchar *current_filename_utf8 = ((File_Name *)((ET_File *)etfilelist->data)->FileNameCur->data)->value_utf8;
        gchar *basename_utf8         = g_path_get_basename(current_filename_utf8);
        File_Tag *FileTag            = ((File_Tag *)((ET_File *)etfilelist->data)->FileTag->data);
        gchar *track;

        // Change background color when changing directory (the first row must not be changed)
        if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(fileListModel), NULL) > 0)
        {
            gchar *dir1_utf8;
            gchar *dir2_utf8;
            gchar *previous_filename_utf8 = ((File_Name *)((ET_File *)etfilelist->prev->data)->FileNameCur->data)->value_utf8;

            dir1_utf8 = g_path_get_dirname(previous_filename_utf8);
            dir2_utf8 = g_path_get_dirname(current_filename_utf8);

            if (g_utf8_collate(dir1_utf8, dir2_utf8) != 0)
                activate_bg_color = !activate_bg_color;

            g_free(dir1_utf8);
            g_free(dir2_utf8);
        }

        // File list displays the current filename (name on HardDisk) and tag fields
        gtk_list_store_append(fileListModel, &rowIter);
        track = g_strconcat(FileTag->track ? FileTag->track : "",FileTag->track_total ? "/" : NULL,FileTag->track_total,NULL);
		
        gtk_list_store_set(fileListModel, &rowIter,
                           LIST_FILE_NAME,          basename_utf8,
                           LIST_FILE_POINTER,       etfilelist->data,
                           LIST_FILE_KEY,           fileKey,
                           LIST_FILE_OTHERDIR,      activate_bg_color,
                           LIST_FILE_TITLE,         FileTag->title,
                           LIST_FILE_ARTIST,        FileTag->artist,
                           LIST_FILE_ALBUM_ARTIST,  FileTag->album_artist,
						   LIST_FILE_ALBUM,         FileTag->album,
                           LIST_FILE_YEAR,          FileTag->year,
                           LIST_FILE_DISCNO,        FileTag->disc_number,
                           LIST_FILE_TRACK,         track,
                           LIST_FILE_GENRE,         FileTag->genre,
                           LIST_FILE_COMMENT,       FileTag->comment,
                           LIST_FILE_COMPOSER,      FileTag->composer,
                           LIST_FILE_ORIG_ARTIST,   FileTag->orig_artist,
                           LIST_FILE_COPYRIGHT,     FileTag->copyright,
                           LIST_FILE_URL,           FileTag->url,
                           LIST_FILE_ENCODED_BY,    FileTag->encoded_by,
                           -1);
        g_free(basename_utf8);
        g_free(track);

        if (etfile_to_select == etfilelist->data)
        {
            Browser_List_Select_File_By_Iter(&rowIter, TRUE);
            //ET_Display_File_Data_To_UI(etfilelist->data);
        }

        // Set appearance of the row
        Browser_List_Set_Row_Appearance(&rowIter);

        etfilelist = g_list_next(etfilelist);
    }
}


/*
 * Update state of files in the list after changes (without clearing the list model!)
 *  - Refresh 'filename' is file saved,
 *  - Change color is something changed on the file
 */
void Browser_List_Refresh_Whole_List (void)
{
    ET_File   *ETFile;
    File_Tag  *FileTag;
    File_Name *FileName;
    //GtkTreeIter iter;
    GtkTreePath *currentPath = NULL;
    GtkTreeIter iter;
    gint row;
    gchar *current_basename_utf8;
    gchar *track;
    gboolean valid;
    GtkWidget *TBViewMode;

    if (!ETCore->ETFileDisplayedList || !BrowserList
    ||  gtk_tree_model_iter_n_children(GTK_TREE_MODEL(fileListModel), NULL) == 0)
    {
        return;
    }

    TBViewMode = gtk_ui_manager_get_widget(UIManager, "/ToolBar/ViewModeToggle");

    // Browse the full list for changes
    //gtk_tree_model_get_iter_first(GTK_TREE_MODEL(fileListModel), &iter);
    //    g_print("above worked %d rows\n", gtk_tree_model_iter_n_children(GTK_TREE_MODEL(fileListModel), NULL));

    currentPath = gtk_tree_path_new_first();

    valid = gtk_tree_model_get_iter(GTK_TREE_MODEL(fileListModel), &iter, currentPath);
    while (valid)
    {
        // Refresh filename and other fields
        gtk_tree_model_get(GTK_TREE_MODEL(fileListModel), &iter,
                           LIST_FILE_POINTER, &ETFile, -1);

        FileName = (File_Name *)ETFile->FileNameCur->data;
        FileTag  = (File_Tag *)ETFile->FileTag->data;

        current_basename_utf8 = g_path_get_basename(FileName->value_utf8);
        track = g_strconcat(FileTag->track ? FileTag->track : "",FileTag->track_total ? "/" : NULL,FileTag->track_total,NULL);

        gtk_list_store_set(fileListModel, &iter,
                           LIST_FILE_NAME,          current_basename_utf8,
                           LIST_FILE_TITLE,         FileTag->title,
                           LIST_FILE_ARTIST,        FileTag->artist,
                           LIST_FILE_ALBUM_ARTIST,  FileTag->album_artist,
						   LIST_FILE_ALBUM,         FileTag->album,
                           LIST_FILE_YEAR,          FileTag->year,
                           LIST_FILE_DISCNO,        FileTag->disc_number,
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

        Browser_List_Set_Row_Appearance(&iter);

        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(fileListModel), &iter);
    }
    gtk_tree_path_free(currentPath);

    // When displaying Artist + Album lists => refresh also rows color
    if ( gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(TBViewMode)) )
    {

        for (row=0; row < gtk_tree_model_iter_n_children(GTK_TREE_MODEL(artistListModel), NULL); row++)
        {
            if (row == 0)
                currentPath = gtk_tree_path_new_first();
            else
                gtk_tree_path_next(currentPath);

            gtk_tree_model_get_iter(GTK_TREE_MODEL(artistListModel), &iter, currentPath);
            Browser_Artist_List_Set_Row_Appearance(&iter);
        }
        gtk_tree_path_free(currentPath);


        for (row=0; row < gtk_tree_model_iter_n_children(GTK_TREE_MODEL(albumListModel), NULL); row++)
        {
            if (row == 0)
                currentPath = gtk_tree_path_new_first();
            else
                gtk_tree_path_next(currentPath);

            gtk_tree_model_get_iter(GTK_TREE_MODEL(albumListModel), &iter, currentPath);
            Browser_Album_List_Set_Row_Appearance(&iter);
        }
        gtk_tree_path_free(currentPath);
    }
}


/*
 * Update state of one file in the list after changes (without clearing the clist!)
 *  - Refresh filename is file saved,
 *  - Change color is something change on the file
 */
void Browser_List_Refresh_File_In_List (ET_File *ETFile)
{
    GList *selectedRow = NULL;
    GtkWidget *TBViewMode;
    GtkTreeSelection *selection;
    GtkTreeIter selectedIter;
    GtkTreePath *currentPath = NULL;
    ET_File   *etfile;
    File_Tag  *FileTag;
    File_Name *FileName;
    gboolean row_found = FALSE;
    gchar *current_basename_utf8;
    gchar *track;
    gboolean valid;
    gint row;
    gchar *artist, *album;

    if (!ETCore->ETFileDisplayedList || !BrowserList || !ETFile ||
        gtk_tree_model_iter_n_children(GTK_TREE_MODEL(fileListModel), NULL) == 0)
    {
        return;
    }

    TBViewMode = gtk_ui_manager_get_widget(UIManager, "/ToolBar/ViewModeToggle");

    // Search the row of the modified file to update it (when found: row_found=TRUE)
    // 1/3. Get position of ETFile in ETFileList
    if (row_found == FALSE)
    {
        valid = gtk_tree_model_iter_nth_child (GTK_TREE_MODEL(fileListModel), &selectedIter, NULL, ETFile->IndexKey-1);
        if (valid)
        {
            gtk_tree_model_get(GTK_TREE_MODEL(fileListModel), &selectedIter,
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
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
        selectedRow = gtk_tree_selection_get_selected_rows(selection, NULL);
        if (selectedRow && selectedRow->data != NULL)
        {
            valid = gtk_tree_model_get_iter(GTK_TREE_MODEL(fileListModel), &selectedIter,
                                    (GtkTreePath*) selectedRow->data);
            if (valid)
            {
                gtk_tree_model_get(GTK_TREE_MODEL(fileListModel), &selectedIter,
                                   LIST_FILE_POINTER, &etfile, -1);
                if (ETFile->ETFileKey == etfile->ETFileKey)
                {
                    row_found = TRUE;
                }
            }
        }
        g_list_foreach(selectedRow, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(selectedRow);
    }

    // 3/3. Fails, now we browse the full list to find it
    if (row_found == FALSE)
    {
        valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(fileListModel), &selectedIter);
        while (valid)
        {
            gtk_tree_model_get(GTK_TREE_MODEL(fileListModel), &selectedIter,
                               LIST_FILE_POINTER, &etfile, -1);
            if (ETFile->ETFileKey == etfile->ETFileKey)
            {
                row_found = TRUE;
                break;
            } else
            {
                valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(fileListModel), &selectedIter);
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

    gtk_list_store_set(fileListModel, &selectedIter,
                       LIST_FILE_NAME,          current_basename_utf8,
                       LIST_FILE_TITLE,         FileTag->title,
                       LIST_FILE_ARTIST,        FileTag->artist,
                       LIST_FILE_ALBUM_ARTIST,  FileTag->album_artist,
					   LIST_FILE_ALBUM,         FileTag->album,
                       LIST_FILE_YEAR,          FileTag->year,
                       LIST_FILE_DISCNO,        FileTag->disc_number,
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

    // Change appearance (line to red) if filename changed
    Browser_List_Set_Row_Appearance(&selectedIter);

    // When displaying Artist + Album lists => refresh also rows color
    if ( gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(TBViewMode)) )
    {
        gchar *current_artist = ((File_Tag *)ETFile->FileTag->data)->artist;
        gchar *current_album  = ((File_Tag *)ETFile->FileTag->data)->album;

        for (row=0; row < gtk_tree_model_iter_n_children(GTK_TREE_MODEL(artistListModel), NULL); row++)
        {
            if (row == 0)
                currentPath = gtk_tree_path_new_first();
            else
                gtk_tree_path_next(currentPath);

            valid = gtk_tree_model_get_iter(GTK_TREE_MODEL(artistListModel), &selectedIter, currentPath);
            if (valid)
            {
                gtk_tree_model_get(GTK_TREE_MODEL(artistListModel), &selectedIter, ARTIST_NAME, &artist, -1);

                if ( (!current_artist && !artist)
                ||   (current_artist && artist && g_utf8_collate(current_artist,artist)==0) )
                {
                    // Set color of the row
                    Browser_Artist_List_Set_Row_Appearance(&selectedIter);
                    g_free(artist);
                    break;
                }
                g_free(artist);
            }
        }
        gtk_tree_path_free(currentPath); currentPath = NULL;

        //
        // FIX ME : see also if we must add a new line / or change list of the ETFile
        //
        for (row=0; row < gtk_tree_model_iter_n_children(GTK_TREE_MODEL(albumListModel), NULL); row++)
        {
            if (row == 0)
                currentPath = gtk_tree_path_new_first();
            else
                gtk_tree_path_next(currentPath);

            valid = gtk_tree_model_get_iter(GTK_TREE_MODEL(albumListModel), &selectedIter, currentPath);
            if (valid)
            {
                gtk_tree_model_get(GTK_TREE_MODEL(albumListModel), &selectedIter, ALBUM_NAME, &album, -1);

                if ( (!current_album && !album)
                ||   (current_album && album && g_utf8_collate(current_album,album)==0) )
                {
                    // Set color of the row
                    Browser_Album_List_Set_Row_Appearance(&selectedIter);
                    g_free(album);
                    break;
                }
                g_free(album);
            }
        }
        gtk_tree_path_free(currentPath); currentPath = NULL;

        //
        // FIX ME : see also if we must add a new line / or change list of the ETFile
        //
    }
}


/*
 * Set the appearance of the row
 *  - change background according LIST_FILE_OTHERDIR
 *  - change foreground according file status (saved or not)
 */
static void
Browser_List_Set_Row_Appearance (GtkTreeIter *iter)
{
    ET_File *rowETFile = NULL;
    gboolean otherdir = FALSE;
    GdkColor *backgroundcolor;
    //gchar *temp = NULL;

    if (iter == NULL)
        return;

    // Get the ETFile reference
    gtk_tree_model_get(GTK_TREE_MODEL(fileListModel), iter,
                       LIST_FILE_POINTER,   &rowETFile,
                       LIST_FILE_OTHERDIR,  &otherdir,
                       //LIST_FILE_NAME,      &temp,
                       -1);

    // Must change background color?
    if (otherdir)
        backgroundcolor = &LIGHT_BLUE;
    else
        backgroundcolor = NULL;

    // Set text to bold/red if 'filename' or 'tag' changed
    if ( ET_Check_If_File_Is_Saved(rowETFile) == FALSE )
    {
        if (CHANGED_FILES_DISPLAYED_TO_BOLD)
        {
            gtk_list_store_set(fileListModel, iter,
                               LIST_FONT_WEIGHT,    PANGO_WEIGHT_BOLD,
                               LIST_ROW_BACKGROUND, backgroundcolor,
                               LIST_ROW_FOREGROUND, NULL, -1);
        } else
        {
            gtk_list_store_set(fileListModel, iter,
                               LIST_FONT_WEIGHT,    PANGO_WEIGHT_NORMAL,
                               LIST_ROW_BACKGROUND, backgroundcolor,
                               LIST_ROW_FOREGROUND, &RED, -1);
        }
    } else
    {
        gtk_list_store_set(fileListModel, iter,
                           LIST_FONT_WEIGHT,    PANGO_WEIGHT_NORMAL,
                           LIST_ROW_BACKGROUND, backgroundcolor,
                           LIST_ROW_FOREGROUND, NULL ,-1);
    }

    // Update text fields
    // Don't do it here
    /*if (rowETFile)
    {
        File_Tag *FileTag = ((File_Tag *)((ET_File *)rowETFile)->FileTag->data);

        gtk_list_store_set(fileListModel, iter,
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
void Browser_List_Remove_File (ET_File *searchETFile)
{
    gint row;
    GtkTreePath *currentPath = NULL;
    GtkTreeIter currentIter;
    ET_File *currentETFile;
    gboolean valid;

    if (searchETFile == NULL)
        return;

    // Go through the file list until it is found
    for (row=0; row < gtk_tree_model_iter_n_children(GTK_TREE_MODEL(fileListModel), NULL); row++)
    {
        if (row == 0)
            currentPath = gtk_tree_path_new_first();
        else
            gtk_tree_path_next(currentPath);

        valid = gtk_tree_model_get_iter(GTK_TREE_MODEL(fileListModel), &currentIter, currentPath);
        if (valid)
        {
            gtk_tree_model_get(GTK_TREE_MODEL(fileListModel), &currentIter,
                               LIST_FILE_POINTER, &currentETFile, -1);

            if (currentETFile == searchETFile)
            {
                // Reinit this value to avoid a crash after deleting files...
                if (LastBrowserListETFileSelected == searchETFile)
                    LastBrowserListETFileSelected = NULL;

                gtk_list_store_remove(fileListModel, &currentIter);
                break;
            }
        }
    }
}

/*
 * Get ETFile pointer of a file from a Tree Iter
 */
ET_File *Browser_List_Get_ETFile_From_Path (GtkTreePath *path)
{
    GtkTreeIter iter;

    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(fileListModel), &iter, path))
        return NULL;

    return Browser_List_Get_ETFile_From_Iter(&iter);
}

/*
 * Get ETFile pointer of a file from a Tree Iter
 */
ET_File *Browser_List_Get_ETFile_From_Iter (GtkTreeIter *iter)
{
    ET_File *etfile;

    gtk_tree_model_get(GTK_TREE_MODEL(fileListModel), iter, LIST_FILE_POINTER, &etfile, -1);
    return etfile;
}


/*
 * Select the specified file in the list, by its ETFile
 */
void Browser_List_Select_File_By_Etfile (ET_File *searchETFile, gboolean select_it)
{
    GtkTreePath *currentPath = NULL;

    currentPath = Browser_List_Select_File_By_Etfile2(searchETFile, select_it, NULL);
    if (currentPath)
        gtk_tree_path_free(currentPath);
}
/*
 * Select the specified file in the list, by its ETFile
 *  - startPath : if set : starting path to try increase speed
 *  - returns allocated "currentPath" to free
 */
GtkTreePath *Browser_List_Select_File_By_Etfile2 (ET_File *searchETFile, gboolean select_it, GtkTreePath *startPath)
{
    gint row;
    GtkTreePath *currentPath = NULL;
    GtkTreeIter currentIter;
    ET_File *currentETFile;
    gboolean valid;

     if (searchETFile == NULL)
         return NULL;

    // If the path is used, we try the next item (to increase speed), as it is correct in many cases...
    if (startPath)
    {
        // Try the next path
        gtk_tree_path_next(startPath);
        valid = gtk_tree_model_get_iter(GTK_TREE_MODEL(fileListModel), &currentIter, startPath);
        if (valid)
        {
            gtk_tree_model_get(GTK_TREE_MODEL(fileListModel), &currentIter,
                               LIST_FILE_POINTER, &currentETFile, -1);
            // It is the good file?
            if (currentETFile == searchETFile)
            {
                Browser_List_Select_File_By_Iter(&currentIter, select_it);
                return startPath;
            }
        }
    }

    // Else, we try the whole list...
    // Go through the file list until it is found
    currentPath = gtk_tree_path_new_first();
    for (row=0; row < gtk_tree_model_iter_n_children(GTK_TREE_MODEL(fileListModel), NULL); row++)
    {
        valid = gtk_tree_model_get_iter(GTK_TREE_MODEL(fileListModel), &currentIter, currentPath);
        if (valid)
        {
            gtk_tree_model_get(GTK_TREE_MODEL(fileListModel), &currentIter,
                               LIST_FILE_POINTER, &currentETFile, -1);

            if (currentETFile == searchETFile)
            {
                Browser_List_Select_File_By_Iter(&currentIter, select_it);
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
Browser_List_Select_File_By_Iter (GtkTreeIter *rowIter, gboolean select_it)
{
    g_return_if_fail (BrowserList != NULL);

    if (select_it)
    {
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
        if (selection)
        {
            g_signal_handlers_block_by_func(G_OBJECT(selection),G_CALLBACK(Browser_List_Row_Selected),NULL);
            gtk_tree_selection_select_iter(selection, rowIter);
            g_signal_handlers_unblock_by_func(G_OBJECT(selection),G_CALLBACK(Browser_List_Row_Selected),NULL);
        }
    }
    Browser_List_Set_Row_Visible(GTK_TREE_MODEL(fileListModel), rowIter);
}

/*
 * Select the specified file in the list, by a string representation of an iter
 * e.g. output of gtk_tree_model_get_string_from_iter()
 */
ET_File *Browser_List_Select_File_By_Iter_String (const gchar* stringIter, gboolean select_it)
{
    GtkTreeIter iter;
    ET_File *current_etfile = NULL;

    g_return_val_if_fail (fileListModel != NULL || BrowserList != NULL, NULL);

    if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(fileListModel), &iter, stringIter))
    {
        gtk_tree_model_get(GTK_TREE_MODEL(fileListModel), &iter,
                           LIST_FILE_POINTER, &current_etfile, -1);

        if (select_it)
        {
            GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));

            // FIX ME : Why signal was blocked if selected? Don't remember...
            if (selection)
            {
                //g_signal_handlers_block_by_func(G_OBJECT(selection),G_CALLBACK(Browser_List_Row_Selected),NULL);
                gtk_tree_selection_select_iter(selection, &iter);
                //g_signal_handlers_unblock_by_func(G_OBJECT(selection),G_CALLBACK(Browser_List_Row_Selected),NULL);
            }
        }
        Browser_List_Set_Row_Visible(GTK_TREE_MODEL(fileListModel), &iter);
    }

    return current_etfile;
}

/*
 * Select the specified file in the list, by fuzzy string matching based on
 * the Damerau-Levenshtein Metric (patch from Santtu Lakkala - 23/08/2004)
 */
ET_File *Browser_List_Select_File_By_DLM (const gchar* string, gboolean select_it)
{
    GtkTreeIter iter;
    GtkTreeIter iter2;
    GtkTreeSelection *selection;
    ET_File *current_etfile = NULL, *retval = NULL;
    gchar *current_filename = NULL, *current_title = NULL;
    int max = 0, this;

    g_return_val_if_fail (fileListModel != NULL || BrowserList != NULL, NULL);

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(fileListModel), &iter))
    {
        do
        {
            gtk_tree_model_get(GTK_TREE_MODEL(fileListModel), &iter,
                               LIST_FILE_NAME,    &current_filename,
                               LIST_FILE_POINTER, &current_etfile, -1);
            current_title = ((File_Tag *)current_etfile->FileTag->data)->title;

            if ((this = dlm((current_title ? current_title : current_filename), string)) > max) // See "dlm.c"
            {
                max = this;
                iter2 = iter;
                retval = current_etfile;
            }
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(fileListModel), &iter));

        if (select_it)
        {
            selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
            if (selection)
            {
                g_signal_handlers_block_by_func(G_OBJECT(selection),G_CALLBACK(Browser_List_Row_Selected),NULL);
                gtk_tree_selection_select_iter(selection, &iter2);
                g_signal_handlers_unblock_by_func(G_OBJECT(selection),G_CALLBACK(Browser_List_Row_Selected),NULL);
            }
        }
        Browser_List_Set_Row_Visible(GTK_TREE_MODEL(fileListModel), &iter2);
    }
    return retval;
}


/*
 * Clear all entries on the file list
 */
void Browser_List_Clear()
{
    gtk_list_store_clear(fileListModel);
    gtk_list_store_clear(artistListModel);
    gtk_list_store_clear(albumListModel);

}

/*
 * Refresh the list sorting (call me after SORTING_FILE_MODE has changed)
 */
void Browser_List_Refresh_Sort (void)
{
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(fileListModel), 0, Browser_List_Sort_Func, NULL, NULL);
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
    //gchar *text1;
    //gchar *text2;
    gint result = 0;

    gtk_tree_model_get(model, a, LIST_FILE_POINTER, &ETFile1, -1);
    gtk_tree_model_get(model, b, LIST_FILE_POINTER, &ETFile2, -1);
    //gtk_tree_model_get(model, a, LIST_FILE_POINTER, &ETFile1, LIST_FILE_NAME, &text1, -1);
    //gtk_tree_model_get(model, b, LIST_FILE_POINTER, &ETFile2, LIST_FILE_NAME, &text2, -1);

    switch (SORTING_FILE_MODE)
    {
        case SORTING_UNKNOWN:
        case SORTING_BY_ASCENDING_FILENAME:
            result = ET_Comp_Func_Sort_File_By_Ascending_Filename(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_FILENAME:
            result = ET_Comp_Func_Sort_File_By_Descending_Filename(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_TRACK_NUMBER:
            result = ET_Comp_Func_Sort_File_By_Ascending_Track_Number(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_TRACK_NUMBER:
            result = ET_Comp_Func_Sort_File_By_Descending_Track_Number(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_CREATION_DATE:
            result = ET_Comp_Func_Sort_File_By_Ascending_Creation_Date(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_CREATION_DATE:
            result = ET_Comp_Func_Sort_File_By_Descending_Creation_Date(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_TITLE:
            result = ET_Comp_Func_Sort_File_By_Ascending_Title(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_TITLE:
            result = ET_Comp_Func_Sort_File_By_Descending_Title(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_ARTIST:
            result = ET_Comp_Func_Sort_File_By_Ascending_Artist(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_ARTIST:
            result = ET_Comp_Func_Sort_File_By_Descending_Artist(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_ALBUM_ARTIST:
            result = ET_Comp_Func_Sort_File_By_Ascending_Album_Artist(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_ALBUM_ARTIST:
            result = ET_Comp_Func_Sort_File_By_Descending_Album_Artist(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_ALBUM:
            result = ET_Comp_Func_Sort_File_By_Ascending_Album(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_ALBUM:
            result = ET_Comp_Func_Sort_File_By_Descending_Album(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_YEAR:
            result = ET_Comp_Func_Sort_File_By_Ascending_Year(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_YEAR:
            result = ET_Comp_Func_Sort_File_By_Descending_Year(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_GENRE:
            result = ET_Comp_Func_Sort_File_By_Ascending_Genre(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_GENRE:
            result = ET_Comp_Func_Sort_File_By_Descending_Genre(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_COMMENT:
            result = ET_Comp_Func_Sort_File_By_Ascending_Comment(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_COMMENT:
            result = ET_Comp_Func_Sort_File_By_Descending_Comment(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_COMPOSER:
            result = ET_Comp_Func_Sort_File_By_Ascending_Composer(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_COMPOSER:
            result = ET_Comp_Func_Sort_File_By_Descending_Composer(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_ORIG_ARTIST:
            result = ET_Comp_Func_Sort_File_By_Ascending_Orig_Artist(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_ORIG_ARTIST:
            result = ET_Comp_Func_Sort_File_By_Descending_Orig_Artist(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_COPYRIGHT:
            result = ET_Comp_Func_Sort_File_By_Ascending_Copyright(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_COPYRIGHT:
            result = ET_Comp_Func_Sort_File_By_Descending_Copyright(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_URL:
            result = ET_Comp_Func_Sort_File_By_Ascending_Url(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_URL:
            result = ET_Comp_Func_Sort_File_By_Descending_Url(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_ENCODED_BY:
            result = ET_Comp_Func_Sort_File_By_Ascending_Encoded_By(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_ENCODED_BY:
            result = ET_Comp_Func_Sort_File_By_Descending_Encoded_By(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_FILE_TYPE:
            result = ET_Comp_Func_Sort_File_By_Ascending_File_Type(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_FILE_TYPE:
            result = ET_Comp_Func_Sort_File_By_Descending_File_Type(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_FILE_SIZE:
            result = ET_Comp_Func_Sort_File_By_Ascending_File_Size(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_FILE_SIZE:
            result = ET_Comp_Func_Sort_File_By_Descending_File_Size(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_FILE_DURATION:
            result = ET_Comp_Func_Sort_File_By_Ascending_File_Duration(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_FILE_DURATION:
            result = ET_Comp_Func_Sort_File_By_Descending_File_Duration(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_FILE_BITRATE:
            result = ET_Comp_Func_Sort_File_By_Ascending_File_Bitrate(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_FILE_BITRATE:
            result = ET_Comp_Func_Sort_File_By_Descending_File_Bitrate(ETFile1, ETFile2);
            break;
        case SORTING_BY_ASCENDING_FILE_SAMPLERATE:
            result = ET_Comp_Func_Sort_File_By_Ascending_File_Samplerate(ETFile1, ETFile2);
            break;
        case SORTING_BY_DESCENDING_FILE_SAMPLERATE:
            result = ET_Comp_Func_Sort_File_By_Descending_File_Samplerate(ETFile1, ETFile2);
            break;
    }

    // Frees allocated item from gtk_tree_model_get...
    //g_free(text1);
    //g_free(text2);

    return result;
}

/*
 * Select all files on the file list
 */
void Browser_List_Select_All_Files (void)
{
    GtkTreeSelection *selection;

    g_return_if_fail (BrowserList != NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
    if (selection)
    {
        // Must block the select signal to avoid the selecting, one by one, of all files in the main files list
        g_signal_handlers_block_by_func(G_OBJECT(selection),G_CALLBACK(Browser_List_Row_Selected),NULL);
        gtk_tree_selection_select_all(selection);
        g_signal_handlers_unblock_by_func(G_OBJECT(selection),G_CALLBACK(Browser_List_Row_Selected),NULL);
    }
}

/*
 * Unselect all files on the file list
 */
void Browser_List_Unselect_All_Files (void)
{
    GtkTreeSelection *selection;

    g_return_if_fail (BrowserList != NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
    if (selection)
    {
        gtk_tree_selection_unselect_all(selection);
    }
}

/*
 * Invert the selection of the file list
 */
void Browser_List_Invert_File_Selection (void)
{
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    gboolean valid;

    g_return_if_fail (fileListModel != NULL || BrowserList != NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
    if (selection)
    {
        /* Must block the select signal to avoid selecting all files (one by one) in the main files list */
        g_signal_handlers_block_by_func(G_OBJECT(selection), G_CALLBACK(Browser_List_Row_Selected), NULL);
        valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(fileListModel), &iter);
        while (valid)
        {
            if (gtk_tree_selection_iter_is_selected(selection, &iter))
            {
                gtk_tree_selection_unselect_iter(selection, &iter);
            } else
            {
                gtk_tree_selection_select_iter(selection, &iter);
            }
            valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(fileListModel), &iter);
        }
        g_signal_handlers_unblock_by_func(G_OBJECT(selection), G_CALLBACK(Browser_List_Row_Selected), NULL);
    }
}


void Browser_Artist_List_Load_Files (ET_File *etfile_to_select)
{
    GList *ArtistList;
    GList *AlbumList;
    GList *etfilelist;
    ET_File *etfile;
    GList *list;
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    gchar *artistname, *artist_to_select = NULL;

    g_return_if_fail (BrowserArtistList != NULL);

    if (etfile_to_select)
        artist_to_select = ((File_Tag *)etfile_to_select->FileTag->data)->artist;

    gtk_list_store_clear(artistListModel);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserArtistList));

    ArtistList = ETCore->ETArtistAlbumFileList;
    while (ArtistList)
    {
        gint   nbr_files = 0;

        // Insert a line for each artist
        AlbumList  = (GList *)ArtistList->data;
        etfilelist = (GList *)AlbumList->data;
        etfile     = (ET_File *)etfilelist->data;
        artistname = ((File_Tag *)etfile->FileTag->data)->artist;

        // Third column text : number of files
        list = g_list_first(AlbumList);
        while (list)
        {
            nbr_files += g_list_length(g_list_first((GList *)list->data));
            list = list->next;
        }

        // Add the new row
        gtk_list_store_append(artistListModel, &iter);
        gtk_list_store_set(artistListModel, &iter,
                           ARTIST_PIXBUF,             "easytag-artist",
                           ARTIST_NAME,               artistname,
                           ARTIST_NUM_ALBUMS,         g_list_length(g_list_first(AlbumList)),
                           ARTIST_NUM_FILES,          nbr_files,
                           ARTIST_ALBUM_LIST_POINTER, AlbumList,
                           -1);

        // Todo: Use something better than string comparison
        if ( (!artistname && !artist_to_select)
        ||   (artistname  &&  artist_to_select && strcmp(artistname,artist_to_select) == 0) )
        {
            GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(artistListModel), &iter);

            g_signal_handlers_block_by_func(G_OBJECT(selection),G_CALLBACK(Browser_Artist_List_Row_Selected),NULL);
            gtk_tree_selection_select_iter(selection, &iter);
            g_signal_handlers_unblock_by_func(G_OBJECT(selection),G_CALLBACK(Browser_Artist_List_Row_Selected),NULL);

            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(BrowserArtistList), path, NULL, FALSE, 0, 0);
            gtk_tree_path_free(path);

            Browser_Album_List_Load_Files(AlbumList, etfile_to_select);

            // Now that we've found the artist, no need to continue searching
            artist_to_select = NULL;
        }

        // Set color of the row
        Browser_Artist_List_Set_Row_Appearance(&iter);

        ArtistList = ArtistList->next;
    }

    // Select the first line if we weren't asked to select anything
    if (!etfile_to_select && gtk_tree_model_get_iter_first(GTK_TREE_MODEL(artistListModel), &iter))
    {
        gtk_tree_model_get(GTK_TREE_MODEL(artistListModel), &iter,
                           ARTIST_ALBUM_LIST_POINTER, &AlbumList,
                           -1);
        ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);
        Browser_Album_List_Load_Files(AlbumList,NULL);
    }
}


/*
 * Callback to select-row event
 */
static void
Browser_Artist_List_Row_Selected (GtkTreeSelection* selection, gpointer data)
{
    GList *AlbumList;
    GtkTreeIter iter;

    // Display the relevant albums
    if(!gtk_tree_selection_get_selected(selection, NULL, &iter))
        return; // We might be called with no row selected

    // Save the current displayed data
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    gtk_tree_model_get(GTK_TREE_MODEL(artistListModel), &iter,
                       ARTIST_ALBUM_LIST_POINTER, &AlbumList, -1);
    Browser_Album_List_Load_Files(AlbumList, NULL);
}

/*
 * Set the color of the row of BrowserArtistList
 */
static void
Browser_Artist_List_Set_Row_Appearance (GtkTreeIter *iter)
{
    GList *AlbumList;
    GList *etfilelist;
    gboolean not_all_saved = FALSE;

    gtk_tree_model_get(GTK_TREE_MODEL(artistListModel), iter,
                       ARTIST_ALBUM_LIST_POINTER, &AlbumList, -1);

    // Change the style (red/bold) of the row if one of the files was changed
    while (AlbumList)
    {
        etfilelist = (GList *)AlbumList->data;
        while (etfilelist)
        {
            if ( ET_Check_If_File_Is_Saved((ET_File *)etfilelist->data) == FALSE )
            {
                if (CHANGED_FILES_DISPLAYED_TO_BOLD)
                {
                    // Set the font-style to "bold"
                    gtk_list_store_set(artistListModel, iter,
                                       ARTIST_FONT_WEIGHT, PANGO_WEIGHT_BOLD, -1);
                } else
                {
                    // Set the background-color to "red"
                    gtk_list_store_set(artistListModel, iter,
                                       ARTIST_FONT_WEIGHT,    PANGO_WEIGHT_NORMAL,
                                       ARTIST_ROW_FOREGROUND, &RED, -1);
                }
                not_all_saved = TRUE;
                break;
            }
            etfilelist = etfilelist->next;
        }
        AlbumList = AlbumList->next;
    }

    // Reset style if all files saved
    if (not_all_saved == FALSE)
    {
        gtk_list_store_set(artistListModel, iter,
                           ARTIST_FONT_WEIGHT,    PANGO_WEIGHT_NORMAL,
                           ARTIST_ROW_FOREGROUND, NULL, -1);
    }

}



/*
 * Load the list of Albums for each Artist
 */
static void
Browser_Album_List_Load_Files (GList *albumlist, ET_File *etfile_to_select)
{
    GList *AlbumList;
    GList *etfilelist = NULL;
    ET_File *etfile;
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    gchar *albumname, *album_to_select = NULL;

    g_return_if_fail (BrowserAlbumList != NULL);

    if (etfile_to_select)
        album_to_select = ((File_Tag *)etfile_to_select->FileTag->data)->album;

    gtk_list_store_clear(albumListModel);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserAlbumList));

    // Create a first row to select all albums of the artist
    // FIX ME : the attached list must be freed!
    AlbumList = albumlist;
    while (AlbumList)
    {
        GList *etfilelist_tmp;
        etfilelist_tmp = (GList *)AlbumList->data;
        // We must make a copy to not "alter" the initial list by appending another list
        etfilelist_tmp = g_list_copy(etfilelist_tmp);
        etfilelist = g_list_concat(etfilelist, etfilelist_tmp);

        AlbumList = AlbumList->next;
    }
    gtk_list_store_append(albumListModel, &iter);
    gtk_list_store_set(albumListModel, &iter,
                       ALBUM_NAME,                _("<All albums>"),
                       ALBUM_NUM_FILES,           g_list_length(g_list_first(etfilelist)),
                       ALBUM_ETFILE_LIST_POINTER, etfilelist,
                       -1);

    // Create a line for each album of the artist
    AlbumList = albumlist;
    while (AlbumList)
    {
        // Insert a line for each album
        etfilelist = (GList *)AlbumList->data;
        etfile     = (ET_File *)etfilelist->data;
        albumname  = ((File_Tag *)etfile->FileTag->data)->album;

        // Add the new row
        gtk_list_store_append(albumListModel, &iter);
        gtk_list_store_set(albumListModel, &iter,
                           ALBUM_PIXBUF,              "easytag-album",
                           ALBUM_NAME,                albumname,
                           ALBUM_NUM_FILES,           g_list_length(g_list_first(etfilelist)),
                           ALBUM_ETFILE_LIST_POINTER, etfilelist,
                           -1);

        if ( (!albumname && !album_to_select)
        ||   (albumname &&  album_to_select && strcmp(albumname,album_to_select) == 0) )
        {
            GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(albumListModel), &iter);

            g_signal_handlers_block_by_func(G_OBJECT(selection),G_CALLBACK(Browser_Album_List_Row_Selected),NULL);
            gtk_tree_selection_select_iter(selection, &iter);
            g_signal_handlers_unblock_by_func(G_OBJECT(selection),G_CALLBACK(Browser_Album_List_Row_Selected),NULL);

            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(BrowserAlbumList), path, NULL, FALSE, 0, 0);
            gtk_tree_path_free(path);

            ET_Set_Displayed_File_List(etfilelist);
            Browser_List_Load_File_List(etfilelist,etfile_to_select);

            // Now that we've found the album, no need to continue searching
            album_to_select = NULL;
        }

        // Set color of the row
        Browser_Album_List_Set_Row_Appearance(&iter);

        AlbumList = AlbumList->next;
    }

    // Select the first line if we werent asked to select anything
    if (!etfile_to_select && gtk_tree_model_get_iter_first(GTK_TREE_MODEL(albumListModel), &iter))
    {
        gtk_tree_model_get(GTK_TREE_MODEL(albumListModel), &iter,
                           ALBUM_ETFILE_LIST_POINTER, &etfilelist,
                           -1);
        ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

        // Set the attached list as "Displayed List"
        ET_Set_Displayed_File_List(etfilelist);
        Browser_List_Load_File_List(etfilelist, NULL);

        // Displays the first item
        Action_Select_Nth_File_By_Etfile((ET_File *)etfilelist->data);
    }
}

/*
 * Callback to select-row event
 */
static void
Browser_Album_List_Row_Selected (GtkTreeSelection *selection, gpointer data)
{
    GList *etfilelist;
    GtkTreeIter iter;


    // We might be called with no rows selected
    if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
        return;

    gtk_tree_model_get(GTK_TREE_MODEL(albumListModel), &iter,
                       ALBUM_ETFILE_LIST_POINTER, &etfilelist, -1);

    // Save the current displayed data
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    // Set the attached list as "Displayed List"
    ET_Set_Displayed_File_List(etfilelist);

    Browser_List_Load_File_List(etfilelist, NULL);

    // Displays the first item
    Action_Select_Nth_File_By_Etfile((ET_File *)etfilelist->data);
}


/*
 * Set the color of the row of BrowserAlbumList
 */
static void
Browser_Album_List_Set_Row_Appearance (GtkTreeIter *iter)
{
    GList *etfilelist;
    gboolean not_all_saved = FALSE;

    gtk_tree_model_get(GTK_TREE_MODEL(albumListModel), iter,
                       ALBUM_ETFILE_LIST_POINTER, &etfilelist, -1);

    // Change the style (red/bold) of the row if one of the files was changed
    while (etfilelist)
    {
        if ( ET_Check_If_File_Is_Saved((ET_File *)etfilelist->data) == FALSE )
        {
            if (CHANGED_FILES_DISPLAYED_TO_BOLD)
            {
                // Set the font-style to "bold"
                gtk_list_store_set(albumListModel, iter,
                                   ALBUM_FONT_WEIGHT, PANGO_WEIGHT_BOLD, -1);
            } else
            {
                // Set the background-color to "red"
                gtk_list_store_set(albumListModel, iter,
                                   ALBUM_FONT_WEIGHT,    PANGO_WEIGHT_NORMAL,
                                   ALBUM_ROW_FOREGROUND, &RED, -1);
            }
            not_all_saved = TRUE;
            break;
        }
        etfilelist = etfilelist->next;
    }

    // Reset style if all files saved
    if (not_all_saved == FALSE)
    {
        gtk_list_store_set(albumListModel, iter,
                           ALBUM_FONT_WEIGHT,    PANGO_WEIGHT_NORMAL,
                           ALBUM_ROW_FOREGROUND, NULL, -1);
    }
}

void Browser_Display_Tree_Or_Artist_Album_List (void)
{
    ET_File *etfile = ETCore->ETFileDisplayed; // ETFile to display again after changing browser view
    GtkWidget *TBViewMode;

    // Save the current displayed data
    ET_Save_File_Data_From_UI(ETCore->ETFileDisplayed);

    // Toggle button to switch view
    TBViewMode = gtk_ui_manager_get_widget(UIManager, "/ToolBar/ViewModeToggle");

    // Button pressed in the toolbar
    if ( gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(TBViewMode)) )
    {
        /*
         * Artist + Album view
         */

        // Display Artist + Album lists
        gtk_notebook_set_current_page(GTK_NOTEBOOK(BrowserNoteBook),1);
        ET_Create_Artist_Album_File_List();
        Browser_Artist_List_Load_Files(etfile);

    }else
    {

        /*
         * Browser (classic) view
         */
        // Set the whole list as "Displayed list"
        ET_Set_Displayed_File_List(ETCore->ETFileList);

        // Display Tree Browser
        gtk_notebook_set_current_page(GTK_NOTEBOOK(BrowserNoteBook),0);
        Browser_List_Load_File_List(ETCore->ETFileDisplayedList, etfile);

        // Displays the first file if nothing specified
        if (!etfile)
        {
            GList *etfilelist = ET_Displayed_File_List_First();
            if (etfilelist)
                etfile = (ET_File *)etfilelist->data;
            Action_Select_Nth_File_By_Etfile(etfile);
        }
    }

    //ET_Display_File_Data_To_UI(etfile); // Causes a crash
}

/*
 * Disable (FALSE) / Enable (TRUE) all user widgets in the browser area (Tree + List + Entry)
 */
void Browser_Area_Set_Sensitive (gboolean activate)
{
    gtk_widget_set_sensitive(GTK_WIDGET(BrowserEntryCombo),activate);
    gtk_widget_set_sensitive(GTK_WIDGET(BrowserTree),      activate);
    gtk_widget_set_sensitive(GTK_WIDGET(BrowserList),      activate);
    gtk_widget_set_sensitive(GTK_WIDGET(BrowserArtistList),activate);
    gtk_widget_set_sensitive(GTK_WIDGET(BrowserAlbumList), activate);
    gtk_widget_set_sensitive(GTK_WIDGET(BrowserButton),    activate);
    gtk_widget_set_sensitive(GTK_WIDGET(BrowserLabel),     activate);
}


/*
 * Browser_Popup_Menu_Handler : displays the corresponding menu
 */
static gboolean
Browser_Popup_Menu_Handler (GtkMenu *menu, GdkEventButton *event)
{
    if (event && (event->type==GDK_BUTTON_PRESS) && (event->button == 3))
    {
        gtk_menu_popup(menu,NULL,NULL,NULL,NULL,event->button,event->time);
        return TRUE;
    }
    return FALSE;
}

/*
 * Destroy the whole tree up to the root node
 */
static void
Browser_Tree_Initialize (void)
{
    GtkTreeIter parent_iter;
    GtkTreeIter dummy_iter;

    g_return_if_fail (directoryTreeModel != NULL);

    gtk_tree_store_clear(directoryTreeModel);

#ifdef G_OS_WIN32
    /* Code strangely familiar with gtkfilesystemwin32.c */

    GdkPixbuf *drive_pixmap;
    DWORD drives;
    UINT drive_type;
    gchar drive[4] = "A:/";
    gchar drive_backslashed[5] = "A:\\";
    gchar drive_slashless[3] = "A:";
    gchar drive_label[256];

    drives = GetLogicalDrives();
    if (!drives)
        g_warning ("GetLogicalDrives failed.");

    while (drives && drive[0] <= 'Z')
    {
        if (drives & 1)
        {
            char *drive_dir_name;

            drive_type = GetDriveType(drive_backslashed);

            // DRIVE_REMOVABLE   2
            // DRIVE_FIXED       3
            // DRIVE_REMOTE      4
            // DRIVE_CDROM       5
            // DRIVE_RAMDISK     6
            // DRIVE_UNKNOWN     0
            // DRIVE_NO_ROOT_DIR 1
            switch(drive_type)
            {
                case DRIVE_FIXED:
                    drive_pixmap = harddrive_pixmap;
                    break;
                case DRIVE_REMOVABLE:
                    drive_pixmap = removable_pixmap;
                    break;
                case DRIVE_CDROM:
                    drive_pixmap = cdrom_pixmap;
                    break;
                case DRIVE_REMOTE:
                    drive_pixmap = network_pixmap;
                    break;
                case DRIVE_RAMDISK:
                    drive_pixmap = ramdisk_pixmap;
                    break;
                default:
                    drive_pixmap = closed_folder_pixmap;
            }

            drive_label[0] = 0;

            GetVolumeInformation(drive_backslashed, drive_label, 256, NULL, NULL, NULL, NULL, 0);

            /* Drive letter first so alphabetical drive list order works */
            drive_dir_name = g_strconcat("(", drive_slashless, ") ", drive_label, NULL);

            gtk_tree_store_append(directoryTreeModel, &parent_iter, NULL);
            gtk_tree_store_set(directoryTreeModel,      &parent_iter,
                               TREE_COLUMN_DIR_NAME,    drive_dir_name,
                               TREE_COLUMN_FULL_PATH,   drive_backslashed,
                               TREE_COLUMN_HAS_SUBDIR,  TRUE,
                               TREE_COLUMN_SCANNED,     FALSE,
                               TREE_COLUMN_PIXBUF,      drive_pixmap,
                               -1);
            // Insert dummy node
            gtk_tree_store_append(directoryTreeModel, &dummy_iter, &parent_iter);

            g_free(drive_dir_name);
        }
        drives >>= 1;
        drive[0]++;
        drive_backslashed[0]++;
        drive_slashless[0]++;
    }

#else /* !G_OS_WIN32 */
    gtk_tree_store_append(directoryTreeModel, &parent_iter, NULL);
    gtk_tree_store_set(directoryTreeModel, &parent_iter,
                       TREE_COLUMN_DIR_NAME,    G_DIR_SEPARATOR_S,
                       TREE_COLUMN_FULL_PATH,   G_DIR_SEPARATOR_S,
                       TREE_COLUMN_HAS_SUBDIR,  TRUE,
                       TREE_COLUMN_SCANNED,     FALSE,
                       TREE_COLUMN_PIXBUF,      closed_folder_pixmap,
                       -1);
    // insert dummy node
    gtk_tree_store_append(directoryTreeModel, &dummy_iter, &parent_iter);
#endif /* !G_OS_WIN32 */
}

/*
 * Browser_Tree_Rebuild: Refresh the tree browser by destroying it and rebuilding it.
 * Opens tree nodes corresponding to 'path_to_load' if this parameter isn't NULL.
 * If NULL, selects the current path.
 */
void Browser_Tree_Rebuild (gchar *path_to_load)
{
    gchar *current_path = NULL;
    GtkTreeSelection *selection;

    /* May be called from GtkUIManager callback */
    if (GTK_IS_ACTION(path_to_load))
        path_to_load = NULL;

    if (path_to_load != NULL)
    {
        Browser_Tree_Initialize();
        Browser_Tree_Select_Dir(path_to_load);
        return;
    }

    /* Memorize the current path to load it again at the end */
    current_path = Browser_Tree_Get_Path_Of_Selected_Node();
    if (current_path==NULL && BrowserEntryCombo)
    {
        /* If no node selected, get path from BrowserEntry or default path */
        if (BrowserCurrentPath != NULL)
            current_path = g_strdup(BrowserCurrentPath);
        else if (g_utf8_strlen(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(BrowserEntryCombo)))), -1) > 0)
            current_path = filename_from_display(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(BrowserEntryCombo)))));
        else
            current_path = g_strdup(DEFAULT_PATH_TO_MP3);
    }

    Browser_Tree_Initialize();
    /* Select again the memorized path without loading files */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserTree));
    if (selection)
    {
        g_signal_handlers_block_by_func(G_OBJECT(selection),G_CALLBACK(Browser_Tree_Node_Selected),NULL);
        Browser_Tree_Select_Dir(current_path);
        g_signal_handlers_unblock_by_func(G_OBJECT(selection),G_CALLBACK(Browser_Tree_Node_Selected),NULL);
    }
    g_free(current_path);

    Update_Command_Buttons_Sensivity();
}

/*
 * Renames a directory
 * last_path:
 * new_path:
 * Parameters are non-utf8!
 */
static void
Browser_Tree_Rename_Directory (const gchar *last_path, const gchar *new_path)
{

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

    /*
     * Find the existing tree entry
     */
    textsplit = g_strsplit(last_path, G_DIR_SEPARATOR_S, 0);

#ifdef G_OS_WIN32
    if (!Browser_Win32_Get_Drive_Root(textsplit[0], &iter, &parentpath))
        return;
#else /* !G_OS_WIN32 */
    parentpath = gtk_tree_path_new_first();
#endif /* !G_OS_WIN32 */

    for (i = 1; textsplit[i] != NULL; i++)
    {
        gtk_tree_model_get_iter(GTK_TREE_MODEL(directoryTreeModel), &iter, parentpath);
        childpath = Find_Child_Node(&iter, textsplit[i]);
        if (childpath == NULL)
        {
            // ERROR! Could not find it!
            gchar *text_utf8 = filename_to_display(textsplit[i]);
            Log_Print(LOG_ERROR,_("Error: Searching for %s, could not find node %s in tree."), last_path, text_utf8);
            g_strfreev(textsplit);
            g_free(text_utf8);
            return;
        }
        gtk_tree_path_free(parentpath);
        parentpath = childpath;
    }

    gtk_tree_model_get_iter(GTK_TREE_MODEL(directoryTreeModel), &iter, parentpath);
    gtk_tree_path_free(parentpath);

    /* Rename the on-screen node */
    new_basename = g_path_get_basename(new_path);
    new_basename_utf8 = filename_to_display(new_basename);
    gtk_tree_store_set(directoryTreeModel, &iter,
                       TREE_COLUMN_DIR_NAME,  new_basename_utf8,
                       TREE_COLUMN_FULL_PATH, new_path,
                       -1);

    /* Update fullpath of child nodes */
    Browser_Tree_Handle_Rename(&iter, last_path, new_path);

    /* Update the variable of the current path */
    path = Browser_Tree_Get_Path_Of_Selected_Node();
    Browser_Update_Current_Path(path);
    g_free(path);

    g_strfreev(textsplit);
    g_free(new_basename);
    g_free(new_basename_utf8);
}

/*
 * Recursive function to update paths of all child nodes
 */
static void
Browser_Tree_Handle_Rename (GtkTreeIter *parentnode, const gchar *old_path,
                            const gchar *new_path)
{
    GtkTreeIter iter;
    gchar *path;
    gchar *path_shift;
    gchar *path_new;

    // If there are no children then nothing needs to be done!
    if (!gtk_tree_model_iter_children(GTK_TREE_MODEL(directoryTreeModel), &iter, parentnode))
        return;

    do
    {
        gtk_tree_model_get(GTK_TREE_MODEL(directoryTreeModel), &iter,
                           TREE_COLUMN_FULL_PATH, &path, -1);
        if(path == NULL)
            continue;

        path_shift = g_utf8_offset_to_pointer(path, g_utf8_strlen(old_path, -1));
        path_new = g_strconcat(new_path, path_shift, NULL);

        gtk_tree_store_set(directoryTreeModel, &iter,
                           TREE_COLUMN_FULL_PATH, path_new, -1);

        g_free(path_new);
        g_free(path);

        // Recurse if necessary
        if(gtk_tree_model_iter_has_child(GTK_TREE_MODEL(directoryTreeModel), &iter))
            Browser_Tree_Handle_Rename(&iter, old_path, new_path);

    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(directoryTreeModel), &iter));

}

/*
 * Find the child node of "parentnode" that has text of "childtext
 * Returns NULL on failure
 */
static
GtkTreePath *Find_Child_Node (GtkTreeIter *parentnode, gchar *childtext)
{
    gint row;
    GtkTreeIter iter;
    gchar *text;
    gchar *temp;

    for (row=0; row < gtk_tree_model_iter_n_children(GTK_TREE_MODEL(directoryTreeModel), parentnode); row++)
    {
        if (row == 0)
        {
            if (gtk_tree_model_iter_children(GTK_TREE_MODEL(directoryTreeModel), &iter, parentnode) == FALSE) return NULL;
        } else
        {
            if (gtk_tree_model_iter_next(GTK_TREE_MODEL(directoryTreeModel), &iter) == FALSE)
                return NULL;
        }
        gtk_tree_model_get(GTK_TREE_MODEL(directoryTreeModel), &iter,
                           TREE_COLUMN_FULL_PATH, &temp, -1);
        text = g_path_get_basename(temp);
        g_free(temp);
        if(strcmp(childtext,text) == 0)
        {
            g_free(text);
            return gtk_tree_model_get_path(GTK_TREE_MODEL(directoryTreeModel), &iter);
        }
        g_free(text);

    }

    return NULL;
}

/*
 * Check if path has any subdirectories
 * Returns true if subdirectories exist.
 * path should be in raw filename format (non-UTF8)
 */
static gboolean check_for_subdir (gchar *path)
{
    DIR *dir;
    struct dirent *dirent;
    struct stat statbuf;
    gchar *npath;

    if( (dir=opendir(path)) )
    {
        while( (dirent=readdir(dir)) )
        {
            // We don't read the directories '.' and '..', but may read hidden directories
            // like '.mydir' (note that '..mydir' or '...mydir' aren't hidden directories)
            // See also the rule into 'expand_cb' function
            if ((g_ascii_strcasecmp(dirent->d_name,".")  != 0)
            && (g_ascii_strcasecmp(dirent->d_name,"..") != 0)
            // Display hidden directories is needed, or keep only the not hidden directories
            && (BROWSE_HIDDEN_DIR
              || !( (g_ascii_strncasecmp(dirent->d_name,".", 1) == 0)
                 && (strlen(dirent->d_name) > 1)
                 && (g_ascii_strncasecmp(dirent->d_name+1,".", 1) != 0) ))
               )
            {
#ifdef G_OS_WIN32
                // On win32 : stat("/path/to/dir") succeed, while stat("/path/to/dir/") fails
                npath = g_strconcat(path,dirent->d_name,NULL);
#else /* !G_OS_WIN32 */
                npath = g_strconcat(path,dirent->d_name,G_DIR_SEPARATOR_S,NULL);
#endif /* !G_OS_WIN32 */

                if (stat(npath,&statbuf) == -1)
                {
                    g_free(npath);
                    continue;
                }

                g_free(npath);

                if (S_ISDIR(statbuf.st_mode))
                {
                    closedir(dir);
                    return TRUE;
                }
            }
        }
        closedir(dir);
    }
    return FALSE;
}

/*
 * Check if you have permissions for directory path (autorized?, readonly? unreadable?).
 * Returns the right pixmap.
 */
static GdkPixbuf *
Pixmap_From_Directory_Permission (const gchar *path)
{
    DIR *dir;

#if 0
    // TESTING : to display a different icon for non writable directories
    struct stat statbuf;
    if (stat(path,&statbuf) == 0)
    {
        //aaaaaaaaaaaaaaaaaaaaaaaaa
        //g_print(">>%04X %x %x %s\n",statbuf.st_mode,S_IRUSR|S_IRGRP,S_IWUSR|S_IWGRP,path);
        g_print(">>%04X %x %x %s\n",statbuf.st_mode,S_IRUSR,S_IWUSR,path);
        if ( (statbuf.st_mode & ~S_IRUSR)==0 )
        {
            return closed_folder_unreadable_pixmap;

        }else if ( (statbuf.st_mode & ~S_IWUSR)==0 )
        {
            return closed_folder_readonly_pixmap;

        }else
        {
            return closed_folder_pixmap;
        }
    }else
    {
            return closed_folder_pixmap;
    }

#else

    if( (dir=opendir(path)) == NULL )
    {
        if (errno == EACCES)
            return closed_folder_unreadable_pixmap;
    } else
    {
        closedir(dir);
    }
    return closed_folder_pixmap;
#endif
}


/*
 * Sets the selection function. If set, this function is called before any node
 * is selected or unselected, giving some control over which nodes are selected.
 * The select function should return TRUE if the state of the node may be toggled,
 * and FALSE if the state of the node should be left unchanged.
 */
static gboolean
Browser_List_Select_Func (GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, gpointer data)
{
    /* This line will be selected at the end of the event.
     * We store the last ETFile selected, as gtk_tree_selection_get_selected_rows
     * returns the selection, in the ascending line order, instead of the real
     * order of line selection (so we can't displayed the last selected file)
     * FIXME : should generate a list to get the previous selected file if unselected the last selected file */
    if (!path_currently_selected)
    {
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(fileListModel), &iter, path))
            gtk_tree_model_get(GTK_TREE_MODEL(fileListModel), &iter,
                               LIST_FILE_POINTER, &LastBrowserListETFileSelected, -1);
    }else
    {
        LastBrowserListETFileSelected = NULL;
    }
    //g_print(">>>%s -> %d -> %x\n",gtk_tree_path_to_string(path),path_currently_selected,LastBrowserListETFileSelected);

    return TRUE;
}


/*
 * Open up a node on the browser tree
 * Scanning and showing all subdirectories
 */
static void expand_cb (GtkWidget *tree, GtkTreeIter *iter, GtkTreePath *gtreePath, gpointer data)
{
    DIR *dir;
    struct dirent *dirent;
    gchar *path;
    gchar *dirname_utf8;
    struct stat statbuf;
    gchar *fullpath_file;
    gchar *parentPath;
    gboolean treeScanned;
    gboolean has_subdir = FALSE;
    GtkTreeIter currentIter;
    GtkTreeIter subNodeIter;
    GdkPixbuf *pixbuf;

    g_return_if_fail (directoryTreeModel != NULL);

    gtk_tree_model_get(GTK_TREE_MODEL(directoryTreeModel), iter,
                       TREE_COLUMN_FULL_PATH, &parentPath,
                       TREE_COLUMN_SCANNED,   &treeScanned, -1);

    if (treeScanned)
        return;

    if ( (dir=opendir(parentPath)) )
    {
        while ( (dirent=readdir(dir)) )
        {
            path = g_strconcat(parentPath, dirent->d_name, NULL);
            stat(path, &statbuf);

            // We don't read the directories '.' and '..', but may read hidden directories
            // like '.mydir' (note that '..mydir' or '...mydir' aren't hidden directories)
            // See also the rule into 'check_for_subdir' function
            if (S_ISDIR(statbuf.st_mode)
            && (g_ascii_strcasecmp(dirent->d_name,".")  != 0)
            && (g_ascii_strcasecmp(dirent->d_name,"..") != 0)
            // Display hidden directories is needed, or keep only the not hidden directories
            && (BROWSE_HIDDEN_DIR
              || !( (g_ascii_strncasecmp(dirent->d_name,".", 1) == 0)
                 && (strlen(dirent->d_name) > 1)
                 && (g_ascii_strncasecmp(dirent->d_name+1,".", 1) != 0) ))
               )
            {

                if (path[strlen(path)-1]!=G_DIR_SEPARATOR)
                    fullpath_file = g_strconcat(path,G_DIR_SEPARATOR_S,NULL);
                else
                    fullpath_file = g_strdup(path);

                dirname_utf8 = filename_to_display(dirent->d_name);

                //if (!dirname_utf8)
                //{
                //    gchar *escaped_temp = g_strescape(dirent->d_name, NULL);
                //    g_free(escaped_temp);
                //}

                if (check_for_subdir(fullpath_file))
                    has_subdir = TRUE;
                else
                    has_subdir = FALSE;

                // Select pixmap according permissions for the directory
                pixbuf = Pixmap_From_Directory_Permission(path);

                gtk_tree_store_append(directoryTreeModel, &currentIter, iter);
                gtk_tree_store_set(directoryTreeModel, &currentIter,
                                   TREE_COLUMN_DIR_NAME,   dirname_utf8,
                                   TREE_COLUMN_FULL_PATH,  fullpath_file,
                                   TREE_COLUMN_HAS_SUBDIR, !has_subdir,
                                   TREE_COLUMN_SCANNED,    FALSE,
                                   TREE_COLUMN_PIXBUF,     pixbuf, -1);

                if (has_subdir)
                {
                    // Insert a dummy node
                    gtk_tree_store_append(directoryTreeModel, &subNodeIter, &currentIter);
                }

                g_free(fullpath_file);
                g_free(dirname_utf8);
            }
            g_free(path);

        }
        closedir(dir);
    }

    // remove dummy node
    gtk_tree_model_iter_children(GTK_TREE_MODEL(directoryTreeModel), &subNodeIter, iter);
    gtk_tree_store_remove(directoryTreeModel, &subNodeIter);

#ifdef G_OS_WIN32
    // set open folder pixmap except on drive (depth == 0)
    if (gtk_tree_path_get_depth(gtreePath) > 1)
    {
        // update the icon of the node to opened folder :-)
        gtk_tree_store_set(directoryTreeModel, iter,
                           TREE_COLUMN_SCANNED, TRUE,
                           TREE_COLUMN_PIXBUF, opened_folder_pixmap, -1);
    }
#else /* !G_OS_WIN32 */
    // update the icon of the node to opened folder :-)
    gtk_tree_store_set(directoryTreeModel, iter,
                       TREE_COLUMN_SCANNED, TRUE,
                       TREE_COLUMN_PIXBUF, opened_folder_pixmap, -1);
#endif /* !G_OS_WIN32 */

    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(directoryTreeModel),
                                         TREE_COLUMN_DIR_NAME, GTK_SORT_ASCENDING);

    g_free(parentPath);
}

static void collapse_cb (GtkWidget *tree, GtkTreeIter *iter, GtkTreePath *treePath, gpointer data)
{
    GtkTreeIter subNodeIter;

    g_return_if_fail (directoryTreeModel != NULL);

    gtk_tree_model_iter_children(GTK_TREE_MODEL(directoryTreeModel),
                                 &subNodeIter, iter);
    while (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(directoryTreeModel), iter))
    {
        gtk_tree_model_iter_children(GTK_TREE_MODEL(directoryTreeModel), &subNodeIter, iter);
        gtk_tree_store_remove(directoryTreeModel, &subNodeIter);
    }

#ifdef G_OS_WIN32
    // set closed folder pixmap except on drive (depth == 0)
    if(gtk_tree_path_get_depth(treePath) > 1)
    {
        // update the icon of the node to closed folder :-)
        gtk_tree_store_set(directoryTreeModel, iter,
                           TREE_COLUMN_SCANNED, FALSE,
                           TREE_COLUMN_PIXBUF,  closed_folder_pixmap, -1);
    }
#else /* !G_OS_WIN32 */
    // update the icon of the node to closed folder :-)
    gtk_tree_store_set(directoryTreeModel, iter,
                       TREE_COLUMN_SCANNED, FALSE,
                       TREE_COLUMN_PIXBUF,  closed_folder_pixmap, -1);
#endif /* !G_OS_WIN32 */

    // insert dummy node
    gtk_tree_store_append(directoryTreeModel, &subNodeIter, iter);
}

/*
 * Create item of the browser (Entry + Tree + List).
 */
GtkWidget *Create_Browser_Items (GtkWidget *parent)
{
	GtkWidget *VerticalBox;
    GtkWidget *HBox;
    GtkWidget *ScrollWindowDirectoryTree;
    GtkWidget *ScrollWindowFileList;
    GtkWidget *ScrollWindowArtistList;
    GtkWidget *ScrollWindowAlbumList;
    GtkWidget *Label;
    GtkWidget *Icon;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkWidget *PopupMenu;
    gchar *BrowserTree_Titles[] = {N_("Tree")};
    gchar *BrowserList_Titles[] = {N_("File Name"),N_("Title"),N_("Artist"),N_("Album Artist"),N_("Album"),
                                   N_("Year"),N_("Disc"),N_("Track"),N_("Genre"),N_("Comment"),
                                   N_("Composer"),N_("Original Artist"),N_("Copyright"),
                                   N_("URL"),N_("Encoded By")};
    gchar *ArtistList_Titles[]  = {N_("Artist"),N_("# Albums"),N_("# Files")};
    gchar *AlbumList_Titles[]   = {N_("Album"),N_("# Files")};

    VerticalBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_container_set_border_width(GTK_CONTAINER(VerticalBox),2);


    // HBox for BrowserEntry + BrowserLabel
    HBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start(GTK_BOX(VerticalBox),HBox,FALSE,TRUE,0);

    /*
     * The button to go to the parent directory
     */
    BrowserParentButton = gtk_button_new();
    Icon = gtk_image_new_from_stock("easytag-parent-folder", GTK_ICON_SIZE_SMALL_TOOLBAR); // On Win32, GTK_ICON_SIZE_BUTTON enlarge the combobox...
    gtk_container_add(GTK_CONTAINER(BrowserParentButton),Icon);
    gtk_box_pack_start(GTK_BOX(HBox),BrowserParentButton,FALSE,FALSE,0);
    gtk_button_set_relief(GTK_BUTTON(BrowserParentButton),GTK_RELIEF_NONE);
    g_signal_connect(G_OBJECT(BrowserParentButton),"clicked",G_CALLBACK(Browser_Parent_Button_Clicked),NULL);
    gtk_widget_set_tooltip_text(BrowserParentButton,_("Go to parent directory"));

    /*
     * The entry box for displaying path
     */
    if (BrowserEntryModel != NULL)
        gtk_list_store_clear(BrowserEntryModel);
    else
        BrowserEntryModel = gtk_list_store_new(MISC_COMBO_COUNT, G_TYPE_STRING);

    BrowserEntryCombo = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(BrowserEntryModel));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(BrowserEntryCombo), MISC_COMBO_TEXT);
    /* History list */
    Load_Path_Entry_List(BrowserEntryModel, MISC_COMBO_TEXT);
    //gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(BrowserEntryCombo),2); // Two columns to display paths

    g_signal_connect(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(BrowserEntryCombo)))),"activate",G_CALLBACK(Browser_Entry_Activated),NULL);
    gtk_box_pack_start(GTK_BOX(HBox),BrowserEntryCombo,TRUE,TRUE,1);
    gtk_widget_set_tooltip_text(GTK_WIDGET(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(BrowserEntryCombo)))),_("Enter a directory to browse."));

    /*
     * The button to select a directory to browse
     */
    BrowserButton = gtk_button_new_from_stock(GTK_STOCK_OPEN);
    gtk_box_pack_start(GTK_BOX(HBox),BrowserButton,FALSE,FALSE,1);
    g_signal_connect_swapped(G_OBJECT(BrowserButton),"clicked",
                             G_CALLBACK(File_Selection_Window_For_Directory),G_OBJECT(gtk_bin_get_child(GTK_BIN(BrowserEntryCombo))));
    gtk_widget_set_tooltip_text(BrowserButton,_("Select a directory to browse."));


    /*
     * The label for displaying number of files in path (without subdirs)
     */
    /* Translators: No files, as in "0 files". */
    BrowserLabel = gtk_label_new(_("No files"));
    gtk_box_pack_start(GTK_BOX(HBox),BrowserLabel,FALSE,FALSE,2);


    /* Create pixmaps */
    if(!opened_folder_pixmap)
    {
        opened_folder_pixmap            = gdk_pixbuf_new_from_xpm_data(opened_folder_xpm);
        closed_folder_pixmap            = gdk_pixbuf_new_from_xpm_data(closed_folder_xpm);
        closed_folder_readonly_pixmap   = gdk_pixbuf_new_from_xpm_data(closed_folder_readonly_xpm);
        closed_folder_unreadable_pixmap = gdk_pixbuf_new_from_xpm_data(closed_folder_unreadable_xpm);

#ifdef G_OS_WIN32
        /* get GTK's theme harddrive and removable icons and render it in a pixbuf */
        harddrive_pixmap =  gtk_icon_set_render_icon(gtk_style_lookup_icon_set(parent->style, GTK_STOCK_HARDDISK),
                                                     parent->style,
                                                     gtk_widget_get_direction(parent),
                                                     GTK_STATE_NORMAL,
                                                     GTK_ICON_SIZE_BUTTON,
                                                     parent, NULL);

        removable_pixmap =  gtk_icon_set_render_icon(gtk_style_lookup_icon_set(parent->style, GTK_STOCK_FLOPPY),
                                                     parent->style,
                                                     gtk_widget_get_direction(parent),
                                                     GTK_STATE_NORMAL,
                                                     GTK_ICON_SIZE_BUTTON,
                                                     parent, NULL);

        cdrom_pixmap =  gtk_icon_set_render_icon(gtk_style_lookup_icon_set(parent->style, GTK_STOCK_CDROM),
                                                 parent->style,
                                                 gtk_widget_get_direction(parent),
                                                 GTK_STATE_NORMAL,
                                                 GTK_ICON_SIZE_BUTTON,
                                                 parent, NULL);

        network_pixmap =  gtk_icon_set_render_icon(gtk_style_lookup_icon_set(parent->style, GTK_STOCK_NETWORK),
                                                   parent->style,
                                                   gtk_widget_get_direction(parent),
                                                   GTK_STATE_NORMAL,
                                                   GTK_ICON_SIZE_BUTTON,
                                                   parent, NULL);

        ramdisk_pixmap = gdk_pixbuf_new_from_xpm_data(ram_disk_xpm);
#endif /* G_OS_WIN32 */
    }

    /* Browser NoteBook :
     *  - one tab for the BrowserTree
     *  - one tab for the BrowserArtistList and the BrowserAlbumList
     */
    BrowserNoteBook = gtk_notebook_new();
    //gtk_notebook_popup_enable(GTK_NOTEBOOK(BrowserNoteBook));
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(BrowserNoteBook),FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(BrowserNoteBook),FALSE);


    /*
     * The ScrollWindow and the Directory-Tree
     */
    ScrollWindowDirectoryTree = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ScrollWindowDirectoryTree),
                                   GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    directoryTreeModel = gtk_tree_store_new(TREE_COLUMN_COUNT,
                                            G_TYPE_STRING,
                                            G_TYPE_STRING,
                                            G_TYPE_BOOLEAN,
                                            G_TYPE_BOOLEAN,
                                            GDK_TYPE_PIXBUF);

    Label = gtk_label_new(_("Tree"));
    gtk_notebook_append_page(GTK_NOTEBOOK(BrowserNoteBook),ScrollWindowDirectoryTree,Label);

    /* The tree view */
    BrowserTree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(directoryTreeModel));
    gtk_container_add(GTK_CONTAINER(ScrollWindowDirectoryTree),BrowserTree);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(BrowserTree), TRUE);

    // Column for the pixbuf + text
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _(BrowserTree_Titles[0]));

    // Cell of the column for the pixbuf
    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                       "pixbuf", TREE_COLUMN_PIXBUF,
                                        NULL);
    // Cell of the column for the text
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                       "text", TREE_COLUMN_DIR_NAME,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserTree), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    Browser_Tree_Initialize();


    /* Signals */
    g_signal_connect(G_OBJECT(BrowserTree), "row-expanded",  G_CALLBACK(expand_cb),NULL);
    g_signal_connect(G_OBJECT(BrowserTree), "row-collapsed", G_CALLBACK(collapse_cb),NULL);
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserTree))),
            "changed", G_CALLBACK(Browser_Tree_Node_Selected), NULL);

    g_signal_connect(G_OBJECT(BrowserTree),"key_press_event", G_CALLBACK(Browser_Tree_Key_Press),NULL);

    /* Create Popup Menu on browser tree view */
    PopupMenu = gtk_ui_manager_get_widget(UIManager, "/DirPopup");
    g_signal_connect_swapped(G_OBJECT(BrowserTree),"button_press_event",
                             G_CALLBACK(Browser_Popup_Menu_Handler), G_OBJECT(PopupMenu));



    /*
     * The ScrollWindows with the Artist and Album Lists
     */

    ArtistAlbumVPaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);

    Label = gtk_label_new(_("Artist & Album"));
    gtk_notebook_append_page(GTK_NOTEBOOK(BrowserNoteBook),ArtistAlbumVPaned,Label);

    ScrollWindowArtistList = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ScrollWindowArtistList),
                                   GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

    artistListModel = gtk_list_store_new(ARTIST_COLUMN_COUNT,
                                         G_TYPE_STRING, // Stock-id
                                         G_TYPE_STRING,
                                         G_TYPE_UINT,
                                         G_TYPE_UINT,
                                         G_TYPE_POINTER,
                                         PANGO_TYPE_STYLE,
                                         G_TYPE_INT,
                                         GDK_TYPE_COLOR);

    BrowserArtistList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(artistListModel));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(BrowserArtistList), TRUE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(BrowserArtistList), FALSE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserArtistList)),GTK_SELECTION_SINGLE);


    // Artist column
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _(ArtistList_Titles[0]));

    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                       "stock-id",        ARTIST_PIXBUF,
                                        NULL);
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           ARTIST_NAME,
                                        "weight",         ARTIST_FONT_WEIGHT,
                                        "style",          ARTIST_FONT_STYLE,
                                        "foreground-gdk", ARTIST_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserArtistList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    // # Albums of Artist column
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _(ArtistList_Titles[1]));

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           ARTIST_NUM_ALBUMS,
                                        "weight",         ARTIST_FONT_WEIGHT,
                                        "style",          ARTIST_FONT_STYLE,
                                        "foreground-gdk", ARTIST_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserArtistList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    // # Files of Artist column
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _(ArtistList_Titles[2]));

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           ARTIST_NUM_FILES,
                                        "weight",         ARTIST_FONT_WEIGHT,
                                        "style",          ARTIST_FONT_STYLE,
                                        "foreground-gdk", ARTIST_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserArtistList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserArtistList))),"changed",G_CALLBACK(Browser_Artist_List_Row_Selected),NULL);

    gtk_container_add(GTK_CONTAINER(ScrollWindowArtistList),BrowserArtistList);

    // Create Popup Menu on browser artist list
    PopupMenu = gtk_ui_manager_get_widget(UIManager, "/DirArtistPopup");
    g_signal_connect_swapped(G_OBJECT(BrowserArtistList),"button_press_event",
                             G_CALLBACK(Browser_Popup_Menu_Handler), G_OBJECT(PopupMenu));
    // Not available yet!
    //ui_widget_set_sensitive(MENU_FILE, AM_ARTIST_OPEN_FILE_WITH, FALSE);

    ScrollWindowAlbumList = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ScrollWindowAlbumList),
                                   GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

    albumListModel = gtk_list_store_new(ALBUM_COLUMN_COUNT,
                                        G_TYPE_STRING, // Stock-id
                                        G_TYPE_STRING,
                                        G_TYPE_UINT,
                                        G_TYPE_POINTER,
                                        PANGO_TYPE_STYLE,
                                        G_TYPE_INT,
                                        GDK_TYPE_COLOR);

    BrowserAlbumList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(albumListModel));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(BrowserAlbumList), TRUE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(BrowserAlbumList), FALSE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserAlbumList)), GTK_SELECTION_SINGLE);

    // Album column
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _(AlbumList_Titles[0]));

    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                       "stock-id",        ALBUM_PIXBUF,
                                        NULL);
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           ALBUM_NAME,
                                        "weight",         ALBUM_FONT_WEIGHT,
                                        "style",          ALBUM_FONT_STYLE,
                                        "foreground-gdk", ALBUM_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserAlbumList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    // # files column
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _(AlbumList_Titles[1]));

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           ALBUM_NUM_FILES,
                                        "weight",         ALBUM_FONT_WEIGHT,
                                        "style",          ALBUM_FONT_STYLE,
                                        "foreground-gdk", ALBUM_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserAlbumList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserAlbumList))),"changed",G_CALLBACK(Browser_Album_List_Row_Selected),NULL);
    gtk_container_add(GTK_CONTAINER(ScrollWindowAlbumList),BrowserAlbumList);

    // Create Popup Menu on browser album list
    PopupMenu = gtk_ui_manager_get_widget(UIManager, "/DirAlbumPopup");
    g_signal_connect_swapped(G_OBJECT(BrowserArtistList),"button_press_event",
                             G_CALLBACK(Browser_Popup_Menu_Handler), G_OBJECT(PopupMenu));
    // Not available yet!
    //ui_widget_set_sensitive(MENU_FILE, AM_ALBUM_OPEN_FILE_WITH, FALSE);


    gtk_paned_pack1(GTK_PANED(ArtistAlbumVPaned),ScrollWindowArtistList,TRUE,TRUE); // Top side
    gtk_paned_pack2(GTK_PANED(ArtistAlbumVPaned),ScrollWindowAlbumList,TRUE,TRUE);   // Bottom side
    gtk_paned_set_position(GTK_PANED(ArtistAlbumVPaned),PANE_HANDLE_POSITION3);


    /*
     * The ScrollWindow and the List
     */
    ScrollWindowFileList = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ScrollWindowFileList),
                                   GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

    /* The file list */
    fileListModel = gtk_list_store_new (LIST_COLUMN_COUNT,
                                        G_TYPE_STRING, /* File name. */
                                        G_TYPE_POINTER, /* File pointer. */
                                        G_TYPE_INT, /* File key. */
                                        G_TYPE_BOOLEAN,	/* File OtherDir. */
                                        G_TYPE_INT, /* Font weight. */
                                        GDK_TYPE_COLOR,	/* Row background. */
                                        GDK_TYPE_COLOR,	/* Row foreground. */
                                        G_TYPE_STRING, /* Title tag. */
                                        G_TYPE_STRING, /* Artist tag. */
                                        G_TYPE_STRING, /* Album artist tag. */
                                        G_TYPE_STRING, /* Album tag. */
                                        G_TYPE_STRING, /* Year tag. */
                                        G_TYPE_STRING, /* Disc/CD number tag. */
                                        G_TYPE_STRING, /* Track tag. */
                                        G_TYPE_STRING, /* Genre tag. */
                                        G_TYPE_STRING, /* Comment tag. */
                                        G_TYPE_STRING, /* Composer tag. */
                                        G_TYPE_STRING, /* Orig. artist tag. */
                                        G_TYPE_STRING, /* Copyright tag. */
                                        G_TYPE_STRING, /* URL tag. */
                                        G_TYPE_STRING);	/* Encoded by tag. */

    BrowserList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(fileListModel));
    gtk_container_add(GTK_CONTAINER(ScrollWindowFileList), BrowserList);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(BrowserList), TRUE);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(BrowserList), FALSE);


    // Column for File Name
    column = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_title(column, _(BrowserList_Titles[0]));
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           LIST_FILE_NAME,
                                        "weight",         LIST_FONT_WEIGHT,
                                        "background-gdk", LIST_ROW_BACKGROUND,
                                        "foreground-gdk", LIST_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserList), column);

    // Column for Title
    column = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_title(column, _(BrowserList_Titles[1]));
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           LIST_FILE_TITLE,
                                        "weight",         LIST_FONT_WEIGHT,
                                        "background-gdk", LIST_ROW_BACKGROUND,
                                        "foreground-gdk", LIST_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserList), column);

    // Column for Artist
    column = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_title(column, _(BrowserList_Titles[2]));
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           LIST_FILE_ARTIST,
                                        "weight",         LIST_FONT_WEIGHT,
                                        "background-gdk", LIST_ROW_BACKGROUND,
                                        "foreground-gdk", LIST_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserList), column);

    // Column for Album Artist
    column = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_title(column, _(BrowserList_Titles[3]));
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           LIST_FILE_ALBUM_ARTIST,
                                        "weight",         LIST_FONT_WEIGHT,
                                        "background-gdk", LIST_ROW_BACKGROUND,
                                        "foreground-gdk", LIST_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserList), column);
	
    // Column for Album
    column = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_title(column, _(BrowserList_Titles[4]));
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           LIST_FILE_ALBUM,
                                        "weight",         LIST_FONT_WEIGHT,
                                        "background-gdk", LIST_ROW_BACKGROUND,
                                        "foreground-gdk", LIST_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserList), column);

    // Column for Year
    column = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_title(column, _(BrowserList_Titles[5]));
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           LIST_FILE_YEAR,
                                        "weight",         LIST_FONT_WEIGHT,
                                        "background-gdk", LIST_ROW_BACKGROUND,
                                        "foreground-gdk", LIST_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserList), column);

    /* Column for disc/CD number. */
    column = gtk_tree_view_column_new ();
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_title (column, _(BrowserList_Titles[6]));
    gtk_tree_view_column_set_attributes (column, renderer,
                                         "text", LIST_FILE_DISCNO,
                                         "weight", LIST_FONT_WEIGHT,
                                         "background-gdk", LIST_ROW_BACKGROUND,
                                         "foreground-gdk", LIST_ROW_FOREGROUND,
                                         NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (BrowserList), column);

    /* Column for track number. */
    column = gtk_tree_view_column_new ();
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_title (column, _(BrowserList_Titles[7]));
    gtk_tree_view_column_set_attributes (column, renderer,
                                         "text", LIST_FILE_TRACK,
                                         "weight", LIST_FONT_WEIGHT,
                                         "background-gdk", LIST_ROW_BACKGROUND,
                                         "foreground-gdk", LIST_ROW_FOREGROUND,
                                         NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (BrowserList), column);

    // Column for Genre
    column = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_title(column, _(BrowserList_Titles[8]));
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           LIST_FILE_GENRE,
                                        "weight",         LIST_FONT_WEIGHT,
                                        "background-gdk", LIST_ROW_BACKGROUND,
                                        "foreground-gdk", LIST_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserList), column);

    // Column for Comment
    column = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_title(column, _(BrowserList_Titles[9]));
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           LIST_FILE_COMMENT,
                                        "weight",         LIST_FONT_WEIGHT,
                                        "background-gdk", LIST_ROW_BACKGROUND,
                                        "foreground-gdk", LIST_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserList), column);

    // Column for Composer
    column = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_title(column, _(BrowserList_Titles[10]));
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           LIST_FILE_COMPOSER,
                                        "weight",         LIST_FONT_WEIGHT,
                                        "background-gdk", LIST_ROW_BACKGROUND,
                                        "foreground-gdk", LIST_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserList), column);

    // Column for Original Artist
    column = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_title(column, _(BrowserList_Titles[11]));
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           LIST_FILE_ORIG_ARTIST,
                                        "weight",         LIST_FONT_WEIGHT,
                                        "background-gdk", LIST_ROW_BACKGROUND,
                                        "foreground-gdk", LIST_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserList), column);

    // Column for Copyright
    column = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_title(column, _(BrowserList_Titles[12]));
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           LIST_FILE_COPYRIGHT,
                                        "weight",         LIST_FONT_WEIGHT,
                                        "background-gdk", LIST_ROW_BACKGROUND,
                                        "foreground-gdk", LIST_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserList), column);

    // Column for Url
    column = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_title(column, _(BrowserList_Titles[13]));
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           LIST_FILE_URL,
                                        "weight",         LIST_FONT_WEIGHT,
                                        "background-gdk", LIST_ROW_BACKGROUND,
                                        "foreground-gdk", LIST_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserList), column);

    // Column for Encoded By
    column = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_title(column, _(BrowserList_Titles[14]));
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           LIST_FILE_ENCODED_BY,
                                        "weight",         LIST_FONT_WEIGHT,
                                        "background-gdk", LIST_ROW_BACKGROUND,
                                        "foreground-gdk", LIST_ROW_FOREGROUND,
                                        NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(BrowserList), column);


    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(BrowserList), FALSE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList)),GTK_SELECTION_MULTIPLE);
    // When selecting a line
    gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList)), Browser_List_Select_Func, NULL, NULL);
    // To sort list
    //gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(fileListModel), 0, Browser_List_Sort_Func, NULL, NULL);
    Browser_List_Refresh_Sort();
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fileListModel), 0, GTK_SORT_ASCENDING);

    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList))),
            "changed", G_CALLBACK(Browser_List_Row_Selected), NULL);
    g_signal_connect(G_OBJECT(BrowserList),"key_press_event", G_CALLBACK(Browser_List_Key_Press),NULL);
    g_signal_connect(G_OBJECT(BrowserList),"button_press_event", G_CALLBACK(Browser_List_Button_Press),NULL);


    /*
     * Create Popup Menu on file list
     */
    PopupMenu = gtk_ui_manager_get_widget(UIManager, "/FilePopup");
    g_signal_connect_swapped(G_OBJECT(BrowserList),"button_press_event",
                             G_CALLBACK(Browser_Popup_Menu_Handler), G_OBJECT(PopupMenu));

    /*
     * The list store for run program combos
     */
    RunProgramModel = gtk_list_store_new(MISC_COMBO_COUNT, G_TYPE_STRING);

    /*
     * The pane for the tree and list
     */
    BrowserHPaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(VerticalBox),BrowserHPaned,TRUE,TRUE,0);
    gtk_paned_pack1(GTK_PANED(BrowserHPaned),BrowserNoteBook,TRUE,TRUE);   // Left side
    gtk_paned_pack2(GTK_PANED(BrowserHPaned),ScrollWindowFileList,TRUE,TRUE); // Right side
    gtk_paned_set_position(GTK_PANED(BrowserHPaned),PANE_HANDLE_POSITION2);

    gtk_widget_show_all(VerticalBox);

    /* Set home variable as current path */
    Browser_Update_Current_Path (g_get_home_dir ());

    return VerticalBox;
}



/*
 * The window to Rename a directory into the browser.
 */
void Browser_Open_Rename_Directory_Window (void)
{
    GtkWidget *Frame;
    GtkWidget *VBox;
    GtkWidget *HBox;
    GtkWidget *Label;
    GtkWidget *ButtonBox;
    GtkWidget *Button;
    GtkWidget *Separator;
    gchar *directory_parent = NULL;
    gchar *directory_name = NULL;
    gchar *directory_name_utf8 = NULL;
    gchar *address = NULL;
    gchar *string;

    if (RenameDirectoryWindow != NULL)
    {
        gtk_window_present(GTK_WINDOW(RenameDirectoryWindow));
        return;
    }

    /* We get the full path but we musn't display the parent directories */
    directory_parent = g_strdup(BrowserCurrentPath);
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

    RenameDirectoryWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(RenameDirectoryWindow),_("Rename the directory"));
    gtk_window_set_transient_for(GTK_WINDOW(RenameDirectoryWindow),GTK_WINDOW(MainWindow));
    gtk_window_set_position(GTK_WINDOW(RenameDirectoryWindow),GTK_WIN_POS_CENTER_ON_PARENT);

    /* We attach useful data to the combobox */
    g_object_set_data(G_OBJECT(RenameDirectoryWindow), "Parent_Directory", directory_parent);
    g_object_set_data(G_OBJECT(RenameDirectoryWindow), "Current_Directory", directory_name);

    Frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(RenameDirectoryWindow),Frame);
    gtk_container_set_border_width(GTK_CONTAINER(Frame),2);

    VBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,4);
    gtk_container_add(GTK_CONTAINER(Frame),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox), 4);

    string = g_strdup_printf(_("Rename the directory '%s' to:"),directory_name_utf8);
    Label = gtk_label_new(_(string));
    g_free(string);
    gtk_box_pack_start(GTK_BOX(VBox),Label,FALSE,TRUE,0);
    gtk_label_set_line_wrap(GTK_LABEL(Label),TRUE);

    /* The combobox to rename the directory */
    RenameDirectoryCombo = gtk_combo_box_text_new_with_entry();
    gtk_box_pack_start(GTK_BOX(VBox),RenameDirectoryCombo,FALSE,FALSE,0);
    /* Set the directory into the combobox */
    gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(RenameDirectoryCombo), directory_name_utf8);
    gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(RenameDirectoryCombo), "");
    gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameDirectoryCombo))),directory_name_utf8);
    Attach_Popup_Menu_To_Tag_Entries(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameDirectoryCombo))));

    /* Rename directory : check box + combo box + Status icon */
    HBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_box_pack_start(GTK_BOX(VBox),HBox,TRUE,TRUE,0);

    RenameDirectoryWithMask = gtk_check_button_new_with_label(_("Use mask:"));
    gtk_box_pack_start(GTK_BOX(HBox),RenameDirectoryWithMask,FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(RenameDirectoryWithMask),RENAME_DIRECTORY_WITH_MASK);
    gtk_widget_set_tooltip_text(RenameDirectoryWithMask,_("If activated, it will use masks to rename directory."));
    g_signal_connect(G_OBJECT(RenameDirectoryWithMask),"toggled",G_CALLBACK(Rename_Directory_With_Mask_Toggled),NULL);

    // Set up list model which is used by the combobox
    /* Rename directory from mask */
    if (!RenameDirectoryMaskModel)
        RenameDirectoryMaskModel = gtk_list_store_new(MASK_EDITOR_COUNT, G_TYPE_STRING);
    else
        gtk_list_store_clear(RenameDirectoryMaskModel);

    // The combo box to select the mask to apply
    RenameDirectoryMaskCombo = gtk_combo_box_new_with_entry();
    gtk_combo_box_set_model(GTK_COMBO_BOX(RenameDirectoryMaskCombo), GTK_TREE_MODEL(RenameDirectoryMaskModel));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(RenameDirectoryMaskCombo), MASK_EDITOR_TEXT);
    gtk_widget_set_size_request(RenameDirectoryMaskCombo, 80, -1);

    gtk_box_pack_start(GTK_BOX(HBox),RenameDirectoryMaskCombo,TRUE,TRUE,0);
    gtk_widget_set_tooltip_text(GTK_WIDGET(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameDirectoryMaskCombo)))),
        _("Select or type in a mask using codes (see Legend in Scanner Window) to rename "
        "the directory from tag fields."));
    // Signal to generate preview (preview of the new directory)
    g_signal_connect_swapped(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameDirectoryMaskCombo)))),"changed",
        G_CALLBACK(Scan_Rename_Directory_Generate_Preview),NULL);

    // Load masks into the combobox from a file
    Load_Rename_Directory_Masks_List(RenameDirectoryMaskModel, MASK_EDITOR_TEXT, Rename_Directory_Masks);
    if (RENAME_DIRECTORY_DEFAULT_MASK)
    {
        Add_String_To_Combo_List(RenameDirectoryMaskModel, RENAME_DIRECTORY_DEFAULT_MASK);
        gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameDirectoryMaskCombo))), RENAME_DIRECTORY_DEFAULT_MASK);
    }else
    {
        gtk_combo_box_set_active(GTK_COMBO_BOX(RenameDirectoryMaskCombo), 0);
    }

    // Mask status icon
    RenameDirectoryMaskStatusIconBox = Create_Pixmap_Icon_With_Event_Box("easytag-forbidden");
    gtk_box_pack_start(GTK_BOX(HBox),RenameDirectoryMaskStatusIconBox,FALSE,FALSE,0);
    gtk_widget_set_tooltip_text(RenameDirectoryMaskStatusIconBox,_("Invalid Scanner Mask"));
    // Signal connection to check if mask is correct into the mask entry
    g_signal_connect_swapped(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameDirectoryMaskCombo)))),"changed",
        G_CALLBACK(Scan_Check_Rename_File_Mask),G_OBJECT(RenameDirectoryMaskStatusIconBox));

    // Preview label
    RenameDirectoryPreviewLabel = gtk_label_new(_("Rename directory preview…"));
    gtk_label_set_line_wrap(GTK_LABEL(RenameDirectoryPreviewLabel),TRUE);
    ////gtk_widget_show(FillTagPreviewLabel);
    gtk_box_pack_start(GTK_BOX(VBox),RenameDirectoryPreviewLabel,TRUE,TRUE,0);

    /* Separator line */
    Separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(VBox),Separator,FALSE,FALSE,0);

    ButtonBox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(VBox),ButtonBox,FALSE,FALSE,0);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(ButtonBox),GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(ButtonBox),10);

    /* Button to cancel */
    Button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    gtk_container_add(GTK_CONTAINER(ButtonBox),Button);
    gtk_widget_set_can_default(Button,TRUE);
    gtk_widget_grab_default(Button);
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",G_CALLBACK(Destroy_Rename_Directory_Window), G_OBJECT(RenameDirectoryCombo));

    /* Button to save: to rename directory */
    Button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
    gtk_container_add(GTK_CONTAINER(ButtonBox),Button);
    gtk_widget_set_can_default(Button,TRUE);
    g_signal_connect_swapped(G_OBJECT(Button),"clicked", G_CALLBACK(Rename_Directory),NULL);
    g_signal_connect_swapped(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameDirectoryCombo)))),"changed",
        G_CALLBACK(Entry_Changed_Disable_Object),G_OBJECT(Button));

    g_signal_connect_swapped(G_OBJECT(RenameDirectoryWindow),"destroy", G_CALLBACK(Destroy_Rename_Directory_Window), NULL);
    g_signal_connect_swapped(G_OBJECT(RenameDirectoryWindow),"delete_event", G_CALLBACK(Destroy_Rename_Directory_Window), NULL);
    g_signal_connect(G_OBJECT(RenameDirectoryWindow),"key_press_event", G_CALLBACK(Rename_Directory_Window_Key_Press),NULL);
    gtk_widget_show(RenameDirectoryWindow);

    // Just center it over the main window
    gtk_window_set_position(GTK_WINDOW(RenameDirectoryWindow), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_resizable(GTK_WINDOW(RenameDirectoryWindow),FALSE);
    gtk_widget_set_size_request(GTK_WIDGET(RenameDirectoryWindow), 350, -1);

    // To avoid/minimize 'flicker'
    gtk_widget_show_all(RenameDirectoryWindow);

    // To initialize the 'Use mask' check button state
    g_signal_emit_by_name(G_OBJECT(RenameDirectoryWithMask),"toggled");

    // To initialize PreviewLabel + MaskStatusIconBox
    g_signal_emit_by_name(G_OBJECT(gtk_bin_get_child(GTK_BIN(RenameDirectoryMaskCombo))),"changed");

    g_free(directory_name_utf8);
}

static void
Destroy_Rename_Directory_Window (void)
{
    if (RenameDirectoryWindow)
    {
        g_free(g_object_get_data(G_OBJECT(RenameDirectoryWindow),"Parent_Directory"));
        g_free(g_object_get_data(G_OBJECT(RenameDirectoryWindow),"Current_Directory"));

        // Prevent recursion (double-freeing)
        // We can't unblock after the destroy is complete, it must be done automatically
        g_signal_handlers_block_by_func(RenameDirectoryWindow, Destroy_Rename_Directory_Window, NULL);

        if (RENAME_DIRECTORY_DEFAULT_MASK) g_free(RENAME_DIRECTORY_DEFAULT_MASK);
        RENAME_DIRECTORY_DEFAULT_MASK = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameDirectoryMaskCombo)))));
        Add_String_To_Combo_List(RenameDirectoryMaskModel, RENAME_DIRECTORY_DEFAULT_MASK);
        Save_Rename_Directory_Masks_List(RenameDirectoryMaskModel, MASK_EDITOR_TEXT);

        RENAME_DIRECTORY_WITH_MASK = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(RenameDirectoryWithMask));

        gtk_list_store_clear(RenameDirectoryMaskModel);

        gtk_widget_destroy(RenameDirectoryWindow);
        RenameDirectoryWindow = NULL;
    }
}

static void
Rename_Directory (void)
{
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


    if (!RenameDirectoryWindow)
        return;

    directory_parent    = g_object_get_data(G_OBJECT(RenameDirectoryWindow),"Parent_Directory");
    directory_last_name = g_object_get_data(G_OBJECT(RenameDirectoryWindow),"Current_Directory");

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(RenameDirectoryWithMask)))
    {
        // Renamed from mask
        gchar *mask = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameDirectoryMaskCombo)))));
        directory_new_name = Scan_Generate_New_Directory_Name_From_Mask(ETCore->ETFileDisplayed,mask,FALSE);
        g_free(mask);

    }else
    {
        // Renamed 'manually'
        directory_new_name  = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RenameDirectoryCombo)))));
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
                                           _("Could not convert '%s' into filename encoding."),
                                           directory_new_name);
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msgdialog),_("Please use another name"));
        gtk_window_set_title(GTK_WINDOW(msgdialog),_("Directory Name Error"));

        gtk_dialog_run(GTK_DIALOG(msgdialog));
        gtk_widget_destroy(msgdialog);
        g_free(directory_new_name);
        g_free(directory_new_name_file);
    }

    /* If the directory name haven't been changed, we do nothing! */
    if (directory_last_name && directory_new_name_file
    && strcmp(directory_last_name,directory_new_name_file)==0)
    {
        Destroy_Rename_Directory_Window();
        g_free(directory_new_name);
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
    //        msgbox = msg_box_new(_("Confirm…"),
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
            gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msgdialog),_("The directory name '%s' already exists"),new_path_utf8);
            gtk_window_set_title(GTK_WINDOW(msgdialog),_("Rename File Error"));

            gtk_dialog_run(GTK_DIALOG(msgdialog));
            gtk_widget_destroy(msgdialog);

            g_free(directory_new_name);
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

        g_free(directory_new_name);
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

        g_free(directory_new_name);
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
    Browser_Tree_Rename_Directory(last_path,new_path);

    // To update file path in the browser entry
    if (ETCore->ETFileDisplayedList)
    {
        ET_Display_File_Data_To_UI(ETCore->ETFileDisplayed);
    }else
    {
        gchar *tmp = filename_to_display(Browser_Get_Current_Path());
        Browser_Entry_Set_Text(tmp);
        g_free(tmp);
    }

    Destroy_Rename_Directory_Window();
    g_free(last_path);
    g_free(last_path_utf8);
    g_free(new_path);
    g_free(new_path_utf8);
    g_free(tmp_path);
    g_free(tmp_path_utf8);
    g_free(directory_new_name);
    g_free(directory_new_name_file);
    Statusbar_Message(_("Directory renamed"),TRUE);
}

static gboolean
Rename_Directory_Window_Key_Press (GtkWidget *window, GdkEvent *event)
{
    GdkEventKey *kevent;

    if (event && event->type == GDK_KEY_PRESS)
    {
        kevent = (GdkEventKey *)event;
        switch(kevent->keyval)
        {
            case GDK_KEY_Escape:
                // Destroy_Rename_Directory_Window();
                g_signal_emit_by_name(window, "destroy");
                break;
        }
    }
    return FALSE;
}

static void
Rename_Directory_With_Mask_Toggled (void)
{
    gtk_widget_set_sensitive(GTK_WIDGET(RenameDirectoryCombo),            !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(RenameDirectoryWithMask)));
    gtk_widget_set_sensitive(GTK_WIDGET(RenameDirectoryMaskCombo),         gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(RenameDirectoryWithMask)));
    gtk_widget_set_sensitive(GTK_WIDGET(RenameDirectoryMaskStatusIconBox), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(RenameDirectoryWithMask)));
    gtk_widget_set_sensitive(GTK_WIDGET(RenameDirectoryPreviewLabel),      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(RenameDirectoryWithMask)));
}


/*
 * Window where is typed the name of the program to run, which
 * receives the current directory as parameter.
 */
void Browser_Open_Run_Program_Tree_Window (void)
{
    GtkWidget *Frame;
    GtkWidget *VBox;
    GtkWidget *HBox;
    GtkWidget *Label;
    GtkWidget *RunProgramComboBox;
    GtkWidget *ButtonBox;
    GtkWidget *Button;
    GtkWidget *Separator;
    gchar *current_directory = NULL;

    if (RunProgramTreeWindow != NULL)
    {
        gtk_window_present(GTK_WINDOW(RunProgramTreeWindow));
        return;
    }

    // Current directory
    current_directory = g_strdup(BrowserCurrentPath);
    if (!current_directory || strlen(current_directory)==0)
        return;

    RunProgramTreeWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(RunProgramTreeWindow),_("Browse Directory with…"));
    gtk_window_set_transient_for(GTK_WINDOW(RunProgramTreeWindow),GTK_WINDOW(MainWindow));
    g_signal_connect(G_OBJECT(RunProgramTreeWindow),"destroy", G_CALLBACK(Destroy_Run_Program_Tree_Window),NULL);
    g_signal_connect(G_OBJECT(RunProgramTreeWindow),"delete_event", G_CALLBACK(Destroy_Run_Program_Tree_Window),NULL);
    g_signal_connect(G_OBJECT(RunProgramTreeWindow),"key_press_event", G_CALLBACK(Run_Program_Tree_Window_Key_Press),NULL);

    // Just center it over mainwindow
    gtk_window_set_position(GTK_WINDOW(RunProgramTreeWindow), GTK_WIN_POS_CENTER_ON_PARENT);

    Frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(RunProgramTreeWindow),Frame);
    gtk_container_set_border_width(GTK_CONTAINER(Frame),2);

    VBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,4);
    gtk_container_add(GTK_CONTAINER(Frame),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox), 4);

    Label = gtk_label_new(_("Program to run:"));
    gtk_box_pack_start(GTK_BOX(VBox),Label,TRUE,FALSE,0);
    gtk_label_set_line_wrap(GTK_LABEL(Label),TRUE);

    HBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,4);
    gtk_box_pack_start(GTK_BOX(VBox),HBox,FALSE,FALSE,2);
    gtk_container_set_border_width(GTK_CONTAINER(HBox), 2);

    /* The combobox to enter the program to run */
    RunProgramComboBox = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(RunProgramModel));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(RunProgramComboBox), MISC_COMBO_TEXT);
    gtk_box_pack_start(GTK_BOX(HBox),RunProgramComboBox,TRUE,TRUE,0);
    gtk_widget_set_size_request(GTK_WIDGET(RunProgramComboBox),250,-1);
    gtk_widget_set_tooltip_text(GTK_WIDGET(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RunProgramComboBox)))),_("Enter the program to run. "
        "It will receive the current directory as parameter."));

    /* History list */
    gtk_list_store_clear(RunProgramModel);
    Load_Run_Program_With_Directory_List(RunProgramModel, MISC_COMBO_TEXT);
    g_signal_connect_swapped(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RunProgramComboBox)))),"activate",
        G_CALLBACK(Run_Program_With_Directory),G_OBJECT(RunProgramComboBox));

    /* The button to Browse */
    Button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
    gtk_box_pack_start(GTK_BOX(HBox),Button,FALSE,FALSE,0);
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(File_Selection_Window_For_File),G_OBJECT(gtk_bin_get_child(GTK_BIN(RunProgramComboBox))));

    /* We attach useful data to the combobox (into Run_Program_With_Directory) */
    g_object_set_data(G_OBJECT(RunProgramComboBox), "Current_Directory", current_directory);

    /* Separator line */
    Separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(VBox),Separator,FALSE,FALSE,0);

    ButtonBox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(VBox),ButtonBox,FALSE,FALSE,0);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(ButtonBox),GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(ButtonBox), 10);

    /* Button to cancel */
    Button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    gtk_container_add(GTK_CONTAINER(ButtonBox),Button);
    gtk_widget_set_can_default(Button,TRUE);
    gtk_widget_grab_default(Button);
    g_signal_connect(G_OBJECT(Button),"clicked", G_CALLBACK(Destroy_Run_Program_Tree_Window),NULL);

    /* Button to execute */
    Button = gtk_button_new_from_stock(GTK_STOCK_EXECUTE);
    gtk_container_add(GTK_CONTAINER(ButtonBox),Button);
    gtk_widget_set_can_default(Button,TRUE);
    g_signal_connect_swapped(G_OBJECT(Button),"clicked", G_CALLBACK(Run_Program_With_Directory),G_OBJECT(RunProgramComboBox));
    g_signal_connect_swapped(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RunProgramComboBox)))),"changed", G_CALLBACK(Entry_Changed_Disable_Object),G_OBJECT(Button));
    g_signal_emit_by_name(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RunProgramComboBox)))),"changed",NULL);

    gtk_widget_show_all(RunProgramTreeWindow);
}

static void
Destroy_Run_Program_Tree_Window (void)
{
    if (RunProgramTreeWindow)
    {
        gtk_widget_destroy(RunProgramTreeWindow);
        RunProgramTreeWindow = NULL;
    }
}

static gboolean
Run_Program_Tree_Window_Key_Press (GtkWidget *window, GdkEvent *event)
{
    GdkEventKey *kevent;

    if (event && event->type == GDK_KEY_PRESS)
    {
        kevent = (GdkEventKey *)event;
        switch(kevent->keyval)
        {
            case GDK_KEY_Escape:
                Destroy_Run_Program_Tree_Window();
                break;
        }
    }
    return FALSE;
}

void Run_Program_With_Directory (GtkWidget *combobox)
{
    gchar *program_name;
    gchar *current_directory;
    GList *args_list = NULL;
    gboolean program_ran;

    g_return_if_fail (GTK_IS_COMBO_BOX (combobox));

    program_name      = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combobox)))));
    current_directory = g_object_get_data(G_OBJECT(combobox), "Current_Directory");
#ifdef G_OS_WIN32
    /* On win32 : 'winamp.exe "c:\path\to\dir"' succeed, while 'winamp.exe "c:\path\to\dir\"' fails */
    ET_Win32_Path_Remove_Trailing_Backslash(current_directory);
#endif /* G_OS_WIN32 */

    // List of parameters (here only one! : the current directory)
    args_list = g_list_append(args_list,current_directory);

    program_ran = Run_Program(program_name,args_list);
    g_list_free(args_list);

    if (program_ran)
    {
        // Append newest choice to the drop down list
        Add_String_To_Combo_List(RunProgramModel, program_name);

        // Save list attached to the combobox
        Save_Run_Program_With_Directory_List(RunProgramModel, MISC_COMBO_TEXT);

        Destroy_Run_Program_Tree_Window();
    }
    g_free(program_name);
}

/*
 * Window where is typed the name of the program to run, which
 * receives the current file as parameter.
 */
void Browser_Open_Run_Program_List_Window (void)
{
    GtkWidget *Frame;
    GtkWidget *VBox;
    GtkWidget *HBox;
    GtkWidget *Label;
    GtkWidget *RunProgramComboBox;
    GtkWidget *ButtonBox;
    GtkWidget *Button;
    GtkWidget *Separator;

    if (RunProgramListWindow != NULL)
    {
        gtk_window_present(GTK_WINDOW(RunProgramListWindow));
        return;
    }

    RunProgramListWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(RunProgramListWindow),_("Open File with…"));
    gtk_window_set_transient_for(GTK_WINDOW(RunProgramListWindow),GTK_WINDOW(MainWindow));
    g_signal_connect(G_OBJECT(RunProgramListWindow),"destroy", G_CALLBACK(Destroy_Run_Program_List_Window),NULL);
    g_signal_connect(G_OBJECT(RunProgramListWindow),"delete_event", G_CALLBACK(Destroy_Run_Program_List_Window),NULL);
    g_signal_connect(G_OBJECT(RunProgramListWindow),"key_press_event", G_CALLBACK(Run_Program_List_Window_Key_Press),NULL);

    // Just center over mainwindow
    gtk_window_set_position(GTK_WINDOW(RunProgramListWindow),GTK_WIN_POS_CENTER_ON_PARENT);

    Frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(RunProgramListWindow),Frame);
    gtk_container_set_border_width(GTK_CONTAINER(Frame),2);

    VBox = gtk_box_new(GTK_ORIENTATION_VERTICAL,4);
    gtk_container_add(GTK_CONTAINER(Frame),VBox);
    gtk_container_set_border_width(GTK_CONTAINER(VBox), 4);

    Label = gtk_label_new(_("Program to run:"));
    gtk_box_pack_start(GTK_BOX(VBox),Label,TRUE,TRUE,0);
    gtk_label_set_line_wrap(GTK_LABEL(Label),TRUE);

    HBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,4);
    gtk_box_pack_start(GTK_BOX(VBox),HBox,FALSE,FALSE,2);
    gtk_container_set_border_width(GTK_CONTAINER(HBox), 2);

    /* The combobox to enter the program to run */
    RunProgramComboBox = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(RunProgramModel));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(RunProgramComboBox),MISC_COMBO_TEXT);
    gtk_box_pack_start(GTK_BOX(HBox),RunProgramComboBox,TRUE,TRUE,0);
    gtk_widget_set_size_request(GTK_WIDGET(RunProgramComboBox),250,-1);
    gtk_widget_set_tooltip_text(GTK_WIDGET(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RunProgramComboBox)))),_("Enter the program to run. "
        "It will receive the current file as parameter."));

    /* History list */
    gtk_list_store_clear(RunProgramModel);
    Load_Run_Program_With_File_List(RunProgramModel, MISC_COMBO_TEXT);
    g_signal_connect_swapped(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RunProgramComboBox)))),"activate",
        G_CALLBACK(Run_Program_With_Selected_Files),G_OBJECT(RunProgramComboBox));

    /* The button to Browse */
    Button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
    gtk_box_pack_start(GTK_BOX(HBox),Button,FALSE,FALSE,0);
    g_signal_connect_swapped(G_OBJECT(Button),"clicked",
                             G_CALLBACK(File_Selection_Window_For_File),G_OBJECT(gtk_bin_get_child(GTK_BIN(RunProgramComboBox))));

    /* We attach useful data to the combobox (into Run_Program_With_Directory) */
    //g_object_set_data(G_OBJECT(Combo), "Current_File", current_file);

    /* Separator line */
    Separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(VBox),Separator,FALSE,FALSE,0);

    ButtonBox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(VBox),ButtonBox,FALSE,FALSE,0);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(ButtonBox),GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(ButtonBox), 10);

    /* Button to cancel */
    Button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    gtk_container_add(GTK_CONTAINER(ButtonBox),Button);
    gtk_widget_set_can_default(Button,TRUE);
    gtk_widget_grab_default(Button);
    g_signal_connect(G_OBJECT(Button),"clicked", G_CALLBACK(Destroy_Run_Program_List_Window),NULL);

    /* Button to execute */
    Button = gtk_button_new_from_stock(GTK_STOCK_EXECUTE);
    gtk_container_add(GTK_CONTAINER(ButtonBox),Button);
    gtk_widget_set_can_default(Button,TRUE);
    g_signal_connect_swapped(G_OBJECT(Button),"clicked", G_CALLBACK(Run_Program_With_Selected_Files),G_OBJECT(RunProgramComboBox));
    g_signal_connect_swapped(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RunProgramComboBox)))),"changed", G_CALLBACK(Entry_Changed_Disable_Object),G_OBJECT(Button));
    g_signal_emit_by_name(G_OBJECT(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(RunProgramComboBox)))),"changed",NULL);

    gtk_widget_show_all(RunProgramListWindow);
}

static void
Destroy_Run_Program_List_Window (void)
{
    if (RunProgramListWindow)
    {
        gtk_widget_destroy(RunProgramListWindow);
        RunProgramListWindow = NULL;
    }
}

static gboolean
Run_Program_List_Window_Key_Press (GtkWidget *window, GdkEvent *event)
{
    GdkEventKey *kevent;

    if (event && event->type == GDK_KEY_PRESS)
    {
        kevent = (GdkEventKey *)event;
        switch(kevent->keyval)
        {
            case GDK_KEY_Escape:
                Destroy_Run_Program_List_Window();
                break;
        }
    }
    return FALSE;
}

static void
Run_Program_With_Selected_Files (GtkWidget *combobox)
{
    gchar   *program_name;
    ET_File *ETFile;
    GList   *selected_paths;
    GList   *args_list = NULL;
    GtkTreeIter iter;
    gboolean program_ran;
    gboolean valid;

    if (!GTK_IS_COMBO_BOX(combobox) || !ETCore->ETFileDisplayedList)
        return;

    // Programe name to run
    program_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combobox)))));

    // List of files to pass as parameters
    selected_paths = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList)), NULL);
    while (selected_paths)
    {
        valid = gtk_tree_model_get_iter(GTK_TREE_MODEL(fileListModel), &iter, (GtkTreePath*)selected_paths->data);
        if (valid)
        {
            gtk_tree_model_get(GTK_TREE_MODEL(fileListModel), &iter,
                               LIST_FILE_POINTER, &ETFile,
                               -1);

            args_list = g_list_append(args_list,((File_Name *)ETFile->FileNameCur->data)->value);
            //args_list = g_list_append(args_list,((File_Name *)ETFile->FileNameCur->data)->value_utf8);
        }

        if (!selected_paths->next) break;
        selected_paths = selected_paths->next;
    }

    program_ran = Run_Program(program_name,args_list);

    g_list_foreach(selected_paths, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(selected_paths);
    g_list_free(args_list);

    if (program_ran)
    {
        // Append newest choice to the drop down list
        //gtk_list_store_prepend(GTK_LIST_STORE(RunProgramModel), &iter);
        //gtk_list_store_set(RunProgramModel, &iter, MISC_COMBO_TEXT, program_name, -1);
        Add_String_To_Combo_List(GTK_LIST_STORE(RunProgramModel), program_name);

        // Save list attached to the combobox
        Save_Run_Program_With_File_List(RunProgramModel, MISC_COMBO_TEXT);

        Destroy_Run_Program_List_Window();
    }
    g_free(program_name);
}

/*
 * Run a program with a list of parameters
 *  - args_list : list of filename (with path)
 */
static gboolean
Run_Program (const gchar *program_name, GList *args_list)
{
#ifdef G_OS_WIN32
    GList              *filelist;
    gchar             **argv;
    gint                argv_index = 0;
    gchar              *argv_join;
    gchar              *full_command;
    STARTUPINFO         siStartupInfo;
    PROCESS_INFORMATION piProcessInfo;
#else /* !G_OS_WIN32 */
    pid_t   pid;
#endif /* !G_OS_WIN32 */
    gchar *program_path;


    /* Check if a name for the program have been supplied */
    if (!program_name || strlen(program_name)<1)
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
        return FALSE;
    }

    if ( !(program_path = Check_If_Executable_Exists(program_name)) )
    {
        GtkWidget *msgdialog;

        msgdialog = gtk_message_dialog_new(GTK_WINDOW(MainWindow),
                                           GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_CLOSE,
                                           _("The program '%s' cannot be found"),
                                           program_name);
        gtk_window_set_title(GTK_WINDOW(msgdialog),_("Program Name Error"));

        gtk_dialog_run(GTK_DIALOG(msgdialog));
        gtk_widget_destroy(msgdialog);
        return FALSE;
    }


#ifdef G_OS_WIN32
    filelist = args_list;

    // See documentation : http://c.developpez.com/faq/vc/?page=ProcessThread and http://www.answers.com/topic/createprocess
    ZeroMemory(&siStartupInfo, sizeof(siStartupInfo));
    siStartupInfo.cb = sizeof(siStartupInfo);
    ZeroMemory(&piProcessInfo, sizeof(piProcessInfo));

    argv = g_new0(gchar *,g_list_length(filelist) + 2); // "+2" for 1rst arg 'foo' and last arg 'NULL'
    //argv[argv_index++] = "foo";

    // Load files as arguments
    while (filelist)
    {
        // We must enclose filename between " because of possible (probable!) spaces in filenames"
        argv[argv_index++] = g_strconcat("\"", (gchar *)filelist->data, "\"", NULL);
        filelist = filelist->next;
    }
    argv[argv_index] = NULL; // Ends the list of arguments

    // Make a command line with all arguments (joins strings together to form one long string separated by a space)
    argv_join = g_strjoinv(" ", argv);
    // Build the full command to pass to CreateProcess (FIX ME : it will ignore args of program)
    full_command = g_strconcat("\"",program_path,"\" ",argv_join,NULL);

    //if (CreateProcess(program_path, // Here it doesn't seem to load all the selected files
    //                  argv_join,
    if (CreateProcess(NULL,
                      full_command,
                      NULL,
                      NULL,
                      FALSE,
                      CREATE_DEFAULT_ERROR_MODE,
                      NULL,
                      NULL,
                      &siStartupInfo,
                      &piProcessInfo) == FALSE)
    {
        Log_Print(LOG_ERROR,_("Cannot execute %s (error %d)\n"), program_name, GetLastError());
    }

    // Free allocated parameters (for each filename)
    for (argv_index = 1; argv[argv_index]; argv_index++)
        g_free(argv[argv_index]);

    g_free(argv_join);
    g_free(full_command);
    g_free(program_path);

#else /* !G_OS_WIN32 */

    g_free(program_path); // Freed as never used

    pid = fork();
    switch (pid)
    {
        case -1:
            Log_Print(LOG_ERROR,_("Cannot fork another process\n"));
            //exit(-1);
            break;
        case 0:
        {
            gchar **argv;
            gint    argv_index = 0;
            gchar **argv_user;
            gint    argv_user_number;
            gchar  *msg;

            argv_user = g_strsplit(program_name," ",0); // the string may contains arguments, space is the delimiter
            // Number of arguments into 'argv_user'
            for (argv_user_number=0;argv_user[argv_user_number];argv_user_number++);

            argv = g_new0(gchar *,argv_user_number + g_list_length(args_list) + 1); // +1 for NULL

            // Load 'user' arguments (program name and more...)
            while (argv_user[argv_index])
            {
                argv[argv_index] = argv_user[argv_index];
                argv_index++;
            }
            // Load arguments from 'args_list'
            while (args_list)
            {
                argv[argv_index] = (gchar *)args_list->data;
                argv_index++;
                args_list = args_list->next;
            }
            argv[argv_index] = NULL;

            // Execution ...
            execvp(argv[0],argv);

            msg = g_strdup_printf(_("Executed command: '%s %s'"),program_name,"…");
            Statusbar_Message(msg,TRUE);
            g_free(msg);
            //_exit(-1);
            break;
        }
        default:
            break;
    }

    return TRUE;
#endif /* !G_OS_WIN32 */
}
