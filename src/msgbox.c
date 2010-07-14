/* msgbox.c - 2000/10/13 */
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
/* Note: Some pieces of codes come from gnome-dialog.c
 */

#include <config.h>

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <stdarg.h>

#include "msgbox.h"
#include "easytag.h"
#include "misc.h"
#include "setting.h"


GtkWidget *msg_box_new (gchar *title, GtkWindow *parent, GtkWidget **check_button, GtkDialogFlags flags, gchar *message, const gchar *icon, ...);



/*
 * Create a new message box to display a message and a check button (for some actions).
 * The first button passed as parameter will have the focus
 */
GtkWidget *msg_box_new (gchar *title, 
                        GtkWindow *parent,
                        GtkWidget **check_button, 
                        GtkDialogFlags flags,
                        gchar *message,
                        const gchar *stock_id,
                        /* Put all the buttons to display, and terminate by 0 */
                        ...)
{
    GtkWidget *dialog;
    GtkWidget *Table;
    GtkWidget *Pixmap;
    GtkWidget *Label;
    GtkWidget *Button;
    gboolean first_button = TRUE;
    va_list args;
    

    g_return_val_if_fail(message!=NULL, NULL);
    
    //dialog = GTK_DIALOG(gtk_dialog_new_empty (title, parent, flags));
    dialog = gtk_dialog_new_with_buttons(title,parent,flags,NULL);

    // Position of the dialog window
    if (MESSAGE_BOX_POSITION_NONE)
        gtk_window_set_position(GTK_WINDOW(dialog),GTK_WIN_POS_NONE);
    else if (MESSAGE_BOX_POSITION_CENTER)
        gtk_window_set_position(GTK_WINDOW(dialog),GTK_WIN_POS_CENTER);
    else if (MESSAGE_BOX_POSITION_MOUSE)
        gtk_window_set_position(GTK_WINDOW(dialog),GTK_WIN_POS_MOUSE);
    else if (MESSAGE_BOX_POSITION_CENTER_ON_PARENT)
        gtk_window_set_position(GTK_WINDOW(dialog),GTK_WIN_POS_CENTER_ON_PARENT);
    

    // Table to insert: the pixmap, the message and the check button
    Table = gtk_table_new(2,2,FALSE);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),Table,TRUE,TRUE,0);
    gtk_container_set_border_width(GTK_CONTAINER(Table),4);
    gtk_widget_show(Table);

    // The pixmap
    Pixmap = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_DIALOG);
    gtk_table_attach_defaults(GTK_TABLE(Table),Pixmap,0,1,0,1);
    //gtk_table_attach(GTK_TABLE(Table),Pixmap,0,1,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_misc_set_padding(GTK_MISC(Pixmap),6,6);
    gtk_widget_show(Pixmap);

    // The message
    Label = gtk_label_new(message);
    gtk_table_attach_defaults(GTK_TABLE(Table),Label,1,2,0,1);
    gtk_misc_set_padding(GTK_MISC(Label),6,6);
    gtk_label_set_justify(GTK_LABEL(Label),GTK_JUSTIFY_CENTER);
    //gtk_label_set_line_wrap(GTK_LABEL(Label),TRUE);
    gtk_widget_show(Label);

    // The check button
    if (check_button)
    {
        *check_button = gtk_check_button_new_with_label(_("Repeat action for the rest of the files")); // Can save or cancel all files
        gtk_table_attach_defaults(GTK_TABLE(Table),*check_button,0,2,1,2);
    }

    // Read buttons from variable arguments
    va_start(args, stock_id);
    for (;;)
    {
        const gchar *button_text = NULL;
        gint response_id = 0;
        
        button_text = va_arg (args, const gchar *);
        if (button_text == NULL)
            break;
        response_id = va_arg (args, gint);
        if (response_id == 0)
            break;
        
        Button = gtk_dialog_add_button(GTK_DIALOG(dialog),button_text,response_id);
        GTK_WIDGET_SET_FLAGS(Button,GTK_CAN_DEFAULT);
        
        // To focus to the first button
        if (first_button)
            gtk_widget_grab_default(Button);
        first_button = FALSE;
    }
    va_end(args);
    
    gtk_widget_show_all(dialog);
    
    return GTK_WIDGET(dialog);
}
