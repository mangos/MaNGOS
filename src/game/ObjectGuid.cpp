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

#include "ObjectGuid.h"

#include "World.h"
#include "ObjectMgr.h"

#include <sstream>

char const* ObjectGuid::GetTypeName(HighGuid high)
{
    switch(high)
    {
        case HIGHGUID_ITEM:         return "Item";
        case HIGHGUID_PLAYER:       return "Player";
        case HIGHGUID_GAMEOBJECT:   return "Gameobject";
        case HIGHGUID_TRANSPORT:    return "Transport";
        case HIGHGUID_UNIT:         return "Creature";
        case HIGHGUID_PET:          return "Pet";
        case HIGHGUID_VEHICLE:      return "Vehicle";
        case HIGHGUID_DYNAMICOBJECT:return "DynObject";
        case HIGHGUID_CORPSE:       return "Corpse";
        case HIGHGUID_MO_TRANSPORT: return "MoTransport";
        case HIGHGUID_INSTANCE:     return "InstanceID";
        case HIGHGUID_GROUP:        return "Group";
        default:
            return "<unknown>";
    }
}

std::string ObjectGuid::GetString() const
{
    std::ostringstream str;
    str << GetTypeName();

    if (IsPlayer())
    {
        std::string name;
        if (sObjectMgr.GetPlayerNameByGUID(*this, name))
            str << " " << name;
    }

    str << " (";
    if (HasEntry())
        str << (IsPet() ? "Petnumber: " : "Entry: ") << GetEntry() << " ";
    str << "Guid: " << GetCounter() << ")";
    return str.str();
}

template<HighGuid high>
uint32 ObjectGuidGenerator<high>::Generate()
{
    if (m_nextGuid >= ObjectGuid::GetMaxCounter(high)-1)
    {
        sLog.outError("%s guid overflow!! Can't continue, shutting down server. ",ObjectGuid::GetTypeName(high));
        World::StopNow(ERROR_EXIT_CODE);
    }
    return m_nextGuid++;
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

template uint32 ObjectGuidGenerator<HIGHGUID_ITEM>::Generate();
template uint32 ObjectGuidGenerator<HIGHGUID_PLAYER>::Generate();
template uint32 ObjectGuidGenerator<HIGHGUID_GAMEOBJECT>::Generate();
template uint32 ObjectGuidGenerator<HIGHGUID_TRANSPORT>::Generate();
template uint32 ObjectGuidGenerator<HIGHGUID_UNIT>::Generate();
template uint32 ObjectGuidGenerator<HIGHGUID_PET>::Generate();
template uint32 ObjectGuidGenerator<HIGHGUID_VEHICLE>::Generate();
template uint32 ObjectGuidGenerator<HIGHGUID_DYNAMICOBJECT>::Generate();
template uint32 ObjectGuidGenerator<HIGHGUID_CORPSE>::Generate();
template uint32 ObjectGuidGenerator<HIGHGUID_INSTANCE>::Generate();
template uint32 ObjectGuidGenerator<HIGHGUID_GROUP>::Generate();
