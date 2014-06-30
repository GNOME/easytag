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

#ifdef ENABLE_libmusicbrainz

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <discid/discid.h>

#include "gtk2_compat.h"
#include "easytag.h"
#include "log.h"
#include "musicbrainz_dialog.h"
#include "mbentityview.h"
#include "mb_search.h"
#include "browser.h"

/***************
 * Declaration *
 ***************/
static void
get_first_selected_file (ET_File  **et_file);

enum TagChoiceColumns
{
    TAG_CHOICE_TITLE,
    TAG_CHOICE_ALBUM,
    TAG_CHOICE_ARTIST,
    TAG_CHOICE_ALBUM_ARTIST,
    TAG_CHOICE_DATE,
    TAG_CHOICE_COUNTRY,
    TAG_CHOICE_COLS_N
};
        
typedef enum
{
    ET_MB_SEARCH_TYPE_MANUAL,
} EtMbSearchType;

typedef struct
{
    EtMbSearchType type;
} EtMbSearch;

typedef struct
{
    EtMbSearch parent;
    gchar *to_search;
    MbEntityKind to_search_type;
    GNode *parent_node;
} EtMbManualSearch;

typedef struct
{
    GNode *mb_tree_root;
    GSimpleAsyncResult *async_result;
    EtMbSearch *search;
    GtkBuilder *tag_choices_builder;
    GtkWidget *tag_choices_dialog;
    GtkTreeModel *tag_choices_list_store;
} MusicBrainzDialogPrivate;

static MusicBrainzDialogPrivate *mb_dialog_priv;

/*
 * ManualSearchThreadData:
 * @text_to_search: Manual Search Text
 * @type: Type of Entity
 *
 * Thread Data for storing Manual Search's data.
 */
typedef struct
{
    gchar *text_to_search;
    int type;
} ManualSearchThreadData;

/****************
 * Declarations *
 ***************/

static void
btn_close_clicked (GtkWidget *button, gpointer data);

/*************
 * Functions *
 *************/

static void
et_musicbrainz_set_search_manual (EtMbSearch **search, gchar *to_search,
                                  GNode *node, MbEntityKind type)
{
    if (*search)
    {
        if ((*search)->type == ET_MB_SEARCH_TYPE_MANUAL)
        {
            g_free (((EtMbManualSearch *)(*search))->to_search);
        }

        g_free (*search);
    }

    *search = g_malloc (sizeof (EtMbManualSearch));
    ((EtMbManualSearch *)(*search))->to_search = g_strdup (to_search);
    (*search)->type = ET_MB_SEARCH_TYPE_MANUAL;
    ((EtMbManualSearch *)(*search))->parent_node = node;
    ((EtMbManualSearch *)(*search))->to_search_type = type;
}

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

        if (mb_dialog_priv)
        {
            free_mb_tree (&mb_dialog_priv->mb_tree_root);
            mb_dialog_priv->mb_tree_root = g_node_new (NULL);
        }

        return;
    }

    et_mb_entity_view_set_tree_root (ET_MB_ENTITY_VIEW (entityView),
                                     mb_dialog_priv->mb_tree_root);
    gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder,
                        "statusbar")), 0, _("Searching Completed"));
    g_object_unref (res);
    combo_box = GTK_COMBO_BOX_TEXT (gtk_builder_get_object (builder,
                                                            "cbManualSearch"));
    gtk_combo_box_text_append_text (combo_box,
                                    gtk_combo_box_text_get_active_text (combo_box));
    et_music_brainz_dialog_stop_set_sensitive (FALSE);
    mb_dialog_priv->search = ET_MB_SEARCH_TYPE_MANUAL;
    et_musicbrainz_set_search_manual (&mb_dialog_priv->search,
                                      ((ManualSearchThreadData *)user_data)->text_to_search,
                                      mb_dialog_priv->mb_tree_root,
                                      ((ManualSearchThreadData *)user_data)->type);
    g_free (user_data);
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
    gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder,
                        "statusbar")), 0, user_data);
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
    gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder,
                                       "statusbar")), 0, dest->message);
    g_error_free (dest);
    et_music_brainz_dialog_stop_set_sensitive (FALSE);

    if (exit_on_complete)
    {
        et_music_brainz_dialog_destroy (mbDialog);
    }
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
        g_set_error (&error, ET_MB5_SEARCH_ERROR,
                     ET_MB5_SEARCH_ERROR_CANCELLED,
                     _("Operation cancelled by user"));
        g_simple_async_report_gerror_in_idle (NULL,
                                              mb5_search_error_callback,
                                              NULL, error);
        return;
    }

    thread_data = (ManualSearchThreadData *)g_async_result_get_user_data (G_ASYNC_RESULT (res));
    status_msg = g_strdup_printf (_("Searching %s"), thread_data->text_to_search);
    et_show_status_msg_in_idle (status_msg);
    g_free (status_msg);

    if (g_cancellable_is_cancelled (cancellable))
    {
        g_set_error (&error, ET_MB5_SEARCH_ERROR,
                     ET_MB5_SEARCH_ERROR_CANCELLED,
                     _("Operation cancelled by user"));
        g_simple_async_report_gerror_in_idle (NULL,
                                              mb5_search_error_callback,
                                              thread_data, error);
        return;
    }

    if (!et_musicbrainz_search (thread_data->text_to_search,
                                thread_data->type, mb_dialog_priv->mb_tree_root, &error,
                                cancellable))
    {
        g_simple_async_report_gerror_in_idle (NULL,
                                              mb5_search_error_callback,
                                              thread_data, error);
        return;
    }

    if (g_cancellable_is_cancelled (cancellable))
    {
        g_set_error (&error, ET_MB5_SEARCH_ERROR,
                     ET_MB5_SEARCH_ERROR_CANCELLED,
                     _("Operation cancelled by user"));
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

    cb_manual_search_in = GTK_WIDGET (gtk_builder_get_object (builder,
                                                              "cbManualSearchIn"));
    type = gtk_combo_box_get_active (GTK_COMBO_BOX (cb_manual_search_in));

    if (type == -1)
    {
        return;
    }

    if (g_node_first_child (mb_dialog_priv->mb_tree_root))
    {
        free_mb_tree (&mb_dialog_priv->mb_tree_root);
        mb_dialog_priv->mb_tree_root = g_node_new (NULL);
    }
 
    et_mb_entity_view_clear_all (ET_MB_ENTITY_VIEW (entityView));
    cb_manual_search = GTK_WIDGET (gtk_builder_get_object (builder,
                                                           "cbManualSearch"));
    thread_data = g_malloc (sizeof (ManualSearchThreadData));
    thread_data->type = type;
    thread_data->text_to_search = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cb_manual_search));
    mb5_search_cancellable = g_cancellable_new ();
    gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder,
                        "statusbar")), 0, _("Starting MusicBrainz Search"));
    mb_dialog_priv->async_result = g_simple_async_result_new (NULL, manual_search_callback,
                                              thread_data,
                                              btn_manual_find_clicked);
    g_simple_async_result_run_in_thread (mb_dialog_priv->async_result,
                                         manual_search_thread_func, 0,
                                         mb5_search_cancellable);
    et_music_brainz_dialog_stop_set_sensitive (TRUE);
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
tool_btn_refresh_clicked (GtkWidget *btn, gpointer user_data)
{
    if (!mb_dialog_priv->search)
    {
        return;
    }

    /* TODO: Implement Refresh Operation */
    if (et_mb_entity_view_get_current_level (ET_MB_ENTITY_VIEW (entityView)) >
        1)
    {
        /* Current level is more than 1, refereshing means downloading an */
        /* entity's children */
        et_mb_entity_view_refresh_current_level (ET_MB_ENTITY_VIEW (entityView));
        return;
    }

    if (mb_dialog_priv->search->type == ET_MB_SEARCH_TYPE_MANUAL)
    {
        EtMbManualSearch *manual_search;
        GtkWidget *entry;

        manual_search = (EtMbManualSearch *)mb_dialog_priv->search;
        et_mb_entity_view_clear_all (ET_MB_ENTITY_VIEW (entityView));
        free_mb_tree (&mb_dialog_priv->mb_tree_root);
        mb_dialog_priv->mb_tree_root = g_node_new (NULL);
        entry = gtk_bin_get_child (GTK_BIN (gtk_builder_get_object (builder, "cbManualSearch")));
        gtk_entry_set_text (GTK_ENTRY (entry), manual_search->to_search);
        gtk_combo_box_set_active (GTK_COMBO_BOX (gtk_builder_get_object (builder, "cbManualSearchIn")),
                                  manual_search->to_search_type);
        btn_manual_find_clicked (NULL, NULL);
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
    if (G_IS_CANCELLABLE (mb5_search_cancellable))
    {
        g_cancellable_cancel (mb5_search_cancellable);
    }
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

static void
bt_selected_find_clicked (GtkWidget *widget, gpointer user_data)
{
    ET_File *et_file;
    File_Tag *file_tag;
    GtkWidget *cb_manual_search;
    GtkWidget *cb_manual_search_in;
    GtkWidget *entry;
    int type;

    et_file = NULL;
    get_first_selected_file (&et_file);

    if (!et_file)
    {
        gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder, "statusbar")),
                        0, _("No File Selected."));
        return;
    }

    file_tag = (File_Tag *)et_file->FileTag->data;
    cb_manual_search_in = GTK_WIDGET (gtk_builder_get_object (builder,
                                                              "cbManualSearchIn"));
    type = gtk_combo_box_get_active (GTK_COMBO_BOX (cb_manual_search_in));

    if (type == -1)
    {
        return;
    }

    cb_manual_search = GTK_WIDGET (gtk_builder_get_object (builder,
                                                           "cbManualSearch"));
    entry = gtk_bin_get_child (GTK_BIN (cb_manual_search));

    if (type == MB_ENTITY_KIND_ARTIST)
    {
        if (file_tag->artist && *(file_tag->artist))
        {
            gtk_entry_set_text (GTK_ENTRY (entry), file_tag->artist);
        }
        else
        {
            gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder, "statusbar")),
                        0, _("Artist of current file is not set"));
        }
    }
    else if (type == MB_ENTITY_KIND_ALBUM)
    {
        if (file_tag->album && *(file_tag->album))
        {
            gtk_entry_set_text (GTK_ENTRY (entry), file_tag->album);
        }
        else
        {
            gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder, "statusbar")),
                        0, _("Album of current file is not set"));
        }
    }
    else if (type == MB_ENTITY_KIND_TRACK)
    {
        if (file_tag->title && *(file_tag->title))
        {
            gtk_entry_set_text (GTK_ENTRY (entry), file_tag->title);
        }
        else
        {
            gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder, "statusbar")),
                        0, _("Track of current file is not set"));
        }
    }

    btn_manual_find_clicked (NULL, NULL);
}

static void
get_first_selected_file (ET_File  **et_file)
{
    GtkListStore *tree_model;
    GtkTreeSelection *selection;
    int count;

    tree_model = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(BrowserList)));
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
    count = gtk_tree_selection_count_selected_rows(selection);
    *et_file = NULL;

    if (count > 0)
    {
        GList* list_sel_rows;
        GtkTreeIter iter;

        list_sel_rows = gtk_tree_selection_get_selected_rows(selection, NULL);
        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(tree_model),
                                    &iter,
                                    (GtkTreePath *)list_sel_rows->data))
        {
            *et_file = Browser_List_Get_ETFile_From_Iter (&iter);
        }

        g_list_free_full (list_sel_rows,
                          (GDestroyNotify)gtk_tree_path_free);
    }
}

static void
discid_search_callback (GObject *source, GAsyncResult *res,
                        gpointer user_data)
{
    if (!g_simple_async_result_get_op_res_gboolean (G_SIMPLE_ASYNC_RESULT (res)))
    {
        g_object_unref (res);
        free_mb_tree (&mb_dialog_priv->mb_tree_root);
        mb_dialog_priv->mb_tree_root = g_node_new (NULL);
        return;
    }

    et_mb_entity_view_set_tree_root (ET_MB_ENTITY_VIEW (entityView),
                                     mb_dialog_priv->mb_tree_root);
    gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder, "statusbar")),
                        0, _("Searching Completed"));
    g_object_unref (res);
    g_free (user_data);
    et_music_brainz_dialog_stop_set_sensitive (FALSE);

    if (exit_on_complete)
    {
        btn_close_clicked (NULL, NULL);
    }
}

static void
discid_search_thread_func (GSimpleAsyncResult *res, GObject *obj,
                           GCancellable *cancellable)
{
    GError *error;
    DiscId *disc;
    gchar *discid;

    error = NULL;
    disc = discid_new ();
    g_simple_async_result_set_op_res_gboolean (G_SIMPLE_ASYNC_RESULT (res),
                                               FALSE);

    if (!discid_read_sparse (disc, discid_get_default_device (), 0))
    {
        /* Error reading disc */
        g_set_error (&error, ET_MB5_SEARCH_ERROR,
                     ET_MB5_SEARCH_ERROR_DISCID,
                     discid_get_error_msg (disc));
        g_simple_async_report_gerror_in_idle (NULL,
                                              mb5_search_error_callback,
                                              NULL, error);
        return;
    }

    discid = discid_get_id (disc);

    if (g_cancellable_is_cancelled (cancellable))
    {
        g_set_error (&error, ET_MB5_SEARCH_ERROR,
                     ET_MB5_SEARCH_ERROR_CANCELLED,
                     _("Operation cancelled by user"));
        g_simple_async_report_gerror_in_idle (NULL,
                                              mb5_search_error_callback,
                                              NULL, error);
        discid_free (disc);
        return;
    }

    if (!et_musicbrainz_search (discid, MB_ENTITY_KIND_DISCID, mb_dialog_priv->mb_tree_root,
                                &error, cancellable))
    {
        g_simple_async_report_gerror_in_idle (NULL,
                                              mb5_search_error_callback,
                                              NULL, error);
        discid_free (disc);
        return;
    }

    et_mb_entity_view_set_tree_root (ET_MB_ENTITY_VIEW (entityView),
                                     mb_dialog_priv->mb_tree_root);
    discid_free (disc);
    g_simple_async_result_set_op_res_gboolean (G_SIMPLE_ASYNC_RESULT (res),
                                               TRUE);
}

static void
btn_discid_search (GtkWidget *button, gpointer data)
{
    mb5_search_cancellable = g_cancellable_new ();
    gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder, "statusbar")),
                        0, _("Starting MusicBrainz Search"));
    mb_dialog_priv->async_result = g_simple_async_result_new (NULL,
                                                              discid_search_callback,
                                                              NULL,
                                                              btn_manual_find_clicked);
    g_simple_async_result_run_in_thread (mb_dialog_priv->async_result,
                                         discid_search_thread_func, 0,
                                         mb5_search_cancellable);
    et_music_brainz_dialog_stop_set_sensitive (TRUE);
}

static void
btn_close_clicked (GtkWidget *button, gpointer data)
{
    gtk_dialog_response (GTK_DIALOG (mbDialog), GTK_RESPONSE_DELETE_EVENT);
}

#if 0
static void
btn_automatic_search_clicked (GtkWidget *button, gpointer data)
{
    GtkListStore *tree_model;
    GtkTreeSelection *selection;
    int count;
    GList *iter_list;
    GList *l;
    GHashTable *hash_table;
    SelectedFindThreadData *thread_data;
    int total_id;
    int num_tracks;
    guint total_frames = 150;
    guint disc_length  = 2;
    gchar *query_string;
    gchar *cddb_discid;

    iter_list = NULL;
    tree_model = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(BrowserList)));
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(BrowserList));
    count = gtk_tree_selection_count_selected_rows(selection);

    if (count > 0)
    {
        GList* list_sel_rows;

        list_sel_rows = gtk_tree_selection_get_selected_rows(selection, NULL);

        for (l = list_sel_rows; l != NULL; l = g_list_next (l))
        {
            GtkTreeIter iter;

            if (gtk_tree_model_get_iter(GTK_TREE_MODEL(tree_model),
                                        &iter,
                                        (GtkTreePath *) l->data))
            {
                iter_list = g_list_prepend (iter_list,
                                            gtk_tree_iter_copy (&iter));
            }
        }

        g_list_free_full (list_sel_rows,
                          (GDestroyNotify)gtk_tree_path_free);

    }
    else /* No rows selected, use the whole list */
    {
        GtkTreeIter current_iter;

        if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tree_model),
                                           &current_iter))
        {
            /* No row is present, return */
            gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder, "statusbar")),
                                0, _("No file selected"));
            return;
        }

        do
        {
            iter_list = g_list_prepend (iter_list,
                                        gtk_tree_iter_copy (&current_iter));
        }
        while (gtk_tree_model_iter_next(GTK_TREE_MODEL(tree_model),
               &current_iter));

        count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(tree_model),
                                               NULL);
    }

    if (count == 0)
    {
        gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder, "statusbar")),
                            0, _("No file selected"));
        return;
    }

    else if (count > 99)
    {
        gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder, "statusbar")),
                            0, _("More than 99 files selected. Cannot send the request."));
        return;
    }

    else
    {
        gchar *msg;
        msg = g_strdup_printf (ngettext (_("One file selected"),
                                         _("%d files selected"), count), count);
        gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder, "statusbar")),
                            0, msg);
        g_free (msg);
    }

    /* Compute Discid from list */
    total_id = 0;
    num_tracks = count;

    for (l = iter_list; l != NULL; l = g_list_next (l))
    {
        ET_File *etfile;
        gulong secs = 0;

        etfile = Browser_List_Get_ETFile_From_Iter ((GtkTreeIter *)l->data);
        secs = etfile->ETFileInfo->duration;
        total_frames += secs * 75;
        disc_length  += secs;
        while (secs > 0)
        {
            total_id = total_id + (secs % 10);
            secs = secs / 10;
        }
    }

    cddb_discid = g_strdup_printf("%08x",(guint)(((total_id % 0xFF) << 24) |
                                         (disc_length << 8) | num_tracks));
}
#endif

void
et_music_brainz_dialog_stop_set_sensitive (gboolean sensitive)
{
    gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btnStop")),
                              sensitive);
}

static void
et_set_file_tag (ET_File *et_file, gchar *title, gchar *artist,
                 gchar *album, gchar *album_artist, gchar *date,
                 gchar *country)
{
    File_Tag *file_tag;

    file_tag = ET_File_Tag_Item_New();
    ET_Copy_File_Tag_Item (et_file, file_tag);
    ET_Set_Field_File_Tag_Item (&file_tag->title, title);
    ET_Set_Field_File_Tag_Item (&file_tag->artist, artist);
    ET_Set_Field_File_Tag_Item (&file_tag->album, album);
    ET_Set_Field_File_Tag_Item (&file_tag->album_artist, album_artist);
    ET_Set_Field_File_Tag_Item (&file_tag->year, date);
    ET_Manage_Changes_Of_File_Data (et_file, NULL, file_tag);
    ET_Display_File_Data_To_UI (ETCore->ETFileDisplayed);
}

static void
btn_apply_changes_clicked (GtkWidget *widget, gpointer data)
{
    if (mb_dialog_priv->search->type == ET_MB_SEARCH_TYPE_MANUAL)
    {
        ET_File *et_file;
        EtMbEntity *et_entity;

        et_entity = et_mb_entity_view_get_selected_entity (ET_MB_ENTITY_VIEW (entityView));

        if (!et_entity || et_entity->type != MB_ENTITY_KIND_TRACK)
        {
            gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder, "statusbar")),
                                0, _("Please select a Track."));
            return;
        }

        get_first_selected_file (&et_file);

        if (et_file)
        {
            int size;
            Mb5ReleaseList *release_list;
            GtkListStore *tag_choice_list_store;
            GtkWidget *tag_choice_dialog;
            GtkWidget *tag_choice_tree_view;
            GtkCellRenderer *renderer;
            GtkTreeViewColumn *column;
            Mb5ArtistCredit artist_credit;
            int i;
            gchar title [NAME_MAX_SIZE];
            gchar *album_artist;
            gchar album[NAME_MAX_SIZE];
            gchar date[NAME_MAX_SIZE];
            gchar country[NAME_MAX_SIZE];

            GString *artist;
            GtkTreeIter iter;
            GtkTreeSelection *selection;

            release_list = mb5_recording_get_releaselist (et_entity->entity);
            artist_credit = mb5_recording_get_artistcredit (et_entity->entity);
            artist = g_string_new ("");

            if (artist_credit)
            {
                Mb5NameCreditList name_list;

                name_list = mb5_artistcredit_get_namecreditlist (artist_credit);

                for (i = 0; i < mb5_namecredit_list_size (name_list); i++)
                {
                    Mb5NameCredit name_credit;
                    Mb5Artist name_credit_artist;

                    name_credit = mb5_namecredit_list_item (name_list, i);
                    name_credit_artist = mb5_namecredit_get_artist (name_credit);
                    size = mb5_artist_get_name (name_credit_artist, title,
                                                sizeof (title));
                    g_string_append_len (artist, title, size);

                    if (i + 1 < mb5_namecredit_list_size (name_list))
                    {
                        g_string_append_len (artist, ", ", 2);
                    }
                }
            }

            size = mb5_recording_get_title (et_entity->entity, title,
                                            sizeof (title));
            title [size] = '\0';
            size = mb5_release_list_size (release_list);

            if (size > 1)
            {
                /* More than one releases show Dialog and let user decide */
                tag_choice_dialog = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                        "tag_choice_dialog"));
                tag_choice_tree_view = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                           "tag_choice_treeview"));
                tag_choice_list_store = gtk_list_store_new (TAG_CHOICE_COLS_N,
                                                            G_TYPE_STRING,
                                                            G_TYPE_STRING,
                                                            G_TYPE_STRING,
                                                            G_TYPE_STRING,
                                                            G_TYPE_STRING,
                                                            G_TYPE_STRING);
                renderer = gtk_cell_renderer_text_new ();
                column = gtk_tree_view_column_new_with_attributes ("Title",
                                                           renderer, "text",
                                                           TAG_CHOICE_TITLE,
                                                           NULL);
                gtk_tree_view_append_column (GTK_TREE_VIEW (tag_choice_tree_view),
                                             column);

                renderer = gtk_cell_renderer_text_new ();
                column = gtk_tree_view_column_new_with_attributes ("Album",
                                                           renderer, "text",
                                                           TAG_CHOICE_ALBUM,
                                                           NULL);
                gtk_tree_view_append_column (GTK_TREE_VIEW (tag_choice_tree_view),
                                             column);

                renderer = gtk_cell_renderer_text_new ();
                column = gtk_tree_view_column_new_with_attributes ("Artist",
                                                           renderer, "text",
                                                           TAG_CHOICE_ARTIST,
                                                           NULL);
                gtk_tree_view_append_column (GTK_TREE_VIEW (tag_choice_tree_view),
                                             column);

                renderer = gtk_cell_renderer_text_new ();
                column = gtk_tree_view_column_new_with_attributes ("Album Artist",
                                                           renderer, "text",
                                                           TAG_CHOICE_ALBUM_ARTIST,
                                                           NULL);
                gtk_tree_view_append_column (GTK_TREE_VIEW (tag_choice_tree_view),
                                             column);

                renderer = gtk_cell_renderer_text_new ();
                column = gtk_tree_view_column_new_with_attributes ("Date",
                                                           renderer, "text",
                                                           TAG_CHOICE_DATE,
                                                           NULL);
                gtk_tree_view_append_column (GTK_TREE_VIEW (tag_choice_tree_view),
                                             column);

                renderer = gtk_cell_renderer_text_new ();
                column = gtk_tree_view_column_new_with_attributes ("Country",
                                                           renderer, "text",
                                                           TAG_CHOICE_COUNTRY,
                                                           NULL);
                gtk_tree_view_append_column (GTK_TREE_VIEW (tag_choice_tree_view),
                                             column);
                gtk_tree_view_set_model (GTK_TREE_VIEW (tag_choice_tree_view),
                                         GTK_TREE_MODEL (tag_choice_list_store));

                for (size--; size >= 0; size--)
                {
                    Mb5Release release;

                    release = mb5_release_list_item (release_list, size);
                    mb5_release_get_title (release, album, sizeof (album));
                    album_artist = et_mb5_release_get_artists_names (release);
                    mb5_release_get_date (release, date, sizeof (date));
                    mb5_release_get_country (release, country, sizeof (country));

                    gtk_list_store_insert_with_values (GTK_LIST_STORE (tag_choice_list_store),
                                                       &iter, -1, TAG_CHOICE_TITLE, title,
                                                       TAG_CHOICE_ALBUM, album,
                                                       TAG_CHOICE_ARTIST, artist->str,
                                                       TAG_CHOICE_ALBUM_ARTIST, album_artist,
                                                       TAG_CHOICE_DATE, date,
                                                       TAG_CHOICE_COUNTRY, country, -1);
                    g_free (album_artist);
                }

                gtk_widget_set_size_request (tag_choice_dialog, 600, 200);
                gtk_widget_show_all (tag_choice_dialog);

                if (gtk_dialog_run (GTK_DIALOG (tag_choice_dialog)) == 0)
                {
                    g_string_free (artist, TRUE);
                    gtk_widget_destroy (tag_choice_dialog);

                    return;
                }

                selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tag_choice_tree_view));
                if (gtk_tree_selection_get_selected (selection,
                                                     (GtkTreeModel **)&tag_choice_list_store,
                                                     &iter))
                {
                    gchar *ret_album;
                    gchar *ret_album_artist;
                    gchar *ret_date;
                    gchar *ret_country;

                    gtk_tree_model_get (GTK_TREE_MODEL (tag_choice_list_store),
                                        &iter, TAG_CHOICE_ALBUM, &ret_album,
                                        TAG_CHOICE_ALBUM_ARTIST,&ret_album_artist,
                                        TAG_CHOICE_DATE, &ret_date,
                                        TAG_CHOICE_COUNTRY, &ret_country,
                                        -1);
                    et_set_file_tag (et_file, title, artist->str,
                                     ret_album, ret_album_artist,
                                     ret_date, ret_country);
                    g_free (ret_album);
                    g_free (ret_album_artist);
                    g_free (ret_date);
                    g_free (ret_country);
                }

                g_string_free (artist, TRUE);
                gtk_widget_destroy (tag_choice_dialog);

                return;
            }

            mb5_release_get_title (mb5_release_list_item (release_list, 0),
                                   album, sizeof (album));
            album_artist = et_mb5_release_get_artists_names (mb5_release_list_item (release_list, 0));
            mb5_release_get_date (mb5_release_list_item (release_list, 0),
                                  date, sizeof (date));
            mb5_release_get_country (mb5_release_list_item (release_list, 0),
                                     country, sizeof (country));
            et_set_file_tag (et_file, title, artist->str,
                             album, album_artist,
                             date, country);
            g_free (album_artist);
            g_string_free (artist, TRUE);
        }
    }
}

void
et_music_brainz_dialog_destroy (GtkWidget *widget)
{
    gtk_widget_destroy (widget);
    g_object_unref (G_OBJECT (builder));
    free_mb_tree (&mb_dialog_priv->mb_tree_root);
    g_free (mb_dialog_priv);
    mb_dialog_priv = NULL;
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
    GtkWidget *cb_search;
    GError *error;
    ET_File *et_file;

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

    mb_dialog_priv = g_malloc (sizeof (MusicBrainzDialogPrivate));
    mb_dialog_priv->mb_tree_root = g_node_new (NULL);
    mb_dialog_priv->search = NULL;
    exit_on_complete = FALSE;
    entityView = et_mb_entity_view_new ();
    mbDialog = GTK_WIDGET (gtk_builder_get_object (builder, "mbDialog"));
    gtk_widget_set_size_request (mbDialog, 660, 500);
    gtk_box_pack_start (GTK_BOX (gtk_builder_get_object (builder, "centralBox")),
                        entityView, TRUE, TRUE, 2);

    cb_search = GTK_WIDGET (gtk_builder_get_object (builder, "cbManualSearch"));
    g_signal_connect (gtk_bin_get_child (GTK_BIN (cb_search)), "activate",
                      G_CALLBACK (btn_manual_find_clicked), NULL);
    g_signal_connect (gtk_builder_get_object (builder, "btnManualFind"),
                      "clicked", G_CALLBACK (btn_manual_find_clicked),
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
    g_signal_connect (gtk_builder_get_object (builder, "toolbtnRefresh"),
                      "clicked", G_CALLBACK (tool_btn_refresh_clicked),
                      NULL);
    g_signal_connect (gtk_builder_get_object (builder, "btnSelectedFind"),
                      "clicked", G_CALLBACK (bt_selected_find_clicked),
                      NULL);
    g_signal_connect (gtk_builder_get_object (builder, "btnDiscFind"),
                      "clicked", G_CALLBACK (btn_discid_search),
                      NULL);
    g_signal_connect (gtk_builder_get_object (builder, "btnStop"),
                      "clicked", G_CALLBACK (btn_manual_stop_clicked),
                      NULL);
    g_signal_connect (gtk_builder_get_object (builder, "btnClose"),
                      "clicked", G_CALLBACK (btn_close_clicked),
                      NULL);
    g_signal_connect (gtk_builder_get_object (builder, "btnApplyChanges"),
                      "clicked", G_CALLBACK (btn_apply_changes_clicked),
                      NULL);
 
    //g_signal_connect (gtk_builder_get_object (builder, "btnAutomaticSearch"),
    //                  "clicked", G_CALLBACK (btn_automatic_search_clicked),
    //                  NULL);
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

    gtk_combo_box_set_active (GTK_COMBO_BOX (cb_manual_search_in), 1);

    /* Set the text of cbManualSearch to the album of selected file */
    get_first_selected_file (&et_file);

    if (et_file && ((File_Tag *)et_file->FileTag->data)->album)
    {
        gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (cb_search))),
                            ((File_Tag *)et_file->FileTag->data)->album);
    }

    gtk_widget_show_all (mbDialog);
    gtk_dialog_run (GTK_DIALOG (mbDialog));

    if (gtk_widget_get_sensitive (GTK_WIDGET (gtk_builder_get_object (builder,
                                  "btnStop"))))
    {
        exit_on_complete = TRUE;
        btn_manual_stop_clicked (NULL, NULL);
        gtk_widget_hide (mbDialog);
    }
    else
    {
        et_music_brainz_dialog_destroy (mbDialog);
    }
}
#endif /* ENABLE_libmusicbrainz */