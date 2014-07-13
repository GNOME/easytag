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

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "bar.h"
#include "charset.h"
#include "gtk2_compat.h"

/***************
 * Declaration *
 ***************/
static GtkWidget *StatusBar = NULL;
static guint StatusBarContext;
static guint timer_cid;
static guint StatusbarTimerId = 0;

/**************
 * Prototypes *
 **************/

static void Statusbar_Remove_Timer (void);

/*************
 * Functions o
 *************/

/*
 * Status bar functions
 */
GtkWidget *Create_Status_Bar (void)
{
    StatusBar = gtk_statusbar_new();
    /* Specify a size to avoid statubar resizing if the message is too long */
    gtk_widget_set_size_request(StatusBar, 200, -1);
    /* Create serie */
    StatusBarContext = gtk_statusbar_get_context_id(GTK_STATUSBAR(StatusBar),"Messages");
    timer_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (StatusBar),
                                              "timer");

    Statusbar_Message (_("Ready to start"), TRUE);

    gtk_widget_show(StatusBar);
    return StatusBar;
}

static gboolean
Statusbar_Stop_Timer (void)
{
    gtk_statusbar_pop (GTK_STATUSBAR (StatusBar), timer_cid);
    return G_SOURCE_REMOVE;
}

static void
et_statusbar_reset_timer (void)
{
    StatusbarTimerId = 0;
}

static void
Statusbar_Start_Timer (void)
{
    Statusbar_Remove_Timer ();
    StatusbarTimerId = g_timeout_add_seconds_full (G_PRIORITY_DEFAULT, 4,
                                                   (GSourceFunc)Statusbar_Stop_Timer,
                                                   NULL,
                                                   (GDestroyNotify)et_statusbar_reset_timer);
    g_source_set_name_by_id (StatusbarTimerId, "Statusbar stop timer");
}

static void
Statusbar_Remove_Timer (void)
{
    if (StatusbarTimerId)
    {
        Statusbar_Stop_Timer ();
        g_source_remove(StatusbarTimerId);
        et_statusbar_reset_timer ();
    }
}

/*
 * Send a message to the status bar
 *  - with_timer: if TRUE, the message will be displayed during 4s
 *                if FALSE, the message will be displayed up to the next posted message
 */
void
Statusbar_Message (const gchar *message, gboolean with_timer)
{
    gchar *msg_temp;

    g_return_if_fail (StatusBar != NULL);

    msg_temp = Try_To_Validate_Utf8_String(message);
    
    /* Push the given message */
    if (with_timer)
    {
        Statusbar_Start_Timer ();
        gtk_statusbar_push (GTK_STATUSBAR (StatusBar), timer_cid, msg_temp);
    }
    else
    {
        gtk_statusbar_pop (GTK_STATUSBAR (StatusBar), StatusBarContext);
        gtk_statusbar_push (GTK_STATUSBAR (StatusBar), StatusBarContext,
                            msg_temp);
    }

    g_free(msg_temp);
}

/*
 * Progress bar
 */
GtkWidget *Create_Progress_Bar (void)
{
    ProgressBar = et_progress_bar_new ();

    gtk_widget_show(ProgressBar);
    return ProgressBar;
}
