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

#include "application.h"

#include <glib/gi18n.h>
#include <stdlib.h>

G_DEFINE_TYPE (EtApplication, et_application, G_TYPE_APPLICATION)

struct _EtApplicationPrivate
{
    GtkWindow *main_window;
};

static void
init_i18n (void)
{
#if ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (PACKAGE_TARNAME, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */
}

static void
display_usage (void)
{
/* Fix from Steve Ralston for gcc-3.2.2 */
#ifdef G_OS_WIN32
#define xPREFIX "c:"
#else /* !G_OS_WIN32 */
#define xPREFIX ""
#endif /* !G_OS_WIN32 */

    /* FIXME: Translators should not have to deal with this! */
    g_print (_("\nUsage: easytag [option] "
               "\n   or: easytag [directory]\n"
               "\n"
               "Option:\n"
               "-------\n"
               "-h, --help        Display this text and exit.\n"
               "-v, --version     Print basic information and exit.\n"
               "\n"
               "Directory:\n"
               "----------\n"
               "%s/path_to/files  Use an absolute path to load,\n"
               "path_to/files     Use a relative path.\n"
               "\n"), xPREFIX);

#undef xPREFIX
}

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
            display_usage ();
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
et_application_finalize (GObject *object)
{
    G_OBJECT_CLASS (et_application_parent_class)->finalize (object);
}

static void
et_application_init (EtApplication *application)
{
    application->priv = G_TYPE_INSTANCE_GET_PRIVATE (application,
                                                     ET_TYPE_APPLICATION,
                                                     EtApplicationPrivate);

    application->priv->main_window = NULL;
}

static void
et_application_class_init (EtApplicationClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = et_application_finalize;
    G_APPLICATION_CLASS (klass)->local_command_line = et_local_command_line;

    g_type_class_add_private (klass, sizeof (EtApplicationPrivate));
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
    init_i18n ();
#if !GLIB_CHECK_VERSION (2, 35, 1)
    g_type_init ();
#endif /* !GLIB_CHECK_VERSION (2, 35, 1) */

    return g_object_new (et_application_get_type (), "application-id",
                         "org.gnome.EasyTAG", "flags",
                         G_APPLICATION_HANDLES_OPEN, NULL);
}

/*
 * et_application_get_window:
 * @application: the application
 *
 * Get the current application window.
 *
 * Returns: the current application window, or %NULL if no window is set
 */
GtkWindow *
et_application_get_window (EtApplication *application)
{
    g_return_val_if_fail (ET_APPLICATION (application), NULL);

    return application->priv->main_window;
}

/*
 * et_application_set_window:
 * @application: the application
 * @window: the window to set
 *
 * Set the application window, if none has been set already.
 */
void
et_application_set_window (EtApplication *application, GtkWindow *window)
{
    g_return_if_fail (ET_APPLICATION (application) || GTK_WINDOW (window)
                      || application->priv->main_window != NULL);

    application->priv->main_window = window;
}
