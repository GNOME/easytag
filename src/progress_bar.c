/* EasyTAG - Tag editor for audio files
 * Copyright (C) 2014  David King <amigadave@amigadave.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "progress_bar.h"

GtkWidget *
et_progress_bar_new (void)
{
    GtkWidget *progress;

    progress = gtk_progress_bar_new ();

    gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (progress), TRUE);
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), "");

    return progress;
}
