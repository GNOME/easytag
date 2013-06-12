/* EasyTAG - tag editor for audio files
 * Copyright (C) 2013  David King <amigadave@amigadave.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "discid.h"
#include "gtk2_compat.h"

#include <discid/discid.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (EtCDInfoDialog, et_cd_info_dialog, GTK_TYPE_DIALOG)

static const guint BOX_SPACING = 6;

struct _EtCDInfoDialogPrivate
{
    /*< private >*/
    DiscId *id;
    GtkWidget *mcn;
    GtkWidget *length;
    GtkWidget *n_tracks;
    GtkListStore *store;
};

typedef enum
{
    COLUMN_NUMBER,
    COLUMN_OFFSET,
    COLUMN_LENGTH,
    COLUMN_ISRC,
    N_COLUMNS
} EtCDInfoDialogColumns;

static void et_cd_info_dialog_on_response (EtCDInfoDialog *dialog,
                                           gint response_id,
                                           gpointer user_data);

static void
et_cd_info_dialog_finalize (GObject *object)
{
    EtCDInfoDialogPrivate *priv = ET_CD_INFO_DIALOG (object)->priv;

    discid_free (priv->id);
    priv->id = NULL;

    G_OBJECT_CLASS (et_cd_info_dialog_parent_class)->finalize (object);
}

static void
et_cd_info_dialog_init (EtCDInfoDialog *dialog)
{
    EtCDInfoDialogPrivate *priv;
    GtkWidget *content_area;
    GtkWidget *grid;
    GtkWidget *treeview;
    GtkWidget *scrolled;
    GtkWidget *label;
    EtCDInfoDialogColumns i;
    const gchar *column_titles[] = { N_("Track number"), N_("Offset"),
                                     N_("Length"), N_("ISRC") };

    priv = dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog,
                                                       ET_TYPE_CD_INFO_DIALOG,
                                                       EtCDInfoDialogPrivate);

    priv->id = discid_new ();

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

    grid = et_grid_new (4, 2);
    gtk_grid_set_row_spacing (GTK_GRID (grid), BOX_SPACING);
    gtk_grid_set_column_spacing (GTK_GRID (grid), BOX_SPACING);

    label = gtk_label_new (_("MCN:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
    label = gtk_label_new (_("Length:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
    label = gtk_label_new (_("Number of tracks:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);

    priv->mcn = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (priv->mcn), 0.0, 0.5);
    gtk_grid_attach (GTK_GRID (grid), priv->mcn, 1, 0, 1, 1);
    priv->length = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (priv->length), 0.0, 0.5);
    gtk_grid_attach (GTK_GRID (grid), priv->length, 1, 1, 1, 1);
    priv->n_tracks = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (priv->n_tracks), 0.0, 0.5);
    gtk_grid_attach (GTK_GRID (grid), priv->n_tracks, 1, 2, 1, 1);

    priv->store = gtk_list_store_new (N_COLUMNS, G_TYPE_INT, G_TYPE_INT,
                                      G_TYPE_INT, G_TYPE_STRING);
    treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (priv->store));
    /* GtkListStore is a GObject, so starts with a reference count of 1. */
    g_object_unref (priv->store);
    gtk_widget_set_size_request (treeview, -1, 300);

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (scrolled), treeview);
    et_grid_attach_full (GTK_GRID (grid), scrolled, 0, 3, 2, 1, FALSE, TRUE, 0,
                         0);

    for (i = 0; i < N_COLUMNS; i++)
    {
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_(column_titles[i]),
                                                           renderer, "text",
                                                           i,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
    }

    gtk_container_add (GTK_CONTAINER (content_area), grid);
    gtk_widget_show_all (grid);

    gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CLOSE,
                            GTK_RESPONSE_CLOSE, _("Read CD"),
                            GTK_RESPONSE_APPLY, NULL);

    g_signal_connect (dialog, "response",
                      G_CALLBACK (et_cd_info_dialog_on_response), NULL);
}

static void
et_cd_info_dialog_class_init (EtCDInfoDialogClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = et_cd_info_dialog_finalize;

    g_type_class_add_private (klass, sizeof (EtCDInfoDialogPrivate));
}

/*
 * et_cd_info_dialog_new:
 *
 * Create a new EtCDInfoDialog instance.
 *
 * Returns: a new #EtCDInfoDialog
 */
EtCDInfoDialog *
et_cd_info_dialog_new (GtkWindow *parent)
{
    return ET_CD_INFO_DIALOG (g_object_new (ET_TYPE_CD_INFO_DIALOG,
                                            "transient-for", parent, "title",
                                            _("Read CD Information"), NULL));
}

/*
 * et_cd_info_dialog_read_cd:
 * @dialog: the dialog
 *
 * Read information from the inserted CD and populate the CD and track
 * information structures.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
static gboolean
et_cd_info_dialog_read_cd (EtCDInfoDialog *dialog)
{
    gint first_track;
    gint last_track;
    gchar *str;
    gint i;
    EtCDInfoDialogPrivate *priv = dialog->priv;
    DiscId *id = priv->id;

    /* TODO: Run asynchronously in a thread. */
    if (!discid_read (id, NULL))
    {
        /* FIXME: set GError. */
        g_warning ("Error reading CD: %s", discid_get_error_msg (id));
        return FALSE;
    }

    gtk_label_set_text (GTK_LABEL (priv->mcn), discid_get_mcn (id));

    first_track = discid_get_first_track_num (id);
    last_track = discid_get_last_track_num (id);
    str = g_strdup_printf ("%i", last_track - first_track);
    gtk_label_set_text (GTK_LABEL (priv->n_tracks), str);
    g_free (str);

    str = g_strdup_printf ("%i", discid_get_sectors (id));
    gtk_label_set_text (GTK_LABEL (priv->length), str);
    g_free (str);

    gtk_list_store_clear (priv->store);

    for (i = first_track; i < last_track; i++)
    {
        gtk_list_store_insert_with_values (priv->store, NULL, G_MAXINT,
                                           COLUMN_NUMBER, i, COLUMN_OFFSET,
                                           discid_get_track_offset (id, i),
                                           COLUMN_LENGTH,
                                           discid_get_track_length (id, i),
                                           COLUMN_ISRC,
                                           discid_get_track_isrc (id, i), -1);
    }

    return TRUE;
}

/*
 * et_cd_info_dialog_on_response:
 * @dialog: the dialog
 * @response_id: the #GtkResponseType
 * @user_data: user data set when the signal was connected
 *
 * Handle a response from the dialog, and respond by either closing the dialog
 * or reading information from a CD.
 */
static void
et_cd_info_dialog_on_response (EtCDInfoDialog *dialog, gint response_id,
                               gpointer user_data)
{
    switch (response_id)
    {
        case GTK_RESPONSE_APPLY:
            if (!et_cd_info_dialog_read_cd (dialog))
            {
                g_warning ("Failed to read CD information");
            }
            break;
        case GTK_RESPONSE_CLOSE:
        case GTK_RESPONSE_DELETE_EVENT:
            /* TODO: destroy here? */
            gtk_widget_destroy (GTK_WIDGET (dialog));
            break;
        default:
            g_assert_not_reached ();
    }
}
