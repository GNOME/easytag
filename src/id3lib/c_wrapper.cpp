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

#include "config.h"

#ifdef ENABLE_ID3LIB

#include <id3.h>
#include <id3/field.h>
#include <id3/tag.h>

#include "id3_bugfix.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define ID3_CATCH(code) try { code; } catch (...) { }

  //
  // Tag wrappers
  //

  ID3_C_EXPORT bool CCONV
  ID3Field_SetEncoding(ID3Field *field, ID3_TextEnc enc)
  {
    bool changed = false;
    if (field)
    {
      ID3_CATCH(changed = reinterpret_cast<ID3_Field *>(field)->SetEncoding(enc));
    }
    return changed;
  }

  ID3_C_EXPORT ID3_TextEnc CCONV
  ID3Field_GetEncoding(const ID3Field *field)
  {
    ID3_TextEnc enc = ID3TE_NONE;
    if (field)
    {
      ID3_CATCH(enc = reinterpret_cast<const ID3_Field *>(field)->GetEncoding());
    }
    return enc;
  }

  ID3_C_EXPORT bool CCONV
  ID3Field_IsEncodable(const ID3Field *field)
  {
    bool isEncodable = false;
    if (field)
    {
      ID3_CATCH(isEncodable = reinterpret_cast<const ID3_Field *>(field)->IsEncodable());
    }
    return isEncodable;
  }

  ID3_C_EXPORT ID3_FieldType CCONV
  ID3Field_GetType(const ID3Field *field)
  {
    ID3_FieldType fieldType = ID3FTY_NONE;
    if (field)
    {
      ID3_CATCH(fieldType = reinterpret_cast<const ID3_Field *>(field)->GetType());
    }
    return fieldType;
  }
  
  /*ID3_C_EXPORT ID3_FieldID CCONV
  ID3Field_GetID(const ID3Field *field)
  {
    ID3_FieldID fieldID = ID3FN_NOFIELD;
    if (field)
    {
      ID3_CATCH(fieldType = reinterpret_cast<const ID3_Field *>(field)->GetID());
    }
    return fieldType;
  }*/



  //
  // Header wrappers
  //
  
  // Call with :
  //    const Mp3_Headerinfo* headerInfo = ID3Tag_GetMp3HeaderInfo(tag);
  ID3_C_EXPORT const Mp3_Headerinfo* CCONV
  ID3Tag_GetMp3HeaderInfo(ID3Tag *tag)
  {
    const Mp3_Headerinfo* headerInfo = NULL;
    if (tag)
    {
      ID3_CATCH(headerInfo = reinterpret_cast<const ID3_Tag *>(tag)->GetMp3HeaderInfo());
    }
    return headerInfo;
  }

  // Call with :
  //    Mp3_Headerinfo* headerInfo = malloc(sizeof(Mp3_Headerinfo));
  //    ID3Tag_GetMp3HeaderInfo(tag, headerInfo);
  /*ID3_C_EXPORT bool CCONV
  ID3Tag_GetMp3HeaderInfo(ID3Tag *tag, Mp3_Headerinfo* headerInfo)
  {
    const Mp3_Headerinfo* rem_headerInfo = NULL;
    if (tag)
    {
     ID3_CATCH(rem_headerInfo = reinterpret_cast<const ID3_Tag * >(tag)->GetMp3HeaderInfo());
    }
    // Does GCC understand this? (VS does)
    if (rem_headerInfo) *headerInfo=*rem_headerInfo; 
    return rem_headerInfo!=NULL;
  }*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ENABLE_ID3LIB */
