/*
* Copyright (C) 2009 MaNGOS <http://getmangos.com/>
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

//lets use Intel scalable_allocator by default and
//switch to OS specific allocator only when _STANDARD_MALLOC is defined
#ifndef USE_STANDARD_MALLOC

#include "../../dep/tbb/include/tbb/scalable_allocator.h"

void * operator new(size_t sz) throw (std::bad_alloc)
{
    void *res = scalable_malloc(sz);
    if (NULL == res) throw std::bad_alloc();
    return res;
}

void* operator new[](size_t sz) throw (std::bad_alloc)
{
    void *res = scalable_malloc(sz);
    if (NULL == res) throw std::bad_alloc();
    return res;
}

void operator delete(void* ptr) throw()
{
    scalable_free(ptr);
}

void operator delete[](void* ptr) throw()
{
    scalable_free(ptr);
}

void* operator new(size_t sz, const std::nothrow_t&) throw()
{
    return scalable_malloc(sz);
}

void* operator new[](size_t sz, const std::nothrow_t&) throw()
{
    return scalable_malloc(sz);
}

void operator delete(void* ptr, const std::nothrow_t&) throw()
{
    scalable_free(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) throw()
{
    scalable_free(ptr);
}

#endif
