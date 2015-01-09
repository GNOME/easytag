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

#include <glib/gi18n.h>
#include <stdlib.h>

#include "about.h"
#include "charset.h"
#include "easytag.h"
#include "log.h"
#include "misc.h"
#include "setting.h"

typedef struct
{
    guint idle_handler;
    gchar *init_directory;
} EtApplicationPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (EtApplication, et_application, GTK_TYPE_APPLICATION)

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
    GList *windows;

    windows = gtk_application_get_windows (GTK_APPLICATION (user_data));

    et_show_about_dialog (windows->data);
}

static void
on_quit (GSimpleAction *action,
         GVariant *parameter,
         gpointer user_data)
{
    GList *windows;

    windows = gtk_application_get_windows (GTK_APPLICATION (user_data));

    et_application_window_quit (windows->data);
}

static const GActionEntry actions[] =
{
    { "help", on_help },
    { "about", on_about },
    { "quit", on_quit }
};

/*
 * Load the default directory when the user interface is completely displayed
 * to avoid bad visualization effect at startup.
 */
static gboolean
on_idle_init (EtApplication *self)
{
    EtApplicationPrivate *priv;

    priv = et_application_get_instance_private (self);

    ET_Core_Free ();
    ET_Core_Create ();

    if (g_settings_get_boolean (MainSettings, "scan-startup"))
    {
        g_action_group_activate_action (G_ACTION_GROUP (MainWindow), "scanner",
                                        NULL);
    }

    if (priv->init_directory)
    {
        et_application_window_select_dir (ET_APPLICATION_WINDOW (MainWindow),
                                          priv->init_directory);
    }
    else
    {
        et_application_window_status_bar_message (ET_APPLICATION_WINDOW (MainWindow),
                                                  _("Select a directory to browse"),
                                                  FALSE);
        g_action_group_activate_action (G_ACTION_GROUP (MainWindow),
                                        "go-default", NULL);
    }

    /* Set sensitivity of buttons if the default directory is invalid. */
    et_application_window_update_actions (ET_APPLICATION_WINDOW (MainWindow));

    priv->idle_handler = 0;

    return G_SOURCE_REMOVE;
}

/*
 * common_init:
 * @application: the application
 *
 * Create and show the main window. Common to all actions which may occur after
 * startup, such as "activate" and "open".
 */
static void
common_init (EtApplication *self)
{
    EtApplicationPrivate *priv;
    gboolean settings_warning;
    EtApplicationWindow *window;

    priv = et_application_get_instance_private (self);

    /* Create all config files. */
    settings_warning = !Setting_Create_Files ();

    /* Load Config */
    Init_Config_Variables ();

    /* Initialization */
    ET_Core_Create ();
    Main_Stop_Button_Pressed = FALSE;

    /* The main window */
    window = et_application_window_new (GTK_APPLICATION (self));
    MainWindow = GTK_WIDGET (window);

    gtk_widget_show (MainWindow);

    /* Starting messages */
    Log_Print (LOG_OK, _("Starting EasyTAG version %s…"), PACKAGE_VERSION);
#ifdef G_OS_WIN32
    if (g_getenv("EASYTAGLANG"))
    {
        Log_Print (LOG_OK,
                   _("Variable EASYTAGLANG defined, setting locale ‘%s’"),
                   g_getenv ("EASYTAGLANG"));
    }
    else
    {
        Log_Print (LOG_OK, _("Setting locale: ‘%s’"), g_getenv ("LANG"));
    }
#endif /* G_OS_WIN32 */

    if (get_locale ())
        Log_Print (LOG_OK,
                   _("System locale is ‘%s’, using ‘%s’"),
                   get_locale (), get_encoding_from_locale (get_locale ()));

    if (settings_warning)
    {
        Log_Print (LOG_WARNING, _("Unable to create setting directories"));
    }

    /* Load the default dir when the UI is created and displayed
     * to the screen and open also the scanner window */
    priv->idle_handler = g_idle_add ((GSourceFunc)on_idle_init, self);
    g_source_set_name_by_id (priv->idle_handler, "Init idle function");
}

/*
 * check_for_hidden_path_in_tree:
 * @arg: the path to check
 *
 * Recursively check for a hidden path in the directory tree given by @arg. If
 * a hidden path is found, set browse-show-hidden to TRUE.
 */
static void
check_for_hidden_path_in_tree (GFile *arg)
{
    GFile *file = NULL;
    GFile *parent;
    GFileInfo *info;
    GError *err = NULL;

    /* Not really the parent until an iteration through the loop below. */
    parent = g_file_dup (arg);

    do
    {
        info = g_file_query_info (parent,
                                  G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN,
                                  G_FILE_QUERY_INFO_NONE, NULL, &err);

        if (info == NULL)
        {
            g_message ("Error querying file information (%s)", err->message);
            g_clear_error (&err);

            if (file)
            {
                g_clear_object (&file);
            }
            g_object_unref (parent);
            break;
        }
        else
        {
            if (g_file_info_get_is_hidden (info))
            {
                g_settings_set_boolean (MainSettings, "browse-show-hidden",
                                        TRUE);
            }
        }

        g_object_unref (info);

        if (file)
        {
            g_clear_object (&file);
        }

        file = parent;
    }
    while ((parent = g_file_get_parent (file)) != NULL);

    g_clear_object (&file);
}

/*
 * et_application_activate:
 * @application: the application
 *
 * Handle the application being activated, which occurs after startup and if
 * no files are opened.
 */
static void
et_application_activate (GApplication *application)
{
    GList *windows;

    windows = gtk_application_get_windows (GTK_APPLICATION (application));

    if (windows != NULL)
    {
        gtk_window_present (windows->data);
    }
    else
    {
        common_init (ET_APPLICATION (application));
    }
}

/*
 * et_application_local_command_line:
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
et_application_local_command_line (GApplication *application,
                                   gchar **arguments[],
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

    g_debug ("Received %u commandline arguments", n_args);

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
            g_print (PACKAGE_TARNAME " " PACKAGE_VERSION "\n");
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

/*
 * et_application_open:
 * @application: the application
 * @files: array of files to open
 * @n_files: the number of files
 * @hint: hint of method to open files, currently empty
 *
 * Handle the files passed to the primary instance.
 *
 * Returns: the exit status to be passed to the calling process
 */
static void
et_application_open (GApplication *self,
                     GFile **files,
                     gint n_files,
                     const gchar *hint)
{
    EtApplicationPrivate *priv;
    GList *windows;
    gboolean activated;
    GFile *arg;
    GFile *parent;
    GFileInfo *info;
    GError *err = NULL;
    GFileType type;
    gchar *path;
    gchar *path_utf8;

    priv = et_application_get_instance_private (ET_APPLICATION (self));

    windows = gtk_application_get_windows (GTK_APPLICATION (self));
    activated = windows ? TRUE : FALSE;

    /* Only take the first file; ignore the rest. */
    arg = files[0];

    check_for_hidden_path_in_tree (arg);

    path = g_file_get_path (arg);
    path_utf8 = filename_to_display (path);
    info = g_file_query_info (arg, G_FILE_ATTRIBUTE_STANDARD_TYPE,
                              G_FILE_QUERY_INFO_NONE, NULL, &err);

    if (info == NULL)
    {
        if (activated)
        {
            Log_Print (LOG_ERROR, _("Error while querying information for file ‘%s’: %s"),
                       path_utf8, err->message);

        }
        else
        {
            g_warning ("Error while querying information for file: '%s' (%s)",
                       path_utf8, err->message);
        }

        g_free (path);
        g_free (path_utf8);
        g_error_free (err);
        return;
    }

    type = g_file_info_get_file_type (info);

    if (type == G_FILE_TYPE_DIRECTORY)
    {
        if (activated)
        {
            et_application_window_select_dir (windows->data, path);
            g_free (path);
        }
        else
        {
            priv->init_directory = path;
        }

        g_free (path_utf8);
        g_object_unref (info);
    }
    else if (type == G_FILE_TYPE_REGULAR)
    {
        /* When given a file, load the parent directory. */
        parent = g_file_get_parent (arg);

        if (parent)
        {
            g_free (path_utf8);
            g_free (path);

            if (activated)
            {
                gchar *parent_path;

                parent_path = g_file_get_path (arg);
                et_application_window_select_dir (windows->data,
                                                  parent_path);

                g_free (parent_path);
            }
            else
            {
                priv->init_directory = g_file_get_path (parent);
            }

            g_object_unref (parent);
            g_object_unref (info);
        }
        else
        {
            Log_Print (LOG_WARNING, _("Cannot open path ‘%s’"), path_utf8);
            g_free (path);
            g_free (path_utf8);
            return;
        }
    }
    else
    {
        Log_Print (LOG_WARNING, _("Cannot open path ‘%s’"), path_utf8);
        g_free (path);
        g_free (path_utf8);
        return;
    }

    if (!activated)
    {
        common_init (ET_APPLICATION (self));
    }
}

/*
 * et_application_shutdown:
 * @application: the application
 * @user_data: user data set when the signal handler was connected
 *
 * Handle the application being shutdown, which occurs after the main loop has
 * exited and before returning from g_application_run().
 */
static void
et_application_shutdown (GApplication *application)
{
    Charset_Insert_Locales_Destroy ();

    G_APPLICATION_CLASS (et_application_parent_class)->shutdown (application);
}

static void
et_application_startup (GApplication *application)
{
    GtkBuilder *builder;
    GError *error = NULL;
    GMenuModel *appmenu;
    GMenuModel *menubar;

    g_action_map_add_action_entries (G_ACTION_MAP (application), actions,
                                     G_N_ELEMENTS (actions), application);

    G_APPLICATION_CLASS (et_application_parent_class)->startup (application);

    /* gtk_init() calls setlocale(), so gettext must be called after that. */
    g_set_application_name (_(PACKAGE_NAME));
    builder = gtk_builder_new ();
    gtk_builder_add_from_resource (builder, "/org/gnome/EasyTAG/menus.ui",
                                   &error);

    if (error != NULL)
    {
        g_error ("Unable to get app menu from resource: %s", error->message);
    }

    appmenu = G_MENU_MODEL (gtk_builder_get_object (builder, "app-menu"));
    gtk_application_set_app_menu (GTK_APPLICATION (application), appmenu);

    menubar = G_MENU_MODEL (gtk_builder_get_object (builder, "menubar"));
    gtk_application_set_menubar (GTK_APPLICATION (application), menubar);

    g_object_unref (builder);

    Charset_Insert_Locales_Init ();
}

static void
et_application_dispose (GObject *object)
{
    EtApplication *self;
    EtApplicationPrivate *priv;

    self = ET_APPLICATION (object);
    priv = et_application_get_instance_private (self);

    if (priv->idle_handler)
    {
        g_source_remove (priv->idle_handler);
        priv->idle_handler = 0;
    }
}

static void
et_application_finalize (GObject *object)
{
    EtApplication *self;
    EtApplicationPrivate *priv;

    self = ET_APPLICATION (object);
    priv = et_application_get_instance_private (self);

    g_free (priv->init_directory);

    G_OBJECT_CLASS (et_application_parent_class)->finalize (object);
}

static void
et_application_init (EtApplication *self)
{
}

static void
et_application_class_init (EtApplicationClass *klass)
{
    GObjectClass *gobject_class;
    GApplicationClass *gapplication_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gapplication_class = G_APPLICATION_CLASS (klass);

    gobject_class->dispose = et_application_dispose;
    gobject_class->finalize = et_application_finalize;
    gapplication_class->activate = et_application_activate;
    gapplication_class->local_command_line = et_application_local_command_line;
    gapplication_class->open = et_application_open;
    gapplication_class->shutdown = et_application_shutdown;
    gapplication_class->startup = et_application_startup;
}

/*
 * et_application_new:
 *
 * Create a new EtApplication instance and initialise internationalization.
 *
 * Returns: a new #EtApplication
 */
EtApplication *
et_application_new (void)
{
    return g_object_new (ET_TYPE_APPLICATION, "application-id",
                         "org.gnome.EasyTAG", "flags",
                         G_APPLICATION_HANDLES_OPEN, NULL);
}
