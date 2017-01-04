/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
 * Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
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

#include "status_bar.h"

#include <glib/gi18n.h>

#include "charset.h"

typedef struct
{
    guint message_context;
    guint timer_context;
    guint timer_id;
} EtStatusBarPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EtStatusBar, et_status_bar, GTK_TYPE_STATUSBAR)

static void et_status_bar_remove_timer (EtStatusBar *self);

static gboolean
et_status_bar_stop_timer (EtStatusBar *self)
{
    EtStatusBarPrivate *priv;

    priv = et_status_bar_get_instance_private (self);

    gtk_statusbar_pop (GTK_STATUSBAR (self), priv->timer_context);

    return G_SOURCE_REMOVE;
}

static void
et_status_bar_reset_timer (EtStatusBar *self)
{
    EtStatusBarPrivate *priv;

    priv = et_status_bar_get_instance_private (self);

    priv->timer_id = 0;
}

static void
et_status_bar_start_timer (EtStatusBar *self)
{
    EtStatusBarPrivate *priv;

    priv = et_status_bar_get_instance_private (self);

    et_status_bar_remove_timer (self);
    priv->timer_id = g_timeout_add_seconds_full (G_PRIORITY_DEFAULT, 4,
                                                 (GSourceFunc)et_status_bar_stop_timer,
                                                 self,
                                                 (GDestroyNotify)et_status_bar_reset_timer);
    g_source_set_name_by_id (priv->timer_id, "Statusbar stop timer");
}

static void
et_status_bar_remove_timer (EtStatusBar *self)
{
    EtStatusBarPrivate *priv;

    priv = et_status_bar_get_instance_private (self);

    if (priv->timer_id)
    {
        et_status_bar_stop_timer (self);
        g_source_remove (priv->timer_id);
        et_status_bar_reset_timer (self);
    }
}

/*
 * Send a message to the status bar
 *  - with_timer: if TRUE, the message will be displayed during 4s
 *                if FALSE, the message will be displayed up to the next posted message
 */
void
et_status_bar_message (EtStatusBar *self,
                       const gchar *message,
                       gboolean with_timer)
{
    EtStatusBarPrivate *priv;
    gchar *msg_temp;

    g_return_if_fail (ET_STATUS_BAR (self));

    priv = et_status_bar_get_instance_private (self);

    msg_temp = Try_To_Validate_Utf8_String (message);
    
    /* Push the given message */
    if (with_timer)
    {
        et_status_bar_start_timer (self);
        gtk_statusbar_push (GTK_STATUSBAR (self), priv->timer_context,
                            msg_temp);
    }
    else
    {
        gtk_statusbar_pop (GTK_STATUSBAR (self), priv->message_context);
        gtk_statusbar_push (GTK_STATUSBAR (self), priv->message_context,
                            msg_temp);
    }

    g_free (msg_temp);
}

static void
create_status_bar (EtStatusBar *self)
{
    EtStatusBarPrivate *priv;

    priv = et_status_bar_get_instance_private (self);

    /* Specify a size to avoid statubar resizing if the message is too long */
    gtk_widget_set_size_request (GTK_WIDGET (self), 200, -1);

    /* Create serie */
    priv->message_context = gtk_statusbar_get_context_id (GTK_STATUSBAR (self),
                                                          "messages");
    priv->timer_context = gtk_statusbar_get_context_id (GTK_STATUSBAR (self),
                                                        "timer");

    et_status_bar_message (self, _("Ready to start"), TRUE);
}

static void
et_status_bar_finalize (GObject *object)
{
    EtStatusBarPrivate *priv;

    priv = et_status_bar_get_instance_private (ET_STATUS_BAR (object));

    if (priv->timer_id)
    {
        g_source_remove (priv->timer_id);
        priv->timer_id = 0;
    }
}

static void
et_status_bar_init (EtStatusBar *self)
{
    create_status_bar (self);
}

static void
et_status_bar_class_init (EtStatusBarClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = et_status_bar_finalize;
}

/*
 * et_status_bar_new:
 *
 * Create a new EtStatusBar instance.
 *
 * Returns: a new #EtStatusBar
 */
GtkWidget *
et_status_bar_new (void)
{
    return g_object_new (ET_TYPE_STATUS_BAR, NULL);
}
