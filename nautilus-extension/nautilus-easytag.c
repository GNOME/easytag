/*
 *  Copyright (C) 2014 Victor A. Santos <victoraur.santos@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "nautilus-easytag.h"

#include <string.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <glib/gi18n-lib.h>
#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-file-info.h>
#include <libnautilus-extension/nautilus-menu-provider.h>

static GObjectClass *parent_class;

void
on_open_in_easytag (NautilusMenuItem *item,
                    gpointer data)
{
    NautilusFileInfo    *dir;
    GList               *uris = NULL;
    GdkDisplay          *display = gdk_display_get_default ();
    GDesktopAppInfo     *appinfo;
    GdkAppLaunchContext *context;

    dir = g_object_get_data (G_OBJECT (item), "dir");

    appinfo = g_desktop_app_info_new ("easytag.desktop");

    if (appinfo)
    {
        uris = g_list_append (uris, nautilus_file_info_get_uri (dir));

        context = gdk_display_get_app_launch_context (display);

        g_app_info_launch_uris (G_APP_INFO (appinfo), uris,
                                G_APP_LAUNCH_CONTEXT (context), NULL);
    }
}

void
on_open_with_easytag (NautilusMenuItem *item,
                      gpointer data)
{
    GList               *files, *scan;
    GList               *uris = NULL;
    GdkDisplay          *display = gdk_display_get_default ();
    GDesktopAppInfo     *appinfo;
    GdkAppLaunchContext *context;

    files = g_object_get_data (G_OBJECT (item), "files");

    appinfo = g_desktop_app_info_new ("easytag.desktop");

    if (appinfo)
    {
        for (scan = files; scan; scan = scan->next)
        {
            uris = g_list_append (uris,
                                  nautilus_file_info_get_uri (scan->data));
        }

        context = gdk_display_get_app_launch_context (display);

        g_app_info_launch_uris (G_APP_INFO (appinfo), uris,
                                G_APP_LAUNCH_CONTEXT (context), NULL);
    }
}

static struct
{
    gchar *mime_type;
    gboolean is_directory;
    gboolean is_file;
} easytag_mime_types[] =
{
    { "inode/directory", TRUE, FALSE },
    { "audio/x-mp3", FALSE, TRUE },
    { "audio/x-mpeg", FALSE, TRUE },
    { "audio/mpeg", FALSE, TRUE },
    { "application/ogg", FALSE, TRUE },
    { "audio/x-vorbis+ogg", FALSE, TRUE },
    { "audio/x-flac", FALSE, TRUE },
    { "audio/x-musepack", FALSE, TRUE },
    { "audio/x-ape", FALSE, TRUE },
    { "audio/x-speex+ogg", FALSE, TRUE },
    { "audio/x-opus+ogg", FALSE, TRUE },
    { NULL, FALSE }
};

typedef struct
{
    gboolean is_directory;
    gboolean is_file;
} FileMimeInfo;

static FileMimeInfo
get_file_mime_info (NautilusFileInfo *file)
{
    FileMimeInfo file_mime_info;
    gsize i;

    file_mime_info.is_directory = FALSE;
    file_mime_info.is_file = FALSE;

    for (i = 0; easytag_mime_types[i].mime_type != NULL; i++)
    {
        if (nautilus_file_info_is_mime_type (file,
                                             easytag_mime_types[i].mime_type))
        {
            file_mime_info.is_directory = easytag_mime_types[i].is_directory;
            file_mime_info.is_file = easytag_mime_types[i].is_file;

            return file_mime_info;
        }
    }

    return file_mime_info;
}

static gboolean
unsupported_scheme (NautilusFileInfo *file)
{
    gboolean result = FALSE;
    GFile *location;
    gchar *scheme;

    location = nautilus_file_info_get_location (file);
    scheme = g_file_get_uri_scheme (location);

    if (scheme != NULL)
    {
        const gchar *unsupported[] = { "trash", "computer", NULL };
        gsize i;

        for (i = 0; unsupported[i] != NULL; i++)
        {
            if (strcmp (scheme, unsupported[i]) == 0)
            {
                result = TRUE;
            }
        }
    }

    g_free (scheme);
    g_object_unref (location);

    return result;
}

static GList *
nautilus_easytag_get_file_items (NautilusMenuProvider *provider,
                                 GtkWidget *window,
                                 GList *files)
{
    GList *items = NULL;
    GList *scan;
    gboolean one_item;
    gboolean  one_directory = TRUE;
    gboolean  all_files = TRUE;

    if (files == NULL)
    {
        return NULL;
    }

    if (unsupported_scheme ((NautilusFileInfo *)files->data))
    {
        return NULL;
    }

    for (scan = files; scan; scan = scan->next)
    {
        NautilusFileInfo *file = scan->data;
        FileMimeInfo file_mime_info;

        file_mime_info = get_file_mime_info (file);

        if (all_files && !file_mime_info.is_file)
        {
            all_files = FALSE;
        }

        if (one_directory && !file_mime_info.is_directory)
        {
            one_directory = FALSE;
        }
    }

    one_item = (files != NULL) && (files->next == NULL);

    if (one_directory && one_item)
    {
        NautilusMenuItem *item;

        item = nautilus_menu_item_new ("NautilusEasyTag::open_directory",
                                       _("Open in EasyTAG"),
                                       _("Open the current selected directory in EasyTAG"),
                                       "easytag");
        g_signal_connect (item,
                          "activate",
                          G_CALLBACK (on_open_in_easytag),
                          provider);
        g_object_set_data (G_OBJECT (item),
                           "dir",
                           files->data);

        items = g_list_append (items, item);
    }
    else if (all_files)
    {
        NautilusMenuItem *item;

        item = nautilus_menu_item_new ("NautilusEasyTag::open_files",
                                       _("Open with EasyTAG"),
                                       _("Open selected files in EasyTAG"),
                                       "easytag");
        g_signal_connect (item,
                          "activate",
                          G_CALLBACK (on_open_with_easytag),
                          provider);
        g_object_set_data_full (G_OBJECT (item),
                                "files",
                                nautilus_file_info_list_copy (files),
                                (GDestroyNotify) nautilus_file_info_list_free);

        items = g_list_append (items, item);
    }

    return items;
}

static void
nautilus_easytag_menu_provider_iface_init (NautilusMenuProviderIface *iface)
{
    iface->get_file_items = nautilus_easytag_get_file_items;
}

static void
nautilus_easytag_instance_init (NautilusEasyTag *fr)
{
}

static void
nautilus_easytag_class_init (NautilusEasyTagClass *class)
{
    parent_class = g_type_class_peek_parent (class);
}

GType easytag_type = 0;

GType nautilus_easytag_get_type ()
{
    return easytag_type;
}

void nautilus_easytag_register_type (GTypeModule *module)
{
    static const GTypeInfo info = {
        sizeof (NautilusEasyTagClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) nautilus_easytag_class_init,
        NULL,
        NULL,
        sizeof (NautilusEasyTag),
        0,
        (GInstanceInitFunc) nautilus_easytag_instance_init,
    };

    static const GInterfaceInfo menu_provider_iface_info = {
        (GInterfaceInitFunc) nautilus_easytag_menu_provider_iface_init,
        NULL,
        NULL
    };

    easytag_type = g_type_module_register_type (module,
                                                G_TYPE_OBJECT,
                                                "NautilusEasyTag",
                                                &info, 0);

    g_type_module_add_interface (module,
                                 easytag_type,
                                 NAUTILUS_TYPE_MENU_PROVIDER,
                                 &menu_provider_iface_info);
}
