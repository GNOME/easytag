/* log.h - 2007/03/25 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2000-2007  Jerome Couderc <easytag@gmail.com>
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


#ifndef __LOG_H__
#define __LOG_H__

#include <gtk/gtk.h>

//#include "et_core.h"


/*
 * Types of errors
 */
typedef enum
{                  
    LOG_UNKNOWN = 0,
    LOG_OK,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} Log_Error_Type;


/**************
 * Prototypes *
 **************/

GtkWidget *Create_Log_Area      (void);

void       Log_Clean_Log_List   (void);

void       Log_Print            (Log_Error_Type error_type, gchar const *format, ...);


#endif /* __LOG_H__ */
