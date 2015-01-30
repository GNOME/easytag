/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
 * Copyright (C) 2000-2007  Jerome Couderc <easytag@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include <glib/gi18n.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "log.h"
#include "application_window.h"
#include "easytag.h"
#include "setting.h"
#include "charset.h"

#include "win32/win32dep.h"

/* TODO: Use G_DEFINE_TYPE_WITH_PRIVATE. */
G_DEFINE_TYPE (EtLogArea, et_log_area, GTK_TYPE_BIN)

#define et_log_area_get_instance_private(self) (self->priv)

struct _EtLogAreaPrivate
{
    GtkWidget *log_view;
    GtkListStore *log_model;

    /* Popup menu. */
    GtkWidget *menu;
};

enum
{
    LOG_ICON_NAME,
    LOG_TIME_TEXT,
    LOG_TEXT,
    LOG_COLUMN_COUNT
};

/* File for log. */
static const gchar LOG_FILE[] = "easytag.log";

/**************
 * Prototypes *
 **************/
static void Log_List_Set_Row_Visible (EtLogArea *self, GtkTreeIter *rowIter);
static gchar *Log_Format_Date (void);



/*************
 * Functions *
 *************/

static void
do_popup_menu (EtLogArea *self,
               GdkEventButton *event)
{
    EtLogAreaPrivate *priv;
    gint button;
    gint event_time;

    priv = et_log_area_get_instance_private (self);

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
    gtk_menu_popup (GTK_MENU (priv->menu), NULL, NULL, NULL, NULL, button,
                    event_time);
}

static gboolean
on_popup_menu (GtkWidget *treeview,
               EtLogArea *self)
{
    do_popup_menu (self, NULL);

    return GDK_EVENT_STOP;
}

/*
 * Log_Popup_Menu_Handler : displays the corresponding menu
 */
static gboolean
on_button_press_event (GtkWidget *treeview,
                       GdkEventButton *event,
                       EtLogArea *self)
{
    if (gdk_event_triggers_context_menu ((GdkEvent *)event))
    {
        do_popup_menu (self, event);

        return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
}


static void
et_log_area_class_init (EtLogAreaClass *klass)
{
    g_type_class_add_private (klass, sizeof (EtLogAreaPrivate));
}

static void
et_log_area_init (EtLogArea *self)
{
    EtLogAreaPrivate *priv;
    GtkWidget *grid;
    GtkBuilder *builder;
    GError *error = NULL;
    GMenuModel *menu_model;

    priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, ET_TYPE_LOG_AREA,
                                                     EtLogAreaPrivate);

    gtk_container_set_border_width (GTK_CONTAINER (self), 2);

    builder = gtk_builder_new ();
    gtk_builder_add_from_resource (builder, "/org/gnome/EasyTAG/log_area.ui",
                                   &error);

    if (error != NULL)
    {
        g_error ("Unable to get log area from resource: %s",
                 error->message);
    }

    grid = GTK_WIDGET (gtk_builder_get_object (builder, "log_grid"));
    gtk_container_add (GTK_CONTAINER (self), grid);

    /* The list. */
    priv->log_model = GTK_LIST_STORE (gtk_builder_get_object (builder,
                                                              "log_model"));
    priv->log_view = GTK_WIDGET (gtk_builder_get_object (builder, "log_view"));

    /* Create popup menu. */
    gtk_builder_add_from_resource (builder, "/org/gnome/EasyTAG/menus.ui",
                                   &error);

    if (error != NULL)
    {
        g_error ("Unable to get popup menu from resource: %s",
                 error->message);
    }

    menu_model = G_MENU_MODEL (gtk_builder_get_object (builder, "log-menu"));
    priv->menu = gtk_menu_new_from_model (menu_model);
    gtk_menu_attach_to_widget (GTK_MENU (priv->menu), priv->log_view, NULL);

    g_object_unref (builder);

    g_signal_connect (priv->log_view, "popup-menu", G_CALLBACK (on_popup_menu),
                      self);
    g_signal_connect (priv->log_view, "button-press-event",
                      G_CALLBACK (on_button_press_event), self);
}


GtkWidget *
et_log_area_new ()
{
    return g_object_new (ET_TYPE_LOG_AREA, NULL);
}

/*
 * Set a row visible in the log list (by scrolling the list)
 */
static void
Log_List_Set_Row_Visible (EtLogArea *self, GtkTreeIter *rowIter)
{
    EtLogAreaPrivate *priv;
    /*
     * TODO: Make this only scroll to the row if it is not visible
     * (like in easytag GTK1)
     * See function gtk_tree_view_get_visible_rect() ??
     */
    GtkTreePath *rowPath;

    priv = et_log_area_get_instance_private (self);

    g_return_if_fail (priv->log_model != NULL);

    rowPath = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->log_model),
                                       rowIter);
    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (priv->log_view), rowPath,
                                  NULL, FALSE, 0, 0);
    gtk_tree_path_free (rowPath);
}


/*
 * Remove all lines in the log list
 */
void
et_log_area_clear (EtLogArea *self)
{
    EtLogAreaPrivate *priv;

    g_return_if_fail (ET_LOG_AREA (self));

    priv = et_log_area_get_instance_private (self);

    if (priv->log_model)
    {
        gtk_list_store_clear (priv->log_model);
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

static const gchar *
get_icon_name_from_error_kind (EtLogAreaKind error_kind)
{
    switch (error_kind)
    {
        /* Same icon for information and OK messages. */
        case LOG_OK:
        case LOG_INFO:
            return "dialog-information";
            break;
        case LOG_WARNING:
            return "dialog-warning";
            break;
        case LOG_ERROR:
            return "dialog-error";
            break;
        case LOG_UNKNOWN:
            return NULL;
            break;
        default:
            g_assert_not_reached ();
    }
}

/*
 * Function to use anywhere in the application to send a message to the LogList
 */
void
Log_Print (EtLogAreaKind error_type, const gchar * const format, ...)
{
    EtLogArea *self;
    EtLogAreaPrivate *priv;
    va_list args;
    gchar *string;
    gchar *time;
    gint n_items;
    GtkTreeIter iter;
    static gboolean first_time = TRUE;
    static gchar *file_path = NULL;
    GFile *file;
    GFileOutputStream *file_ostream;
    GError *error = NULL;

    self = ET_LOG_AREA (et_application_window_get_log_area (ET_APPLICATION_WINDOW (MainWindow)));

    g_return_if_fail (self != NULL);

    priv = et_log_area_get_instance_private (self);

    va_start (args, format);
    string = g_strdup_vprintf (format, args);
    va_end (args);

    time = Log_Format_Date ();

    /* Remove lines that exceed the limit. */
    n_items = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (priv->log_model),
                                              NULL);

    if (n_items > g_settings_get_uint (MainSettings, "log-lines") - 1
        && gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->log_model),
                                          &iter))
    {
        gtk_list_store_remove (GTK_LIST_STORE (priv->log_model), &iter);
    }

    gtk_list_store_insert_with_values (priv->log_model, &iter, G_MAXINT,
                                       LOG_ICON_NAME,
                                       get_icon_name_from_error_kind (error_type),
                                       LOG_TIME_TEXT, time, LOG_TEXT,
                                       string, -1);
    Log_List_Set_Row_Visible (self, &iter);
    g_free (time);

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
