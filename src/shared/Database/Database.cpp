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
#include "Config/Config.h"
#include "Database/SqlOperations.h"

#include <ctime>
#include <iostream>
#include <fstream>
#include <memory>

#define MIN_CONNECTION_POOL_SIZE 1
#define MAX_CONNECTION_POOL_SIZE 16

//////////////////////////////////////////////////////////////////////////
SqlPreparedStatement * SqlConnection::CreateStatement( const std::string& fmt )
{
    return new SqlPlainPreparedStatement(fmt, *this);
}

void SqlConnection::FreePreparedStatements()
{
    SqlConnection::Lock guard(this);

    size_t nStmts = m_holder.size();
    for (size_t i = 0; i < nStmts; ++i)
        delete m_holder[i];

    m_holder.clear();
}

SqlPreparedStatement * SqlConnection::GetStmt( int nIndex )
{
    if(nIndex < 0)
        return NULL;

    //resize stmt container
    if(m_holder.size() <= nIndex)
        m_holder.resize(nIndex + 1, NULL);

    SqlPreparedStatement * pStmt = NULL;

    //create stmt if needed
    if(m_holder[nIndex] == NULL)
    {
        //obtain SQL request string
        std::string fmt = m_db.GetStmtString(nIndex);
        MANGOS_ASSERT(fmt.length());
        //allocate SQlPreparedStatement object
        pStmt = CreateStatement(fmt);
        //prepare statement
        if(!pStmt->prepare())
        {
            MANGOS_ASSERT(false && "Unable to prepare SQL statement");
            return NULL;
        }

        //save statement in internal registry
        m_holder[nIndex] = pStmt;
    }
    else
        pStmt = m_holder[nIndex];

    return pStmt;
}

bool SqlConnection::ExecuteStmt(int nIndex, const SqlStmtParameters& id )
{
    if(nIndex == -1)
        return false;

    //get prepared statement object
    SqlPreparedStatement * pStmt = GetStmt(nIndex);
    //bind parameters
    pStmt->bind(id);
    //execute statement
    return pStmt->execute();
}

//////////////////////////////////////////////////////////////////////////
Database::~Database()
{
    StopServer();
}

bool Database::Initialize(const char * infoString, int nConns /*= 1*/)
{
    // Enable logging of SQL commands (usually only GM commands)
    // (See method: PExecuteLog)
    m_logSQL = sConfig.GetBoolDefault("LogSQL", false);
    m_logsDir = sConfig.GetStringDefault("LogsDir","");
    if(!m_logsDir.empty())
    {
        if((m_logsDir.at(m_logsDir.length()-1)!='/') && (m_logsDir.at(m_logsDir.length()-1)!='\\'))
            m_logsDir.append("/");
    }

    m_pingIntervallms = sConfig.GetIntDefault ("MaxPingTime", 30) * (MINUTE * 1000);

    //create DB connections

    //setup connection pool size
    if(nConns < MIN_CONNECTION_POOL_SIZE)
        m_nQueryConnPoolSize = MIN_CONNECTION_POOL_SIZE;
    else if(nConns > MAX_CONNECTION_POOL_SIZE)
        m_nQueryConnPoolSize = MAX_CONNECTION_POOL_SIZE;
    else
        m_nQueryConnPoolSize = nConns;

    //create connection pool for sync requests
    for (int i = 0; i < m_nQueryConnPoolSize; ++i)
    {
        SqlConnection * pConn = CreateConnection();
        if(!pConn->Initialize(infoString))
        {
            delete pConn;
            return false;
        }

        m_pQueryConnections.push_back(pConn);
    }

    //create and initialize connection for async requests
    m_pAsyncConn = CreateConnection();
    if(!m_pAsyncConn->Initialize(infoString))
        return false;

    m_pResultQueue = new SqlResultQueue;

    InitDelayThread();
    return true;
}

void Database::StopServer()
{
    HaltDelayThread();
    /*Delete objects*/
    if(m_pResultQueue)
    {
        delete m_pResultQueue;
        m_pResultQueue = NULL;
    }

    if(m_pAsyncConn)
    {
        delete m_pAsyncConn;
        m_pAsyncConn = NULL;
    }

    for (size_t i = 0; i < m_pQueryConnections.size(); ++i)
        delete m_pQueryConnections[i];

    m_pQueryConnections.clear();

}

SqlDelayThread * Database::CreateDelayThread()
{ 
    assert(m_pAsyncConn);
    return new SqlDelayThread(this, m_pAsyncConn);
}

void Database::InitDelayThread()
{
    assert(!m_delayThread);

    //New delay thread for delay execute
    m_threadBody = CreateDelayThread();              // will deleted at m_delayThread delete
    m_delayThread = new ACE_Based::Thread(m_threadBody);
}

void Database::HaltDelayThread()
{
    if (!m_threadBody || !m_delayThread) return;

    m_threadBody->Stop();                                   //Stop event
    m_delayThread->wait();                                  //Wait for flush to DB
    delete m_delayThread;                                   //This also deletes m_threadBody
    m_delayThread = NULL;
    m_threadBody = NULL;
}

void Database::ThreadStart()
{
}

void Database::ThreadEnd()
{
}

void Database::ProcessResultQueue()
{
    if(m_pResultQueue)
        m_pResultQueue->Update();
}

void Database::escape_string(std::string& str)
{
    if(str.empty())
        return;

    char* buf = new char[str.size()*2+1];
    //we don't care what connection to use - escape string will be the same
    m_pQueryConnections[0]->escape_string(buf,str.c_str(),str.size());
    str = buf;
    delete[] buf;
}

SqlConnection * Database::getQueryConnection()
{
    int nCount = 0;

    if(m_nQueryCounter == long(1 << 31))
        m_nQueryCounter = 0;
    else
        nCount = ++m_nQueryCounter;

    return m_pQueryConnections[nCount % m_nQueryConnPoolSize];
}

void Database::Ping()
{
    const char * sql = "SELECT 1";

    {
        SqlConnection::Lock guard(m_pAsyncConn);
        delete guard->Query(sql);
    }

    for (int i = 0; i < m_nQueryConnPoolSize; ++i)
    {
        SqlConnection::Lock guard(m_pQueryConnections[i]);
        delete guard->Query(sql);
    }
}

bool Database::PExecuteLog(const char * format,...)
{
    if (!format)
        return false;

    va_list ap;
    char szQuery [MAX_QUERY_LEN];
    va_start(ap, format);
    int res = vsnprintf( szQuery, MAX_QUERY_LEN, format, ap );
    va_end(ap);

    if(res==-1)
    {
        sLog.outError("SQL Query truncated (and not execute) for format: %s",format);
        return false;
    }

    if( m_logSQL )
    {
        time_t curr;
        tm local;
        time(&curr);                                        // get current time_t value
        local=*(localtime(&curr));                          // dereference and assign
        char fName[128];
        sprintf( fName, "%04d-%02d-%02d_logSQL.sql", local.tm_year+1900, local.tm_mon+1, local.tm_mday );

        FILE* log_file;
        std::string logsDir_fname = m_logsDir+fName;
        log_file = fopen(logsDir_fname.c_str(), "a");
        if (log_file)
        {
            fprintf(log_file, "%s;\n", szQuery);
            fclose(log_file);
        }
        else
        {
            // The file could not be opened
            sLog.outError("SQL-Logging is disabled - Log file for the SQL commands could not be openend: %s",fName);
        }
    }

    return Execute(szQuery);
}

QueryResult* Database::PQuery(const char *format,...)
{
    if(!format) return NULL;

    va_list ap;
    char szQuery [MAX_QUERY_LEN];
    va_start(ap, format);
    int res = vsnprintf( szQuery, MAX_QUERY_LEN, format, ap );
    va_end(ap);

    if(res==-1)
    {
        sLog.outError("SQL Query truncated (and not execute) for format: %s",format);
        return false;
    }

    return Query(szQuery);
}

QueryNamedResult* Database::PQueryNamed(const char *format,...)
{
    if(!format) return NULL;

    va_list ap;
    char szQuery [MAX_QUERY_LEN];
    va_start(ap, format);
    int res = vsnprintf( szQuery, MAX_QUERY_LEN, format, ap );
    va_end(ap);

    if(res==-1)
    {
        sLog.outError("SQL Query truncated (and not execute) for format: %s",format);
        return false;
    }

    return QueryNamed(szQuery);
}

bool Database::Execute(const char *sql)
{
    if (!m_pAsyncConn)
        return false;

    SqlTransaction * pTrans = m_TransStorage->get();
    if(pTrans)
    {
        //add SQL request to trans queue
        pTrans->DelayExecute(new SqlPlainRequest(sql));
    }
    else
    {
        //if async execution is not available
        if(!m_bAllowAsyncTransactions)
            return DirectExecute(sql);

        // Simple sql statement
        m_threadBody->Delay(new SqlPlainRequest(sql));
    }

    return true;
}

bool Database::PExecute(const char * format,...)
{
    if (!format)
        return false;

    va_list ap;
    char szQuery [MAX_QUERY_LEN];
    va_start(ap, format);
    int res = vsnprintf( szQuery, MAX_QUERY_LEN, format, ap );
    va_end(ap);

    if(res==-1)
    {
        sLog.outError("SQL Query truncated (and not execute) for format: %s",format);
        return false;
    }

    return Execute(szQuery);
}

bool Database::DirectPExecute(const char * format,...)
{
    if (!format)
        return false;

    va_list ap;
    char szQuery [MAX_QUERY_LEN];
    va_start(ap, format);
    int res = vsnprintf( szQuery, MAX_QUERY_LEN, format, ap );
    va_end(ap);

    if(res==-1)
    {
        sLog.outError("SQL Query truncated (and not execute) for format: %s",format);
        return false;
    }

    return DirectExecute(szQuery);
}

bool Database::BeginTransaction()
{
    if (!m_pAsyncConn)
        return false;

    //initiate transaction on current thread
    //currently we do not support queued transactions
    m_TransStorage->init();
    return true;
}

bool Database::CommitTransaction()
{
    if (!m_pAsyncConn)
        return false;

    //check if we have pending transaction
    if(!m_TransStorage->get())
        return false;

    //if async execution is not available
    if(!m_bAllowAsyncTransactions)
        return CommitTransactionDirect();

    //add SqlTransaction to the async queue
    m_threadBody->Delay(m_TransStorage->detach());
    return true;
}

bool Database::CommitTransactionDirect()
{
    if (!m_pAsyncConn)
        return false;

    //check if we have pending transaction
    if(!m_TransStorage->get())
        return false;

    //directly execute SqlTransaction
    SqlTransaction * pTrans = m_TransStorage->detach();
    pTrans->Execute(m_pAsyncConn);
    delete pTrans;

    return true;
}

bool Database::RollbackTransaction()
{
    if (!m_pAsyncConn)
        return false;

    if(!m_TransStorage->get())
        return false;

    //remove scheduled transaction
    m_TransStorage->reset();

    return true;
}

bool Database::CheckRequiredField( char const* table_name, char const* required_name )
{
    // check required field
    QueryResult* result = PQuery("SELECT %s FROM %s LIMIT 1",required_name,table_name);
    if(result)
    {
        delete result;
        return true;
    }

    // check fail, prepare readabale error message

    // search current required_* field in DB
    const char* db_name;
    if(!strcmp(table_name, "db_version"))
        db_name = "WORLD";
    else if(!strcmp(table_name, "character_db_version"))
        db_name = "CHARACTER";
    else if(!strcmp(table_name, "realmd_db_version"))
        db_name = "REALMD";
    else
        db_name = "UNKNOWN";

    char const* req_sql_update_name = required_name+strlen("required_");

    QueryNamedResult* result2 = PQueryNamed("SELECT * FROM %s LIMIT 1",table_name);
    if(result2)
    {
        QueryFieldNames const& namesMap = result2->GetFieldNames();
        std::string reqName;
        for(QueryFieldNames::const_iterator itr = namesMap.begin(); itr != namesMap.end(); ++itr)
        {
            if(itr->substr(0,9)=="required_")
            {
                reqName = *itr;
                break;
            }
        }

        delete result2;

        std::string cur_sql_update_name = reqName.substr(strlen("required_"),reqName.npos);

        if(!reqName.empty())
        {
            sLog.outErrorDb("The table `%s` in your [%s] database indicates that this database is out of date!",table_name,db_name);
            sLog.outErrorDb();
            sLog.outErrorDb("  [A] You have: --> `%s.sql`",cur_sql_update_name.c_str());
            sLog.outErrorDb();
            sLog.outErrorDb("  [B] You need: --> `%s.sql`",req_sql_update_name);
            sLog.outErrorDb();
            sLog.outErrorDb("You must apply all updates after [A] to [B] to use mangos with this database.");
            sLog.outErrorDb("These updates are included in the sql/updates folder.");
            sLog.outErrorDb("Please read the included [README] in sql/updates for instructions on updating.");
        }
        else
        {
            sLog.outErrorDb("The table `%s` in your [%s] database is missing its version info.",table_name,db_name);
            sLog.outErrorDb("MaNGOS cannot find the version info needed to check that the db is up to date.");
            sLog.outErrorDb();
            sLog.outErrorDb("This revision of MaNGOS requires a database updated to:");
            sLog.outErrorDb("`%s.sql`",req_sql_update_name);
            sLog.outErrorDb();

            if(!strcmp(db_name, "WORLD"))
                sLog.outErrorDb("Post this error to your database provider forum or find a solution there.");
            else
                sLog.outErrorDb("Reinstall your [%s] database with the included sql file in the sql folder.",db_name);
        }
    }
    else
    {
        sLog.outErrorDb("The table `%s` in your [%s] database is missing or corrupt.",table_name,db_name);
        sLog.outErrorDb("MaNGOS cannot find the version info needed to check that the db is up to date.");
        sLog.outErrorDb();
        sLog.outErrorDb("This revision of mangos requires a database updated to:");
        sLog.outErrorDb("`%s.sql`",req_sql_update_name);
        sLog.outErrorDb();

        if(!strcmp(db_name, "WORLD"))
            sLog.outErrorDb("Post this error to your database provider forum or find a solution there.");
        else
            sLog.outErrorDb("Reinstall your [%s] database with the included sql file in the sql folder.",db_name);
    }

    return false;
}

bool Database::ExecuteStmt( const SqlStatementID& id, SqlStmtParameters * params )
{
    if (!m_pAsyncConn)
        return false;

    SqlTransaction * pTrans = m_TransStorage->get();
    if(pTrans)
    {
        //add SQL request to trans queue
        pTrans->DelayExecute(new SqlPreparedRequest(id.ID(), params));
    }
    else
    {
        //if async execution is not available
        if(!m_bAllowAsyncTransactions)
            return DirectExecuteStmt(id, params);

        // Simple sql statement
        m_threadBody->Delay(new SqlPreparedRequest(id.ID(), params));
    }

    return true;
}

bool Database::DirectExecuteStmt( const SqlStatementID& id, SqlStmtParameters * params )
{
    MANGOS_ASSERT(params);
    std::auto_ptr<SqlStmtParameters> p(params);
    //execute statement
    SqlConnection::Lock _guard(getAsyncConnection());
    return _guard->ExecuteStmt(id.ID(), *params);
}

SqlStatement Database::CreateStatement(SqlStatementID& index, const char * fmt )
{
    int nId = -1;
    //check if statement ID is initialized
    if(!index.initialized())
    {
        //convert to lower register
        std::string szFmt(fmt);
        //count input parameters
        int nParams = std::count(szFmt.begin(), szFmt.end(), '?');
        //find existing or add a new record in registry
        LOCK_GUARD _guard(m_stmtGuard);
        PreparedStmtRegistry::const_iterator iter = m_stmtRegistry.find(szFmt);
        if(iter == m_stmtRegistry.end())
        {
            nId = ++m_iStmtIndex;
            m_stmtRegistry[szFmt] = nId;
        }
        else
            nId = iter->second;

        //save initialized statement index info
        index.init(nId, nParams);
    }

    return SqlStatement(index, *this);
}

std::string Database::GetStmtString(const int stmtId) const
{
    LOCK_GUARD _guard(m_stmtGuard);
    
    if(stmtId == -1 || stmtId > m_iStmtIndex)
        return std::string();

    PreparedStmtRegistry::const_iterator iter_last = m_stmtRegistry.end();
    for(PreparedStmtRegistry::const_iterator iter = m_stmtRegistry.begin(); iter != iter_last; ++iter)
    {
        if(iter->second == stmtId)
            return iter->first;
    }

    return std::string();
}

//HELPER CLASSES AND FUNCTIONS
Database::TransHelper::~TransHelper()
{
    reset();
}

SqlTransaction * Database::TransHelper::init()
{
    MANGOS_ASSERT(!m_pTrans);   //if we will get a nested transaction request - we MUST fix code!!!
    m_pTrans = new SqlTransaction;
    return m_pTrans;
}

SqlTransaction * Database::TransHelper::detach()
{
    SqlTransaction * pRes = m_pTrans;
    m_pTrans = NULL;
    return pRes;
}

void Database::TransHelper::reset()
{
    if(m_pTrans)
    {
        delete m_pTrans;
        m_pTrans = NULL;
    }
}
