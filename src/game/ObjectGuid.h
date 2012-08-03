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

#ifndef MANGOS_OBJECT_GUID_H
#define MANGOS_OBJECT_GUID_H

#include "Common.h"
#include "ByteBuffer.h"

#include <functional>

enum TypeID
{
    TYPEID_OBJECT        = 0,
    TYPEID_ITEM          = 1,
    TYPEID_CONTAINER     = 2,
    TYPEID_UNIT          = 3,
    TYPEID_PLAYER        = 4,
    TYPEID_GAMEOBJECT    = 5,
    TYPEID_DYNAMICOBJECT = 6,
    TYPEID_CORPSE        = 7
};

#define MAX_TYPE_ID        8

enum TypeMask
{
    TYPEMASK_OBJECT         = 0x0001,
    TYPEMASK_ITEM           = 0x0002,
    TYPEMASK_CONTAINER      = 0x0004,
    TYPEMASK_UNIT           = 0x0008,                       // players also have it
    TYPEMASK_PLAYER         = 0x0010,
    TYPEMASK_GAMEOBJECT     = 0x0020,
    TYPEMASK_DYNAMICOBJECT  = 0x0040,
    TYPEMASK_CORPSE         = 0x0080,

    // used combinations in Player::GetObjectByTypeMask (TYPEMASK_UNIT case ignore players in call)
    TYPEMASK_CREATURE_OR_GAMEOBJECT = TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT,
    TYPEMASK_CREATURE_GAMEOBJECT_OR_ITEM = TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT | TYPEMASK_ITEM,
    TYPEMASK_CREATURE_GAMEOBJECT_PLAYER_OR_ITEM = TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT | TYPEMASK_ITEM | TYPEMASK_PLAYER,

    TYPEMASK_WORLDOBJECT = TYPEMASK_UNIT | TYPEMASK_PLAYER | TYPEMASK_GAMEOBJECT | TYPEMASK_DYNAMICOBJECT | TYPEMASK_CORPSE,
};

enum HighGuid
{
    HIGHGUID_ITEM           = 0x470,                        // blizz 470
    HIGHGUID_CONTAINER      = 0x470,                        // blizz 470
    HIGHGUID_PLAYER         = 0x000,                        // blizz 070 (temporary reverted back to 0 high guid
                                                            // in result unknown source visibility player with
                                                            // player problems. please reapply only after its resolve)
    HIGHGUID_GAMEOBJECT     = 0xF11,                        // blizz F11/F51
    HIGHGUID_TRANSPORT      = 0xF12,                        // blizz F12/F52 (for GAMEOBJECT_TYPE_TRANSPORT)
    HIGHGUID_UNIT           = 0xF13,                        // blizz F13/F53
    HIGHGUID_PET            = 0xF14,                        // blizz F14/F54
    HIGHGUID_VEHICLE        = 0xF15,                        // blizz F15/F55
    HIGHGUID_DYNAMICOBJECT  = 0xF10,                        // blizz F10/F50
    HIGHGUID_CORPSE         = 0xF50,                        // blizz F10/F50 used second variant to resolve conflict with HIGHGUID_DYNAMICOBJECT
    HIGHGUID_MO_TRANSPORT   = 0x1FC,                        // blizz 1FC (for GAMEOBJECT_TYPE_MO_TRANSPORT)
    HIGHGUID_INSTANCE       = 0x1F4,                        // blizz 1F4
    HIGHGUID_GROUP          = 0x1F5,                        // blizz 1F5
};

class ObjectGuid;
class PackedGuid;

struct PackedGuidReader
{
    explicit PackedGuidReader(ObjectGuid& guid) : m_guidPtr(&guid) {}
    ObjectGuid* m_guidPtr;
};

class MANGOS_DLL_SPEC ObjectGuid
{
    public:                                                 // constructors
        ObjectGuid() : m_guid(0) {}
        explicit ObjectGuid(uint64 guid) : m_guid(guid) {}
        ObjectGuid(HighGuid hi, uint32 entry, uint32 counter) : m_guid(counter ? uint64(counter) | (uint64(entry) << 24) | (uint64(hi) << 52) : 0) {}
        ObjectGuid(HighGuid hi, uint32 counter) : m_guid(counter ? uint64(counter) | (uint64(hi) << 52) : 0) {}

        operator uint64() const { return m_guid; }
    private:
        explicit ObjectGuid(uint32 const&);                 // no implementation, used for catch wrong type assign
        ObjectGuid(HighGuid, uint32, uint64 counter);       // no implementation, used for catch wrong type assign
        ObjectGuid(HighGuid, uint64 counter);               // no implementation, used for catch wrong type assign

    public:                                                 // modifiers
        PackedGuidReader ReadAsPacked() { return PackedGuidReader(*this); }

        void Set(uint64 guid) { m_guid = guid; }
        void Clear() { m_guid = 0; }

        PackedGuid WriteAsPacked() const;
    public:                                                 // accessors
        uint64   GetRawValue() const { return m_guid; }
        HighGuid GetHigh() const { return HighGuid((m_guid >> 52) & 0x00000FFF); }
        uint32   GetEntry() const { return HasEntry() ? uint32((m_guid >> 24) & UI64LIT(0x0000000000FFFFFF)) : 0; }
        uint32   GetCounter()  const
        {
            return HasEntry()
                ? uint32(m_guid & UI64LIT(0x0000000000FFFFFF))
                : uint32(m_guid & UI64LIT(0x00000000FFFFFFFF));
        }

        static uint32 GetMaxCounter(HighGuid high)
        {
            return HasEntry(high)
                ? uint32(0x00FFFFFF)
                : uint32(0xFFFFFFFF);
        }

        uint32 GetMaxCounter() const { return GetMaxCounter(GetHigh()); }

        bool IsEmpty()             const { return m_guid == 0;                                }
        bool IsCreature()          const { return GetHigh() == HIGHGUID_UNIT;                 }
        bool IsPet()               const { return GetHigh() == HIGHGUID_PET;                  }
        bool IsVehicle()           const { return GetHigh() == HIGHGUID_VEHICLE;              }
        bool IsCreatureOrPet()     const { return IsCreature() || IsPet();                    }
        bool IsCreatureOrVehicle() const { return IsCreature() || IsVehicle();                }
        bool IsAnyTypeCreature()   const { return IsCreature() || IsPet() || IsVehicle();     }
        bool IsPlayer()            const { return !IsEmpty() && GetHigh() == HIGHGUID_PLAYER; }
        bool IsUnit()              const { return IsAnyTypeCreature() || IsPlayer();          }
        bool IsItem()              const { return GetHigh() == HIGHGUID_ITEM;                 }
        bool IsGameObject()        const { return GetHigh() == HIGHGUID_GAMEOBJECT;           }
        bool IsDynamicObject()     const { return GetHigh() == HIGHGUID_DYNAMICOBJECT;        }
        bool IsCorpse()            const { return GetHigh() == HIGHGUID_CORPSE;               }
        bool IsTransport()         const { return GetHigh() == HIGHGUID_TRANSPORT;            }
        bool IsMOTransport()       const { return GetHigh() == HIGHGUID_MO_TRANSPORT;         }
        bool IsInstance()          const { return GetHigh() == HIGHGUID_INSTANCE;             }
        bool IsGroup()             const { return GetHigh() == HIGHGUID_GROUP;                }

        static TypeID GetTypeId(HighGuid high)
        {
            switch(high)
            {
                case HIGHGUID_ITEM:         return TYPEID_ITEM;
                //case HIGHGUID_CONTAINER:    return TYPEID_CONTAINER; HIGHGUID_CONTAINER==HIGHGUID_ITEM currently
                case HIGHGUID_UNIT:         return TYPEID_UNIT;
                case HIGHGUID_PET:          return TYPEID_UNIT;
                case HIGHGUID_PLAYER:       return TYPEID_PLAYER;
                case HIGHGUID_GAMEOBJECT:   return TYPEID_GAMEOBJECT;
                case HIGHGUID_DYNAMICOBJECT:return TYPEID_DYNAMICOBJECT;
                case HIGHGUID_CORPSE:       return TYPEID_CORPSE;
                case HIGHGUID_MO_TRANSPORT: return TYPEID_GAMEOBJECT;
                case HIGHGUID_VEHICLE:      return TYPEID_UNIT;
                // unknown
                case HIGHGUID_INSTANCE:
                case HIGHGUID_GROUP:
                default:                    return TYPEID_OBJECT;
            }
        }

        TypeID GetTypeId() const { return GetTypeId(GetHigh()); }

        bool operator! () const { return IsEmpty(); }
        bool operator== (ObjectGuid const& guid) const { return GetRawValue() == guid.GetRawValue(); }
        bool operator!= (ObjectGuid const& guid) const { return GetRawValue() != guid.GetRawValue(); }
        bool operator< (ObjectGuid const& guid) const { return GetRawValue() < guid.GetRawValue(); }

        uint8& operator[] (uint8 index)
        {
            MANGOS_ASSERT(index < 8);

#if MANGOS_ENDIAN == MANGOS_LITTLEENDIAN
            return m_guidBytes[index];
#else
            return m_guidBytes[7 - index];
#endif
        }

        uint8 const& operator[] (uint8 index) const
        {
            MANGOS_ASSERT(index < 8);

#if MANGOS_ENDIAN == MANGOS_LITTLEENDIAN
            return m_guidBytes[index];
#else
            return m_guidBytes[7 - index];
#endif
        }

    public:                                                 // accessors - for debug
        static char const* GetTypeName(HighGuid high);
        char const* GetTypeName() const { return !IsEmpty() ? GetTypeName(GetHigh()) : "None"; }
        std::string GetString() const;

    private:                                                // internal functions
        static bool HasEntry(HighGuid high)
        {
            switch(high)
            {
                case HIGHGUID_ITEM:
                case HIGHGUID_PLAYER:
                case HIGHGUID_DYNAMICOBJECT:
                case HIGHGUID_CORPSE:
                case HIGHGUID_MO_TRANSPORT:
                case HIGHGUID_INSTANCE:
                case HIGHGUID_GROUP:
                    return false;
                case HIGHGUID_GAMEOBJECT:
                case HIGHGUID_TRANSPORT:
                case HIGHGUID_UNIT:
                case HIGHGUID_PET:
                case HIGHGUID_VEHICLE:
                default:
                    return true;
            }
        }

        bool HasEntry() const { return HasEntry(GetHigh()); }

    private:                                                // fields
        union
        {
            uint64 m_guid;
            uint8 m_guidBytes[8];
        };
};

typedef std::set<ObjectGuid> ObjectGuidSet;

//minimum buffer size for packed guid is 9 bytes
#define PACKED_GUID_MIN_BUFFER_SIZE 9

class PackedGuid
{
    friend ByteBuffer& operator<< (ByteBuffer& buf, PackedGuid const& guid);

    public:                                                 // constructors
        explicit PackedGuid() : m_packedGuid(PACKED_GUID_MIN_BUFFER_SIZE) { m_packedGuid.appendPackGUID(0); }
        explicit PackedGuid(uint64 const& guid) : m_packedGuid(PACKED_GUID_MIN_BUFFER_SIZE) { m_packedGuid.appendPackGUID(guid); }
        explicit PackedGuid(ObjectGuid const& guid) : m_packedGuid(PACKED_GUID_MIN_BUFFER_SIZE) { m_packedGuid.appendPackGUID(guid.GetRawValue()); }

    public:                                                 // modifiers
        void Set(uint64 const& guid) { m_packedGuid.wpos(0); m_packedGuid.appendPackGUID(guid); }
        void Set(ObjectGuid const& guid) { m_packedGuid.wpos(0); m_packedGuid.appendPackGUID(guid.GetRawValue()); }

    public:                                                 // accessors
        size_t size() const { return m_packedGuid.size(); }

    private:                                                // fields
        ByteBuffer m_packedGuid;
};

template<HighGuid high>
class ObjectGuidGenerator
{
    public:                                                 // constructors
        explicit ObjectGuidGenerator(uint32 start = 1) : m_nextGuid(start) {}

    public:                                                 // modifiers
        void Set(uint32 val) { m_nextGuid = val; }
        uint32 Generate();

    public:                                                 // accessors
        uint32 GetNextAfterMaxUsed() const { return m_nextGuid; }

    private:                                                // fields
        uint32 m_nextGuid;
};

ByteBuffer& operator<< (ByteBuffer& buf, ObjectGuid const& guid);
ByteBuffer& operator>> (ByteBuffer& buf, ObjectGuid&       guid);

ByteBuffer& operator<< (ByteBuffer& buf, PackedGuid const& guid);
ByteBuffer& operator>> (ByteBuffer& buf, PackedGuidReader const& guid);

inline PackedGuid ObjectGuid::WriteAsPacked() const { return PackedGuid(*this); }

HASH_NAMESPACE_START

    template<>
    class hash<ObjectGuid>
    {
        public:

            size_t operator() (ObjectGuid const& key) const
            {
                return hash<uint64>()(key.GetRawValue());
            }
    };

    // for pre-TR1 Visual Studio versions (VS90 SP1 or early)
    inline size_t hash_value(ObjectGuid const& key)
    {
        return hash_value(key.GetRawValue());
    }

HASH_NAMESPACE_END

#define DEFINE_READGUIDMASK(T1, T2) template <T1> \
    void ByteBuffer::ReadGuidMask(ObjectGuid& guid) \
    { \
        uint8 maskArr[] = { T2 }; \
        for (uint8 i = 0; i < countof(maskArr); ++i) \
            guid[maskArr[i]] = ReadBit(); \
    }

#define DEFINE_WRITEGUIDMASK(T1, T2) template <T1> \
    void ByteBuffer::WriteGuidMask(ObjectGuid& guid) \
    { \
        uint8 maskArr[] = { T2 }; \
        for (uint8 i = 0; i < countof(maskArr); ++i) \
            WriteBit(guid[maskArr[i]]); \
    }

#define DEFINE_READGUIDBYTES(T1, T2) template <T1> \
    void ByteBuffer::ReadGuidBytes(ObjectGuid& guid) \
    { \
        uint8 maskArr[] = { T2 }; \
        for (uint8 i = 0; i < countof(maskArr); ++i) \
            if (guid[maskArr[i]] != 0) \
                guid[maskArr[i]] ^= read<uint8>(); \
    }

#define DEFINE_WRITEGUIDBYTES(T1, T2) template <T1> \
    void ByteBuffer::WriteGuidBytes(ObjectGuid& guid) \
    { \
        uint8 maskArr[] = { T2 }; \
        for (uint8 i = 0; i < countof(maskArr); ++i) \
            if (guid[maskArr[i]] != 0) \
                (*this) << uint8(guid[maskArr[i]] ^ 1); \
    }

DEFINE_READGUIDMASK(BITS_1, BIT_VALS_1)
DEFINE_READGUIDMASK(BITS_2, BIT_VALS_2)
DEFINE_READGUIDMASK(BITS_3, BIT_VALS_3)
DEFINE_READGUIDMASK(BITS_4, BIT_VALS_4)
DEFINE_READGUIDMASK(BITS_5, BIT_VALS_5)
DEFINE_READGUIDMASK(BITS_6, BIT_VALS_6)
DEFINE_READGUIDMASK(BITS_7, BIT_VALS_7)
DEFINE_READGUIDMASK(BITS_8, BIT_VALS_8)

DEFINE_WRITEGUIDMASK(BITS_1, BIT_VALS_1)
DEFINE_WRITEGUIDMASK(BITS_2, BIT_VALS_2)
DEFINE_WRITEGUIDMASK(BITS_3, BIT_VALS_3)
DEFINE_WRITEGUIDMASK(BITS_4, BIT_VALS_4)
DEFINE_WRITEGUIDMASK(BITS_5, BIT_VALS_5)
DEFINE_WRITEGUIDMASK(BITS_6, BIT_VALS_6)
DEFINE_WRITEGUIDMASK(BITS_7, BIT_VALS_7)
DEFINE_WRITEGUIDMASK(BITS_8, BIT_VALS_8)

DEFINE_READGUIDBYTES(BITS_1, BIT_VALS_1)
DEFINE_READGUIDBYTES(BITS_2, BIT_VALS_2)
DEFINE_READGUIDBYTES(BITS_3, BIT_VALS_3)
DEFINE_READGUIDBYTES(BITS_4, BIT_VALS_4)
DEFINE_READGUIDBYTES(BITS_5, BIT_VALS_5)
DEFINE_READGUIDBYTES(BITS_6, BIT_VALS_6)
DEFINE_READGUIDBYTES(BITS_7, BIT_VALS_7)
DEFINE_READGUIDBYTES(BITS_8, BIT_VALS_8)

DEFINE_WRITEGUIDBYTES(BITS_1, BIT_VALS_1)
DEFINE_WRITEGUIDBYTES(BITS_2, BIT_VALS_2)
DEFINE_WRITEGUIDBYTES(BITS_3, BIT_VALS_3)
DEFINE_WRITEGUIDBYTES(BITS_4, BIT_VALS_4)
DEFINE_WRITEGUIDBYTES(BITS_5, BIT_VALS_5)
DEFINE_WRITEGUIDBYTES(BITS_6, BIT_VALS_6)
DEFINE_WRITEGUIDBYTES(BITS_7, BIT_VALS_7)
DEFINE_WRITEGUIDBYTES(BITS_8, BIT_VALS_8)

#endif
