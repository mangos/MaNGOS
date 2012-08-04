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

/** \file
  \ingroup realmd
  */

#ifndef _BUFFEREDSOCKET_H_
#define _BUFFEREDSOCKET_H_

#include <ace/Basic_Types.h>
#include <ace/Synch_Traits.h>
#include <ace/Svc_Handler.h>
#include <ace/SOCK_Stream.h>
#include <ace/Message_Block.h>
#include <ace/Basic_Types.h>

#include <string>

class BufferedSocket: public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>
{
    protected:
        typedef ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH> Base;

        virtual void OnRead(void) { }
        virtual void OnAccept(void) { }
        virtual void OnClose(void) { }

    public:
        BufferedSocket(void);
        virtual ~BufferedSocket(void);

        size_t recv_len(void) const;
        bool recv_soft(char *buf, size_t len);
        bool recv(char *buf, size_t len);
        void recv_skip(size_t len);

        bool send(const char *buf, size_t len);

        const std::string& get_remote_address(void) const;

        virtual int open(void *);

        void close_connection(void);

        virtual int handle_input(ACE_HANDLE = ACE_INVALID_HANDLE);
        virtual int handle_output(ACE_HANDLE = ACE_INVALID_HANDLE);

        virtual int handle_close(ACE_HANDLE = ACE_INVALID_HANDLE,
                ACE_Reactor_Mask = ACE_Event_Handler::ALL_EVENTS_MASK);

    private:
        ssize_t noblk_send(ACE_Message_Block &message_block);

    private:
        ACE_Message_Block input_buffer_;

    protected:
        std::string remote_address_;

};

#endif /* _BUFFEREDSOCKET_H_ */

