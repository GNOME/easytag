/*
 * gtkwin32dep.h UI Win32 Specific Functionality
 * easytag
 *
 * File: win32dep.h
 *
 * Copyright (C) 2002-2003, Herman Bloggs <hermanator12002@yahoo.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 *
 */
#ifndef _GTKWIN32DEP_H_
#define _GTKWIN32DEP_H_
#include <windows.h>
#include <gtk/gtk.h>

HINSTANCE wineasytag_dll_hinstance(void);
HINSTANCE wineasytag_exe_hinstance(void);


/* Misc */
void wineasytag_notify_uri(const char *uri);
void wineasytag_shell_execute(const char *target, const char *verb, const char *clazz);

/* init / cleanup */
void wineasytag_init(HINSTANCE);
void wineasytag_post_init(void);
void wineasytag_cleanup(void);

#endif /* _GTKWIN32DEP_H_ */

