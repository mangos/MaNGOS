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

#include "DatabaseEnv.h"

SqlStmtParameters::SqlStmtParameters( int nParams )
{
    //reserve memory if needed
    if(nParams > 0)
        m_params.reserve(nParams);
}

void SqlStmtParameters::reset( const SqlStatement& stmt )
{
    m_params.clear();
    //reserve memory if needed
    if(stmt.arguments() > 0)
        m_params.reserve(stmt.arguments());
}

//////////////////////////////////////////////////////////////////////////
SqlStatement& SqlStatement::operator=( const SqlStatement& index )
{
    if(this != &index)
    {
        m_index = index.m_index;
        m_pDB = index.m_pDB;

        if(m_pParams)
        {
            delete m_pParams;
            m_pParams = NULL;
        }

        if(index.m_pParams)
            m_pParams = new SqlStmtParameters(*(index.m_pParams));
    }

    return *this;
}

bool SqlStatement::Execute()
{
    SqlStmtParameters * args = detach();
    //verify amount of bound parameters
    if(args->boundParams() != arguments())
    {
        sLog.outError("SQL ERROR: wrong amount of parameters (%i instead of %i)", args->boundParams(), arguments());
        sLog.outError("SQL ERROR: statement: %s", m_pDB->GetStmtString(ID()).c_str());
        MANGOS_ASSERT(false);
        return false;
    }

    return m_pDB->ExecuteStmt(m_index, args);
}

bool SqlStatement::DirectExecute()
{
    SqlStmtParameters * args = detach();
    //verify amount of bound parameters
    if(args->boundParams() != arguments())
    {
        sLog.outError("SQL ERROR: wrong amount of parameters (%i instead of %i)", args->boundParams(), arguments());
        sLog.outError("SQL ERROR: statement: %s", m_pDB->GetStmtString(ID()).c_str());
        MANGOS_ASSERT(false);
        return false;
    }

    return m_pDB->DirectExecuteStmt(m_index, args);
}

//////////////////////////////////////////////////////////////////////////
SqlPlainPreparedStatement::SqlPlainPreparedStatement( const std::string& fmt, SqlConnection& conn ) : SqlPreparedStatement(fmt, conn)
{
    m_bPrepared = true;
    m_nParams = std::count(m_szFmt.begin(), m_szFmt.end(), '?');
    m_bIsQuery = strnicmp(m_szFmt.c_str(), "select", 6) == 0;
}

void SqlPlainPreparedStatement::bind( const SqlStmtParameters& holder )
{
    //verify if we bound all needed input parameters
    if(m_nParams != holder.boundParams())
    {
        MANGOS_ASSERT(false);
        return;
    }

    //reset resulting plain SQL request
    m_szPlainRequest = m_szFmt;
    size_t nLastPos = 0;

    SqlStmtParameters::ParameterContainer const& _args = holder.params();

    SqlStmtParameters::ParameterContainer::const_iterator iter_last = _args.end();
    for (SqlStmtParameters::ParameterContainer::const_iterator iter = _args.begin(); iter != iter_last; ++iter)
    {
        //bind parameter
        const SqlStmtFieldData& data = (*iter);

        std::ostringstream fmt;
        DataToString(data, fmt);

        nLastPos = m_szPlainRequest.find('?', nLastPos);
        if(nLastPos != std::string::npos)
        {
            std::string tmp = fmt.str();
            m_szPlainRequest.replace(nLastPos, 1, tmp);
            nLastPos += tmp.length();
        }
    }
}

bool SqlPlainPreparedStatement::execute()
{
    if(m_szPlainRequest.empty())
        return false;

    return m_pConn.Execute(m_szPlainRequest.c_str());
}

void SqlPlainPreparedStatement::DataToString( const SqlStmtFieldData& data, std::ostringstream& fmt )
{
    switch (data.type())
    {
        case FIELD_BOOL:    fmt << "'" << uint32(data.toBool()) << "'";     break;
        case FIELD_UI8:     fmt << "'" << uint32(data.toUint8()) << "'";    break;
        case FIELD_UI16:    fmt << "'" << uint32(data.toUint16()) << "'";   break;
        case FIELD_UI32:    fmt << "'" << data.toUint32() << "'";           break;
        case FIELD_UI64:    fmt << "'" << data.toUint64() << "'";           break;
        case FIELD_I8:      fmt << "'" << int32(data.toInt8()) << "'";      break;
        case FIELD_I16:     fmt << "'" << int32(data.toInt16()) << "'";     break;
        case FIELD_I32:     fmt << "'" << data.toInt32() << "'";            break;
        case FIELD_I64:     fmt << "'" << data.toInt64() << "'";            break;
        case FIELD_FLOAT:   fmt << "'" << data.toFloat() << "'";            break;
        case FIELD_DOUBLE:  fmt << "'" << data.toDouble() << "'";           break;
        case FIELD_STRING:
        {
            std::string tmp = data.toStr();
            m_pConn.DB().escape_string(tmp);
            fmt << "'" << tmp << "'";
        }
        break;
    }
}
