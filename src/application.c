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
    gpointer *data;
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
 * Returns: %FALSE to indicate that the command-line arguments were not
 *          completely handled in the local instance, or does not return
 */
static gboolean
et_local_command_line (GApplication *application, gchar **arguments[],
                       gint *exit_status)
{
    gint i;
    gchar **argv;

    argv = *arguments;
    *exit_status = 0;
    i = 1;

    while (argv[i])
    {
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
            i++;
        }
    }

    return FALSE;
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
                         G_APPLICATION_FLAGS_NONE, NULL);
}
