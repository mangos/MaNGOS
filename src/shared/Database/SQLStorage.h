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

#ifndef SQLSTORAGE_H
#define SQLSTORAGE_H

#include "Common.h"
#include "Database/DatabaseEnv.h"

class SQLStorage
{
    template<class T>
    friend struct SQLStorageLoaderBase;

    public:

        SQLStorage(const char* fmt, const char * _entry_field, const char * sqlname)
        {
            src_format = fmt;
            dst_format = fmt;
            init(_entry_field, sqlname);
        }

        SQLStorage(const char* src_fmt, const char* dst_fmt, const char * _entry_field, const char * sqlname)
        {
            src_format = src_fmt;
            dst_format = dst_fmt;
            init(_entry_field, sqlname);
        }


        ~SQLStorage()
        {
            Free();
        }

        template<class T>
            T const* LookupEntry(uint32 id) const
        {
            if( id == 0 )
                return NULL;
            if(id >= MaxEntry)
                return NULL;
            return reinterpret_cast<T const*>(pIndex[id]);
        }

        uint32 RecordCount;
        uint32 MaxEntry;
        uint32 iNumFields;

        char const* GetTableName() const { return table; }

        void Load();
        void Free();

        void EraseEntry(uint32 id);
    private:
        void init(const char * _entry_field, const char * sqlname)
        {
            entry_field = _entry_field;
            table=sqlname;
            data=NULL;
            pIndex=NULL;
            iNumFields = strlen(src_format);
            MaxEntry = 0;
        }

        char** pIndex;

        char *data;
        const char *src_format;
        const char *dst_format;
        const char *table;
        const char *entry_field;
        //bool HasString;
};

template <class T>
struct SQLStorageLoaderBase
{
    public:
        void Load(SQLStorage &storage, bool error_at_empty = true);

        template<class S, class D>
            void convert(uint32 field_pos, S src, D &dst);
        template<class S>
            void convert_to_str(uint32 field_pos, S src, char * & dst);
        template<class D>
            void convert_from_str(uint32 field_pos, char const* src, D& dst);
        void convert_str_to_str(uint32 field_pos, char const* src, char *&dst);

        // trap, no body
        template<class D>
            void convert_from_str(uint32 field_pos, char* src, D& dst);
        void convert_str_to_str(uint32 field_pos, char* src, char *&dst);
    private:
        template<class V>
            void storeValue(V value, SQLStorage &store, char *p, uint32 x, uint32 &offset);
        void storeValue(char const* value, SQLStorage &store, char *p, uint32 x, uint32 &offset);

        // trap, no body
        void storeValue(char * value, SQLStorage &store, char *p, uint32 x, uint32 &offset);
};

struct SQLStorageLoader : public SQLStorageLoaderBase<SQLStorageLoader>
{
};

#endif
