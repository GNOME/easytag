/* musicbrainz_dialog.c - 2014/05/05 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2000-2014  Abhinav Jangda <abhijangda@hotmail.com>
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

#include "gtk2_compat.h"
#include "easytag.h"
#include "log.h"
#include "musicbrainz_dialog.h"
#include "mbentityview.h"
#include "mb_search.h"

/***************
 * Declaration *
 ***************/

static GNode *mb_tree_root;
static GSimpleAsyncResult *async_result;

typedef struct
{
    gchar *text_to_search;
    int type;
} ManualSearchThreadData;

/*************
 * Functions *
 *************/

/*
 * manual_search_callback:
 * @source: Source Object
 * @res: GAsyncResult
 * @user_data: User data
 *
 * Callback function for GAsyncResult for Manual Search.
 */
static void
manual_search_callback (GObject *source, GAsyncResult *res,
                        gpointer user_data)
{
    GtkComboBoxText *combo_box;

    if (!g_simple_async_result_get_op_res_gboolean (G_SIMPLE_ASYNC_RESULT (res)))
    {
        g_object_unref (res);
        g_free (user_data);
        free_mb_tree (mb_tree_root);
        mb_tree_root = g_node_new (NULL);
        return;
    }

    et_mb_entity_view_set_tree_root (ET_MB_ENTITY_VIEW (entityView),
                                     mb_tree_root);
    gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder, "statusbar")),
                        0, "Searching Completed");
    g_object_unref (res);
    g_free (user_data);

    combo_box = GTK_COMBO_BOX_TEXT (gtk_builder_get_object (builder, "cbManualSearch"));
    gtk_combo_box_text_append_text (combo_box,
                                    gtk_combo_box_text_get_active_text (combo_box));
}

/*
 * et_show_status_msg_in_idle_cb:
 * @obj: Source Object
 * @res: GAsyncResult
 * @user_data: User data
 *
 * Callback function for Displaying StatusBar Message.
 */
static void
et_show_status_msg_in_idle_cb (GObject *obj, GAsyncResult *res,
                               gpointer user_data)
{
    gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder, "statusbar")),
                        0, user_data);
    g_free (user_data);
    g_object_unref (res);
}

/*
 * et_show_status_msg_in_idle_cb:
 * @obj: Source Object
 * @res: GAsyncResult
 * @user_data: User data
 *
 * Function to Display StatusBar Messages in Main Thread.
 */
void
et_show_status_msg_in_idle (gchar *message)
{
    GSimpleAsyncResult *async_res;

    async_res = g_simple_async_result_new (NULL,
                                           et_show_status_msg_in_idle_cb,
                                           g_strdup (message),
                                           et_show_status_msg_in_idle);
    g_simple_async_result_complete_in_idle (async_res);
}

/*
 * mb5_search_error_callback:
 * @obj: Source Object
 * @res: GAsyncResult
 * @user_data: User data
 *
 * Callback function for displaying errors.
 */
void
mb5_search_error_callback (GObject *source, GAsyncResult *res,
                           gpointer user_data)
{
    GError *dest;
    dest = NULL;
    g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res),
                                           &dest);
    Log_Print (LOG_ERROR,
               _("Error searching MusicBrainz Database '%s'"), dest->message);
    gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder, "statusbar")),
                        0, dest->message);
    g_error_free (dest);
}

/*
 * manual_search_thread_func:
 * @res: GSimpleAsyncResult
 * @obj: Source GObject
 * @cancellable: GCancellable to cancel the operation
 *
 * Thread func of GSimpleAsyncResult to do Manual Search in another thread.
 */
static void
manual_search_thread_func (GSimpleAsyncResult *res, GObject *obj,
                           GCancellable *cancellable)
{
    GError *error;
    ManualSearchThreadData *thread_data;
    gchar *status_msg;

    error = NULL;
    g_simple_async_result_set_op_res_gboolean (res, FALSE);

    if (g_cancellable_is_cancelled (cancellable))
    {
        et_set_cancel_error (&error);
        g_simple_async_report_gerror_in_idle (NULL,
                                              mb5_search_error_callback,
                                              NULL, error);
        return;
    }

    thread_data = (ManualSearchThreadData *)g_async_result_get_user_data (G_ASYNC_RESULT (res));
    status_msg = g_strconcat ("Searching ", thread_data->text_to_search,
                              NULL);
    et_show_status_msg_in_idle (status_msg);
    g_free (status_msg);

    if (g_cancellable_is_cancelled (cancellable))
    {
        et_set_cancel_error (&error);
        g_simple_async_report_gerror_in_idle (NULL,
                                              mb5_search_error_callback,
                                              thread_data, error);
        return;
    }

    if (!et_musicbrainz_search (thread_data->text_to_search,
                                thread_data->type, mb_tree_root, &error,
                                cancellable))
    {
        g_simple_async_report_gerror_in_idle (NULL,
                                              mb5_search_error_callback,
                                              thread_data, error);
        return;
    }

    if (g_cancellable_is_cancelled (cancellable))
    {
        et_set_cancel_error (&error);
        g_simple_async_report_gerror_in_idle (NULL,
                                              mb5_search_error_callback,
                                              thread_data, error);
        return;
    }

    g_simple_async_result_set_op_res_gboolean (res, TRUE);
}

/*
 * btn_manual_stop_clicked:
 * @btn: GtkButton
 * @user_data: User data
 *
 * Signal Handler for "clicked" signal of btnManualFind.
 */
static void
btn_manual_find_clicked (GtkWidget *btn, gpointer user_data)
{
    GtkWidget *cb_manual_search;
    GtkWidget *cb_manual_search_in;
    int type;
    ManualSearchThreadData *thread_data;

    if (g_node_first_child (mb_tree_root))
    {
        free_mb_tree (mb_tree_root);
        mb_tree_root = g_node_new (NULL);
    }
  
    cb_manual_search = GTK_WIDGET (gtk_builder_get_object (builder,
                                                           "cbManualSearch"));
    cb_manual_search_in = GTK_WIDGET (gtk_builder_get_object (builder,
                                                              "cbManualSearchIn"));
    type = gtk_combo_box_get_active (GTK_COMBO_BOX (cb_manual_search_in));
    thread_data = g_malloc (sizeof (ManualSearchThreadData));
    thread_data->type = type;
    thread_data->text_to_search = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cb_manual_search));
    mb5_search_cancellable = g_cancellable_new ();
    gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder, "statusbar")),
                        0, "Starting MusicBrainz Search");
    async_result = g_simple_async_result_new (NULL, manual_search_callback, 
                                              thread_data,
                                              btn_manual_find_clicked);
    g_simple_async_result_run_in_thread (async_result,
                                         manual_search_thread_func, 0,
                                         mb5_search_cancellable);
}

/*
 * btn_manual_stop_clicked:
 * @btn: GtkButton
 * @user_data: User data
 *
 * Signal Handler for "clicked" signal of toolbtnToggleRedLines.
 */
static void
tool_btn_toggle_red_lines_clicked (GtkWidget *btn, gpointer user_data)
{
    et_mb_entity_view_toggle_red_lines (ET_MB_ENTITY_VIEW (entityView));
}

/*
 * btn_manual_stop_clicked:
 * @btn: GtkButton
 * @user_data: User data
 *
 * Signal Handler for "clicked" signal of toolbtnUp.
 */
static void
tool_btn_up_clicked (GtkWidget *btn, gpointer user_data)
{
    et_mb_entity_view_select_up (ET_MB_ENTITY_VIEW (entityView));
}

/*
 * btn_manual_stop_clicked:
 * @btn: GtkButton
 * @user_data: User data
 *
 * Signal Handler for "clicked" signal of toolbtnDown.
 */
static void
tool_btn_down_clicked (GtkWidget *btn, gpointer user_data)
{
    et_mb_entity_view_select_down (ET_MB_ENTITY_VIEW (entityView));
}

/*
 * btn_manual_stop_clicked:
 * @btn: GtkButton
 * @user_data: User data
 *
 * Signal Handler for "clicked" signal of toolbtnInvertSelection.
 */
static void
tool_btn_invert_selection_clicked (GtkWidget *btn, gpointer user_data)
{
    et_mb_entity_view_invert_selection (ET_MB_ENTITY_VIEW (entityView));
}

/*
 * btn_manual_stop_clicked:
 * @btn: GtkButton
 * @user_data: User data
 *
 * Signal Handler for "clicked" signal of toolbtnSelectAll.
 */
static void
tool_btn_select_all_clicked (GtkWidget *btn, gpointer user_data)
{
    et_mb_entity_view_select_all (ET_MB_ENTITY_VIEW (entityView));
}

/*
 * btn_manual_stop_clicked:
 * @btn: GtkButton
 * @user_data: User data
 *
 * Signal Handler for "clicked" signal of toolbtnUnselectAll.
 */
static void
tool_btn_unselect_all_clicked (GtkWidget *btn, gpointer user_data)
{
    et_mb_entity_view_unselect_all (ET_MB_ENTITY_VIEW (entityView));
}

/*
 * btn_manual_stop_clicked:
 * @btn: GtkButton
 * @user_data: User data
 *
 * Signal Handler for "clicked" signal of btnManualStop.
 */
static void
tool_btn_refersh_clicked (GtkWidget *btn, gpointer user_data)
{
    /* TODO: Implement Refresh Operation */
    if (et_mb_entity_view_get_current_level (ET_MB_ENTITY_VIEW (entityView)) >
        1)
    {
        /* Current level is more than 1, refereshing means downloading an */
        /* entity's children */
        
    }
}

/*
 * btn_manual_stop_clicked:
 * @btn: GtkButton
 * @user_data: User data
 *
 * Signal Handler for "clicked" signal of btnManualStop.
 */
static void
btn_manual_stop_clicked (GtkWidget *btn, gpointer user_data)
{
    g_cancellable_cancel (mb5_search_cancellable);
}

/*
 * entry_tree_view_search_changed:
 * @editable: GtkEditable for which handler is called
 * @user_data: User data
 *
 * Signal Handler for "changed" signal of entryTreeViewSearch.
 */
static void
entry_tree_view_search_changed (GtkEditable *editable, gpointer user_data)
{
    et_mb_entity_view_search_in_results (ET_MB_ENTITY_VIEW (entityView),
                                         gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder,
                                                                        "entryTreeViewSearch"))));
}

/*
 * et_open_musicbrainz_dialog:
 *
 * This function will open the musicbrainz dialog.
 */
void
et_open_musicbrainz_dialog ()
{
    GtkWidget *cb_manual_search_in;
    GError *error;

    builder = gtk_builder_new ();
    error = NULL;

    if (!gtk_builder_add_from_resource (builder,
                                        "/org/gnome/EasyTAG/musicbrainz_dialog.ui",
                                        &error))
    {
        Log_Print (LOG_ERROR,
                   _("Error loading MusicBrainz Search Dialog '%s'"),
                   error->message);
        g_error_free (error);
        g_object_unref (G_OBJECT (builder));
        return;
    }

    mb_tree_root = g_node_new (NULL);
    entityView = et_mb_entity_view_new ();
    mbDialog = GTK_WIDGET (gtk_builder_get_object (builder, "mbDialog"));
    gtk_box_pack_start (GTK_BOX (gtk_builder_get_object (builder, "centralBox")),
                        entityView, TRUE, TRUE, 2);
    /* FIXME: This should not be needed. */
    //gtk_box_reorder_child (GTK_BOX (gtk_builder_get_object (builder, "centralBox")),
    //                       entityView, 0);
    g_signal_connect (gtk_builder_get_object (builder, "btnManualFind"),
                      "clicked", G_CALLBACK (btn_manual_find_clicked),
                      NULL);
    g_signal_connect (gtk_builder_get_object (builder, "btnManualStop"),
                      "clicked", G_CALLBACK (btn_manual_stop_clicked),
                      NULL);
    g_signal_connect (gtk_builder_get_object (builder, "toolbtnUp"),
                      "clicked", G_CALLBACK (tool_btn_up_clicked),
                      NULL);
    g_signal_connect (gtk_builder_get_object (builder, "toolbtnDown"),
                      "clicked", G_CALLBACK (tool_btn_down_clicked),
                      NULL);
    g_signal_connect (gtk_builder_get_object (builder, "toolbtnSelectAll"),
                      "clicked", G_CALLBACK (tool_btn_select_all_clicked),
                      NULL);
    g_signal_connect (gtk_builder_get_object (builder, "toolbtnUnselectAll"),
                      "clicked", G_CALLBACK (tool_btn_unselect_all_clicked),
                      NULL);
    g_signal_connect (gtk_builder_get_object (builder, "toolbtnInvertSelection"),
                      "clicked", G_CALLBACK (tool_btn_invert_selection_clicked),
                      NULL);
    g_signal_connect (gtk_builder_get_object (builder, "toolbtnToggleRedLines"),
                      "clicked", G_CALLBACK (tool_btn_toggle_red_lines_clicked),
                      NULL);
    g_signal_connect_after (gtk_builder_get_object (builder, "entryTreeViewSearch"),
                            "changed",
                            G_CALLBACK (entry_tree_view_search_changed),
                            NULL);
    /* Fill Values in cb_manual_search_in */
    cb_manual_search_in = GTK_WIDGET (gtk_builder_get_object (builder,
                                                              "cbManualSearchIn"));

    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cb_manual_search_in), "Artist");
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cb_manual_search_in), "Album");
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cb_manual_search_in), "Track");

    gtk_widget_show_all (mbDialog);
    gtk_dialog_run (GTK_DIALOG (mbDialog));
    gtk_widget_destroy (mbDialog);
    g_object_unref (G_OBJECT (builder));
    free_mb_tree (mb_tree_root);
}