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

#include "mbentityview.h"

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


/*
 * EtMbEntityViewPrivate:
 * @breadCrumbBox: GtkToolbar which represents the BreadCrumbWidget
 * @treeView: GtkTreeView to display the recieved music brainz data
 * @breadCrumbNodes: Array of GNode being displayed by the GtkToggleToolButton
 * @listStore: GtkTreeStore for treeView
 * @scrolledWindow: GtkScrolledWindow for treeView
 * @mbTreeRoot: Root Node of the Mb5Entity Tree
 * @mbTreeCurrentNode: Current node being displayed by EtMbEntityView
 * @activeToggleButton: Current active GtkToggleToolButton
 *
 * Private data for EtMbEntityView.
 */
typedef struct
{
    GtkWidget *breadCrumbBox;
    GNode *breadCrumbNodes[MB_ENTITY_TYPE_COUNT];
    GtkWidget *treeView;
    GtkTreeModel *listStore;
    GtkWidget *scrolledWindow;
    GNode *mbTreeRoot;
    GNode *mbTreeCurrentNode;
    GtkToolItem *activeToggleButton;
} EtMbEntityViewPrivate;

/**************
 * Prototypes *
 **************/

static void
et_mb_entity_view_class_init (EtMbEntityViewClass *klass);
static void
et_mb_entity_view_init (EtMbEntityView *proj_notebook);
static GtkToolItem *
insert_togglebtn_in_toolbar (GtkToolbar *toolbar);
static void
add_iter_to_list_store (GtkListStore *list_store, GNode *node);
static void
show_data_in_entity_view (EtMbEntityView *entity_view);
static void
toggle_button_clicked (GtkToolButton *btn, gpointer user_data);
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
 * @toolbar: GtkToolbar in which GtkToggleToolButton will be inserted
 *
 * Returns: GtkToolItem inserted in GtkToolbar.
 * Insert a GtkToggleToolButton in GtkToolbar.
 */
static GtkToolItem *
insert_togglebtn_in_toolbar (GtkToolbar *toolbar)
{
    GtkToolItem *btn;
    btn = gtk_toggle_tool_button_new ();
    gtk_toolbar_insert (toolbar, btn, -1);
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
    /* Traverse node in GNode and add it to listStore */
    enum MB_ENTITY_TYPE type;
    Mb5ReleaseList *release_list;
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
                mb5_releasegroup_get_title ((Mb5ReleaseGroup)entity, name, sizeof (name));
                gtk_list_store_append (list_store, &iter);
                gtk_list_store_set (list_store, &iter, 
                                     MB_ALBUM_COLUMNS_NAME, name, -1);

                artist_credit = mb5_releasegroup_get_artistcredit ((Mb5ReleaseGroup)entity);
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

                release_list = mb5_releasegroup_get_releaselist ((Mb5ReleaseGroup)entity);
                gtk_list_store_set (list_store, &iter,
                                    MB_ALBUM_COLUMNS_RELEASES,
                                    mb5_release_list_get_count (release_list), -1);
                mb5_release_list_delete (release_list);
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
    if (GTK_IS_LIST_STORE (priv->listStore))
    {
        gtk_list_store_clear (GTK_LIST_STORE (priv->listStore));
    }

    gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeView), NULL);

    /* Remove all colums */
    list_cols = gtk_tree_view_get_columns (GTK_TREE_VIEW (priv->treeView));
    for (list = g_list_first (list_cols); list != NULL; list = g_list_next (list))
    {
        gtk_tree_view_remove_column (GTK_TREE_VIEW (priv->treeView),
                                     GTK_TREE_VIEW_COLUMN (list->data));
    }

    g_list_free (list_cols);

    /* Create new List Store and add it */
    type = ((EtMbEntity *)(g_node_first_child (priv->mbTreeCurrentNode)->data))->type;
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
        gtk_tree_view_append_column (GTK_TREE_VIEW (priv->treeView), column);
    }

    priv->listStore = GTK_TREE_MODEL (gtk_list_store_newv (total_cols, types));
    g_free (types);

    gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeView), priv->listStore);
    g_object_unref (priv->listStore);

    add_iter_to_list_store (GTK_LIST_STORE (priv->listStore),
                            g_node_first_child (priv->mbTreeCurrentNode));
}

/*
 * toggle_button_clicked:
 * @btn: The GtkToolButton clicked.
 * @user_data: User Data passed to the handler.
 *
 * The singal handler for GtkToggleToolButton's clicked signal.
 */
static void
toggle_button_clicked (GtkToolButton *btn, gpointer user_data)
{
    EtMbEntityView *entity_view;
    EtMbEntityViewPrivate *priv;

    entity_view = ET_MB_ENTITY_VIEW (user_data);
    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);

    if (btn == GTK_TOOL_BUTTON (priv->activeToggleButton))
    {
        return;
    }

    if (!gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (btn)))
    {
        return;
    }

    if (priv->activeToggleButton)
    {
        gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (priv->activeToggleButton),
                                           FALSE);
    }

    priv->mbTreeCurrentNode = priv->breadCrumbNodes[gtk_toolbar_get_item_index (GTK_TOOLBAR (priv->breadCrumbBox),
                                                    GTK_TOOL_ITEM (btn))];
    priv->activeToggleButton = GTK_TOOL_ITEM (btn);
    show_data_in_entity_view (entity_view);
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
    EtMbEntityView *entity_view;
    EtMbEntityViewPrivate *priv;
    GtkToolItem *toggle_btn;
    int depth;
    GtkTreeIter iter;
    gchar *entity_name;
    GNode *child;

    entity_view = ET_MB_ENTITY_VIEW (user_data);
    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);

    /* Depth is 1-based */
    depth = gtk_tree_path_get_depth (path);
    child = g_node_nth_child (priv->mbTreeCurrentNode,
                              depth - 1);

    /* Check if child node has children or not */
    if (!g_node_first_child (child))
    {
        return;
    }

    priv->mbTreeCurrentNode = child;

    if (((EtMbEntity *)(priv->mbTreeCurrentNode->data))->type ==
        MB_ENTITY_TYPE_TRACK)
    {
        return;
    }

    toggle_btn = insert_togglebtn_in_toolbar (GTK_TOOLBAR (priv->breadCrumbBox));
    priv->breadCrumbNodes [gtk_toolbar_get_n_items (GTK_TOOLBAR (priv->breadCrumbBox)) - 1] = priv->mbTreeCurrentNode;

    for (depth = gtk_toolbar_get_n_items (GTK_TOOLBAR (priv->breadCrumbBox));
         depth < MB_ENTITY_TYPE_COUNT; depth++)
    {
        priv->breadCrumbNodes [depth] = NULL;
    }

    if (priv->activeToggleButton)
    {
        gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (priv->activeToggleButton),
                                           FALSE);
    }

    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (toggle_btn), TRUE);
    g_signal_connect (G_OBJECT (toggle_btn), "clicked",
                      G_CALLBACK (toggle_button_clicked), entity_view);
    priv->activeToggleButton = toggle_btn;
    gtk_tree_model_get_iter (priv->listStore, &iter, path);
    /* TODO: Toggle Tool Button not setting the label */
    gtk_tree_model_get (priv->listStore, &iter, 0, &entity_name, -1);
    printf ("entity_name %s\n", entity_name);
    gtk_tool_button_set_label_widget (GTK_TOOL_BUTTON (toggle_btn), NULL);
    gtk_tool_button_set_label (GTK_TOOL_BUTTON (toggle_btn), entity_name);
    gtk_widget_show_all (GTK_WIDGET (priv->breadCrumbBox));
    show_data_in_entity_view (entity_view);
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
    priv->breadCrumbBox = gtk_toolbar_new ();
    priv->treeView = gtk_tree_view_new ();
    priv->scrolledWindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (priv->scrolledWindow), priv->treeView);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->scrolledWindow),
                                    GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
    gtk_box_pack_start (GTK_BOX (entity_view), priv->breadCrumbBox,
                        FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (entity_view), priv->scrolledWindow,
                        TRUE, TRUE, 2);
    priv->mbTreeRoot = NULL;
    priv->mbTreeCurrentNode = NULL;
    priv->activeToggleButton = NULL;

    g_signal_connect (G_OBJECT (priv->treeView), "row-activated",
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
    GtkToolItem *btn;
    priv = ET_MB_ENTITY_VIEW_GET_PRIVATE (entity_view);
    priv->mbTreeRoot = treeRoot;
    priv->mbTreeCurrentNode = treeRoot;
    btn = insert_togglebtn_in_toolbar (GTK_TOOLBAR (priv->breadCrumbBox));
    gtk_tool_button_set_label (GTK_TOOL_BUTTON (btn), "Artists");
    priv->breadCrumbNodes [0] = treeRoot;
    priv->activeToggleButton = btn;
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (btn), TRUE);
    show_data_in_entity_view (entity_view);
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