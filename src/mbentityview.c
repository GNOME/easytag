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

#define ET_MB_ENTITY_VIEW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                            ET_MB_ENTITY_VIEW_TYPE, \
                                            EtMbEntityViewPrivate))

/***************
 * Declaration *
 ***************/

/*
 * EtMbEntityViewPrivate:
 * @breadCrumbBox: GtkToolbar which represents the BreadCrumbWidget
 * @treeView: GtkTreeView to display the recieved music brainz data
 * @listStore: GtkListStore for treeView
 * @scrolledWindow: GtkScrolledWindow for treeView
 *
 * Private data for EtMbEntityView.
 */
typedef struct
{
    GtkWidget *breadCrumbBox;
    GtkWidget *treeView;
    GtkListStore *listStore;
    GtkWidget *scrolledWindow;
} EtMbEntityViewPrivate;

/**************
 * Prototypes *
 **************/

static void
et_mb_entity_view_class_init (EtMbEntityViewClass *klass);

static void
et_mb_entity_view_init (EtMbEntityView *proj_notebook);

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
 * klass: EtMbEntityViewClass to initialize.
 *
 * Initializes an EtMbEntityViewClass class.
 */
static void
et_mb_entity_view_class_init (EtMbEntityViewClass *klass)
{
    g_type_class_add_private (klass, sizeof (EtMbEntityViewPrivate));
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

    /* Adding child widgets */
    priv->breadCrumbBox = gtk_toolbar_new ();
    priv->listStore = gtk_list_store_new (0);
    priv->treeView = gtk_tree_view_new_with_model (GTK_TREE_MODEL (priv->listStore));
    priv->scrolledWindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (priv->scrolledWindow), priv->treeView);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->scrolledWindow),
                                    GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
    gtk_box_pack_start (GTK_BOX (entity_view), priv->breadCrumbBox,
                        FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (entity_view), priv->scrolledWindow,
                        TRUE, TRUE, 2);
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