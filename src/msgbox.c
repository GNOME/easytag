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


static void msg_box_class_init (MsgBoxClass *class);
static void msg_box_init (MsgBox *mb);

static void msg_box_show (GtkWidget *widget);
static gint msg_box_delete_event (GtkWidget *widget, GdkEventAny *event);
static void msg_box_button_clicked (GtkButton *button, gpointer data);
void        check_button_toggled (GtkCheckButton *checkbutton, gpointer data);


static GtkDialogClass *parent_klass = NULL;


GType msg_box_get_type(void)
{
    static GType mb_type = 0;

    if (!mb_type)
    {
        GTypeInfo mb_info =
        {
            sizeof(MsgBoxClass),
            NULL,
            NULL,
            (GClassInitFunc) msg_box_class_init,
            NULL,
            NULL,
            sizeof(MsgBox),
            0,
            (GInstanceInitFunc) msg_box_init
        };
        mb_type = g_type_register_static(GTK_TYPE_DIALOG, "MsgBox", &mb_info, 0);
    }
    return mb_type;
}


static void msg_box_button_clicked (GtkButton *button, gpointer data)
{
    MsgBox *mb;

    g_return_if_fail( (mb = MSG_BOX(data)) );

    mb->button_clicked = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button),"button_value"));

    if (gtk_main_level()>1)
        gtk_main_quit();
}


static void msg_box_class_init (MsgBoxClass *class)
{
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;
    GtkDialogClass *dialog_class;

    object_class = (GtkObjectClass*) class;
    widget_class = (GtkWidgetClass*) class;
    dialog_class = (GtkDialogClass*) class;

    parent_klass = gtk_type_class(gtk_dialog_get_type());

    object_class->destroy      = msg_box_destroy;
    widget_class->delete_event = msg_box_delete_event;
    widget_class->show         = msg_box_show;
}


static void msg_box_init (MsgBox *mb)
{
    g_return_if_fail(mb != NULL);
    g_return_if_fail(IS_MSG_BOX(mb));

    mb->icon                = -1;
    mb->check_button        = NULL;
    mb->check_button_state  = 0;

}


/*
 * Create a new message box
 */
GtkWidget *msg_box_new (gchar *title, gchar *message, const gchar *stock_id,
            /* Put all the buttons to display, and terminate by 0 */
            ...)
{
    MsgBox *mb;
    GtkWidget *Table;
    GtkWidget *Pixmap;
    GtkWidget *Label;
    GtkWidget *ButtonBox;
    GtkWidget *Button = NULL;
    va_list cursor;
    gint cursor_value;


    g_return_val_if_fail(message!=NULL, NULL);

    mb = MSG_BOX(g_object_new(TYPE_MSG_BOX, NULL));
    //mb->icon = icon;
    mb->check_button = gtk_check_button_new_with_label(_("Repeat action for the rest of the files")); /* Can save or cancel all files */

    /* Window configuration */
    gtk_window_set_title(GTK_WINDOW(mb),title);
    gtk_window_set_transient_for(GTK_WINDOW(mb),GTK_WINDOW(MainWindow));
    gtk_window_set_modal(GTK_WINDOW(mb),TRUE);

    if (MESSAGE_BOX_POSITION_NONE)
        gtk_window_set_position(GTK_WINDOW(mb),GTK_WIN_POS_NONE);
    else if (MESSAGE_BOX_POSITION_CENTER)
        gtk_window_set_position(GTK_WINDOW(mb),GTK_WIN_POS_CENTER);
    else if (MESSAGE_BOX_POSITION_MOUSE)
        gtk_window_set_position(GTK_WINDOW(mb),GTK_WIN_POS_MOUSE);
    else if (MESSAGE_BOX_POSITION_CENTER_ON_PARENT)
        gtk_window_set_position(GTK_WINDOW(mb),GTK_WIN_POS_CENTER_ON_PARENT);


    /* Table to put: the pixmap, the message and the check button */
    Table = gtk_table_new(2,2,FALSE);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(mb)->vbox),Table,TRUE,TRUE,0);
    gtk_container_set_border_width(GTK_CONTAINER(Table),4);
    gtk_widget_show(Table);

    /* The Pixmap */
    Pixmap = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_DIALOG);
    gtk_table_attach_defaults(GTK_TABLE(Table),Pixmap,0,1,0,1);
    //gtk_table_attach(GTK_TABLE(Table),Pixmap,0,1,0,1,GTK_FILL,GTK_FILL,0,0);
    gtk_misc_set_padding(GTK_MISC(Pixmap),6,6);
    gtk_widget_show(Pixmap);

    /* The Message */
    Label = gtk_label_new (message);
    gtk_table_attach_defaults(GTK_TABLE(Table),Label,1,2,0,1);
    gtk_misc_set_padding(GTK_MISC(Label),6,6);
    gtk_label_set_justify(GTK_LABEL(Label),GTK_JUSTIFY_CENTER);
    //gtk_label_set_line_wrap(GTK_LABEL(Label),TRUE);
    gtk_widget_show(Label);

    /* The Check Button */
    gtk_table_attach_defaults(GTK_TABLE(Table),mb->check_button,0,2,1,2);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mb->check_button),mb->check_button_state);
    g_signal_connect(G_OBJECT(mb->check_button),"toggled",G_CALLBACK(check_button_toggled),mb);
    gtk_widget_show(mb->check_button);


    /* Buttons */
    ButtonBox = gtk_hbutton_box_new();
    gtk_box_set_spacing(GTK_BOX(ButtonBox), 15);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(ButtonBox), GTK_BUTTONBOX_END);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(mb)->action_area),ButtonBox);

    /* Read buttons from variable arguments */
    va_start (cursor,stock_id);
    while ( (cursor_value = va_arg(cursor,gint)) != 0 )
    {
        Button = Create_Button_With_Pixmap(cursor_value);
        gtk_container_add(GTK_CONTAINER(ButtonBox),Button);
        GTK_WIDGET_SET_FLAGS(Button,GTK_CAN_DEFAULT);
        g_signal_connect(G_OBJECT(Button),"destroy", G_CALLBACK(gtk_widget_destroyed),&Button);
        g_signal_connect(G_OBJECT(Button),"clicked", G_CALLBACK(msg_box_button_clicked),mb);
        g_object_set_data(G_OBJECT(Button),"button_value", GINT_TO_POINTER(cursor_value));
    }
    va_end(cursor);
    gtk_widget_grab_default(Button);
    gtk_widget_show_all(ButtonBox);

    return GTK_WIDGET(mb);
}


void msg_box_destroy (GtkObject *object)
{
    MsgBox *mb;

    g_return_if_fail( (mb = MSG_BOX(object)) );

    if (mb->check_button)
        gtk_widget_destroy(mb->check_button);

/* FIXME: causes segfault in some cases (when callin gtk_widget_destroy(msgbox) ) */
/*    if (GTK_OBJECT_CLASS(parent_klass)->destroy)
 *        (* GTK_OBJECT_CLASS(parent_klass)->destroy) (object);
 */
}


static gint msg_box_delete_event (GtkWidget *widget, GdkEventAny *event)
{
    MsgBox *mb;

    g_return_val_if_fail((mb = MSG_BOX(widget)), FALSE);

    /* If the window is closed whitout pressing a button, we return -1 */
    mb->button_clicked = -1;

    if (gtk_main_level()>1)
        gtk_main_quit();

    return TRUE;
}


/*
 * "Run" and show the msgbox, and send which button was pressed (or not a button)
 */
gint msg_box_run (MsgBox *msgbox)
{
    MsgBox *mb;

    g_return_val_if_fail((mb = MSG_BOX(msgbox)),0);

    gtk_widget_show(GTK_WIDGET(mb));
    gtk_main();
    gtk_widget_hide(GTK_WIDGET(mb));
    gtk_window_set_modal(GTK_WINDOW(mb),FALSE);

    return (MSG_BOX(mb)->button_clicked);
}


static void msg_box_show (GtkWidget *widget)
{
    if (GTK_WIDGET_CLASS(parent_klass)->show)
        (* (GTK_WIDGET_CLASS(parent_klass)->show))(widget);
}


/*
 * Callback when the check_button is pressed
 */
void check_button_toggled (GtkCheckButton *checkbutton, gpointer data)
{
    MsgBox *mb;

    g_return_if_fail((mb = MSG_BOX(data)));

    mb->check_button_state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton));
}


/*
 * Set the check_button to the 'is_active' state
 */
void msg_box_check_button_set_active (MsgBox *msgbox, gboolean is_active)
{
    MsgBox *mb;

    g_return_if_fail((mb = MSG_BOX(msgbox)));

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mb->check_button),is_active);
    mb->check_button_state = is_active;
}


/*
 * Get state of the check_button
 */
gboolean msg_box_check_button_get_active (MsgBox *msgbox)
{
    MsgBox *mb;

    g_return_val_if_fail((mb = MSG_BOX(msgbox)),FALSE);

    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mb->check_button));
}


/*
 * Hide the check_button.
 */
void msg_box_hide_check_button (MsgBox *msgbox)
{
    MsgBox *mb;

    g_return_if_fail((mb = MSG_BOX(msgbox)));

    gtk_widget_hide(mb->check_button);
}
