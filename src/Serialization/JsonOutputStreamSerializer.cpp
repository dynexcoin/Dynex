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

#include "JsonOutputStreamSerializer.h"
#include <cassert>
#include <stdexcept>
#include "Common/StringTools.h"

using Common::JsonValue;
using namespace CryptoNote;

namespace CryptoNote {
std::ostream& operator<<(std::ostream& out, const JsonOutputStreamSerializer& enumerator) {
  out << enumerator.root;
  return out;
}
}

namespace {

template <typename T>
void insertOrPush(JsonValue& js, Common::StringView name, const T& value) {
  if (js.isArray()) {
    js.pushBack(JsonValue(value));
  } else {
    js.insert(std::string(name), JsonValue(value));
  }
}

}

JsonOutputStreamSerializer::JsonOutputStreamSerializer() : root(JsonValue::OBJECT) {
  chain.push_back(&root);
}

JsonOutputStreamSerializer::~JsonOutputStreamSerializer() {
}

ISerializer::SerializerType JsonOutputStreamSerializer::type() const {
  return ISerializer::OUTPUT;
}

bool JsonOutputStreamSerializer::beginObject(Common::StringView name) {
  JsonValue& parent = *chain.back();
  JsonValue obj(JsonValue::OBJECT);

  if (parent.isObject()) {
    chain.push_back(&parent.insert(std::string(name), obj));
  } else {
    chain.push_back(&parent.pushBack(obj));
  }

  return true;
}

void JsonOutputStreamSerializer::endObject() {
  assert(!chain.empty());
  chain.pop_back();
}

bool JsonOutputStreamSerializer::beginArray(size_t& size, Common::StringView name) {
  JsonValue val(JsonValue::ARRAY);
  JsonValue& res = chain.back()->insert(std::string(name), val);
  chain.push_back(&res);
  return true;
}

void JsonOutputStreamSerializer::endArray() {
  assert(!chain.empty());
  chain.pop_back();
}

bool JsonOutputStreamSerializer::operator()(uint64_t& value, Common::StringView name) {
  int64_t v = static_cast<int64_t>(value);
  return operator()(v, name);
}

bool JsonOutputStreamSerializer::operator()(uint16_t& value, Common::StringView name) {
  uint64_t v = static_cast<uint64_t>(value);
  return operator()(v, name);
}

bool JsonOutputStreamSerializer::operator()(int16_t& value, Common::StringView name) {
  int64_t v = static_cast<int64_t>(value);
  return operator()(v, name);
}

bool JsonOutputStreamSerializer::operator()(uint32_t& value, Common::StringView name) {
  uint64_t v = static_cast<uint64_t>(value);
  return operator()(v, name);
}

bool JsonOutputStreamSerializer::operator()(int32_t& value, Common::StringView name) {
  int64_t v = static_cast<int64_t>(value);
  return operator()(v, name);
}

bool JsonOutputStreamSerializer::operator()(int64_t& value, Common::StringView name) {
  insertOrPush(*chain.back(), name, value);
  return true;
}

bool JsonOutputStreamSerializer::operator()(double& value, Common::StringView name) {
  insertOrPush(*chain.back(), name, value);
  return true;
}

bool JsonOutputStreamSerializer::operator()(std::string& value, Common::StringView name) {
  insertOrPush(*chain.back(), name, value);
  return true;
}

bool JsonOutputStreamSerializer::operator()(uint8_t& value, Common::StringView name) {
  insertOrPush(*chain.back(), name, static_cast<int64_t>(value));
  return true;
}

bool JsonOutputStreamSerializer::operator()(bool& value, Common::StringView name) {
  insertOrPush(*chain.back(), name, value);
  return true;
}

bool JsonOutputStreamSerializer::binary(void* value, size_t size, Common::StringView name) {
  std::string hex = Common::toHex(value, size);
  return (*this)(hex, name);
}

bool JsonOutputStreamSerializer::binary(std::string& value, Common::StringView name) {
  return binary(const_cast<char*>(value.data()), value.size(), name);
}
