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

#if !defined(QUERYRESULTPOSTGRE_H)
#define QUERYRESULTPOSTGRE_H

#ifdef WIN32
#define FD_SETSIZE 1024
#include <winsock2.h>
#include <postgre/libpq-fe.h>
#include <postgre/pg_type.h>
#else
// Define OID's from pg_type.h in postgresql server includes.
#define BOOLOID 16
#define BYTEAOID 17
#define CHAROID 18
#define NAMEOID 19
#define INT8OID 20
#define INT2OID 21
#define INT2VECTOROID 22
#define INT4OID 23
#define REGPROCOID 24
#define TEXTOID 25
#define OIDOID 26
#define TIDOID 27
#define XIDOID 28
#define CIDOID 29
#define OIDVECTOROID 30
#define POINTOID 600
#define LSEGOID 601
#define PATHOID 602
#define BOXOID 603
#define POLYGONOID 604
#define LINEOID 628
#define FLOAT4OID 700
#define FLOAT8OID 701
#define ABSTIMEOID 702
#define RELTIMEOID 703
#define TINTERVALOID 704
#define UNKNOWNOID 705
#define CIRCLEOID 718
#define CASHOID 790
#define INETOID 869
#define CIDROID 650
#define BPCHAROID 1042
#define VARCHAROID 1043
#define DATEOID 1082
#define TIMEOID 1083
#define TIMESTAMPOID 1114
#define TIMESTAMPTZOID 1184
#define INTERVALOID 1186
#define TIMETZOID 1266
#define BITOID 1560
#define VARBITOID 1562
#define NUMERICOID 1700
#include <libpq-fe.h>
#endif

class QueryResultPostgre : public QueryResult
{
    public:
        QueryResultPostgre(PGresult *result, uint64 rowCount, uint32 fieldCount);

        ~QueryResultPostgre();

        bool NextRow();

    private:
        enum Field::DataTypes ConvertNativeType(Oid pOid) const;
        void EndQuery();

        PGresult *mResult;
        uint32 mTableIndex;
};
#endif
