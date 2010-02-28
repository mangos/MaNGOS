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

/** \file
    \ingroup mangosd
*/

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "RASocket.h"
#include "World.h"
#include "Config/ConfigEnv.h"
#include "Util.h"
#include "AccountMgr.h"
#include "Language.h"
#include "ObjectMgr.h"

// TODO: drop old socket library and implement RASocket using ACE

/// RASocket constructor
RASocket::RASocket(ISocketHandler &h): TcpSocket(h)
{

    ///- Get the config parameters
    bSecure = sConfig.GetBoolDefault( "RA.Secure", true );
    iMinLevel = sConfig.GetIntDefault( "RA.MinLevel", 3 );

    ///- Initialize buffer and data
    iInputLength=0;
    stage=NONE;
}

/// RASocket destructor
RASocket::~RASocket()
{
    sLog.outRALog("Connection was closed.\n");
}

/// Accept an incoming connection
void RASocket::OnAccept()
{
    std::string ss=GetRemoteAddress();
    sLog.outRALog("Incoming connection from %s.\n",ss.c_str());

    ///- print Motd
    Sendf("%s\r\n",sWorld.GetMotd());
    Sendf("\r\n%s",sObjectMgr.GetMangosStringForDBCLocale(LANG_RA_USER));
}

/// Read data from the network
void RASocket::OnRead()
{
    ///- Read data and check input length
    TcpSocket::OnRead();

    unsigned int sz=ibuf.GetLength();
    if (iInputLength+sz>=RA_BUFF_SIZE)
    {
        sLog.outRALog("Input buffer overflow, possible DOS attack.\n");
        SetCloseAndDelete();
        return;
    }

    char *inp = new char [sz+1];
    ibuf.Read(inp,sz);

    ///- Discard data after line break or line feed
    bool gotenter=false;
    unsigned int y=0;
    for(;y<sz;y++)
    {
        if (inp[y]=='\r'||inp[y]=='\n')
        {
            gotenter=true;
            break;
        }
    }

    //No buffer overflow (checked above)
    memcpy(&buff[iInputLength],inp,y);
    iInputLength+=y;
    delete [] inp;
    if (gotenter)
    {
        buff[iInputLength]=0;
        iInputLength=0;
        switch(stage)
        {
            /// <ul> <li> If the input is 'USER <username>'
            case NONE:
            {
                ///- If we're interactive we don't expect "USER " to be there
                szLogin=&buff[0];

                ///- Get the gmlevel from the account table
                std::string login = szLogin;

                ///- Convert Account name to Upper Format
                AccountMgr::normalizeString(login);

                ///- Escape the Login to allow quotes in names
                loginDatabase.escape_string(login);

                QueryResult* result = loginDatabase.PQuery("SELECT gmlevel FROM account WHERE username = '%s'",login.c_str());

                ///- If the user is not found, deny access
                if(!result)
                {
                    Sendf("-No such user.\r\n");
                    sLog.outRALog("User %s does not exist.\n",szLogin.c_str());
                    if(bSecure)
                        SetCloseAndDelete();
                    Sendf("\r\n%s",sObjectMgr.GetMangosStringForDBCLocale(LANG_RA_USER));
                }
                else
                {
                    Field *fields = result->Fetch();

                    ///- if gmlevel is too low, deny access
                    if (fields[0].GetUInt32()<iMinLevel)
                    {
                        Sendf("-Not enough privileges.\r\n");
                        sLog.outRALog("User %s has no privilege.\n",szLogin.c_str());
                        if(bSecure)
                            SetCloseAndDelete();
                        Sendf("\r\n%s",sObjectMgr.GetMangosStringForDBCLocale(LANG_RA_USER));
                    }
                    else
                    {
                        stage=LG;
                        Sendf(sObjectMgr.GetMangosStringForDBCLocale(LANG_RA_PASS));
                    }
                    delete result;
                }
                break;
            }
            ///<li> If the input is 'PASS <password>' (and the user already gave his username)
            case LG:
            {                                               //login+pass ok
                ///- If password is correct, increment the number of active administrators
                std::string login = szLogin;

                ///- If we're interactive we don't expect "PASS " to be there
                std::string pw = &buff[0];

                AccountMgr::normalizeString(login);
                AccountMgr::normalizeString(pw);
                loginDatabase.escape_string(login);
                loginDatabase.escape_string(pw);

                QueryResult *check = loginDatabase.PQuery(
                    "SELECT 1 FROM account WHERE username = '%s' AND sha_pass_hash=SHA1(CONCAT(username,':','%s'))",
                    login.c_str(), pw.c_str());

                if (check)
                {
                    delete check;
                    GetSocket();
                    stage=OK;

                    Sendf("+Logged in.\r\n");
                    sLog.outRALog("User %s has logged in.\n",szLogin.c_str());
                    Sendf("mangos>");
                }
                else
                {
                    ///- Else deny access
                    Sendf("-Wrong pass.\r\n");
                    sLog.outRALog("User %s has failed to log in.\n",szLogin.c_str());
                    if(bSecure)
                        SetCloseAndDelete();
                    Sendf("\r\n%s",sObjectMgr.GetMangosStringForDBCLocale(LANG_RA_PASS));
                }
                break;
            }
            ///<li> If user is logged, parse and execute the command
            case OK:
                if (strlen(buff))
                {
                    sLog.outRALog("Got '%s' cmd.\n",buff);
                    if (strncmp(buff,"quit",4)==0)
                        SetCloseAndDelete();
                    else
                    {
                        SetDeleteByHandler(false);
                        CliCommandHolder* cmd = new CliCommandHolder(this, buff, &RASocket::zprint, &RASocket::commandFinished);
                        sWorld.QueueCliCommand(cmd);
                        ++pendingCommands;
                    }
                }
                else
                    Sendf("mangos>");
                break;
            ///</ul>
        };

    }
}

/// Output function
void RASocket::zprint(void* callbackArg, const char * szText )
{
    if( !szText )
        return;

    unsigned int sz=strlen(szText);
    send(((RASocket*)callbackArg)->GetSocket(), szText, sz, 0);
}

void RASocket::commandFinished(void* callbackArg, bool success)
{
    RASocket* raSocket = (RASocket*)callbackArg;
    raSocket->Sendf("mangos>");
    uint64 remainingCommands = --raSocket->pendingCommands;

    if(remainingCommands == 0)
        raSocket->SetDeleteByHandler(true);
}

