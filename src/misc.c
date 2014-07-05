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

#include "config.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>

#include "gtk2_compat.h"
#include "misc.h"
#include "easytag.h"
#include "id3_tag.h"
#include "browser.h"
#include "setting.h"
#include "bar.h"
#include "preferences_dialog.h"
#include "genres.h"
#include "log.h"
#include "charset.h"

#ifdef G_OS_WIN32
#include <windows.h>
#endif /* G_OS_WIN32 */


/***************
 * Declaration *
 ***************/
static const guint BOX_SPACING = 6;

static GdkCursor *MouseCursor;

/**************
 * Prototypes *
 **************/
static void Create_Xpm_Icon_Factory (const char **xpm_data,
                                     const char *name_in_factory);

/* Browser */
static void Open_File_Selection_Window (GtkWidget *entry, gchar *title, GtkFileChooserAction action);
void        File_Selection_Window_For_File      (GtkWidget *entry);
void        File_Selection_Window_For_Directory (GtkWidget *entry);


/*************
 * Functions *
 *************/

/******************************
 * Functions managing pixmaps *
 ******************************/

/*
 * Return a button with an icon and a label
 */
GtkWidget *Create_Button_With_Icon_And_Label (const gchar *pixmap_name, gchar *label)
{
    GtkWidget *Button;
    GtkWidget *HBox;
    GtkWidget *Label;
    GtkWidget *Pixmap;

    Button = gtk_button_new();
    HBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_container_add(GTK_CONTAINER(Button),HBox);

    /* Add a pixmap if not null */
    if (pixmap_name != NULL)
    {
        gtk_widget_realize(MainWindow);
        Pixmap = gtk_image_new_from_stock(pixmap_name, GTK_ICON_SIZE_BUTTON);
        gtk_container_add(GTK_CONTAINER(HBox),Pixmap);
    }

    /* Add a label if not null */
    if (label != NULL)
    {
        Label = gtk_label_new(label);
        gtk_container_add(GTK_CONTAINER(HBox),Label);
    }

    /* Display a warning message if the both parameters are NULL */
    if (pixmap_name==NULL && label==NULL)
        g_warning("Empty button created 'adr=%p' (no icon and no label)!",Button);

    return Button;
}

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

    if (!string || g_utf8_strlen(string, -1) <= 0)
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
    gtk_list_store_insert_with_values (liststore, &iter, -1, MISC_COMBO_TEXT,
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
 * Returns the text of the selected item in a combo box
 * Remember to free the returned value...
 */
gchar *Get_Active_Combo_Box_Item (GtkComboBox *combo)
{
    gchar *text;
    GtkTreeIter iter;
    GtkTreeModel *model;

    g_return_val_if_fail (combo != NULL, NULL);

    model = gtk_combo_box_get_model(combo);
    if (!gtk_combo_box_get_active_iter(combo, &iter))
        return NULL;

    gtk_tree_model_get(model, &iter, MISC_COMBO_TEXT, &text, -1);
    return text;
}

/*
 * To insert only digits in an entry. If the text contains only digits: returns it,
 * else only first digits.
 */
void Insert_Only_Digit (GtkEditable *editable, const gchar *inserted_text, gint length,
                        gint *position, gpointer data)
{
    int i = 1; // Ignore first character
    int j = 1;
    gchar *result;

    if (length<=0 || !inserted_text)
        return;

    if (!isdigit((guchar)inserted_text[0]) && inserted_text[0] != '-')
    {
        g_signal_stop_emission_by_name(G_OBJECT(editable),"insert_text");
        return;
    } else if (length == 1)
    {
        // We already checked the first digit...
        return;
    }

    g_signal_stop_emission_by_name(G_OBJECT(editable),"insert_text");
    result = g_malloc0(length+1);
    result[0] = inserted_text[0];

    // Check the rest, if any...
    for (i = 1; i < length; i++)
    {
        if (isdigit((guchar)inserted_text[i]))
        {
            result[j++] = inserted_text[i];
        }
    }
    // Null terminate for the benefit of glib/gtk
    result[j] = '\0';

    if (result[0] == '\0')
    {
        g_free(result);
        return;
    }

    g_signal_handlers_block_by_func(G_OBJECT(editable),G_CALLBACK(Insert_Only_Digit),data);
    gtk_editable_insert_text(editable, result, j, position);
    g_signal_handlers_unblock_by_func(G_OBJECT(editable),G_CALLBACK(Insert_Only_Digit),data);
    g_free(result);
}

/*
 * Parse and auto complete date entry if you don't type the 4 digits.
 */
#include <stdlib.h>
gboolean Parse_Date (void)
{
    const gchar *year;
    gchar *tmp, *tmp1;
    gchar *current_year;
    GDateTime *dt;

    /* Early return. */
    if (!g_settings_get_boolean (MainSettings, "tag-date-autocomplete"))
    {
        return FALSE;
    }

    /* Get the info entered by user */
    year = gtk_entry_get_text(GTK_ENTRY(YearEntry));

    if ( strcmp(year,"")!=0 && strlen(year)<4 )
    {
        dt = g_date_time_new_now_local ();
        current_year = g_date_time_format (dt, "%Y");
        g_date_time_unref (dt);

        tmp = &current_year[4-strlen(year)];
        if ( atoi(year) <= atoi(tmp) )
        {
            sprintf(current_year,"%d",atoi(current_year)-atoi(tmp));
            tmp1 = g_strdup_printf("%d",atoi(current_year)+atoi(year));
            gtk_entry_set_text(GTK_ENTRY(YearEntry),tmp1);
            g_free(tmp1);

        }else
        {
            sprintf(current_year,"%d",atoi(current_year)-atoi(tmp)
                -(strlen(year)<=0 ? 1 : strlen(year)<=1 ? 10 :          // pow(10,strlen(year)) returns 99 instead of 100 under Win32...
                 strlen(year)<=2 ? 100 : strlen(year)<=3 ? 1000 : 0));
            tmp1 = g_strdup_printf("%d",atoi(current_year)+atoi(year));
            gtk_entry_set_text(GTK_ENTRY(YearEntry),tmp1);
            g_free(tmp1);
        }

        g_free (current_year);
    }
    return FALSE;
}

/*
 * Load the genres list to the combo, and sorts it
 */
void Load_Genres_List_To_UI (void)
{
    gsize i;

    g_return_if_fail (GenreComboModel != NULL);

    gtk_list_store_insert_with_values (GTK_LIST_STORE (GenreComboModel), NULL,
                                       G_MAXINT, MISC_COMBO_TEXT, "", -1);
    gtk_list_store_insert_with_values (GTK_LIST_STORE (GenreComboModel), NULL,
                                       G_MAXINT, MISC_COMBO_TEXT, "Unknown",
                                       -1);

    for (i = 0; i <= GENRE_MAX; i++)
    {
        gtk_list_store_insert_with_values (GTK_LIST_STORE (GenreComboModel),
                                           NULL, G_MAXINT, MISC_COMBO_TEXT,
                                           id3_genres[i], -1);
    }
}

/*
 * Load the track numbers into the track combo list
 * We limit the preloaded values to 30 to avoid lost of time with lot of files...
 */
void Load_Track_List_To_UI (void)
{
    /* Number mini of items
     *if ((len=ETCore->ETFileDisplayedList_Length) < 30)
     * Length limited to 30 (instead to the number of files)! */
    const gsize len = 30;
    gsize i;
    gchar *text;

    g_return_if_fail (ETCore->ETFileDisplayedList != NULL ||
                      TrackEntryComboModel != NULL);

    /* Remove the entries in the list to avoid duplicates. */
    gtk_list_store_clear (TrackEntryComboModel);

    /* Create list of tracks. */
    for (i = 1; i <= len; i++)
    {
        text = et_track_number_to_string (i);

        gtk_list_store_insert_with_values (GTK_LIST_STORE (TrackEntryComboModel),
                                           NULL, G_MAXINT, MISC_COMBO_TEXT,
                                           text, -1);
        g_free(text);
    }

}


/*
 * Change mouse cursor
 */
void Init_Mouse_Cursor (void)
{
    MouseCursor = NULL;
}

static void
Destroy_Mouse_Cursor (void)
{
    if (MouseCursor)
    {
        g_object_unref (MouseCursor);
        MouseCursor = NULL;
    }
}

void Set_Busy_Cursor (void)
{
    /* If still built, destroy it to avoid memory leak */
    Destroy_Mouse_Cursor();
    /* Create the new cursor */
    MouseCursor = gdk_cursor_new(GDK_WATCH);
    gdk_window_set_cursor(gtk_widget_get_window(MainWindow),MouseCursor);
}

void Set_Unbusy_Cursor (void)
{
    /* Back to standard cursor */
    gdk_window_set_cursor(gtk_widget_get_window(MainWindow),NULL);
    Destroy_Mouse_Cursor();
}



/*
 * Add easytag specific icons to GTK stock set
 */
#include "data/pixmaps/all_uppercase.xpm"
#include "data/pixmaps/all_downcase.xpm"
#include "data/pixmaps/artist.xpm"
#include "data/pixmaps/artist_album.xpm"
//#include "data/pixmaps/blackwhite.xpm"
#include "data/pixmaps/first_letter_uppercase.xpm"
#include "data/pixmaps/first_letter_uppercase_word.xpm"
#include "data/pixmaps/invert_selection.xpm"
#include "data/pixmaps/mask.xpm"
#include "data/pixmaps/red_lines.xpm"
//#include "data/pixmaps/sequence_track.xpm"
#include "data/pixmaps/unselect_all.xpm"
void Init_Custom_Icons (void)
{
    Create_Xpm_Icon_Factory((const char**)artist_xpm,               "easytag-artist");
    Create_Xpm_Icon_Factory((const char**)invert_selection_xpm,     "easytag-invert-selection");
    Create_Xpm_Icon_Factory((const char**)unselect_all_xpm,         "easytag-unselect-all");
    Create_Xpm_Icon_Factory((const char**)mask_xpm,                 "easytag-mask");
    //Create_Xpm_Icon_Factory((const char**)blackwhite_xpm,         "easytag-blackwhite");
    //Create_Xpm_Icon_Factory((const char**)sequence_track_xpm,     "easytag-sequence-track");
    Create_Xpm_Icon_Factory((const char**)red_lines_xpm,            "easytag-red-lines");
    Create_Xpm_Icon_Factory((const char**)artist_album_xpm,     "easytag-artist-album");
    Create_Xpm_Icon_Factory((const char**)all_uppercase_xpm,        "easytag-all-uppercase");
    Create_Xpm_Icon_Factory((const char**)all_downcase_xpm,         "easytag-all-downcase");
    Create_Xpm_Icon_Factory((const char**)first_letter_uppercase_xpm,       "easytag-first-letter-uppercase");
    Create_Xpm_Icon_Factory((const char**)first_letter_uppercase_word_xpm,  "easytag-first-letter-uppercase-word");
}


/*
 * Create an icon factory from the specified pixmap
 * Also add it to the GTK stock images
 */
static void
Create_Xpm_Icon_Factory (const char **xpm_data, const char *name_in_factory)
{
    GtkIconSet      *icon;
    GtkIconFactory  *factory;
    GdkPixbuf       *pixbuf;

    if (!*xpm_data || !name_in_factory)
        return;

    pixbuf = gdk_pixbuf_new_from_xpm_data(xpm_data);

    if (pixbuf)
    {
        icon = gtk_icon_set_new_from_pixbuf(pixbuf);
        g_object_unref(G_OBJECT(pixbuf));

        factory = gtk_icon_factory_new();
        gtk_icon_factory_add(factory, name_in_factory, icon);
        gtk_icon_set_unref(icon);
        gtk_icon_factory_add_default(factory);
    }
}

/*
 * Return a widget with a pixmap
 * Note: for pixmap 'pixmap.xpm', pixmap_name is 'pixmap_xpm'
 */
GtkWidget *Create_Xpm_Image (const char **xpm_name)
{
    GdkPixbuf *pixbuf;
    GtkWidget *image;

    g_return_val_if_fail (*xpm_name != NULL, NULL);

    pixbuf = gdk_pixbuf_new_from_xpm_data(xpm_name);
    image = gtk_image_new_from_pixbuf(GDK_PIXBUF(pixbuf));

    return image;
}



/*
 * Iter compare func: Sort alphabetically
 */
gint Combo_Alphabetic_Sort (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer data)
{
    gchar *text1, *text1_folded;
    gchar *text2, *text2_folded;
    gint ret;

    gtk_tree_model_get(model, a, MISC_COMBO_TEXT, &text1, -1);
    gtk_tree_model_get(model, b, MISC_COMBO_TEXT, &text2, -1);

    if (text1 == text2)
    {
        g_free(text1);
        g_free(text2);
        return 0;
    }

    if (text1 == NULL)
    {
        g_free(text2);
        return -1;
    }

    if (text2 == NULL)
    {
        g_free(text1);
        return 1;
    }

    text1_folded = g_utf8_casefold(text1, -1);
    text2_folded = g_utf8_casefold(text2, -1);
    ret = g_utf8_collate(text1_folded, text2_folded);

    g_free(text1);
    g_free(text2);
    g_free(text1_folded);
    g_free(text2_folded);
    return ret;
}


/*
 * Run a program with a list of parameters
 *  - args_list : list of filename (with path)
 */
gboolean
et_run_program (const gchar *program_name, GList *args_list)
{
    gchar *program_tmp;
    const gchar *program_args;
    gchar **program_args_argv = NULL;
    guint n_program_args = 0;
    gsize i;
    gchar  *msg;
    GPid pid;
    GError *error = NULL;
    gchar **argv;
    GList *l;
    gchar *program_path;
    gboolean res = FALSE;

    g_return_val_if_fail (program_name != NULL && args_list != NULL, FALSE);

    /* Check if a name for the program have been supplied */
    if (!program_name && *program_name)
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

    /* Skip the binary name and a delimiter. Same logic in
     * Check_If_Executable_Exists()*/
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
    if (g_spawn_async (NULL, argv, NULL,
                       G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                       NULL, NULL, &pid, &error))
    {
        g_child_watch_add (pid, et_on_child_exited, NULL);

        msg = g_strdup_printf (_("Executed command: %s"), program_name);
        Statusbar_Message (msg, TRUE);
        g_free (msg);
        res = TRUE;
    }
    else
    {
        Log_Print (LOG_ERROR, _("Failed to launch program: %s"),
                   error->message);
        g_clear_error (&error);
    }

    g_strfreev (program_args_argv);
    g_free (program_path);
    g_free (argv);

    return res;
}

/*************************
 * File selection window *
 *************************/
void File_Selection_Window_For_File (GtkWidget *entry)
{
    Open_File_Selection_Window (entry, _("Select File"),
                                GTK_FILE_CHOOSER_ACTION_OPEN);
}

void File_Selection_Window_For_Directory (GtkWidget *entry)
{
    Open_File_Selection_Window (entry, _("Select Directory"),
                                GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
}

/*
 * Open the file selection window and saves the selected file path into entry
 */
static void Open_File_Selection_Window (GtkWidget *entry, gchar *title, GtkFileChooserAction action)
{
    const gchar *tmp;
    gchar *filename, *filename_utf8;
    GtkWidget *FileSelectionWindow;
    GtkWindow *parent_window = NULL;
    gint response;

    parent_window = (GtkWindow*) gtk_widget_get_toplevel(entry);
    if (!gtk_widget_is_toplevel(GTK_WIDGET(parent_window)))
    {
        g_warning("Could not get parent window\n");
        return;
    }

    FileSelectionWindow = gtk_file_chooser_dialog_new(title, parent_window, action,
                                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                      GTK_STOCK_OPEN,   GTK_RESPONSE_ACCEPT,
                                                      NULL);
    // Set initial directory
    tmp = gtk_entry_get_text(GTK_ENTRY(entry));
    if (tmp && *tmp)
    {
        if (!gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(FileSelectionWindow),tmp))
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(FileSelectionWindow),tmp);
    }

    response = gtk_dialog_run(GTK_DIALOG(FileSelectionWindow));
    if (response == GTK_RESPONSE_ACCEPT)
    {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(FileSelectionWindow));
        filename_utf8 = filename_to_display(filename);
        gtk_entry_set_text(GTK_ENTRY(entry),filename_utf8);
        g_free (filename);
        g_free(filename_utf8);
        /* Useful for the button on the main window. */
        gtk_widget_grab_focus (GTK_WIDGET (entry));
	g_signal_emit_by_name (entry, "activate");
    }
	
    gtk_widget_destroy(FileSelectionWindow);
}



void
et_run_audio_player (GList *files)
{
    GFileInfo *info;
    GError *error = NULL;
    const gchar *content_type;
    GAppInfo *app_info;
    GdkAppLaunchContext *context;

    g_return_if_fail (files != NULL);

    info = g_file_query_info (files->data,
                              G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                              G_FILE_QUERY_INFO_NONE, NULL, &error);

    if (error)
    {
        g_warning ("Unable to get content type for file: %s",
                   error->message);
        g_error_free (error);
        return;
    }

    content_type = g_file_info_get_content_type (info);
    app_info = g_app_info_get_default_for_type (content_type, FALSE);
    g_object_unref (info);

    context = gdk_display_get_app_launch_context (gdk_display_get_default ());

    if (!g_app_info_launch (app_info, files, G_APP_LAUNCH_CONTEXT (context),
                            &error))
    {
        Log_Print (LOG_ERROR, _("Failed to launch program: %s"),
                   error->message);
        g_error_free (error);
    }

    g_object_unref (context);
    g_object_unref (app_info);
}

void
Run_Audio_Player_Using_Directory (void)
{
    GList *l;
    GList *file_list = NULL;

    for (l = g_list_first (ETCore->ETFileList); l != NULL; l = g_list_next (l))
    {
        ET_File *etfile = (ET_File *)l->data;
        gchar *path = ((File_Name *)etfile->FileNameCur->data)->value;
        file_list = g_list_prepend (file_list, g_file_new_for_path (path));
    }

    file_list = g_list_reverse (file_list);

    et_run_audio_player (file_list);

    g_list_free_full (file_list, g_object_unref);
}

/*
 * Check if the executable passed in parameter can be launched
 * Returns the full path of the file (must be freed if not used)
 */
gchar *Check_If_Executable_Exists (const gchar *program)
{
    gchar *program_tmp;
    gchar *tmp;

    g_return_val_if_fail (program != NULL, NULL);

    program_tmp = g_strdup(program);
    g_strstrip(program_tmp);

#ifdef G_OS_WIN32
    // Remove arguments if found, after '.exe'
    if ( (tmp=strstr(program_tmp,".exe")) )
        *(tmp + 4) = 0;
#else /* !G_OS_WIN32 */
    // Remove arguments if found
    if ( (tmp=strchr(program_tmp,' ')) )
        *tmp = 0;
#endif /* !G_OS_WIN32 */

    if (g_path_is_absolute(program_tmp))
    {
        if (access(program_tmp, X_OK) == 0)
        {
            return program_tmp;
        } else
        {
            g_free(program_tmp);
            return NULL;
        }
    } else
    {
        tmp = g_find_program_in_path(program_tmp);
        if (tmp)
        {
            g_free(program_tmp);
            return tmp;
        }else
        {
            g_free(program_tmp);
            return NULL;
        }
    }

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
        return g_strdup_printf("%d:%.2d",minute,second);

    hour   = duration/3600;
    minute = (duration%3600)/60;
    second = (duration%3600)%60;

    if (hour)
        data = g_strdup_printf("%d:%.2d:%.2d",hour,minute,second);
    else
        data = g_strdup_printf("%d:%.2d",minute,second);

    return data;
}

/*
 * et_show_help:
 */
void
et_show_help (void)
{
    GError *error = NULL;

    /* TODO: Add link to application help instead. */
    gtk_show_uri (gtk_window_get_screen (GTK_WINDOW (MainWindow)),
                  "help:easytag", GDK_CURRENT_TIME, &error);

    if (error)
    {
        g_debug ("Error while opening help: %s", error->message);
        g_error_free (error);
    }
}

/*
 * @filename: (type filename): the path to a file
 *
 * Gets the size of a file in bytes.
 *
 * Returns: the size of a file, in bytes
 */
goffset
et_get_file_size (const gchar *filename)
{
    GFile *file;
    GFileInfo *info;
    /* TODO: Take a GError from the caller. */
    GError *error = NULL;
    goffset size;

    g_return_val_if_fail (filename != NULL, 0);

    file = g_file_new_for_path (filename);
    info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_SIZE,
                              G_FILE_QUERY_INFO_NONE, NULL, &error);

    if (!info)
    {
        g_object_unref (file);
        g_error_free (error);
        return FALSE;
    }

    g_object_unref (file);

    size = g_file_info_get_size (info);
    g_object_unref (info);

    return size;
}

gchar *
et_disc_number_to_string (const guint disc_number)
{
    if (g_settings_get_boolean (MainSettings, "tag-disc-padded"))
    {
        return g_strdup_printf ("%.*d",
                                g_settings_get_uint (MainSettings,
                                                     "tag-disc-length"),
                                disc_number);
    }

    return g_strdup_printf ("%d", disc_number);
}

gchar *
et_track_number_to_string (const guint track_number)
{
    if (g_settings_get_boolean (MainSettings, "tag-number-padded"))
    {
        return g_strdup_printf ("%.*d", g_settings_get_uint (MainSettings,
                                                             "tag-number-length"),
                                track_number);
    }
    else
    {
        return g_strdup_printf ("%d", track_number);
    }
}

void
et_on_child_exited (GPid pid, gint status, gpointer user_data)
{
    g_spawn_close_pid (pid);
}
