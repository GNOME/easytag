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


#include <gtk/gtkdialog.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define TYPE_MSG_BOX            (msg_box_get_type())
#define MSG_BOX(obj)            (GTK_CHECK_CAST((obj), TYPE_MSG_BOX, MsgBox))
#define MSG_BOX_CLASS(klass)    (GTK_CHECK_CLASS_CAST((klass), TYPE_MSG_BOX, MsgBoxClass))
#define IS_MSG_BOX(obj)         (GTK_CHECK_TYPE((obj), TYPE_MSG_BOX))
#define IS_MSG_BOX_CLASS(klass) (GTK_CHECK_CLASS_TYPE((klass), TYPE_MSG_BOX))


typedef struct _MsgBox      MsgBox;
typedef struct _MsgBoxClass MsgBoxClass;

struct _MsgBox
{
    GtkDialog dialog;

    gint icon;

    /* Check button */
    GtkWidget *check_button;
    gint       check_button_state;

    /* To know which button have been pressed */
    gint button_clicked;
};

struct _MsgBoxClass
{
    GtkDialogClass parent_class;
};


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


GType msg_box_get_type(void);

GtkWidget *msg_box_new (gchar *title, gchar *message, const gchar *icon, ...);
gint       msg_box_run (MsgBox *msgbox);
void       msg_box_destroy (GtkObject *object);
void       msg_box_check_button_set_active (MsgBox *msgbox, gboolean is_active);
gboolean   msg_box_check_button_get_active (MsgBox *msgbox);
void       msg_box_hide_check_button (MsgBox *msgbox);


#ifdef __cplusplus
};
#endif /* __cplusplus */


#endif /* __MSGBOX_H__ */
