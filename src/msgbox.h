/* msgbox.h - 2000/10/13 */
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


#ifndef __MSGBOX_H__
#define __MSGBOX_H__


#include <gtk/gtk.h>


/*
enum Button_Type 
{     
    BUTTON_OK       = 1<<0 ,
    BUTTON_YES      = 1<<1 ,
    BUTTON_NO       = 1<<2 ,
    BUTTON_APPLY    = 1<<3 ,
    BUTTON_SAVE     = 1<<4 ,
    BUTTON_CANCEL   = 1<<5 ,
    BUTTON_CLOSE    = 1<<6 ,
    BUTTON_WRITE    = 1<<7 ,
    BUTTON_EXECUTE  = 1<<8 ,
    BUTTON_SEARCH   = 1<<9 ,
    BUTTON_BROWSE   = 1<<10
};

enum Message_Type 
{     
    MSG_INFO        = 1<<0,
    MSG_ERROR       = 1<<1,
    MSG_QUESTION    = 1<<2,
    MSG_WARNING     = 1<<3
};
*/


GtkWidget *msg_box_new (gchar *title, GtkWindow *parent, GtkWidget **check_button, GtkDialogFlags flags, gchar *message, const gchar *icon, ...);



#endif /* __MSGBOX_H__ */
