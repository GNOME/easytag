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

#include "application.h"

#include "about.h"
#include "charset.h"
#include "easytag.h"

#include <glib/gi18n.h>
#include <stdlib.h>

G_DEFINE_TYPE (EtApplication, et_application, GTK_TYPE_APPLICATION)

static const GOptionEntry entries[] =
{
    { "version", 'v', 0, G_OPTION_ARG_NONE, NULL,
      N_("Print the version and exit"), NULL },
    { NULL }
};

static void
on_help (GSimpleAction *action,
         GVariant *parameter,
         gpointer user_data)
{
/* TODO: Link to help.gnome.org, or locally-installed help, on Windows. */
#ifndef G_OS_WIN32
    GError *error = NULL;

    gtk_show_uri (gtk_window_get_screen (GTK_WINDOW (MainWindow)),
                  "help:easytag", GDK_CURRENT_TIME, &error);

    if (error)
    {
        g_debug ("Error while opening help: %s", error->message);
        g_error_free (error);
    }
#endif
}

static void
on_about (GSimpleAction *action,
          GVariant *parameter,
          gpointer user_data)
{
    Show_About_Window ();
}

static void
on_quit (GSimpleAction *action,
         GVariant *parameter,
         gpointer user_data)
{
    Quit_MainWindow ();
}

static const GActionEntry actions[] =
{
    { "help", on_help },
    { "about", on_about },
    { "quit", on_quit }
};

/*
 * et_local_command_line:
 * @application: the application
 * @arguments: pointer to the argument string array
 * @exit_status: pointer to the returned exit status
 *
 * Parse the local instance command-line arguments.
 *
 * Returns: %TRUE to indicate that the command-line arguments were completely
 * handled in the local instance
 */
static gboolean
et_local_command_line (GApplication *application, gchar **arguments[],
                       gint *exit_status)
{
    GError *error = NULL;
    guint n_args;
    gchar **argv;

    /* Try to register. */
    if (!g_application_register (application, NULL, &error))
    {
        g_critical ("Error registering EtApplication: %s", error->message);
        g_error_free (error);
        *exit_status = 1;
        return TRUE;
    }

    argv = *arguments;
    n_args = g_strv_length (argv);
    *exit_status = 0;

    g_debug ("Received %d commandline arguments", n_args);

    if (n_args <= 1)
    {
        g_application_activate (application);
        return TRUE;
    }
    else
    {
        const gsize i = 1;

        /* Exit the local instance for --help and --version. */
        if ((strcmp (argv[i], "--version") == 0)
            || (strcmp (argv[i], "-v") == 0))
        {
            g_print (PACKAGE_NAME " " PACKAGE_VERSION "\n");
            g_print (_("Website: %s"), PACKAGE_URL "\n");
            exit (0);
        }
        else if ((strcmp (argv[i], "--help") == 0)
                 || (strcmp (argv[i], "-h") == 0))
        {
            GOptionContext *context;
            gchar *help;

            context = g_option_context_new (_("- Tag and rename audio files"));
            g_option_context_add_main_entries (context, entries,
                                               GETTEXT_PACKAGE);
            help = g_option_context_get_help (context, TRUE, NULL);
            g_print ("%s", help);
            exit (0);
        }
        else
        {
            /* Assume a filename otherwise, and allow the primary instance to
             * handle it. */
            GFile **files;
            gsize n_files;
            gsize j;

            n_files = n_args - 1;
            files = g_new (GFile *, n_files);

            for (j = 0; j < n_files; j++)
            {
                files[j] = g_file_new_for_commandline_arg (argv[j + 1]);
            }

            g_application_open (application, files, n_files, "");

            for (j = 0; j < n_files; j++)
            {
                g_object_unref (files[j]);
            }

            g_free (files);
        }
    }

    return TRUE;
}

static void
et_application_startup (GApplication *application)
{
    GtkBuilder *builder;
    GError *error = NULL;
    GMenuModel *appmenu;

    g_action_map_add_action_entries (G_ACTION_MAP (application), actions,
                                     G_N_ELEMENTS (actions), application);

    G_APPLICATION_CLASS (et_application_parent_class)->startup (application);

    builder = gtk_builder_new ();
    gtk_builder_add_from_resource (builder, "/org/gnome/EasyTAG/menus.ui",
                                   &error);

    if (error != NULL)
    {
        g_error ("Unable to get app menu from resource: %s", error->message);
    }

    appmenu = G_MENU_MODEL (gtk_builder_get_object (builder, "app-menu"));
    gtk_application_set_app_menu (GTK_APPLICATION (application), appmenu);

    g_object_unref (builder);

    Charset_Insert_Locales_Init ();
}

static void
et_application_finalize (GObject *object)
{
    G_OBJECT_CLASS (et_application_parent_class)->finalize (object);
}

static void
et_application_init (EtApplication *application)
{
}

static void
et_application_class_init (EtApplicationClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = et_application_finalize;
    G_APPLICATION_CLASS (klass)->local_command_line = et_local_command_line;
    G_APPLICATION_CLASS (klass)->startup = et_application_startup;
}

/*
 * et_application_new:
 *
 * Create a new EtApplication instance and initialise internationalization.
 *
 * Returns: a new #EtApplication
 */
EtApplication *
et_application_new ()
{
    return g_object_new (ET_TYPE_APPLICATION, "application-id",
                         "org.gnome.EasyTAG", "flags",
                         G_APPLICATION_HANDLES_OPEN, NULL);
}
