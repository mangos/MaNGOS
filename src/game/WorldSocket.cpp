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

#include <ace/Message_Block.h>
#include <ace/OS_NS_string.h>
#include <ace/OS_NS_unistd.h>
#include <ace/os_include/arpa/os_inet.h>
#include <ace/os_include/netinet/os_tcp.h>
#include <ace/os_include/sys/os_types.h>
#include <ace/os_include/sys/os_socket.h>
#include <ace/OS_NS_string.h>
#include <ace/Reactor.h>
#include <ace/Auto_Ptr.h>

#include "WorldSocket.h"
#include "Common.h"

#include "Util.h"
#include "World.h"
#include "WorldPacket.h"
#include "SharedDefines.h"
#include "ByteBuffer.h"
#include "Opcodes.h"
#include "Database/DatabaseEnv.h"
#include "Auth/Sha1.h"
#include "WorldSession.h"
#include "WorldSocketMgr.h"
#include "Log.h"
#include "DBCStores.h"

#if defined( __GNUC__ )
#pragma pack(1)
#else
#pragma pack(push,1)
#endif

struct ServerPktHeader
{
    /**
     * size is the length of the payload _plus_ the length of the opcode
     */
    ServerPktHeader(uint32 size, uint16 cmd) : size(size)
    {
        uint8 headerIndex=0;
        if(isLargePacket())
        {
            DEBUG_LOG("initializing large server to client packet. Size: %u, cmd: %u", size, cmd);
            header[headerIndex++] = 0x80|(0xFF &(size>>16));
        }
        header[headerIndex++] = 0xFF &(size>>8);
        header[headerIndex++] = 0xFF &size;

        header[headerIndex++] = 0xFF & cmd;
        header[headerIndex++] = 0xFF & (cmd>>8);
    }

    uint8 getHeaderLength()
    {
        // cmd = 2 bytes, size= 2||3bytes
        return 2+(isLargePacket()?3:2);
    }

    bool isLargePacket()
    {
        return size > 0x7FFF;
    }

    const uint32 size;
    uint8 header[5];
};

struct ClientPktHeader
{
    uint16 size;
    uint32 cmd;
};

#if defined( __GNUC__ )
#pragma pack()
#else
#pragma pack(pop)
#endif

WorldSocket::WorldSocket (void) :
WorldHandler (),
m_LastPingTime (ACE_Time_Value::zero),
m_OverSpeedPings (0),
m_Session (0),
m_RecvWPct (0),
m_RecvPct (),
m_Header (sizeof (ClientPktHeader)),
m_OutBuffer (0),
m_OutBufferSize (65536),
m_OutActive (false),
m_Seed (static_cast<uint32> (rand32 ()))
{
    reference_counting_policy ().value (ACE_Event_Handler::Reference_Counting_Policy::ENABLED);

    msg_queue()->high_water_mark(8*1024*1024);
    msg_queue()->low_water_mark(8*1024*1024);
}

WorldSocket::~WorldSocket (void)
{
    if (m_RecvWPct)
        delete m_RecvWPct;

    if (m_OutBuffer)
        m_OutBuffer->release ();

    closing_ = true;

    peer ().close ();
}

bool WorldSocket::IsClosed (void) const
{
    return closing_;
}

void WorldSocket::CloseSocket (void)
{
    {
        ACE_GUARD (LockType, Guard, m_OutBufferLock);

        if (closing_)
            return;

        closing_ = true;
        peer ().close_writer ();
    }

    {
        ACE_GUARD (LockType, Guard, m_SessionLock);

        m_Session = NULL;
    }
}

const std::string& WorldSocket::GetRemoteAddress (void) const
{
    return m_Address;
}

int WorldSocket::SendPacket (const WorldPacket& pct)
{
    ACE_GUARD_RETURN (LockType, Guard, m_OutBufferLock, -1);

    if (closing_)
        return -1;

    // Dump outgoing packet.
    sLog.outWorldPacketDump(uint32(get_handle()), pct.GetOpcode(), LookupOpcodeName(pct.GetOpcode()), &pct, false);

    ServerPktHeader header(pct.size()+2, pct.GetOpcode());
    m_Crypt.EncryptSend ((uint8*)header.header, header.getHeaderLength());

    if (m_OutBuffer->space () >= pct.size () + header.getHeaderLength() && msg_queue()->is_empty())
    {
        // Put the packet on the buffer.
        if (m_OutBuffer->copy ((char*) header.header, header.getHeaderLength()) == -1)
            MANGOS_ASSERT (false);

        if (!pct.empty ())
            if (m_OutBuffer->copy ((char*) pct.contents (), pct.size ()) == -1)
                MANGOS_ASSERT (false);
    }
    else
    {
        // Enqueue the packet.
        ACE_Message_Block* mb;

        ACE_NEW_RETURN(mb, ACE_Message_Block(pct.size () + header.getHeaderLength()), -1);

        mb->copy((char*) header.header, header.getHeaderLength());

        if (!pct.empty ())
            mb->copy((const char*)pct.contents(), pct.size ());

        if(msg_queue()->enqueue_tail(mb,(ACE_Time_Value*)&ACE_Time_Value::zero) == -1)
        {
            sLog.outError("WorldSocket::SendPacket enqueue_tail");
            mb->release();
            return -1;
        }
    }

    return 0;
}

long WorldSocket::AddReference (void)
{
    return static_cast<long> (add_reference ());
}

long WorldSocket::RemoveReference (void)
{
    return static_cast<long> (remove_reference ());
}

int WorldSocket::open (void *a)
{
    ACE_UNUSED_ARG (a);

    // Prevent double call to this func.
    if (m_OutBuffer)
        return -1;

    // This will also prevent the socket from being Updated
    // while we are initializing it.
    m_OutActive = true;

    // Hook for the manager.
    if (sWorldSocketMgr->OnSocketOpen (this) == -1)
        return -1;

    // Allocate the buffer.
    ACE_NEW_RETURN (m_OutBuffer, ACE_Message_Block (m_OutBufferSize), -1);

    // Store peer address.
    ACE_INET_Addr remote_addr;

    if (peer ().get_remote_addr (remote_addr) == -1)
    {
        sLog.outError ("WorldSocket::open: peer ().get_remote_addr errno = %s", ACE_OS::strerror (errno));
        return -1;
    }

    m_Address = remote_addr.get_host_addr ();

    // Send startup packet.
    WorldPacket packet (SMSG_AUTH_CHALLENGE, 40);
    packet << uint32(1);                                    // 1...31
    packet << m_Seed;

    BigNumber seed1;
    seed1.SetRand(16 * 8);
    packet.append(seed1.AsByteArray(16), 16);               // new encryption seeds

    BigNumber seed2;
    seed2.SetRand(16 * 8);
    packet.append(seed2.AsByteArray(16), 16);               // new encryption seeds

    if (SendPacket (packet) == -1)
        return -1;

    // Register with ACE Reactor
    if (reactor ()->register_handler(this, ACE_Event_Handler::READ_MASK | ACE_Event_Handler::WRITE_MASK) == -1)
    {
        sLog.outError ("WorldSocket::open: unable to register client handler errno = %s", ACE_OS::strerror (errno));
        return -1;
    }

    // reactor takes care of the socket from now on
    remove_reference ();

    return 0;
}

int WorldSocket::close (int)
{
    shutdown ();

    closing_ = true;

    remove_reference ();

    return 0;
}

int WorldSocket::handle_input (ACE_HANDLE)
{
    if (closing_)
        return -1;

    switch (handle_input_missing_data ())
    {
        case -1 :
        {
            if ((errno == EWOULDBLOCK) ||
                (errno == EAGAIN))
            {
                return Update ();                           // interesting line ,isn't it ?
            }

            DEBUG_LOG ("WorldSocket::handle_input: Peer error closing connection errno = %s", ACE_OS::strerror (errno));

            errno = ECONNRESET;
            return -1;
        }
        case 0:
        {
            DEBUG_LOG ("WorldSocket::handle_input: Peer has closed connection");

            errno = ECONNRESET;
            return -1;
        }
        case 1:
            return 1;
        default:
            return Update ();                               // another interesting line ;)
    }

    ACE_NOTREACHED(return -1);
}

int WorldSocket::handle_output (ACE_HANDLE)
{
    ACE_GUARD_RETURN (LockType, Guard, m_OutBufferLock, -1);

    if (closing_)
        return -1;

    const size_t send_len = m_OutBuffer->length ();

    if (send_len == 0)
        return handle_output_queue (Guard);

#ifdef MSG_NOSIGNAL
    ssize_t n = peer ().send (m_OutBuffer->rd_ptr (), send_len, MSG_NOSIGNAL);
#else
    ssize_t n = peer ().send (m_OutBuffer->rd_ptr (), send_len);
#endif // MSG_NOSIGNAL

    if (n == 0)
        return -1;
    else if (n == -1)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return schedule_wakeup_output (Guard);

        return -1;
    }
    else if (n < (ssize_t)send_len) //now n > 0
    {
        m_OutBuffer->rd_ptr (static_cast<size_t> (n));

        // move the data to the base of the buffer
        m_OutBuffer->crunch ();

        return schedule_wakeup_output (Guard);
    }
    else //now n == send_len
    {
        m_OutBuffer->reset ();

        return handle_output_queue (Guard);
    }

    ACE_NOTREACHED (return 0);
}

int WorldSocket::handle_output_queue (GuardType& g)
{
    if(msg_queue()->is_empty())
        return cancel_wakeup_output(g);

    ACE_Message_Block *mblk;

    if(msg_queue()->dequeue_head(mblk, (ACE_Time_Value*)&ACE_Time_Value::zero) == -1)
    {
        sLog.outError("WorldSocket::handle_output_queue dequeue_head");
        return -1;
    }

    const size_t send_len = mblk->length ();

#ifdef MSG_NOSIGNAL
    ssize_t n = peer ().send (mblk->rd_ptr (), send_len, MSG_NOSIGNAL);
#else
    ssize_t n = peer ().send (mblk->rd_ptr (), send_len);
#endif // MSG_NOSIGNAL

    if (n == 0)
    {
        mblk->release();

        return -1;
    }
    else if (n == -1)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            msg_queue()->enqueue_head(mblk, (ACE_Time_Value*) &ACE_Time_Value::zero);
            return schedule_wakeup_output (g);
        }

        mblk->release();
        return -1;
    }
    else if (n < (ssize_t)send_len) //now n > 0
    {
        mblk->rd_ptr (static_cast<size_t> (n));

        if (msg_queue()->enqueue_head(mblk, (ACE_Time_Value*) &ACE_Time_Value::zero) == -1)
        {
            sLog.outError("WorldSocket::handle_output_queue enqueue_head");
            mblk->release();
            return -1;
        }

        return schedule_wakeup_output (g);
    }
    else //now n == send_len
    {
        mblk->release();

        return msg_queue()->is_empty() ? cancel_wakeup_output(g) : ACE_Event_Handler::WRITE_MASK;
    }

    ACE_NOTREACHED(return -1);
}

int WorldSocket::handle_close (ACE_HANDLE h, ACE_Reactor_Mask)
{
    // Critical section
    {
        ACE_GUARD_RETURN (LockType, Guard, m_OutBufferLock, -1);

        closing_ = true;

        if (h == ACE_INVALID_HANDLE)
            peer ().close_writer ();
    }

    // Critical section
    {
        ACE_GUARD_RETURN (LockType, Guard, m_SessionLock, -1);

        m_Session = NULL;
    }

    reactor()->remove_handler(this, ACE_Event_Handler::DONT_CALL | ACE_Event_Handler::ALL_EVENTS_MASK);
    return 0;
}

int WorldSocket::Update (void)
{
    if (closing_)
        return -1;

    if (m_OutActive || (m_OutBuffer->length () == 0 && msg_queue()->is_empty()))
        return 0;

    int ret;
    do
        ret = handle_output (get_handle ());
    while( ret > 0 );

    return ret;
}

int WorldSocket::handle_input_header (void)
{
    MANGOS_ASSERT (m_RecvWPct == NULL);

    MANGOS_ASSERT (m_Header.length () == sizeof (ClientPktHeader));

    m_Crypt.DecryptRecv ((uint8*) m_Header.rd_ptr (), sizeof (ClientPktHeader));

    ClientPktHeader& header = *((ClientPktHeader*) m_Header.rd_ptr ());

    EndianConvertReverse(header.size);
    EndianConvert(header.cmd);

    if ((header.size < 4) || (header.size > 10240) || (header.cmd  > 10240))
    {
        sLog.outError ("WorldSocket::handle_input_header: client sent malformed packet size = %d , cmd = %d",
                       header.size, header.cmd);

        errno = EINVAL;
        return -1;
    }

    header.size -= 4;

    ACE_NEW_RETURN (m_RecvWPct, WorldPacket ((uint16) header.cmd, header.size), -1);

    if(header.size > 0)
    {
        m_RecvWPct->resize (header.size);
        m_RecvPct.base ((char*) m_RecvWPct->contents (), m_RecvWPct->size ());
    }
    else
    {
        MANGOS_ASSERT(m_RecvPct.space() == 0);
    }

    return 0;
}

int WorldSocket::handle_input_payload (void)
{
    // set errno properly here on error !!!
    // now have a header and payload

    MANGOS_ASSERT (m_RecvPct.space () == 0);
    MANGOS_ASSERT (m_Header.space () == 0);
    MANGOS_ASSERT (m_RecvWPct != NULL);

    const int ret = ProcessIncoming (m_RecvWPct);

    m_RecvPct.base (NULL, 0);
    m_RecvPct.reset ();
    m_RecvWPct = NULL;

    m_Header.reset ();

    if (ret == -1)
        errno = EINVAL;

    return ret;
}

int WorldSocket::handle_input_missing_data (void)
{
    char buf [4096];

    ACE_Data_Block db ( sizeof (buf),
                        ACE_Message_Block::MB_DATA,
                        buf,
                        0,
                        0,
                        ACE_Message_Block::DONT_DELETE,
                        0);

    ACE_Message_Block message_block(&db,
                                    ACE_Message_Block::DONT_DELETE,
                                    0);

    const size_t recv_size = message_block.space ();

    const ssize_t n = peer ().recv (message_block.wr_ptr (),
                                          recv_size);

    if (n <= 0)
        return (int)n;

    message_block.wr_ptr (n);

    while (message_block.length () > 0)
    {
        if (m_Header.space () > 0)
        {
            //need to receive the header
            const size_t to_header = (message_block.length () > m_Header.space () ? m_Header.space () : message_block.length ());
            m_Header.copy (message_block.rd_ptr (), to_header);
            message_block.rd_ptr (to_header);

            if (m_Header.space () > 0)
            {
                // Couldn't receive the whole header this time.
                MANGOS_ASSERT (message_block.length () == 0);
                errno = EWOULDBLOCK;
                return -1;
            }

            // We just received nice new header
            if (handle_input_header () == -1)
            {
                MANGOS_ASSERT ((errno != EWOULDBLOCK) && (errno != EAGAIN));
                return -1;
            }
        }

        // Its possible on some error situations that this happens
        // for example on closing when epoll receives more chunked data and stuff
        // hope this is not hack ,as proper m_RecvWPct is asserted around
        if (!m_RecvWPct)
        {
            sLog.outError ("Forcing close on input m_RecvWPct = NULL");
            errno = EINVAL;
            return -1;
        }

        // We have full read header, now check the data payload
        if (m_RecvPct.space () > 0)
        {
            //need more data in the payload
            const size_t to_data = (message_block.length () > m_RecvPct.space () ? m_RecvPct.space () : message_block.length ());
            m_RecvPct.copy (message_block.rd_ptr (), to_data);
            message_block.rd_ptr (to_data);

            if (m_RecvPct.space () > 0)
            {
                // Couldn't receive the whole data this time.
                MANGOS_ASSERT (message_block.length () == 0);
                errno = EWOULDBLOCK;
                return -1;
            }
        }

        //just received fresh new payload
        if (handle_input_payload () == -1)
        {
            MANGOS_ASSERT ((errno != EWOULDBLOCK) && (errno != EAGAIN));
            return -1;
        }
    }

    return size_t(n) == recv_size ? 1 : 2;
}

int WorldSocket::cancel_wakeup_output (GuardType& g)
{
    if (!m_OutActive)
        return 0;

    m_OutActive = false;

    g.release ();

    if (reactor ()->cancel_wakeup
        (this, ACE_Event_Handler::WRITE_MASK) == -1)
    {
        // would be good to store errno from reactor with errno guard
        sLog.outError ("WorldSocket::cancel_wakeup_output");
        return -1;
    }

    return 0;
}

int WorldSocket::schedule_wakeup_output (GuardType& g)
{
    if (m_OutActive)
        return 0;

    m_OutActive = true;

    g.release ();

    if (reactor ()->schedule_wakeup
        (this, ACE_Event_Handler::WRITE_MASK) == -1)
    {
        sLog.outError ("WorldSocket::schedule_wakeup_output");
        return -1;
    }

    return 0;
}

int WorldSocket::ProcessIncoming (WorldPacket* new_pct)
{
    MANGOS_ASSERT (new_pct);

    // manage memory ;)
    ACE_Auto_Ptr<WorldPacket> aptr (new_pct);

    const ACE_UINT16 opcode = new_pct->GetOpcode ();

    if (opcode >= NUM_MSG_TYPES)
    {
        sLog.outError( "SESSION: received nonexistent opcode 0x%.4X", opcode);
        return -1;
    }

    if (closing_)
        return -1;

    // Dump received packet.
    sLog.outWorldPacketDump(uint32(get_handle()), new_pct->GetOpcode(), LookupOpcodeName(new_pct->GetOpcode()), new_pct, true);

    try
    {
        switch(opcode)
        {
            case CMSG_PING:
                return HandlePing (*new_pct);
            case CMSG_AUTH_SESSION:
                if (m_Session)
                {
                    sLog.outError ("WorldSocket::ProcessIncoming: Player send CMSG_AUTH_SESSION again");
                    return -1;
                }

                return HandleAuthSession (*new_pct);
            case CMSG_KEEP_ALIVE:
                DEBUG_LOG ("CMSG_KEEP_ALIVE ,size: "SIZEFMTD" ", new_pct->size ());

                return 0;
            default:
            {
                ACE_GUARD_RETURN (LockType, Guard, m_SessionLock, -1);

                if (m_Session != NULL)
                {
                    // OK ,give the packet to WorldSession
                    aptr.release ();
                    // WARNING here we call it with locks held.
                    // Its possible to cause deadlock if QueuePacket calls back
                    m_Session->QueuePacket (new_pct);
                    return 0;
                }
                else
                {
                    sLog.outError ("WorldSocket::ProcessIncoming: Client not authed opcode = %u", uint32(opcode));
                    return -1;
                }
            }
        }
    }
    catch (ByteBufferException &)
    {
        sLog.outError("WorldSocket::ProcessIncoming ByteBufferException occured while parsing an instant handled packet (opcode: %u) from client %s, accountid=%i.",
                opcode, GetRemoteAddress().c_str(), m_Session?m_Session->GetAccountId():-1);
        if (sLog.HasLogLevelOrHigher(LOG_LVL_DEBUG))
        {
            sLog.outDebug("Dumping error-causing packet:");
            new_pct->hexlike();
        }

        if (sWorld.getConfig(CONFIG_BOOL_KICK_PLAYER_ON_BAD_PACKET))
        {
            DETAIL_LOG("Disconnecting session [account id %i / address %s] for badly formatted packet.",
                m_Session?m_Session->GetAccountId():-1, GetRemoteAddress().c_str());

            return -1;
        }
        else
            return 0;
    }

    ACE_NOTREACHED (return 0);
}

int WorldSocket::HandleAuthSession (WorldPacket& recvPacket)
{
    // NOTE: ATM the socket is singlethread, have this in mind ...
    uint8 digest[20];
    uint32 clientSeed, id, security;
    uint32 ClientBuild;
    uint8 expansion = 0;
    LocaleConstant locale;
    std::string account;
    Sha1Hash sha1;
    BigNumber v, s, g, N, K;
    WorldPacket packet;

    // Read the content of the packet
    recvPacket >> ClientBuild;
    recvPacket.read_skip<uint32>();
    recvPacket >> account;
    recvPacket.read_skip<uint32>();
    recvPacket >> clientSeed;
    recvPacket.read_skip<uint32>();
    recvPacket.read_skip<uint32>();
    recvPacket.read_skip<uint32>();
    recvPacket.read_skip<uint64>();
    recvPacket.read (digest, 20);

    DEBUG_LOG ("WorldSocket::HandleAuthSession: client build %u, account %s, clientseed %X",
                ClientBuild,
                account.c_str(),
                clientSeed);

    // Check the version of client trying to connect
    if(!IsAcceptableClientBuild(ClientBuild))
    {
        packet.Initialize (SMSG_AUTH_RESPONSE, 1);
        packet << uint8 (AUTH_VERSION_MISMATCH);

        SendPacket (packet);

        sLog.outError ("WorldSocket::HandleAuthSession: Sent Auth Response (version mismatch).");
        return -1;
    }

    // Get the account information from the realmd database
    std::string safe_account = account; // Duplicate, else will screw the SHA hash verification below
    LoginDatabase.escape_string (safe_account);
    // No SQL injection, username escaped.

    QueryResult *result =
          LoginDatabase.PQuery ("SELECT "
                                "id, "                      //0
                                "gmlevel, "                 //1
                                "sessionkey, "              //2
                                "last_ip, "                 //3
                                "locked, "                  //4
                                "v, "                       //5
                                "s, "                       //6
                                "expansion, "               //7
                                "mutetime, "                //8
                                "locale "                   //9
                                "FROM account "
                                "WHERE username = '%s'",
                                safe_account.c_str ());

    // Stop if the account is not found
    if (!result)
    {
        packet.Initialize (SMSG_AUTH_RESPONSE, 1);
        packet << uint8 (AUTH_UNKNOWN_ACCOUNT);

        SendPacket (packet);

        sLog.outError ("WorldSocket::HandleAuthSession: Sent Auth Response (unknown account).");
        return -1;
    }

    Field* fields = result->Fetch ();

    expansion = ((sWorld.getConfig(CONFIG_UINT32_EXPANSION) > fields[7].GetUInt8()) ? fields[7].GetUInt8() : sWorld.getConfig(CONFIG_UINT32_EXPANSION));

    N.SetHexStr ("894B645E89E1535BBDAD5B8B290650530801B18EBFBF5E8FAB3C82872A3E9BB7");
    g.SetDword (7);

    v.SetHexStr(fields[5].GetString());
    s.SetHexStr (fields[6].GetString());
    m_s = s;

    const char* sStr = s.AsHexStr ();                       //Must be freed by OPENSSL_free()
    const char* vStr = v.AsHexStr ();                       //Must be freed by OPENSSL_free()

    DEBUG_LOG ("WorldSocket::HandleAuthSession: (s,v) check s: %s v: %s",
                sStr,
                vStr);

    OPENSSL_free ((void*) sStr);
    OPENSSL_free ((void*) vStr);

    ///- Re-check ip locking (same check as in realmd).
    if (fields[4].GetUInt8 () == 1) // if ip is locked
    {
        if (strcmp (fields[3].GetString (), GetRemoteAddress ().c_str ()))
        {
            packet.Initialize (SMSG_AUTH_RESPONSE, 1);
            packet << uint8 (AUTH_FAILED);
            SendPacket (packet);

            delete result;
            BASIC_LOG("WorldSocket::HandleAuthSession: Sent Auth Response (Account IP differs).");
            return -1;
        }
    }

    id = fields[0].GetUInt32 ();
    security = fields[1].GetUInt16 ();
    if(security > SEC_ADMINISTRATOR)                        // prevent invalid security settings in DB
        security = SEC_ADMINISTRATOR;

    K.SetHexStr (fields[2].GetString ());

    time_t mutetime = time_t (fields[8].GetUInt64 ());

    locale = LocaleConstant (fields[9].GetUInt8 ());
    if (locale >= MAX_LOCALE)
        locale = LOCALE_enUS;

    delete result;

    // Re-check account ban (same check as in realmd)
    QueryResult *banresult =
          LoginDatabase.PQuery ("SELECT 1 FROM account_banned WHERE id = %u AND active = 1 AND (unbandate > UNIX_TIMESTAMP() OR unbandate = bandate)"
                                "UNION "
                                "SELECT 1 FROM ip_banned WHERE (unbandate = bandate OR unbandate > UNIX_TIMESTAMP()) AND ip = '%s'",
                                id, GetRemoteAddress().c_str());

    if (banresult) // if account banned
    {
        packet.Initialize (SMSG_AUTH_RESPONSE, 1);
        packet << uint8 (AUTH_BANNED);
        SendPacket (packet);

        delete banresult;

        sLog.outError ("WorldSocket::HandleAuthSession: Sent Auth Response (Account banned).");
        return -1;
    }

    // Check locked state for server
    AccountTypes allowedAccountType = sWorld.GetPlayerSecurityLimit ();

    if (allowedAccountType > SEC_PLAYER && AccountTypes(security) < allowedAccountType)
    {
        WorldPacket Packet (SMSG_AUTH_RESPONSE, 1);
        Packet << uint8 (AUTH_UNAVAILABLE);

        SendPacket (packet);

        BASIC_LOG("WorldSocket::HandleAuthSession: User tries to login but his security level is not enough");
        return -1;
    }

    // Check that Key and account name are the same on client and server
    Sha1Hash sha;

    uint32 t = 0;
    uint32 seed = m_Seed;

    sha.UpdateData (account);
    sha.UpdateData ((uint8 *) & t, 4);
    sha.UpdateData ((uint8 *) & clientSeed, 4);
    sha.UpdateData ((uint8 *) & seed, 4);
    sha.UpdateBigNumbers (&K, NULL);
    sha.Finalize ();

    if (memcmp (sha.GetDigest (), digest, 20))
    {
        packet.Initialize (SMSG_AUTH_RESPONSE, 1);
        packet << uint8 (AUTH_FAILED);

        SendPacket (packet);

        sLog.outError ("WorldSocket::HandleAuthSession: Sent Auth Response (authentification failed).");
        return -1;
    }

    std::string address = GetRemoteAddress ();

    DEBUG_LOG ("WorldSocket::HandleAuthSession: Client '%s' authenticated successfully from %s.",
                account.c_str (),
                address.c_str ());

    // Update the last_ip in the database
    // No SQL injection, username escaped.
    static SqlStatementID updAccount;

    SqlStatement stmt = LoginDatabase.CreateStatement(updAccount, "UPDATE account SET last_ip = ? WHERE username = ?");
    stmt.PExecute(address.c_str(), account.c_str());

    // NOTE ATM the socket is single-threaded, have this in mind ...
    ACE_NEW_RETURN (m_Session, WorldSession (id, this, AccountTypes(security), expansion, mutetime, locale), -1);

    m_Crypt.Init(&K);

    m_Session->LoadGlobalAccountData();
    m_Session->LoadTutorialsData();
    m_Session->ReadAddonsInfo(recvPacket);

    // In case needed sometime the second arg is in microseconds 1 000 000 = 1 sec
    ACE_OS::sleep (ACE_Time_Value (0, 10000));

    sWorld.AddSession (m_Session);

    return 0;
}

int WorldSocket::HandlePing (WorldPacket& recvPacket)
{
    uint32 ping;
    uint32 latency;

    // Get the ping packet content
    recvPacket >> ping;
    recvPacket >> latency;

    if (m_LastPingTime == ACE_Time_Value::zero)
        m_LastPingTime = ACE_OS::gettimeofday (); // for 1st ping
    else
    {
        ACE_Time_Value cur_time = ACE_OS::gettimeofday ();
        ACE_Time_Value diff_time (cur_time);
        diff_time -= m_LastPingTime;
        m_LastPingTime = cur_time;

        if (diff_time < ACE_Time_Value (27))
        {
            ++m_OverSpeedPings;

            uint32 max_count = sWorld.getConfig (CONFIG_UINT32_MAX_OVERSPEED_PINGS);

            if (max_count && m_OverSpeedPings > max_count)
            {
                ACE_GUARD_RETURN (LockType, Guard, m_SessionLock, -1);

                if (m_Session && m_Session->GetSecurity () == SEC_PLAYER)
                {
                    sLog.outError  ("WorldSocket::HandlePing: Player kicked for "
                                    "over-speed pings address = %s",
                                    GetRemoteAddress ().c_str ());

                    return -1;
                }
            }
        }
        else
            m_OverSpeedPings = 0;
    }

    // critical section
    {
        ACE_GUARD_RETURN (LockType, Guard, m_SessionLock, -1);

        if (m_Session)
            m_Session->SetLatency (latency);
        else
        {
            sLog.outError ("WorldSocket::HandlePing: peer sent CMSG_PING, "
                            "but is not authenticated or got recently kicked,"
                            " address = %s",
                            GetRemoteAddress ().c_str ());
             return -1;
        }
    }

    WorldPacket packet (SMSG_PONG, 4);
    packet << ping;
    return SendPacket (packet);
}
