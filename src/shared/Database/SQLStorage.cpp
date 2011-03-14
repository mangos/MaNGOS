/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "SQLStorage.h"
#include "SQLStorageImpl.h"

void SQLStorage::EraseEntry(uint32 id)
{
    uint32 offset = 0;
    for(uint32 x = 0; x < iNumFields; ++x)
    {
        switch(dst_format[x])
        {
            case FT_LOGIC:
                offset += sizeof(bool);   break;
            case FT_BYTE:
                offset += sizeof(char);   break;
            case FT_INT:
                offset += sizeof(uint32); break;
            case FT_FLOAT:
                offset += sizeof(float);  break;
            case FT_STRING:
            {
                if(pIndex[id])
                    delete [] *(char**)((char*)(pIndex[id])+offset);

                offset += sizeof(char*);
                break;
            }
            case FT_NA:
            case FT_NA_BYTE:
                break;
            case FT_IND:
            case FT_SORT:
                assert(false && "SQL storage not have sort field types");
                break;
            default:
                assert(false && "unknown format character");
                break;
        }
    }

    pIndex[id] = NULL;
}

void SQLStorage::Free ()
{
    uint32 offset = 0;
    for(uint32 x = 0; x < iNumFields; ++x)
    {
        switch(dst_format[x])
        {
            case FT_LOGIC:
                offset += sizeof(bool);   break;
            case FT_BYTE:
                offset += sizeof(char);   break;
            case FT_INT:
                offset += sizeof(uint32); break;
            case FT_FLOAT:
                offset += sizeof(float);  break;
            case FT_STRING:
            {
                for(uint32 y = 0; y < MaxEntry; ++y)
                    if(pIndex[y])
                        delete [] *(char**)((char*)(pIndex[y])+offset);

                offset += sizeof(char*);
                break;
            }
            case FT_NA:
            case FT_NA_BYTE:
                break;
            case FT_IND:
            case FT_SORT:
                assert(false && "SQL storage not have sort field types");
                break;
            default:
                assert(false && "unknown format character");
                break;
        }
    }

    delete [] pIndex;
    delete [] data;
}

void SQLStorage::Load()
{
    SQLStorageLoader loader;
    loader.Load(*this);
}
