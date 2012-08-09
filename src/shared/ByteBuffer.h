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

#ifndef _BYTEBUFFER_H
#define _BYTEBUFFER_H

#include "Common.h"
#include "Utilities/ByteConverter.h"

class ByteBufferException
{
    public:
        ByteBufferException(bool _add, size_t _pos, size_t _esize, size_t _size)
            : add(_add), pos(_pos), esize(_esize), size(_size)
        {
            PrintPosError();
        }

        void PrintPosError() const;
    private:
        bool add;
        size_t pos;
        size_t esize;
        size_t size;
};

template<class T>
struct Unused
{
    Unused() {}
};

class ByteBuffer
{
    public:
        const static size_t DEFAULT_SIZE = 64;

        // constructor
        ByteBuffer(): _rpos(0), _wpos(0), _bitpos(8), _curbitval(0)
        {
            _storage.reserve(DEFAULT_SIZE);
        }

        // constructor
        ByteBuffer(size_t res): _rpos(0), _wpos(0), _bitpos(8), _curbitval(0)
        {
            _storage.reserve(res);
        }

        // copy constructor
        ByteBuffer(const ByteBuffer& buf): _rpos(buf._rpos), _wpos(buf._wpos), _storage(buf._storage)
                                           , _bitpos(buf._bitpos), _curbitval(buf._curbitval) { }

        void clear()
        {
            _storage.clear();
            _rpos = _wpos = 0;
        }

        template <typename T> void put(size_t pos, T value)
        {
            EndianConvert(value);
            put(pos, (uint8*)&value, sizeof(value));
        }

        ByteBuffer& operator<<(uint8 value)
        {
            append<uint8>(value);
            return *this;
        }

        ByteBuffer& operator<<(uint16 value)
        {
            append<uint16>(value);
            return *this;
        }

        ByteBuffer& operator<<(uint32 value)
        {
            append<uint32>(value);
            return *this;
        }

        ByteBuffer& operator<<(uint64 value)
        {
            append<uint64>(value);
            return *this;
        }

        // signed as in 2e complement
        ByteBuffer& operator<<(int8 value)
        {
            append<int8>(value);
            return *this;
        }

        ByteBuffer& operator<<(int16 value)
        {
            append<int16>(value);
            return *this;
        }

        ByteBuffer& operator<<(int32 value)
        {
            append<int32>(value);
            return *this;
        }

        ByteBuffer& operator<<(int64 value)
        {
            append<int64>(value);
            return *this;
        }

        // floating points
        ByteBuffer& operator<<(float value)
        {
            append<float>(value);
            return *this;
        }

        ByteBuffer& operator<<(double value)
        {
            append<double>(value);
            return *this;
        }

        ByteBuffer& operator<<(const std::string& value)
        {
            append((uint8 const*)value.c_str(), value.length());
            append((uint8)0);
            return *this;
        }

        ByteBuffer& operator<<(const char* str)
        {
            append((uint8 const*)str, str ? strlen(str) : 0);
            append((uint8)0);
            return *this;
        }

        ByteBuffer& operator>>(bool& value)
        {
            value = read<char>() > 0 ? true : false;
            return *this;
        }

        ByteBuffer& operator>>(uint8& value)
        {
            value = read<uint8>();
            return *this;
        }

        ByteBuffer& operator>>(uint16& value)
        {
            value = read<uint16>();
            return *this;
        }

        ByteBuffer& operator>>(uint32& value)
        {
            value = read<uint32>();
            return *this;
        }

        ByteBuffer& operator>>(uint64& value)
        {
            value = read<uint64>();
            return *this;
        }

        // signed as in 2e complement
        ByteBuffer& operator>>(int8& value)
        {
            value = read<int8>();
            return *this;
        }

        ByteBuffer& operator>>(int16& value)
        {
            value = read<int16>();
            return *this;
        }

        ByteBuffer& operator>>(int32& value)
        {
            value = read<int32>();
            return *this;
        }

        ByteBuffer& operator>>(int64& value)
        {
            value = read<int64>();
            return *this;
        }

        ByteBuffer& operator>>(float& value)
        {
            value = read<float>();
            return *this;
        }

        ByteBuffer& operator>>(double& value)
        {
            value = read<double>();
            return *this;
        }

        ByteBuffer& operator>>(std::string& value)
        {
            value.clear();
            while (rpos() < size())                         // prevent crash at wrong string format in packet
            {
                char c = read<char>();
                if (c == 0)
                    break;
                value += c;
            }
            return *this;
        }

        template<class T>
        ByteBuffer& operator>>(Unused<T> const&)
        {
            return read_skip<T>();
        }


        uint8 operator[](size_t pos) const
        {
            return read<uint8>(pos);
        }

        size_t rpos() const { return _rpos; }

        size_t rpos(size_t rpos_)
        {
            _rpos = rpos_;
            return _rpos;
        }

        size_t wpos() const { return _wpos; }

        size_t wpos(size_t wpos_)
        {
            _wpos = wpos_;
            return _wpos;
        }

        template<typename T>
        ByteBuffer& read_skip()
        {
            read_skip(sizeof(T));
            return *this;
        }

        ByteBuffer& read_skip(size_t skip)
        {
            if (_rpos + skip > size())
                throw ByteBufferException(false, _rpos, skip, size());
            _rpos += skip;

            return *this;
        }

        template <typename T> T read()
        {
            T r = read<T>(_rpos);
            _rpos += sizeof(T);
            return r;
        }

        template <typename T> T read(size_t pos) const
        {
            if (pos + sizeof(T) > size())
                throw ByteBufferException(false, pos, sizeof(T), size());
            T val = *((T const*)&_storage[pos]);
            EndianConvert(val);
            return val;
        }

        ByteBuffer& read(uint8* dest, size_t len)
        {
            if (_rpos  + len > size())
                throw ByteBufferException(false, _rpos, len, size());
            memcpy(dest, &_storage[_rpos], len);
            _rpos += len;

            return *this;
        }

        uint64 readPackGUID()
        {
            uint64 guid = 0;
            uint8 guidmark = 0;
            (*this) >> guidmark;

            for (int i = 0; i < 8; ++i)
            {
                if (guidmark & (uint8(1) << i))
                {
                    uint8 bit;
                    (*this) >> bit;
                    guid |= (uint64(bit) << (i * 8));
                }
            }

            return guid;
        }

        std::string ReadString(uint32 count)
        {
            std::string s;
            for (uint32 i = 0; i < count; i++)
                s += read<char>();

            return s;
        }

        const uint8* contents() const { return &_storage[0]; }

        size_t size() const { return _storage.size(); }
        bool empty() const { return _storage.empty(); }

        void resize(size_t newsize)
        {
            _storage.resize(newsize);
            _rpos = 0;
            _wpos = size();
        }

        void reserve(size_t ressize)
        {
            if (ressize > size())
                _storage.reserve(ressize);
        }

        ByteBuffer& append(const std::string& str)
        {
            return append((uint8 const*)str.c_str(), str.size() + 1);
        }

        ByteBuffer& append(const char* src, size_t cnt)
        {
            return append((const uint8*)src, cnt);
        }

        template<class T> ByteBuffer& append(const T* src, size_t cnt)
        {
            return append((const uint8*)src, cnt * sizeof(T));
        }

        ByteBuffer& append(const uint8* src, size_t cnt)
        {
            if (!cnt)
                return *this;

            MANGOS_ASSERT(size() < 10000000);

            if (_storage.size() < _wpos + cnt)
                _storage.resize(_wpos + cnt);
            memcpy(&_storage[_wpos], src, cnt);
            _wpos += cnt;

            return *this;
        }

        ByteBuffer& append(const ByteBuffer& buffer)
        {
            if (buffer.wpos())
                return append(buffer.contents(), buffer.wpos());

            return *this;
        }

        // can be used in SMSG_MONSTER_MOVE opcode
        ByteBuffer& appendPackXYZ(float x, float y, float z)
        {
            uint32 packed = 0;
            packed |= ((int)(x / 0.25f) & 0x7FF);
            packed |= ((int)(y / 0.25f) & 0x7FF) << 11;
            packed |= ((int)(z / 0.25f) & 0x3FF) << 22;
            *this << packed;

            return *this;
        }

        ByteBuffer& appendPackGUID(uint64 guid)
        {
            uint8 packGUID[8 + 1];
            packGUID[0] = 0;
            size_t size = 1;
            for (uint8 i = 0; guid != 0; ++i)
            {
                if (guid & 0xFF)
                {
                    packGUID[0] |= uint8(1 << i);
                    packGUID[size] =  uint8(guid & 0xFF);
                    ++size;
                }

                guid >>= 8;
            }

            return append(packGUID, size);
        }

        void put(size_t pos, const uint8* src, size_t cnt)
        {
            if (pos + cnt > size())
                throw ByteBufferException(true, pos, cnt, size());
            memcpy(&_storage[pos], src, cnt);
        }

        bool ReadBit()
        {
            ++_bitpos;
            if (_bitpos > 7)
            {
                _bitpos = 0;
                _curbitval = read<uint8>();
            }

            return ((_curbitval >> (7-_bitpos)) & 1) != 0;
        }

        template <typename T> bool WriteBit(T bit)
        {
            --_bitpos;
            if (bit)
                _curbitval |= (1 << (_bitpos));

            if (_bitpos == 0)
            {
                _bitpos = 8;
                append((uint8 *)&_curbitval, sizeof(_curbitval));
                _curbitval = 0;
            }

            return (bit != 0);
        }

        uint32 ReadBits(size_t bits)
        {
            uint32 value = 0;
            for (int32 i = bits-1; i >= 0; --i)
                if (ReadBit())
                    value |= (1 << i);

            return value;
        }

        uint64 ReadGuid(uint8* mask, uint8* bytes)
        {
            uint8 guidMask[8] = { 0 };
            uint8 guidBytes[8]= { 0 };

            for (int i = 0; i < 8; i++)
                guidMask[i] = ReadBit();

            for (uint8 i = 0; i < 8; i++)
                if (guidMask[mask[i]])
                    guidBytes[bytes[i]] = uint8(read<uint8>() ^ 1);

            uint64 guid = guidBytes[0];
            for (int i = 1; i < 8; i++)
                guid |= ((uint64)guidBytes[i]) << (i * 8);

            return guid;
        }

        template <typename T> void WriteBits(T value, size_t bits)
        {
            for (int32 i = bits-1; i >= 0; --i)
                WriteBit((value >> i) & 1);
        }

        void WriteGuidMask(uint64 guid, uint8* maskOrder, uint8 maskCount, uint8 maskPos = 0)
        {
            uint8* guidByte = ((uint8*)&guid);

            for (uint8 i = 0; i < maskCount; i++)
                WriteBit(guidByte[maskOrder[i + maskPos]]);
        }

        void WriteGuidBytes(uint64 guid, uint8* byteOrder, uint8 byteCount, uint8 bytePos)
        {
            uint8* guidByte = ((uint8*)&guid);

            for (uint8 i = 0; i < byteCount; i++)
                if (guidByte[byteOrder[i + bytePos]])
                    (*this) << uint8(guidByte[byteOrder[i + bytePos]] ^ 1);
        }

        void FlushBits()
        {
            if (_bitpos == 8)
                return;

            append((uint8 *)&_curbitval, sizeof(uint8));
            _curbitval = 0;
            _bitpos = 8;
        }

        void print_storage() const
        {
            if (!sLog.HasLogLevelOrHigher(LOG_LVL_DEBUG))   // optimize disabled debug output
                return;

            std::ostringstream ss;
            ss <<  "STORAGE_SIZE: " << size() << "\n";

            if (sLog.IsIncludeTime())
                ss << "         ";

            for (size_t i = 0; i < size(); ++i)
                ss << uint32(read<uint8>(i)) << " - ";

            sLog.outDebug(ss.str().c_str());
        }

        void textlike() const
        {
            if (!sLog.HasLogLevelOrHigher(LOG_LVL_DEBUG))   // optimize disabled debug output
                return;

            std::ostringstream ss;
            ss <<  "STORAGE_SIZE: " << size() << "\n";

            if (sLog.IsIncludeTime())
                ss << "         ";

            for (size_t i = 0; i < size(); ++i)
                ss << read<uint8>(i);

            sLog.outDebug(ss.str().c_str());
        }

        void hexlike() const
        {
            if (!sLog.HasLogLevelOrHigher(LOG_LVL_DEBUG))   // optimize disabled debug output
                return;

            std::ostringstream ss;
            ss <<  "STORAGE_SIZE: " << size() << "\n";

            if (sLog.IsIncludeTime())
                ss << "         ";

            size_t j = 1, k = 1;

            for (size_t i = 0; i < size(); ++i)
            {
                if ((i == (j * 8)) && ((i != (k * 16))))
                {
                    ss << "| ";
                    ++j;
                }
                else if (i == (k * 16))
                {
                    ss << "\n";

                    if (sLog.IsIncludeTime())
                        ss << "         ";

                    ++k;
                    ++j;
                }

                char buf[4];
                snprintf(buf, 4, "%02X", read<uint8>(i));
                ss << buf << " ";

            }
            sLog.outDebug(ss.str().c_str());
        }

    private:
        // limited for internal use because can "append" any unexpected type (like pointer and etc) with hard detection problem
        template <typename T> ByteBuffer& append(T value)
        {
            EndianConvert(value);
            return append((uint8*)&value, sizeof(value));
        }

    protected:
        uint8 _curbitval;
        size_t _rpos, _wpos, _bitpos;
        std::vector<uint8> _storage;
};

template <typename T>
inline ByteBuffer& operator<<(ByteBuffer& b, std::vector<T> const& v)
{
    b << (uint32)v.size();
    for (typename std::vector<T>::iterator i = v.begin(); i != v.end(); ++i)
    {
        b << *i;
    }
    return b;
}

template <typename T>
inline ByteBuffer& operator>>(ByteBuffer& b, std::vector<T>& v)
{
    uint32 vsize;
    b >> vsize;
    v.clear();
    while (vsize--)
    {
        T t;
        b >> t;
        v.push_back(t);
    }
    return b;
}

template <typename T>
inline ByteBuffer& operator<<(ByteBuffer& b, std::list<T> const& v)
{
    b << (uint32)v.size();
    for (typename std::list<T>::iterator i = v.begin(); i != v.end(); ++i)
    {
        b << *i;
    }
    return b;
}

template <typename T>
inline ByteBuffer& operator>>(ByteBuffer& b, std::list<T>& v)
{
    uint32 vsize;
    b >> vsize;
    v.clear();
    while (vsize--)
    {
        T t;
        b >> t;
        v.push_back(t);
    }
    return b;
}

template <typename K, typename V>
inline ByteBuffer& operator<<(ByteBuffer& b, std::map<K, V>& m)
{
    b << (uint32)m.size();
    for (typename std::map<K, V>::iterator i = m.begin(); i != m.end(); ++i)
    {
        b << i->first << i->second;
    }
    return b;
}

template <typename K, typename V>
inline ByteBuffer& operator>>(ByteBuffer& b, std::map<K, V>& m)
{
    uint32 msize;
    b >> msize;
    m.clear();
    while (msize--)
    {
        K k;
        V v;
        b >> k >> v;
        m.insert(make_pair(k, v));
    }
    return b;
}

template<>
inline ByteBuffer& ByteBuffer::read_skip<char*>()
{
    std::string temp;
    *this >> temp;

    return *this;
}

template<>
inline ByteBuffer& ByteBuffer::read_skip<char const*>()
{
    return read_skip<char*>();
}

template<>
inline ByteBuffer& ByteBuffer::read_skip<std::string>()
{
    return read_skip<char*>();
}
#endif
