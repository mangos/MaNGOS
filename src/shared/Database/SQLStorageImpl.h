/*
 * Copyright (C) 2005-2012 MaNGOS <http://getmangos.com/>
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

#ifndef SQLSTORAGE_IMPL_H
#define SQLSTORAGE_IMPL_H

#include "ProgressBar.h"
#include "Log.h"
#include "DBCFileLoader.h"

template<class T>
template<class S, class D>
void SQLStorageLoaderBase<T>::convert(uint32 /*field_pos*/, S src, D &dst)
{
    dst = D(src);
}

template<class T>
void SQLStorageLoaderBase<T>::convert_str_to_str(uint32 /*field_pos*/, char const *src, char *&dst)
{
    if (!src)
    {
        dst = new char[1];
        *dst = 0;
    }
    else
    {
        uint32 l = strlen(src) + 1;
        dst = new char[l];
        memcpy(dst, src, l);
    }
}

template<class T>
template<class S>
void SQLStorageLoaderBase<T>::convert_to_str(uint32 /*field_pos*/, S /*src*/, char * & dst)
{
    dst = new char[1];
    *dst = 0;
}

template<class T>
template<class D>
void SQLStorageLoaderBase<T>::convert_from_str(uint32 /*field_pos*/, char const* /*src*/, D& dst)
{
    dst = 0;
}

template<class T>
template<class S, class D>
void SQLStorageLoaderBase<T>::default_fill(uint32 /*field_pos*/, S src, D &dst)
{
    dst = D(src);
}

template<class T>
void SQLStorageLoaderBase<T>::default_fill_to_str(uint32 /*field_pos*/, char const* /*src*/, char * & dst)
{
    dst = new char[1];
    *dst = 0;
}

template<class T>
template<class V>
void SQLStorageLoaderBase<T>::storeValue(V value, SQLStorage &store, char *p, uint32 x, uint32 &offset)
{
    T * subclass = (static_cast<T*>(this));
    switch(store.dst_format[x])
    {
        case FT_LOGIC:
            subclass->convert(x, value, *((bool*)(&p[offset])) );
            offset+=sizeof(bool);
            break;
        case FT_BYTE:
            subclass->convert(x, value, *((char*)(&p[offset])) );
            offset+=sizeof(char);
            break;
        case FT_INT:
            subclass->convert(x, value, *((uint32*)(&p[offset])) );
            offset+=sizeof(uint32);
            break;
        case FT_FLOAT:
            subclass->convert(x, value, *((float*)(&p[offset])) );
            offset+=sizeof(float);
            break;
        case FT_STRING:
            subclass->convert_to_str(x, value, *((char**)(&p[offset])) );
            offset+=sizeof(char*);
            break;
        case FT_NA:
            subclass->default_fill(x, value, *((int32*)(&p[offset])) );
            offset+=sizeof(uint32);
            break;
        case FT_NA_BYTE:
            subclass->default_fill(x, value, *((char*)(&p[offset])) );
            offset+=sizeof(char);
            break;
        case FT_NA_FLOAT:
            subclass->default_fill(x, value, *((float*)(&p[offset])) );
            offset+=sizeof(float);
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

template<class T>
void SQLStorageLoaderBase<T>::storeValue(char const* value, SQLStorage &store, char *p, uint32 x, uint32 &offset)
{
    T * subclass = (static_cast<T*>(this));
    switch(store.dst_format[x])
    {
        case FT_LOGIC:
            subclass->convert_from_str(x, value, *((bool*)(&p[offset])) );
            offset+=sizeof(bool);
            break;
        case FT_BYTE:
            subclass->convert_from_str(x, value, *((char*)(&p[offset])) );
            offset += sizeof(char);
            break;
        case FT_INT:
            subclass->convert_from_str(x, value, *((uint32*)(&p[offset])) );
            offset += sizeof(uint32);
            break;
        case FT_FLOAT:
            subclass->convert_from_str(x, value, *((float*)(&p[offset])) );
            offset += sizeof(float);
            break;
        case FT_STRING:
            subclass->convert_str_to_str(x, value, *((char**)(&p[offset])) );
            offset += sizeof(char*);
            break;
        case FT_NA_POINTER:
            subclass->default_fill_to_str(x, value, *((char**)(&p[offset])) );
            offset += sizeof(char*);
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

template<class T>
void SQLStorageLoaderBase<T>::Load(SQLStorage &store, bool error_at_empty /*= true*/)
{
    uint32 maxi;
    Field *fields;
    QueryResult *result  = WorldDatabase.PQuery("SELECT MAX(%s) FROM %s", store.entry_field, store.table);
    if(!result)
    {
        sLog.outError("Error loading %s table (not exist?)\n", store.table);
        Log::WaitBeforeContinueIfNeed();
        exit(1);                                            // Stop server at loading non exited table or not accessable table
    }

    maxi = (*result)[0].GetUInt32()+1;
    delete result;

    result = WorldDatabase.PQuery("SELECT COUNT(*) FROM %s", store.table);
    if(result)
    {
        fields = result->Fetch();
        store.RecordCount = fields[0].GetUInt32();
        delete result;
    }
    else
        store.RecordCount = 0;

    result = WorldDatabase.PQuery("SELECT * FROM %s", store.table);

    if(!result)
    {
        if (error_at_empty)
            sLog.outError("%s table is empty!\n", store.table);
        else
            sLog.outString("%s table is empty!\n", store.table);

        store.RecordCount = 0;
        return;
    }

    uint32 recordsize = 0;
    uint32 offset = 0;

    if (store.iNumFields != result->GetFieldCount())
    {
        store.RecordCount = 0;
        sLog.outError("Error in %s table, probably sql file format was updated (there should be %d fields in sql).\n", store.table, store.iNumFields);
        delete result;
        Log::WaitBeforeContinueIfNeed();
        exit(1);                                            // Stop server at loading broken or non-compatible table.
    }

    // get struct size
    for (uint32 x = 0; x < store.oNumFields; ++x)
    {
        switch(store.dst_format[x])
        {
            case FT_LOGIC:
                recordsize += sizeof(bool);   break;
            case FT_BYTE:
                recordsize += sizeof(char);   break;
            case FT_INT:
                recordsize += sizeof(uint32); break;
            case FT_FLOAT:
                recordsize += sizeof(float);  break;
            case FT_STRING:
                recordsize += sizeof(char*);  break;
            case FT_NA:
                recordsize += sizeof(uint32); break;
            case FT_NA_BYTE:
                recordsize += sizeof(char);   break;
            case FT_NA_FLOAT:
                recordsize += sizeof(float);  break;
            case FT_NA_POINTER:
                recordsize += sizeof(char*);  break;
            case FT_IND:
            case FT_SORT:
                assert(false && "SQL storage not have sort field types");
                break;
            default:
                assert(false && "unknown format character");
                break;
        }
    }

    char** newIndex = new char*[maxi];
    memset(newIndex, 0, maxi*sizeof(char*));

    char* _data= new char[store.RecordCount * recordsize];
    uint32 count = 0;
    BarGoLink bar(store.RecordCount);
    do
    {
        fields = result->Fetch();
        bar.step();
        char* p = (char*)&_data[recordsize * count];
        newIndex[fields[0].GetUInt32()] = p;

        offset = 0;
        // dependend on dest-size
        // iterate two indexes: x over dest, y over source, y++ IFF x1=x/X
        for (uint32 x = 0, y = 0; x < store.oNumFields; ++x)
        {
            switch (store.dst_format[x])
            {
                // For default fill continue and do not increase y
                case FT_NA:         storeValue((uint32)0, store, p, x, offset);         continue;
                case FT_NA_BYTE:    storeValue((char)0, store, p, x, offset);           continue;
                case FT_NA_FLOAT:   storeValue((float)0.0f, store, p, x, offset);       continue;
                case FT_NA_POINTER: storeValue((char const*)NULL, store, p, x, offset); continue;
            }

            // It is required that the input has at least as many columns set as the output requires
            if (y >= store.iNumFields)
                assert(false && "SQL storage has too few columns!");

            switch(store.src_format[y])
            {
                case FT_LOGIC:
                    storeValue((bool)(fields[y].GetUInt32() > 0), store, p, x, offset); break;
                case FT_BYTE:
                    storeValue((char)fields[y].GetUInt8(), store, p, x, offset); break;
                case FT_INT:
                    storeValue((uint32)fields[y].GetUInt32(), store, p, x, offset); break;
                case FT_FLOAT:
                    storeValue((float)fields[y].GetFloat(), store, p, x, offset); break;
                case FT_STRING:
                    storeValue((char const*)fields[y].GetString(), store, p, x, offset); break;
                case FT_NA:
                case FT_NA_BYTE:
                case FT_NA_FLOAT:
                    break;
                case FT_IND:
                case FT_SORT:
                case FT_NA_POINTER:
                    assert(false && "SQL storage not have sort or pointer field types");
                    break;
                default:
                    assert(false && "unknown format character");
            }
            ++y;
        }
        ++count;
    }
    while (result->NextRow());

    delete result;

    store.pIndex = newIndex;
    store.MaxEntry = maxi;
    store.data = _data;
}

#endif
