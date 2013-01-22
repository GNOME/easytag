/* about.c - 2000/05/05 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "about.h"
#include "easytag.h"

void Show_About_Window (void)
{
    static const gchar * const artists[] =
    {
        "Waqas Qamar <wakas88@gmail.com>",
        "Der Humph <humph@gmx.de>",
        NULL
    };

    static const gchar * const authors[] =
    {
        "David King <amigadave@amigadave.com>",
        "Kip Warner <kip@thevertigo.com>",
        "Santtu Lakkala <inzane@ircing.org>",
        "Jérôme Couderc <easytag@gmail.com>",
        "Daniel Drake <dsd@gentoo.org>",
        "Mihael Vrbanec <miqster@gmx.net>",
        "Michael Pujos <pujos.michael@laposte.net>",
        "Adrian Bunk <bunk@fs.tum.de>",
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
        "Emmanuel Brun <brunema@wanadoo.fr>",
        "Daniel Pichler <dpichler@dnet.it>",
        "Anders Strömer <anders.stromer@comhem.se>",
        NULL
    };

    static const gchar copyright[] = "Copyright © 2012–2013 David King\n"
                                     "Copyright © 2009–2012 Kip Warner\n"
                                     "Copyright © 2000–2008 Jérôme Couderc";

    const gchar *license;
    const gchar *translators;

    license =  _("This program is free software; you can redistribute it "
                 "and/or modify it under the terms of the GNU General Public "
                 "License as published by the Free Software Foundation; "
                 "either version 2 of the License, or (at your option) any "
                 "later version.\n"
                 "\n"
                 "This program is distributed in the hope that it will be "
                 "useful, but WITHOUT ANY WARRANTY; without even the implied "
                 "warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR "
                 "PURPOSE.  See the GNU General Public License for more "
                 "details.\n"
                 "\n"
                 "You should have received a copy of the GNU General Public "
                 "License along with this program. If not, see "
                 "<http://www.gnu.org/licenses/>.");

    /* Translators: put your own name here to appear in the about dialog. */
    translators = _("translator-credits");

    if (!strcmp (translators,"translator-credits"))
    {
        translators = NULL;
    }

    gtk_show_about_dialog (GTK_WINDOW (MainWindow),
                           "artists", artists,
                           "authors", authors,
                           "comments", _("View and edit tags in audio files"),
                           "copyright", copyright,
                           "documenters", documenters,
                           "license", license,
                           "logo-icon-name", PACKAGE_TARNAME,
                           "program-name", PACKAGE_NAME,
                           "translator-credits", translators,
                           "version", PACKAGE_VERSION,
                           "website", PACKAGE_URL,
                           "wrap-license", TRUE,
                           NULL);
}
