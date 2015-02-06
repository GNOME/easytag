/* EasyTAG - tag editor for audio files
 * Copyright (C) 2014 David King <amigadave@amigadave.com>
 * Copyright (C) 1999, 2000  Scott Thomas Haug
 * Copyright (C) 2002 Thijmen Klok (thijmen@id3lib.org)
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
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef ET_ID3LIB_BUGFIX_H_
#define ET_ID3LIB_BUGFIX_H_

#ifdef ENABLE_ID3LIB

#include <glib.h>
#include <id3.h>

G_BEGIN_DECLS

#if !HAVE_DECL_ID3FIELD_SETENCODING
ID3_C_EXPORT bool                  CCONV ID3Field_SetEncoding    (ID3Field *field, ID3_TextEnc enc);
#endif /* !HAVE_DECL_ID3FIELD_SETENCODING */
#if !HAVE_DECL_ID3FIELD_GETENCODING
ID3_C_EXPORT ID3_TextEnc           CCONV ID3Field_GetEncoding    (const ID3Field *field);
#endif /* !HAVE_DECL_ID3FIELD_GETENCODING */
#if !HAVE_DECL_ID3FIELD_ISENCODABLE
ID3_C_EXPORT bool                  CCONV ID3Field_IsEncodable    (const ID3Field *field);
#endif /* !HAVE_DECL_ID3FIELD_ISENCODABLE */
ID3_C_EXPORT ID3_FieldType         CCONV ID3Field_GetType        (const ID3Field *field);
//ID3_C_EXPORT ID3_FieldID           CCONV ID3Field_GetID          (const ID3Field *field);

ID3_C_EXPORT const Mp3_Headerinfo* CCONV ID3Tag_GetMp3HeaderInfo (ID3Tag *tag);
  
G_END_DECLS

#endif /* ENABLE_ID3LIB */

#endif /* ET_ID3LIB_BUGFIX_H_ */
