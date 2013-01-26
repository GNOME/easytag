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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <glib/gi18n-lib.h>
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

#ifdef G_OS_WIN32
#include "win32/win32dep.h"
#endif /* G_OS_WIN32 */


/****************
 * Declarations *
 ****************/

static GtkWidget *LogList = NULL;
static GtkListStore *logListModel;
/* Temporary list to store messages for the LogList when this control was not
 * yet created. */
static GList *LogPrintTmpList = NULL;
static gint LogListNbrRows;

enum
{
    LOG_PIXBUF,
    LOG_TIME_TEXT,
    LOG_TEXT,
    LOG_ROW_BACKGROUND,
    LOG_ROW_FOREGROUND,
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
static gboolean Log_Popup_Menu_Handler (GtkMenu *menu, GdkEventButton *event);
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
                                      G_TYPE_STRING,
                                      GDK_TYPE_COLOR,
                                      GDK_TYPE_COLOR);

    LogList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(logListModel));
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
                                        "background-gdk", LOG_ROW_BACKGROUND,
                                        "foreground-gdk", LOG_ROW_FOREGROUND,
                                        NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text",           LOG_TEXT,
                                        "background-gdk", LOG_ROW_BACKGROUND,
                                        "foreground-gdk", LOG_ROW_FOREGROUND,
                                        NULL);

    // Create Popup Menu on browser album list
    PopupMenu = gtk_ui_manager_get_widget(UIManager, "/LogPopup");
    g_signal_connect_swapped(G_OBJECT(LogList),"button_press_event",
                             G_CALLBACK(Log_Popup_Menu_Handler), G_OBJECT(PopupMenu));

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
Log_Popup_Menu_Handler (GtkMenu *menu, GdkEventButton *event)
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
        LogListNbrRows = 0;
    }
}


/*
 * Return time in allocated data
 */
static gchar *
Log_Format_Date (void)
{
    struct tm *tms;
    time_t nowtime;
    gchar *current_date, *current_work = g_malloc0(21);

    // Get current time and date
    nowtime = time(NULL);
    tms = localtime(&nowtime);
    strftime(current_work,20,"%X",tms); // Time without date in current locale
    //strftime(current_work,20,"%x",ptr); // Date without time in current locale
    current_date = Try_To_Validate_Utf8_String(current_work);
    g_free(current_work);

    return current_date;
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
    FILE *file = NULL;


    va_start (args, format);
    string = g_strdup_vprintf(format, args);
    va_end (args);

    // If the log window is displayed then messages are displayed, else
    // the messages are stored in a temporary list.
    if (LogList && logListModel)
    {
        gchar *time = Log_Format_Date();

        // Remove lines that exceed the limit
        if (LogListNbrRows > LOG_MAX_LINES - 1
        &&  gtk_tree_model_get_iter_first(GTK_TREE_MODEL(logListModel), &iter))
        {
            gtk_list_store_remove(GTK_LIST_STORE(logListModel), &iter);
        }

        LogListNbrRows++;
        gtk_list_store_append(logListModel, &iter);
        gtk_list_store_set(logListModel, &iter,
                           LOG_PIXBUF,         Log_Get_Stock_Id_From_Error_Type(error_type),
                           LOG_TIME_TEXT,      time,
                           LOG_TEXT,           string,
                           LOG_ROW_BACKGROUND, NULL,
                           LOG_ROW_FOREGROUND, NULL,
                           -1);
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

                return;
            }
        }

        file_path = g_build_filename (cache_path, LOG_FILE, NULL);
        g_free (cache_path);
    }

    // The first time, the whole file is deleted. Else, text is appended.
    if (first_time)
        file = fopen(file_path,"w+");
    else
        file = fopen(file_path,"a+");
    //g_free(file_path);

    if (file)
    {
        gchar *time = Log_Format_Date();
        gchar *data = g_strdup_printf("%s %s\n",time,string);
        fwrite(data,strlen(data),1,file);
        g_free(data);
        g_free(time);

        first_time = FALSE;
        fclose(file);
    }

    g_free(string);
}

/*
 * Display pending messages in the LogList
 */
static void
Log_Print_Tmp_List (void)
{
    GtkTreeIter iter;

    LogPrintTmpList = g_list_first(LogPrintTmpList);
    while (LogPrintTmpList)
    {

        if (LogList && logListModel)
        {
            LogListNbrRows++;
            gtk_list_store_append(logListModel, &iter);
            gtk_list_store_set(logListModel, &iter,
                               LOG_PIXBUF,    Log_Get_Stock_Id_From_Error_Type( ((Log_Data *)LogPrintTmpList->data)->error_type ),
                               LOG_TIME_TEXT, ((Log_Data *)LogPrintTmpList->data)->time,
                               LOG_TEXT,      ((Log_Data *)LogPrintTmpList->data)->string,
                               LOG_ROW_BACKGROUND, NULL,
                               LOG_ROW_FOREGROUND, NULL,
                               -1);
            Log_List_Set_Row_Visible(GTK_TREE_MODEL(logListModel), &iter);
        }

        if (!LogPrintTmpList->next) break;
        LogPrintTmpList = LogPrintTmpList->next;
    }

    // Free the list...
    if (LogPrintTmpList)
    {
        LogPrintTmpList = g_list_first(LogPrintTmpList);
        while (LogPrintTmpList)
        {
            g_free(((Log_Data *)LogPrintTmpList->data)->string);
            g_free(((Log_Data *)LogPrintTmpList->data)->time);
            g_free(((Log_Data *)LogPrintTmpList->data));

            if (!LogPrintTmpList->next) break;
            LogPrintTmpList = LogPrintTmpList->next;
        }

        g_list_free(LogPrintTmpList);
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
            stock_id = GTK_STOCK_APPLY;
            break;

        case LOG_INFO:
            stock_id = GTK_STOCK_DIALOG_INFO;
            break;

        case LOG_WARNING:
            stock_id = GTK_STOCK_DIALOG_WARNING;
            break;

        case LOG_ERROR:
            stock_id = GTK_STOCK_CANCEL;
            break;

        case LOG_UNKNOWN:
        default:
            stock_id = NULL;
            break;
    }

    return stock_id;
}
