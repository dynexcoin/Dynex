// Copyright (c) 2021-2022, The TuringX Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Parts of this file are originally copyright (c) 2012-2016 The Cryptonote developers

#pragma once

#include <cstdint>

namespace CryptoNote {

const uint32_t PORTABLE_STORAGE_SIGNATUREA = 0x01011101;
const uint32_t PORTABLE_STORAGE_SIGNATUREB = 0x01020101; // bender's nightmare 
const uint8_t PORTABLE_STORAGE_FORMAT_VER = 1;

const uint8_t PORTABLE_RAW_SIZE_MARK_MASK = 0x03;
const uint8_t PORTABLE_RAW_SIZE_MARK_BYTE = 0;
const uint8_t PORTABLE_RAW_SIZE_MARK_WORD = 1;
const uint8_t PORTABLE_RAW_SIZE_MARK_DWORD = 2;
const uint8_t PORTABLE_RAW_SIZE_MARK_INT64 = 3;

#ifndef MAX_STRING_LEN_POSSIBLE       
#define MAX_STRING_LEN_POSSIBLE       2000000000 //do not let string be so big
#endif

//data types 

const uint8_t BIN_KV_SERIALIZE_TYPE_INT64 = 1;
const uint8_t BIN_KV_SERIALIZE_TYPE_INT32 = 2;
const uint8_t BIN_KV_SERIALIZE_TYPE_INT16 = 3;
const uint8_t BIN_KV_SERIALIZE_TYPE_INT8 = 4;
const uint8_t BIN_KV_SERIALIZE_TYPE_UINT64 = 5;
const uint8_t BIN_KV_SERIALIZE_TYPE_UINT32 = 6;
const uint8_t BIN_KV_SERIALIZE_TYPE_UINT16 = 7;
const uint8_t BIN_KV_SERIALIZE_TYPE_UINT8 = 8;
const uint8_t BIN_KV_SERIALIZE_TYPE_DOUBLE = 9;
const uint8_t BIN_KV_SERIALIZE_TYPE_STRING = 10;
const uint8_t BIN_KV_SERIALIZE_TYPE_BOOL = 11;
const uint8_t BIN_KV_SERIALIZE_TYPE_OBJECT = 12;
const uint8_t BIN_KV_SERIALIZE_TYPE_ARRAY = 13;
const uint8_t BIN_KV_SERIALIZE_FLAG_ARRAY = 0x80;

#pragma pack(push)
#pragma pack(1)
struct KVBinaryStorageBlockHeader
{
  uint32_t m_signature_a;
  uint32_t m_signature_b;
  uint8_t  m_ver;
};
#pragma pack(pop)


}
