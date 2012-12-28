/*
 * Copyright (C) 2005-2013 MaNGOS <http://getmangos.com/>
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

#define _CRT_SECURE_NO_DEPRECATE

#include "dbcfile.h"
#include "loadlib/loadlib.h"

DBCFile::DBCFile(const std::string& filename):
    filename(filename),
    data(0)
{

}

DBCFile::DBCFile(HANDLE file) : fileHandle(file), data(0)
{

}

bool DBCFile::open()
{
    //if (!OpenNewestFile(filename.c_str(), &fileHandle))
    //    return false;

    char header[4];
    unsigned int na, nb, es, ss;

    if (!SFileReadFile(fileHandle, header, 4, NULL, NULL))              // Magic header
    {
        SFileCloseFile(fileHandle);
        return false;
    }

    if (header[0] != 'W' || header[1] != 'D' || header[2] != 'B' || header[3] != 'C')
    {
        SFileCloseFile(fileHandle);
        return false;
    }

    if (!SFileReadFile(fileHandle, &na, 4, NULL, NULL))                 // Number of records
    {
        SFileCloseFile(fileHandle);
        return false;
    }

    if (!SFileReadFile(fileHandle, &nb, 4, NULL, NULL))                 // Number of fields
    {
        SFileCloseFile(fileHandle);
        return false;
    }

    if (!SFileReadFile(fileHandle, &es, 4, NULL, NULL))                 // Size of a record
    {
        SFileCloseFile(fileHandle);
        return false;
    }

    if (!SFileReadFile(fileHandle, &ss, 4, NULL, NULL))                 // String size
    {
        SFileCloseFile(fileHandle);
        return false;
    }

    recordSize = es;
    recordCount = na;
    fieldCount = nb;
    stringSize = ss;
    if (fieldCount * 4 != recordSize)
    {
        SFileCloseFile(fileHandle);
        return false;
    }

    data = new unsigned char[recordSize * recordCount + stringSize];
    stringTable = data + recordSize * recordCount;

    size_t data_size = recordSize * recordCount + stringSize;

    if (!SFileReadFile(fileHandle, data, data_size, NULL, NULL))
    {
        SFileCloseFile(fileHandle);
        return false;
    }

    SFileCloseFile(fileHandle);
    return true;
}

DBCFile::~DBCFile()
{
    delete [] data;
}

DBCFile::Record DBCFile::getRecord(size_t id)
{
    assert(data);
    return Record(*this, data + id * recordSize);
}

size_t DBCFile::getMaxId()
{
    assert(data);

    size_t maxId = 0;
    for (size_t i = 0; i < getRecordCount(); ++i)
    {
        if (maxId < getRecord(i).getUInt(0))
            maxId = getRecord(i).getUInt(0);
    }
    return maxId;
}

DBCFile::Iterator DBCFile::begin()
{
    assert(data);
    return Iterator(*this, data);
}
DBCFile::Iterator DBCFile::end()
{
    assert(data);
    return Iterator(*this, stringTable);
}
