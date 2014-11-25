/* EasyTAG - tag editor for audio files
 * Copyright (C) 2015  David King <amigadave@amigadave.com>
 * Copyright (C) 2014  Abhinav Jangda <abhijangda@hotmail.com>
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

#ifdef ENABLE_MUSICBRAINZ

#include "musicbrainz_dialog.h"

#include <glib/gi18n.h>

#include "musicbrainz.h"

typedef struct
{
    EtMusicbrainz *mb;

    GtkWidget *search_combo;
    GtkWidget *search_entry;
    GtkWidget *search_button;
    GtkWidget *stop_button;
    GtkWidget *results_view;

    GtkListStore *results_model;

    GCancellable *cancellable;
} EtMusicbrainzDialogPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EtMusicbrainzDialog, et_musicbrainz_dialog, GTK_TYPE_DIALOG)

static void
stop_search (EtMusicbrainzDialog *self)
{
    EtMusicbrainzDialogPrivate *priv;

    priv = et_musicbrainz_dialog_get_instance_private (self);

    gtk_widget_set_sensitive (priv->search_combo, TRUE);
    gtk_widget_set_sensitive (priv->search_entry, TRUE);
    gtk_widget_set_sensitive (priv->search_button, TRUE);
    gtk_widget_set_sensitive (priv->stop_button, FALSE);
    gtk_widget_set_sensitive (priv->results_view, TRUE);
}

static void
on_stop_button_clicked (EtMusicbrainzDialog *self,
                        GtkButton *stop_button)
{
    EtMusicbrainzDialogPrivate *priv;

    priv = et_musicbrainz_dialog_get_instance_private (self);

    g_cancellable_cancel (priv->cancellable);
    g_object_unref (priv->cancellable);
    priv->cancellable = g_cancellable_new ();

    stop_search (self);
}

static void
add_string_to_results_model (const gchar *string,
                             GtkListStore *model)
{
    gtk_list_store_insert_with_values (model, NULL, -1, 0, string, -1);
}

static void
query_complete_cb (GObject *source_object,
                   GAsyncResult *res,
                   gpointer user_data)
{
    EtMusicbrainzDialog *self;
    EtMusicbrainzDialogPrivate *priv;
    EtMusicbrainzResult *result;
    GError *error = NULL;
    GList *results;

    self = ET_MUSICBRAINZ_DIALOG (user_data);
    priv = et_musicbrainz_dialog_get_instance_private (self);

    result = et_musicbrainz_search_finish (priv->mb, res, &error);

    if (!result)
    {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
            g_debug ("%s", "MusicBrainz search cancelled by user");
        }
        else
        {
            /* TODO: Show the error in the UI. */
            g_message ("Search failed: %s", error->message);
        }

        g_error_free (error);
        return;
    }

    results = et_musicbrainz_result_get_results (result);

    g_list_foreach (results, (GFunc)add_string_to_results_model,
                    priv->results_model);

    et_musicbrainz_result_unref (result);

    stop_search (self);
}

static void
start_search (EtMusicbrainzDialog *self)
{
    EtMusicbrainzDialogPrivate *priv;
    EtMusicbrainzEntity entity;
    EtMusicbrainzQuery *query;

    priv = et_musicbrainz_dialog_get_instance_private (self);

    gtk_widget_set_sensitive (priv->search_combo, FALSE);
    gtk_widget_set_sensitive (priv->search_entry, FALSE);
    gtk_widget_set_sensitive (priv->search_button, FALSE);
    gtk_widget_set_sensitive (priv->stop_button, TRUE);
    gtk_widget_set_sensitive (priv->results_view, FALSE);

    gtk_list_store_clear (priv->results_model);

    entity = gtk_combo_box_get_active (GTK_COMBO_BOX (priv->search_combo));
    query = et_musicbrainz_query_new (entity,
                                      gtk_entry_get_text (GTK_ENTRY (priv->search_entry)));

    et_musicbrainz_search_async (priv->mb, query, priv->cancellable,
                                 query_complete_cb, self);

    et_musicbrainz_query_unref (query);
}

static void
on_search_button_clicked (EtMusicbrainzDialog *self,
                          GtkButton *search_button)
{
    start_search (self);
}

static void
on_search_entry_activate (EtMusicbrainzDialog *self,
                          GtkEntry *search_entry)
{
    start_search (self);
}

static void
et_musicbrainz_dialog_finalize (GObject *object)
{
    EtMusicbrainzDialog *self;
    EtMusicbrainzDialogPrivate *priv;

    self = ET_MUSICBRAINZ_DIALOG (object);
    priv = et_musicbrainz_dialog_get_instance_private (self);

    g_clear_object (&priv->mb);

    if (priv->cancellable)
    {
        g_cancellable_cancel (priv->cancellable);
        g_clear_object (&priv->cancellable);
    }

    G_OBJECT_CLASS (et_musicbrainz_dialog_parent_class)->finalize (object);
}

static void
et_musicbrainz_dialog_init (EtMusicbrainzDialog *self)
{
    EtMusicbrainzDialogPrivate *priv;

    priv = et_musicbrainz_dialog_get_instance_private (self);

    gtk_widget_init_template (GTK_WIDGET (self));

    priv->mb = et_musicbrainz_new ();
    priv->cancellable = g_cancellable_new ();
}

static void
et_musicbrainz_dialog_class_init (EtMusicbrainzDialogClass *klass)
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;

    gobject_class = G_OBJECT_CLASS (klass);
    widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/EasyTAG/musicbrainz_dialog.ui");
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_search_button_clicked);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_search_entry_activate);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_stop_button_clicked);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtMusicbrainzDialog,
                                                  results_model);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtMusicbrainzDialog,
                                                  results_view);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtMusicbrainzDialog,
                                                  search_combo);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtMusicbrainzDialog,
                                                  search_entry);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtMusicbrainzDialog,
                                                  search_button);
    gtk_widget_class_bind_template_child_private (widget_class,
                                                  EtMusicbrainzDialog,
                                                  stop_button);

    gobject_class->finalize = et_musicbrainz_dialog_finalize;
}

/*
 * et_musicbrainz_dialog_new:
 *
 * Create a new EtMusicbrainzDialog instance.
 *
 * Returns: a new #EtMusicbrainzDialog
 */
EtMusicbrainzDialog *
et_musicbrainz_dialog_new (GtkWindow *parent)
{
    g_return_val_if_fail (GTK_WINDOW (parent), NULL);

    return g_object_new (ET_TYPE_MUSICBRAINZ_DIALOG, "transient-for", parent,
                         NULL);
}

#endif /* ENABLE_MUSICBRAINZ */
