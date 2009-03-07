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

#include "Auth/SARC4.h"
#include <openssl/sha.h>

SARC4::SARC4()
{
    EVP_CIPHER_CTX_init(&m_encryptctx);
    EVP_EncryptInit_ex(&m_encryptctx, EVP_rc4(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_set_key_length(&m_encryptctx, SHA_DIGEST_LENGTH);
    EVP_CIPHER_CTX_init(&m_decryptctx);
    EVP_DecryptInit_ex(&m_decryptctx, EVP_rc4(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_set_key_length(&m_decryptctx, SHA_DIGEST_LENGTH);
}

SARC4::~SARC4()
{
    EVP_CIPHER_CTX_cleanup(&m_encryptctx);
    EVP_CIPHER_CTX_cleanup(&m_decryptctx);
}

void SARC4::Init(uint8 *seed1, uint8 *seed2)
{
    EVP_EncryptInit_ex(&m_encryptctx, NULL, NULL, seed1, NULL);
    EVP_DecryptInit_ex(&m_decryptctx, NULL, NULL, seed2, NULL);
}

void SARC4::Encrypt(uint32 len, uint8 *data)
{
    int outlen = 0;
    EVP_EncryptUpdate(&m_encryptctx, data, &outlen, data, len);
    EVP_EncryptFinal_ex(&m_encryptctx, data, &outlen);
}

void SARC4::Decrypt(uint32 len, uint8 *data)
{
    int outlen = 0;
    EVP_DecryptUpdate(&m_decryptctx, data, &outlen, data, len);
    EVP_DecryptFinal_ex(&m_decryptctx, data, &outlen);
}
