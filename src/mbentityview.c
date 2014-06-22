/* mbentityview.c - 2014/05/05 */
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

#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>

#include "gtk2_compat.h"
#include "easytag.h"
#include "mbentityview.h"
#include "log.h"
#include "musicbrainz_dialog.h"
#include "mb_search.h"

#define ET_MB_ENTITY_VIEW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                            ET_MB_ENTITY_VIEW_TYPE, \
                                            EtMbEntityViewPrivate))

G_DEFINE_TYPE (EtMbEntityView, et_mb_entity_view, GTK_TYPE_BOX)

/***************
 * Declaration *
 ***************/

char *columns [MB_ENTITY_TYPE_COUNT][8] = {
    {"Name", "Gender", "Type"},
    {"Name", "Artist", "Type"},
    {"Name", "Album", "Artist", "Time"},
    };

/*
 * ET_MB_DISPLAY_RESULTS:
 * @ET_MB_DISPLAY_RESULTS_ALL: Display all results.
 * @ET_MB_DISPLAY_RESULTS_RED: Display Red Lines
 * @ET_MB_DISPLAY_RESULTS_SEARCH: Display Search Results
 */
enum ET_MB_DISPLAY_RESULTS
{
    ET_MB_DISPLAY_RESULTS_ALL = 0,
    ET_MB_DISPLAY_RESULTS_RED = 1,
    ET_MB_DISPLAY_RESULTS_SEARCH = 1 << 1,
};

static GSimpleAsyncResult *async_result;

/*
 * EtMbEntityViewPrivate:
 * @bread_crumb_box: GtkBox which represents the BreadCrumbWidget
 * @bread_crumb_nodes: Array of GNode being displayed by the GtkToggleButton
 * @tree_view: GtkTreeView to display the recieved music brainz data
 * @list_store: GtkTreeStore for treeView
 * @scrolled_window: GtkScrolledWindow for treeView
 * @mb_tree_root: Root Node of the Mb5Entity Tree
 * @mb_tree_current_node: Current node being displayed by EtMbEntityView
 * @active_toggle_button: Current active GtkToggleToolButton
 * @filter: GtkTreeModelFilter to filter rows based on the conditions
 * @search_or_red: Toggle Red Lines or Search in results
 * @toggle_red_lines: Display Red Lines or not
 * @text_to_search_in_results: Text to search in results
 *
 * Private data for EtMbEntityView.
 */
typedef struct
{
    GtkWidget *bread_crumb_box;
    GNode *bread_crumb_nodes[MB_ENTITY_TYPE_COUNT];
    GtkWidget *tree_view;
    GtkTreeModel *list_store;
    GtkWidget *scrolled_window;
    GNode *mb_tree_root;
    GNode *mb_tree_current_node;
    GtkWidget *active_toggle_button;
    GtkTreeModel *filter;
    int search_or_red;
    gboolean toggle_red_lines;
    const gchar *text_to_search_in_results;
    GtkTreeViewColumn *color_column;
} EtMbEntityViewPrivate;

/*
 * SearchInLevelThreadData:
 * @entity_view: EtMbEntityView
 * @child: GNode for which to retrieve children
 * @iter: Iter to the row of GNode
 *
 * Used for passing to Thread Function of Search In Levels.
 */
typedef struct
{
    EtMbEntityView *entity_view;
    GNode *child;
    GtkTreeIter iter;
} SearchInLevelThreadData;

/**************
 * Prototypes *
 **************/

static void
et_mb_entity_view_class_init (EtMbEntityViewClass *klass);
static void
et_mb_entity_view_init (EtMbEntityView *proj_notebook);
static GtkWidget *
insert_togglebtn_in_breadcrumb (GtkBox *breadCrumb);
static void
add_iter_to_list_store (GtkListStore *list_store, GNode *node);
static void
show_data_in_entity_view (EtMbEntityView *entity_view);
static void
toggle_button_clicked (GtkWidget *btn, gpointer user_data);
static void
tree_view_row_activated (GtkTreeView *tree_view, GtkTreePath *path,
                         GtkTreeViewColumn *column, gpointer user_data);
static gboolean
tree_filter_visible_func (GtkTreeModel *model, GtkTreeIter *iter,
                          gpointer data);
static void
et_mb_entity_view_destroy (GtkWidget *object);

/*************
 * Functions *
 *************/

/*
 * tree_filter_visible_func:
 * @model: GtkTreeModel
 * @iter: GtkTreeIter of current tree row
 * @data: user data passed
 *
 * Filter function of GtkTreeModelFilter
 *
 * Returns: TRUE if row should be displayed or FALSE if it shouldn't.
 */
static gboolean
tree_filter_visible_func (GtkTreeModel *model, GtkTreeIter *iter,
                          gpointer data)
{
    EtMbEntityViewPrivate *priv;

    priv = (EtMbEntityViewPrivate *)data;

    if (priv->search_or_red == ET_MB_DISPLAY_RESULTS_ALL)
    {
        return TRUE;
    }

    if (priv->search_or_red & ET_MB_DISPLAY_RESULTS_SEARCH)
    {
        gchar *value;

        gtk_tree_model_get (model, iter, 0, &value, -1);

        if (g_strstr_len (value, -1, priv->text_to_search_in_results))
        {
            g_free (value);
            return TRUE;
        }

        g_free (value);
    }
 
    if (priv->search_or_red & ET_MB_DISPLAY_RESULTS_RED)
    {
        gchar *value;

        if (gtk_tree_model_get_n_columns (model) == MB_ARTIST_COLUMNS_N + 1)
        {
            gtk_tree_model_get (model, iter, MB_ARTIST_COLUMNS_N, &value, -1);
        }
        else if (gtk_tree_model_get_n_columns (model) ==
                 MB_ALBUM_COLUMNS_N + 1)
        {
            gtk_tree_model_get (model, iter, MB_ALBUM_COLUMNS_N, &value, -1);
        }
        else if (gtk_tree_model_get_n_columns (model) ==
                 MB_TRACK_COLUMNS_N + 1)
        {
            gtk_tree_model_get (model, iter, MB_TRACK_COLUMNS_N, &value, -1);
        }
        else
        {
            return TRUE;
        }

        if (g_strcmp0 (value, "black") == 0)
        {
            g_free (value);
            return TRUE;
        }

        g_free (value);

        return priv->toggle_red_lines;
    }

    return FALSE;
}

/*
 * et_mb_entity_view_class_init:
 * @klass: EtMbEntityViewClass to initialize.
 *
 * Initializes an EtMbEntityViewClass class.
 */
static void
et_mb_entity_view_class_init (EtMbEntityViewClass *klass)
{
    GtkWidgetClass *widget_class;

    g_type_class_add_private (klass, sizeof (EtMbEntityViewPrivate));
    widget_class = GTK_WIDGET_CLASS (klass);
    widget_class->destroy = et_mb_entity_view_destroy;
}

/*
 * insert_togglebtn_in_toolbar:
 * @toolbar: GtkBox in which GtkToggleToolButton will be inserted
 *
 * Returns: GtkWidget inserted in GtkBox.
 * Insert a GtkToggleButton in GtkBox.
 */
static GtkWidget *
insert_togglebtn_in_breadcrumb (GtkBox *breadCrumb)
{
    GtkWidget *btn;
    btn = gtk_toggle_button_new ();
    gtk_box_pack_start (breadCrumb, btn, FALSE, FALSE, 2);
    return btn;
}

/*
 * add_iter_to_list_store:
 * @list_store: GtkListStore to add GtkTreeIter in.
 * @node: GNode which has information to add in GtkListStore.
 *
 * Traverses GNode and its next nodes, to add them in GtkListStore.
 */
static void
add_iter_to_list_store (GtkListStore *list_store, GNode *node)
{
    /* Traverse node in GNode and add it to list_store */
    enum MB_ENTITY_TYPE type;
    Mb5ArtistCredit artist_credit;
    Mb5NameCreditList name_list;
    Mb5ReleaseGroup release_group;
    Mb5ReleaseList release_list;
    int i;
    int minutes;
    int seconds;
    GString *gstring;
    GtkTreeIter iter;
    gchar name [NAME_MAX_SIZE];

    type = ((EtMbEntity *)node->data)->type;
    while (node)
    {
        Mb5Entity entity;

        entity = ((EtMbEntity *)node->data)->entity;

        switch (type)
        {
            case MB_ENTITY_TYPE_ARTIST:
                mb5_artist_get_name ((Mb5Artist)entity, name, sizeof (name));
                gtk_list_store_insert_with_values (list_store, &iter, -1,
                                        MB_ARTIST_COLUMNS_N, "black", -1);
                gtk_list_store_set (list_store, &iter,
                                    MB_ARTIST_COLUMNS_NAME, name, -1);

                mb5_artist_get_gender ((Mb5Artist)entity, name,
                                       sizeof (name));
                gtk_list_store_set (list_store, &iter,
                                    MB_ARTIST_COLUMNS_GENDER, name, -1);

                mb5_artist_get_type ((Mb5Artist)entity, name, sizeof (name));
                gtk_list_store_set (list_store, &iter,
                                    MB_ARTIST_COLUMNS_TYPE,
                                    name, -1);

                if (((EtMbEntity *)node->data)->is_red_line)
                {
                    gtk_list_store_set (list_store, &iter,
                                        MB_ARTIST_COLUMNS_N, "red", -1);
                }
                else
                {
                    gtk_list_store_set (list_store, &iter,
                                        MB_ARTIST_COLUMNS_N, "black", -1);
                }

                break;

            case MB_ENTITY_TYPE_ALBUM:
                mb5_release_get_title ((Mb5Release)entity, name,
                                       sizeof (name));
                gtk_list_store_insert_with_values (list_store, &iter, -1,
                                        MB_ALBUM_COLUMNS_N, "black", -1);
                gtk_list_store_set (list_store, &iter,
                                    MB_ALBUM_COLUMNS_NAME, name, -1);

                artist_credit = mb5_release_get_artistcredit ((Mb5Release)entity);
                if (artist_credit)
                {
                    name_list = mb5_artistcredit_get_namecreditlist (artist_credit);
                    gstring = g_string_new ("");

                    for (i = 0; i < mb5_namecredit_list_size (name_list); i++)
                    {
                        Mb5NameCredit name_credit;
                        Mb5Artist name_credit_artist;
                        int size;

                        name_credit = mb5_namecredit_list_item (name_list, i);
                        name_credit_artist = mb5_namecredit_get_artist (name_credit);
                        size = mb5_artist_get_name (name_credit_artist, name,
                                                    sizeof (name));
                        g_string_append_len (gstring, name, size);
                        g_string_append_c (gstring, ' ');
                    }

                    gtk_list_store_set (list_store, &iter,
                                        MB_ALBUM_COLUMNS_ARTIST,
                                        gstring->str, -1);
                    g_string_free (gstring, TRUE);
                }

                if (((EtMbEntity *)node->data)->is_red_line)
                {
                    gtk_list_store_set (list_store, &iter,
                                        MB_ALBUM_COLUMNS_N, "red", -1);
                }
                else
                {
                    gtk_list_store_set (list_store, &iter,
                                        MB_ALBUM_COLUMNS_N, "black", -1);
                }

                release_group = mb5_release_get_releasegroup ((Mb5Release)entity);
                mb5_releasegroup_get_primarytype (release_group, name,
                                                  sizeof (name));
                gtk_list_store_set (list_store, &iter,
                                    MB_ALBUM_COLUMNS_TYPE, name, -1);
                break;

            case MB_ENTITY_TYPE_TRACK:
                mb5_recording_get_title ((Mb5Recording)entity, name,
                                         sizeof (name));
                gtk_list_store_append (list_store, &iter);
                gtk_list_store_set (list_store, &iter,
                                    MB_TRACK_COLUMNS_NAME, name, -1);

                artist_credit = mb5_recording_get_artistcredit ((Mb5Release)entity);
                if (artist_credit)
                {
                    name_list = mb5_artistcredit_get_namecreditlist (artist_credit);
                    gstring = g_string_new ("");

                    for (i = 0; i < mb5_namecredit_list_size (name_list); i++)
                    {
                        Mb5NameCredit name_credit;
                        Mb5Artist name_credit_artist;
                        int size;

                        name_credit = mb5_namecredit_list_item (name_list, i);
                        name_credit_artist = mb5_namecredit_get_artist (name_credit);
                        size = mb5_artist_get_name (name_credit_artist, name,
                                                    sizeof (name));
                        g_string_append_len (gstring, name, size);
                        g_string_append_c (gstring, ' ');
                    }

                    gtk_list_store_set (list_store, &iter,
                                        MB_TRACK_COLUMNS_ARTIST,
                                        gstring->str, -1);
                    g_string_free (gstring, TRUE);
                }

                release_list = mb5_recording_get_releaselist ((Mb5Recording)entity);

                if (release_list)
                {
                    gstring = g_string_new ("");

                    for (i = 0; i < mb5_release_list_size (release_list); i++)
                    {
                        Mb5Release release;
                        int size;

                        release = mb5_release_list_item (release_list, i);
                        size = mb5_release_get_title (release, name,
                                                      sizeof (name));
                        g_string_append_len (gstring, name, size);
                        g_string_append_c (gstring, ' ');
                    }

                    gtk_list_store_set (list_store, &iter,
                                        MB_TRACK_COLUMNS_ALBUM,
                                        gstring->str, -1);
                    g_string_free (gstring, TRUE);
                }

                minutes = mb5_recording_get_length ((Mb5Recording)entity)/60000;
                seconds = mb5_recording_get_length ((Mb5Recording)entity)%60000;
                i = g_snprintf (name, NAME_MAX_SIZE, "%d:%d", minutes,
                                seconds/1000);
                name [i] = '\0';
                gtk_list_store_set (list_store, &iter,
                                    MB_TRACK_COLUMNS_TIME,
                                    name, -1);
                break;

            case MB_ENTITY_TYPE_COUNT:
            case MB_ENTITY_TYPE_DISCID:
                break;
 
        }

        node = g_node_next_sibling (node);
    }
}

/*
 * show_data_in_entity_view:
 * @entity_view: EtMbEntityView to show data.
 *
 * Show retrieved MusicBrainz data in EtMbEntityView.
 */
static void
show_data_in_entity_view (EtMbEntityView *entity_view)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    EtMbEntityViewPrivate *priv;
    int i, total_cols, type;
    GList *list_cols, *list;
    GType *types;

    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);

    /* Remove the previous List Store */
    if (GTK_IS_LIST_STORE (priv->list_store))
    {
        gtk_list_store_clear (GTK_LIST_STORE (priv->list_store));
        g_object_unref (priv->list_store);
    }

    gtk_tree_view_set_model (GTK_TREE_VIEW (priv->tree_view), NULL);

    if (GTK_IS_TREE_MODEL_FILTER (priv->filter))
    {
        g_object_unref (priv->filter);
    }

    /* Remove all colums */
    list_cols = gtk_tree_view_get_columns (GTK_TREE_VIEW (priv->tree_view));
    for (list = g_list_first (list_cols); list != NULL;
         list = g_list_next (list))
    {
        gtk_tree_view_remove_column (GTK_TREE_VIEW (priv->tree_view),
                                     GTK_TREE_VIEW_COLUMN (list->data));
    }

    g_list_free (list_cols);

    /* Create new List Store and add it */
    type = ((EtMbEntity *)(g_node_first_child (priv->mb_tree_current_node)->data))->type;
    switch (type)
    {
        case MB_ENTITY_TYPE_ARTIST:
            total_cols = MB_ARTIST_COLUMNS_N;
            break;

        case MB_ENTITY_TYPE_ALBUM:
            total_cols = MB_ALBUM_COLUMNS_N;
            break;

        case MB_ENTITY_TYPE_TRACK:
            total_cols = MB_TRACK_COLUMNS_N;
            break;

        default:
            total_cols = 0;
    }

    types = g_malloc (sizeof (GType) * (total_cols + 1));

    for (i = 0; i < total_cols; i++)
    {
        types [i] = G_TYPE_STRING;
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (columns[type][i],
                                                           renderer, "text",
                                                           i, "foreground",
                                                           total_cols, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (priv->tree_view), column);
    }

    /* Setting the colour column */
    types [total_cols] = G_TYPE_STRING;
    priv->list_store = GTK_TREE_MODEL (gtk_list_store_newv (total_cols + 1,
                                                            types));
    priv->filter = GTK_TREE_MODEL (gtk_tree_model_filter_new (priv->list_store,
                                                              NULL));
    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (priv->filter),
                                            tree_filter_visible_func, priv,
                                            NULL);
    g_free (types);
    gtk_tree_view_set_model (GTK_TREE_VIEW (priv->tree_view), priv->filter);
    g_object_unref (priv->filter);
    add_iter_to_list_store (GTK_LIST_STORE (priv->list_store),
                            g_node_first_child (priv->mb_tree_current_node));
}

/*
 * toggle_button_clicked:
 * @btn: The GtkToggleButton clicked.
 * @user_data: User Data passed to the handler.
 *
 * The singal handler for GtkToggleButton's clicked signal.
 */
static void
toggle_button_clicked (GtkWidget *btn, gpointer user_data)
{
    EtMbEntityView *entity_view;
    EtMbEntityViewPrivate *priv;
    GList *children;

    entity_view = ET_MB_ENTITY_VIEW (user_data);
    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);

    if (btn == priv->active_toggle_button)
    {
        return;
    }

    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (btn)))
    {
        return;
    }

    if (priv->active_toggle_button)
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->active_toggle_button),
                                      FALSE);
    }

    children = gtk_container_get_children (GTK_CONTAINER (priv->bread_crumb_box));
    priv->mb_tree_current_node = priv->bread_crumb_nodes[g_list_index (children,
                                                                       btn)];
    priv->active_toggle_button = btn;
    show_data_in_entity_view (entity_view);
}

/*
 * manual_search_callback:
 * @source: Source Object
 * @res: GAsyncResult
 * @user_data: User data
 *
 * Callback function for GAsyncResult for Search In Levels.
 */
static void
search_in_levels_callback (GObject *source, GAsyncResult *res,
                           gpointer user_data)
{
    EtMbEntityView *entity_view;
    EtMbEntityViewPrivate *priv;
    GtkWidget *toggle_btn;
    gchar *entity_name;
    SearchInLevelThreadData *thread_data;
    GList *children;
    GList *active_child;

    if (res &&
        !g_simple_async_result_get_op_res_gboolean (G_SIMPLE_ASYNC_RESULT (res)))
    {
        g_object_unref (res);
        g_free (user_data);
        return;
    }

    thread_data = user_data;
    entity_view = thread_data->entity_view;
    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);

    gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder,
                        "statusbar")),
                        0, "Retrieving Completed");
    /* Check if child node has children or not */
    if (!g_node_first_child (thread_data->child))
    {
        return;
    }

    priv->mb_tree_current_node = thread_data->child;
    children = gtk_container_get_children (GTK_CONTAINER (priv->bread_crumb_box));
    active_child = g_list_find (children, priv->active_toggle_button);
    while ((active_child = g_list_next (active_child)))
    {
        gtk_container_remove (GTK_CONTAINER (priv->bread_crumb_box),
                              GTK_WIDGET (active_child->data));
    }

    toggle_btn = insert_togglebtn_in_breadcrumb (GTK_BOX (priv->bread_crumb_box));
    children = gtk_container_get_children (GTK_CONTAINER (priv->bread_crumb_box));
    priv->bread_crumb_nodes [g_list_length (children) - 1] = thread_data->child;

    if (priv->active_toggle_button)
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->active_toggle_button),
                                      FALSE);
    }

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle_btn), TRUE);
    g_signal_connect (G_OBJECT (toggle_btn), "clicked",
                      G_CALLBACK (toggle_button_clicked), entity_view);
    priv->active_toggle_button = toggle_btn;

    gtk_tree_model_get (priv->list_store, &thread_data->iter, 0, &entity_name,
                        -1);
    gtk_button_set_label (GTK_BUTTON (toggle_btn), entity_name);
    gtk_widget_show_all (GTK_WIDGET (priv->bread_crumb_box));
    ((EtMbEntity *)thread_data->child->data)->is_red_line = TRUE;
    show_data_in_entity_view (entity_view);

    if (res)
    {
        g_object_unref (res);
    }

    g_free (thread_data);
}

/*
 * manual_search_callback:
 * @res: GAsyncResult
 * @obj: Source Object
 * @cancellable: User data
 *
 * Search in Levels Thread Function
 */
static void
search_in_levels_thread_func (GSimpleAsyncResult *res, GObject *obj,
                              GCancellable *cancellable)
{
    SearchInLevelThreadData *thread_data;
    gchar mbid [NAME_MAX_SIZE];
    GError *error;
    gchar *status_msg;
    gchar parent_entity_str [NAME_MAX_SIZE];
    gchar *child_entity_type_str;

    child_entity_type_str = NULL;
    thread_data = g_async_result_get_user_data (G_ASYNC_RESULT (res));
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

    if (((EtMbEntity *)thread_data->child->data)->type ==
        MB_ENTITY_TYPE_TRACK)
    {
        return;
    }
    else if (((EtMbEntity *)thread_data->child->data)->type ==
             MB_ENTITY_TYPE_ARTIST)
    {
        child_entity_type_str = g_strdup ("Albums ");
        mb5_artist_get_id (((EtMbEntity *)thread_data->child->data)->entity,
                           mbid, sizeof (mbid));
        mb5_artist_get_name (((EtMbEntity *)thread_data->child->data)->entity,
                             parent_entity_str, sizeof (parent_entity_str));
    }
    else if (((EtMbEntity *)thread_data->child->data)->type ==
             MB_ENTITY_TYPE_ALBUM)
    {
        child_entity_type_str = g_strdup ("Tracks ");
        mb5_release_get_id (((EtMbEntity *)thread_data->child->data)->entity,
                            mbid, sizeof (mbid));
        mb5_release_get_title (((EtMbEntity *)thread_data->child->data)->entity,
                               parent_entity_str, sizeof (parent_entity_str));
    }

    error = NULL;
    status_msg = g_strconcat (_("Retrieving "), child_entity_type_str, "for ",
                              parent_entity_str, NULL);
    et_show_status_msg_in_idle (status_msg);
    g_free (status_msg);
    g_free (child_entity_type_str);

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

    if (!et_musicbrainz_search_in_entity (((EtMbEntity *)thread_data->child->data)->type + 1,
                                          ((EtMbEntity *)thread_data->child->data)->type,
                                          mbid, thread_data->child, &error,
                                          cancellable))
    {
        g_simple_async_report_gerror_in_idle (NULL,
                                              mb5_search_error_callback,
                                              thread_data, error);
        return;
    }

    g_simple_async_result_set_op_res_gboolean (res, TRUE);
}

/*
 * tree_view_row_activated:
 * @tree_view: the object on which the signal is emitted
 * @path: the GtkTreePath for the activated row
 * @column: the GtkTreeViewColumn in which the activation occurred
 * @user_data: user data set when the signal handler was connected.
 *
 * Signal Handler for GtkTreeView "row-activated" signal.
 */
static void
tree_view_row_activated (GtkTreeView *tree_view, GtkTreePath *path,
                         GtkTreeViewColumn *column, gpointer user_data)
{
    SearchInLevelThreadData *thread_data;
    EtMbEntityView *entity_view;
    EtMbEntityViewPrivate *priv;
    GNode *child;
    int depth;
    GtkTreeIter filter_iter;
    GtkTreeIter iter;

    entity_view = ET_MB_ENTITY_VIEW (user_data);
    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);
    gtk_tree_model_get_iter (priv->filter, &filter_iter, path);
    gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (priv->filter),
                                                      &iter, &filter_iter);
    depth = 0;

    while (gtk_tree_model_iter_previous (priv->list_store, &iter))
    {
        depth++;
    }

    child = g_node_nth_child (priv->mb_tree_current_node,
                              depth);

    if (((EtMbEntity *)child->data)->type ==
        MB_ENTITY_TYPE_TRACK)
    {
        return;
    }

    thread_data = g_malloc (sizeof (SearchInLevelThreadData));
    thread_data->entity_view = ET_MB_ENTITY_VIEW (user_data);
    thread_data->child = child;
    gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (priv->filter),
                                                      &thread_data->iter,
                                                      &filter_iter);
    if (((EtMbEntity *)child->data)->is_red_line)
    {
        search_in_levels_callback (NULL, NULL, thread_data);
        return;
    }

    gtk_statusbar_push (GTK_STATUSBAR (gtk_builder_get_object (builder, "statusbar")),
                        0, _("Starting MusicBrainz Search"));
    async_result = g_simple_async_result_new (NULL,
                                              search_in_levels_callback,
                                              thread_data,
                                              tree_view_row_activated);
    mb5_search_cancellable = g_cancellable_new ();
    g_simple_async_result_run_in_thread (async_result,
                                         search_in_levels_thread_func,
                                         0, mb5_search_cancellable);
}

/*
 * et_mb_entity_view_init:
 * @entity_view: EtMbEntityView to initialize.
 *
 * Initializes an EtMbEntityView.
 */
static void
et_mb_entity_view_init (EtMbEntityView *entity_view)
{
    EtMbEntityViewPrivate *priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);

    gtk_orientable_set_orientation (GTK_ORIENTABLE (entity_view),
                                    GTK_ORIENTATION_VERTICAL);

    /* Adding child widgets */
    priv->bread_crumb_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
    priv->tree_view = gtk_tree_view_new ();
    priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (priv->scrolled_window),
                       priv->tree_view);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->scrolled_window),
                                    GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
    gtk_box_pack_start (GTK_BOX (entity_view), priv->bread_crumb_box,
                        FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (entity_view), priv->scrolled_window,
                        TRUE, TRUE, 2);
    priv->mb_tree_root = NULL;
    priv->mb_tree_current_node = NULL;
    priv->active_toggle_button = NULL;
    priv->toggle_red_lines = TRUE;
    priv->search_or_red = ET_MB_DISPLAY_RESULTS_ALL;

    g_signal_connect (G_OBJECT (priv->tree_view), "row-activated",
                      G_CALLBACK (tree_view_row_activated), entity_view);
}

/*
 * delete_bread_crumb_button:
 * @button: Button to delete.
 * @data: User data
 *
 * Callback function to delete a button from bread crumb.
 */
static void
delete_bread_crumb_button (GtkWidget *button, gpointer data)
{
    gtk_container_remove (GTK_CONTAINER (data), button);
}

/*
 * et_mb_entity_view_set_tree_root:
 * @entity_view: EtMbEntityView for which tree root to set.
 * @treeRoot: GNode to set as tree root.
 *
 * To set tree root of EtMbEntityView.
 */
void
et_mb_entity_view_set_tree_root (EtMbEntityView *entity_view, GNode *treeRoot)
{
    EtMbEntityViewPrivate *priv;
    GtkWidget *btn;
    GNode *child;

    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);
    priv->mb_tree_root = treeRoot;
    priv->mb_tree_current_node = treeRoot;
    gtk_container_foreach (GTK_CONTAINER (priv->bread_crumb_box),
                           delete_bread_crumb_button,
                           priv->bread_crumb_box);
    btn = insert_togglebtn_in_breadcrumb (GTK_BOX (priv->bread_crumb_box));
    child = g_node_first_child (treeRoot);

    if (child)
    {
        switch (((EtMbEntity *)child->data)->type)
        {
            case MB_ENTITY_TYPE_ARTIST:
                gtk_button_set_label (GTK_BUTTON (btn), _("Artists"));
                break;

            case MB_ENTITY_TYPE_ALBUM:
                gtk_button_set_label (GTK_BUTTON (btn), _("Albums"));
                break;

            case MB_ENTITY_TYPE_TRACK:
                gtk_button_set_label (GTK_BUTTON (btn), _("Tracks"));
                break;

            default:
                break;
        }

        priv->bread_crumb_nodes [0] = treeRoot;
        priv->active_toggle_button = btn;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (btn), TRUE);
        gtk_widget_show_all (priv->bread_crumb_box);
        show_data_in_entity_view (entity_view);
        g_signal_connect (G_OBJECT (btn), "clicked",
                          G_CALLBACK (toggle_button_clicked), entity_view);
        gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree_view)),
                                     GTK_SELECTION_MULTIPLE);
    }
}


/*
 * et_mb_entity_view_new:
 *
 * Creates a new EtMbEntityView.
 *
 * Returns: GtkWidget, a new EtMbEntityView.
 */
GtkWidget *
et_mb_entity_view_new ()
{
    return GTK_WIDGET (g_object_new (et_mb_entity_view_get_type (), NULL));
}

/*
 * et_mb_entity_view_set_tree_root:
 * @entity_view: EtMbEntityView
 *
 * To select all rows.
 */
void
et_mb_entity_view_select_all (EtMbEntityView *entity_view)
{
    EtMbEntityViewPrivate *priv;

    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);
    gtk_tree_selection_select_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree_view)));
}

/*
 * et_mb_entity_view_unselect_all:
 * @entity_view: EtMbEntityView
 *
 * To unselect all rows.
 */
void
et_mb_entity_view_unselect_all (EtMbEntityView *entity_view)
{
    EtMbEntityViewPrivate *priv;

    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);
    gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree_view)));
}

/*
 * et_mb_entity_view_toggle_red_lines:
 * @entity_view: EtMbEntityView
 *
 * To toggle the state of red lines being displayed.
 */
void
et_mb_entity_view_toggle_red_lines (EtMbEntityView *entity_view)
{
    EtMbEntityViewPrivate *priv;

    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);
    priv->search_or_red = priv->search_or_red | ET_MB_DISPLAY_RESULTS_RED;
    priv->toggle_red_lines = !priv->toggle_red_lines;
    gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter));
}

/*
 * et_mb_entity_view_invert_selection:
 * @entity_view: EtMbEntityView
 *
 * To select the unselected rows and unselect the selected rows.
 */
void
et_mb_entity_view_invert_selection (EtMbEntityView *entity_view)
{
    EtMbEntityViewPrivate *priv;
    GtkTreeIter filter_iter;
    GtkTreeSelection *selection;

    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree_view));
    if (!gtk_tree_model_get_iter_first (priv->filter, &filter_iter))
    {
        return;
    }

    do
    {
        if (!gtk_tree_selection_iter_is_selected (selection, &filter_iter))
        {
            gtk_tree_selection_select_iter (selection, &filter_iter);
        }
        else
        {
            gtk_tree_selection_unselect_iter (selection, &filter_iter);
        }
    }
    while (gtk_tree_model_iter_next (priv->filter, &filter_iter));
}

/*
 * et_mb_entity_view_get_current_level:
 * @entity_view: EtMbEntityView
 *
 * Get the current level at which search results are shown
 */
int
et_mb_entity_view_get_current_level (EtMbEntityView *entity_view)
{
    EtMbEntityViewPrivate *priv;
    GList *list;
    int n;

    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);
    list = gtk_container_get_children (GTK_CONTAINER (priv->bread_crumb_box));
    n = g_list_length (list);
    g_list_free (list);

    return n;
}

/*
 * et_mb_entity_view_search_in_results:
 * @entity_view: EtMbEntityView
 *
 * To search in the results obtained
 */
void
et_mb_entity_view_search_in_results (EtMbEntityView *entity_view,
                                     const gchar *text)
{
    EtMbEntityViewPrivate *priv;

    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);
    priv->text_to_search_in_results = text;
    priv->search_or_red = priv->search_or_red | ET_MB_DISPLAY_RESULTS_SEARCH;
    gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter));
}

/*
 * et_mb_entity_view_select_up:
 * @entity_view: EtMbEntityView
 *
 * To select the row above the current row.
 */
void
et_mb_entity_view_select_up (EtMbEntityView *entity_view)
{
    EtMbEntityViewPrivate *priv;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GList *selected_rows;

    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree_view));
    selected_rows = gtk_tree_selection_get_selected_rows (selection,
                                                          &priv->filter);
    gtk_tree_model_get_iter (priv->filter, &iter,
                             (g_list_first (selected_rows)->data));
    if (!gtk_tree_model_iter_next (priv->filter, &iter))
    {
        goto exit;
    }

    gtk_tree_selection_select_iter (selection, &iter);

    exit:
    g_list_free_full (selected_rows, (GDestroyNotify)gtk_tree_path_free);
}

/*
 * et_mb_entity_view_select_down:
 * @entity_view: EtMbEntityView
 *
 * To select the row below the current row.
 */
void
et_mb_entity_view_select_down (EtMbEntityView *entity_view)
{
    EtMbEntityViewPrivate *priv;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GList *selected_rows;

    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree_view));
    selected_rows = gtk_tree_selection_get_selected_rows (selection,
                                                          &priv->filter);
    gtk_tree_model_get_iter (priv->filter, &iter,
                             g_list_last (selected_rows)->data);
    if (!gtk_tree_model_iter_next (priv->filter, &iter))
    {
        goto exit;
    }

    gtk_tree_selection_select_iter (selection, &iter);

    exit:
    g_list_free_full (selected_rows, (GDestroyNotify)gtk_tree_path_free);
}

/*
 * et_mb_entity_view_refresh_current_level:
 * @entity_view: EtMbEntityView
 *
 * To re download data from MusicBrainz Server at the current level.
 */
void
et_mb_entity_view_refresh_current_level (EtMbEntityView *entity_view)
{
}

void
et_mb_entity_view_clear_all (EtMbEntityView *entity_view)
{
    EtMbEntityViewPrivate *priv;

    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);
    gtk_list_store_clear (GTK_LIST_STORE (priv->list_store));
}

/*
 * et_mb_entity_view_destroy:
 * @object: EtMbEntityView
 *
 * Overloaded destructor for EtMbEntityView.
 */
static void
et_mb_entity_view_destroy (GtkWidget *object)
{
    EtMbEntityView *entity_view;
    EtMbEntityViewPrivate *priv;

    g_return_if_fail (object != NULL);
    g_return_if_fail (IS_ET_MB_ENTITY_VIEW(object));

    entity_view = ET_MB_ENTITY_VIEW (object);
    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);

    if (GTK_IS_LIST_STORE (priv->list_store))
    {
        g_object_unref (priv->list_store);
    }

    if (GTK_WIDGET_CLASS (et_mb_entity_view_parent_class)->destroy)
    {
        (*GTK_WIDGET_CLASS (et_mb_entity_view_parent_class)->destroy)(object);
    }
}