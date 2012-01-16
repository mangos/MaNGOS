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

#include "PatchHandler.h"
#include "AuthCodes.h"
#include "Log.h"
#include "Common.h"

#include <ace/OS_NS_sys_socket.h>
#include <ace/OS_NS_dirent.h>
#include <ace/OS_NS_errno.h>
#include <ace/OS_NS_unistd.h>

#include <ace/os_include/netinet/os_tcp.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#if defined( __GNUC__ )
#pragma pack(1)
#else
#pragma pack(push,1)
#endif

struct Chunk
{
    ACE_UINT8 cmd;
    ACE_UINT16 data_size;
    ACE_UINT8 data[4096]; // 4096 - page size on most arch
};

#if defined( __GNUC__ )
#pragma pack()
#else
#pragma pack(pop)
#endif

PatchHandler::PatchHandler(ACE_HANDLE socket, ACE_HANDLE patch)
{
    reactor(NULL);
    set_handle(socket);
    patch_fd_ = patch;
}

PatchHandler::~PatchHandler()
{
    if(patch_fd_ != ACE_INVALID_HANDLE)
        ACE_OS::close(patch_fd_);
}

int PatchHandler::open(void*)
{
    if(get_handle() == ACE_INVALID_HANDLE || patch_fd_ == ACE_INVALID_HANDLE)
        return -1;

    int nodelay = 0;
    if (-1 == peer().set_option(ACE_IPPROTO_TCP,
                TCP_NODELAY,
                &nodelay,
                sizeof(nodelay)))
    {
        return -1;
    }

#if defined(TCP_CORK)
    int cork = 1;
    if (-1 == peer().set_option(ACE_IPPROTO_TCP,
                TCP_CORK,
                &cork,
                sizeof(cork)))
    {
        return -1;
    }
#endif //TCP_CORK

    (void) peer().disable(ACE_NONBLOCK);

    return activate(THR_NEW_LWP | THR_DETACHED | THR_INHERIT_SCHED);
}

int PatchHandler::svc(void)
{
    // Do 1 second sleep, similar to the one in game/WorldSocket.cpp
    // Seems client have problems with too fast sends.
    ACE_OS::sleep(1);

    int flags = MSG_NOSIGNAL;

    Chunk data;
    data.cmd = CMD_XFER_DATA;

    ssize_t r;

    while((r = ACE_OS::read(patch_fd_, data.data, sizeof(data.data))) > 0)
    {
        data.data_size = (ACE_UINT16)r;

        if(peer().send((const char*)&data,
                    ((size_t) r) + sizeof(data) - sizeof(data.data),
                    flags) == -1)
        {
            return -1;
        }
    }

    if(r == -1)
    {
        return -1;
    }

    return 0;
}

PatchCache::~PatchCache()
{
    for (Patches::iterator i = patches_.begin (); i != patches_.end (); i++)
        delete i->second;
}

PatchCache::PatchCache()
{
    LoadPatchesInfo();
}

PatchCache* PatchCache::instance()
{
    return ACE_Singleton<PatchCache, ACE_Thread_Mutex>::instance();
}

void PatchCache::LoadPatchMD5(const char* szFileName)
{
    // Try to open the patch file
    std::string path = "./patches/";
    path += szFileName;
    FILE * pPatch = fopen(path.c_str (), "rb");
    sLog.outDebug("Loading patch info from %s", path.c_str());

    if(!pPatch)
        return;

    // Calculate the MD5 hash
    MD5_CTX ctx;
    MD5_Init(&ctx);

    const size_t check_chunk_size = 4*1024;

    ACE_UINT8 buf[check_chunk_size];

    while(!feof (pPatch))
    {
        size_t read = fread(buf, 1, check_chunk_size, pPatch);
        MD5_Update(&ctx, buf, read);
    }

    fclose(pPatch);

    // Store the result in the internal patch hash map
    patches_[path] = new PATCH_INFO;
    MD5_Final((ACE_UINT8 *) & patches_[path]->md5, &ctx);
}

bool PatchCache::GetHash(const char * pat, ACE_UINT8 mymd5[MD5_DIGEST_LENGTH])
{
    for (Patches::iterator i = patches_.begin (); i != patches_.end (); i++)
        if (!stricmp(pat, i->first.c_str ()))
        {
            memcpy(mymd5, i->second->md5, MD5_DIGEST_LENGTH);
            return true;
        }

    return false;
}

void PatchCache::LoadPatchesInfo()
{
    ACE_DIR* dirp = ACE_OS::opendir(ACE_TEXT("./patches/"));

    if(!dirp)
        return;

    ACE_DIRENT* dp;

    while((dp = ACE_OS::readdir(dirp)) != NULL)
    {
        int l = strlen(dp->d_name);
        if (l < 8)
            continue;

        if(!memcmp(&dp->d_name[l - 4], ".mpq", 4))
            LoadPatchMD5(dp->d_name);
    }

    ACE_OS::closedir(dirp);
}

