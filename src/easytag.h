/* easytag.h - 2000/04/28 */
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#ifndef __EASYTAG_H__
#define __EASYTAG_H__


/* 'include' and 'define' created by autoconf/automake */
#include "config.h"

#include "et_core.h"


#define MAX_STRING_LEN     1024


/***************
 * Declaration *
 ***************/

/* Variable to force to quit recursive functions (reading dirs) or stop saving files */
gboolean Main_Stop_Button_Pressed;

GtkWidget *MainWindow;

#ifndef errno
extern int errno;
#endif

/* A flag to start/avoid a new reading while another one is running */
gboolean ReadingDirectory;


/**************
 * Prototypes *
 **************/
void Action_Save_Selected_Files         (void);
void Action_Force_Saving_Selected_Files (void);
gint Save_All_Files_With_Answer         (gboolean force_saving_files);

void Action_Main_Stop_Button_Pressed    (void);

gboolean Read_Directory (const gchar *path);

#endif /* __EASYTAG_H__ */
