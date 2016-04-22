/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014-2015  David King <amigadave@amigadave.com>
 * Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
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

#include "about.h"

#include <glib/gi18n.h>

void
et_show_about_dialog (GtkWindow *parent)
{
    static const gchar * const artists[] =
    {
        "Ekaterina Gerasimova <kittykat3756@gmail.com>",
        "Waqas Qamar <wakas88@gmail.com>",
        "Der Humph <humph@gmx.de>",
        NULL
    };

    static const gchar * const authors[] =
    {
        "David King <amigadave@amigadave.com>",
        "Kip Warner <kip@thevertigo.com>",
        "Santtu Lakkala <inz@inz.fi>",
        "Jérôme Couderc <easytag@gmail.com>",
        "Daniel Drake <dsd@gentoo.org>",
        "Avhinav Jangda <abhijangda@hotmail.com>",
        "Mihael Vrbanec <miqster@gmx.net>",
        "Michael Pujos <pujos.michael@laposte.net>",
        "Goetz Waschk <waschk@informatik.uni-rostock.de>",
        "Holger Schemel <aeglos@valinor.owl.de>",
        "Charles Kerr <charles@superimp.org>",
        "Maciej Kasprzyk <kasprzol@go2.pl>",
        "Artur Polaczynski <artii@o2.pl>",
        "Philipp Thomas <pthomas@suse.de>",
        "Oliver <oliver@are-b.org>",
        "Tony Mancill <tony@mancill.com>",
        "Pavel Minayev <thalion@front.ru>",
        "Justus Schwartz <justus@gmx.li>",
        "Fredrik Noring <noring@nocrew.org>",
        "Paul Giordano <giordano@covad.net>",
        "Michael Ihde <mike.ihde@randomwalking.com>",
        "Stewart Whitman <swhitman@cox.net>",
        "Javier Kohen <jkohen@users.sourceforge.net>",
        "FutureFog <futurefog@katamail.com>",
        "Maarten Maathuis <madman2003@gmail.com>",
        "Pierre Dumuid <pierre.dumuid@adelaide.edu.au>",
        "Nick Lanham <nick@afternight.org>",
        "Wojciech Wierchola <admin@webcarrot.pl>",
        "Julian Taylor <jtaylor@ubuntu.com>",
        "Honore Doktorr <hdfssk@gmail.com>",
        "Guilherme Destefani <gd@helixbrasil.com.br>",
        "Andreas Winkelmann <ml@awinkelmann.de>",
        "Adrian Bunk <bunk@stusta.de>",
        NULL
    };

    static const gchar * const documenters[] =
    {
        "Ekaterina Gerasimova <kittykat3756@gmail.com>",
        NULL
    };

    static const gchar copyright[] = "Copyright © 2012–2016 David King\n"
                                     "Copyright © 2009–2012 Kip Warner\n"
                                     "Copyright © 2000–2008 Jérôme Couderc";

    const gchar *translators;

    /* Translators: put your own name here to appear in the about dialog. */
    translators = _("translator-credits");

    if (!strcmp (translators,"translator-credits"))
    {
        translators = NULL;
    }

    gtk_show_about_dialog (parent,
                           "artists", artists,
                           "authors", authors,
                           "comments", _("View and edit tags in audio files"),
                           "copyright", copyright,
                           "documenters", documenters,
                           "license-type", GTK_LICENSE_GPL_2_0,
                           "logo-icon-name", "org.gnome.EasyTAG",
                           "translator-credits", translators,
                           "version", PACKAGE_VERSION,
                           "website", PACKAGE_URL,
                           NULL);
}
