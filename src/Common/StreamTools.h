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
#include <vector>
#include <string>

namespace Common {

class IInputStream;
class IOutputStream;

void read(IInputStream& in, void* data, size_t size);
void read(IInputStream& in, int8_t& value);
void read(IInputStream& in, int16_t& value);
void read(IInputStream& in, int32_t& value);
void read(IInputStream& in, int64_t& value);
void read(IInputStream& in, uint8_t& value);
void read(IInputStream& in, uint16_t& value);
void read(IInputStream& in, uint32_t& value);
void read(IInputStream& in, uint64_t& value);
void read(IInputStream& in, std::vector<uint8_t>& data, size_t size);
void read(IInputStream& in, std::string& data, size_t size);
void readVarint(IInputStream& in, uint8_t& value);
void readVarint(IInputStream& in, uint16_t& value);
void readVarint(IInputStream& in, uint32_t& value);
void readVarint(IInputStream& in, uint64_t& value);

void write(IOutputStream& out, const void* data, size_t size);
void write(IOutputStream& out, int8_t value);
void write(IOutputStream& out, int16_t value);
void write(IOutputStream& out, int32_t value);
void write(IOutputStream& out, int64_t value);
void write(IOutputStream& out, uint8_t value);
void write(IOutputStream& out, uint16_t value);
void write(IOutputStream& out, uint32_t value);
void write(IOutputStream& out, uint64_t value);
void write(IOutputStream& out, const std::vector<uint8_t>& data);
void write(IOutputStream& out, const std::string& data);
void writeVarint(IOutputStream& out, uint64_t value);

template<typename T> T read(IInputStream& in) {
  T value;
  read(in, value);
  return value;
}

template<typename T> T read(IInputStream& in, size_t size) {
  T value;
  read(in, value, size);
  return value;
}

template<typename T> T readVarint(IInputStream& in) {
  T value;
  readVarint(in, value);
  return value;
}

};
