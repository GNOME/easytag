/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
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

#include "file_area.h"

#include <glib/gi18n.h>

#include "charset.h"
#include "et_core.h"
#include "log.h"
#include "setting.h"
#include "tag_area.h"

typedef struct
{
    GtkWidget *file_label;

    GtkWidget *index_label;
    GtkWidget *name_entry;

    GtkWidget *header_grid;

    GtkWidget *version_label;
    GtkWidget *version_value_label;
    GtkWidget *bitrate_label;
    GtkWidget *bitrate_value_label;
    GtkWidget *samplerate_label;
    GtkWidget *samplerate_value_label;
    GtkWidget *mode_label;
    GtkWidget *mode_value_label;
    GtkWidget *size_label;
    GtkWidget *size_value_label;
    GtkWidget *duration_label;
    GtkWidget *duration_value_label;
} EtFileAreaPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EtFileArea, et_file_area, GTK_TYPE_BIN)

static void
on_file_show_header_changed (EtFileArea *self,
                             const gchar *key,
                             GSettings *settings)
{
    EtFileAreaPrivate *priv;

    priv = et_file_area_get_instance_private (self);

    if (g_settings_get_boolean (settings, key))
    {
        gtk_widget_show (priv->header_grid);
    }
    else
    {
        gtk_widget_hide (priv->header_grid);
    }
}

static void
create_file_area (EtFileArea *self)
{
    EtFileAreaPrivate *priv;
    GtkBuilder *builder;
    GError *error = NULL;
    GtkWidget *grid;

    priv = et_file_area_get_instance_private (self);

    builder = gtk_builder_new ();
    gtk_builder_add_from_resource (builder,
                                   "/org/gnome/EasyTAG/file_area.ui",
                                   &error);

    if (error != NULL)
    {
        g_error ("Unable to get file area from resource: %s",
                 error->message);
    }

    grid = GTK_WIDGET (gtk_builder_get_object (builder, "file_grid"));
    gtk_container_add (GTK_CONTAINER (self), grid);

    priv->file_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                           "file_label"));

    priv->index_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                            "index_label"));

    /* Filename. */
    priv->name_entry = GTK_WIDGET (gtk_builder_get_object (builder,
                                                           "filename_entry"));

    g_signal_connect (priv->name_entry, "populate-popup",
                      G_CALLBACK (on_entry_populate_popup), NULL);

    /* File information. */
    priv->header_grid = GTK_WIDGET (gtk_builder_get_object (builder,
                                                            "header_grid"));
    priv->version_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                              "version_label"));
    priv->version_value_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                    "version_value_label"));

    priv->bitrate_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                              "bitrate_label"));
    priv->bitrate_value_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                    "bitrate_value_label"));

    priv->samplerate_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                 "samplerate_label"));
    priv->samplerate_value_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                       "samplerate_value_label"));

    priv->mode_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                           "mode_label"));
    priv->mode_value_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                 "mode_value_label"));

    priv->size_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                           "size_label"));
    priv->size_value_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                 "size_value_label"));

    priv->duration_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                               "duration_label"));
    priv->duration_value_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                     "duration_value_label"));

    g_object_unref (builder);

    g_signal_connect_swapped (MainSettings, "changed::file-show-header",
                              G_CALLBACK (on_file_show_header_changed), self);
    on_file_show_header_changed (self, "file-show-header", MainSettings);
}

static void
et_file_area_init (EtFileArea *self)
{
    create_file_area (self);
}

static void
et_file_area_class_init (EtFileAreaClass *klass)
{
}

/*
 * et_file_area_new:
 *
 * Create a new EtFileArea instance.
 *
 * Returns: a new #EtFileArea
 */
GtkWidget *
et_file_area_new (void)
{
    return g_object_new (ET_TYPE_FILE_AREA, NULL);
}

void
et_file_area_clear (EtFileArea *self)
{
    EtFileAreaPrivate *priv;
    EtFileHeaderFields fields;
    gchar *empty_str;

    g_return_if_fail (ET_FILE_AREA (self));

    priv = et_file_area_get_instance_private (self);

    empty_str = g_strdup ("");

    /* Default values are MPEG data. */
    fields.description = _("File");
    fields.version_label = _("Encoder:");
    fields.version =  empty_str;
    fields.bitrate = empty_str;
    fields.samplerate = empty_str;
    fields.mode_label = _("Mode:");
    fields.mode = empty_str;
    fields.size = empty_str;
    fields.duration = empty_str;

    et_file_area_set_header_fields (self, &fields);

    gtk_entry_set_text (GTK_ENTRY (priv->name_entry), empty_str);
    gtk_label_set_text (GTK_LABEL (priv->index_label), "0/0:");

    g_free (empty_str);
}

void
et_file_area_set_header_fields (EtFileArea *self,
                                EtFileHeaderFields *fields)
{
    EtFileAreaPrivate *priv;

    g_return_if_fail (ET_FILE_AREA (self));
    g_return_if_fail (fields != NULL);

    priv = et_file_area_get_instance_private (self);

    gtk_label_set_text (GTK_LABEL (priv->file_label), fields->description);
    gtk_label_set_text (GTK_LABEL (priv->version_label),
                        fields->version_label);
    gtk_label_set_text (GTK_LABEL (priv->version_value_label),
                        fields->version);
    gtk_label_set_text (GTK_LABEL (priv->bitrate_value_label),
                        fields->bitrate);
    gtk_label_set_text (GTK_LABEL (priv->samplerate_value_label),
                        fields->samplerate);
    gtk_label_set_text (GTK_LABEL (priv->mode_label), fields->mode_label);
    gtk_label_set_text (GTK_LABEL (priv->mode_value_label), fields->mode);
    gtk_label_set_text (GTK_LABEL (priv->size_value_label), fields->size);
    gtk_label_set_text (GTK_LABEL (priv->duration_value_label),
                        fields->duration);
}

/* Toggle visibility of the small status icon if filename is read-only or not
 * found. Show the position of the current file in the list, by using the index
 * and list length. */
void
et_file_area_set_file_fields (EtFileArea *self,
                              const ET_File *ETFile)
{
    EtFileAreaPrivate *priv;
    GFile *file;
    gchar *text;
    const gchar *cur_filename;
    gchar *basename_utf8;
    gchar *pos;
    GFileInfo *info;
    GError *error = NULL;

    g_return_if_fail (ET_FILE_AREA (self));
    g_return_if_fail (ETFile != NULL);

    priv = et_file_area_get_instance_private (self);

    cur_filename = ((File_Name *)((GList *)ETFile->FileNameCur)->data)->value;

    file = g_file_new_for_path (cur_filename);

    info = g_file_query_info (file, G_FILE_ATTRIBUTE_ACCESS_CAN_READ ","
                              G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,
                              G_FILE_QUERY_INFO_NONE, NULL, &error);

    /* Show/hide 'AccessStatusIcon' */
    if (!info)
    {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
        {
            /* No such file or directory. */
            GIcon *emblem_icon;

            emblem_icon = g_themed_icon_new ("emblem-unreadable");

            gtk_entry_set_icon_from_gicon (GTK_ENTRY (priv->name_entry),
                                           GTK_ENTRY_ICON_SECONDARY,
                                           emblem_icon);
            gtk_entry_set_icon_tooltip_text (GTK_ENTRY (priv->name_entry),
                                             GTK_ENTRY_ICON_SECONDARY,
                                             _("File not found"));
            g_object_unref (emblem_icon);
        }
        else
        {
            Log_Print (LOG_ERROR, _("Cannot query file information ‘%s’"),
                       error->message);
            g_error_free (error);
            g_object_unref (file);
            return;
        }
    }
    else
    {
        gboolean readable, writable;

        readable = g_file_info_get_attribute_boolean (info,
                                                      G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
        writable = g_file_info_get_attribute_boolean (info,
                                                      G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE);

        if (readable && writable)
        {
            /* User has all necessary permissions. */
            gtk_entry_set_icon_from_gicon (GTK_ENTRY (priv->name_entry),
                                           GTK_ENTRY_ICON_SECONDARY, NULL);
        }
        else if (!writable)
        {
            /* Read only file or permission denied. */
            GIcon *emblem_icon;

            emblem_icon = g_themed_icon_new ("emblem-readonly");

            gtk_entry_set_icon_from_gicon (GTK_ENTRY (priv->name_entry),
                                           GTK_ENTRY_ICON_SECONDARY,
                                           emblem_icon);
            gtk_entry_set_icon_tooltip_text (GTK_ENTRY (priv->name_entry),
                                             GTK_ENTRY_ICON_SECONDARY,
                                             _("Read-only file"));
            g_object_unref (emblem_icon);

        }
        else
        {
            /* Otherwise unreadable. */
            GIcon *emblem_icon;

            emblem_icon = g_themed_icon_new ("emblem-unreadable");

            gtk_entry_set_icon_from_gicon (GTK_ENTRY (priv->name_entry),
                                           GTK_ENTRY_ICON_SECONDARY,
                                           emblem_icon);
            gtk_entry_set_icon_tooltip_text (GTK_ENTRY (priv->name_entry),
                                             GTK_ENTRY_ICON_SECONDARY,
                                             _("File not found"));
            g_object_unref (emblem_icon);
        }
        g_object_unref (info);
    }

    /* Set filename into name_entry. */
    basename_utf8 = g_path_get_basename (((File_Name *)((GList *)ETFile->FileNameNew)->data)->value_utf8);

    /* Remove the extension. */
    if ((pos = g_utf8_strrchr (basename_utf8, -1, '.')) != NULL)
    {
        *pos = 0;
    }

    gtk_entry_set_text (GTK_ENTRY (priv->name_entry), basename_utf8);
    g_free (basename_utf8);

    /* Show position of current file in list */
    text = g_strdup_printf ("%u/%u:", ETFile->IndexKey,
                            ETCore->ETFileDisplayedList_Length);
    gtk_label_set_text (GTK_LABEL (priv->index_label), text);
    g_object_unref (file);
    g_free (text);
}

const gchar *
et_file_area_get_filename (EtFileArea *self)
{
    EtFileAreaPrivate *priv;

    g_return_val_if_fail (ET_FILE_AREA (self), NULL);

    priv = et_file_area_get_instance_private (self);

    return gtk_entry_get_text (GTK_ENTRY (priv->name_entry));
}
