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

#include <string>
#include <cstdint>

#include <Common/StringView.h>

namespace CryptoNote {

class ISerializer {
public:

  enum SerializerType {
    INPUT,
    OUTPUT
  };

  virtual ~ISerializer() {}

  virtual SerializerType type() const = 0;

  virtual bool beginObject(Common::StringView name) = 0;
  virtual void endObject() = 0;
  virtual bool beginArray(size_t& size, Common::StringView name) = 0;
  virtual void endArray() = 0;

  virtual bool operator()(uint8_t& value, Common::StringView name) = 0;
  virtual bool operator()(int16_t& value, Common::StringView name) = 0;
  virtual bool operator()(uint16_t& value, Common::StringView name) = 0;
  virtual bool operator()(int32_t& value, Common::StringView name) = 0;
  virtual bool operator()(uint32_t& value, Common::StringView name) = 0;
  virtual bool operator()(int64_t& value, Common::StringView name) = 0;
  virtual bool operator()(uint64_t& value, Common::StringView name) = 0;
  virtual bool operator()(double& value, Common::StringView name) = 0;
  virtual bool operator()(bool& value, Common::StringView name) = 0;
  virtual bool operator()(std::string& value, Common::StringView name) = 0;
  
  // read/write binary block
  virtual bool binary(void* value, size_t size, Common::StringView name) = 0;
  virtual bool binary(std::string& value, Common::StringView name) = 0;

  template<typename T>
  bool operator()(T& value, Common::StringView name);
};

template<typename T>
bool ISerializer::operator()(T& value, Common::StringView name) {
  return serialize(value, name, *this);
}

template<typename T>
bool serialize(T& value, Common::StringView name, ISerializer& serializer) {
  if (!serializer.beginObject(name)) {
    return false;
  }

  serialize(value, serializer);
  serializer.endObject();
  return true;
}

template<typename T>
void serialize(T& value, ISerializer& serializer) {
  value.serialize(serializer);
}

#ifdef __clang__
template<> inline
bool ISerializer::operator()(size_t& value, Common::StringView name) {
  return operator()(*reinterpret_cast<uint64_t*>(&value), name);
}
#endif

#define KV_MEMBER(member) s(member, #member);

}
