/* log.c - 2007/03/25 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2000-2007  Jerome Couderc <easytag@gmail.com>
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

#include <config.h>

#include <glib/gi18n.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "log.h"
#include "easytag.h"
#include "bar.h"
#include "setting.h"
#include "charset.h"

#include "win32/win32dep.h"


/****************
 * Declarations *
 ****************/

static GtkWidget *LogList = NULL;
static GtkListStore *logListModel;
/* Temporary list to store messages for the LogList when this control was not
 * yet created. */
static GList *LogPrintTmpList = NULL;

enum
{
    LOG_PIXBUF,
    LOG_TIME_TEXT,
    LOG_TEXT,
    LOG_COLUMN_COUNT
};

// File for log
static const gchar LOG_FILE[] = "easytag.log";

// Structure used to store information for the temporary list
typedef struct _Log_Data Log_Data;
struct _Log_Data
{
    gchar         *time;    /* The time of this line of log */
    Log_Error_Type error_type;
    gchar         *string;  /* The string of the line of log to display */
};


/**************
 * Prototypes *
 **************/
static gboolean Log_Popup_Menu_Handler (GtkWidget *treeview,
                                        GdkEventButton *event, GtkMenu *menu);
static void Log_List_Set_Row_Visible (GtkTreeModel *treeModel,
                                      GtkTreeIter *rowIter);
static void Log_Print_Tmp_List (void);
static gchar *Log_Format_Date (void);
static gchar *Log_Get_Stock_Id_From_Error_Type (Log_Error_Type error_type);



/*************
 * Functions *
 *************/

GtkWidget *Create_Log_Area (void)
{
    GtkWidget *Frame;
    GtkWidget *ScrollWindowLogList;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkWidget *PopupMenu;


    Frame = gtk_frame_new(_("Log"));
    gtk_container_set_border_width(GTK_CONTAINER(Frame), 2);

    /*
     * The ScrollWindow and the List
     */
    ScrollWindowLogList = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ScrollWindowLogList),
                                   GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(Frame),ScrollWindowLogList);

    /* The file list */
    logListModel = gtk_list_store_new(LOG_COLUMN_COUNT,
                                      G_TYPE_STRING,
                                      G_TYPE_STRING,
                                      G_TYPE_STRING);

    LogList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(logListModel));
    g_object_unref (logListModel);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(LogList), FALSE);
    gtk_container_add(GTK_CONTAINER(ScrollWindowLogList), LogList);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(LogList), FALSE);
    gtk_widget_set_size_request(LogList, 0, 0);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(LogList), FALSE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(LogList)),GTK_SELECTION_MULTIPLE);

    column = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(LogList), column);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                       "stock-id", LOG_PIXBUF,
                                        NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           LOG_TIME_TEXT,
                                        NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           LOG_TEXT,
                                        NULL);

    // Create Popup Menu on browser album list
    PopupMenu = gtk_ui_manager_get_widget(UIManager, "/LogPopup");
    gtk_menu_attach_to_widget (GTK_MENU (PopupMenu), LogList, NULL);
    g_signal_connect (G_OBJECT (LogList), "button-press-event",
                      G_CALLBACK (Log_Popup_Menu_Handler), PopupMenu);

    // Load pending messages in the Log list
    Log_Print_Tmp_List();

    if (SHOW_LOG_VIEW)
        //gtk_widget_show_all(ScrollWindowLogList);
        gtk_widget_show_all(Frame);

    //return ScrollWindowLogList;
    return Frame;
}


/*
 * Log_Popup_Menu_Handler : displays the corresponding menu
 */
static gboolean
Log_Popup_Menu_Handler (GtkWidget *treeview, GdkEventButton *event,
                        GtkMenu *menu)
{
    if (event && (event->type==GDK_BUTTON_PRESS) && (event->button == 3))
    {
        gtk_menu_popup(menu,NULL,NULL,NULL,NULL,event->button,event->time);
        return TRUE;
    }
    return FALSE;
}


/*
 * Set a row visible in the log list (by scrolling the list)
 */
static void
Log_List_Set_Row_Visible (GtkTreeModel *treeModel, GtkTreeIter *rowIter)
{
    /*
     * TODO: Make this only scroll to the row if it is not visible
     * (like in easytag GTK1)
     * See function gtk_tree_view_get_visible_rect() ??
     */
    GtkTreePath *rowPath;

    g_return_if_fail (treeModel != NULL);

    rowPath = gtk_tree_model_get_path(treeModel, rowIter);
    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(LogList), rowPath, NULL, FALSE, 0, 0);
    gtk_tree_path_free(rowPath);
}


/*
 * Remove all lines in the log list
 */
void Log_Clean_Log_List (void)
{
    if (logListModel)
    {
        gtk_list_store_clear(logListModel);
    }
}


/*
 * Return time in allocated data
 */
static gchar *
Log_Format_Date (void)
{
    GDateTime *dt;
    gchar *time;

    dt = g_date_time_new_now_local ();
    /* Time without date in current locale. */
    time = g_date_time_format (dt, "%X");

    g_date_time_unref (dt);

    return time;
}


/*
 * Function to use anywhere in the application to send a message to the LogList
 */
void Log_Print (Log_Error_Type error_type, gchar const *format, ...)
{
    va_list args;
    gchar *string;

    GtkTreeIter iter;
    static gboolean first_time = TRUE;
    static gchar *file_path = NULL;
    GFile *file;
    GFileOutputStream *file_ostream;
    GError *error = NULL;

    va_start (args, format);
    string = g_strdup_vprintf(format, args);
    va_end (args);

    // If the log window is displayed then messages are displayed, else
    // the messages are stored in a temporary list.
    if (LogList && logListModel)
    {
        gint n_items;
        gchar *time = Log_Format_Date();

        /* Remove lines that exceed the limit. */
        n_items = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (logListModel),
                                                  NULL);

        if (n_items > LOG_MAX_LINES - 1
        &&  gtk_tree_model_get_iter_first(GTK_TREE_MODEL(logListModel), &iter))
        {
            gtk_list_store_remove(GTK_LIST_STORE(logListModel), &iter);
        }

        gtk_list_store_insert_with_values (logListModel, &iter, G_MAXINT,
                                           LOG_PIXBUF,
                                           Log_Get_Stock_Id_From_Error_Type (error_type),
                                           LOG_TIME_TEXT, time, LOG_TEXT,
                                           string, -1);
        Log_List_Set_Row_Visible(GTK_TREE_MODEL(logListModel), &iter);
        g_free(time);
    }else
    {
        Log_Data *LogData = g_malloc0(sizeof(Log_Data));
        LogData->time       = Log_Format_Date();
        LogData->error_type = error_type;
        LogData->string     = g_strdup(string);

        LogPrintTmpList = g_list_append(LogPrintTmpList,LogData);
        //g_print("%s",string);
    }

    // Store also the messages in the log file.
    if (!file_path)
    {
        gchar *cache_path = g_build_filename (g_get_user_cache_dir (),
                                              PACKAGE_TARNAME, NULL);

        if (!g_file_test (cache_path, G_FILE_TEST_IS_DIR))
        {
            gint result = g_mkdir_with_parents (cache_path, S_IRWXU);

            if (result == -1)
            {
                g_printerr ("%s", "Unable to create cache directory");
                g_free (cache_path);
                g_free (string);

                return;
            }
        }

        file_path = g_build_filename (cache_path, LOG_FILE, NULL);
        g_free (cache_path);
    }

    file = g_file_new_for_path (file_path);

    /* On startup, the log is cleared. The log is then appended to for the
     * remainder of the application lifetime. */
    if (first_time)
    {
        file_ostream = g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE,
                                       NULL, &error);
    }
    else
    {
        file_ostream = g_file_append_to (file, G_FILE_CREATE_NONE, NULL,
                                         &error);
    }

    if (file_ostream)
    {
        gchar *time;
        GString *data;
        gsize bytes_written;

        time = Log_Format_Date ();
        data = g_string_new (time);
        g_free (time);

        data = g_string_append_c (data, ' ');
        data = g_string_append (data, string);
        g_free (string);

        data = g_string_append_c (data, '\n');

        if (!g_output_stream_write_all (G_OUTPUT_STREAM (file_ostream),
                                        data->str, data->len, &bytes_written,
                                        NULL, &error))
        {
            g_debug ("Only %" G_GSIZE_FORMAT " bytes out of %" G_GSIZE_FORMAT
                     "bytes of data were written", bytes_written, data->len);

            /* To avoid recursion of Log_Print. */
            g_warning ("Error writing to the log file '%s' ('%s')", file_path,
                       error->message);

            g_error_free (error);

            g_string_free (data, TRUE);
            g_object_unref (file_ostream);
            g_object_unref (file);

            return;
        }

        first_time = FALSE;

        g_string_free (data, TRUE);
    }
    else
    {
        g_warning ("Error opening output stream of file '%s' ('%s')",
                   file_path, error->message);
        g_error_free (error);
    }

    g_object_unref (file_ostream);
    g_object_unref (file);
}

/*
 * Display pending messages in the LogList
 */
static void
Log_Print_Tmp_List (void)
{
    GList *l;
    GtkTreeIter iter;

    LogPrintTmpList = g_list_first (LogPrintTmpList);
    for (l = LogPrintTmpList; l != NULL; l = g_list_next (l))
    {
        if (LogList && logListModel)
        {
            gtk_list_store_insert_with_values (logListModel, &iter, G_MAXINT,
                                               LOG_PIXBUF,
                                               Log_Get_Stock_Id_From_Error_Type (((Log_Data *)l->data)->error_type),
                                               LOG_TIME_TEXT,
                                               ((Log_Data *)l->data)->time,
                                               LOG_TEXT,
                                               ((Log_Data *)l->data)->string,
                                               -1);
            Log_List_Set_Row_Visible(GTK_TREE_MODEL(logListModel), &iter);
        }
    }

    // Free the list...
    if (LogPrintTmpList)
    {
        GList *l;

        for (l = LogPrintTmpList; l != NULL; l = g_list_next (l))
        {
            g_free (((Log_Data *)l->data)->string);
            g_free (((Log_Data *)l->data)->time);
            g_free (((Log_Data *)l->data));
        }

        g_list_free (LogPrintTmpList);
        LogPrintTmpList = NULL;
    }
}


static gchar *
Log_Get_Stock_Id_From_Error_Type (Log_Error_Type error_type)
{
    gchar *stock_id;

    switch (error_type)
    {
        case LOG_OK:
            stock_id = GTK_STOCK_OK;
            break;

        case LOG_INFO:
            stock_id = GTK_STOCK_DIALOG_INFO;
            break;

        case LOG_WARNING:
            stock_id = GTK_STOCK_DIALOG_WARNING;
            break;

        case LOG_ERROR:
            stock_id = GTK_STOCK_DIALOG_ERROR;
            break;

        case LOG_UNKNOWN:
            stock_id = NULL;
            break;
        default:
            g_assert_not_reached ();
    }

    return stock_id;
}
