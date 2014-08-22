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
#include "cddb.h"
#include "easytag.h"
#include "et_core.h"
#include "browser.h"
#include "scan_dialog.h"
#include "log.h"
#include "misc.h"
#include "setting.h"
#include "id3_tag.h"
#include "setting.h"
#include "charset.h"
#include "musicbrainz_dialog.h"
#include "mbentityview.h"

/***************
 * Declaration *
 ***************/

static GtkBuilder *builder;
static GtkWidget *mbDialog;
static GtkWidget *entityView;

/*************
 * Functions *
 *************/

/*
 * et_open_musicbrainz_dialog:
 *
 * This function will open the musicbrainz dialog.
 */
void
et_open_musicbrainz_dialog ()
{
    entityView = et_mb_entity_view_new ();
    builder = gtk_builder_new ();
    /* TODO: Check the error. */
    gtk_builder_add_from_resource (builder,
                                   "/org/gnome/EasyTAG/musicbrainz_dialog.ui",
                                   NULL);

    mbDialog = GTK_WIDGET (gtk_builder_get_object (builder, "mbDialog"));

    gtk_box_pack_start (GTK_BOX (gtk_builder_get_object (builder, "centralBox")),
                        entityView, TRUE, TRUE, 2);
    /* FIXME: This should not be needed. */
    gtk_box_reorder_child (GTK_BOX (gtk_builder_get_object (builder, "centralBox")),
                           entityView, 0);

    gtk_widget_show_all (mbDialog);
    gtk_dialog_run (GTK_DIALOG (mbDialog));
    gtk_widget_destroy (mbDialog);
    g_object_unref (G_OBJECT (builder));
}
