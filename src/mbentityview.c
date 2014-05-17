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

#define NAME_MAX_SIZE 256
#define ET_MB_ENTITY_VIEW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                            ET_MB_ENTITY_VIEW_TYPE, \
                                            EtMbEntityViewPrivate))

/***************
 * Declaration *
 ***************/

char *columns [MB_ENTITY_TYPE_COUNT][8] = {
    {"Name", "Gender", "Type"},
    {"Name", "Artist", "Release", "Type"},
    {"Name", "Country", "Album", "Date", "Time", "Number"},
    };

static GSimpleAsyncResult *async_result;

/*
 * EtMbEntityViewPrivate:
 * @bread_crumb_box: GtkBox which represents the BreadCrumbWidget
 * @tree_view: GtkTreeView to display the recieved music brainz data
 * @bread_crumb_nodes: Array of GNode being displayed by the GtkToggleButton
 * @list_store: GtkTreeStore for treeView
 * @scrolled_window: GtkScrolledWindow for treeView
 * @mb_tree_root: Root Node of the Mb5Entity Tree
 * @mb_tree_current_node: Current node being displayed by EtMbEntityView
 * @active_toggle_button: Current active GtkToggleToolButton
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
} EtMbEntityViewPrivate;

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

/*************
 * Functions *
 *************/

/*
 * et_mb_entity_view_get_type
 *
 * Returns: A GType, type of EtMbEntityView.
 */
GType
et_mb_entity_view_get_type (void)
{
    /* TODO: Use G_DEFINE_TYPE */
    static GType et_mb_entity_view_type = 0;

    if (!et_mb_entity_view_type)
    {
        static const GTypeInfo et_mb_entity_view_type_info = 
        {
            sizeof (EtMbEntityViewClass),
            NULL,
            NULL,
            (GClassInitFunc) et_mb_entity_view_class_init,
            NULL,
            NULL,
            sizeof (EtMbEntityView),
            0,
            (GInstanceInitFunc) et_mb_entity_view_init,
        };
        et_mb_entity_view_type = g_type_register_static (GTK_TYPE_BOX, 
                                                         "EtMbEntityView",
                                                         &et_mb_entity_view_type_info,
                                                         0);
    }
    return et_mb_entity_view_type;
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
    g_type_class_add_private (klass, sizeof (EtMbEntityViewPrivate));
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
    //Mb5ReleaseList *release_list;
    Mb5ArtistCredit artist_credit;
    Mb5NameCreditList name_list;
    int i;
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
            /* Following code may depend on the search code */
            case MB_ENTITY_TYPE_ARTIST:
                mb5_artist_get_name ((Mb5Artist)entity, name, sizeof (name));
                gtk_list_store_append (list_store, &iter);
                gtk_list_store_set (list_store, &iter, 
                                    MB_ARTIST_COLUMNS_NAME, name, -1);

                mb5_artist_get_gender ((Mb5Artist)entity, name, sizeof (name));
                gtk_list_store_set (list_store, &iter,
                                    MB_ARTIST_COLUMNS_GENDER, name, -1);

                mb5_artist_get_type ((Mb5Artist)entity, name, sizeof (name));
                gtk_list_store_set (list_store, &iter,
                                    MB_ARTIST_COLUMNS_TYPE,
                                    name, -1);
                break;

            case MB_ENTITY_TYPE_ALBUM:
                mb5_release_get_title ((Mb5Release)entity, name, sizeof (name));
                gtk_list_store_append (list_store, &iter);
                gtk_list_store_set (list_store, &iter, 
                                    MB_ALBUM_COLUMNS_NAME, name, -1);

                artist_credit = mb5_release_get_artistcredit ((Mb5Release)entity);
                name_list = mb5_artistcredit_get_namecreditlist (artist_credit);
                gstring = g_string_new ("");

                for (i = 0; i < mb5_namecredit_list_get_count (name_list); i++)
                {
                    Mb5NameCredit name_credit;
                    int size;
                    name_credit = mb5_namecredit_list_item (name_list, i);
                    size = mb5_namecredit_get_name (name_credit, name, 
                                                    NAME_MAX_SIZE);
                    g_string_append_len (gstring, name, size);
                    g_string_append_c (gstring, ' ');
                    mb5_namecredit_delete (name_credit);
                }

                mb5_namecredit_list_delete (name_list);
                mb5_artistcredit_delete (artist_credit);

                gtk_list_store_set (list_store, &iter,
                                    MB_ALBUM_COLUMNS_ARTIST, gstring->str, -1);
                g_string_free (gstring, TRUE);

                //TODO: Correct below code
                /*release_list = mb5_release_get_releaselist ((Mb5Release)entity);
                gtk_list_store_set (list_store, &iter,
                                    MB_ALBUM_COLUMNS_RELEASES,
                                    mb5_release_list_get_count (release_list), -1);
                mb5_release_list_delete (release_list);*/
                break;

            case MB_ENTITY_TYPE_TRACK:
                mb5_recording_get_id ((Mb5Recording)entity, name, sizeof (name));
                gtk_list_store_append (list_store, &iter);
                gtk_list_store_set (list_store, &iter, 
                                    MB_TRACK_COLUMNS_NAME, name, -1);

                /* TODO: Get country and number */
                gtk_list_store_set (list_store, &iter,
                                    MB_TRACK_COLUMNS_COUNTRY, name, -1);

                gtk_list_store_set (list_store, &iter,
                                    MB_TRACK_COLUMNS_TIME,
                                    mb5_recording_get_length ((Mb5Recording)entity), -1);
                break;

            case MB_ENTITY_TYPE_COUNT:
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
    }

    gtk_tree_view_set_model (GTK_TREE_VIEW (priv->tree_view), NULL);

    /* Remove all colums */
    list_cols = gtk_tree_view_get_columns (GTK_TREE_VIEW (priv->tree_view));
    for (list = g_list_first (list_cols); list != NULL; list = g_list_next (list))
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

    types = g_malloc (sizeof (GType) * total_cols);

    for (i = 0; i < total_cols; i++)
    {
        types [i] = G_TYPE_STRING;
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (columns[type][i], 
                                                           renderer, "text", i, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (priv->tree_view), column);
    }

    priv->list_store = GTK_TREE_MODEL (gtk_list_store_newv (total_cols, types));
    g_free (types);

    gtk_tree_view_set_model (GTK_TREE_VIEW (priv->tree_view), priv->list_store);
    g_object_unref (priv->list_store);

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
    priv->mb_tree_current_node = priv->bread_crumb_nodes[g_list_index (children, btn)];
    priv->active_toggle_button = btn;
    show_data_in_entity_view (entity_view);
}

static void
search_in_levels_callback (GObject *source, GAsyncResult *res,
                           gpointer user_data)
{
    EtMbEntityView *entity_view;
    EtMbEntityViewPrivate *priv;
    GtkWidget *toggle_btn;
    GtkTreeSelection *selection;
    int depth;
    GtkTreeIter iter;
    GtkTreePath *path;
    gchar *entity_name;
    GNode *child;

    entity_view = ET_MB_ENTITY_VIEW (user_data);
    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);

    /* Depth is 1-based */
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree_view));
    gtk_tree_selection_get_selected (selection, &(priv->list_store), &iter);
    path = gtk_tree_model_get_path (priv->list_store, &iter);
    depth = gtk_tree_path_get_depth (path);
    gtk_tree_path_free (path);
    child = g_node_nth_child (priv->mb_tree_current_node,
                              depth - 1);

    /* Check if child node has children or not */
    if (!g_node_first_child (child))
    {
        return;
    }

    priv->mb_tree_current_node = child;

    if (((EtMbEntity *)(priv->mb_tree_current_node->data))->type ==
        MB_ENTITY_TYPE_TRACK)
    {
        return;
    }

    toggle_btn = insert_togglebtn_in_breadcrumb (GTK_BOX (priv->bread_crumb_box));
    priv->bread_crumb_nodes [g_list_length (gtk_container_get_children (GTK_CONTAINER (priv->bread_crumb_box))) - 1] = priv->mb_tree_current_node;

    if (priv->active_toggle_button)
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->active_toggle_button),
                                      FALSE);
    }

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle_btn), TRUE);
    g_signal_connect (G_OBJECT (toggle_btn), "clicked",
                      G_CALLBACK (toggle_button_clicked), entity_view);
    priv->active_toggle_button = toggle_btn;
    gtk_tree_model_get (priv->list_store, &iter, 0, &entity_name, -1);
    gtk_button_set_label (GTK_BUTTON (toggle_btn), entity_name);
    gtk_widget_show_all (GTK_WIDGET (priv->bread_crumb_box));
    show_data_in_entity_view (entity_view);
    g_object_unref (res);
}

static void
search_in_levels_thread_func (GSimpleAsyncResult *res, GObject *obj,
                              GCancellable *cancellable)
{
    /* TODO: Call GTK+ functions from main thread */
    EtMbEntityView *entity_view;
    EtMbEntityViewPrivate *priv;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GtkTreePath *path;
    GNode *child;
    gchar mbid [NAME_MAX_SIZE];
    int depth;
    GError *error;

    entity_view = ET_MB_ENTITY_VIEW (g_async_result_get_user_data (G_ASYNC_RESULT (res)));
    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree_view));
    gtk_tree_selection_get_selected (selection, &(priv->list_store), &iter);
    path = gtk_tree_model_get_path (priv->list_store, &iter);
    depth = gtk_tree_path_get_depth (path);
    gtk_tree_path_free (path);
    child = g_node_nth_child (priv->mb_tree_current_node,
                              depth - 1);
    if (((EtMbEntity *)child->data)->type == MB_ENTITY_TYPE_TRACK)
    {
        return;
    }
    else if (((EtMbEntity *)child->data)->type == MB_ENTITY_TYPE_ARTIST)
    {
        mb5_artist_get_id (((EtMbEntity *)child->data)->entity, mbid, sizeof (mbid));
    }
    else if (((EtMbEntity *)child->data)->type == MB_ENTITY_TYPE_ALBUM)
    {
        mb5_release_get_id (((EtMbEntity *)child->data)->entity, mbid, sizeof (mbid));
    }

    error = NULL;

    if (!et_musicbrainz_search_in_entity (((EtMbEntity *)child->data)->type + 1,
                                          ((EtMbEntity *)child->data)->type, mbid,
                                          child, &error))
    {
        g_simple_async_report_gerror_in_idle (NULL,
                                              mb5_search_error_callback,
                                              NULL, error);
    }
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
    /* TODO: Add Cancellable object */
    /* TODO: Use GSimpleAsyncResult with GError */
    /* TODO: Display Status Bar messages */
    async_result = g_simple_async_result_new (NULL,
                                              search_in_levels_callback,
                                              user_data,
                                              tree_view_row_activated);
    g_simple_async_result_run_in_thread (async_result,
                                         search_in_levels_thread_func,
                                         0, NULL);
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

    gtk_orientable_set_orientation (GTK_ORIENTABLE (entity_view), GTK_ORIENTATION_VERTICAL);

    /* Adding child widgets */    
    priv->bread_crumb_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
    priv->tree_view = gtk_tree_view_new ();
    priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (priv->scrolled_window), priv->tree_view);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->scrolled_window),
                                    GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
    gtk_box_pack_start (GTK_BOX (entity_view), priv->bread_crumb_box,
                        FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (entity_view), priv->scrolled_window,
                        TRUE, TRUE, 2);
    priv->mb_tree_root = NULL;
    priv->mb_tree_current_node = NULL;
    priv->active_toggle_button = NULL;

    g_signal_connect (G_OBJECT (priv->tree_view), "row-activated",
                      G_CALLBACK (tree_view_row_activated), entity_view);
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
    btn = insert_togglebtn_in_breadcrumb (GTK_BOX (priv->bread_crumb_box));
    child = g_node_first_child (treeRoot);
    if (child)
    {
        switch (((EtMbEntity *)child->data)->type)
        {
            case MB_ENTITY_TYPE_ARTIST:
                gtk_button_set_label (GTK_BUTTON (btn), "Artists");
                break;

            case MB_ENTITY_TYPE_ALBUM:
                gtk_button_set_label (GTK_BUTTON (btn), "Album");
                break;

            case MB_ENTITY_TYPE_TRACK:
                gtk_button_set_label (GTK_BUTTON (btn), "Track");
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