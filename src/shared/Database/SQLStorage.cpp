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

#include "SQLStorage.h"

// -----------------------------------  SQLStorageBase  ---------------------------------------- //

SQLStorageBase::SQLStorageBase() :
    m_recordCount(0),
    m_maxEntry(0),
    m_dstFieldCount(0),
    m_srcFieldCount(0),
    m_recordSize(0),
    m_tableName(NULL),
    m_entry_field(NULL),
    m_src_format(NULL),
    m_dst_format(NULL),
    m_data(NULL)
{}

void SQLStorageBase::Initialize( const char* tableName, const char* entry_field, const char* src_format, const char* dst_format)
{
    m_tableName = tableName;
    m_entry_field = entry_field;
    m_src_format = src_format;
    m_dst_format = dst_format;

    m_srcFieldCount = strlen(m_src_format);
    m_dstFieldCount = strlen(m_dst_format);
}

char* SQLStorageBase::createRecord(uint32 recordId)
{
    char* newRecord = &m_data[m_recordCount * m_recordSize];
    ++m_recordCount;

    JustCreatedRecord(recordId, newRecord);
    return newRecord;
}

void SQLStorageBase::prepareToLoad(uint32 maxEntry, uint32 recordCount, uint32 recordSize)
{
    m_maxEntry = maxEntry;
    m_recordSize = recordSize;

    delete[] m_data;
    m_data = new char[recordCount * m_recordSize];
    memset(m_data, 0, recordCount * m_recordSize);

    m_recordCount = 0;
}

// Function to delete the data
void SQLStorageBase::Free()
{
    if (!m_data)
        return;

    uint32 offset = 0;
    for (uint32 x = 0; x < m_dstFieldCount; ++x)
    {
        switch (m_dst_format[x])
        {
            case FT_LOGIC:
                offset += sizeof(bool);
                break;
            case FT_STRING:
            {
                for (uint32 recordItr = 0; recordItr < m_recordCount; ++recordItr)
                        delete[] *(char**)((char*)(m_data + (recordItr * m_recordSize)) + offset);

                offset += sizeof(char*);
                break;
            }
            case FT_NA:
            case FT_INT:
                offset += sizeof(uint32);
                break;
            case FT_BYTE:
            case FT_NA_BYTE:
                offset += sizeof(char);
                break;
            case FT_FLOAT:
            case FT_NA_FLOAT:
                offset += sizeof(float);
                break;
            case FT_NA_POINTER:
                // TODO- possible (and small) memleak here possible
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
    delete[] m_data;
    m_data = NULL;
    m_recordCount = 0;
}

// -----------------------------------  SQLStorage  -------------------------------------------- //

void SQLStorage::EraseEntry(uint32 id)
{
    m_Index[id] = NULL;
}

void SQLStorage::Free()
{
    SQLStorageBase::Free();
    delete[] m_Index;
    m_Index = NULL;
}

void SQLStorage::Load()
{
    SQLStorageLoader loader;
    loader.Load(*this);
}

SQLStorage::SQLStorage(const char* fmt, const char* _entry_field, const char* sqlname)
{
    Initialize(sqlname, _entry_field, fmt, fmt);
    m_Index = NULL;
}

SQLStorage::SQLStorage(const char* src_fmt, const char* dst_fmt, const char* _entry_field, const char* sqlname)
{
    Initialize(sqlname, _entry_field, src_fmt, dst_fmt);
    m_Index = NULL;
}

void SQLStorage::prepareToLoad(uint32 maxRecordId, uint32 recordCount, uint32 recordSize)
{
    // Clear (possible) old data and old index array
    Free();

    // Set index array
    m_Index = new char*[maxRecordId];
    memset(m_Index, NULL, maxRecordId * sizeof(char*));

    SQLStorageBase::prepareToLoad(maxRecordId, recordCount, recordSize);
}

// -----------------------------------  SQLHashStorage  ---------------------------------------- //
void SQLHashStorage::Load()
{
    SQLHashStorageLoader loader;
    loader.Load(*this);
}

void SQLHashStorage::Free()
{
    SQLStorageBase::Free();
    m_indexMap.clear();
}

void SQLHashStorage::prepareToLoad(uint32 maxRecordId, uint32 recordCount, uint32 recordSize)
{
    // Clear (possible) old data and old index array
    Free();

    SQLStorageBase::prepareToLoad(maxRecordId, recordCount, recordSize);
}

void SQLHashStorage::EraseEntry(uint32 id)
{
    // do not erase from m_records
    RecordMap::iterator find = m_indexMap.find(id);
    if (find != m_indexMap.end())
        find->second = NULL;
}

SQLHashStorage::SQLHashStorage(const char* fmt, const char * _entry_field, const char * sqlname)
{
    Initialize(sqlname, _entry_field, fmt, fmt);
}

SQLHashStorage::SQLHashStorage(const char* src_fmt, const char* dst_fmt, const char * _entry_field, const char * sqlname)
{
    Initialize(sqlname, _entry_field, src_fmt, dst_fmt);
}

// -----------------------------------  SQLMultiStorage  --------------------------------------- //
void SQLMultiStorage::Load()
{
    SQLMultiStorageLoader loader;
    loader.Load(*this);
}

void SQLMultiStorage::Free()
{
    SQLStorageBase::Free();
    m_indexMultiMap.clear();
}

void SQLMultiStorage::prepareToLoad(uint32 maxRecordId, uint32 recordCount, uint32 recordSize)
{
    // Clear (possible) old data and old index array
    Free();

    SQLStorageBase::prepareToLoad(maxRecordId, recordCount, recordSize);
}

void SQLMultiStorage::EraseEntry(uint32 id)
{
    m_indexMultiMap.erase(id);
}

SQLMultiStorage::SQLMultiStorage(const char* fmt, const char * _entry_field, const char * sqlname)
{
    Initialize(sqlname, _entry_field, fmt, fmt);
}

SQLMultiStorage::SQLMultiStorage(const char* src_fmt, const char* dst_fmt, const char * _entry_field, const char * sqlname)
{
    Initialize(sqlname, _entry_field, src_fmt, dst_fmt);
}
