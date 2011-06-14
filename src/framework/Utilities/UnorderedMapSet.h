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

#ifndef MANGOS_UNORDERED_MAP_H
#define MANGOS_UNORDERED_MAP_H

#include "Platform/CompilerDefs.h"
#include "Platform/Define.h"

#if COMPILER == COMPILER_INTEL
#  include <ext/hash_map>
#  include <ext/hash_set>
#elif COMPILER == COMPILER_GNU && (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#  include <tr1/unordered_map>
#  include <tr1/unordered_set>
#elif COMPILER == COMPILER_GNU && __GNUC__ >= 3
#  include <ext/hash_map>
#  include <ext/hash_set>
#elif COMPILER == COMPILER_MICROSOFT && _MSC_VER >= 1500 && _HAS_TR1   // VC9.0 SP1 and later
#  include <unordered_map>
#  include <unordered_set>
#else
#  include <hash_map>
#  include <hash_set>
#endif

#ifdef _STLPORT_VERSION
#  define UNORDERED_MAP std::hash_map
#  define UNORDERED_SET std::hash_set
#  define HASH_NAMESPACE_START namespace std {
#  define HASH_NAMESPACE_END }
using std::hash_map;
using std::hash_set;
#elif COMPILER == COMPILER_MICROSOFT && _MSC_VER >= 1600    // VS100
#  define UNORDERED_MAP std::tr1::unordered_map
#  define UNORDERED_SET std::tr1::unordered_set
#  define HASH_NAMESPACE_START namespace std {
#  define HASH_NAMESPACE_END }
#elif COMPILER == COMPILER_MICROSOFT && _MSC_VER >= 1500 && _HAS_TR1
#  define UNORDERED_MAP std::tr1::unordered_map
#  define UNORDERED_SET std::tr1::unordered_set
#  define HASH_NAMESPACE_START namespace std { namespace tr1 {
#  define HASH_NAMESPACE_END } }
#elif COMPILER == COMPILER_MICROSOFT && _MSC_VER >= 1300
#  define UNORDERED_MAP stdext::hash_map
#  define UNORDERED_SET stdext::hash_set
#  define HASH_NAMESPACE_START namespace stdext {
#  define HASH_NAMESPACE_END }
using stdext::hash_map;
using stdext::hash_set;

#if !_HAS_TRADITIONAL_STL

// can be not used by some platforms, so provide fake forward
HASH_NAMESPACE_START

template<class K>
class hash
{
    public:
        size_t operator() (K const&);
};

HASH_NAMESPACE_END

#endif

#elif COMPILER == COMPILER_INTEL
#  define UNORDERED_MAP std::hash_map
#  define UNORDERED_SET std::hash_set
#  define HASH_NAMESPACE_START namespace std {
#  define HASH_NAMESPACE_END }
using std::hash_map;
using std::hash_set;
#elif COMPILER == COMPILER_GNU && (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#  define UNORDERED_MAP std::tr1::unordered_map
#  define UNORDERED_SET std::tr1::unordered_set
#  define HASH_NAMESPACE_START namespace std { namespace tr1 {
#  define HASH_NAMESPACE_END } }
#elif COMPILER == COMPILER_GNU && __GNUC__ >= 3
#  define UNORDERED_MAP __gnu_cxx::hash_map
#  define UNORDERED_SET __gnu_cxx::hash_set
#  define HASH_NAMESPACE_START namespace __gnu_cxx {
#  define HASH_NAMESPACE_END }

HASH_NAMESPACE_START

    template<>
    class hash<unsigned long long>
    {
        public:
            size_t operator()(const unsigned long long &__x) const { return (size_t)__x; }
    };

    template<typename T>
    class hash<T *>
    {
        public:
            size_t operator()(T * const &__x) const { return (size_t)__x; }
    };

    template<> struct hash<std::string>
    {
        size_t operator()(const std::string &__x) const
        {
            return hash<const char *>()(__x.c_str());
        }
    };

HASH_NAMESPACE_END

#else
#  define UNORDERED_MAP std::hash_map
#  define UNORDERED_SET std::hash_set
#  define HASH_NAMESPACE_START namespace std {
#  define HASH_NAMESPACE_END }
using std::hash_map;
using std::hash_set;
#endif

#endif
