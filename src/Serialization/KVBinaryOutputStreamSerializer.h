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

#include <vector>
#include <Common/IOutputStream.h>
#include "ISerializer.h"
#include "MemoryStream.h"

namespace CryptoNote {

class KVBinaryOutputStreamSerializer : public ISerializer {
public:

  KVBinaryOutputStreamSerializer();
  virtual ~KVBinaryOutputStreamSerializer() {}

  void dump(Common::IOutputStream& target);

  virtual ISerializer::SerializerType type() const override;

  virtual bool beginObject(Common::StringView name) override;
  virtual void endObject() override;

  virtual bool beginArray(size_t& size, Common::StringView name) override;
  virtual void endArray() override;

  virtual bool operator()(uint8_t& value, Common::StringView name) override;
  virtual bool operator()(int16_t& value, Common::StringView name) override;
  virtual bool operator()(uint16_t& value, Common::StringView name) override;
  virtual bool operator()(int32_t& value, Common::StringView name) override;
  virtual bool operator()(uint32_t& value, Common::StringView name) override;
  virtual bool operator()(int64_t& value, Common::StringView name) override;
  virtual bool operator()(uint64_t& value, Common::StringView name) override;
  virtual bool operator()(double& value, Common::StringView name) override;
  virtual bool operator()(bool& value, Common::StringView name) override;
  virtual bool operator()(std::string& value, Common::StringView name) override;
  virtual bool binary(void* value, size_t size, Common::StringView name) override;
  virtual bool binary(std::string& value, Common::StringView name) override;

  template<typename T>
  bool operator()(T& value, Common::StringView name) {
    return ISerializer::operator()(value, name);
  }

private:

  void writeElementPrefix(uint8_t type, Common::StringView name);
  void checkArrayPreamble(uint8_t type);
  void updateState(uint8_t type);
  MemoryStream& stream();

  enum class State {
    Root,
    Object,
    ArrayPrefix,
    Array
  };

  struct Level {
    State state;
    std::string name;
    size_t count;

    Level(Common::StringView nm) :
      name(nm), state(State::Object), count(0) {}

    Level(Common::StringView nm, size_t arraySize) :
      name(nm), state(State::ArrayPrefix), count(arraySize) {}

    Level(Level&& rv) {
      state = rv.state;
      name = std::move(rv.name);
      count = rv.count;
    }

  };

  std::vector<MemoryStream> m_objectsStack;
  std::vector<Level> m_stack;
};

}