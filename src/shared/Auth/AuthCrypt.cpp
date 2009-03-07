/*
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "AuthCrypt.h"
#include "Hmac.h"

AuthCrypt::AuthCrypt()
{
    _initialized = false;
}

AuthCrypt::~AuthCrypt()
{

}

void AuthCrypt::Init(BigNumber *K)
{
    uint8 recvSeed[SEED_KEY_SIZE] = { 0x22, 0xBE, 0xE5, 0xCF, 0xBB, 0x07, 0x64, 0xD9, 0x00, 0x45, 0x1B, 0xD0, 0x24, 0xB8, 0xD5, 0x45 };
    HmacHash recvHash(SEED_KEY_SIZE, (uint8*)recvSeed);
    recvHash.UpdateBigNumber(K);
    recvHash.Finalize();

    uint8 sendSeed[SEED_KEY_SIZE] = { 0xF4, 0x66, 0x31, 0x59, 0xFC, 0x83, 0x6E, 0x31, 0x31, 0x02, 0x51, 0xD5, 0x44, 0x31, 0x67, 0x98 };
    HmacHash sendHash(SEED_KEY_SIZE, (uint8*)sendSeed);
    sendHash.UpdateBigNumber(K);
    sendHash.Finalize();

    _recvCrypt.Init(recvHash.GetDigest(), sendHash.GetDigest());
    _sendCrypt.Init(recvHash.GetDigest(), sendHash.GetDigest());

    uint8 emptyBuf1[1024];
    memset(emptyBuf1, 0, 1024);

    _sendCrypt.Encrypt(1024, (uint8*)emptyBuf1);
    _sendCrypt.Decrypt(1024, (uint8*)emptyBuf1);

    uint8 emptyBuf2[1024];
    memset(emptyBuf2, 0, 1024);

    _recvCrypt.Encrypt(1024, (uint8*)emptyBuf2);
    _recvCrypt.Decrypt(1024, (uint8*)emptyBuf2);

    _initialized = true;
}

void AuthCrypt::DecryptRecv(uint8 *data, size_t len)
{
    if (!_initialized)
        return;

    _recvCrypt.Decrypt(len, data);
}

void AuthCrypt::EncryptSend(uint8 *data, size_t len)
{
    if (!_initialized)
        return;

    _sendCrypt.Encrypt(len, data);
}
