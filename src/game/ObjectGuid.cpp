/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
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

#include "ObjectGuid.h"
#include <sstream>

char const* ObjectGuid::GetTypeName() const
{
    switch(GetHigh())
    {
        case HIGHGUID_ITEM:         return "item";
        case HIGHGUID_PLAYER:       return !IsEmpty() ? "player" : "none";
        case HIGHGUID_GAMEOBJECT:   return "gameobject";
        case HIGHGUID_TRANSPORT:    return "transport";
        case HIGHGUID_UNIT:         return "creature";
        case HIGHGUID_PET:          return "pet";
        case HIGHGUID_VEHICLE:      return "vehicle";
        case HIGHGUID_DYNAMICOBJECT:return "dynobject";
        case HIGHGUID_CORPSE:       return "corpse";
        case HIGHGUID_MO_TRANSPORT: return "mo_transport";
        default:
            return "<unknown>";
    }
}

std::string ObjectGuid::GetString() const
{
    std::ostringstream str;
    str << GetTypeName() << " (";
    if (HasEntry())
        str << "Entry: " << GetEntry() << " ";
    str << "Guid: " << GetCounter() << ")";
    return str.str();
}

ByteBuffer& operator<< (ByteBuffer& buf, ObjectGuid const& guid)
{
    buf << uint64(guid.GetRawValue());
    return buf;
}

ByteBuffer &operator>>(ByteBuffer& buf, ObjectGuid& guid)
{
    guid.Set(buf.read<uint64>());
    return buf;
}

ByteBuffer& operator<< (ByteBuffer& buf, PackedGuid const& guid)
{
    buf.append(guid.m_packedGuid);
    return buf;
}

ByteBuffer &operator>>(ByteBuffer& buf, PackedGuidReader const& guid)
{
    guid.m_guidPtr->Set(buf.readPackGUID());
    return buf;
}
