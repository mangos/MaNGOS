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

#include <stdio.h>

#include "ProgressBar.h"
#include "Errors.h"

bool BarGoLink::m_showOutput = true;

char const* const BarGoLink::empty = " ";
#ifdef _WIN32
char const* const BarGoLink::full  = "\x3D";
#else
char const* const BarGoLink::full  = "*";
#endif

BarGoLink::BarGoLink(int row_count)
{
    init(row_count);
}

BarGoLink::BarGoLink(uint32 row_count)
{
    MANGOS_ASSERT(row_count < (uint32)ACE_INT32_MAX);
    init((int)row_count);
}

BarGoLink::BarGoLink(uint64 row_count)
{
    MANGOS_ASSERT(row_count < (uint64)ACE_INT32_MAX);
    init((int)row_count);
}

BarGoLink::~BarGoLink()
{
    if (!m_showOutput)
        return;

    printf( "\n" );
    fflush(stdout);
}

void BarGoLink::init(int row_count)
{
    rec_no    = 0;
    rec_pos   = 0;
    indic_len = 50;
    num_rec   = row_count;

    if (!m_showOutput)
        return;

    #ifdef _WIN32
    printf( "\x3D" );
    #else
    printf( "[" );
    #endif
    for ( int i = 0; i < indic_len; i++ ) printf( empty );
    #ifdef _WIN32
    printf( "\x3D 0%%\r\x3D" );
    #else
    printf( "] 0%%\r[" );
    #endif
    fflush(stdout);
}

void BarGoLink::step()
{
    if (!m_showOutput)
        return;

    int i, n;

    if ( num_rec == 0 ) return;
    ++rec_no;
    n = rec_no * indic_len / num_rec;
    if ( n != rec_pos )
    {
        #ifdef _WIN32
        printf( "\r\x3D" );
        #else
        printf( "\r[" );
        #endif
        for ( i = 0; i < n; i++ ) printf( full );
        for ( ; i < indic_len; i++ ) printf( empty );
        float percent = (((float)n/(float)indic_len)*100);
        #ifdef _WIN32
        printf( "\x3D %i%%  \r\x3D", (int)percent);
        #else
        printf( "] %i%%  \r[", (int)percent);
        #endif
        fflush(stdout);

        rec_pos = n;
    }
}

// avoid use inline version because linking problems with private static field
void BarGoLink::SetOutputState(bool on)
{
    m_showOutput = on;
}
